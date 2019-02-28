#pragma once

//在应用层使用方法
/*
int main()
{
	auto h_event = OpenEvent(EVENT_ALL_ACCESS,FALSE, _T("Global\\test_event_a"));
	if (h_event)
	{
	SetEvent(h_event);
	CloseHandle(h_event);
	}
	while (1)
	{
	Sleep(1);
	}
	return 0;
}
*/
namespace ddk::lock
{
	class CEvent
	{
	private:
		WCHAR m_FullName[MAX_PATH];
		HANDLE m_hEvent;
	public://构造、析构、属性操作
		CEvent()
		{
			m_hEvent = nullptr;
			RtlZeroMemory(m_FullName, MAX_PATH * sizeof(WCHAR));
		}
		CEvent(WCHAR *event_name) : CEvent()
		{
			create(event_name);
		}
		~CEvent()
		{
			if (m_hEvent)
			{
				ZwClose(m_hEvent);
				m_hEvent = nullptr;
				RtlZeroMemory(m_FullName, MAX_PATH * sizeof(WCHAR));
			}
		}
		HANDLE get_handle() {
			return m_hEvent;
		}
		WCHAR *get_fullname() {
			return m_FullName;
		}
	public://创建打开事件、等待事件、设置事件
		BOOL open(WCHAR *event_name)
		{
			BOOL ret = FALSE;
			NTSTATUS status = STATUS_SUCCESS;
			SECURITY_DESCRIPTOR Se;
			OBJECT_ATTRIBUTES oa;
			UNICODE_STRING nsEvent;
			wcscpy_s(m_FullName, L"\\BaseNamedObjects\\");
			wcscat_s(m_FullName, event_name);

			do
			{
				status = RtlCreateSecurityDescriptor(&Se, SECURITY_DESCRIPTOR_REVISION);
				if (!NT_SUCCESS(status))
				{
					ddk::log::StatusInfo::Print(status);
					break;
				}

				status = RtlSetDaclSecurityDescriptor(&Se, TRUE, NULL, TRUE);
				if (!NT_SUCCESS(status))
				{
					ddk::log::StatusInfo::Print(status);
					break;
				}

				RtlInitUnicodeString(&nsEvent, m_FullName);
				InitializeObjectAttributes(&oa, &nsEvent, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
					NULL, &Se);
				status = ZwOpenEvent(&m_hEvent, EVENT_ALL_ACCESS,&oa);
				if (!NT_SUCCESS(status))
				{
					ddk::log::StatusInfo::Print(status);
					break;
				}
				ret = TRUE;
			} while (FALSE);
			return ret;	
		}
		bool create()
		{
			SECURITY_DESCRIPTOR Se;
			auto ns = RtlCreateSecurityDescriptor(&Se, SECURITY_DESCRIPTOR_REVISION);
			if (NT_SUCCESS(ns))
			{
				ns = RtlSetDaclSecurityDescriptor(&Se, TRUE, NULL, TRUE);
				if (NT_SUCCESS(ns))
				{
					OBJECT_ATTRIBUTES oa;
					InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE/* | OBJ_PERMANENT*/,
						NULL, &Se);
					ns = ZwCreateEvent(&m_hEvent,
						EVENT_ALL_ACCESS,
						&oa,
						SynchronizationEvent,
						FALSE);
					if (!NT_SUCCESS(ns))
					{
						m_hEvent = nullptr;
					}
				}
			}
			if (m_hEvent)
				return TRUE;
			return FALSE;
		}
		BOOL create(WCHAR *event_name)
		{
			SECURITY_DESCRIPTOR Se;
			wcscpy_s(m_FullName, L"\\BaseNamedObjects\\");
			wcscat_s(m_FullName, event_name);
			auto ns = RtlCreateSecurityDescriptor(&Se, SECURITY_DESCRIPTOR_REVISION);
			if (NT_SUCCESS(ns))
			{
				ns = RtlSetDaclSecurityDescriptor(&Se, TRUE, NULL, TRUE);
				if (NT_SUCCESS(ns))
				{
					OBJECT_ATTRIBUTES oa;
					UNICODE_STRING nsEvent;
					RtlInitUnicodeString(&nsEvent, m_FullName);
					InitializeObjectAttributes(&oa, &nsEvent, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
						NULL, &Se);
					ns = ZwCreateEvent(&m_hEvent, EVENT_ALL_ACCESS,
						&oa,
						SynchronizationEvent,
						FALSE);
					if (!NT_SUCCESS(ns) && ns != STATUS_OBJECT_NAME_COLLISION)
					{
						ddk::log::StatusInfo::Print(ns);
						m_hEvent = nullptr;
					}
				}
			}
			if (!m_hEvent)
			{
				return FALSE;
			}
			return TRUE;
		}
		void wait()
		{
			NT_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
			ZwWaitForSingleObject(m_hEvent, FALSE, NULL);
		}
		void wait_alert()
		{
			NT_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
			ZwWaitForSingleObject(m_hEvent, TRUE, NULL);
		}
		BOOL wait_time(LONGLONG _time)
		{
			LARGE_INTEGER time = { 0 };
			time.QuadPart = -_time;
			auto ns = ZwWaitForSingleObject(m_hEvent, FALSE, &time);
			if (ns == STATUS_SUCCESS)
			{
				return TRUE;
			}
			return FALSE;
		}
		void set()
		{
			LONG oldState;
			ZwSetEvent(m_hEvent, &oldState);
		}
	public://运算符重载
		CEvent &operator = (CEvent &event_)
		{
			this->m_hEvent = event_.get_handle();
			wcscpy(this->m_FullName, event_.get_fullname());
			event_.clear_handle();
			return (*this);
		}
	private:
		void clear_handle() {
			m_hEvent = NULL;
		}
	};
};