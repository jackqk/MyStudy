#pragma once
#include <stdafx.h>

//声明的函数、变量
namespace ddk::flt::miniflt {
	//Pre操作
	FLT_PREOP_CALLBACK_STATUS FsFltPreOperation(
		_Inout_  PFLT_CALLBACK_DATA Data,
		_In_ PCFLT_RELATED_OBJECTS FltObjects,
		_Out_  PVOID *CompletionContext);
	//Post操作
	FLT_POSTOP_CALLBACK_STATUS FsFltPostOperation(
		_Inout_ PFLT_CALLBACK_DATA Data,
		_In_ PCFLT_RELATED_OBJECTS FltObjects,
		_In_ PVOID CompletionContext OPTIONAL,
		_In_ FLT_POST_OPERATION_FLAGS Flags);
	//U盘、硬盘插入时调用
	NTSTATUS FsInstanceSetup(
		_In_ PCFLT_RELATED_OBJECTS FltObjects,
		_In_ FLT_INSTANCE_SETUP_FLAGS Flags,
		_In_ DEVICE_TYPE VolumeDeviceType,
		_In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType);
	//U盘、硬盘拔出时调用
	NTSTATUS FsInstanceQueryTeardown(
		_In_ PCFLT_RELATED_OBJECTS FltObjects,
		_In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags);
	//minifilter卸载时调用
	NTSTATUS FsFilterUnload(_In_ FLT_FILTER_UNLOAD_FLAGS Flags);

	//上下文清理
	VOID FsContextCleanup(
		_In_ PFLT_CONTEXT Context,
		_In_ FLT_CONTEXT_TYPE ContextType
	);
	//context相关设置
	static const auto FS_CTX_INSTANCE_TAG = 'FSIC';
	static const auto FS_CTX_STREAM_TAG = 'FSSC';
	static const auto FS_CTX_STREAMHANDLE_TAG = 'FSHC';
	static const auto FS_CTX_SECTION_TAG = 'FSNC';
	static const auto FS_CTX_FILE_TAG = 'FSFC';
	//默认一个PAGE_SIZE比较完美
	static const auto FS_INSTANCE_CONTEXT_SIZE = PAGE_SIZE * 8;
	static const auto FS_SECTION_CONTEXT_SIZE = PAGE_SIZE;
	static const auto FS_STREAM_CONTEXT_SIZE = PAGE_SIZE;
	static const auto FS_STREAMHANDLE_CONTEXT_SIZE = PAGE_SIZE;
	static const auto FS_FILE_CONTEXT_SIZE = PAGE_SIZE;
	//FLT_REGISTRATION结构第四个成员
	static const FLT_CONTEXT_REGISTRATION FltContextRegistration[] =
	{
		{
			FLT_INSTANCE_CONTEXT,
			0,
			FsContextCleanup,
			FS_INSTANCE_CONTEXT_SIZE,
			FS_CTX_INSTANCE_TAG
		},
		{
			FLT_STREAM_CONTEXT,
			0,
			FsContextCleanup,
			FS_STREAM_CONTEXT_SIZE,
			FS_CTX_STREAM_TAG
		},
		{
			FLT_STREAMHANDLE_CONTEXT,
			0,
			FsContextCleanup,
			FS_STREAMHANDLE_CONTEXT_SIZE,
			FS_CTX_STREAMHANDLE_TAG
		},
		{
			FLT_FILE_CONTEXT,
			0,
			FsContextCleanup,
			FS_FILE_CONTEXT_SIZE,
			FS_CTX_FILE_TAG,
		},
#if WINVER >= _WIN32_WINNT_WIN8
		{
			FLT_SECTION_CONTEXT,
			0,
			FsContextCleanup,
			FS_SECTION_CONTEXT_SIZE,
			FS_CTX_SECTION_TAG
		},
#endif

		{ FLT_CONTEXT_END }
	};
	//FLT_REGISTRATION结构第五个成员
	static const FLT_OPERATION_REGISTRATION FltCallbacks[] =
	{
		{ 
			IRP_MJ_CREATE,
			0,
			FsFltPreOperation,
			FsFltPostOperation 
		},
		{ 
			IRP_MJ_CREATE_NAMED_PIPE,
			0,
			FsFltPreOperation,
			FsFltPostOperation 
		},
		{ 
			IRP_MJ_CLOSE,
			0,
			FsFltPreOperation,
			FsFltPostOperation 
		},
		{
			IRP_MJ_READ,
			0,
			FsFltPreOperation,
			FsFltPostOperation 
		},
		{
			IRP_MJ_WRITE,
			0,
			FsFltPreOperation,
			FsFltPostOperation 
		},
		{ 
			IRP_MJ_QUERY_INFORMATION,
			0,
			FsFltPreOperation,
			FsFltPostOperation
		},
		{
			IRP_MJ_SET_INFORMATION,
			0,
			FsFltPreOperation,
			FsFltPostOperation 
		},
		{ 
			IRP_MJ_QUERY_EA,
			0,
			FsFltPreOperation,
			FsFltPostOperation 
		},
		{ 
			IRP_MJ_SET_EA,
			0,
			FsFltPreOperation,
			FsFltPostOperation 
		},
		{ 
			IRP_MJ_FLUSH_BUFFERS,
			0,
			FsFltPreOperation,
			FsFltPostOperation 
		},
		{ 
			IRP_MJ_QUERY_VOLUME_INFORMATION,
			0,
			FsFltPreOperation,
			FsFltPostOperation 
		},
		{ 
			IRP_MJ_SET_VOLUME_INFORMATION,
			0,
			FsFltPreOperation,
			FsFltPostOperation 
		},
		{ 
			IRP_MJ_DIRECTORY_CONTROL,
			0,
			FsFltPreOperation,
			FsFltPostOperation 
		},
		{ 
			IRP_MJ_FILE_SYSTEM_CONTROL,
			0,
			FsFltPreOperation,
			FsFltPostOperation 
		},
		{ 
			IRP_MJ_DEVICE_CONTROL,
			0,
			FsFltPreOperation,
			FsFltPostOperation 
		},
		{ 
			IRP_MJ_INTERNAL_DEVICE_CONTROL,
			0,
			FsFltPreOperation,
			FsFltPostOperation 
		},
		{ 
			IRP_MJ_SHUTDOWN,
			0,
			FsFltPreOperation,
			NULL					//post operations not supported
		},                               
		{ 
			IRP_MJ_LOCK_CONTROL,
			0,
			FsFltPreOperation,
			FsFltPostOperation 
		},
		{ 
			IRP_MJ_CLEANUP,
			0,
			FsFltPreOperation,
			FsFltPostOperation 
		},
		{ 
			IRP_MJ_CREATE_MAILSLOT,
			0,
			FsFltPreOperation,
			FsFltPostOperation 
		},
		{ 
			IRP_MJ_QUERY_SECURITY,
			0,
			FsFltPreOperation,
			FsFltPostOperation 
		},
		{ 
			IRP_MJ_SET_SECURITY,
			0,
			FsFltPreOperation,
			FsFltPostOperation 
			},
		{ 
			IRP_MJ_QUERY_QUOTA,
			0,
			FsFltPreOperation,
			FsFltPostOperation 
		},
		{ 
			IRP_MJ_SET_QUOTA,
			0,
			FsFltPreOperation,
			FsFltPostOperation 
		},
		{ 
			IRP_MJ_PNP,
			0,
			FsFltPreOperation,
			FsFltPostOperation 
		},
		{ 
			IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION,
			0,
			FsFltPreOperation,
			FsFltPostOperation 
		},
		{ 
			IRP_MJ_RELEASE_FOR_SECTION_SYNCHRONIZATION,
			0,
			FsFltPreOperation,
			FsFltPostOperation 
		},
		{ 
			IRP_MJ_ACQUIRE_FOR_MOD_WRITE,
			0,
			FsFltPreOperation,
			FsFltPostOperation 
		},
		{ 
			IRP_MJ_RELEASE_FOR_MOD_WRITE,
			0,
			FsFltPreOperation,
			FsFltPostOperation 
		},
		{ 
			IRP_MJ_ACQUIRE_FOR_CC_FLUSH,
			0,
			FsFltPreOperation,
			FsFltPostOperation 
		},
		{ 
			IRP_MJ_RELEASE_FOR_CC_FLUSH,
			0,
			FsFltPreOperation,
			FsFltPostOperation 
		},
		{
			IRP_MJ_FAST_IO_CHECK_IF_POSSIBLE,
			0,
			FsFltPreOperation,
			FsFltPostOperation 
		},
		{ 
			IRP_MJ_NETWORK_QUERY_OPEN,
			0,
			FsFltPreOperation,
			FsFltPostOperation 
		},
		{ 
			IRP_MJ_MDL_READ,
			0,
			FsFltPreOperation,
			FsFltPostOperation 
		},
		{ 
			IRP_MJ_MDL_READ_COMPLETE,
			0,
			FsFltPreOperation,
			FsFltPostOperation 
		},
		{
			IRP_MJ_PREPARE_MDL_WRITE,
			0,
			FsFltPreOperation,
			FsFltPostOperation 
		},
		{ 
			IRP_MJ_MDL_WRITE_COMPLETE,
			0,
			FsFltPreOperation,
			FsFltPostOperation
		},
		{ 
			IRP_MJ_VOLUME_MOUNT,
			0,
			FsFltPreOperation,
			FsFltPostOperation 
		},
		{ 
			IRP_MJ_VOLUME_DISMOUNT,
			0,
			FsFltPreOperation,
			FsFltPostOperation 
		},
		{ 
			IRP_MJ_OPERATION_END 
		}
	};
	//FltRegisterFilter函数的第二个参数
	static const FLT_REGISTRATION FsFilterRegistration = {
			sizeof(FLT_REGISTRATION),	// Size
			FLT_REGISTRATION_VERSION,	// Version
			FLTFL_REGISTRATION_DO_NOT_SUPPORT_SERVICE_STOP,//不让系统自动处理，必须我们自己来搞定！！！FLTFL_REGISTRATION_DO_NOT_SUPPORT_SERVICE_STOP
			FltContextRegistration,		// Context
			FltCallbacks,				// Operation callbacks
			nullptr,					// FsFilterUnload:MiniFilterUnload 使用C++体系时，卸载过程是我们手工控制的！！！！
			FsInstanceSetup,			// InstanceSetup
			FsInstanceQueryTeardown,	// InstanceQueryTeardown
			NULL,						// InstanceTeardownStart
			NULL,						// InstanceTeardownComplete
			NULL,						// GenerateFileName
			NULL,						// GenerateDestinationFileName
			NULL						// NormalizeNameComponent
	};
}
//封装函数
namespace ddk::flt::miniflt {
	static NTSTATUS GetFileNameInformation(_In_ PFLT_CALLBACK_DATA Data, _Out_ PFLT_FILE_NAME_INFORMATION *FileNameInfo)
		/*++
		Routine Description:

		Get parsed file name information from filter callback data.

		Arguments:

		Data - Pointer to the filter callback Data that is passed to us.

		FileNameInfo - Pointer to pass out the parsed file name information.

		Return Value:

		The return value is the status of the operation.

		--*/
	{
		PFLT_FILE_NAME_INFORMATION nameInfo;
		NTSTATUS status = STATUS_SUCCESS;

		//
		//  The SL_OPEN_TARGET_DIRECTORY flag indicates the caller is attempting
		//  to open the target of a rename or hard link creation operation. We
		//  must clear this flag when asking fltmgr for the name or the result
		//  will not include the final component. We need the full path in order
		//  to compare the name to our mapping.
		//
		if (FlagOn(Data->Iopb->OperationFlags, SL_OPEN_TARGET_DIRECTORY)) {

			ClearFlag(Data->Iopb->OperationFlags, SL_OPEN_TARGET_DIRECTORY);

			//
			//  Get the filename as it appears below this filter. Note that we use 
			//  FLT_FILE_NAME_QUERY_FILESYSTEM_ONLY when querying the filename
			//  so that the filename as it appears below this filter does not end up
			//  in filter manager's name cache.
			//

			status = FltGetFileNameInformation(Data,
				FLT_FILE_NAME_OPENED | FLT_FILE_NAME_QUERY_FILESYSTEM_ONLY,
				&nameInfo);
			//
			//  Restore the SL_OPEN_TARGET_DIRECTORY flag so the create will proceed 
			//  for the target. The file systems depend on this flag being set in 
			//  the target create in order for the subsequent SET_INFORMATION 
			//  operation to proceed correctly.
			//
			SetFlag(Data->Iopb->OperationFlags, SL_OPEN_TARGET_DIRECTORY);
		}
		else
		{
			//
			//  Note that we use FLT_FILE_NAME_QUERY_DEFAULT when querying the 
			//  filename. In the precreate the filename should not be in filter
			//  manager's name cache so there is no point looking there.
			//
			status = FltGetFileNameInformation(Data,
				FLT_FILE_NAME_OPENED | FLT_FILE_NAME_QUERY_DEFAULT,
				&nameInfo);
		}

		if (!NT_SUCCESS(status)) {
			ddk::log::StatusInfo::Print(status);
			goto Exit;
		}

		status = FltParseFileNameInformation(nameInfo);

		if (!NT_SUCCESS(status)) {
			ddk::log::StatusInfo::Print(status);
			goto Exit;
		}

		*FileNameInfo = nameInfo;
	Exit:
		return status;
	}
	static LONG     FsMessageExceptionFilter(_In_ PEXCEPTION_POINTERS ExceptionPointer, _In_ BOOLEAN AccessingUserBuffer)
	{
		NTSTATUS Status = ExceptionPointer->ExceptionRecord->ExceptionCode;

		if (!FsRtlIsNtstatusExpected(Status) && !AccessingUserBuffer) {
			return EXCEPTION_CONTINUE_SEARCH;
		}

		return EXCEPTION_EXECUTE_HANDLER;
	}
	static NTSTATUS FsGetFileId64(_In_ PFLT_INSTANCE Instance, _In_ PFILE_OBJECT FileObject,_Out_ PFILE_INTERNAL_INFORMATION FileId)
	{
		return FltQueryInformationFile(
			Instance,
			FileObject,
			FileId,
			sizeof(FILE_INTERNAL_INFORMATION),
			FileInternalInformation,
			NULL);
	}
	static NTSTATUS FsDeleteFile(_In_ PFLT_INSTANCE Instance, _In_ PFILE_OBJECT FileObject)
	{
		FILE_DISPOSITION_INFORMATION fileDispositionInfo;

		fileDispositionInfo.DeleteFile = TRUE;

		return FltSetInformationFile(
			Instance,
			FileObject,
			&fileDispositionInfo,
			sizeof(fileDispositionInfo),
			FileDispositionInformation);
	}
	static NTSTATUS FsGetFileSize(_In_ PFLT_INSTANCE Instance, _In_ PFILE_OBJECT FileObject, _Out_ PLONGLONG FileSize)
	{
		NTSTATUS status;
		FILE_STANDARD_INFORMATION standardInfo;

		status = FltQueryInformationFile(
			Instance,
			FileObject,
			&standardInfo,
			sizeof(FILE_STANDARD_INFORMATION),
			FileStandardInformation,
			NULL);

		if (NT_SUCCESS(status))
			*FileSize = standardInfo.EndOfFile.QuadPart;

		return status;
	}
	static BOOLEAN  FsIsNameExpression(PUNICODE_STRING usName, const wchar_t *scFilter, BOOLEAN CaseInSens)
	{

		UNICODE_STRING nsFilter;
		RtlInitUnicodeString(&nsFilter, scFilter);
		return FsRtlIsNameInExpression(&nsFilter, usName, CaseInSens, nullptr);
	}
};
//Base类
namespace ddk::flt::miniflt {
	class BaseMiniFlt
	{
	private://定义函数类型
		using fnMinPreCallback  = std::function<FLT_PREOP_CALLBACK_STATUS(PFLT_CALLBACK_DATA, PCFLT_RELATED_OBJECTS, PVOID*)>;
		using fnMinPostCallback = std::function<FLT_POSTOP_CALLBACK_STATUS(PFLT_CALLBACK_DATA,
																		   PCFLT_RELATED_OBJECTS, 
																		   PVOID, 
																		   FLT_POST_OPERATION_FLAGS)>;
		using fnMinContextCleanupCallback = std::function<VOID(PFLT_CONTEXT, FLT_CONTEXT_TYPE)>;
		using fnMinUnload                 = std::function<void()>;
		using fnMinAttachVolumeCall       = std::function<NTSTATUS(PCFLT_RELATED_OBJECTS,
																   FLT_INSTANCE_SETUP_FLAGS,
																   DEVICE_TYPE,
																   FLT_FILESYSTEM_TYPE)>;
		using fnMinDetachVolume           = std::function<NTSTATUS(PCFLT_RELATED_OBJECTS, FLT_INSTANCE_QUERY_TEARDOWN_FLAGS)>;
	private:
		ddk::lock::CFGLock m_lock;
		bool m_isInit;
		PFLT_FILTER m_pFilter;
		fnMinUnload m_fnUnload;
		fnMinAttachVolumeCall m_fnInstanceSetup;
		fnMinDetachVolume     m_fnTearDown;
		fnMinPreCallback      m_fnAllPreCallback;
		std::map<UCHAR, fnMinPreCallback>  m_fnPreCallbackMaps;
		std::map<UCHAR, fnMinPostCallback> m_fnPostCallbackMaps;
		std::map<FLT_CONTEXT_TYPE, fnMinContextCleanupCallback> m_fnCleanupContextCallbackMaps;
		
	private://隐藏单例的方法
		BaseMiniFlt() {
			m_pFilter = nullptr;
			m_isInit = false;
			m_fnUnload = nullptr;
			m_fnInstanceSetup = nullptr;
			m_fnTearDown = nullptr;
			m_fnAllPreCallback = nullptr;
		}
		BaseMiniFlt(BaseMiniFlt &) = delete;
		BaseMiniFlt& operator = (BaseMiniFlt) = delete;
	public://公开单例的方法
		~BaseMiniFlt() {
			m_lock.wait_for_release();
			if (m_isInit)
			{
				FsFilterUnload(FLTFL_FILTER_UNLOAD_MANDATORY);
			}
		}
		static BaseMiniFlt &getIntanceRef() {
			static BaseMiniFlt self;
			return self;
		}
		static BaseMiniFlt *getInstancePtr() {
			return &getIntanceRef();
		}
	public://入口点
		bool initFlt(PDRIVER_OBJECT DriverObject)
		{
			if (m_isInit)
				return false;

			auto status = FltRegisterFilter(DriverObject, &FsFilterRegistration, &m_pFilter);
			if (!NT_SUCCESS(status))
			{
				ddk::log::StatusInfo::Print(status);
				return false;
			}

			status = FltStartFiltering(m_pFilter);
			if (!NT_SUCCESS(status))
			{
				ddk::log::StatusInfo::Print(status);
				FsFilterUnload(FLTFL_FILTER_UNLOAD_MANDATORY);
				return false;
			}
			m_isInit = true;
			return true;
		}
		PFLT_FILTER getFilter()
		{
			return m_pFilter;
		}
	public:
		void setCleanupContextCallback(FLT_CONTEXT_TYPE type, fnMinContextCleanupCallback _callback)
		{
			m_fnCleanupContextCallbackMaps[type] = _callback;
		}
		void setAllPreCallback(fnMinPreCallback _allcallback)
		{
			m_fnAllPreCallback = _allcallback;
		}
		void setPreCallback(UCHAR _maj, fnMinPreCallback _callback)
		{
			m_fnPreCallbackMaps[_maj] = _callback;
		}
		void setPostCallback(UCHAR _maj, fnMinPostCallback _callback)
		{
			m_fnPostCallbackMaps[_maj] = _callback;
		}
		void setInstanceSetupCallback(fnMinAttachVolumeCall _setup)
		{
			m_fnInstanceSetup = _setup;
		}
		void setTeardownCallback(fnMinDetachVolume _teardown)
		{
			m_fnTearDown = _teardown;
		}
		void setUnload(fnMinUnload unload_function)
		{
			m_fnUnload = unload_function;
		}
	public://context回调：在释放context之前调用，context数组中指定。
		void FLTAPI FsContextCleanup(
			_In_ PFLT_CONTEXT Context,
			_In_ FLT_CONTEXT_TYPE ContextType)
		{
			UNREFERENCED_PARAMETER(Context);
			UNREFERENCED_PARAMETER(ContextType);
			m_lock.only_acquire();
			if (m_fnCleanupContextCallbackMaps.find(ContextType) != m_fnCleanupContextCallbackMaps.end())
			{
				auto _func = m_fnCleanupContextCallbackMaps[ContextType];
				_func(Context, ContextType);
			}
			m_lock.release();
			return;
		}
	public://Pre、Post回调
		FLT_PREOP_CALLBACK_STATUS FsFltPreOperation(
			_Inout_ PFLT_CALLBACK_DATA Data,
			_In_    PCFLT_RELATED_OBJECTS FltObjects,
			_Out_   PVOID *CompletionContext)
		{
			m_lock.only_acquire();
			auto ret = FLT_PREOP_SUCCESS_WITH_CALLBACK;
			auto majorFun = Data->Iopb->MajorFunction;
			do {
				if (m_fnAllPreCallback)
				{
					ret = m_fnAllPreCallback(Data, FltObjects, CompletionContext);
					break;
				}
				if (m_fnPreCallbackMaps.find(majorFun) != m_fnPreCallbackMaps.end())
				{
					auto preCallback = m_fnPreCallbackMaps[majorFun];
					ret = preCallback(Data, FltObjects, CompletionContext);
				}
			} while (0);
			m_lock.release();
			return ret;
		}
		FLT_POSTOP_CALLBACK_STATUS FLTAPI FsFltPostOperation(
			_Inout_ PFLT_CALLBACK_DATA Data,
			_In_ PCFLT_RELATED_OBJECTS FltObjects,
			_In_ PVOID CompletionContext OPTIONAL,
			_In_ FLT_POST_OPERATION_FLAGS Flags)
		{
			m_lock.only_acquire();
			auto ret = FLT_POSTOP_FINISHED_PROCESSING;
			auto parameter = Data->Iopb;
			auto majorFun = parameter->MajorFunction;
			auto status = Data->IoStatus.Status;
			do {
				//都歇菜不需要继续了
				if (!NT_SUCCESS(status) || (status == STATUS_REPARSE))
					break;
				//开始
				if (m_fnPostCallbackMaps.find(majorFun) != m_fnPostCallbackMaps.end())
				{
					auto postCallback = m_fnPostCallbackMaps[majorFun];
					ret = postCallback(Data, FltObjects, CompletionContext, Flags);
				}
			} while (0);
			m_lock.release();
			return ret;
		}
	public://系统回调，如卸载，插拔
		NTSTATUS FsFilterUnload(_In_ FLT_FILTER_UNLOAD_FLAGS Flags)
		{
			KdBreakPoint();
			if (FlagOn(Flags, FLTFL_FILTER_UNLOAD_MANDATORY))
			{
				//调用下级注册的Unload
				if (m_fnUnload)
				{
					m_fnUnload();
				}
				if (m_pFilter)
				{
					FltUnregisterFilter(m_pFilter);
				}
				m_isInit = false;
				return STATUS_SUCCESS;
			}
			return STATUS_ACCESS_DENIED;
		}
		NTSTATUS FsInstanceSetup(
			_In_ PCFLT_RELATED_OBJECTS FltObjects,
			_In_ FLT_INSTANCE_SETUP_FLAGS Flags,
			_In_ DEVICE_TYPE VolumeDeviceType,
			_In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType)
		{
			//注册的InstanceSetup
			m_lock.only_acquire();
			auto ns = STATUS_SUCCESS;
			if (m_fnInstanceSetup)
			{
				ns = m_fnInstanceSetup(FltObjects, Flags, VolumeDeviceType, VolumeFilesystemType);
			}
			m_lock.release();
			return ns;//STATUS_FLT_DO_NOT_ATTACH
		}
		NTSTATUS FsInstanceQueryTeardown(
			_In_ PCFLT_RELATED_OBJECTS FltObjects,
			_In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags)
		{
			//调用下层注册的
			auto ns = STATUS_SUCCESS;
			m_lock.only_acquire();
			if (m_fnTearDown)
			{
				ns = m_fnTearDown(FltObjects, Flags);
			}
			m_lock.release();
			return ns;
		}
	};
}

#define MINIFLT_SET_PRE(major, precall)     ddk::flt::miniflt::BaseMiniFlt::getIntanceRef().setPreCallback(major, precall);
#define MINIFLT_SET_POST(major, postcall)   ddk::flt::miniflt::BaseMiniFlt::getIntanceRef().setPostCallback(major, postcall);
#define MINIFLT_SET_CLEANUP(type,callback)  ddk::flt::miniflt::BaseMiniFlt::getIntanceRef().setCleanupContextCallback(type, callback);
#define MINIFLT_SET_INSTANTCE(callback)     ddk::flt::miniflt::BaseMiniFlt::getIntanceRef().setInstanceSetupCallback(callback);