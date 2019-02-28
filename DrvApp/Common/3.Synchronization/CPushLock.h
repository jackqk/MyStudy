#pragma once
namespace ddk::lock
{
	class CPushLock
	{
	public:
		CPushLock() {
			FltInitializePushLock(&_lock);
		}
		~CPushLock() {
			FltDeletePushLock(&_lock);
		}
		void acqurie_read() {
			FltAcquirePushLockShared(&_lock);
		}
		void acqurie_write() {
			FltAcquirePushLockExclusive(&_lock);
		}
		void release() {
			FltReleasePushLock(&_lock);
		}
	private:
		EX_PUSH_LOCK _lock;
	};
};