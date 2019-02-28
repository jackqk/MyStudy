#pragma once

namespace ddk {
	class CProcess {
	private:
		HANDLE m_hProcess;
	public:
		CProcess() {

		}
		~CProcess() {

		}
	public:
		static NTSTATUS PspGetIrpProcess(PIRP pIrp, PEPROCESS &Process)
		{
			//通过IRP获得进程的EPROCESS
			Process = nullptr;
			auto pThread = pIrp->Tail.Overlay.Thread;
			if (!pThread)
			{
				Process = IoGetCurrentProcess();
			}
			else
			{
				Process = IoThreadToProcess(pThread);
			}
			if (Process)
			{
				return STATUS_SUCCESS;
			}
			return STATUS_UNSUCCESSFUL;
		}
	};
}