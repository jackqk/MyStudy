#include <stdafx.h>
#include <HookEngine/DetoursHeader.h>


using pfnNtCreateFile = decltype(&NtCreateFile);
pfnNtCreateFile OldNtCreateFile;
using pfnZwTerminateProcess = decltype(&ZwTerminateProcess);
pfnZwTerminateProcess OldZwTerminateProcess;

NTSTATUS Hook_NtCreateFile(
	_Out_    PHANDLE            FileHandle,
	_In_     ACCESS_MASK        DesiredAccess,
	_In_     POBJECT_ATTRIBUTES ObjectAttributes,
	_Out_    PIO_STATUS_BLOCK   IoStatusBlock,
	_In_opt_ PLARGE_INTEGER     AllocationSize,
	_In_     ULONG              FileAttributes,
	_In_     ULONG              ShareAccess,
	_In_     ULONG              CreateDisposition,
	_In_     ULONG              CreateOptions,
	_In_opt_ PVOID              EaBuffer,
	_In_     ULONG              EaLength)
{
	NTSTATUS status;
	HANDLE pid = PsGetCurrentProcessId();
	PFILE_OBJECT obj;

	auto ret = OldNtCreateFile(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize,
		FileAttributes, ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength);
	do
	{
		if (OBJ_KERNEL_HANDLE & ObjectAttributes->Attributes)
		{
			status = ObReferenceObjectByHandle(*FileHandle, FILE_READ_ACCESS, *IoFileObjectType, KernelMode, (PVOID *)&obj, NULL);
		}
		else {
			status = ObReferenceObjectByHandle(*FileHandle, FILE_READ_ACCESS, *IoFileObjectType, UserMode, (PVOID *)&obj, NULL);
		}
		if (!NT_SUCCESS(status))
			break;
		if (obj->FileName.Length == 0)
			break;

		LOG_DEBUG("%wZ\r\n", &obj->FileName);
		ObDereferenceObject(obj);
	} while (FALSE);
	return ret;
}

NTSTATUS Hook_ZwTerminateProcess(
	_In_opt_ HANDLE ProcessHandle,
	_In_ NTSTATUS ExitStatus) {
	LOG_DEBUG("进程%d结束\r\n", ProcessHandle);
	return OldZwTerminateProcess(ProcessHandle, ExitStatus);
}

void HookEntry() {
	auto addr = ddk::util::CommonUtil::GetRoutineAddr(L"NtCreateFile");
	OldNtCreateFile = (pfnNtCreateFile)hook::DetourFunction((uint8_t*)addr, (uint8_t *)Hook_NtCreateFile);

	addr = ddk::util::CommonUtil::GetRoutineAddr(L"ZwTerminateProcess");
	OldZwTerminateProcess = (pfnZwTerminateProcess)hook::DetourFunction((uint8_t*)addr, (uint8_t *)Hook_ZwTerminateProcess);
}