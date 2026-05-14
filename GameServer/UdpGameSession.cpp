#include "pch.h"
#include "UdpGameSession.h"
#include "UdpServerPacketHandler.h"

void UdpGameSession::OnRecvPacket(const SOCKADDR_IN& senderAddr, BYTE* buffer, int32 len)
{
    // _service는 weak_ptr<UdpService>.
    // weak_ptr은 shared_ptr과 달리 참조 카운트를 올리지 않아 순환 참조를 방지함.
    // (UdpService → UdpGameSession, UdpGameSession → UdpService 가 shared_ptr이면
    //  서로가 서로를 붙잡아 둘 다 소멸 불가 → 메모리 누수)
    // .lock()은 weak_ptr → shared_ptr 승격 시도.
    // 이미 UdpService가 소멸했으면 nullptr 반환, 살아있으면 shared_ptr 반환.
    // USE_LOCK(뮤텍스)과는 별개로 weak_ptr 자체 기능.
    auto service = _service.lock();
    if (service == nullptr)
        return;

    // 헤더 최소 크기 검사
    if (len < static_cast<int32>(sizeof(UdpPacketHeader)))
        return;

    UdpPacketHeader* header = reinterpret_cast<UdpPacketHeader*>(buffer);

    // 클라이언트 컨텍스트 조회 or 최초 핸드셰이크(sessionId == 0) 처리
    UdpClientContextRef ctx = nullptr;
    if (header->sessionId == 0)
    {
        // 새 클라이언트 — 컨텍스트 등록 후 sessionId 발급
        ctx = service->RegisterClient(senderAddr);
        if (ctx == nullptr)
            return;
    }
    else
    {
        ctx = service->FindByAddr(senderAddr);
        if (ctx == nullptr)
            return; // 등록되지 않은 주소 → 무시

        // 중복 패킷 필터링 (이미 처리한 sequence)
        if (header->sequence != 0 && header->sequence <= ctx->lastSeq)
            return;

        ctx->lastSeq     = header->sequence;
        ctx->lastAliveMs = static_cast<int64>(::GetTickCount64());
    }

    UdpServerPacketHandler::HandlePacket(service, ctx, buffer, len);
}
