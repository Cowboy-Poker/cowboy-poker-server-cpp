#pragma once

/*-----------------
	Net Address
-----------------*/

class NetAddress {
public:
	NetAddress() = default;
	// 기본 생성자를 쓰겠다는 뜻
	// 나중에 = delete로 생성 금지, 다른 생성자 추가/정책 변경할 때 의도가 분명해짐
	NetAddress(SOCKADDR_IN sockAddr);
	NetAddress(wstring ip, uint16 port);
	// InetPtonA/W 함수를 사용해서 주소를 변환하기 위해 wstring 사용

	SOCKADDR_IN& GetSockAddr() { return _sockAddr; }
	wstring GetIpAddress();
	uint16 GetPort() { return ::ntohs(_sockAddr.sin_port); }

public:
	static IN_ADDR Ip2Address(const WCHAR* ip);
	// DNS을 IP주소로 환산하는 기능 등을 넣어줄 수 있음

private:
	SOCKADDR_IN _sockAddr = {};
};