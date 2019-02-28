#pragma once
#include <stdafx.h>
#include "../DetoursHeader.h"

namespace ddk::hook {

	class IRPHookEx
	{
	private:
		PDRIVER_OBJECT m_pOldDrvObj;
		PDRIVER_OBJECT m_pNewDrvObj;
		std::vector<PDEVICE_OBJECT> m_oldDevList;
		PDRIVER_DISPATCH m_oldMajorFun[IRP_MJ_MAXIMUM_FUNCTION + 1];
		BOOL isHooked;
		IRPHookEx() {
			m_pOldDrvObj = NULL;
			m_pNewDrvObj = NULL;
			isHooked = FALSE;
			RtlZeroMemory(m_oldMajorFun, sizeof(m_oldMajorFun));
		}
	public://构造（公开一个有参构造）、析构
		IRPHookEx(WCHAR *driverName) : IRPHookEx() {
			do
			{
				m_pNewDrvObj = ddk::CDriver::getInstance().createDriverObj();
				if (!m_pNewDrvObj)
					break;
				if (!ddk::CDriver::getDriverObjectByDriverName(driverName, &m_pOldDrvObj))
					break;
				m_pNewDrvObj->DeviceObject = m_pOldDrvObj->DeviceObject;
				m_pNewDrvObj->DriverExtension = m_pOldDrvObj->DriverExtension;
				m_pNewDrvObj->DriverStartIo = m_pOldDrvObj->DriverStartIo;
				m_pNewDrvObj->FastIoDispatch = m_pOldDrvObj->FastIoDispatch;
				m_pNewDrvObj->HardwareDatabase = m_pOldDrvObj->HardwareDatabase;
				m_pNewDrvObj->Flags = m_pOldDrvObj->Flags;
				m_pNewDrvObj->Type = m_pOldDrvObj->Type;
				RtlCopyMemory(
					m_pNewDrvObj->MajorFunction,
					m_pOldDrvObj->MajorFunction,
					sizeof(m_pOldDrvObj->MajorFunction));

				ddk::CDriver::getDriverAllDevice(m_pOldDrvObj, m_oldDevList);
				for (auto pObj : m_oldDevList) {
					InterlockedExchangePointer((PVOID *)&pObj->DriverObject, (PVOID)m_pNewDrvObj);
				}
				isHooked = TRUE;
			} while (FALSE);
		}
		~IRPHookEx() {
			//都不用还原hook的函数
			for (auto pObj : m_oldDevList) {
				InterlockedExchangePointer((PVOID *)&pObj->DriverObject, (PVOID)m_pOldDrvObj);
			}
			if (m_pOldDrvObj)
			{
				ObDereferenceObject(m_pOldDrvObj);
				m_pOldDrvObj = NULL;
			}
		}
	public:
		PDRIVER_DISPATCH doHook(USHORT irpCode, PDRIVER_DISPATCH userFun) {
			PDRIVER_DISPATCH oldFun = NULL;
			m_oldMajorFun[irpCode] = m_pNewDrvObj->MajorFunction[irpCode];
			InterlockedExchangePointer((PVOID *)&m_pNewDrvObj->MajorFunction[irpCode], (PVOID)userFun);
			return m_oldMajorFun[irpCode];
		}
		VOID unHook(USHORT irpCode) {
			InterlockedExchangePointer((PVOID *)&m_pNewDrvObj->MajorFunction[irpCode], m_oldMajorFun[irpCode]);
			m_oldMajorFun[irpCode] = NULL;
		}
	};
}