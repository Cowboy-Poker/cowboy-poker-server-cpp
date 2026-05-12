#include "pch.h"
#include "ServerPacketHandler.h"
#include "BufferReader.h"
#include "BufferWriter.h"

/*------------------------
	ServerPacketHandler
------------------------*/

void ServerPacketHandler::HandlePacket(BYTE* buffer, int32 len) {
	BufferReader br(buffer, len);

	PacketHeader header;
	br.Peek(&header);

	switch (header.id)
	{
	default:
			break;
	}
}

SendBufferRef ServerPacketHandler::Make_S_TEST(uint64 id, uint32 hp, uint16 attack, vector<BuffData> buffs) 
{
	SendBufferRef sendBuffer = MakeShared<SendBuffer>(4096);

	BufferWriter bw(sendBuffer->Buffer(), sendBuffer->AllocSize());

	// Reserve로 header가 들어갈 공간 미리 확보
	PacketHeader* header = bw.Reserve<PacketHeader>();

	// id(uint64), 체력(uint32), 공격력(uint16)
	bw << id << hp << attack;

	// 가변 데이터
	bw << static_cast<uint16>(buffs.size());

	for (BuffData& buff : buffs)
	{
		bw << buff.buffId << buff.remainTime;
	}

	sendBuffer->SetWriteSize(bw.WriteSize());

	header->size = bw.WriteSize();
	header->id = S_TEST;	 // 1: Test Msg

	return sendBuffer;
}
