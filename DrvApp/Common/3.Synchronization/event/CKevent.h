#pragma once

namespace ddk::lock
{
	class CKEvent
	{
	public:
		CKEvent() {
			KeInitializeEvent(&_Event, SynchronizationEvent, FALSE);
		}
		~CKEvent() {}
	private:
		KEVENT _Event;
	public:
		void wait() {
			KeWaitForSingleObject(&_Event, Executive, KernelMode, FALSE, 0);
		}
		void set() {
			KeSetEvent(&_Event, 0, FALSE);
		}
	};
}