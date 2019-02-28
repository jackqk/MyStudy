#pragma once
#include <stdafx.h>
#include "../DetoursHeader.h"

namespace ddk::hook {
	class IRPHook
	{
	private:
		typedef struct _ORIGIN_FUN_INFO {
			PDRIVER_OBJECT pDrvObj;
			PDRIVER_DISPATCH pOrginFun;
			USHORT irpCode;
		}ORIGIN_FUN_INFO;
		std::vector<ORIGIN_FUN_INFO> m_funInfo;
	public:
		IRPHook() {

		}
		~IRPHook() {
			for (auto info : m_funInfo) {
				Detours::Internal::AtomicCopy4X8((uint8_t *)&(info.pDrvObj->MajorFunction[info.irpCode]),
					(uint8_t *)&info.pOrginFun, sizeof(PVOID));
				if (info.pDrvObj)
					ObDereferenceObject(info.pDrvObj);
			}
		}
		PDRIVER_DISPATCH doHook(WCHAR *drvName, USHORT irpCode, PDRIVER_DISPATCH newFun) {
			PDRIVER_DISPATCH oldDispatch = NULL;
			ORIGIN_FUN_INFO info;
			do
			{
				if (!ddk::CDriver::getDriverObjectByDriverName(drvName, &info.pDrvObj))
					break;
				info.irpCode = irpCode;
				info.pOrginFun = info.pDrvObj->MajorFunction[irpCode];
				if (!Detours::Internal::AtomicCopy4X8((uint8_t *)&(info.pDrvObj->MajorFunction[irpCode]),
					(uint8_t *)&newFun, sizeof(PVOID)))
					break;
				oldDispatch = info.pOrginFun;
				m_funInfo.push_back(info);
			} while (FALSE);

			return oldDispatch;
		}
	};
}