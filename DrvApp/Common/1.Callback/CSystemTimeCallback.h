#pragma once
namespace ddk::callback {
	class CSystemTimeCallback {
	private:
		using pfnSystemTimeCallback = std::function<VOID(PVOID, PVOID)>;
		CExCallback m_powerCallback;
	public:
		CSystemTimeCallback() {
			m_powerCallback.Create(L"\\Callback\\SetSystemTime");
		}
		~CSystemTimeCallback() {

		}
		BOOL SetCallback(pfnSystemTimeCallback callback) {
			return m_powerCallback.SetCallback(callback);
		}
	};
}