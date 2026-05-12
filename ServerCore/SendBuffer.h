#pragma once

/*------------------
	SendBuffer
-------------------*/

class SendBuffer : public enable_shared_from_this<SendBuffer> {
public:
	SendBuffer(uint32 bufferSize);
	~SendBuffer();

	BYTE* Buffer() { return _buffer.data(); }
	uint32 AllocSize() { return static_cast<uint32>(_buffer.size()); }
	uint32 WriteSize() { return _writeSize; }
	uint32 Capacity() { return static_cast<uint32>(_buffer.size()); }

	void SetWriteSize(uint32 writeSize) { _writeSize = writeSize; }

private:
	Vector<BYTE> _buffer;
	uint32 _writeSize;
};
