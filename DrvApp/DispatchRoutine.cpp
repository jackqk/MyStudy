#include <stdafx.h>
#include "ioctrl.h"

NTSTATUS CreateDispatch(PDEVICE_OBJECT pDevObj, PIRP pIrp);
NTSTATUS ReadDispatch  (PDEVICE_OBJECT pDevObj, PIRP pIrp);
NTSTATUS WriteDispatch (PDEVICE_OBJECT pDevObj, PIRP pIrp);
NTSTATUS CloseDispatch (PDEVICE_OBJECT pDevObj, PIRP pIrp);
NTSTATUS HELLO1_HANDLE(PVOID InputBuffer, ULONG InputBufferSize,PVOID OutputBuffer, ULONG OutputBufferSize, ULONG_PTR *ReturnSize);
NTSTATUS HELLO2_HANDLE(PVOID InputBuffer, ULONG InputBufferSize, PVOID OutputBuffer, ULONG OutputBufferSize, ULONG_PTR *ReturnSize);
NTSTATUS HELLO3_HANDLE(PVOID InputBuffer, ULONG InputBufferSize, PVOID OutputBuffer, ULONG OutputBufferSize, ULONG_PTR *ReturnSize);

void setDispatchRoutine() {
	//普通分发函数
	g_dev.set_irp_callback(IRP_MJ_CREATE, CreateDispatch);
	g_dev.set_irp_callback(IRP_MJ_READ  , ReadDispatch);
	g_dev.set_irp_callback(IRP_MJ_WRITE , WriteDispatch);
	g_dev.set_irp_callback(IRP_MJ_CLOSE , CloseDispatch);
	//IO_DEVICE_CONTROL分发函数
	g_dev.set_ioctrl_callback(FG_HELLO1, HELLO1_HANDLE);	//METHOD_BUFFER方式
	g_dev.set_ioctrl_callback(FG_HELLO2, HELLO2_HANDLE);	//DIRECT_IO方式
	g_dev.set_ioctrl_callback(FG_HELLO3, HELLO3_HANDLE);	//DIRECT_IO方式
}
NTSTATUS CreateDispatch(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	LOG_DEBUG("create dispatch\r\n");
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}
NTSTATUS ReadDispatch(PDEVICE_OBJECT pDevObj, PIRP pIrp) {
	LOG_DEBUG("read dispatch\r\n");
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	ULONG length = stack->Parameters.Read.Length;                       //对应ReadFile/WriteFile长度
	ULONG mdl_len = MmGetMdlByteCount(pIrp->MdlAddress);                //锁定缓冲区长度
	PVOID mdl_address = MmGetMdlVirtualAddress(pIrp->MdlAddress);       //锁定缓冲区首地址
	ULONG mdl_offset = MmGetMdlByteOffset(pIrp->MdlAddress);            //锁定缓冲区的偏移量
	PVOID kernel_address = MmGetSystemAddressForMdlSafe(pIrp->MdlAddress, NormalPagePriority);//得到MDL内核模式下的映射+

	//TODO...
	char *buf = "hello world";
	RtlCopyMemory(kernel_address, buf, strlen(buf) + 1);

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = strlen(buf) + 1;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}
NTSTATUS WriteDispatch(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	LOG_DEBUG("write dispatch\r\n");
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	ULONG length = stack->Parameters.Write.Length;                       //对应ReadFile/WriteFile长度
	ULONG mdl_len = MmGetMdlByteCount(pIrp->MdlAddress);                //锁定缓冲区长度
	PVOID mdl_address = MmGetMdlVirtualAddress(pIrp->MdlAddress);       //锁定缓冲区首地址
	ULONG mdl_offset = MmGetMdlByteOffset(pIrp->MdlAddress);            //锁定缓冲区的偏移量
	PVOID kernel_address = MmGetSystemAddressForMdlSafe(pIrp->MdlAddress, NormalPagePriority);//得到MDL内核模式下的映射+

	//TODO...
	LOG_DEBUG("%s\r\n", kernel_address);

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}
NTSTATUS CloseDispatch(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	LOG_DEBUG("Close dispatch\r\n");
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS HELLO1_HANDLE(PVOID InputBuffer,ULONG InputBufferSize,
	PVOID OutputBuffer,ULONG OutputBufferSize,ULONG_PTR *ReturnSize)
{
	char *str = reinterpret_cast<char *>(InputBuffer);
	char *retStr = "曹你爸爸";
	LOG_DEBUG("%s\r\n", str);
	*ReturnSize = strlen(retStr) + 1;
	RtlCopyMemory(OutputBuffer, retStr, *ReturnSize);
	return STATUS_SUCCESS;
}
NTSTATUS HELLO2_HANDLE(PVOID InputBuffer, ULONG InputBufferSize,
	PVOID OutputBuffer, ULONG OutputBufferSize, ULONG_PTR *ReturnSize) {
	char *str = reinterpret_cast<char *>(InputBuffer);
	char *retStr = "曹你爷爷";
	LOG_DEBUG("%s\r\n", str);
	*ReturnSize = strlen(retStr) + 1;
	RtlCopyMemory(OutputBuffer, retStr, *ReturnSize);
	return STATUS_SUCCESS;
}
NTSTATUS HELLO3_HANDLE(PVOID InputBuffer, ULONG InputBufferSize, 
	PVOID OutputBuffer, ULONG OutputBufferSize, ULONG_PTR *ReturnSize) {
	char *str = reinterpret_cast<char *>(InputBuffer);
	char *retStr = "曹你奶奶";
	LOG_DEBUG("%s\r\n", str);
	*ReturnSize = strlen(retStr) + 1;
	RtlCopyMemory(OutputBuffer, retStr, *ReturnSize);
	return STATUS_SUCCESS;
}