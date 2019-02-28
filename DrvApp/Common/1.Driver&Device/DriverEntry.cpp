#include <stdafx.h>

//DriverMain、DriverUnload都在Main.cpp中
extern NTSTATUS DriverMain(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pRegistryPath);
extern VOID DriverUnload(PDRIVER_OBJECT pDriverObject);

//全局资源
wchar_t g_RegistryPath[MAX_PATH] = { 0 };
PDRIVER_OBJECT g_pDriverObject = nullptr;
PVOID g_pDrvImageBase = nullptr;
SIZE_T g_DrvImageSize = 0;
PLIST_ENTRY g_KernelModuleList = nullptr;
RTL_OSVERSIONINFOEXW g_WindowsVersion = {};
OS_INDEX g_OsIndex = OsIndex_UNK;
ddk::CDevice g_dev;

//真正Unload的地方
VOID Unload(PDRIVER_OBJECT pDriverObject)
{
	DriverUnload(pDriverObject);
	//沉睡3秒补上，因为有些回调，你析构了但还在执行，所以延迟三秒宇宙大法
	ddk::util::TimeUtil::sleep(ddk::util::TimeUtil::seconds(3));
	//调用全局、静态析构函数
	cc_doexit(0, 0, 0);
}

EXTERN_C NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT pDriverObject,
	IN PUNICODE_STRING pRegistryPath)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	pDriverObject->DriverUnload = Unload;
	//g_CDevice = new ddk::CDevice;

	//初始化版本信息
	ddk::util::OsVerUtil::InitVersion();
	if (g_OsIndex == OsIndex_UNK)
	{
		LOG_DEBUG("unknown system version\r\n");
		return STATUS_UNSUCCESSFUL;
	}
	//调用全局、静态构造函数
	cc_init(0);
	//初始化内存
	//===========暂未写==============
	

	if (pRegistryPath)
	{
		RtlSecureZeroMemory(g_RegistryPath, sizeof(g_RegistryPath));
		RtlStringCchPrintfW(g_RegistryPath, RTL_NUMBER_OF(g_RegistryPath), L"%wZ", pRegistryPath);
	}

	if (pDriverObject)
	{
		g_pDriverObject = pDriverObject;
		//不加这句话，当使用ObRegisterCallbacks会失败。Win10不一定有用；绕过MmVerifyCallbackFunction
		*(PULONG)((PCHAR)pDriverObject->DriverSection + 13 * sizeof(void*)) |= 0x20;
		PKLDR_DATA_TABLE_ENTRY entry = reinterpret_cast<PKLDR_DATA_TABLE_ENTRY>(pDriverObject->DriverSection);
		g_pDrvImageBase = entry->DllBase;
		g_DrvImageSize = entry->SizeOfImage;
		g_KernelModuleList = entry->InLoadOrderLinks.Flink;
	}
	
	//调用给定DriverMain
	status = DriverMain(pDriverObject, pRegistryPath);
	if (NT_SUCCESS(status))
		LOG_DEBUG("DriverImageBase= %p ImageSize=%x\r\n", g_pDrvImageBase, g_DrvImageSize);
	return status;
}



