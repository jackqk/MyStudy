#pragma once

namespace ddk::callback {
	class CObjectCallback
	{
	private:
		typedef struct _OBJECT_TYPE_INITIALIZER                                                                                                                                                                                                       // 32 elements, 0x78 bytes (sizeof) 
		{
			UINT16 Length;
			union                                                                                                                                                                                                                                     // 2 elements, 0x2 bytes (sizeof)   
			{
				UINT16 ObjectTypeFlags;
				struct                                                                                                                                                                                                                                // 2 elements, 0x2 bytes (sizeof)   
				{
					struct                                                                                                                                                                                                                            // 8 elements, 0x1 bytes (sizeof)   
					{
						UINT8        CaseInsensitive : 1;                                                                                                                                                                                             // 0 BitPosition                    
						UINT8        UnnamedObjectsOnly : 1;                                                                                                                                                                                          // 1 BitPosition                    
						UINT8        UseDefaultObject : 1;                                                                                                                                                                                            // 2 BitPosition                    
						UINT8        SecurityRequired : 1;                                                                                                                                                                                            // 3 BitPosition                    
						UINT8        MaintainHandleCount : 1;                                                                                                                                                                                         // 4 BitPosition                    
						UINT8        MaintainTypeList : 1;                                                                                                                                                                                            // 5 BitPosition                    
						UINT8        SupportsObjectCallbacks : 1;                                                                                                                                                                                     // 6 BitPosition                    
						UINT8        CacheAligned : 1;                                                                                                                                                                                                // 7 BitPosition                    
					};
					struct                                                                                                                                                                                                                            // 2 elements, 0x1 bytes (sizeof)   
					{
						UINT8        UseExtendedParameters : 1;                                                                                                                                                                                       // 0 BitPosition                    
						UINT8        Reserved : 7;                                                                                                                                                                                                    // 1 BitPosition                    
					};
				};
			};
			ULONG32      ObjectTypeCode;
			ULONG32      InvalidAttributes;
			struct _GENERIC_MAPPING GenericMapping;                                                                                                                                                                                                   // 4 elements, 0x10 bytes (sizeof)  
			ULONG32      ValidAccessMask;
			ULONG32      RetainAccess;
			enum _POOL_TYPE PoolType;
			ULONG32      DefaultPagedPoolCharge;
			ULONG32      DefaultNonPagedPoolCharge;
			PVOID DumpProcedure;
			PVOID OpenProcedure;
			PVOID CloseProcedure;
			PVOID DeleteProcedure;
			union                                                                                                                                                                                                                                     // 2 elements, 0x8 bytes (sizeof)   
			{
				PVOID* ParseProcedure;
				PVOID* ParseProcedureEx;
			};
			PVOID SecurityProcedure;
			PVOID QueryNameProcedure;
			PVOID OkayToCloseProcedure;
			ULONG32      WaitObjectFlagMask;
			UINT16       WaitObjectFlagOffset;
			UINT16       WaitObjectPointerOffset;
		}OBJECT_TYPE_INITIALIZER, *POBJECT_TYPE_INITIALIZER;
		typedef struct _OBJECT_TYPE                   // 12 elements, 0xD0 bytes (sizeof) 
		{
			struct _LIST_ENTRY TypeList;              // 2 elements, 0x10 bytes (sizeof)  
			struct _UNICODE_STRING Name;              // 3 elements, 0x10 bytes (sizeof)  
			VOID*        DefaultObject;
			UINT8        Index;
			UINT8        _PADDING0_[0x3];
			ULONG32      TotalNumberOfObjects;
			ULONG32      TotalNumberOfHandles;
			ULONG32      HighWaterNumberOfObjects;
			ULONG32      HighWaterNumberOfHandles;
			UINT8        _PADDING1_[0x4];
			struct _OBJECT_TYPE_INITIALIZER TypeInfo; // 25 elements, 0x70 bytes (sizeof) 
			ULONG_PTR TypeLock;            // 7 elements, 0x8 bytes (sizeof)   
			ULONG32      Key;
			UINT8        _PADDING2_[0x4];
			struct _LIST_ENTRY CallbackList;          // 2 elements, 0x10 bytes (sizeof)  
		}OBJECT_TYPE_X, *POBJECT_TYPE_X;

	private://成员变量
		using pfnPreCallback  = std::function<OB_PREOP_CALLBACK_STATUS(PVOID, POB_PRE_OPERATION_INFORMATION)>;
		using pfnPostCallback = std::function<VOID(PVOID, POB_POST_OPERATION_INFORMATION)>;
		PVOID m_pRegistionHandle;
		pfnPreCallback  m_preFun;
		pfnPostCallback m_postFun;
		CObjectCallback() {
			m_preFun = NULL;
			m_postFun = NULL;
			m_pRegistionHandle = NULL;
		}
	public://构造、析构、注册回调
		CObjectCallback(POBJECT_TYPE *ObjType, ULONG Altitude) {
			//ObjType：ExEventObjectType、ExSemaphoreObjectType、IoFileObjectType、PsProcessType
			//PsThreadType、SeTokenObjectType、TmEnlistmentObjectType、TmResourceManagerObjectType
			//TmTransactionManagerObjectType、TmTransactionObjectType
			OB_CALLBACK_REGISTRATION  CallBackReg = {};
			OB_OPERATION_REGISTRATION OperationReg = {};

			CallBackReg.Version = ObGetFilterVersion();
			CallBackReg.OperationRegistrationCount = 1;
			CallBackReg.RegistrationContext = this;

			WCHAR buf[MAX_PATH];
			CallBackReg.Altitude.Length = CallBackReg.Altitude.MaximumLength = MAX_PATH * 2;
			CallBackReg.Altitude.Buffer = buf;
			RtlIntegerToUnicodeString(Altitude, 10, &CallBackReg.Altitude);
			
			if (ObjType != PsProcessType && ObjType != PsThreadType)
			{
				//其他objectType需要设置CallBackFlag!!!
				auto pObjectType = reinterpret_cast<POBJECT_TYPE_X>(*ObjType);
				__try
				{
					pObjectType->TypeInfo.SupportsObjectCallbacks = 1;
				}
				__except (1)
				{
					
				}
			}
			OperationReg.ObjectType = ObjType;
			OperationReg.Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;

			OperationReg.PreOperation = PreCallback;
			OperationReg.PostOperation = PostCallback;
			CallBackReg.OperationRegistration = &OperationReg;
			ObRegisterCallbacks(&CallBackReg, &m_pRegistionHandle);
		}
		~CObjectCallback() {
			if (m_pRegistionHandle)
			{
				ObUnRegisterCallbacks(m_pRegistionHandle);
				m_pRegistionHandle = NULL;
			}
		}
		VOID RegCallback(pfnPreCallback preCallback, pfnPostCallback postCallback) {
			if (preCallback)
				m_preFun = preCallback;
			if (postCallback)
				m_postFun = postCallback;
		}
	private:
		static OB_PREOP_CALLBACK_STATUS PreCallback(PVOID RegistrationContext,
			_In_ POB_PRE_OPERATION_INFORMATION OperationInformation) {
			auto pThis = reinterpret_cast<CObjectCallback *>(RegistrationContext);
			if (pThis->m_preFun)
				return pThis->m_preFun(RegistrationContext, OperationInformation);
			return OB_PREOP_SUCCESS;
		}
		static VOID PostCallback(PVOID RegistrationContext,
			POB_POST_OPERATION_INFORMATION OperationInformation) {
			auto pThis = reinterpret_cast<CObjectCallback *>(RegistrationContext);
			if (pThis->m_postFun)
				pThis->m_postFun(RegistrationContext, OperationInformation);
		}
	};
}