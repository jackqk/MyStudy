#pragma once
namespace ddk {
	//文件路径格式：L"\\??\\C:\\Users\\FrogGod\\Desktop\\"
	class CFile {	
	public://封装的类型
		enum OPEN_TYPE
		{
			OPEN_EXIST = 1,
			CREATE_NEW,
			OPEN_IF,
		};
		enum SEEK_TYPE
		{
			FILE_BEGIN = 1,
			FILE_END,
			CURRENT_OFFSET,
		};
		struct FILE_ATTR
		{
			DWORD file_attr;
			WCHAR file_name[MAX_PATH];
		};
		using CFILE_LIST = std::vector<ddk::CFile::FILE_ATTR>;
	private://成员变量
		HANDLE   m_hFile;
		WCHAR    m_filePath[MAX_PATH];
		LONGLONG m_fileOffset;
	public://构造析构、获得成员变量
		CFile() {
			m_hFile = NULL;
			m_fileOffset = 0;
			RtlZeroMemory(m_filePath, MAX_PATH);
		}
		CFile(WCHAR *strFilePath, OPEN_TYPE type = OPEN_EXIST) : CFile(){
			switch (type) {
			case ddk::CFile::OPEN_EXIST:
				open(strFilePath);
				break;
			case ddk::CFile::CREATE_NEW:
				create(strFilePath);
				break;
			case ddk::CFile::OPEN_IF:
				create(strFilePath);
				break;
			default:
				break;
			}
		}
		~CFile() {
			close();
		}
		WCHAR *getFilePath() {
			return m_filePath;
		}
		HANDLE getHandle() {
			return m_hFile;
		}
	public://创建、打开、重命名、复制，剪切，删除文件
		BOOL open(WCHAR *strFilePath) { 
			ULONG       _share   = FILE_SHARE_READ | FILE_SHARE_WRITE;
			ACCESS_MASK _generic = GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE;
			
		redo_create:
			{
				UNICODE_STRING usFileName;
				OBJECT_ATTRIBUTES oa = {};
				RtlInitUnicodeString(&usFileName, strFilePath);
				InitializeObjectAttributes(&oa,
					&usFileName,
					OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
					NULL,
					NULL
				);
				IO_STATUS_BLOCK iosb = {};
				HANDLE temp_handle = 0;

				auto ns = ZwCreateFile(
					&temp_handle,			//句柄指针
					_generic,				//读写访问
					&oa,					//OA
					&iosb,					//io状态
					NULL,					//一般添NULL
					FILE_ATTRIBUTE_NORMAL,	//文件属性一般写NORMAL
					_share,					//文件共享性，
											// 一般只填写FILE_SHARE_READ就行，
											// 但是特殊情况下需要写入0x7也就是全共享模式
					FILE_OPEN,				//当文件不存在时，返回失败
					FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE,//文件非目录性质
																		   // 并且文件操作直接写入文件系统（直接写入不会产生缓冲延时问题，但IO占用很多）
					NULL,					//EA属性填写NULL，这是文件创建不是驱动设备的交互，所以EA写NULL,EA长度也是0
					0
				);
				if (ns == STATUS_SUCCESS)
				{
					close();
					m_hFile = temp_handle;
					wcsncpy_s(m_filePath, strFilePath, MAX_PATH);
					return TRUE;
				}
				if (ns == STATUS_SHARING_VIOLATION)
				{
					_share = FILE_SHARE_READ;
					_generic = GENERIC_READ | SYNCHRONIZE;
					goto redo_create;
				}
				ddk::log::StatusInfo::Print(ns);
			}
			return FALSE;
		}
		BOOL create(WCHAR * strFilePath)
		{
			UNICODE_STRING usFileName;
			OBJECT_ATTRIBUTES oa;

			RtlInitUnicodeString(&usFileName, strFilePath);
			InitializeObjectAttributes(&oa,
				&usFileName,
				OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,//内核模式句柄，忽略大小写
														 // windows本身其实可以支持大小写，
														 // 但是基本仅限于对象名称的大小写，
														 // 低版本的文件系统是忽略文件名大小写的。
				NULL,
				NULL
			);//OA的初始化一般都是这样填写的
			IO_STATUS_BLOCK iosb;
			HANDLE temp_handle = 0;
			auto ns = ZwCreateFile(
				&temp_handle,//句柄指针
				GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,//读写访问
				&oa,//OA
				&iosb,//io状态
				NULL,//一般添NULL
				FILE_ATTRIBUTE_NORMAL,//文件属性一般写NORMAL
				FILE_SHARE_READ | FILE_SHARE_WRITE,//文件共享性，
												   // 一般只填写FILE_SHARE_READ就行，
												   // 但是特殊情况下需要写入0x7也就是全共享模式
				FILE_CREATE,//当文件存在时，返回失败
				FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE,//
																	   // 文件非目录性质
																	   // 并且文件操作直接写入文件系统（直接写入不会产生缓冲延时问题，但IO占用很多）
				NULL, //EA属性填写NULL，这是文件创建不是驱动设备的交互，所以EA写NULL,EA长度也是0
				0
			);
			if (ns == STATUS_SUCCESS)
			{
				close();
				m_hFile = temp_handle;
				wcsncpy_s(m_filePath, strFilePath, MAX_PATH);
				return TRUE;
			}
			ddk::log::StatusInfo::Print(ns);
			return FALSE;
		}
		BOOL open_if(WCHAR * strFilePath)
		{
			UNICODE_STRING usFileName;
			OBJECT_ATTRIBUTES oa;
			
			RtlInitUnicodeString(&usFileName, strFilePath);
			InitializeObjectAttributes(&oa,
				&usFileName,
				OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,//内核模式句柄，忽略大小写
														 // windows本身其实可以支持大小写，
														 // 但是基本仅限于对象名称的大小写，
														 // 低版本的文件系统是忽略文件名大小写的。
				NULL,
				NULL
			);//OA的初始化一般都是这样填写的
			IO_STATUS_BLOCK iosb;
			HANDLE temp_handle = 0;
			auto ns = ZwCreateFile(
				&temp_handle,//句柄指针
				GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,//读写访问
				&oa,//OA
				&iosb,//io状态
				NULL,//一般添NULL
				FILE_ATTRIBUTE_NORMAL,//文件属性一般写NORMAL
				FILE_SHARE_READ | FILE_SHARE_WRITE,//文件共享性，
												   // 一般只填写FILE_SHARE_READ就行，
												   // 但是特殊情况下需要写入0x7也就是全共享模式
				FILE_OPEN_IF,//当文件不存在时，创建，存在打开
				FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE,//
																	   // 文件非目录性质
																	   // 并且文件操作直接写入文件系统（直接写入不会产生缓冲延时问题，但IO占用很多）
				NULL, //EA属性填写NULL，这是文件创建不是驱动设备的交互，所以EA写NULL,EA长度也是0
				0
			);
			if (ns == STATUS_SUCCESS)
			{
				close();
				m_hFile = temp_handle;
				wcsncpy_s(m_filePath, strFilePath, MAX_PATH);
				return TRUE;
			}
			ddk::log::StatusInfo::Print(ns);
			return FALSE;
		}
		BOOL rename(WCHAR * strFilePath)
		{
			if (m_hFile)
			{
				WCHAR szBuffer[MAX_PATH * 3] = { 0 };
				IO_STATUS_BLOCK iosb;
				PFILE_RENAME_INFORMATION pFileRename;
				UNICODE_STRING nsFileName;
				RtlInitUnicodeString(&nsFileName, strFilePath);
				pFileRename = (PFILE_RENAME_INFORMATION)szBuffer;
				pFileRename->ReplaceIfExists = TRUE;
				pFileRename->RootDirectory = NULL;
				pFileRename->FileNameLength = nsFileName.Length;
				RtlCopyMemory(pFileRename->FileName, nsFileName.Buffer, pFileRename->FileNameLength);
				auto ns = ZwSetInformationFile(m_hFile,
					&iosb,
					pFileRename,
					sizeof(FILE_RENAME_INFORMATION) + pFileRename->FileNameLength,
					FileRenameInformation);
				if (NT_SUCCESS(ns))
					return TRUE;
				LOG_DEBUG("rename %ws %x\r\n", pFileRename->FileName, ns);
			}
			return FALSE;
		}
		void close() {
			//若以前有句柄存在，则关闭，并清零
			NTSTATUS status = STATUS_UNSUCCESSFUL;
			if (m_hFile)
				status = ZwClose(m_hFile);
			if (NT_SUCCESS(status))
			{
				m_fileOffset = 0;
				m_hFile = nullptr;
			}
		}
		static BOOL del_file1(WCHAR *strFilePath)
		{
			UNICODE_STRING usFileName;
			NTSTATUS ns;
			OBJECT_ATTRIBUTES oa = { 0 };
			RtlInitUnicodeString(&usFileName, strFilePath);
			InitializeObjectAttributes(&oa,
				&usFileName,
				OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
				NULL,
				NULL
			);
			ns = ZwDeleteFile(&oa);
			if (NT_SUCCESS(ns))
				return TRUE;
			return FALSE;
		}
		static BOOL del_file2(WCHAR *fileName)
		{
			OBJECT_ATTRIBUTES    objAttributes = { 0 };
			IO_STATUS_BLOCK  iosb = { 0 };
			HANDLE    handle = NULL;
			FILE_DISPOSITION_INFORMATION     disInfo = { 0 };
			UNICODE_STRING                      uFileName = { 0 };
			NTSTATUS     status = 0;

			RtlInitUnicodeString(&uFileName, fileName);

			InitializeObjectAttributes(&objAttributes,
				&uFileName,
				OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
				NULL,
				NULL);

			status = ZwCreateFile(&handle, SYNCHRONIZE | FILE_WRITE_DATA | DELETE, &objAttributes, &iosb, NULL, FILE_ATTRIBUTE_NORMAL,
				FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT | FILE_DELETE_ON_CLOSE,
				NULL,
				0);
			if (!NT_SUCCESS(status))
			{
				if (status == STATUS_ACCESS_DENIED)
				{
					status = ZwCreateFile(&handle, SYNCHRONIZE | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
						&objAttributes,
						&iosb,
						NULL,
						FILE_ATTRIBUTE_NORMAL,
						FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
						FILE_OPEN,
						FILE_SYNCHRONOUS_IO_NONALERT,
						NULL,
						0);
					if (NT_SUCCESS(status))
					{
						FILE_BASIC_INFORMATION basicInfo = { 0 };
						status = ZwQueryInformationFile(handle, &iosb, &basicInfo, sizeof(basicInfo), FileBasicInformation);
						if (!NT_SUCCESS(status))
						{
							DbgPrint("ZwQueryInformationFile(%wZ) failed(%x)\n", &uFileName, status);
						}

						basicInfo.FileAttributes = FILE_ATTRIBUTE_NORMAL;
						status = ZwSetInformationFile(handle, &iosb, &basicInfo, sizeof(basicInfo), FileBasicInformation);
						if (!NT_SUCCESS(status))
						{
							DbgPrint("ZwSetInformationFile(%wZ) failed(%x)\n", &uFileName, status);
						}

						ZwClose(handle);
						status = ZwCreateFile(&handle,
							SYNCHRONIZE | FILE_WRITE_DATA | DELETE,
							&objAttributes,
							&iosb,
							NULL,
							FILE_ATTRIBUTE_NORMAL,
							FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
							FILE_OPEN,
							FILE_SYNCHRONOUS_IO_NONALERT | FILE_DELETE_ON_CLOSE,
							NULL,
							0);
					}
				}

				if (!NT_SUCCESS(status))
				{
					DbgPrint("ZwCreateFile(%wZ) failed(%x)\n", &uFileName, status);
					return FALSE;
				}
			}

			disInfo.DeleteFile = TRUE;
			status = ZwSetInformationFile(handle, &iosb, &disInfo, sizeof(disInfo), FileDispositionInformation);
			if (!NT_SUCCESS(status))
			{
				DbgPrint("ZwSetInformationFile(%wZ) failed(%x)\n", &uFileName, status);
				return FALSE;
			}

			ZwClose(handle);
			return TRUE;;
		}
		static BOOL del_force1(HANDLE handle) {
			NTSTATUS        ntStatus = STATUS_SUCCESS;
			PFILE_OBJECT    fileObject;
			PDEVICE_OBJECT  DeviceObject;
			PIRP            Irp;
			KEVENT          event;
			FILE_DISPOSITION_INFORMATION  FileInformation;
			IO_STATUS_BLOCK ioStatus;
			PIO_STACK_LOCATION irpSp;
			PSECTION_OBJECT_POINTERS pSectionObjectPointer;

			ntStatus = ObReferenceObjectByHandle(handle,
				DELETE,
				*IoFileObjectType,
				KernelMode,
				(PVOID *)&fileObject,
				NULL);

			if (!NT_SUCCESS(ntStatus))
			{
				DbgPrint("ObReferenceObjectByHandle()");
				ZwClose(handle);
				return FALSE;
			}

			DeviceObject = IoGetRelatedDeviceObject(fileObject);

			//构建IRP
			Irp = IoAllocateIrp(DeviceObject->StackSize, TRUE);

			if (Irp == NULL)
			{
				ObDereferenceObject(fileObject);
				ZwClose(handle);
				return FALSE;
			}

			KeInitializeEvent(&event, SynchronizationEvent, FALSE);

			FileInformation.DeleteFile = TRUE;

			Irp->AssociatedIrp.SystemBuffer = &FileInformation;
			Irp->UserEvent = &event;
			Irp->UserIosb = &ioStatus;
			Irp->Tail.Overlay.OriginalFileObject = fileObject;
			Irp->Tail.Overlay.Thread = (PETHREAD)KeGetCurrentThread();
			Irp->RequestorMode = KernelMode;

			irpSp = IoGetNextIrpStackLocation(Irp);
			irpSp->MajorFunction = IRP_MJ_SET_INFORMATION;
			irpSp->DeviceObject = DeviceObject;
			irpSp->FileObject = fileObject;
			irpSp->Parameters.SetFile.Length = sizeof(FILE_DISPOSITION_INFORMATION);
			irpSp->Parameters.SetFile.FileInformationClass = FileDispositionInformation;
			irpSp->Parameters.SetFile.FileObject = fileObject;

			//设置IRP的完成例程
			IoSetCompletionRoutine(
				Irp,
				dfSkillSetFileCompletion,
				&event,
				TRUE,
				TRUE,
				TRUE);
			pSectionObjectPointer = fileObject->SectionObjectPointer;
			if (pSectionObjectPointer)
			{
				pSectionObjectPointer->ImageSectionObject = 0;
				pSectionObjectPointer->DataSectionObject = 0;
			}
			ntStatus = IoCallDriver(DeviceObject, Irp); //往下发IRP
			if (!NT_SUCCESS(ntStatus))
			{
				ObDereferenceObject(fileObject);
				ZwClose(handle);
				return FALSE;
			}

			KeWaitForSingleObject(&event, Executive, KernelMode, TRUE, NULL);
			ObDereferenceObject(fileObject);

			if (Irp)
			{
				if (!NT_SUCCESS(Irp->UserIosb->Status))
					return FALSE;
			}
			return TRUE;
		}
		static NTSTATUS copy_file(WCHAR *src, WCHAR *dst)
		{
			HANDLE                  hSrcFile = NULL;
			HANDLE                  hDstFile = NULL;
			UNICODE_STRING          uSrc = { 0 };
			UNICODE_STRING          uDst = { 0 };
			OBJECT_ATTRIBUTES       objSrcAttrib = { 0 };
			OBJECT_ATTRIBUTES       objDstAttrib = { 0 };
			NTSTATUS                status = 0;
			ULONG                   uReadSize = 0;
			ULONG                   uWriteSize = 0;
			ULONG                   length = 0;
			PVOID                   buffer = NULL;
			LARGE_INTEGER           offset = { 0 };
			IO_STATUS_BLOCK         io_status = { 0 };

			RtlInitUnicodeString(&uSrc, src);
			RtlInitUnicodeString(&uDst, dst);

			InitializeObjectAttributes(&objSrcAttrib,
				&uSrc,
				OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
				NULL,
				NULL);
			InitializeObjectAttributes(&objDstAttrib,
				&uDst,
				OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
				NULL,
				NULL);

			status = ZwCreateFile(
				&hSrcFile,
				FILE_READ_DATA | FILE_READ_ATTRIBUTES,
				&objSrcAttrib,
				&io_status,
				NULL,
				FILE_ATTRIBUTE_NORMAL,
				FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
				FILE_OPEN,
				FILE_SYNCHRONOUS_IO_NONALERT,
				NULL,
				0);
			if (!NT_SUCCESS(status))
			{
				return status;
			}

			status = ZwCreateFile(
				&hDstFile,
				GENERIC_WRITE,
				&objDstAttrib,
				&io_status,
				NULL,
				FILE_ATTRIBUTE_NORMAL,
				FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
				FILE_OPEN_IF,
				FILE_SYNCHRONOUS_IO_NONALERT,
				NULL,
				0);
			if (!NT_SUCCESS(status))
			{
				ZwClose(hSrcFile);
				return status;
			}

			buffer = ExAllocatePoolWithTag(PagedPool, 1024, 'ELIF');
			if (buffer == NULL)
			{
				ZwClose(hSrcFile);
				ZwClose(hDstFile);
				return STATUS_INSUFFICIENT_RESOURCES;
			}

			while (1)
			{
				status = ZwReadFile(hSrcFile, NULL, NULL, NULL, &io_status, buffer, PAGE_SIZE, &offset, NULL);
				if (!NT_SUCCESS(status))
				{
					if (status == STATUS_END_OF_FILE)
					{
						status = STATUS_SUCCESS;
					}
					break;
				}

				length = (ULONG)io_status.Information;
				status = ZwWriteFile(hDstFile, NULL, NULL, NULL, &io_status, buffer, length, &offset, NULL);
				if (!NT_SUCCESS(status))
					break;
				offset.QuadPart += length;
			}
			ExFreePool(buffer);
			ZwClose(hSrcFile);
			ZwClose(hDstFile);
			return status;
		}
		static NTSTATUS move_file(WCHAR *src, WCHAR *dst)
		{
			NTSTATUS status = STATUS_SUCCESS;
			status = copy_file(src, dst);
			if (NT_SUCCESS(status))
			{
				status = del_file1(src);
			}
			return status;
		}
	public://读、写、设置位置、
		BOOL   seek(size_t distance, ddk::CFile::SEEK_TYPE type)
		{
			auto new_offset = m_fileOffset;
			if (!m_hFile)
				return FALSE;
			switch (type)
			{
			case ddk::CFile::FILE_BEGIN:
				new_offset = distance;
				break;
			case ddk::CFile::FILE_END:
				new_offset = get_file_size() + distance;
				break;
			case ddk::CFile::CURRENT_OFFSET:
				new_offset = m_fileOffset + distance;
				break;
			default:
				return FALSE;
				break;
			}

			FILE_POSITION_INFORMATION fpinfo;
			IO_STATUS_BLOCK iosb;
			fpinfo.CurrentByteOffset.QuadPart = new_offset;
			auto ns = ZwSetInformationFile(m_hFile,
				&iosb,
				&fpinfo,
				sizeof(fpinfo),
				FilePositionInformation);
			if (NT_SUCCESS(ns))
			{
				m_fileOffset = new_offset;
				return TRUE;
			}
			return FALSE;
		}
		void   set_file_append()
		{
			m_fileOffset = get_file_size();
		}
		BOOL   is_eof()
		{
			if (m_hFile && get_file_size() == m_fileOffset)
				return TRUE;
			return FALSE;
		}
		size_t write(PVOID in_buffer, size_t in_size)
		{
			size_t write_size = 0;
			NTSTATUS ns;
			IO_STATUS_BLOCK iosb;
			LARGE_INTEGER offset;

			if (!m_hFile) return FALSE;
			offset.QuadPart = m_fileOffset;
			ns = ZwWriteFile(m_hFile,
				NULL,
				NULL,
				NULL,
				&iosb,
				in_buffer,
				(ULONG)in_size,
				&offset,
				NULL);
			if (NT_SUCCESS(ns))
			{
				write_size = (size_t)iosb.Information;
				if (iosb.Information == in_size)
				{
					m_fileOffset += in_size;
					return write_size;
				}
			}
			return -1;
		}
		size_t writeline(char *strline)
		{
			std::string write_string(strline);
			write_string += std::string("\r\n");
			size_t write_size = 0;
			return write(PVOID(write_string.c_str()), write_string.size());
		}
		size_t writeline(WCHAR *strline)
		{
			std::wstring write_string(strline);
			write_string += std::wstring(L"\r\n");
			size_t write_size = 0;
			return write(PVOID(write_string.c_str()), write_string.size() * sizeof(wchar_t));
		}
		size_t read(PVOID outBuffer, size_t length)
		{
			size_t retLength = 0;
			if (!m_hFile)
				return (size_t)-1;

			IO_STATUS_BLOCK iosb = { 0 };
			NTSTATUS		ns;
			LARGE_INTEGER offset;
			offset.QuadPart = m_fileOffset;
			ns = ZwReadFile(
				m_hFile,
				NULL,
				NULL,
				NULL,
				&iosb,
				outBuffer,
				(LONG)length,
				&offset,
				NULL);
			if (NT_SUCCESS(ns))
			{
				retLength = iosb.Information;
				m_fileOffset += retLength;
				return retLength;
			}
			LOG_DEBUG("错误:%x\r\n", ns);
			return -1;
		}
		BOOL   readline(std::string & strline)
		{
			auto ret = FALSE;
			strline = std::string("");
			do
			{
				CHAR nC[2] = { 0 };
				size_t readsize = 0;
				readsize = read(&nC, sizeof(CHAR));
				if (!readsize) break;
				if (nC[0] == 13 || nC[0] == 10)
				{
					do
					{
						readsize = read(&nC, sizeof(CHAR));
						if (!(nC[0] == 13 || nC[0] == 10))
						{
							m_fileOffset--;
							break;
						}
					} while (true);
					ret = TRUE;
					break;
				}
				else
				{
					nC[1] = 0;
					if (nC[0] != 0)
						strline += std::string(nC);
				}
			} while (!is_eof());
			return ret;
		}		
	public://文件夹操作
		static BOOL dir_file(WCHAR *strDir, CFILE_LIST &file_list)
		{
			//遍历一层目录
			NTSTATUS status;
			UNICODE_STRING dirname_unicode;
			OBJECT_ATTRIBUTES oa;
			IO_STATUS_BLOCK io_status;
			HANDLE handle = nullptr;
			

			RtlInitUnicodeString(&dirname_unicode, strDir);
			InitializeObjectAttributes(&oa, &dirname_unicode, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
			

			status = ZwCreateFile(&handle, FILE_LIST_DIRECTORY,
				&oa, &io_status, NULL, 0,
				FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
				FILE_OPEN,
				FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE,
				NULL, 0);
			if (!NT_SUCCESS(status))
				return FALSE;

			auto file_exit = std::experimental::make_scope_exit([&] {ZwClose(handle); });
			auto dir_size = 0x10000;
			auto dir_entries_buffer = (FILE_DIRECTORY_INFORMATION*)malloc(dir_size);
			if (!dir_entries_buffer)
			{
				return false;
			}
			auto free_dir = std::experimental::make_scope_exit([&] {free(dir_entries_buffer); });
			auto first_iteration = BOOLEAN(TRUE);
			for (;;)
			{
				IO_STATUS_BLOCK iob = { 0 };
				RtlSecureZeroMemory(dir_entries_buffer, dir_size);
				auto ns = ZwQueryDirectoryFile(handle,
					NULL,
					NULL,
					NULL,
					&iob,
					dir_entries_buffer,
					dir_size,
					FileDirectoryInformation,
					FALSE,
					NULL,
					first_iteration
				);
				if (!NT_SUCCESS(ns))
				{
					break;
				}
				auto entry = dir_entries_buffer;
				for (;;)
				{
					FILE_ATTR file;
					wcscpy(file.file_name, strDir);
					wcscat(file.file_name, entry->FileName);
					file.file_attr = entry->FileAttributes;
					file_list.push_back(file);
					if (entry->NextEntryOffset == 0)
						break;
					entry = (FILE_DIRECTORY_INFORMATION *)(((char *)entry) + entry->NextEntryOffset);
				}
				first_iteration = FALSE;
			}
			return true;
		}
		//删除一个文件夹
	public://其他操作
		BOOL     set_file_attr(DWORD dwFileAttributes)
		{

			FILE_BASIC_INFORMATION fbi;
			IO_STATUS_BLOCK iosb;
			auto ns = ZwQueryInformationFile(m_hFile,
				&iosb,
				&fbi,
				sizeof(FILE_BASIC_INFORMATION),
				FileBasicInformation);
			if (NT_SUCCESS(ns))
			{
				fbi.FileAttributes = dwFileAttributes;
				ns = ZwSetInformationFile(m_hFile ,
					&iosb,
					&fbi,
					sizeof(FILE_BASIC_INFORMATION),
					FileBasicInformation);
				if (NT_SUCCESS(ns))
				{
					return TRUE;
				}
			}
			return FALSE;
		}
		BOOL     set_file_size(LONGLONG file_size)
		{
			IO_STATUS_BLOCK iosb;
			FILE_END_OF_FILE_INFORMATION fendinfo;
			if (!m_hFile)
				return FALSE;
			fendinfo.EndOfFile.QuadPart = file_size;
			auto ns = ZwSetInformationFile(m_hFile,
				&iosb,
				&fendinfo,
				sizeof(FILE_END_OF_FILE_INFORMATION),
				FileEndOfFileInformation);
			if (NT_SUCCESS(ns))
				return TRUE;
			return FALSE;
		}
		LONGLONG get_file_size()
		{
			if (!m_hFile)
				return LONGLONG(0);
			FILE_STANDARD_INFORMATION fsi = { 0 };
			IO_STATUS_BLOCK iosb = { 0 };
			auto ns = ZwQueryInformationFile(m_hFile,
				&iosb,
				&fsi,
				sizeof(FILE_STANDARD_INFORMATION),
				FileStandardInformation);
			if (NT_SUCCESS(ns))
				return fsi.EndOfFile.QuadPart;
			return LONGLONG(0);
		}
		static BOOL is_file_exist(WCHAR  *strFile)
		{
			OBJECT_ATTRIBUTES				oa = { 0 };
			UNICODE_STRING					usName = { 0 };
			FILE_NETWORK_OPEN_INFORMATION 	info = { 0 };

			RtlInitUnicodeString(&usName, strFile);
			RtlZeroMemory(&info, sizeof(FILE_NETWORK_OPEN_INFORMATION));
			InitializeObjectAttributes(
				&oa,
				&usName,
				OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
				NULL,
				NULL
			);
			auto ns = ZwQueryFullAttributesFile(
				&oa,
				&info);
			if (ns == STATUS_SUCCESS)
				return (!(info.FileAttributes&FILE_ATTRIBUTE_DIRECTORY));
			return FALSE;
		}
		static BOOL close_handle(WCHAR *filePath)
		{
			//传进去是\\??\\c:\\123.txt，在系统找123.txt的句柄，找到则关闭
			NTSTATUS					 status;
			PVOID						 buf = NULL;
			PSYSTEM_HANDLE_INFORMATION 	 pSysHandleInfo;
			SYSTEM_HANDLE_TABLE_ENTRY_INFO handleTEI;
			ULONG						size = 1;
			ULONG						NumOfHandle = 0;
			ULONG						i;
			CLIENT_ID 					cid;
			HANDLE						hHandle;
			HANDLE						hProcess;
			HANDLE 						hDupObj;
			OBJECT_ATTRIBUTES 			oa;
			ULONG						processID;
			UNICODE_STRING 				uLinkName;
			UNICODE_STRING				uLink;
			ULONG 						ulRet;
			PVOID    			 		fileObject;
			POBJECT_NAME_INFORMATION 	pObjName = NULL;
			UNICODE_STRING				delFileName = { 0 };
			WCHAR						wVolumeLetter[3];
			WCHAR						*pFilePath;
			UNICODE_STRING				uVolume;
			UNICODE_STRING				uFilePath;
			UNICODE_STRING 				NullString = RTL_CONSTANT_STRING(L"");
			BOOLEAN					bRet = FALSE;


			//获得全局句柄表
			for (size = 1; ; size *= 2)
			{
				if (NULL == (buf = ExAllocatePoolWithTag(NonPagedPool, size, 'FILE')))
				{
					DbgPrint(("alloc mem failed\n"));
					goto Exit;
				}
				RtlZeroMemory(buf, size);
				status = ZwQuerySystemInformation(SystemHandleInformation, buf, size, NULL);
				if (!NT_SUCCESS(status))
				{
					if (STATUS_INFO_LENGTH_MISMATCH == status)
					{
						ExFreePool(buf);
						buf = NULL;
					}
					else
					{
						DbgPrint(("ZwQuerySystemInformation() failed"));
						goto Exit;
					}
				}
				else
				{
					break;
				}
			}

			pSysHandleInfo = (PSYSTEM_HANDLE_INFORMATION)buf;
			NumOfHandle = pSysHandleInfo->NumberOfHandles;
			/* Get the volume character like C: */
			wVolumeLetter[0] = filePath[4];
			wVolumeLetter[1] = filePath[5];
			wVolumeLetter[2] = 0;
			uLinkName.Buffer = (PWCH)ExAllocatePoolWithTag(NonPagedPool, 256 + sizeof(ULONG), 'A1');
			uLinkName.MaximumLength = 256;
			RtlInitUnicodeString(&uVolume, wVolumeLetter);
			RtlInitUnicodeString(&uLink, L"\\DosDevices\\");
			RtlCopyUnicodeString(&uLinkName, &uLink);
			status = RtlAppendUnicodeStringToString(&uLinkName, &uVolume);
			if (!NT_SUCCESS(status))
			{
				LOG_DEBUG("RtlAppendUnicodeStringToString() failed");
				return FALSE;
			}

			ddk::util::PathUtil::SymbolName2DeviceName(&uLinkName, &delFileName);
			RtlFreeUnicodeString(&uLinkName);
			pFilePath = (WCHAR *)&filePath[6];
			RtlInitUnicodeString(&uFilePath, pFilePath);
			RtlAppendUnicodeStringToString(&delFileName, &uFilePath);
			if (!NT_SUCCESS(status))
			{
				LOG_DEBUG("RtlAppendUnicodeStringToString() failed\r\n");
				return FALSE;
			}

			for (i = 0; i < NumOfHandle; i++)
			{
				handleTEI = pSysHandleInfo->Handles[i];
				if (handleTEI.ObjectTypeIndex != 25 && handleTEI.ObjectTypeIndex != 28)//28文件,25设备对象
					continue;
				processID = (ULONG)handleTEI.UniqueProcessId;
				cid.UniqueProcess = (HANDLE)processID;
				cid.UniqueThread = (HANDLE)0;
				hHandle = (HANDLE)handleTEI.HandleValue;
				InitializeObjectAttributes(&oa, NULL, 0, NULL, NULL);
				status = ZwOpenProcess(&hProcess, PROCESS_DUP_HANDLE, &oa, &cid);
				if (!NT_SUCCESS(status))
				{
					KdPrint(("ZwOpenProcess:%d Fail ", processID));
					continue;
				}

				status = ZwDuplicateObject(hProcess, hHandle, NtCurrentProcess(), &hDupObj, \
					PROCESS_ALL_ACCESS, 0, DUPLICATE_SAME_ACCESS);
				if (!NT_SUCCESS(status))
				{
					DbgPrint(("ZwDuplicateObject1 : Fail "));
					continue;
				}
				status = ObReferenceObjectByHandle(
					hDupObj,
					FILE_ANY_ACCESS,
					0,
					KernelMode,
					&fileObject,
					NULL);

				if (!NT_SUCCESS(status))
				{
					DbgPrint(("ObReferenceObjectByHandle : Fail "));
					continue;
				}

				pObjName = (POBJECT_NAME_INFORMATION)ExAllocatePoolWithTag(NonPagedPool, \
					sizeof(OBJECT_NAME_INFORMATION) + 1024 * sizeof(WCHAR), 'A1');

				if (STATUS_SUCCESS != (status = ObQueryNameString(fileObject, pObjName, \
					sizeof(OBJECT_NAME_INFORMATION) + 1024 * sizeof(WCHAR), &ulRet)))
				{
					ObDereferenceObject(fileObject);
					continue;
				}
				if (RtlCompareUnicodeString(&pObjName->Name, &delFileName, TRUE) == 0)
				{

					ObDereferenceObject(fileObject);
					ZwClose(hDupObj);

					status = ZwDuplicateObject(hProcess, hHandle, NtCurrentProcess(), &hDupObj, \
						PROCESS_ALL_ACCESS, 0, DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE);
					if (!NT_SUCCESS(status))
					{
						DbgPrint(("ZwDuplicateObject2 : Fail "));
					}
					else
					{
						ZwClose(hDupObj);
						bRet = TRUE;
					}
					break;
				}

				ExFreePool(pObjName);
				pObjName = NULL;

				ObDereferenceObject(fileObject);
				ZwClose(hDupObj);
				ZwClose(hProcess);

			}

		Exit:
			if (pObjName != NULL)
			{
				ExFreePool(pObjName);
				pObjName = NULL;
			}
			if (delFileName.Buffer != NULL)
			{
				ExFreePool(delFileName.Buffer);
			}
			if (buf != NULL)
			{
				ExFreePool(buf);
				buf = NULL;
			}
			return(bRet);

		}
	private:
		static NTSTATUS dfSkillSetFileCompletion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context)
		{
			//del_force1 强删文件回调
			Irp->UserIosb->Status = Irp->IoStatus.Status;
			Irp->UserIosb->Information = Irp->IoStatus.Information;
			KeSetEvent(Irp->UserEvent, IO_NO_INCREMENT, FALSE);
			IoFreeIrp(Irp);
			return STATUS_MORE_PROCESSING_REQUIRED;
		}
};
}
