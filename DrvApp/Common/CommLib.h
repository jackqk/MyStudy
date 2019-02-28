#pragma once
#define _CRT_ALLOCATION_DEFINED
#define NDIS61 1
#define NDIS_SUPPORT_NDIS61 1

#if defined(DBG)
#define LOG_BUILD 1//加上這個build速度很慢
#endif

inline bool IsX64() {
#ifdef _AMD64_
	return true;
#else
	return false;
#endif
}

//常用头文件
#ifdef __cplusplus
#include <initguid.h>
#include <fltKernel.h>
#include <Wdmsec.h>
#include <ntdef.h>
#include <ntimage.h>
#include <stdarg.h>
#include <ntstrsafe.h>
#include <ntdddisk.h>
#include <mountdev.h>
#include <ntddvol.h>
#include <intrin.h>
#include <Aux_klib.h>
#include <wdmguid.h>
#include <ntifs.h>
#include <fwpmk.h>
#include <fwpsk.h>
#include <ntddkbd.h>
#include <ntddscsi.h>
#include <srb.h>
#include <scsi.h>
#include <wsk.h>
#include <basetsd.h>
#endif

//常用CPP和STL库
#ifdef __cplusplus
#include "4.Cpp&Stl/stdcpp.h"
#include "4.Cpp&Stl/kernel_stl.h"
#include "4.Cpp&Stl/unique_resource.h"
#include "4.Cpp&Stl/scope_exit.h"
#include "4.Cpp&Stl/Singleton.h"
#include <tuple>
#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include <random>
#include <array>
#include <memory>
#include <list>
#include <deque>
#include <functional>
#include <regex>
#include <utility>
#include <memory>
#include <unordered_map>
#include <sstream>
#include <atomic>
#include <set>
#endif

/*****************打印调试信息宏**************************/
#if defined(DBG)
#define LOG_DEBUG(format, ...) \
	DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, (format), __VA_ARGS__)
#define LOG_INFO(format, ...) \
	DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, (format), __VA_ARGS__)
#define LOG_WARN(format, ...) \
	DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, (format), __VA_ARGS__)
#define LOG_ERROR(format, ...) \
	DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, (format), __VA_ARGS__)
#define LOG_DEBUG_SAFE(format, ...) \
	DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, (format), __VA_ARGS__)
#define LOG_INFO_SAFE(format, ...) \
	DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, (format), __VA_ARGS__)                                       
#define LOG_WARN_SAFE(format, ...) \
	DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, (format), __VA_ARGS__)                                         
#define LOG_ERROR_SAFE(format, ...) \
	DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, (format), __VA_ARGS__)
#else
#define LOG_DEBUG(format, ...)
#define LOG_DEBUG_SAFE(format, ...)   
#define LOG_INFO_SAFE(format, ...)                                         
#define LOG_WARN_SAFE(format, ...)                                         
#define LOG_ERROR_SAFE(format, ...)
#endif

//函数头定义
#include "4.win_head/ntos_func_def.h"
#include "4.win_head/win_head.h"

//log
#include "3.Log/StatusInfo.h"

//锁
#include "3.Synchronization/CMutex.h"
#include "3.Synchronization/event/CEvent.h"
#include "2.Timer/CTimer.h"
#include "3.Synchronization/event/CKevent.h"
#include "3.Synchronization/CFGLock.h"		//自造车轮锁
#include "3.Synchronization/CRWLock.h"
#include "3.Synchronization/CCpuLock.h"
#include "3.Synchronization/CPushLock.h"
#include "3.Synchronization/CSpinLock.h"
#include "3.Synchronization/CSpinLockMutex.h"
#include "3.Synchronization/CRundownProtect.h"

//utils
#include "3.Utils/DbgUtil.h"
#include "3.Utils/PEUtil.h"
#include "3.Utils/TimeUtil.h"
#include "3.Utils/GuidUtil.h"
#include "3.Utils/OsVerUtil.h"
#include "3.Utils/PathUtil.h"
#include "3.Utils/MemUtil.h"
#include "3.Utils/CommonUtil.h"
#include "3.Utils/SyscallUtil.h"

//lib
#include "2.Dpc/CDpc.h"
#include "2.Reg/CReg.h"
#include "2.File/CFile.h"
#include "2.Timer/CTimer.h"
#include "2.Process/CProcess.h"
#include "2.Thread/CThread.h"
#include "2.Thread/CThreadPlus.h"
#include "2.WorkItem/CWorkItem.h"

//network
#include "2.NetWork/Wsk/WskHeader.h"

//callback
#include "1.Callback/CProcessCallback.h"
#include "1.Callback/CThreadCallback.h"
#include "1.Callback/CRegCallback.h"
#include "1.Callback/CImageCallback.h"
#include "1.Callback/CExCallback.h"
#include "1.Callback/CObjectCallback.h"
#include "1.Callback/CPowerCallback.h"
#include "1.Callback/CSystemTimeCallback.h"
#include "1.Callback/CPnpCallback.h"

//其他
#include "1.Driver&Device/CDriver.h"
#include "1.Driver&Device/CDevice.h" 















