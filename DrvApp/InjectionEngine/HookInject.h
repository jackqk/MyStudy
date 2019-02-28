#pragma once
//32位、64位通用
namespace ddk::injection {
#pragma warning(disable:4311 4302)
	class HookInject
	{
	public:
		HookInject()
		{
			is_dll_32 = is_dll_64 = false;
			PsNtDllBase64 = fnProtectVirtualMemory64 = fnHookFunc64 = fnLdrLoadDll64 = nullptr;
			PsNtDllBase = fnProtectVirtualMemory = fnHookFunc = fnLdrLoadDll = nullptr;
			m_set_filter = false;
			ddk::callback::CImageCallback::getInstancePtr()->RegCallback(std::bind(&ddk::injection::HookInject::ImageCallback, this, std::placeholders::_1,
				std::placeholders::_2, std::placeholders::_3));
		}
		~HookInject()
		{
			m_set_filter = false;
		}
	public:
		bool SetHookInject(WCHAR *dll_path32, WCHAR *dll_path64, CHAR *process_name)
		{
			if (m_set_filter)
			{
				//已经设置过了！！
				return false;
			}
			if (dll_path32)
			{
				dll32 = dll_path32;
				is_dll_32 = true;
			}
			if (dll_path64)
			{
				dll64 = dll_path64;
				is_dll_64 = true;
			}
			filter_name = process_name;
			m_set_filter = true;
			return true;
		}
	private:
		void ImageCallback(__in_opt PUNICODE_STRING  FullImageName,__in HANDLE  ProcessId,__in PIMAGE_INFO  ImageInfo)
		{
			
			if (ImageInfo->SystemModeImage)
			{
				return;
			}
			if (ProcessId == HANDLE(0) || ProcessId == HANDLE(4))
			{
				return;
			}

			if (m_set_filter)
			{
				PEPROCESS Process = nullptr;
				auto ns = PsLookupProcessByProcessId(ProcessId, &Process);
				if (NT_SUCCESS(ns))
				{
					const int MaxLength = 15;
					//ProcessName会返回奇怪的串，重新弄下
					auto ProcessName = PsGetProcessImageFileName(Process);
					CHAR szProcess[MaxLength] = { 0 };
					BOOL bSet0 = FALSE;
					//改造下
					for (int i = 0; i < MaxLength; i++)
					{
						if (bSet0)
						{
							szProcess[i] = '\0';
							i++;
							continue;
						}
						int remainLength = MaxLength -1 - i - 1;
						if (ProcessName[0] == '.' && remainLength >= 3)
						{
							if (ProcessName[1] == 'e' && ProcessName[2] == 'x' && ProcessName[3] == 'e')
								memcpy(&szProcess[i], ProcessName, 4);
							i += 4;
							ProcessName += 4;
							continue;
						}
						szProcess[i] = *ProcessName;
						ProcessName++;
					}
					
					CHAR szCmpProcess[MaxLength] = { 0 };
					RtlCopyMemory(szCmpProcess, filter_name.c_str(), MaxLength - 1);//最后一位'\0'

					if (_strnicmp(szProcess, szCmpProcess, MaxLength) == 0)//指定进程
					{
						//加载的dll来分辨是32位还是64位
						UNICODE_STRING wow64NtdllName =	RTL_CONSTANT_STRING(L"SysWOW64\\ntdll.dll");
						UNICODE_STRING ntdllName = RTL_CONSTANT_STRING(L"\\SystemRoot\\System32\\ntdll.dll");
						if (is_dll_64 && RtlEqualUnicodeString(FullImageName, &ntdllName,TRUE))
								InjectHookDll64(Process, ImageInfo->ImageBase);
						if (is_dll_32 && RtlCheckImageName(FullImageName, &wow64NtdllName))
								InjectHookDll32(Process, ImageInfo->ImageBase);
					}
					ObDereferenceObject(Process);
				}
			}
		}
		void InjectHookDll64(PEPROCESS Process, PVOID ImageBase)
		{
			
			UNICODE_STRING dllPath;
			RtlInitUnicodeString(&dllPath, dll64.c_str());
			if (PsGetProcessWow64Process(Process))
			{
				//64位注入，只针对64位进程!!
				return;
			}
			HANDLE ProcessHandle;
			auto ns = ObOpenObjectByPointer(Process,
				OBJ_KERNEL_HANDLE,
				NULL,
				PROCESS_ALL_ACCESS,
				*PsProcessType,
				KernelMode,
				(PHANDLE)&ProcessHandle
			);
			if (!NT_SUCCESS(ns))
			{
				return;
			}
			if (!PsNtDllBase64)
			{
				PsNtDllBase64 = ImageBase;
			}
			if (!fnLdrLoadDll64 || !fnHookFunc64 || !fnProtectVirtualMemory64)
			{
				KAPC_STATE kApc;
				KeStackAttachProcess(Process, &kApc);
				fnProtectVirtualMemory64 = PayLoad::BBGetModuleExport(ImageBase, "ZwProtectVirtualMemory");
				fnLdrLoadDll64 = PayLoad::BBGetModuleExport(ImageBase, "LdrLoadDll");
				fnHookFunc64 = PayLoad::BBGetModuleExport(ImageBase, "ZwTestAlert");
				KeUnstackDetachProcess(&kApc);
			}
			if (fnLdrLoadDll64 && fnHookFunc64 && fnProtectVirtualMemory64)
			{
				PayLoad::PINJECT_BUFFER pBuffer = GetInlineHookCode64(ProcessHandle, &dllPath);
				if (pBuffer)
				{
					UCHAR trampo[] = { 0xE9, 0, 0, 0, 0 };
					ULONG OldProtect = 0;
					PVOID ProtectAddress = fnHookFunc64;
					SIZE_T ProtectSize = sizeof(trampo);

					*(DWORD *)(trampo + 1) = (DWORD)((ULONG_PTR)pBuffer->code - ((ULONG_PTR)fnHookFunc64 + 5));
					
					auto status = FGNtProtectVirtualMemory(ProcessHandle, &ProtectAddress, &ProtectSize, PAGE_EXECUTE_READWRITE, &OldProtect);
					if (NT_SUCCESS(status))
					{
						ULONG ReturnLength = 0;
						FGNtWriteVirtualMemory(ProcessHandle, fnHookFunc64, trampo, sizeof(trampo), &ReturnLength);
						FGNtProtectVirtualMemory(ProcessHandle, &ProtectAddress, &ProtectSize, OldProtect, &OldProtect);
					}
				}
			}
			ZwClose(ProcessHandle);
			return;
		}
		void InjectHookDll32(PEPROCESS Process, PVOID ImageBase)
		{
			LOG_DEBUG("dll inject 32\r\n");
			UNICODE_STRING dllPath;
			RtlInitUnicodeString(&dllPath, dll32.c_str());
			if (!PsGetProcessWow64Process(Process))
			{
				return;
			}
			HANDLE ProcessHandle;
			auto ns = ObOpenObjectByPointer(Process,
				OBJ_KERNEL_HANDLE,
				NULL,
				PROCESS_ALL_ACCESS,
				*PsProcessType,
				KernelMode,
				(PHANDLE)&ProcessHandle
			);
			if (!NT_SUCCESS(ns))
			{
				return;
			}
			if (!PsNtDllBase)
			{
				PsNtDllBase = ImageBase;
			}
			if (!fnLdrLoadDll || !fnHookFunc || !fnProtectVirtualMemory)
			{
				KAPC_STATE kApc;
				KeStackAttachProcess(Process, &kApc);
				fnProtectVirtualMemory = PayLoad::BBGetModuleExport(ImageBase, "ZwProtectVirtualMemory");
				fnLdrLoadDll = PayLoad::BBGetModuleExport(ImageBase, "LdrLoadDll");
				fnHookFunc = PayLoad::BBGetModuleExport(ImageBase, "ZwTestAlert");
				KeUnstackDetachProcess(&kApc);
			}
			if (fnLdrLoadDll && fnHookFunc && fnProtectVirtualMemory)
			{
				PayLoad::PINJECT_BUFFER pBuffer = GetInlineHookCode(ProcessHandle, &dllPath);
				if (pBuffer)
				{
					UCHAR trampo[] = { 0xE9, 0, 0, 0, 0 };
					ULONG OldProtect = 0;
					PVOID ProtectAddress = fnHookFunc;
					SIZE_T ProtectSize = sizeof(trampo);

					*(DWORD *)(trampo + 1) = (DWORD)((DWORD)pBuffer->code - ((DWORD)fnHookFunc + 5));

					auto status = FGNtProtectVirtualMemory(ProcessHandle, &ProtectAddress, &ProtectSize, PAGE_EXECUTE_READWRITE, &OldProtect);
					if (NT_SUCCESS(status))
					{
						ULONG ReturnLength = 0;
						FGNtWriteVirtualMemory(ProcessHandle, fnHookFunc, trampo, sizeof(trampo), &ReturnLength);
						FGNtProtectVirtualMemory(ProcessHandle, &ProtectAddress, &ProtectSize, OldProtect, &OldProtect);
					}
				}
			}
			ZwClose(ProcessHandle);
		}
		PayLoad::PINJECT_BUFFER GetInlineHookCode64(IN HANDLE ProcessHandle, IN PUNICODE_STRING pDllPath)
		{
			NTSTATUS status = STATUS_UNSUCCESSFUL;
			PayLoad::PINJECT_BUFFER pBuffer = NULL;
			PayLoad::INJECT_BUFFER Buffer = { 0 };

			//Try to allocate before ntdll.dll
			pBuffer = (PayLoad::PINJECT_BUFFER)PayLoad::AllocateInjectMemory(ProcessHandle, (PVOID)PsNtDllBase64, PAGE_SIZE);
			if (pBuffer != NULL)
			{
				status = FGNtReadVirtualMemory(ProcessHandle, fnHookFunc64, Buffer.original_code, sizeof(Buffer.original_code), NULL);
				if (NT_SUCCESS(status))
				{

					const UCHAR HookCode64[] = {
						0x50, 							// push       rax
						0x51, 							// push       rcx
						0x52, 							// push       rdx
						0x41, 0x50, 					// push       r8
						0x41, 0x51, 					// push       r9
						0x41, 0x53, 					// push       r11
						0x48, 0x83, 0xEC, 0x38,			// sub         rsp,38h
						0x48, 0x8B, 0x05, 0, 0, 0, 0,	// mov         rax,qword ptr [fnHookPort]	offset+16
						0x4C, 0x8D, 0x44, 0x24, 0x48,   // lea         r8,[rsp + 48h]
						0x48, 0x89, 0x44, 0x24, 0x50,	// mov         qword ptr [rsp+50h],rax  
						0x48, 0x8D, 0x54, 0x24, 0x50,   // lea         rdx,[rsp + 50h]
						0x48, 0x8D, 0x44, 0x24, 0x40,   // lea         rax,[rsp + 40h]
						0x48, 0xC7, 0x44, 0x24, 0x48,   // mov         qword ptr[rsp + 48h],5
						5, 0, 0, 0,
						0x41, 0xB9, 0x40, 0, 0, 0,		// mov         r9d,40h
						0x48, 0x89, 0x44, 0x24, 0x20,	// mov         qword ptr[rsp + 20h],rax
						0x48, 0x83, 0xC9, 0xFF,			// or          rcx, 0FFFFFFFFFFFFFFFFh
						0xE8, 0, 0, 0, 0,				// call		   fnProtectVirtualMemory		 offset +65
						0x8B, 0x05, 0, 0, 0, 0,			// mov         eax,dword ptr[fnOriCode]		offset+71
						0x4C, 0x8D, 0x44, 0x24, 0x48,   // lea         r8,[rsp + 48h]
						0x48, 0x8B, 0x15, 0, 0, 0, 0,	// mov         rdx,qword ptr[fnHookPort]	 offset+83
						0x48, 0x83, 0xC9, 0xFF,			// or          rcx, 0FFFFFFFFFFFFFFFFh
						0x89, 0x02,						// mov         dword ptr[rdx],eax
						0x0F, 0xB6, 0x05, 0, 0, 0, 0,	// movzx       eax,byte ptr[fnOriCode+4]	offset+96
						0x88, 0x42, 0x04,	            // mov         byte ptr[rdx + 4],al
						0x48, 0x8D, 0x44, 0x24, 0x40,   // lea         rax,[rsp + 40h]
						0x44, 0x8B, 0x4C, 0x24, 0x40,   // mov         r9d,dword ptr[rsp + 40h]
						0x48, 0x8D, 0x54, 0x24, 0x50,	// lea         rdx,[rsp + 50h]
						0x48, 0x89, 0x44, 0x24, 0x20,	// mov         qword ptr [rsp+20h],rax
						0xE8, 0, 0, 0, 0,				// call        fnProtectVirtualMemory		offset +124
						0x4C, 0x8D, 0x0D, 0, 0, 0, 0,	// lea         r9,qword ptr [pModuleHandle]  offset+131
						0x33, 0xD2,						// xor         edx,edx
						0x4C, 0x8D, 0x05, 0, 0, 0, 0,	// lea         r8,qword ptr [pModuleName]	 offset+140			
						0x33, 0xC9,						// xor         ecx,ecx
						0xE8, 0, 0, 0, 0,				// call        fnLdrLoadDll					 offset +147
						0x48, 0x83, 0xC4, 0x38,			// add         rsp,38h
						0x41, 0x5B, 					// pop        r11
						0x41, 0x59, 					// pop        r9
						0x41, 0x58, 					// pop        r8
						0x5A, 							// pop        rdx
						0x59, 							// pop        rcx
						0x58, 							// pop        rax
						0xE9, 0, 0, 0, 0, 				// jmp        OriFunc offset+165
						0xCC,							// padding
					};
					// Fill data
					Buffer.path.Length = min(pDllPath->Length, sizeof(Buffer.buffer));
					Buffer.path.MaximumLength = min(pDllPath->MaximumLength, sizeof(Buffer.buffer));
					Buffer.path.Buffer = (PWCH)pBuffer->buffer;
					Buffer.hook_func = fnHookFunc64;
					memcpy(Buffer.buffer, pDllPath->Buffer, Buffer.path.Length);
					memcpy(Buffer.code, HookCode64, sizeof(HookCode64));

					// Fill code
					*(ULONG*)((PUCHAR)Buffer.code + 16) = (ULONG)((ULONGLONG)&pBuffer->hook_func - ((ULONGLONG)pBuffer + 20));
					*(ULONG*)((PUCHAR)Buffer.code + 65) = (ULONG)((ULONGLONG)fnProtectVirtualMemory64 - ((ULONGLONG)pBuffer + 69));
					*(ULONG*)((PUCHAR)Buffer.code + 71) = (ULONG)((ULONGLONG)pBuffer->original_code - ((ULONGLONG)pBuffer + 75));
					*(ULONG*)((PUCHAR)Buffer.code + 83) = (ULONG)((ULONGLONG)&pBuffer->hook_func - ((ULONGLONG)pBuffer + 87));
					*(ULONG*)((PUCHAR)Buffer.code + 96) = (ULONG)((ULONGLONG)(pBuffer->original_code + 4) - ((ULONGLONG)pBuffer + 100));
					*(ULONG*)((PUCHAR)Buffer.code + 124) = (ULONG)((ULONGLONG)fnProtectVirtualMemory64 - ((ULONGLONG)pBuffer + 128));
					*(ULONG*)((PUCHAR)Buffer.code + 131) = (ULONG)((ULONGLONG)&pBuffer->module - ((ULONGLONG)pBuffer + 135));
					*(ULONG*)((PUCHAR)Buffer.code + 140) = (ULONG)((ULONGLONG)&pBuffer->path - ((ULONGLONG)pBuffer + 144));
					*(ULONG*)((PUCHAR)Buffer.code + 147) = (ULONG)((ULONGLONG)fnLdrLoadDll64 - ((ULONGLONG)pBuffer + 151));
					*(ULONG*)((PUCHAR)Buffer.code + 165) = (ULONG)((ULONGLONG)fnHookFunc64 - ((ULONGLONG)pBuffer + 169));

					//Write all
					FGNtWriteVirtualMemory(ProcessHandle, pBuffer, &Buffer, sizeof(Buffer), NULL);

					return pBuffer;
				}
				else
				{
					LOG_DEBUG("%s: Failed to read original code %X\n", __FUNCTION__, status);
				}
			}
			else
			{
				LOG_DEBUG("%s: Failed to allocate memory\n", __FUNCTION__);
			}
			return NULL;
		}
		PayLoad::PINJECT_BUFFER GetInlineHookCode(IN HANDLE ProcessHandle, IN PUNICODE_STRING pDllPath)
		{
			NTSTATUS status = STATUS_UNSUCCESSFUL;
			PayLoad::PINJECT_BUFFER pBuffer = NULL;
			PayLoad::INJECT_BUFFER Buffer = { 0 };

			//Try to allocate before ntdll.dll
			pBuffer = (PayLoad::PINJECT_BUFFER)PayLoad::AllocateInjectMemory(ProcessHandle, (PVOID)PsNtDllBase, PAGE_SIZE);
			if (pBuffer != NULL)
			{
				status = FGNtReadVirtualMemory(ProcessHandle, fnHookFunc, Buffer.original_code, sizeof(Buffer.original_code), NULL);
				if (NT_SUCCESS(status))
				{
					const UCHAR HookCode[] =
					{
						0x55,									// push        ebp
						0x8B, 0xEC,								// mov         ebp,esp
						0x83, 0xEC, 0x0C,						// sub         esp,0Ch
						0xA1, 0, 0, 0, 0,						// mov         eax,dword ptr[fnHookFunc] //offset +7
						0x89, 0x45, 0xF4,						// mov         dword ptr[ebp-0Ch],eax
						0x8D, 0x45, 0xFC,						// lea         eax,[ebp - 4]
						0x50,									// push        eax
						0x6A, 0x40,								// push        40h
						0x8D, 0x45, 0xF8,						// lea         eax,[ebp - 8]
						0xC7, 0x45, 0xF8, 5, 0, 0, 0,			// mov         dword ptr[ebp - 8],5
						0x50,									// push        eax
						0x8D, 0x45, 0xF4,						// lea         eax,[ebp - 0Ch]
						0x50,									// push        eax
						0x6A, 0xFF,								// push        0FFFFFFFFh
						0xE8, 0, 0, 0, 0,						// call        NtProtectVirtualMemory //offset +38
						0x8B, 0x0D, 0, 0, 0, 0,					// mov         ecx,dword ptr ds : [fnHookFunc] //offset + 44
						0xA1, 0, 0, 0, 0,						// mov         eax,dword ptr ds : [fnOriCode] //offset + 49
						0x89, 0x01,								// mov         dword ptr[ecx],eax
						0xA0, 0, 0, 0, 0,						// mov         al,byte ptr ds : [fnOriCode+4] //offset +56
						0x88, 0x41, 0x04,						// mov         byte ptr[ecx + 4],al
						0x8D, 0x45, 0xFC,						// lea         eax,[ebp-4]
						0x50,									// push        eax
						0xFF, 0x75, 0xFC,						// push        dword ptr[ebp-4]
						0x8D, 0x45, 0xF8,						// lea         eax,[ebp - 8]
						0x50,									// push        eax
						0x8D, 0x45, 0xF4,						// lea         eax,[ebp - 0Ch]
						0x50,									// push        eax
						0x6A, 0xFF,								// push        0FFFFFFFFh
						0xE8, 0, 0, 0, 0,                       // call        NtProtectVirtualMemory //offset +81
						0x68, 0, 0, 0, 0,                       // push        ModuleHandle           //offset +86
						0x68, 0, 0, 0, 0,                       // push        ModuleFileName         //offset +91
						0x6A, 0,                                // push        0  
						0x6A, 0,                                // push        0
						0xE8, 0, 0, 0, 0,                       // call        LdrLoadDll              //offset +100
						0x8B, 0xE5,								// mov         esp,ebp
						0x5D,									// pop         ebp
						0xE9, 0, 0, 0, 0,						// jmp								   //offset+108
						0xCC,									// padding
					};
					// Fill data
					Buffer.path32.Length = min(pDllPath->Length, sizeof(Buffer.buffer));
					Buffer.path32.MaximumLength = min(pDllPath->MaximumLength, sizeof(Buffer.buffer));
					Buffer.path32.Buffer = (ULONG)pBuffer->buffer;
					Buffer.hook_func = fnHookFunc;
					memcpy(Buffer.buffer, pDllPath->Buffer, Buffer.path32.Length);
					memcpy(Buffer.code, HookCode, sizeof(HookCode));

					// Fill code
					*(DWORD*)((PUCHAR)Buffer.code + 7) = (DWORD)&pBuffer->hook_func;
					*(DWORD*)((PUCHAR)Buffer.code + 38) = (DWORD)((DWORD)fnProtectVirtualMemory - ((DWORD)pBuffer + 42));
					*(DWORD*)((PUCHAR)Buffer.code + 44) = (DWORD)&pBuffer->hook_func;
					*(DWORD*)((PUCHAR)Buffer.code + 49) = (DWORD)pBuffer->original_code;
					*(DWORD*)((PUCHAR)Buffer.code + 56) = (DWORD)pBuffer->original_code + 4;
					*(DWORD*)((PUCHAR)Buffer.code + 81) = (DWORD)((DWORD)fnProtectVirtualMemory - ((DWORD)pBuffer + 85));
					*(DWORD*)((PUCHAR)Buffer.code + 86) = (DWORD)&pBuffer->module;
					*(DWORD*)((PUCHAR)Buffer.code + 91) = (DWORD)&pBuffer->path32;
					*(DWORD*)((PUCHAR)Buffer.code + 100) = (DWORD)((DWORD)fnLdrLoadDll - ((DWORD)pBuffer + 104));
					*(DWORD*)((PUCHAR)Buffer.code + 108) = (DWORD)((DWORD)fnHookFunc - ((DWORD)pBuffer + 112));

					// Copy all
					FGNtWriteVirtualMemory(ProcessHandle, pBuffer, &Buffer, sizeof(Buffer), NULL);

					return pBuffer;
				}
				else
				{
					LOG_DEBUG("%s: Failed to read original code %X\n", __FUNCTION__, status);
				}
			}
			else
			{
				LOG_DEBUG("%s: Failed to allocate memory\n", __FUNCTION__);
			}

			return NULL;
		}
		BOOLEAN RtlCheckImageName(IN PUNICODE_STRING ImageName,	IN PUNICODE_STRING CheckString)
		{
			UNICODE_STRING imageName = *ImageName;
			USHORT wcharsToSkip;
			PAGED_CODE();
			if (imageName.Length < CheckString->Length) {
				return FALSE;
			}
			wcharsToSkip = (imageName.Length - CheckString->Length) / sizeof(WCHAR);
			imageName.Buffer += wcharsToSkip;
			imageName.Length -= wcharsToSkip * sizeof(WCHAR);
			imageName.MaximumLength -= wcharsToSkip * sizeof(WCHAR);
			return RtlEqualUnicodeString(&imageName, CheckString, TRUE);
		}
	private:
		bool m_set_filter;
		std::wstring dll64;
		bool is_dll_64;
		std::wstring dll32;
		bool is_dll_32;
		std::string filter_name;
		PVOID fnLdrLoadDll64;
		PVOID fnProtectVirtualMemory64;
		PVOID fnHookFunc64;
		PVOID PsNtDllBase64;
		PVOID fnLdrLoadDll;
		PVOID fnProtectVirtualMemory;
		PVOID fnHookFunc;
		PVOID PsNtDllBase;
	};
}