#include "pch.h"
#include "NetAddress.h"
#include "Types.h"

/*-----------------
	Net Address
-----------------*/

NetAddress::NetAddress(SOCKADDR_IN sockAddr) : _sockAddr(sockAddr) {};

NetAddress::NetAddress(wstring ip, uint16 port) {
	::memset(&_sockAddr, 0, sizeof(_sockAddr));
	_sockAddr.sin_family = AF_INET;
	_sockAddr.sin_addr = Ip2Address(ip.c_str());
	_sockAddr.sin_port = htons(port);
}

wstring NetAddress::GetIpAddress() {
	WCHAR buffer[100];
	::InetNtopW(AF_INET, &_sockAddr.sin_addr, buffer, len32(buffer));
	return wstring(buffer);
}

// wstring의 ip를 IN_ADDR로 변환하는 함수
IN_ADDR NetAddress::Ip2Address(const WCHAR* ip) {
	IN_ADDR addr;
	::InetPtonW(AF_INET, ip, &addr);
	return addr;
}
