#include "pch.h"
#include "ThreadManager.h"
#include "UdpGameSession.h"
#include "UdpService.h"
#include "UdpServerPacketHandler.h"

int main()
{
	string address = "0.0.0.0";
	std::wstring waddress(address.begin(), address.end());

	IocpCoreRef iocpCore = MakeShared<IocpCore>();

	// UdpService 생성 전에 람다에서 udpService를 캡처하면
	// (자기 초기식 안에서 자기 자신 참조) MSVC가 구문 오류·연쇄 오류를 낼 수 있음.
	UdpGameSessionRef session = MakeShared<UdpGameSession>();

	UdpServiceRef udpService = MakeShared<UdpService>(
		NetAddress(waddress, 7777),
		iocpCore,
		[session]() -> UdpSessionRef { return session; });

	session->SetService(udpService);

	ASSERT_CRASH(udpService->Start());

	for (int32 i = 0; i < 5; i++)
	{
		GThreadManager->Launch([=] {
			while (true)
				iocpCore->Dispatch();
		});
	}

	cout << "Cowboy Poker UDP Server Start\n";
	cout << "Address: " << address << " Port: 7777\n";

	while (true)
	{
		Vector<uint64> left = udpService->Tick();
		for (uint64 id : left)
			UdpServerPacketHandler::OnClientLeave(udpService, id);
		this_thread::sleep_for(1000ms);
	}

	GThreadManager->Join();
	return 0;
}
