#include "pch.h"
#include <iostream>
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"
#include "GameSession.h"
#include "GameSessionManager.h"
#include "BufferWriter.h"
#include "ServerPacketHandler.h"

int main() {
	
	string address = "127.0.0.1";
	std::wstring waddress(address.begin(), address.end());

	ServerServiceRef service =
		MakeShared<ServerService>(NetAddress(waddress, 7777), MakeShared<IocpCore>(), MakeShared<GameSession>, 100);

	ASSERT_CRASH(service->Start());

	for (int32 i = 0; i < 5; i++) {
		GThreadManager->Launch([=] {
			while (true) {
				service->GetIocpCore()->Dispatch();
			}
		});
	}

	char sendData[] = "Hello World";
	cout << "C++ Server Start" << "\n";
	cout << "Address: " << address << "\n";

	while (true)
	{
		vector<BuffData> buffs{BuffData{100, 1.2f}, BuffData{200, 3.4}, BuffData{300, 1.9}};
		SendBufferRef sendBuffer = ServerPacketHandler::Make_S_TEST(1001, 100, 10, buffs);
		GSessionManager.Broadcast(sendBuffer);

		this_thread::sleep_for(250ms);
	}

	GThreadManager->Join();
	// CoreGlobal() 호출용 (사용 안 하면 컴파일러가 생성자 호출을 스킵함)
	return 0;
}

/*


#include "pch.h"
#include <iostream>
#include "CorePch.h"
#include <atomic>
#include <mutex>
#include <Windows.h>
#include <future>
#include "ThreadManager.h"
#include "SocketUtils.h"
#include "Listener.h"

int main() {
	Listener listener;
	listener.StartAccept(NetAddress(waddress, 7777));

	for (int32 i = 0; i < 5; i++) {
		GThreadManager->Launch([=] {
			while (true) {
				GIocpCore.Dispatch();
			}
			});
	}

	GThreadManager->Join();
	// CoreGlobal() 호출용 (사용 안 하면 컴파일러가 생성자 호출을 스킵함)

	return 0;
}
*/