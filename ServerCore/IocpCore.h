#pragma once

/*-----------------
	IocpObject
------------------*/
// Completion Port에는 소켓 뿐만 아니라 다양한 범위로 활용할 수 있음
// IocpObject는 Completion Port에 등록된 객체를 관리하는 클래스

class IocpObject : public enable_shared_from_this<IocpObject> {
public:
	virtual HANDLE GetHandle() abstract;
	virtual void Dispatch(class IocpEvent* iocpEvent, int32 numOfBytes = 0) abstract;
};

/*-----------------
	IocpCore
------------------*/

class IocpCore {
public:
	IocpCore();
	~IocpCore();

	HANDLE GetHandle() { return _iocpHandle; }

	bool Register(IocpObjectRef iocpObject);
	bool Dispatch(uint32 timeoutMs = INFINITE);

private:
	HANDLE _iocpHandle;
};
