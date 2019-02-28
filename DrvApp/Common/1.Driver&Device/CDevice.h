#pragma once
namespace ddk {
	class CDevice {
	private:
		typedef struct _DEVICE_EXTENSION
		{
			DWORD32			Tag;				//防止意外发生的Tag
			LIST_ENTRY		ListHead;			//待处理的IRP链表
			KSPIN_LOCK      ListLock;		    //IRP链表操作锁   
			KEVENT			RequestEvent;		//控制/请求事件
			PVOID			ThreadObject;		//工作线程对象
			BOOLEAN			bTerminateThread;	//是否需要终止线程
			PSECURITY_CLIENT_CONTEXT SecurityClientCtx;
			CDevice   *pSelf;
		} DEVICE_EXTENSION, *PDEVICE_EXTENSION;
		using  callback_ioctrl = std::function<NTSTATUS(PVOID, ULONG, PVOID, ULONG, ULONG_PTR *)>;
		using  callback_irp = std::function<NTSTATUS(PDEVICE_OBJECT, PIRP)>;
	private:
		PDRIVER_OBJECT m_pDriverObject;
		PDEVICE_OBJECT m_pDeviceObject;
		UNICODE_STRING m_nsDosName;
		UNICODE_STRING m_nsDeviceName;
		DWORD m_dwDeviceType;		//设备类型：IoCreateDeviceSecure第四个参数
		BOOL m_bAsynAble;
		std::map<DWORD, callback_irp> m_map_irpRoutine;
		std::map<DWORD, callback_ioctrl> m_map_ioctrlRoutine;
	//构造、析构、成员获取与设置
	public:
		CDevice() {
			m_pDriverObject = ddk::CDriver::getInstance().createDriverObj();
			m_pDeviceObject = NULL;
			m_dwDeviceType = 0x00000022;	//随便先给个号
			m_bAsynAble = FALSE;
			m_map_irpRoutine.clear();
			m_map_ioctrlRoutine.clear();
		};
		~CDevice() {
			IoDeleteSymbolicLink(&m_nsDosName);
			IoDeleteDevice(m_pDeviceObject);
		};
		PDEVICE_OBJECT getDeviceObject() {
			return m_pDeviceObject;
		}
		void setDeviceType(DWORD dwCode) {
			m_dwDeviceType = dwCode;
		}
		void set_ioctrl_callback(DWORD dwCode, callback_ioctrl callback) {
			m_map_ioctrlRoutine[dwCode] = callback;
		}
		void set_irp_callback(DWORD irp, callback_irp callback) {
			m_map_irpRoutine[irp] = callback;
		}
	public://操作
		BOOL create_device(LPCWSTR device_name, LPCWSTR dos_name, 
			DWORD IoMethod = DO_DIRECT_IO, BOOL b_asyn = FALSE)
		{
			//有同步和异步，感觉异步也没什么屌用
			BOOL ret = FALSE;
			NTSTATUS status;
			DEVICE_EXTENSION *pDeviceExtension;
			do
			{
				m_bAsynAble = b_asyn;
				if (m_pDeviceObject) break;
				if (!m_pDriverObject) break;

				status = AuxKlibInitialize();
				if (!NT_SUCCESS(status))
					break;

				RtlInitUnicodeString(&m_nsDeviceName, device_name);
				RtlInitUnicodeString(&m_nsDosName, dos_name);
				status = IoCreateDeviceSecure(m_pDriverObject,
					sizeof(DEVICE_EXTENSION),
					&m_nsDeviceName,
					m_dwDeviceType,
					FILE_DEVICE_SECURE_OPEN,
					FALSE,
					&SDDL_DEVOBJ_SYS_ALL_ADM_ALL, nullptr,
					&m_pDeviceObject);
				if (!NT_SUCCESS(status))
					break;
				//不在DriverEntry流程里(也就是不受IO管理器控制的DeviceObject)，
				//创建需要自己清除init标记
				m_pDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
				m_pDeviceObject->Flags |= IoMethod;
				
				//设置Device_Extension
				pDeviceExtension = reinterpret_cast<PDEVICE_EXTENSION>(m_pDeviceObject->DeviceExtension);
				RtlZeroMemory(pDeviceExtension, sizeof(DEVICE_EXTENSION));
				pDeviceExtension->Tag = 'Obj1';
				pDeviceExtension->bTerminateThread = FALSE;
				InitializeListHead(&pDeviceExtension->ListHead);
				//KeInitializeSpinLock(&pDeviceExtension->ListLock);//在这里初始化，驱动都打不开了，去插入前初始化看看
				KeInitializeEvent(&pDeviceExtension->RequestEvent, SynchronizationEvent, FALSE);
				pDeviceExtension->pSelf = this;

				if (m_bAsynAble)
				{
					HANDLE hThread = 0;
					status = PsCreateSystemThread(&hThread,
						THREAD_ALL_ACCESS,
						NULL,
						NULL,
						NULL,
						ddk::CDevice::asyn_thread_routine,
						this);
					if (!NT_SUCCESS(status))
						break;

					status = ObReferenceObjectByHandle(hThread,
						THREAD_ALL_ACCESS,
						*PsThreadType,
						KernelMode,
						&pDeviceExtension->ThreadObject,
						NULL);
					ZwClose(hThread);
					if (!NT_SUCCESS(status))//线程对象获取失败，也许是线程异常，驱动的业务无法完成
					{
						pDeviceExtension->bTerminateThread = TRUE;
						KeSetEvent(&pDeviceExtension->RequestEvent, 0, FALSE);
						break;
					}
				}

				status = IoCreateSymbolicLink(&m_nsDosName, &m_nsDeviceName);
				if (!NT_SUCCESS(status))
					break;
				for (auto i = 0; i < IRP_MJ_MAXIMUM_FUNCTION + 1; i++)
				{
					m_pDriverObject->MajorFunction[i] = ddk::CDevice::DeviceIrpProc;
				}
				ret = TRUE;
			} while (FALSE);
			return ret;
		};
		static PDEVICE_OBJECT getDeviceObjectByName(WCHAR *name) {
			UNICODE_STRING usName;
			PFILE_OBJECT pFileObj = NULL;
			PDEVICE_OBJECT pDevObj = NULL;
			NTSTATUS status;

			RtlInitUnicodeString(&usName, name);
			status = IoGetDeviceObjectPointer(&usName, FILE_ALL_ACCESS, &pFileObj, &pDevObj);
			if (!NT_SUCCESS(status))
				ddk::log::StatusInfo::Print(status);
			else
				ObDereferenceObject(pFileObj);
			return pDevObj;
		}
	//私有成员
	private:
		//通用入口点
		static NTSTATUS DeviceIrpProc(PDEVICE_OBJECT pDeviceObject, PIRP pIrp) {
			PDEVICE_EXTENSION pDevExt = reinterpret_cast<PDEVICE_EXTENSION>(pDeviceObject->DeviceExtension);
			NTSTATUS status = STATUS_UNSUCCESSFUL;

			do
			{
				if (!pDevExt || pDevExt->Tag != 'Obj1')
					break;
				CDevice *pThis = pDevExt->pSelf;
				if (!pThis->m_bAsynAble)//同步
				{
					return pThis->device_irp(pIrp);
				}else {//异步
					SECURITY_QUALITY_OF_SERVICE SeQ = { 0 };

					if (pDevExt->SecurityClientCtx != NULL)
					{
						SeDeleteClientSecurity(pDevExt->SecurityClientCtx);
					}
					else
					{
						pDevExt->SecurityClientCtx = (PSECURITY_CLIENT_CONTEXT)malloc(sizeof(SECURITY_CLIENT_CONTEXT));
					}
					RtlZeroMemory(&SeQ, sizeof(SECURITY_QUALITY_OF_SERVICE));
					SeQ.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
					SeQ.ImpersonationLevel = SecurityImpersonation;
					SeQ.ContextTrackingMode = SECURITY_STATIC_TRACKING;
					SeQ.EffectiveOnly = FALSE;

					SeCreateClientSecurity(
						PsGetCurrentThread(),
						&SeQ,
						FALSE,
						pDevExt->SecurityClientCtx
					);

					IoMarkIrpPending(pIrp);
					ExInterlockedInsertTailList(&pDevExt->ListHead, &pIrp->Tail.Overlay.ListEntry, &pDevExt->ListLock);
					KeSetEvent(&pDevExt->RequestEvent, 0, FALSE);
					status = STATUS_PENDING;
				}
			} while (FALSE);
			return status;
		}
		NTSTATUS device_irp(PIRP pIrp) {
			ULONG information = 0;
			NTSTATUS status = STATUS_SUCCESS;
			PIO_STACK_LOCATION pStack = IoGetCurrentIrpStackLocation(pIrp);
			UCHAR majorCode = pStack->MajorFunction;

			//设置了回调走这里
			if (m_map_irpRoutine.find(majorCode) != m_map_irpRoutine.end())
			{
				callback_irp funIrp = m_map_irpRoutine[majorCode];
				return funIrp(m_pDeviceObject, pIrp);
			}//IO_CTRL走这里
			else if (majorCode == IRP_MJ_DEVICE_CONTROL) {
				PVOID inputBuffer = NULL;
				PVOID outputBuffer = NULL;
				ULONG inputLength = pStack->Parameters.DeviceIoControl.InputBufferLength;
				ULONG outputLength = pStack->Parameters.DeviceIoControl.OutputBufferLength;
				ULONG ioCode = pStack->Parameters.DeviceIoControl.IoControlCode;

				if (m_map_ioctrlRoutine.find(ioCode) != m_map_ioctrlRoutine.end())
				{
					switch (METHOD_FROM_CTL_CODE(ioCode))
					{
					case METHOD_BUFFERED:
						inputBuffer = pIrp->AssociatedIrp.SystemBuffer;
						outputBuffer = inputBuffer;
						break;
					case METHOD_IN_DIRECT:
					case METHOD_OUT_DIRECT:
						inputBuffer = pIrp->AssociatedIrp.SystemBuffer;
						outputBuffer = pIrp->MdlAddress 
							? MmGetSystemAddressForMdlSafe(pIrp->MdlAddress, NormalPagePriority | MdlMappingNoExecute) 
							: nullptr;
						break;
					case METHOD_NEITHER:
						inputBuffer = pStack->Parameters.DeviceIoControl.Type3InputBuffer;
						outputBuffer = pIrp->UserBuffer;
						break;
					}

					callback_ioctrl funcIoctrl = m_map_ioctrlRoutine[ioCode];
					status = funcIoctrl(inputBuffer, inputLength, outputBuffer, outputLength, reinterpret_cast<ULONG_PTR *>(&information));
				}
				else {
					status = STATUS_NOT_IMPLEMENTED;
				}
				pIrp->IoStatus.Status = status;
				pIrp->IoStatus.Information = information;
				IoCompleteRequest(pIrp, IO_NO_INCREMENT);
			}//没设置走这里
			else
			{
				pIrp->IoStatus.Status = status;
				pIrp->IoStatus.Information = information;
				IoCompleteRequest(pIrp, IO_NO_INCREMENT);
			}
			return status;
		}
		static VOID asyn_thread_routine(PVOID context) {
			CDevice *pNtDev = reinterpret_cast<CDevice *>(context);
			pNtDev->asyn_thread_work();
		}
		void asyn_thread_work() {
			PDEVICE_EXTENSION   pDeviceExtension;
			PLIST_ENTRY         Request = NULL;
			PIRP                Irp;
			pDeviceExtension = (PDEVICE_EXTENSION)m_pDeviceObject->DeviceExtension;
			//设置线程运行于低优先级,否则会一卡一卡的！！
			KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY);
			KeLowerIrql(PASSIVE_LEVEL);
			ddk::util::CommonUtil::AdjustPrivilege(SE_IMPERSONATE_PRIVILEGE, TRUE);
			while (TRUE)
			{
				KeWaitForSingleObject(&pDeviceExtension->RequestEvent,
					Executive,
					KernelMode,
					FALSE,
					NULL);
				if (pDeviceExtension->bTerminateThread)
				{
					PsTerminateSystemThread(STATUS_SUCCESS);//终止线程
				}
#pragma warning(push)
#pragma warning(disable:4706)
				Request = ExInterlockedRemoveHeadList(&pDeviceExtension->ListHead, &pDeviceExtension->ListLock);
				while (Request)
				{
					Irp = CONTAINING_RECORD(Request, IRP, Tail.Overlay.ListEntry);
					SeImpersonateClient(pDeviceExtension->SecurityClientCtx, NULL);
					device_irp(Irp);
					PsRevertToSelf();
					Request = ExInterlockedRemoveHeadList(&pDeviceExtension->ListHead, &pDeviceExtension->ListLock);
				}
#pragma warning(pop)
			}
		}
	};
}