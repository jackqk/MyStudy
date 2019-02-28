#pragma once
namespace ddk {
	// 路径写法
	// L"\\Registry\\Machine"
	// L"\\Registry\\User"
	// L"\\Registry\\Machine\\Software\\CLASSES"
	// L"\\Registry\\Machine\\System\\CurrentControlSet\\Hardware Profiles\\Current"
	class CReg
	{
	public://封装类型
		enum DDK_HKEY
		{
			HKEY_CLASSES_ROOT = 1,
			HKEY_CURRENT_USER,
			HKEY_LOCAL_MACHINE,
			HKEY_USERS,
			HKEY_CURRENT_CONFIG,
		};
	private://成员变量
		HANDLE m_hKey;
		WCHAR  m_szKey[MAX_PATH];
	public://构造析构、获得成员变量
		CReg() {
			m_hKey   = NULL;
		}
		CReg(WCHAR *szKey) : CReg(){
			open_if(szKey);
		}
		CReg(WCHAR *szKey, DDK_HKEY key) : CReg(){
			open_if(szKey, key);
		}
		~CReg() {
			close();
		}
		void close() {
			//打开、创建新的注册表句柄前，先关闭以前的。
			NTSTATUS status = STATUS_UNSUCCESSFUL;
			if (m_hKey)
				status = ZwClose(m_hKey);
			if (NT_SUCCESS(status))
			{
				m_hKey = NULL;
				RtlZeroMemory(m_szKey, MAX_PATH);
			}
		}
		HANDLE getHandle() {
			return m_hKey;
		}
		WCHAR *getPath() {
			return m_szKey;
		}
	public://key操作
		BOOL open_if(WCHAR *szKey) {
			BOOL ret = FALSE;
			NTSTATUS status;
			HANDLE tmpHandle;
			UNICODE_STRING usKeyName = { 0 };
			OBJECT_ATTRIBUTES oa = { 0 };
			RtlInitUnicodeString(&usKeyName, szKey);
			wcscpy_s(m_szKey, szKey);

			do
			{
				InitializeObjectAttributes(&oa, &usKeyName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
				ULONG ulResult = 0;
				status = ZwCreateKey(&tmpHandle,
					KEY_ALL_ACCESS,
					&oa,
					0,
					NULL,
					REG_OPTION_NON_VOLATILE,//重启后仍然在，
											// 如果使用REG_OPTION_VOLATILE，重启后，注册表就消失了。
					&ulResult);				//这里保存键状态：
											// REG_CREATED_NEW_KEY 新建的键！
											// REG_OPENED_EXISTING_KEY 已经存在的键
				if (!NT_SUCCESS(status))
				{
					ddk::log::StatusInfo::Print(status);
					break;
				}
				close();
				m_hKey = tmpHandle;
				ret = TRUE;
			} while (FALSE);
			return ret;
		}
		BOOL open_if(WCHAR *szKeyPath, DDK_HKEY key)
		{
			WCHAR szFullPath[MAX_PATH];
			
			switch (key)
			{
			case HKEY_CLASSES_ROOT:
				wcscpy_s(szFullPath, L"\\Registry\\Machine\\Software\\CLASSES");
				break;
			case HKEY_CURRENT_USER:
				//这个很复杂
				if (!get_current_user(szFullPath)) {
					return FALSE;
				}
				break;
			case HKEY_LOCAL_MACHINE:
				wcscpy_s(szFullPath, L"\\Registry\\Machine");
				break;
			case HKEY_USERS:
				wcscpy_s(szFullPath, L"\\Registry\\User");
				break;
			case HKEY_CURRENT_CONFIG:
				wcscpy_s(szFullPath, L"\\Registry\\Machine\\System\\CurrentControlSet\\Hardware Profiles\\Current");
				break;
			default:
				return false;
				break;
			}
			wcsncat_s(szFullPath, L"\\", 1);
			wcsncat_s(szFullPath, szKeyPath, wcslen(szKeyPath));
			return open_if(szFullPath);
		}
		BOOL del_key() {
			//这个只能删除空key
			NTSTATUS status;
			if (m_hKey) {
				status = ZwDeleteKey(m_hKey);
				if (NT_SUCCESS(status))
				{
					close();
					return TRUE;
				}
			}
			return FALSE;
		}
		//补充，删除带子Key的key
		static BOOL create(WCHAR *szKey) {
			BOOL ret = FALSE;
			NTSTATUS status;
			HANDLE tmpHandle;
			UNICODE_STRING usKeyName = { 0 };
			OBJECT_ATTRIBUTES oa = { 0 };
			RtlInitUnicodeString(&usKeyName, szKey);
			do
			{
				InitializeObjectAttributes(&oa, &usKeyName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
				ULONG ulResult = 0;
				status = ZwCreateKey(&tmpHandle,
					KEY_ALL_ACCESS,
					&oa,
					0,
					NULL,
					REG_OPTION_NON_VOLATILE,//重启后仍然在，
											// 如果使用REG_OPTION_VOLATILE，重启后，注册表就消失了。
					&ulResult);				//这里保存键状态：
											// REG_CREATED_NEW_KEY 新建的键！
											// REG_OPENED_EXISTING_KEY 已经存在的键
				if (!NT_SUCCESS(status))
				{
					ddk::log::StatusInfo::Print(status);
					break;
				}
				ZwClose(tmpHandle);
				ret = TRUE;
			} while (FALSE);
			return ret;
		}
	public://ValueKey操作
		BOOL set_value(WCHAR *value_name, ULONG val_type, PVOID value_data, size_t data_size)
		{
			NTSTATUS status = STATUS_UNSUCCESSFUL;
			if (m_hKey)
			{
				UNICODE_STRING usValueName = { 0 };
				RtlInitUnicodeString(&usValueName, value_name);
				status = ZwSetValueKey(m_hKey,
					&usValueName,
					0,
					val_type,
					value_data,
					(ULONG)data_size);
				if (NT_SUCCESS(status))
				{
					return TRUE;
				}
			}
			ddk::log::StatusInfo::Print(status);
			return FALSE;
		}
		BOOL get_value(WCHAR *value_name, ULONG val_type, PVOID value_buffer, size_t &buffer_size)
		{
			if (m_hKey)
			{
				ULONG ulSize = 0;
				UNICODE_STRING usValueName = { 0 };
				RtlInitUnicodeString(&usValueName, value_name);
				auto ns = ZwQueryValueKey(m_hKey,
					&usValueName,
					KeyValuePartialInformation,
					NULL,
					0,
					&ulSize);
				if (ns == STATUS_BUFFER_OVERFLOW || ns == STATUS_BUFFER_TOO_SMALL)
				{
					auto pkvpi = (PKEY_VALUE_PARTIAL_INFORMATION)malloc(ulSize);//CHK模式下是不能使用ExAllocatePool的
					if (pkvpi)
					{
						RtlZeroMemory(pkvpi, ulSize);
						ns = ZwQueryValueKey(m_hKey,
							&usValueName,
							KeyValuePartialInformation,
							pkvpi,
							ulSize,
							&ulSize);
						if (NT_SUCCESS(ns))
						{
							if (pkvpi->Type != val_type)
							{
								return false;
							}
							if (pkvpi->DataLength <= buffer_size)
							{
								buffer_size = pkvpi->DataLength;
								if (value_buffer)
									RtlCopyMemory(value_buffer, pkvpi->Data, pkvpi->DataLength);
								return TRUE;
							}
							buffer_size = pkvpi->DataLength;
						}
						else
							ddk::log::StatusInfo::Print(ns);
						free(pkvpi);
					}
				}
			}
			return FALSE;
		}
		BOOL del_value(WCHAR *value_name)
		{
			NTSTATUS status = STATUS_UNSUCCESSFUL;
			if (m_hKey)
			{
				UNICODE_STRING usValueName;
				RtlInitUnicodeString(&usValueName, value_name);
				status = ZwDeleteValueKey(m_hKey, &usValueName);
				if (status == STATUS_SUCCESS)
				{
					return TRUE;
				}
			}
			ddk::log::StatusInfo::Print(status);
			return FALSE;
		}
	private:
		BOOL get_current_user(WCHAR *path)
		{
			HANDLE hRegister;
			OBJECT_ATTRIBUTES ObjectAttributes;
			ULONG ulSize;
			UNICODE_STRING uniKeyName;
			WCHAR ProfileListbuf[256];
			UNICODE_STRING RegCurrentUser, RegUser;
			UNICODE_STRING RegProfileList, RegProf;
			RTL_QUERY_REGISTRY_TABLE paramTable[2];
			ULONG udefaultData = 0;
			ULONG uQueryValue;

			RtlZeroMemory(paramTable, sizeof(paramTable));

			paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
			paramTable[0].Name = L"RefCount";
			paramTable[0].EntryContext = &uQueryValue;
			paramTable[0].DefaultType = REG_DWORD;
			paramTable[0].DefaultData = &udefaultData;
			paramTable[0].DefaultLength = sizeof(ULONG);


			RtlInitUnicodeString(&RegProf, L"\\Registry\\Machine\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\");
			RtlInitUnicodeString(&RegUser, L"\\Registry\\User\\");
			RtlInitEmptyUnicodeString(&RegCurrentUser, path, 256 * sizeof(WCHAR));
			RtlInitEmptyUnicodeString(&RegProfileList, ProfileListbuf, 256 * sizeof(WCHAR));
			RtlCopyUnicodeString(&RegCurrentUser, &RegUser);
			RtlCopyUnicodeString(&RegProfileList, &RegProf);

			InitializeObjectAttributes(&ObjectAttributes, &RegProf, OBJ_CASE_INSENSITIVE, NULL, NULL);
			do
			{
				auto ns = ZwOpenKey(&hRegister, KEY_ALL_ACCESS, &ObjectAttributes);
				if (!NT_SUCCESS(ns))
				{
					break;
				}
				auto hkey_exit = std::experimental::make_scope_exit([&]() {ZwClose(hRegister); });
				ns = ZwQueryKey(hRegister, KeyFullInformation, NULL, 0, &ulSize);
				if (ns != STATUS_BUFFER_TOO_SMALL && ns != STATUS_BUFFER_OVERFLOW)
				{
					break;
				}
				auto pfi = (PKEY_FULL_INFORMATION)malloc(ulSize);
				auto pfi_exit = std::experimental::make_scope_exit([&]() {free(pfi); });
				ns = ZwQueryKey(hRegister, KeyFullInformation, pfi, ulSize, &ulSize);
				if (!NT_SUCCESS(ns))
				{
					break;
				}

				for (auto i = ULONG(0); i < pfi->SubKeys; i++)
				{
					ns = ZwEnumerateKey(hRegister,
						i,
						KeyBasicInformation,
						NULL,
						0,
						&ulSize);
					if (ns != STATUS_BUFFER_TOO_SMALL && ns != STATUS_BUFFER_OVERFLOW)
					{
						break;
					}
					auto pbi = (PKEY_BASIC_INFORMATION)malloc(ulSize);
					auto pbi_exit = std::experimental::make_scope_exit([&]() {free(pbi); });
					ns = ZwEnumerateKey(hRegister,
						i,
						KeyBasicInformation,
						pbi,
						ulSize,
						&ulSize);
					if (!NT_SUCCESS(ns))
					{
						break;
					}

					uniKeyName.Length = uniKeyName.MaximumLength = (USHORT)pbi->NameLength;
					uniKeyName.Buffer = pbi->Name;

					if (pbi->NameLength > 20)
					{
						RtlAppendUnicodeStringToString(&RegCurrentUser, &uniKeyName);
						RtlAppendUnicodeStringToString(&RegProfileList, &uniKeyName);
						RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE, RegProfileList.Buffer, paramTable, NULL, NULL);
						if (uQueryValue > 0)
							return TRUE;
					}
					RtlCopyUnicodeString(&RegCurrentUser, &RegUser);
					RtlCopyUnicodeString(&RegProfileList, &RegProf);
				}
			} while (FALSE);
			return FALSE;
		}
	};
}