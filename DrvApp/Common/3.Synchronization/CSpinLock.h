#pragma once

namespace ddk::lock {
	class CSpinLock
	{
	public:
		CSpinLock()
		{
			spinlock = 0;
			KeInitializeSpinLock(&spinlock);
		}
		~CSpinLock()
		{

		}
		void lock(PKLOCK_QUEUE_HANDLE handle)
		{
			if (KeGetCurrentIrql() < DISPATCH_LEVEL)
				KeAcquireInStackQueuedSpinLock(&spinlock, handle);
			else
				KeAcquireInStackQueuedSpinLockAtDpcLevel(&spinlock, handle);
		}
		void unlock(PKLOCK_QUEUE_HANDLE handle)
		{
			if (handle->OldIrql < DISPATCH_LEVEL)
				KeReleaseInStackQueuedSpinLock(handle);
			else
				KeReleaseInStackQueuedSpinLockFromDpcLevel(handle);
		}
	private:
		KSPIN_LOCK spinlock;
	};
}