#include <stdafx.h>
#include <FilterEngine/MiniFilter/BaseMiniFlt.h>

extern void setDispatchRoutine();
void test(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pRegistryPath);

//直接在Driver.cpp里设置了
VOID DriverUnload(PDRIVER_OBJECT pDriverObject)
{
	UNREFERENCED_PARAMETER(pDriverObject);
	LOG_DEBUG("Goodbye world!\r\n");
}

NTSTATUS DriverMain(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pRegistryPath)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL; 
	setDispatchRoutine();
	if (g_dev.create_device(DEVICE_NAME, LINK_NAME))
		status = STATUS_SUCCESS;
	test(pDriverObject, pRegistryPath);
	return status;
}


/////////////R0代码//////////////////
typedef struct _CTX_STREAM_CONTEXT {
	UNICODE_STRING FileName;	//文件名，一般在CreatePost获得
	ULONG CreateCount;			
	ULONG CleanupCount;			
	ULONG CloseCount;
	ddk::lock::CMutex *pMutex;
} CTX_STREAM_CONTEXT, *PCTX_STREAM_CONTEXT;

NTSTATUS CreateStreamContext(
	_In_      PFLT_CALLBACK_DATA Data,
	_In_      BOOLEAN CreateIfNotFound,	
	_Outptr_  PCTX_STREAM_CONTEXT *StreamContext)
{
	NTSTATUS status;
	PCTX_STREAM_CONTEXT streamContext;
	PCTX_STREAM_CONTEXT oldStreamContext;

	status = FltGetStreamContext(Data->Iopb->TargetInstance, 
								 Data->Iopb->TargetFileObject, 
							     (PFLT_CONTEXT *)&streamContext);

	if (!NT_SUCCESS(status) && status == STATUS_NOT_FOUND && CreateIfNotFound == TRUE)
	{
		status = FltAllocateContext(ddk::flt::miniflt::BaseMiniFlt::getIntanceRef().getFilter(),
									FLT_STREAM_CONTEXT, 
									ddk::flt::miniflt::FS_STREAM_CONTEXT_SIZE, 
									PagedPool, 
									(PFLT_CONTEXT *)&streamContext);
		if (!NT_SUCCESS(status))
		{
			ddk::log::StatusInfo::Print(status);
			return status;
		}

		//
		//  Context创建完毕，初始化
		//
		
		RtlZeroMemory(streamContext, ddk::flt::miniflt::FS_STREAM_CONTEXT_SIZE);
		streamContext->pMutex = new ddk::lock::CMutex();

		//  初始完成
		
		status = FltSetStreamContext(Data->Iopb->TargetInstance,
			Data->Iopb->TargetFileObject,
			FLT_SET_CONTEXT_KEEP_IF_EXISTS,
			(PFLT_CONTEXT)streamContext, (PFLT_CONTEXT *)&oldStreamContext);
		if (!NT_SUCCESS(status))
		{
			FltReleaseContext(streamContext);	//释放新创建的context
			if (status != STATUS_FLT_CONTEXT_ALREADY_DEFINED)
			{
				ddk::log::StatusInfo::Print(status);
				return status;
			}

			//当status == STATUS_FLT_CONTEXT_ALREADY_DEFINED
			streamContext = oldStreamContext;
			status = STATUS_SUCCESS;
		}
	}
	*StreamContext = streamContext;
	return status;
}

FLT_POSTOP_CALLBACK_STATUS testPost(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ PVOID CompletionContext OPTIONAL,
	_In_ FLT_POST_OPERATION_FLAGS Flags)
{
	NTSTATUS status;
	PCTX_STREAM_CONTEXT streamContext = NULL;
	PFLT_FILE_NAME_INFORMATION pNameInfo = NULL;

	do
	{
		status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &pNameInfo);
		if (!NT_SUCCESS(status))
		{
			ddk::log::StatusInfo::Print(status);
			break;
		}
		status = CreateStreamContext(Data, TRUE, &streamContext);
		if (!NT_SUCCESS(status))
		{
			ddk::log::StatusInfo::Print(status);
			break;
		}
		streamContext->pMutex->lock();

		streamContext->CreateCount++;
		//更新文件名
		if (streamContext->FileName.Buffer != NULL)
			ddk::util::CommonUtil::FGFreeUnicodeString(&streamContext->FileName);
		streamContext->FileName.MaximumLength = pNameInfo->Name.MaximumLength;
		ddk::util::CommonUtil::FGAllocateUnicodeString(&streamContext->FileName);
		RtlCopyUnicodeString(&streamContext->FileName, &pNameInfo->Name);
		streamContext->pMutex->unlock();
		
	} while (FALSE);
	
	if (pNameInfo)
		FltReleaseFileNameInformation(pNameInfo);
	if (streamContext)
		FltReleaseContext(streamContext);

	return FLT_POSTOP_FINISHED_PROCESSING;
}
void test(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pRegistryPath) {
	MINIFLT_SET_POST(IRP_MJ_CREATE, testPost);
	ddk::flt::miniflt::BaseMiniFlt::getInstancePtr()->initFlt(pDriverObject);
}


