#pragma once

extern PDRIVER_OBJECT g_pDriverObject;
namespace ddk::callback {
	//在DriverUnload手动调用下析构。
	class CRegCallback
	{
	private:
		using pfnRegCallback = std::function<NTSTATUS(PVOID, PVOID, PVOID)>;
		pfnRegCallback m_callback;
		LARGE_INTEGER m_cookie;
	private://隐藏单例的方法
		CRegCallback() {
			m_callback = NULL;
			m_cookie.QuadPart = 0;
		}
		CRegCallback(CRegCallback &) = delete;
		CRegCallback& operator = (CRegCallback) = delete;
	public://公开单例的方法
		~CRegCallback() {
			if (m_cookie.QuadPart)
				CmUnRegisterCallback(m_cookie);
		}
		static CRegCallback &getIntanceRef() {
			static CRegCallback self;
			return self;
		}
		static CRegCallback *getInstancePtr() {
			return &getIntanceRef();
		}
		void RegCallback(pfnRegCallback callback, WCHAR *alti = L"400000") {
			UNICODE_STRING usAltitude;
			NTSTATUS status;
			RtlInitUnicodeString(&usAltitude, alti);

			m_callback = callback;
			status = CmRegisterCallbackEx(
				RegistryCallback,
				&usAltitude,
				g_pDriverObject,
				this,
				&m_cookie,
				NULL);
			if (!NT_SUCCESS(status))
				ddk::log::StatusInfo::Print(status);
		}
	private://回调
		static NTSTATUS RegistryCallback(PVOID CallbackContext, PVOID Argument1,PVOID Argument2)
		{
			__try {
				return getIntanceRef().m_callback(CallbackContext, Argument1, Argument2);
			}
			__except (1) {

			}
		}
	};
}