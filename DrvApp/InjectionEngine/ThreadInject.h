#pragma once
//32位、64位通用
namespace ddk::injection {
	class ThreadInject
	{
	public:
		ThreadInject(){}
		~ThreadInject() {}
		static BOOL InjectDll(ULONG ProcessId, std::wstring dll_path)
		{
			PVOID shell_code = nullptr;
			if (PayLoad::MakeShellcodeLoadDll((HANDLE)ProcessId, &shell_code, dll_path))
				return PayLoad::ExecuteThread((HANDLE)ProcessId, shell_code, nullptr);
			else
				return FALSE;
		}
	};
}