#pragma once

namespace ddk::util {
	/////////GUID工具/////////
	class GuidUtil {
	public:
		//返回宽字节字符串
		static NTSTATUS GetGUID(WCHAR *szGuid) {
			GUID guid;
			NTSTATUS status = STATUS_SUCCESS;
			ExUuidCreate(&guid);
			status = RtlStringCchPrintfW(szGuid, MAX_PATH, L"{%08x-%04x-%04x-%02x-%02x-%02x-%02x}",
				guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3]);
			return status;
		}
		//返回UNICODE_STRING
		static NTSTATUS GetGUID(PUNICODE_STRING pucGuid) {
			WCHAR szGuid[MAX_PATH] = { 0 };
			NTSTATUS status = STATUS_SUCCESS;
			status = GetGUID(szGuid);
			if (!NT_SUCCESS(status))
				return status;
			RtlCopyMemory(pucGuid->Buffer, szGuid, sizeof(szGuid));
			pucGuid->Length = (USHORT)wcslen(szGuid) * sizeof(WCHAR);
			return status;
		}
	};
}