#pragma once
namespace ddk::util {
	class PEUtil {
	public:
		//传入EPROCESS、以及dll名称，获得dll的imageBase（基地址）。
		static PVOID GetUserModuleHandle(IN PEPROCESS pProcess, IN PUNICODE_STRING ModuleName, IN BOOLEAN isWow64)
		{
			//RtlInitUnicodeString(&dll, L"ntdll.dll");
			//KAPC_STATE apcState;
			//status = PsLookupProcessByProcessId((HANDLE)2940, &Process);
			//KeStackAttachProcess(Process, &apcState);
			//auto addr1 = BBGetUserModule(Process, &dll, TRUE);
			//KeUnstackDetachProcess(&apcState);
			int i;
			if (pProcess == NULL)
				return NULL;

			// Protect from UserMode AV
			__try
			{
				LARGE_INTEGER time = { 0 };
				time.QuadPart = -250ll * 10 * 1000;     // 250 msec.
				if (isWow64)
				{
					PLIST_ENTRY32 pListEntry;
					PPEB32 pPeb32 = (PPEB32)PsGetProcessWow64Process(pProcess);
					if (pPeb32 == NULL)
					{
						LOG_DEBUG("%s: No PEB present. Aborting\n", __FUNCTION__);
						return NULL;
					}

					// Wait for loader a bit
					for (i = 0; !pPeb32->Ldr && i < 10; i++)
					{
						LOG_DEBUG("%s: Loader not intialiezd, waiting\n", __FUNCTION__);
						KeDelayExecutionThread(KernelMode, TRUE, &time);
					}

					// Still no loader
					if (!pPeb32->Ldr)
					{
						LOG_DEBUG("%s: Loader was not intialiezd in time. Aborting\n", __FUNCTION__);
						return NULL;
					}

					// Search in InLoadOrderModuleList
					for (pListEntry = (PLIST_ENTRY32)((PPEB_LDR_DATA32)pPeb32->Ldr)->InLoadOrderModuleList.Flink;
						pListEntry != &((PPEB_LDR_DATA32)pPeb32->Ldr)->InLoadOrderModuleList;
						pListEntry = (PLIST_ENTRY32)pListEntry->Flink)
					{
						UNICODE_STRING ustr;
						PLDR_DATA_TABLE_ENTRY32 pEntry = CONTAINING_RECORD(pListEntry, LDR_DATA_TABLE_ENTRY32, InLoadOrderLinks);

						RtlUnicodeStringInit(&ustr, (PWCH)pEntry->BaseDllName.Buffer);

						if (RtlCompareUnicodeString(&ustr, ModuleName, TRUE) == 0)
							return (PVOID)pEntry->DllBase;
					}
				}
				// Native process
				else
				{
					PLIST_ENTRY pListEntry;
					PPEB pPeb = (PPEB)PsGetProcessPeb(pProcess);
					if (!pPeb)
					{
						LOG_DEBUG("%s: No PEB present. Aborting\n", __FUNCTION__);
						return NULL;
					}

					// Wait for loader a bit
					for (i = 0; !pPeb->Ldr && i < 10; i++)
					{
						LOG_DEBUG("%s: Loader not intialiezd, waiting\n", __FUNCTION__);
						KeDelayExecutionThread(KernelMode, TRUE, &time);
					}

					// Still no loader
					if (!pPeb->Ldr)
					{
						LOG_DEBUG("%s: Loader was not intialiezd in time. Aborting\n", __FUNCTION__);
						return NULL;
					}

					// Search in InLoadOrderModuleList
					for (pListEntry = pPeb->Ldr->InLoadOrderModuleList.Flink;
						pListEntry != &pPeb->Ldr->InLoadOrderModuleList;
						pListEntry = pListEntry->Flink)
					{
						PLDR_DATA_TABLE_ENTRY pEntry = CONTAINING_RECORD(pListEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
						if (RtlCompareUnicodeString(&pEntry->BaseDllName, ModuleName, TRUE) == 0)
							return pEntry->DllBase;
					}

				}

			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				LOG_DEBUG("%s: Exception, Code: 0x%X\n", __FUNCTION__, GetExceptionCode());
			}

			return NULL;
		}
		//传入imageBase（基地址），获得PE在内存中的大小
		static ULONG LdrGetImageSize(PVOID imageBase)
		{
			//从PE文件中获得镜像大小
			auto size = 0UL;
			do
			{
				__try
				{
					auto dosHeader = (PIMAGE_DOS_HEADER)imageBase;
					if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
					{
						break;
					}
					auto ntheader = (PIMAGE_NT_HEADERS)((PUCHAR)imageBase + dosHeader->e_lfanew);
					if (ntheader->Signature != IMAGE_NT_SIGNATURE)
					{
						break;
					}
					if (ntheader)
					{
					}
					size = (ntheader->OptionalHeader.SizeOfCode);
				}
				__except (1)
				{
					return 0UL;
				}
			} while (0);

			return size;
		}
		//参数一：imageBase（基地址）
		//参数二：函数名
		//返回：函数地址。
		static PVOID GetProcAddressR0(IN PVOID pBase, IN PCHAR funName)
		{
			//使用：GetUserModuleHandle先取得基地址，在调用此函数
			PIMAGE_DOS_HEADER pDosHdr = (PIMAGE_DOS_HEADER)pBase;
			PIMAGE_NT_HEADERS32 pNtHdr32 = NULL;
			PIMAGE_NT_HEADERS64 pNtHdr64 = NULL;
			PIMAGE_EXPORT_DIRECTORY pExport = NULL;
			ULONG expSize = 0;
			ULONG_PTR pAddress = 0;
			PUSHORT pAddressOfOrds;
			PULONG  pAddressOfNames;
			PULONG  pAddressOfFuncs;
			ULONG i;

			ASSERT(pBase != NULL);
			if (pBase == NULL)
				return NULL;

			/// Not a PE file
			if (pDosHdr->e_magic != IMAGE_DOS_SIGNATURE)
				return NULL;

			pNtHdr32 = (PIMAGE_NT_HEADERS32)((PUCHAR)pBase + pDosHdr->e_lfanew);
			pNtHdr64 = (PIMAGE_NT_HEADERS64)((PUCHAR)pBase + pDosHdr->e_lfanew);

			// Not a PE file
			if (pNtHdr32->Signature != IMAGE_NT_SIGNATURE)
				return NULL;

			// 64 bit image
			if (pNtHdr32->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
			{
				pExport = (PIMAGE_EXPORT_DIRECTORY)(pNtHdr64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + (ULONG_PTR)pBase);
				expSize = pNtHdr64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
			}
			// 32 bit image
			else
			{
				pExport = (PIMAGE_EXPORT_DIRECTORY)(pNtHdr32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + (ULONG_PTR)pBase);
				expSize = pNtHdr32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
			}

			pAddressOfOrds = (PUSHORT)(pExport->AddressOfNameOrdinals + (ULONG_PTR)pBase);
			pAddressOfNames = (PULONG)(pExport->AddressOfNames + (ULONG_PTR)pBase);
			pAddressOfFuncs = (PULONG)(pExport->AddressOfFunctions + (ULONG_PTR)pBase);

			for (i = 0; i < pExport->NumberOfFunctions; ++i)
			{
				USHORT OrdIndex = 0xFFFF;
				PCHAR  pName = NULL;

				// Find by index
				if ((ULONG_PTR)funName <= 0xFFFF)
				{
					OrdIndex = (USHORT)i;
				}
				// Find by name
				else if ((ULONG_PTR)funName > 0xFFFF && i < pExport->NumberOfNames)
				{
					pName = (PCHAR)(pAddressOfNames[i] + (ULONG_PTR)pBase);
					OrdIndex = pAddressOfOrds[i];
				}
				// Weird params
				else
					return NULL;

				if (((ULONG_PTR)funName <= 0xFFFF && (USHORT)((ULONG_PTR)funName) == OrdIndex + pExport->Base) ||
					((ULONG_PTR)funName > 0xFFFF && strcmp(pName, funName) == 0))
				{
					pAddress = pAddressOfFuncs[OrdIndex] + (ULONG_PTR)pBase;

					// Check forwarded export
					if (pAddress >= (ULONG_PTR)pExport && pAddress <= (ULONG_PTR)pExport + expSize)
					{
						return NULL;
					}

					break;
				}
			}

			return (PVOID)pAddress;
		}
	public:
		// 加载DLL（PE展开了）
		static PVOID LoadDll(WCHAR *path)
		{
			#define SEC_IMAGE 0x1000000  
			HANDLE hSection = NULL, hFile = NULL;
			UNICODE_STRING dllName;
			PVOID BaseAddress = NULL;
			SIZE_T size = 0;
			NTSTATUS status;
			OBJECT_ATTRIBUTES oa = { sizeof(oa), NULL, &dllName, OBJ_CASE_INSENSITIVE };
			IO_STATUS_BLOCK iosb;

			do
			{
				RtlInitUnicodeString(&dllName, path);
				status = ZwOpenFile(&hFile, FILE_EXECUTE | SYNCHRONIZE, &oa, &iosb,
					FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_NONALERT);
				if (!NT_SUCCESS(status)) {
					ddk::log::StatusInfo::Print(status);
					break;
				}

				status = ZwCreateSection(&hSection, SECTION_ALL_ACCESS, NULL, NULL, PAGE_EXECUTE,
					SEC_IMAGE, hFile);//最后两个参数是关键,逆向LoadLibrary可以得到
				if (!NT_SUCCESS(status)) {
					ddk::log::StatusInfo::Print(status);
					break;
				}

				status = ZwMapViewOfSection(hSection, NtCurrentProcess(), &BaseAddress, 0,
					1000, 0, &size, SECTION_INHERIT::ViewShare, 0/*MEM_TOP_DOWN*/, PAGE_READWRITE);
				if (!NT_SUCCESS(status)) {
					ddk::log::StatusInfo::Print(status);
					break;
				}
			} while (FALSE);

			if (hSection)
				ZwClose(hSection);
			if (hFile)
				ZwClose(hFile);
			return BaseAddress;
		}
		// 卸载DLL
		static void FreeDll(HANDLE hMod)
		{
			ZwUnmapViewOfSection(NtCurrentProcess(), hMod);
		}
	};
}