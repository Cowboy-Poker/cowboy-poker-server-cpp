#include "pch.h"
#include "SendBuffer.h"

/*------------------
	SendBuffer
-------------------*/

SendBuffer::SendBuffer(uint32 bufferSize) {
	_buffer.resize(bufferSize);
	_writeSize = 0;
}

SendBuffer::~SendBuffer() {}
