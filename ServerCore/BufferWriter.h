#pragma once
/*-------------------------
	BufferWriter
-------------------------*/

class BufferWriter {
public:
	BufferWriter();
	BufferWriter(BYTE* buffer, uint32 size, uint32 pos = 0);
	~BufferWriter();

	BYTE* Buffer() { return _buffer; }
	uint32 Size() { return _size; }
	uint32 WriteSize() { return _pos; }
	uint32 FreeSize() { return _size - _pos; }

	template <typename T>
	bool Write(T* src) {
		return Write(src, sizeof(T));
	}
	bool Write(void* src, uint32 len);

	// packetHeader를 마지막에 채울 수 있기 때문에 이를 편하게 하기 위해
	template <typename T>
	T* Reserve();

	template <typename T>
	BufferWriter& operator<<(T&& src);
	// <<로 데이터를 밀어넣는 연산자 만듬

private:
	BYTE* _buffer = nullptr;
	uint32 _size = 0;
	uint32 _pos = 0;
};

template <typename T>
T* BufferWriter::Reserve() {
	if (FreeSize() < sizeof(T))
		return nullptr;

	T* ret = reinterpret_cast<T*>(&_buffer[_pos]);
	_pos += sizeof(T);
	return ret;
}

template <typename T>
BufferWriter& BufferWriter::operator<<(T&& src) {
	using DataType = std::remove_cv_t<std::remove_reference_t<T>>;
	*reinterpret_cast<DataType*>(&_buffer[_pos]) = static_cast<DataType>(src);
	_pos += sizeof(DataType);
	return *this;
}