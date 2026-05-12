#pragma once

/*-------------------------
	BufferReader
-------------------------*/

class BufferReader {
public:
	BufferReader();
	BufferReader(BYTE* buffer, uint32 size, uint32 pos = 0);
	~BufferReader();

	BYTE* Buffer() { return _buffer; }
	uint32 Size() { return _size; }
	uint32 ReadSize() { return _pos; }
	uint32 FreeSize() { return _size - _pos; }

	// 데이터를 옅보고 싶지만, 커서는 옮기고 싶지 않을 때
	template <typename T>
	bool Peek(T* dest) {
		return Peek(dest, sizeof(T));
	}
	bool Peek(void* dest, uint32 len);

	template <typename T>
	bool Read(T* dest) {
		return Read(dest, sizeof(T));
	}
	bool Read(void* dest, uint32 len);

	template <typename T>
	BufferReader& operator>>(OUT T& dest);
	// >>로 데이터를 꺼내쓰는 연산자 만듬

private:
	BYTE* _buffer = nullptr;
	uint32 _size = 0;
	uint32 _pos = 0;
};

template <typename T>
inline BufferReader& BufferReader::operator>>(OUT T& dest) {
	dest = *reinterpret_cast<T*>(&_buffer[_pos]);
	_pos += sizeof(T);
	return *this;
}