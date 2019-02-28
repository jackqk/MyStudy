#pragma once
//32位、64位都不行
namespace ddk
{
	namespace injection {
		class MemInject
		{
		public:
			MemInject() {

			}
			~MemInject() {

			}
			static bool InjectDll(ULONG ProcessId, WCHAR *dll_path)
			{
				auto file = ddk::CFile(dll_path);
				auto file_size = ULONG(file.get_file_size());
				auto pfile_data = new CHAR[file_size];
				if (pfile_data)
				{
					file.read((PVOID)pfile_data, file_size);
					auto b_2 = mem_load_dll((HANDLE)ProcessId, pfile_data, file_size);
					delete[] pfile_data;
					return b_2;
				}
				return false;
			}
		private:
			static bool mem_load_dll(HANDLE ProcessId, PVOID dll_data, ULONG dll_size)
			{
				//仅支持32位！！64位dll手工加载参考BlackBone里的那个复杂的代码
				//shellcode版内存加载器！！
				PVOID shell_code = nullptr;
				auto b_1 = PayLoad::MakeShellcodeMemLoad32(ProcessId, &shell_code, dll_data, dll_size);
				if (b_1)
				{
					return PayLoad::ExecuteThread(ProcessId, shell_code, nullptr);
				}
				return false;
			}
		};
	}
};