#pragma once
//设备对象名
#define DEVICE_NAME L"\\Device\\TestDriverB"
#define LINK_NAME   L"\\Dosdevices\\TestDriverB"

//定义在DriverEntry.cpp中
extern wchar_t g_RegistryPath[MAX_PATH];		//驱动注册表位置
extern PDRIVER_OBJECT g_pDriverObject;			//驱动对象
extern PVOID g_pDrvImageBase;					//驱动模块在内存的基地址
extern SIZE_T g_DrvImageSize;					//驱动模块大小
extern PLIST_ENTRY g_KernelModuleList;			//可以遍历系统的驱动模,,当前指向自己（PKLDR_DATA_TABLE_ENTRY）
extern RTL_OSVERSIONINFOEXW g_WindowsVersion;	//当前系统版本信息
extern OS_INDEX g_OsIndex;						//版本号块
extern ddk::CDevice g_dev;					//nt_device全局变量


