#pragma once
namespace ddk
{
	namespace util {
		class SyscallUtil
		{
		private:
			ULONG PrevModeOffset;
			std::unordered_map<std::string, PVOID> _funcs;
			SyscallUtil()
			{

				PrevModeOffset = CommonUtil::GetPreviousModeOffset();
				build_syscall_table();
			}
			SyscallUtil(SyscallUtil &) = delete;
			SyscallUtil& operator = (SyscallUtil) = delete;
		public:
			~SyscallUtil()
			{
			}
			static SyscallUtil &getIntanceRef() {
				static SyscallUtil self;
				return self;
			}
			static SyscallUtil *getInstancePtr() {
				return &getIntanceRef();
			}
		public://ssdt相关
			inline PVOID getSysCall(std::string name)
			{

				auto pfn = ddk::util::CommonUtil::GetRoutineAddr((WCHAR *)std::wstring(name.begin(), name.end()).c_str());
				if (pfn)
				{
					_funcs.insert(std::make_pair(name, pfn));
				}
				return pfn;
			}
			static PKSERVICE_TABLE_DESCRIPTOR GetSSDT()
			{
				static PKSERVICE_TABLE_DESCRIPTOR Ssdt = nullptr;
				if (Ssdt)
				{
					return Ssdt;
				}
				PVOID ptrKiSystemCall64 = NULL;
				ptrKiSystemCall64 = (PVOID)__readmsr(0xC0000082);
				UCHAR PTRN_WALL_Ke[] = { 0x42, 0x3b, 0x44, 0x17, 0x10, 0x0f, 0x83 };
				LONG OFFS_WNO8_Ke = -0x19;
				LONG OFFS_WIN8_Ke = -0x16;
				LONG OFFS_WIN10_Ke = -0x27;
				auto ns = ddk::util::MemUtil::MmGenericPointerSearch((PUCHAR *)&Ssdt,
					((PUCHAR)ptrKiSystemCall64) - (1 * PAGE_SIZE),
					((PUCHAR)ptrKiSystemCall64) + (1 * PAGE_SIZE),
					PTRN_WALL_Ke,
					sizeof(PTRN_WALL_Ke),
					(g_OsIndex < OsIndex_8) ? OFFS_WNO8_Ke : (g_OsIndex < OsIndex_10_1607) ? OFFS_WIN8_Ke : OFFS_WIN10_Ke);

				if (NT_SUCCESS(ns))
				{
					LOG_DEBUG("SSDT %p\r\n", Ssdt);
					return Ssdt;
				}
				return nullptr;
			}
			static PVOID GetSSDTFunctionAddressByIndex(DWORD index)
			{
#ifdef _X86_
#define EX_FAST_REF_MASK	0x07
				//待补充
				return NULL;
#else
#define EX_FAST_REF_MASK	0x0f
				auto SystemTable = GetSSDT();
				if (index == DWORD(-1))
				{
					return nullptr;
				}
				auto OldFunction = (ULONG_PTR)SystemTable->OffsetToService;
				if (g_OsIndex < OsIndex_VISTA)
				{
					OldFunction += SystemTable->OffsetToService[index] & ~EX_FAST_REF_MASK;
					//	NewOffset = ((LONG)(Function - (ULONG_PTR)KeServiceDescriptorTable->OffsetToService)) | EX_FAST_REF_MASK + KeServiceDescriptorTable->OffsetToService[ssdtNumber] & EX_FAST_REF_MASK;
				}
				else
				{
					OldFunction += SystemTable->OffsetToService[index] >> 4;
					//NewOffset = (((LONG)(Function - (ULONG_PTR)KeServiceDescriptorTable->OffsetToService)) << 4) + KeServiceDescriptorTable->OffsetToService[ssdtNumber] & 0x0F;
				}
				return reinterpret_cast<PVOID>(OldFunction);
#endif // X64
			}
		public:
			//使用get获得routine地址，泛型填PVOID
			//auto pAddress = ddk::util::SyscallUtil::getIntanceRef().get<PVOID>("PsIsProtectedProcess");
			template<typename T>
			inline T get(const std::string& name)
			{
				auto iter = _funcs.find(name);
				if (iter != _funcs.end())
					return reinterpret_cast<T>(iter->second);
				else
				{
					auto pfn = getSysCall(name);
					if (pfn)
					{
						return  reinterpret_cast<T>(pfn);
					}
				}
				return reinterpret_cast<T>(nullptr);
			}
		public://给下面的宏使用
			template<typename T, typename... Args>
			inline NTSTATUS safeNativeCall(const std::string& name, Args&&... args)
			{
				auto pfn = SyscallUtil::get<T>(name);
				return pfn ? pfn(std::forward<Args>(args)...) : STATUS_ORDINAL_NOT_FOUND;
			}
			template<typename T, typename... Args>
			inline NTSTATUS safeSysCall(const std::string& name, Args&&... args)
			{
				auto pfn = SyscallUtil::get<T>(name);
				PUCHAR pPrevMode = (PUCHAR)PsGetCurrentThread() + CommonUtil::GetPreviousModeOffset();
				UCHAR prevMode = *pPrevMode;
				*pPrevMode = KernelMode;
				auto ret = pfn ? pfn(std::forward<Args>(args)...) : STATUS_ORDINAL_NOT_FOUND;
				*pPrevMode = prevMode;
				return ret;
			}
			template<typename T, typename... Args>
			inline auto safeCall(const std::string& name, Args&&... args) -> typename std::result_of<T(Args...)>::type
			{
				auto pfn = SyscallUtil::get<T>(name);
				return pfn ? pfn(std::forward<Args>(args)...) : (std::result_of<T(Args...)>::type)(0);
			}
		private:
			void build_syscall_table()
			{
				auto NtdllBase = ddk::util::PEUtil::LoadDll(L"\\SystemRoot\\System32\\ntdll.dll");
				auto exit_1 = std::experimental::make_scope_exit([&]() {
					if (NtdllBase)
						ddk::util::PEUtil::FreeDll(NtdllBase); });
				auto get_syscall_number = [=](auto FuncRva)
				{
					if (FuncRva)
					{
						PUCHAR Func = (PUCHAR)NtdllBase + FuncRva;
#ifdef _X86_
						// check for mov eax,imm32
						if (*Func == 0xB8)
						{
							// return imm32 argument (syscall numbr)
							return *(PULONG)((PUCHAR)Func + 1);
						}
#elif _AMD64_
						// check for mov eax,imm32
						if (*(Func + 3) == 0xB8)
						{
							// return imm32 argument (syscall numbr)
							return *(PULONG)(Func + 4);
						}
#endif
					}
					return DWORD(-1);
				};
				if (NtdllBase)
				{
					auto RVATOVA = [](auto _base_, auto _offset_) {
						return ((PUCHAR)(_base_)+(ULONG)(_offset_));
					};
					__try
					{
						PIMAGE_EXPORT_DIRECTORY pExport = NULL;

						PIMAGE_NT_HEADERS32 pHeaders32 = (PIMAGE_NT_HEADERS32)
							((PUCHAR)NtdllBase + ((PIMAGE_DOS_HEADER)NtdllBase)->e_lfanew);

						if (pHeaders32->FileHeader.Machine == IMAGE_FILE_MACHINE_I386)
						{
							// 32-bit image
							if (pHeaders32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress)
							{
								pExport = (PIMAGE_EXPORT_DIRECTORY)RVATOVA(
									NtdllBase,
									pHeaders32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress
								);
							}
						}
						else if (pHeaders32->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64)
						{
							// 64-bit image
							PIMAGE_NT_HEADERS64 pHeaders64 = (PIMAGE_NT_HEADERS64)
								((PUCHAR)NtdllBase + ((PIMAGE_DOS_HEADER)NtdllBase)->e_lfanew);

							if (pHeaders64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress)
							{
								pExport = (PIMAGE_EXPORT_DIRECTORY)RVATOVA(
									NtdllBase,
									pHeaders64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress
								);
							}
						}

						if (pExport)
						{
							PULONG AddressOfFunctions = (PULONG)RVATOVA(NtdllBase, pExport->AddressOfFunctions);
							PSHORT AddrOfOrdinals = (PSHORT)RVATOVA(NtdllBase, pExport->AddressOfNameOrdinals);
							PULONG AddressOfNames = (PULONG)RVATOVA(NtdllBase, pExport->AddressOfNames);
							ULONG i = 0;
							for (i = 0; i < pExport->NumberOfFunctions; i++)
							{
								auto func_name = std::string((char *)RVATOVA(NtdllBase, AddressOfNames[i]));

								if (func_name.size() > 2
									&& func_name.at(0) == 'N'&&func_name.at(1) == 't')
								{
									auto syscall_id = get_syscall_number(AddressOfFunctions[AddrOfOrdinals[i]]);
									auto pfn = GetSSDTFunctionAddressByIndex(syscall_id);
									if (pfn)
									{
										//LOG_DEBUG("load %s %p\r\n", func_name.c_str(), pfn);
										_funcs.insert(std::make_pair(func_name, pfn));
									}
								}
							}
						}
					}
					__except (EXCEPTION_EXECUTE_HANDLER)
					{

					}
				}
			}
		};
	}
};

//使用方法
//auto pAddress = ddk::util::SyscallUtil::getIntanceRef().get<PVOID>("PsIsProtectedProcess");
#define GET_IMPORT(name) (ddk::util::SyscallUtil::getIntanceRef().get<fn ## name>( #name ))

//使用方法
//1.声明：using fnIoThreadToProcess = std::function<PEPROCESS(PETHREAD)>;
//2.调用：PETHREAD pt;
//       SAFE_NATIVE_CALL(IoThreadToProcess, pt);
#define SAFE_NATIVE_CALL(name, ...) (ddk::util::SyscallUtil::getIntanceRef().safeNativeCall<fn ## name>( #name, __VA_ARGS__ ))
#define SAFE_CALL(name, ...) (ddk::util::SyscallUtil::getIntanceRef().safeCall<fn ## name>( #name, __VA_ARGS__ ))
#define SAFE_SYSCALL(name, ...) (ddk::util::SyscallUtil::getIntanceRef().safeSysCall<fn ## name>( #name, __VA_ARGS__ ))