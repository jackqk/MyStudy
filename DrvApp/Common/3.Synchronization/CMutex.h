#pragma once
namespace ddk::lock
{
	class CMutex
	{
	private:
		KGUARDED_MUTEX mutex;
	public:
		CMutex()
		{
			KeInitializeGuardedMutex(&mutex);
		}
		~CMutex()
		{

		}
		void lock()
		{
			NT_ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
			KeAcquireGuardedMutex(&mutex);
		}
		void unlock()
		{
			KeReleaseGuardedMutex(&mutex);
		}
		BOOL try_lock()
		{
			NT_ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
			if (!KeTryToAcquireGuardedMutex(&mutex)) return FALSE;
			return TRUE;
		}
	};
};