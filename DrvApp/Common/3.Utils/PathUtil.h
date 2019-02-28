#pragma once

namespace ddk::util {
	class PathUtil {
		//获得文件路径
	public:
		//传入fileObject，获得全路径
		static BOOL GetFileObjectFullPath(IN PFILE_OBJECT fileobject, OUT PUNICODE_STRING usFullPath)
		{
			bool relate_name = false;
			auto relate_file_object = fileobject->RelatedFileObject;
			auto IsValidUnicodeString = [](PUNICODE_STRING pstr)->decltype(relate_name) {
				bool bRc = false;
				ULONG   ulIndex = 0;
				__try
				{
					if (!MmIsAddressValid(pstr))
						return false;

					if ((NULL == pstr->Buffer) || (0 == pstr->Length))
						return false;

					for (ulIndex = 0; ulIndex < pstr->Length; ulIndex++)
					{
						if (!MmIsAddressValid((UCHAR *)pstr->Buffer + ulIndex))
							return false;
					}

					bRc = true;
				}
				__except (EXCEPTION_EXECUTE_HANDLER)
				{
					bRc = false;
				}
				return bRc;
			};

			auto is_file_object_name = IsValidUnicodeString(&fileobject->FileName);
			auto is_relate_object_name = relate_file_object ? IsValidUnicodeString(&relate_file_object->FileName) : false;

			if (is_relate_object_name && is_file_object_name)
			{
				if (fileobject->FileName.Buffer[0] != L'\\')
					relate_name = true;
			}
			//if (!KeAreAllApcsDisabled() && //《--ObQueryNameString的崩溃问题，如果Apcs全部Disable下...
			//	// Windows 10: Apcs disabled, Kernel apcs enabled, ObQueryString() does not crash
			//	// Windows 7: Apcs disabled, Kernel apcs disabled (leads to crash if ObQueryNameString() is called)
			if (is_file_object_name)
			{
				NTSTATUS status;
				DEVICE_OBJECT* VolumeDeviceObject = NULL;
				if (fileobject->Vpb != NULL &&
					fileobject->Vpb->RealDevice != NULL) {
					VolumeDeviceObject = fileobject->Vpb->RealDevice;
				}
				else {
					VolumeDeviceObject = fileobject->DeviceObject;
				}
				ULONG ReturnLength = 0;
				status = ObQueryNameString(VolumeDeviceObject, NULL, 0, &ReturnLength);
				if (ReturnLength == 0) {
					return false;
				}
				POBJECT_NAME_INFORMATION NameInfo =
					(POBJECT_NAME_INFORMATION)malloc(ReturnLength);
				if (!NameInfo)
				{
					return false;
				}
				auto free_nameinfo = std::experimental::make_scope_exit([&]() {free(NameInfo); });
				status = ObQueryNameString(VolumeDeviceObject,
					(POBJECT_NAME_INFORMATION)NameInfo,
					ReturnLength,
					&ReturnLength);
				if (NT_SUCCESS(status))
				{
					//\Device\HarddiskVolume2\Windows\System32\notepad.exe
					UNICODE_STRING* usDriverName = &NameInfo->Name;
					UNICODE_STRING usSymbolName = { 0 };
					WCHAR SymbolBuffer[16] = { L"\\??\\X:" };
					RtlInitUnicodeString(&usSymbolName, SymbolBuffer);
					for (WCHAR c = L'A'; c < (L'Z' + 1); ++c)
					{
						usSymbolName.Buffer[wcslen(L"\\??\\")] = c;
						OBJECT_ATTRIBUTES oa;
						InitializeObjectAttributes(
							&oa,
							&usSymbolName,
							OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
							NULL, NULL);

						HANDLE hSymbol;
						status = ZwOpenSymbolicLinkObject(
							&hSymbol,
							GENERIC_READ,
							&oa);

						if (!NT_SUCCESS(status)) {
							continue;
						}

						WCHAR TargetBuffer[64] = { 0 };
						UNICODE_STRING usTarget = { 0 };
						RtlInitEmptyUnicodeString(&usTarget, TargetBuffer, sizeof(TargetBuffer));

						ULONG ReturnLength;
						status = ZwQuerySymbolicLinkObject(hSymbol, &usTarget, &ReturnLength);

						ZwClose(hSymbol);
						hSymbol = NULL;

						if (NT_SUCCESS(status)) {

							if (0 == RtlCompareUnicodeString(usDriverName, &usTarget, FALSE)) {

								RtlCopyUnicodeString(usFullPath, &usSymbolName);
								if (relate_name)
								{
									if (relate_file_object->FileName.Buffer[0] != L'\\'
										&& (L'\0' != relate_file_object->FileName.Buffer[0]))
									{
										RtlAppendUnicodeToString(usFullPath, L"\\");
									}
									RtlAppendUnicodeStringToString(usFullPath, &relate_file_object->FileName);
								}
								if (fileobject->FileName.Buffer[0] != L'\\'
									&&fileobject->FileName.Buffer[0] != L'\0')
								{
									RtlAppendUnicodeToString(usFullPath, L"\\");
								}
								RtlAppendUnicodeStringToString(usFullPath, &fileobject->FileName);
								return true;
							}
						}
					}
				}
			}
			return false;
		}
		//这个还未实验，不知道结果
		static BOOL GetFileFullPath(PUNICODE_STRING inPath, PUNICODE_STRING outPath)
		{
			OBJECT_ATTRIBUTES oa = { 0 };
			InitializeObjectAttributes(&oa, inPath,
				/*OBJ_CASE_INSENSITIVE |*/ OBJ_KERNEL_HANDLE, NULL, NULL);

			IO_STATUS_BLOCK iosb = { 0 };
			HANDLE  FileHandle = NULL;
			auto ns = ZwOpenFile(&FileHandle,
				GENERIC_READ | SYNCHRONIZE,
				&oa, &iosb,
				FILE_SHARE_VALID_FLAGS,
				FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE);
			if (NT_SUCCESS(ns)) {
				PFILE_OBJECT file_object = nullptr;
				ns = ObReferenceObjectByHandle(FileHandle,
					0,
					*IoFileObjectType,
					KernelMode,
					(PVOID *)&file_object,
					NULL);
				ZwClose(FileHandle);
				FileHandle = NULL;
				if (NT_SUCCESS(ns))
				{
					auto ret = GetFileObjectFullPath(file_object, outPath);
					ObDereferenceObject(file_object);
					return ret;
				}
			}
			return false;
		}
	public://盘符转换
		// "\\DosDevices\\c:" 或 "\\??\\c:" ===>  \\Device\\HarddiskVolume2 
		static BOOL SymbolName2DeviceName(IN const UNICODE_STRING *symbolName, OUT UNICODE_STRING *deviceName)
		{
			//返回成功 注意要释放LinkTarget.Buffer
			OBJECT_ATTRIBUTES oa;
			NTSTATUS status;
			HANDLE handle;
			InitializeObjectAttributes(
				&oa,
				(PUNICODE_STRING)symbolName,
				OBJ_CASE_INSENSITIVE,
				0,
				0);
			status = ZwOpenSymbolicLinkObject(&handle, GENERIC_READ, &oa);
			if (!NT_SUCCESS(status))
				return FALSE;

			deviceName->MaximumLength = 1024 * sizeof(WCHAR);
			deviceName->Length = 0;
			deviceName->Buffer = (PWCH)ExAllocatePoolWithTag(PagedPool, deviceName->MaximumLength, 'A0');
			if (!deviceName->Buffer)
			{
				ZwClose(handle);
				return FALSE;
			}
			RtlZeroMemory(deviceName->Buffer, deviceName->MaximumLength);
			status = ZwQuerySymbolicLinkObject(handle, deviceName, NULL);
			ZwClose(handle);
			if (!NT_SUCCESS(status))
			{
				ExFreePool(deviceName->Buffer);
				return FALSE;
			}
			return TRUE;
		}
		static BOOL SymbolName2DeviceName(IN WCHAR *symbolName, OUT WCHAR *deviceName) {
			BOOL ret = FALSE;
			NTSTATUS status;
			HANDLE hSymbol;
			OBJECT_ATTRIBUTES oa;
			UNICODE_STRING ucSymbolName;
			UNICODE_STRING ucDeviceName = { 0 };
			RtlInitUnicodeString(&ucSymbolName, symbolName);

			do
			{
				InitializeObjectAttributes(&oa, &ucSymbolName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, 0, 0);
				status = ZwOpenSymbolicLinkObject(&hSymbol, GENERIC_ALL, &oa);
				if (!NT_SUCCESS(status))
					break;
				ucDeviceName.MaximumLength = 1024 * sizeof(WCHAR);
				ucDeviceName.Length = 0;
				ucDeviceName.Buffer = (PWCH)ExAllocatePoolWithTag(PagedPool, ucDeviceName.MaximumLength, 'A0');
				if (!ucDeviceName.Buffer)
					break;
				RtlZeroMemory(ucDeviceName.Buffer, ucDeviceName.MaximumLength);
				status = ZwQuerySymbolicLinkObject(hSymbol, &ucDeviceName, NULL);
				if (!NT_SUCCESS(status))
					break;
				//UNICODE_STRING不是以'\0'字符结尾的
				RtlCopyMemory(deviceName, ucDeviceName.Buffer, ucDeviceName.Length + 2);
				ret = TRUE;
			} while (FALSE);

			if (ret == FALSE)
				LOG_DEBUG("错误 = %x \r\n", status);
			if (hSymbol)
				ZwClose(hSymbol);

			if (ucDeviceName.Buffer)
				ExFreePool(ucDeviceName.Buffer);
			return ret;
		}
		// \\Device\\harddiskvolume1 ===> C:
		static BOOL DeviceName2DosName(IN PUNICODE_STRING devName, OUT PUNICODE_STRING dosName)
		{
			BOOL                    ret = FALSE;
			UNICODE_STRING			driveLetterName = { 0 };
			WCHAR					driveLetterNameBuf[128] = { 0 };
			WCHAR					c = L'\0';
			WCHAR					DriLetter[3] = { 0 };
			UNICODE_STRING			linkTarget = { 0 };

			for (c = L'A'; c <= L'Z'; c++)
			{
				RtlInitEmptyUnicodeString(&driveLetterName, driveLetterNameBuf, sizeof(driveLetterNameBuf));
				RtlAppendUnicodeToString(&driveLetterName, L"\\??\\");
				DriLetter[0] = c;
				DriLetter[1] = L':';
				DriLetter[2] = 0;

				RtlAppendUnicodeToString(&driveLetterName, DriLetter);
				ret = SymbolName2DeviceName(&driveLetterName, &linkTarget);
				if (!ret)
					continue;

				if (RtlEqualUnicodeString(&linkTarget, devName, TRUE))
				{
					ExFreePool(linkTarget.Buffer);
					break;
				}
				ExFreePool(linkTarget.Buffer);
			}

			if (c <= L'Z')
			{
				dosName->Buffer = (PWCH)ExAllocatePoolWithTag(PagedPool, 3 * sizeof(WCHAR), 'SOD');
				if (!dosName->Buffer)
					return STATUS_INSUFFICIENT_RESOURCES;
				dosName->MaximumLength = 6;
				dosName->Length = 4;
				*dosName->Buffer = c;
				*(dosName->Buffer + 1) = ':';
				*(dosName->Buffer + 2) = 0;
				return TRUE;
			}
			return FALSE;
		}
		static BOOL DeviceName2DosName(IN WCHAR *devName, OUT WCHAR *dosName) {
			UNICODE_STRING uDevName;
			UNICODE_STRING uDosName;
			RtlInitUnicodeString(&uDevName, devName);
			if (DeviceName2DosName(&uDevName, &uDosName))
			{
				RtlCopyMemory(dosName, uDosName.Buffer, uDosName.Length);
				ExFreePool(uDosName.Buffer);
				return TRUE;
			}
			return FALSE;
		}
	public://路径转换
		// \\??\\c:\\windows\\hi.txt ===> \\device\\harddiskvolume1\\windows\\hi.txt
		static BOOL SymbolPath2DevicePath(IN WCHAR *symbolPath, OUT WCHAR *devPath)
		{
			UNICODE_STRING uVolName = { 0,0,0 };
			WCHAR volName[MAX_PATH] = L"";
			WCHAR tmpName[MAX_PATH] = L"";
			WCHAR chVol = L'\0';
			WCHAR * pPath = NULL;
			int i = 0;


			RtlStringCbCopyW(tmpName, MAX_PATH * sizeof(WCHAR), symbolPath);

			for (i = 1; i < MAX_PATH - 1; i++)
			{
				if (tmpName[i] == L':')
				{
					pPath = &tmpName[(i + 1) % MAX_PATH];
					chVol = tmpName[i - 1];
					break;
				}
			}

			if (pPath == NULL)
			{
				return FALSE;
			}

			if (chVol == L'?')
			{
				uVolName.Length = 0;
				uVolName.MaximumLength = MAX_PATH * sizeof(WCHAR);
				uVolName.Buffer = devPath;
				RtlAppendUnicodeToString(&uVolName, L"\\Device\\HarddiskVolume?");
				RtlAppendUnicodeToString(&uVolName, pPath);
				return TRUE;
			}
			else if (QueryVolumeName(chVol, volName, MAX_PATH * sizeof(WCHAR)))
			{
				uVolName.Length = 0;
				uVolName.MaximumLength = MAX_PATH * sizeof(WCHAR);
				uVolName.Buffer = devPath;
				RtlAppendUnicodeToString(&uVolName, volName);
				RtlAppendUnicodeToString(&uVolName, pPath);
				return TRUE;
			}
			return FALSE;
		}
		// \\device\\harddiskvolume1\\windows\\hi.txt ===> c:\\windows\\hi.txt
		static BOOL DevicePath2DosPath(IN WCHAR *devPath, OUT WCHAR *dosPath)
		{
			UNICODE_STRING		ustrFileName = { 0 };
			UNICODE_STRING		ustrDosName = { 0 };
			UNICODE_STRING		ustrDeviceName = { 0 };
			WCHAR				tmpDevPath[MAX_PATH] = { 0 };
			WCHAR				*pPath = NULL;
			ULONG				i = 0;
			ULONG				ulSepNum = 0;

			wcscpy_s(tmpDevPath, devPath);
			if (dosPath == NULL ||
				tmpDevPath == NULL ||
				_wcsnicmp(tmpDevPath, L"\\device\\harddiskvolume", wcslen(L"\\device\\harddiskvolume")) != 0)
				return FALSE;
			ustrFileName.Buffer = dosPath;
			ustrFileName.Length = 0;
			ustrFileName.MaximumLength = sizeof(WCHAR)*MAX_PATH;

			while (tmpDevPath[i] != L'\0')
			{
				if (tmpDevPath[i] == L'\0')
					break;
				if (tmpDevPath[i] == L'\\')
					ulSepNum++;
				if (ulSepNum == 3)
				{
					tmpDevPath[i] = UNICODE_NULL;
					pPath = &tmpDevPath[i + 1];
					break;
				}
				i++;
			}

			if (pPath == NULL)
				return FALSE;

			RtlInitUnicodeString(&ustrDeviceName, tmpDevPath);
			if (!DeviceName2DosName(&ustrDeviceName, &ustrDosName))
				return FALSE;

			RtlCopyUnicodeString(&ustrFileName, &ustrDosName);
			RtlAppendUnicodeToString(&ustrFileName, L"\\");
			RtlAppendUnicodeToString(&ustrFileName, pPath);
			ExFreePool(ustrDosName.Buffer);
			return TRUE;
		}
		// \\??\\c:\\windows\\hi.txt ===> c:\\windows\\hi.txt
		static BOOL SymbolPath2DosPath(IN WCHAR *symbolPath, OUT WCHAR *dosPath) {
			WCHAR devPath[MAX_PATH] = { 0 };
			if (SymbolPath2DevicePath(symbolPath, devPath)) {
				return DevicePath2DosPath(devPath, dosPath);
			}
			return FALSE;
		}
	public://长短路径
		// 判断是否是短路径
		BOOLEAN IsShortNamePath(WCHAR * wszFileName)
		{
			WCHAR *p = wszFileName;
			while (*p != L'\0')
			{
				if (*p == L'~')
				{
					return TRUE;
				}
				p++;
			}
			return FALSE;
		}
		// 短路径--->长路径：c:\\Program\\ax\by\\hi~1.txt ====> c:\\Program\\ax\by\\hiz.txt
		BOOLEAN ConverShortToLongName(WCHAR *wszLongName, WCHAR *wszShortName, ULONG longNameSize)
		{

			//先把根目录拷贝到目标目录中，剩下的找到下一级目录是否含有~，如果有，则开始转化。
			//如：c:\\Progam\\a~1\\b~1\hi~1.txt
			//pchStart指向目录中前一个\\,pchEnd扫描并指向目录的下一个\\，其中如果发现了~，则是短名，需要转换。
			//传c:\\Program\\a~1-->c:\\Progam\\ax
			//传c:\\Program\\ax\\b~1-->c:\\Program\\ax\\by
			//传c:\\Program\\ax\by\\hi~1.txt-->c:\\Program\\ax\by\\hiz.txt
			WCHAR                 *szResult = NULL;
			WCHAR                 *pchResult = NULL;
			WCHAR                 *pchStart = wszShortName;
			INT                           Offset = 0;
			szResult = (WCHAR *)ExAllocatePoolWithTag(PagedPool,
				sizeof(WCHAR) * (MAX_PATH * 2 + 1),
				'L2S');
			if (szResult == NULL)
			{
				return FALSE;
			}
			RtlZeroMemory(szResult, sizeof(WCHAR) * (MAX_PATH * 2 + 1));
			pchResult = szResult;
			//C:\\x\\-->\\??\\c:
			if (pchStart[0] && pchStart[1] == L':')
			{
				*pchResult++ = L'\\';
				*pchResult++ = L'?';
				*pchResult++ = L'?';
				*pchResult++ = L'\\';
				*pchResult++ = *pchStart++;
				*pchResult++ = *pchStart++;
				Offset = 4;
			}
			//\\DosDevices\\c:\\xx-->\\??\\c:
			else if (_wcsnicmp(pchStart, L"\\DosDevices\\", 12) == 0)
			{
				RtlStringCbCopyW(pchResult, sizeof(WCHAR) * (MAX_PATH * 2 + 1), L"\\??\\");
				pchResult += 4;
				pchStart += 12;
				while (*pchStart && !IsDirectorySep(*pchStart))
					*pchResult++ = *pchStart++;
				Offset = 4;
			}
			//\\Device\\HarddiskVolume1\\xx-->\\Device\\HarddiskVolume1
			else if (_wcsnicmp(pchStart, L"\\Device\\HardDiskVolume", 22) == 0)
			{
				RtlStringCbCopyW(pchResult, sizeof(WCHAR) * (MAX_PATH * 2 + 1), L"\\Device\\HardDiskVolume");
				pchResult += 22;
				pchStart += 22;
				while (*pchStart && !IsDirectorySep(*pchStart))
					*pchResult++ = *pchStart++;
			}
			//\\??\\c:\\xx-->\\??\\c:
			else if (_wcsnicmp(pchStart, L"\\??\\", 4) == 0)
			{
				RtlStringCbCopyW(pchResult, sizeof(WCHAR) * (MAX_PATH * 2 + 1), L"\\??\\");
				pchResult += 4;
				pchStart += 4;
				while (*pchStart && !IsDirectorySep(*pchStart))
					*pchResult++ = *pchStart++;
			}
			else
			{
				ExFreePool(szResult);
				return FALSE;
			}
			while (IsDirectorySep(*pchStart))
			{
				BOOLEAN               bShortName = FALSE;
				WCHAR                 *pchEnd = NULL;
				WCHAR                 *pchReplacePos = NULL;
				*pchResult++ = *pchStart++;
				pchEnd = pchStart;
				pchReplacePos = pchResult;
				while (*pchEnd && !IsDirectorySep(*pchEnd))
				{
					if (*pchEnd == L'~')
					{
						bShortName = TRUE;
					}
					*pchResult++ = *pchEnd++;
				}
				*pchResult = L'\0';
				if (bShortName)
				{
					WCHAR  * szLong = NULL;
					szLong = (WCHAR *)ExAllocatePoolWithTag(PagedPool,
						sizeof(WCHAR) * MAX_PATH,
						'L2S');
					if (szLong)
					{
						RtlZeroMemory(szLong, sizeof(WCHAR) * MAX_PATH);
						if (QueryLongName(szResult, szLong, sizeof(WCHAR) * MAX_PATH))
						{
							RtlStringCbCopyW(pchReplacePos, sizeof(WCHAR) * (MAX_PATH * 2 + 1), szLong);
							pchResult = pchReplacePos + wcslen(pchReplacePos);
						}
						ExFreePool(szLong);
					}
				}
				pchStart = pchEnd;
			}
			wcsncpy(wszLongName, szResult + Offset, longNameSize / sizeof(WCHAR));
			ExFreePool(szResult);
			return TRUE;
		}
		// SharedDocs\\111.txt -> C:\\Documents and Settings\\All Users\\Documents\\111.txt
		static NTSTATUS Unc2Local(PUNICODE_STRING IN pstrUnc, PUNICODE_STRING OUT pstrLocal)
		{
			// 网络路径转本地路径
			NTSTATUS			status = STATUS_UNSUCCESSFUL;
			OBJECT_ATTRIBUTES	objectAttr = { 0 };
			HANDLE				hRegister = NULL;
			UNICODE_STRING		ustrReg = { 0 };
			ULONG				uResult = 0;
			WCHAR				*pTmp = NULL;
			DECLARE_UNICODE_STRING_SIZE(strShare, MAX_PATH + 64);	//多加64保险点
			DECLARE_UNICODE_STRING_SIZE(strName, MAX_PATH + 64);
			PKEY_VALUE_PARTIAL_INFORMATION pkpi = NULL;
			pTmp = RtlUnicodeStringChr(pstrUnc, L'\\');
			if (NULL == pTmp)
			{
				status = STATUS_INVALID_PARAMETER;
				return status;
			}
			//取 SharedDocs
#pragma warning( push )
#pragma warning( disable : 4311 )
#pragma warning( disable : 4302 )
			strShare.Length = (USHORT)((ULONG)pTmp - (ULONG)pstrUnc->Buffer);
#pragma warning( pop )

			RtlCopyMemory(strShare.Buffer, pstrUnc->Buffer, strShare.Length);
			//取 \\111.txt
			strName.Length = pstrUnc->Length - strShare.Length;
			RtlCopyMemory(strName.Buffer, pTmp, strName.Length);
			//查看 HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\LanmanServer\Shares  
			//有木有 SharedDocs 字段
			RtlInitUnicodeString(&ustrReg, L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Shares");
			InitializeObjectAttributes(&objectAttr,
				&ustrReg,
				OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
				NULL,
				NULL);
			status = ZwCreateKey(
				&hRegister,
				KEY_ALL_ACCESS,
				&objectAttr,
				0,
				NULL,
				REG_OPTION_NON_VOLATILE,
				&uResult);
			if (!NT_SUCCESS(status))
			{
				goto __error;
			}

			status = ZwQueryValueKey(hRegister,
				&strShare,
				KeyValuePartialInformation,
				NULL,
				0,
				&uResult);
			//非法UNC路径
			if (status != STATUS_BUFFER_OVERFLOW &&
				status != STATUS_BUFFER_TOO_SMALL)
			{
				goto __error;
			}
			pkpi = (PKEY_VALUE_PARTIAL_INFORMATION)ExAllocatePoolWithTag(PagedPool, uResult, 'kyle');
			if (pkpi == NULL)
			{
				status = STATUS_MEMORY_NOT_ALLOCATED;
				goto __error;
			}

			status = ZwQueryValueKey(hRegister,
				&strShare,
				KeyValuePartialInformation,
				pkpi,
				uResult,
				&uResult);
			if (!NT_SUCCESS(status))
			{
				goto __error;
			}

			//类型必须是这个
			if (pkpi->Type != REG_MULTI_SZ)
			{
				goto __error;
			}

			//解析出Path=来
			ustrReg.Length = (USHORT)pkpi->DataLength;
			ustrReg.MaximumLength = (USHORT)pkpi->DataLength;
			ustrReg.Buffer = (WCHAR*)(pkpi->Data);
			RtlInitUnicodeString(&strShare, L"path=");
			pTmp = RtlUnicodeStringStr(&ustrReg, &strShare);
			if (NULL == pTmp)
			{
				status = STATUS_UNSUCCESSFUL;
				goto __error;
			}
			else
			{
				RtlInitUnicodeString(&strShare, pTmp + wcslen(L"path="));
				RtlCopyUnicodeString(pstrLocal, &strShare);
				RtlAppendUnicodeStringToString(pstrLocal, &strName);
				status = STATUS_SUCCESS;
			}
		__error:
			if (pkpi)
			{
				ExFreePool(pkpi);
			}
			if (hRegister)
			{
				ZwClose(hRegister);
			}
			return status;
		}
	public://正则匹配
		static BOOLEAN IsPatternMatch(PUNICODE_STRING Expression, PUNICODE_STRING Name, BOOLEAN IgnoreCase)
		{
			return FsRtlIsNameInExpression(
				Expression,
				Name,
				IgnoreCase,//如果这里设置为TRUE,那么Expression必须是大写的
				NULL
			);
		}
		static BOOLEAN PatternMatch(WCHAR * pat, WCHAR * str)
		{
			WCHAR * s;
			WCHAR * p;
			BOOLEAN star = FALSE;
		loopStart:
			for (s = str, p = pat; *s; ++s, ++p) {
				switch (*p) {
				case L'?':
					if (*s == L'.') goto starCheck;
					break;
				case L'*':
					star = TRUE;
					str = s, pat = p;
					if (!*++pat) return TRUE;
					goto loopStart;
				default:
					if (towlower(*s) != towlower(*p))
						goto starCheck;
					break;
				}
			}
			if (*p == L'*') ++p;
			return (!*p);
		starCheck:
			if (!star) return FALSE;
			str++;
			goto loopStart;
		}
		//私有函数
	private:
		static BOOL QueryVolumeName(WCHAR ch, WCHAR * name, USHORT size)
		{
			WCHAR szVolume[7] = L"\\??\\C:";
			UNICODE_STRING LinkName;
			UNICODE_STRING VolName;
			UNICODE_STRING ustrTarget;
			NTSTATUS ntStatus = 0;

			RtlInitUnicodeString(&LinkName, szVolume);
			szVolume[4] = ch;
			ustrTarget.Buffer = name;
			ustrTarget.Length = 0;
			ustrTarget.MaximumLength = size;

			if (SymbolName2DeviceName(&LinkName, &VolName))
			{
				RtlCopyUnicodeString(&ustrTarget, &VolName);
				ExFreePool(VolName.Buffer);
				return TRUE;
			}
			return FALSE;
		}
		static WCHAR *RtlUnicodeStringChr(PUNICODE_STRING IN pStr, WCHAR chr)
		{
			ULONG i = 0;
			ULONG uSize = pStr->Length >> 1;
			for (i = 0; i < uSize; i++)
			{
				if (pStr->Buffer[i] == chr)
					return pStr->Buffer + i;
			}
			return NULL;
		}
		static WCHAR *RtlUnicodeStringStr(PUNICODE_STRING IN pSource, PUNICODE_STRING IN pStrDst)
		{
			ULONG i = 0;
			ULONG uLengthSetp = 0;
			ULONG uLengthSrc = 0;
			ULONG uLengthDst = 0;
			UNICODE_STRING str1 = { 0 };
			UNICODE_STRING str2 = { 0 };

			uLengthSrc = pSource->Length;
			uLengthDst = pStrDst->Length;

			if (uLengthSrc < uLengthDst)
				return NULL;

			uLengthSetp = ((uLengthSrc - uLengthDst) >> 1) + 1;
			for (i = 0; i < uLengthSetp; i++)
			{
				str1.Length = str1.MaximumLength = (USHORT)uLengthDst;
				str2.Length = str2.MaximumLength = (USHORT)uLengthDst;
				str1.Buffer = pSource->Buffer + i;
				str2.Buffer = pStrDst->Buffer;

				if (0 == RtlCompareUnicodeString(&str1, &str2, TRUE))
					return pSource->Buffer + i;
			}
			return NULL;
		}
	private://短路径转长路径用到的
		BOOLEAN IsRootDirecotry(WCHAR * wszDir)
		{
			SIZE_T length = wcslen(wszDir);
			// c:
			if ((length == 2) && (wszDir[1] == L':'))
				return TRUE;
			//\\??\\c:
			if ((length == 6) &&
				(_wcsnicmp(wszDir, L"\\??\\", 4) == 0) &&
				(wszDir[5] == L':'))
				return TRUE;
			//\\DosDevices\\c:
			if ((length == 14) &&
				(_wcsnicmp(wszDir, L"\\DosDevices\\", 12) == 0) &&
				(wszDir[13] == L':'))
				return TRUE;
			//\\Device\\HarddiskVolume1
			if ((length == 23) &&
				(_wcsnicmp(wszDir, L"\\Device\\HarddiskVolume", 22) == 0))
				return TRUE;
			return FALSE;
		}
		BOOLEAN IsDirectorySep(WCHAR ch)
		{
			return (ch == L'\\' || ch == L'/');
		}
		BOOLEAN QueryDirectoryForLongName(WCHAR * wszRootDir, WCHAR * wszShortName, WCHAR *wszLongName, ULONG ulSize)
		{
			//C:\\Program\\123456~1
			//wszRootdir为:c:\\Program
			//wszShortName为：123456~1
#define  MAX_PATH 260
			typedef unsigned char BYTE;
			UNICODE_STRING                       ustrRootDir = { 0 };
			UNICODE_STRING                       ustrShortName = { 0 };
			UNICODE_STRING                       ustrLongName = { 0 };
			OBJECT_ATTRIBUTES                    oa = { 0 };
			IO_STATUS_BLOCK                              Iosb = { 0 };
			NTSTATUS                                     ntStatus = 0;
			HANDLE                                       hDirHandle = 0;
			BYTE                                         *Buffer = NULL;
			WCHAR                                        *wszRoot = NULL;
			PFILE_BOTH_DIR_INFORMATION    pInfo = NULL;
			RtlZeroMemory(&Iosb, sizeof(IO_STATUS_BLOCK));
			Iosb.Status = STATUS_NO_SUCH_FILE;
			wszRoot = (WCHAR *)ExAllocatePoolWithTag(PagedPool,
				MAX_PATH * sizeof(WCHAR),
				'L2S');
			if (wszRoot == NULL)
			{
				return FALSE;
			}
			RtlZeroMemory(wszRoot, MAX_PATH * sizeof(WCHAR));
			wcsncpy(wszRoot, wszRootDir, MAX_PATH);
			RtlInitUnicodeString(&ustrRootDir, wszRoot);
			RtlInitUnicodeString(&ustrShortName, wszShortName);
			if (IsRootDirecotry(wszRoot))
				RtlAppendUnicodeToString(&ustrRootDir, L"\\");
			InitializeObjectAttributes(&oa,
				&ustrRootDir,
				OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
				0,
				0);
			ntStatus = ZwCreateFile(&hDirHandle,
				GENERIC_READ | SYNCHRONIZE,
				&oa,
				&Iosb,
				0,
				FILE_ATTRIBUTE_DIRECTORY,
				FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
				FILE_OPEN,
				FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
				0,
				0);
			if (!NT_SUCCESS(ntStatus))
			{
				ExFreePool(wszRoot);
				return FALSE;
			}
			ExFreePool(wszRoot);
			Buffer = (BYTE *)ExAllocatePoolWithTag(PagedPool,
				1024,
				'L2S');
			if (Buffer == NULL)
			{
				ZwClose(hDirHandle);
				return FALSE;
			}
			RtlZeroMemory(Buffer, 1024);
			ntStatus = ZwQueryDirectoryFile(hDirHandle,
				NULL,
				0,
				0,
				&Iosb,
				Buffer,
				1024,
				FileBothDirectoryInformation,
				TRUE,
				&ustrShortName, //传回与 ustrShortName Match的项
				TRUE);
			if (!NT_SUCCESS(ntStatus))
			{
				ExFreePool(Buffer);
				ZwClose(hDirHandle);
				return FALSE;
			}
			ZwClose(hDirHandle);
			pInfo = (PFILE_BOTH_DIR_INFORMATION)Buffer;
			if (pInfo->FileNameLength == 0)
			{
				ExFreePool(Buffer);
				return FALSE;
			}
			ustrShortName.Length = (USHORT)pInfo->FileNameLength;
			ustrShortName.MaximumLength = (USHORT)pInfo->FileNameLength;
			ustrShortName.Buffer = pInfo->FileName;      //长名
			if (ulSize < ustrShortName.Length)
			{
				ExFreePool(Buffer);
				return FALSE;
			}
			ustrLongName.Length = 0;
			ustrLongName.MaximumLength = (USHORT)ulSize;
			ustrLongName.Buffer = wszLongName;
			RtlCopyUnicodeString(&ustrLongName, &ustrShortName);
			ExFreePool(Buffer);
			return TRUE;
		}
		BOOLEAN QueryLongName(WCHAR * wszFullPath, WCHAR * wszLongName, ULONG size)
		{
			BOOLEAN        rtn = FALSE;
			WCHAR *        pchStart = wszFullPath;
			WCHAR *        pchEnd = NULL;
			WCHAR *        wszShortName = NULL;
			//c:\\Program\\Files1~1-->获得Files1~1的长名
			while (*pchStart)
			{
				if (IsDirectorySep(*pchStart))
					pchEnd = pchStart;
				pchStart++;
			}
			//wszFullPath=c:\\Program
			//pchEnd = Files~1
			if (pchEnd)
			{
				*pchEnd++ = L'\0';
				//c:\\Program\\Files1~1
				//wszFullPath:c:\\Program
				//pchEnd:Files1~1
				wszShortName = pchEnd;
				rtn = QueryDirectoryForLongName(wszFullPath, wszShortName, wszLongName, size);
				*(--pchEnd) = L'\\';
				//wszFullPath=c:\\Program\\Files1~1
			}
			return rtn;
		}
	};
}