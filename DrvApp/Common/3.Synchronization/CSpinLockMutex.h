#pragma once
namespace ddk::lock
{
	class CSpinLockMutex
	{
		std::atomic_flag flag;
	public:
		CSpinLockMutex() {
			flag._My_flag = 0;
		}
		void lock()
		{
			while (flag.test_and_set(std::memory_order_acquire))
			{
				YieldProcessor();
			}
		}
		void unlock()
		{
			flag.clear(std::memory_order_release);
		}
	};
}