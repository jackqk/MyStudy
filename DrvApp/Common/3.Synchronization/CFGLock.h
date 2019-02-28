#pragma once
namespace ddk::lock
{
	class CFGLock
	{
	public:
		CFGLock()
		{
			llock = 0;
		}
		~CFGLock()
		{

		}
		void acquire()
		{
			while (InterlockedCompareExchange(&llock, 0, 0))
			{
				YieldProcessor();
			}
			InterlockedIncrement(&llock);
		}
		void release()
		{
			InterlockedDecrement(&llock);
		}
		void only_acquire() {
			InterlockedIncrement(&llock);
		}
		bool try_acquire()
		{
			if (InterlockedCompareExchange(&llock, 0, 0))
			{
				return false;
			}
			InterlockedIncrement(&llock);
			return true;
		}
		void wait_for_release()
		{
			while (InterlockedCompareExchange(&llock, 0, 0))
			{
				YieldProcessor();
			}
		}
		LONG get()
		{
			return llock;
		}
	private:
		LONG llock;
	};
};
