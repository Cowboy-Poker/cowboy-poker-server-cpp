# Cowboy Poker Server

Cowboy Poker의 **로비·전투 실시간 동기화**를 담당하는 C++ 게임 서버입니다.

**실시간 트래픽은 UDP + IOCP**로 동작합니다. 로그인·방·포커 등 **신뢰성이 필요한 흐름은 별도 Node.js 서버에서 TCP**로 처리하는 구성을 가정합니다.

---

## 개요

| 항목 | 내용 |
|------|------|
| 언어 | C++ (Visual Studio 2022, `v143`) |
| 플랫폼 | Windows |
| 실시간 동기화 | **UDP** — `WSARecvFrom`(비동기, IOCP) / `WSASendTo`(동기) |
| 레거시 스택 | `ServerCore`에 **TCP** IOCP (`Session`, `Listener`, `ServerService`) 코드 유지 |
| 빌드 | `cowboy-poker-server-cpp.sln` → `ServerCore`(정적 라이브러리) → `GameServer`(실행 파일) |
| 기본 바인드 | `0.0.0.0:7777` (UDP) |

---

## 프로토콜 저장소

`protocols/`는 Git 서브모듈로 **[cowboy-poker-protocols](https://github.com/Cowboy-Poker/cowboy-poker-protocols)** 를 가리킵니다.

| 경로 | 용도 |
|------|------|
| `protocols/packetTypes/` | `packetTypes.h` / `PacketTypes.cs` / `packetTypes.js` — 패킷 ID 공유 |
| `protocols/lobby/` | 로비 UDP용 `.proto` (예: `C_UDP_HELLO`, `S_UDP_HELLO` 메시지 정의) |
| `protocols/common/` | `TransformInfo` 등 공통 메시지 |

C++ `GameServer`는 `pch.h`에서 `protocols/packetTypes/packetTypes.h`를 include 하거나, 동일 내용의 `GameServer/packetTypes.h`를 사용할 수 있습니다. ID 상수는 **`protocols/packetTypes/packetTypes.h`** 기준으로 맞춥니다.

---

## 프로젝트 구조

```
cowboy-poker-server-cpp/
├── ServerCore/                 # 네트워크 코어 (IOCP 공통 + TCP + UDP)
│   ├── IocpCore / IocpEvent    # Completion Port, OVERLAPPED 이벤트
│   ├── Session / PacketSession # TCP 세션 (비동기 송수신, 패킷 조립)
│   ├── Listener / Service      # TCP AcceptEx, ServerService
│   ├── UdpSession               # 단일 UDP 소켓, WSARecvFrom(IOCP)
│   ├── UdpService               # Bind, 클라 SessionId 맵, Broadcast, 타임아웃 Tick
│   ├── NetAddress, SocketUtils, SendBuffer, BufferReader/Writer, …
│   └── ThreadManager, Lock, Allocator, …
│
├── GameServer/
│   ├── GameServer.cpp           # 엔트리: UdpService + IOCP 워커 + Tick 루프
│   ├── UdpGameSession           # UdpSession 상속 → 수신 시 핸들러로 전달
│   ├── UdpServerPacketHandler   # UDP 패킷 디스패치 (현재 핸드셰이크 위주)
│   ├── GameSession / GameSessionManager / ServerPacketHandler  # TCP 샘플·레거시
│   └── pch.h                    # ServerCore 링크, 공통 헤더
│
└── protocols/                    # 서브모듈 (위 참조)
```

---

## UDP 아키텍처 (현재 엔트리)

```
┌─────────────────────────────────────────────────────────┐
│  GameServer::main()                                      │
│    UdpGameSession + UdpService 생성 → session.SetService│
│    UdpService::Start()  → Bind, IOCP 등록, RegisterRecv  │
│                                                          │
│  Worker × N                                              │
│    IocpCore::Dispatch()                                  │
│      → UdpSession::ProcessRecv()                         │
│           → UdpGameSession::OnRecvPacket()               │
│                → UdpServerPacketHandler::HandlePacket()  │
│                                                          │
│  Main thread                                             │
│    UdpService::Tick()  (세션 타임아웃 정리)               │
└─────────────────────────────────────────────────────────┘
```

- **Listener / Accept 없음** — UDP는 연결 없이 `SOCKADDR_IN`으로 송수신합니다.
- **클라이언트 식별**: `UdpService`가 `sessionId` ↔ 주소 맵을 유지합니다. 첫 패킷 `C_UDP_HELLO`(`sessionId == 0`) 시 등록 후 응답(패킷 타입 `S_UDP_HANDSHAKE_ACK`, 값 `350`, proto `lobby_response.S_UDP_HELLO`와 동일 역할)을 보냅니다.

### UDP 바이너리 헤더 (`UdpPacketHeader`)

```
[ sessionId: uint64 ][ sequence: uint32 ][ id: uint16 ][ payload ... ]
```

- `id`: `protocols/packetTypes/packetTypes.h`의 `PacketType` 값.
- 페이로드는 MTU를 고려해 약 **1200바이트 이하** 권장.

### TCP 패킷 (레거시 경로)

`PacketSession` 기준:

```
[ size: uint16 ][ id: uint16 ][ payload... ]
```

`GameServer` 기본 실행 경로는 UDP이며, TCP 스택은 **다른 진입점이나 추후 하이브리드**용으로 `ServerCore`에 남아 있습니다.

---

## 구현 상태

### 완료

- [x] IOCP + 단일 UDP 소켓 (`UdpSession` / `UdpService`)
- [x] `WSARecvFrom` 비동기 수신, 완료 시 `Dispatch` → 핸들러
- [x] `WSASendTo` 동기 송신 (데이터그램 단위, 구조 단순화)
- [x] 세션 타임아웃(`Tick`) 및 `sessionId` 발급·맵 조회
- [x] `UdpServerPacketHandler`: `C_UDP_HELLO` / `S_UDP_HANDSHAKE_ACK` 처리
- [x] 프로토콜 ID·로비 proto는 `protocols` 서브모듈과 동기화

### 예정 / 확장

- [ ] 로비 이동·하트비트 등 추가 UDP 패킷 처리 (`C_LOBBY_MOVE` 등)
- [ ] 전투 씬 UDP 패킷 및 서버 권위 검증
- [ ] 신뢰 채널(재전송)·암호화·스푸핑 방지
- [ ] Protobuf 직렬화와 C++ 핸들러 연동
- [ ] 운영 로그·메트릭

---

## 빌드 및 실행

### 요구 사항

- Windows 10 이상  
- Visual Studio 2022, Windows 10 SDK  
- `protocols` 서브모듈 초기화:  
  `git submodule update --init --recursive`

### 빌드

솔루션 `cowboy-poker-server-cpp.sln`을 연 뒤 **빌드** (먼저 `ServerCore`, 이어 `GameServer`).

### 실행

출력 디렉터리(예: `Binary\Debug\`)의 `GameServer.exe` 실행.

콘솔 예:

```
Cowboy Poker UDP Server Start
Address: 0.0.0.0 Port: 7777
```

방화벽에서 **UDP 7777** 인바운드를 허용해야 외부 클라이언트에서 접속할 수 있습니다.

---

## 의존성

외부 NuGet 등 없이 **Windows SDK + 링크 라이브러리**만 사용합니다.

| 라이브러리 | 용도 |
|-----------|------|
| `ws2_32.lib` | WinSock2 (`WSARecvFrom`, `WSASendTo`, 소켓) |
| `mswsock.lib` | (TCP 경로) `AcceptEx` / `ConnectEx` / `DisconnectEx` |

`GameServer`는 `pch.h`의 `#pragma comment`로 `ServerCore` 정적 라이브러리를 링크합니다.
