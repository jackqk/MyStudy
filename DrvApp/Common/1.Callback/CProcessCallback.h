#pragma once

namespace ddk::callback {
	//在DriverUnload手动调用下析构。
	class CProcessCallback
	{
	private://函数指针
		using pfnPsSetCreateProcessNotifyRoutineEx = NTSTATUS(NTAPI*)(
			_In_ PCREATE_PROCESS_NOTIFY_ROUTINE_EX NotifyRoutine,
			_In_ BOOLEAN                           Remove);
		using pfnProcessCallback   = std::function<VOID(HANDLE, HANDLE, BOOLEAN)>;
		using pfnProcessCallbackEx = std::function<VOID(PEPROCESS, HANDLE, PPS_CREATE_NOTIFY_INFO)>;
	private://成员变量
		BOOL m_bIsCallbackEx;
		pfnProcessCallback   m_callback;
		pfnProcessCallbackEx m_callback_ex;
	private://隐藏单例的方法
		CProcessCallback() {
			using namespace ddk::util;
			PVOID funcAddr = CommonUtil::GetRoutineAddr(L"PsSetCreateProcessNotifyRoutineEx");
			if (funcAddr)
			{
				m_bIsCallbackEx = TRUE;
				pfnPsSetCreateProcessNotifyRoutineEx fun = (pfnPsSetCreateProcessNotifyRoutineEx)funcAddr;
				fun(ProcessCallbackEx, FALSE);
			}
			else
			{
				m_bIsCallbackEx = FALSE;
				PsSetCreateProcessNotifyRoutine(ProcessCallback, FALSE);
			}
		}
		CProcessCallback(CProcessCallback &) = delete;
		CProcessCallback& operator = (CProcessCallback) = delete;
	public://公开单例的方法
		~CProcessCallback() {
			if (m_bIsCallbackEx)
			{
				using namespace ddk::util;
				PVOID funcAddr = CommonUtil::GetRoutineAddr(L"PsSetCreateProcessNotifyRoutineEx");
				pfnPsSetCreateProcessNotifyRoutineEx fun = (pfnPsSetCreateProcessNotifyRoutineEx)funcAddr;
				fun(ProcessCallbackEx, TRUE);
			}
			else 
				PsSetCreateProcessNotifyRoutine(ProcessCallback, TRUE);
			
		}
		static CProcessCallback &getIntanceRef() {
			static CProcessCallback self;
			return self;
		}
		static CProcessCallback *getInstancePtr() {
			return &getIntanceRef();
		}
	public://注册回调
		void RegCallback(pfnProcessCallback callback) {
			m_callback = callback;
		}
		void RegCallbackEx(pfnProcessCallbackEx callbackex)
		{
			m_callback_ex = callbackex;
		}
	private://真实回调
		static VOID ProcessCallback(HANDLE ParentId, HANDLE ProcessId, BOOLEAN Create)
		{
			CProcessCallback::getIntanceRef().m_callback(ParentId, ProcessId, Create);
		}
		static VOID ProcessCallbackEx(PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo) {
			CProcessCallback::getIntanceRef().m_callback_ex(Process, ProcessId, CreateInfo);
		}
	};
}