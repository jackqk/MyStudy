#include <stdafx.h>
#include "BaseMiniFlt.h"

namespace ddk::flt::miniflt
{
	//全过滤模版
	FLT_PREOP_CALLBACK_STATUS FsFltPreOperation(
		_Inout_ PFLT_CALLBACK_DATA Data,
		_In_    PCFLT_RELATED_OBJECTS FltObjects,
		_Out_   PVOID *CompletionContext) 
	{
		return BaseMiniFlt::getInstancePtr()->FsFltPreOperation(Data, FltObjects, CompletionContext);
	}
	FLT_POSTOP_CALLBACK_STATUS FsFltPostOperation(
		_Inout_ PFLT_CALLBACK_DATA Data,
		_In_ PCFLT_RELATED_OBJECTS FltObjects,
		_In_ PVOID CompletionContext OPTIONAL,
		_In_ FLT_POST_OPERATION_FLAGS Flags) 
	{
		return BaseMiniFlt::getInstancePtr()->FsFltPostOperation(Data, FltObjects, CompletionContext, Flags);
	}
	VOID FLTAPI FsContextCleanup(_In_ PFLT_CONTEXT Context, _In_ FLT_CONTEXT_TYPE ContextType)
	{
		BaseMiniFlt::getInstancePtr()->FsContextCleanup(Context, ContextType);
	}
	NTSTATUS FLTAPI FsFilterUnload(_In_ FLT_FILTER_UNLOAD_FLAGS Flags)
	{
		return BaseMiniFlt::getIntanceRef().FsFilterUnload(Flags);
	}
	NTSTATUS FLTAPI FsInstanceSetup(
		_In_ PCFLT_RELATED_OBJECTS FltObjects,
		_In_ FLT_INSTANCE_SETUP_FLAGS Flags,
		_In_ DEVICE_TYPE VolumeDeviceType,
		_In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType) 
	{
		return BaseMiniFlt::getIntanceRef().FsInstanceSetup(FltObjects, Flags, VolumeDeviceType, VolumeFilesystemType);
	}

	NTSTATUS FLTAPI FsInstanceQueryTeardown(
		_In_ PCFLT_RELATED_OBJECTS FltObjects,
		_In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags) 
	{
		return BaseMiniFlt::getInstancePtr()->FsInstanceQueryTeardown(FltObjects, Flags);
	}

	NTSTATUS FLTAPI FsFilterPortConnect(_In_ PFLT_PORT ClientPort,
		_In_opt_ PVOID ServerPortCookie,
		_In_reads_bytes_opt_(SizeOfContext) PVOID ConnectionContext,
		_In_ ULONG SizeOfContext,
		_Outptr_result_maybenull_ PVOID* ConnectionCookie) {
		/*if (ServerPortCookie)
		{
			auto p_class = reinterpret_cast<nt_miniflt_lpc*>(ServerPortCookie);
			return p_class->FsFilterPortConnect(ClientPort,
				ServerPortCookie,
				ConnectionContext,
				SizeOfContext,
				ConnectionCookie);
		}*/
		return STATUS_NOT_IMPLEMENTED;
	}

	VOID FLTAPI FsFilterPortDisconnect(_In_opt_ PVOID ConnectionCookie)
	{
		/*if (ConnectionCookie)
		{
			auto p_class = reinterpret_cast<nt_miniflt_lpc*>(ConnectionCookie);
			p_class->FsFilterPortDisconnect(ConnectionCookie);
		}*/
		return;
	}

	NTSTATUS FLTAPI FsFilterClientMessage(_In_ PVOID PortCookie,
		_In_opt_ PVOID InputBuffer,
		_In_ ULONG InputBufferLength,
		_Out_opt_ PVOID OutputBuffer,
		_In_ ULONG OutputBufferLength,
		_Out_ PULONG ReturnOutputBufferLength) {
		/*if (PortCookie)
		{
			auto p_class = reinterpret_cast<nt_miniflt_lpc*>(PortCookie);
			return p_class->FsFilterClientMessage(PortCookie,
				InputBuffer,
				InputBufferLength,
				OutputBuffer,
				OutputBufferLength,
				ReturnOutputBufferLength);
		}*/
		return STATUS_NOT_IMPLEMENTED;
	}
};