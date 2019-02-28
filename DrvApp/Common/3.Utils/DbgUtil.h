#pragma once

namespace ddk::util {
	class DbgUtil
	{
	public:
		DbgUtil() {}
		~DbgUtil() {}
	public:
		// @desc：排除不感兴趣的进程，并下断
		// @prama doBreak: TRUE断下来，FALSE只打印
		static VOID FGBreakPoint(BOOL doBreak) {
			auto process_name = PsGetProcessImageFileName(IoGetCurrentProcess());
			BOOL isFind = FALSE;
			char *find[] = { "svchost.exe", "service.exe",  "TrustedInstall" , "conhost.exe" ,
							"explorer.exe" ,"SearchProtocol" , "SearchFilterHo", "WmiPrvSE.exe",
							"taskhost.exe", "taskmgr.exe", "dllhost.exe", "wuauclt.exe", "MpCmdRun.exe",
							"consent.exe", "services.exe", "wermgr.exe", "csrss.exe", "SearchIndexer.",
							"MpSigStub.exe", "CompatTelRunne", "sppsvc.exe", "makecab.exe", "mobsync.exe",
							"rundll32.exe", "DeviceDisplayO" };
			int length = sizeof(find) / sizeof(find[0]);
			for (int i = 0; i < length; i++)
			{
				if (!strcmp(find[i], process_name))
				{
					isFind = TRUE;
					break;
				}
			}
			if (!isFind)
			{
				if (doBreak)
					KdBreakPoint();
				else
					LOG_DEBUG("%s\r\n", process_name);
			}
		}
	};

}
