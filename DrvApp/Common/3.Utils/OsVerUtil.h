	#pragma once
typedef enum _OS_INDEX {
	OsIndex_UNK = 0,
	OsIndex_XP = 1,
	OsIndex_2K3 = 2,
	OsIndex_VISTA = 3,
	OsIndex_7 = 4,
	OsIndex_8 = 5,
	OsIndex_BLUE = 6,
	OsIndex_10_1507 = 7,
	OsIndex_10_1511 = 8,
	OsIndex_10_1607 = 9,
	OsIndex_10_1707 = 10,
	OsIndex_MAX = 11,
} OS_INDEX, *POS_INDEX;
extern "C" PSHORT NtBuildNumber;				//系统导出变量
extern RTL_OSVERSIONINFOEXW g_WindowsVersion;	//当前系统版本信息
extern OS_INDEX g_OsIndex;						//版本号块,当前指向自己（PKLDR_DATA_TABLE_ENTRY）

namespace ddk::util {
	class OsVerUtil {
	public://所有都是静态方法
		static void InitVersion()
		{
			g_OsIndex = GetWindowsIndex();
			g_WindowsVersion.dwOSVersionInfoSize = sizeof(g_WindowsVersion);
			RtlGetVersion(reinterpret_cast<RTL_OSVERSIONINFOW*>(&g_WindowsVersion));
		}
		static bool IsWin10()
		{
			return g_OsIndex > OsIndex_8;
		}
		static bool IsVistaOrGreater()
		{
			return g_OsIndex >= OsIndex_VISTA;
		}
		static bool IsWin7OrGreater()
		{
			return g_OsIndex >= OsIndex_7;
		}
		static bool IsWin8OrGreater()
		{
			return g_OsIndex >= OsIndex_8;
		}
	private:
		static OS_INDEX GetWindowsIndex()
		{
			if (*NtBuildNumber > 15063) // forever 10 =)
				return OsIndex_10_1707;
			switch (*NtBuildNumber)
			{
			case 2600:
				return OsIndex_XP;
				break;
			case 3790:
				return OsIndex_2K3;
				break;
			case 6000:
			case 6001:
			case 6002:
				return OsIndex_VISTA;
				break;
			case 7600:
			case 7601:
				return OsIndex_7;
				break;
			case 8102:
			case 8250:
			case 9200:
				return OsIndex_8;
			case 9431:
			case 9600:
				return OsIndex_BLUE;
				break;
			case 10240:
				return OsIndex_10_1507;
				break;
			case 10586:
				return OsIndex_10_1511;
				break;
			case 14393:
				return OsIndex_10_1607;
				break;
			case 15063:
				return OsIndex_10_1707;
				break;
			default:
				return OsIndex_UNK;
			}
		}
	};
}