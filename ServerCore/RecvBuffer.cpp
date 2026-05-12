#include "pch.h"
#include "RecvBuffer.h"


/*------------------------
		RecvBuffer
------------------------*/

RecvBuffer::RecvBuffer(int32 bufferSize) : _bufferSize(bufferSize) {
	_capacity = bufferSize * BUFFER_COUNT;
	// 버퍼 사이즈를 BUFFER_COUNT배 키워서 읽기, 쓰기 커서가 동일한 위치일 확률을 높임
	_buffer.resize(_capacity);
}

RecvBuffer::~RecvBuffer() {}

void RecvBuffer::Clean() {
	int32 dataSize = DataSize();
	if (dataSize == 0) {
		// 마침 읽기, 쓰기 커서가 동일한 위치라면, 둘다 0으로 리셋
		_readPos = _writePos = 0;
	} else {
		// 여유 공간이 버퍼 1개 크기 미만이면
		if (FreeSize() < _bufferSize) {
			// 읽기 커서와 쓰기 커서 사이의 데이터와 함께 0으로 리셋
			::memcpy(&_buffer[0], &_buffer[_readPos], dataSize);
			_readPos = 0;
			_writePos = dataSize;
		}
	}
}

bool RecvBuffer::OnRead(int32 numOfBytes) {
	if (numOfBytes > DataSize())
		return false;

	_readPos += numOfBytes;
	return true;
}

bool RecvBuffer::OnWrite(int32 numOfBytes) {
	if (numOfBytes > FreeSize())
		return false;

	_writePos += numOfBytes;
	return true;
}
