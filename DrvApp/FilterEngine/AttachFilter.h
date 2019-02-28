#pragma once
#include <stdafx.h>
namespace ddk::flt {
	//这个过滤，涉及到文件，会BSOD，可能没处理fastio，
	//暂不会处理fastio，以后补充
	class AttachFilter
	{
	public:
		typedef struct _DEVICE_EXTENSION
		{
			DEVICE_OBJECT	*attachedDev;	//过滤设备的下一层
			AttachFilter	*self;
		} DEVICE_EXTENSION, *PDEVICE_EXTENSION;
	private://成员变量
		PDRIVER_OBJECT m_pNewDrvObj;
		std::vector<PDEVICE_OBJECT> m_fltDevsVec;	//过滤设备，最上层
		PDRIVER_DISPATCH m_fltDispatch[IRP_MJ_MAXIMUM_FUNCTION + 1];
	public://构造、析构
		AttachFilter() {
			m_pNewDrvObj = ddk::CDriver::getInstance().createDriverObj();
			RtlZeroMemory(m_fltDispatch, RTL_NUMBER_OF(m_fltDispatch));
			if (m_pNewDrvObj)
			{
				for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION + 1; i++)
					m_pNewDrvObj->MajorFunction[i] = DispatchRoutine;
			}
		}
		~AttachFilter() {
			if (!m_fltDevsVec.empty())
			{
				for (auto fltDev : m_fltDevsVec) {
					auto devExt = reinterpret_cast<PDEVICE_EXTENSION>(fltDev->DeviceExtension);
					IoDetachDevice(devExt->attachedDev);
					IoDeleteDevice(fltDev);
				}
				//这个一打开，特么就蓝屏，目前不知道什么原因。感觉跟fastio有关系
				//ddk::CDriver::getInstance().delDriverObj(m_pNewDrvObj);
				m_pNewDrvObj = NULL;
				m_fltDevsVec.clear();
				clearDispatch();
			}
		}
		AttachFilter(const AttachFilter&) = delete;
		AttachFilter & operator = (const AttachFilter &) = delete;
	public:
		BOOL attachDriver(WCHAR *drvName) {
			BOOL ret = FALSE;
			PDRIVER_OBJECT pAttachDrv;
			std::vector<PDEVICE_OBJECT> tarDevs;
			do
			{
				if (!ddk::CDriver::getDriverObjectByDriverName(drvName, &pAttachDrv))
					break;
				if (!ddk::CDriver::getDriverAllDevice(pAttachDrv, tarDevs))
					break;

				for (auto dev : tarDevs)
				{
					PDEVICE_OBJECT filterDev, attachDev;
					if (!attachDevice(&filterDev, dev, &attachDev))
						break;
				}
			} while (FALSE);
			if (pAttachDrv)
				ObfDereferenceObject(pAttachDrv);
			return ret;
		}
		BOOL attachDevice(WCHAR *devName)
		{
			PDEVICE_OBJECT filterDev, tarDev, attachDev;
			if (!m_pNewDrvObj)
				return FALSE;
			tarDev = ddk::CDevice::getDeviceObjectByName(devName);
			if (!tarDev)
				return FALSE;
			if (!attachDevice(&filterDev, tarDev, &attachDev))
				return FALSE;
			return TRUE;
		}
		VOID regDispatch(USHORT irpCode, PDRIVER_DISPATCH dispatch) {
			m_fltDispatch[irpCode] = dispatch;
		}
		VOID unRegDispatch(USHORT irpCode) {
			m_fltDispatch[irpCode] = NULL;
		}
		VOID clearDispatch() {
			RtlZeroMemory(m_fltDispatch, RTL_NUMBER_OF(m_fltDispatch));
		}
	private:
		BOOL attachDevice(
			PDEVICE_OBJECT _Out_ *filterDev,		//创建的过滤设备
			PDEVICE_OBJECT _In_  tarDev,			//目标设备
			PDEVICE_OBJECT _Out_ *attachDev)		//返回的过滤设备的attached设备
		{
			NTSTATUS status;
			PDEVICE_EXTENSION devExt;

			if (!m_pNewDrvObj)
				return FALSE;
			status = IoCreateDevice(m_pNewDrvObj,
				sizeof(DEVICE_EXTENSION),
				nullptr,
				tarDev->DeviceType,
				tarDev->Characteristics,
				FALSE,
				filterDev);
			if (!NT_SUCCESS(status))
				ddk::log::StatusInfo::Print(status);
			else
				(*filterDev)->Flags &= ~DO_DEVICE_INITIALIZING;

			status = IoAttachDeviceToDeviceStackSafe(*filterDev, tarDev, attachDev);
			if (!NT_SUCCESS(status))
			{
				IoDeleteDevice(*filterDev);
				*filterDev = nullptr;
				return FALSE;
			}
			else
				m_fltDevsVec.push_back(*filterDev);

			devExt = reinterpret_cast<PDEVICE_EXTENSION>((*filterDev)->DeviceExtension);
			RtlZeroMemory(devExt, sizeof(DEVICE_EXTENSION));
			devExt->attachedDev = *attachDev;
			devExt->self = this;
			(*filterDev)->DeviceType = (*attachDev)->DeviceType;
			(*filterDev)->Characteristics = (*attachDev)->Characteristics;
			(*filterDev)->StackSize = (*attachDev)->StackSize + 1;
			(*filterDev)->Flags |= (*attachDev)->Flags & (DO_BUFFERED_IO | DO_DIRECT_IO | DO_POWER_PAGABLE);
			return TRUE;
		}
	private://DriverObject->MajorFunction通用函数
		static NTSTATUS DispatchRoutine(struct _DEVICE_OBJECT *DeviceObject, struct _IRP *Irp) {
			PDEVICE_EXTENSION ext = reinterpret_cast<PDEVICE_EXTENSION>(DeviceObject->DeviceExtension);
			PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(Irp);
			AttachFilter *tmpThis = ext->self;

			for (auto fltDev : tmpThis->m_fltDevsVec)
			{
				if (DeviceObject == fltDev)
				{
					switch (irpsp->MajorFunction)
					{
					case IRP_MJ_PNP_POWER:
						return DispatchPnp(DeviceObject, Irp);
					case IRP_MJ_POWER:
						return DispatchPower(DeviceObject, Irp);
					default:
						return CommonDispatch(DeviceObject, Irp);
					}
				}
			}

			//在过滤设备中找不到，则返回下列代码
			Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
			Irp->IoStatus.Information = 0;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
			return STATUS_NOT_IMPLEMENTED;
		}
		static NTSTATUS CommonDispatch(PDEVICE_OBJECT devObj, PIRP Irp) {
			PDEVICE_EXTENSION ext = reinterpret_cast<PDEVICE_EXTENSION>(devObj->DeviceExtension);
			AttachFilter *tmpThis = ext->self;
			PIO_STACK_LOCATION psirp = IoGetCurrentIrpStackLocation(Irp);

			if (tmpThis->m_fltDispatch[psirp->MajorFunction] != NULL)
				return tmpThis->m_fltDispatch[psirp->MajorFunction](devObj, Irp);

			IoSkipCurrentIrpStackLocation(Irp);
			return IoCallDriver(ext->attachedDev, Irp);
		}
		static NTSTATUS DispatchPower(PDEVICE_OBJECT devObj, PIRP Irp)
		{
			auto ext = reinterpret_cast<PDEVICE_EXTENSION>(devObj->DeviceExtension);
			PoStartNextPowerIrp(Irp);
			IoSkipCurrentIrpStackLocation(Irp);
			return PoCallDriver(ext->attachedDev, Irp);
		}
		static NTSTATUS DispatchPnp(PDEVICE_OBJECT devObj, PIRP Irp)
		{
			NTSTATUS status = STATUS_NOT_IMPLEMENTED;
			auto irpstack = IoGetCurrentIrpStackLocation(Irp);
			auto ext = reinterpret_cast<PDEVICE_EXTENSION>(devObj->DeviceExtension);

			switch (irpstack->MinorFunction)
			{
			case IRP_MN_REMOVE_DEVICE:
				IoSkipCurrentIrpStackLocation(Irp);
				IoCallDriver(ext->attachedDev, Irp);
				IoDetachDevice(ext->attachedDev);
				IoDeleteDevice(devObj);
				status = STATUS_SUCCESS;
				break;
			default:
				IoSkipCurrentIrpStackLocation(Irp);
				status = IoCallDriver(ext->attachedDev, Irp);
				break;
			}
			return status;
		}
	};
}