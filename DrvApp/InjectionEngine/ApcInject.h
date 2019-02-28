#pragma once
//32位可以、64位有问题
namespace ddk::injection {
	class ApcInject
	{
	public:
		ApcInject() {}
		~ApcInject() {}
	public:
		static BOOL InjectDll(ULONG ProcessId, std::wstring dll_path)
		{
			PVOID shellcode = nullptr;
			auto flag = ddk::injection::PayLoad::MakeShellcodeLoadDll((HANDLE)ProcessId, &shellcode, dll_path);
			if (flag)
				return InjectCode((HANDLE)ProcessId, shellcode);
			return false;
		}
	private:
		static BOOL InjectCode(HANDLE ProcessId, PVOID shellcode)
		{
			PETHREAD thread = nullptr;
			auto flag = GetProcessThread(ProcessId, thread);
			if (!flag)
				return false;
			return QueryApc(thread, shellcode, nullptr, nullptr, nullptr, true);
		}
	private:
		static BOOL GetProcessThread(HANDLE ProcessId, PETHREAD &ret_thread)
		{
			ret_thread = nullptr;
			//查找合适的thread进行apc注入
			PSYSTEM_PROCESS_INFORMATION FindInfo = nullptr;
			auto pFin = ddk::util::CommonUtil::GetSysInfo(SystemProcessInformation);
			auto pInfo = reinterpret_cast<PSYSTEM_PROCESS_INFORMATION>(pFin);
			if (!pInfo)
			{
				return false;
			}
			auto fin = pFin;
			auto exit_1 = std::experimental::make_scope_exit([&]() {if (fin)free(fin); });

			for (;;)
			{
				if (pInfo->UniqueProcessId == ProcessId)
				{
					FindInfo = pInfo;
					break;
				}

				if (pInfo->NextEntryOffset == 0)
				{
					break;
				}
				pInfo = reinterpret_cast<PSYSTEM_PROCESS_INFORMATION>((ULONG_PTR)pInfo + pInfo->NextEntryOffset);
			}

			if (!FindInfo)
				return false;

			for (auto i = 0; i < FindInfo->NumberOfThreads; i++)
			{
				if (FindInfo->thread_info[i].ClientId.UniqueThread == PsGetCurrentThreadId())
				{
					continue;
				}
				PETHREAD thread = nullptr;
				auto ns = PsLookupThreadByThreadId(FindInfo->thread_info[i].ClientId.UniqueThread, &thread);
				if (NT_SUCCESS(ns))
				{
					ret_thread = thread;
					break;
				}
			}
			if (ret_thread)
				return true;
			return false;
		}
		static BOOL QueryApc(PETHREAD pThread, PVOID shellcode, PVOID Arg1, PVOID Arg2, PVOID Arg3, BOOL force)
		{
			//先插入个注入代码APC，再插一个激活代码的APC
			PKAPC pPrepareApc = nullptr;
			auto pInjectApc = reinterpret_cast<PKAPC>(malloc(sizeof(KAPC)));
			if (!pInjectApc)
			{
				return FALSE;
			}
			KeInitializeApc(pInjectApc, (PKTHREAD)pThread, OriginalApcEnvironment, &KernelApcInjectCallback,
				NULL, (PKNORMAL_ROUTINE)(ULONG_PTR)shellcode, UserMode, Arg1);
			if (force)
			{
				pPrepareApc = reinterpret_cast<PKAPC>(malloc(sizeof(KAPC)));
				KeInitializeApc(pPrepareApc, (PKTHREAD)pThread, OriginalApcEnvironment, &KernelApcPrepareCallback,
					NULL, NULL, KernelMode, NULL);
			}

			if (KeInsertQueueApc(pInjectApc, Arg2, Arg3, 0))
			{
				if (force && pPrepareApc)
					KeInsertQueueApc(pPrepareApc, NULL, NULL, 0);
				return TRUE;
			}

			else
			{
				free(pInjectApc);
				if (pPrepareApc)
					free(pPrepareApc);
			}
			return FALSE;
		}
		static VOID NTAPI KernelApcInjectCallback(
			PRKAPC Apc,
			PKNORMAL_ROUTINE* NormalRoutine,
			PVOID* NormalContext,
			PVOID* SystemArgument1,
			PVOID* SystemArgument2)
		{
			UNREFERENCED_PARAMETER(SystemArgument1);
			UNREFERENCED_PARAMETER(SystemArgument2);
			// Skip execution
			if (PsIsThreadTerminating(PsGetCurrentThread()))
				*NormalRoutine = NULL;

			// Fix Wow64 APC
			if (PsGetCurrentProcessWow64Process() != NULL)
				PsWrapApcWow64Thread(NormalContext, (PVOID*)NormalRoutine);

			free(Apc);
		}
		static VOID NTAPI KernelApcPrepareCallback(
			PRKAPC Apc,
			PKNORMAL_ROUTINE* NormalRoutine,
			PVOID* NormalContext,
			PVOID* SystemArgument1,
			PVOID* SystemArgument2)
		{
			UNREFERENCED_PARAMETER(NormalRoutine);
			UNREFERENCED_PARAMETER(NormalContext);
			UNREFERENCED_PARAMETER(SystemArgument1);
			UNREFERENCED_PARAMETER(SystemArgument2);
			//调用这个就可以让用户层调用APC代码
			KeTestAlertThread(UserMode);
			free(Apc);
		}
	};
}