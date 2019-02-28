#pragma once

namespace ddk::callback {
	class CPowerCallback {
	private:
		using pfnPowerCallback = std::function<VOID(PVOID, PVOID)>;
		CExCallback m_powerCallback;
	public:
		CPowerCallback() {
			m_powerCallback.Create(L"\\Callback\\PowerState");
		}
		~CPowerCallback() {

		}
		BOOL SetCallback(pfnPowerCallback callback) {
			m_powerCallback.SetCallback(callback);
		}
	};
}
