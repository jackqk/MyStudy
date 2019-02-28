#pragma once

namespace ddk::callback {
	//在DriverUnload手动调用下析构。
	class CThreadCallback
	{
	private:
		using pfnThreadCallback = std::function<VOID(HANDLE, HANDLE, BOOLEAN)>;
	private:
		pfnThreadCallback m_callback;
		BOOLEAN m_hasCallback = FALSE;	//是否注册成功
	private://隐藏单例的方法
		CThreadCallback() {
			NTSTATUS status = PsSetCreateThreadNotifyRoutine(ThreadCallback);
			if (NT_SUCCESS(status))
				m_hasCallback = TRUE;
		}
		CThreadCallback(CThreadCallback &) = delete;
		CThreadCallback& operator = (CThreadCallback) = delete;
	public://公开单例的方法
		~CThreadCallback() {
			if (m_hasCallback)
				PsRemoveCreateThreadNotifyRoutine(ThreadCallback);
		}
		static CThreadCallback &getIntanceRef() {
			static CThreadCallback self;
			return self;
		}
		static CThreadCallback *getInstancePtr() {
			return &getIntanceRef();
		}
		void RegCallback(pfnThreadCallback callback) {
			m_callback = callback;
		}
	public:
		static VOID ThreadCallback(IN HANDLE  ProcessId, IN HANDLE  ThreadId, IN BOOLEAN  Create) {
			CThreadCallback::getIntanceRef().m_callback(ProcessId, ThreadId, Create);
		}
	};
}