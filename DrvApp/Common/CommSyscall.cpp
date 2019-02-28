#include <stdafx.h>

//使用方法
//1.声明：using fnIoThreadToProcess = std::function<PEPROCESS(PETHREAD)>;
//2.调用：PETHREAD pt;
//       SAFE_NATIVE_CALL(IoThreadToProcess, pt);
using fnNtQueryVirtualMemory = decltype(&NtQueryVirtualMemory);
using fnNtWriteVirtualMemory = NTSTATUS(NTAPI*)(HANDLE ProcessHandle, PVOID BaseAddress, PVOID Buffer, ULONG BufferLength, PULONG ReturnLength);
using fnNtReadVirtualMemory = NTSTATUS(NTAPI*)(HANDLE ProcessHandle, PVOID BaseAddress, PVOID Buffer, ULONG BufferLength, PULONG ReturnLength);
using fnNtProtectVirtualMemory = NTSTATUS(NTAPI*)(HANDLE ProcessHandle, PVOID* BaseAddress, SIZE_T* NumberOfBytesToProtect, ULONG NewAccessProtection, PULONG OldAccessProtection);
using fnNtCreateThreadEx = NTSTATUS(NTAPI*)(OUT PHANDLE hThread, IN ACCESS_MASK DesiredAccess, IN PVOID ObjectAttributes, IN HANDLE ProcessHandle, IN PVOID lpStartAddress, IN PVOID lpParameter, IN ULONG Flags, IN SIZE_T StackZeroBits, IN SIZE_T SizeOfStackCommit, IN SIZE_T SizeOfStackReserve, OUT PVOID lpBytesBuffer);

NTSTATUS NTAPI FGNtQueryVirtualMemory(
	HANDLE ProcessHandle, 
	PVOID BaseAddress,
	MEMORY_INFORMATION_CLASS MemoryInformationClass,
	PVOID MemoryInformation, 
	SIZE_T MemoryInformationLength, 
	PSIZE_T ReturnLength)
{
	NTSTATUS status = STATUS_SUCCESS;
	status = SAFE_SYSCALL(NtQueryVirtualMemory, ProcessHandle, BaseAddress, MemoryInformationClass, MemoryInformation, MemoryInformationLength, ReturnLength);
	return status;
}

NTSTATUS NTAPI FGNtWriteVirtualMemory(
	HANDLE ProcessHandle, 
	PVOID BaseAddress,
	PVOID Buffer,
	ULONG BufferSize, 
	PULONG ReturnLength)
{
	NTSTATUS status = STATUS_SUCCESS;
	status = SAFE_SYSCALL(NtWriteVirtualMemory, ProcessHandle, BaseAddress, Buffer, BufferSize, ReturnLength);
	return status;
}
NTSTATUS FGNtReadVirtualMemory(
	HANDLE ProcessHandle, 
	PVOID BaseAddress, 
	PVOID Buffer, 
	ULONG BufferSize, 
	PULONG ReturnLength)
{
	NTSTATUS status = STATUS_SUCCESS;
	status = SAFE_SYSCALL(NtReadVirtualMemory, ProcessHandle, BaseAddress, Buffer, BufferSize, ReturnLength);
	return status;
}

NTSTATUS FGNtProtectVirtualMemory(
	HANDLE ProcessHandle, 
	PVOID * BaseAddress, 
	PSIZE_T RegionSize,
	ULONG NewProtect, 
	PULONG OldProtect)
{
	NTSTATUS status = STATUS_SUCCESS;
	status = SAFE_SYSCALL(NtProtectVirtualMemory, ProcessHandle, BaseAddress, RegionSize, NewProtect, OldProtect);
	return status;
}

NTSTATUS FGNtCreateThreadEx(
	PHANDLE ThreadHandle,
	ACCESS_MASK DesiredAccess, 
	POBJECT_ATTRIBUTES ObjectAttributes,
	HANDLE ProcessHandle, 
	PVOID StartRoutine, 
	PVOID Argument,
	ULONG CreateFlags,
	SIZE_T ZeroBits, 
	SIZE_T StackSize,
	SIZE_T MaximumStackSize, 
	PNT_PROC_THREAD_ATTRIBUTE_LIST AttributeList)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	status = SAFE_SYSCALL(NtCreateThreadEx,
		ThreadHandle, DesiredAccess, ObjectAttributes,
		ProcessHandle, StartRoutine, Argument,
		CreateFlags, ZeroBits, StackSize,
		MaximumStackSize, AttributeList);
	return status;
}