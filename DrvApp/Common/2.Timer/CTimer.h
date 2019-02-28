#pragma once
namespace ddk
{
	//这个timer都是周期性触发
	class CTimer
	{
	private://回调
		static VOID _launch_callback_timer(
			PKDPC Dpc,
			PVOID DeferredContext,
			PVOID SystemArgument1,
			PVOID SystemArgument2) {
			UNREFERENCED_PARAMETER(Dpc);
			UNREFERENCED_PARAMETER(SystemArgument1);
			UNREFERENCED_PARAMETER(SystemArgument2);
			auto p_this = reinterpret_cast<CTimer*>(DeferredContext);
			__try
			{
				//p_this->timer_function();
				p_this->m_function();
			}
			__except (1)
			{
				LOG_DEBUG("callback failed\r\n");
			}
		}
	private://成员
		LARGE_INTEGER m_DueTime;
		KDPC m_kDpc;
		LONGLONG ltime;
		LONG m_period;
		KTIMER m_kTimer;
		std::function<void()>m_function;
		bool p_dd;
	public:
		CTimer()
		{
			ltime = 0;
			m_period = 0;
			p_dd = false;
		}
		template<class _Fn,
			class... _Args,
			class = typename std::enable_if<
			!std::is_same<typename std::decay<_Fn>::type, CTimer>::value>::type>
			explicit CTimer(LONGLONG timer_time, LONG timer_times, _Fn&& _Fx, _Args&&... _Ax)
		{
			ltime = -timer_time;
			m_period = timer_times;
			p_dd = true;
			if (timer_times)
				KeInitializeTimerEx(&m_kTimer, SynchronizationTimer);
			else
				KeInitializeTimer(&m_kTimer);
			m_function = std::bind(std::forward<_Fn>(_Fx), std::forward<_Args>(_Ax)...);

			KeInitializeDpc(&m_kDpc, _launch_callback_timer, this);
			m_DueTime.QuadPart = this->ltime;
			if (this->m_period != 0)
			{
				KeSetTimerEx(&m_kTimer, m_DueTime, m_period, &m_kDpc);
			}
			else
			{
				KeSetTimer(&m_kTimer, m_DueTime, &m_kDpc);
			}
		}
		CTimer & operator = (CTimer &timer_)
		{
			KdBreakPoint();
			this->p_dd = timer_.p_dd;
			this->ltime = timer_.ltime;
			this->m_period = timer_.m_period;
			this->m_function = timer_.m_function;
			timer_.set_rel();

			if (this->m_period)
				KeInitializeTimerEx(&this->m_kTimer, SynchronizationTimer);
			else
				KeInitializeTimer(&this->m_kTimer);
			KeInitializeDpc(&this->m_kDpc, _launch_callback_timer, this);
			this->m_DueTime.QuadPart = this->ltime;
			if (this->m_period != 0)
			{
				KeSetTimerEx(&this->m_kTimer, this->m_DueTime, this->m_period, &this->m_kDpc);
			}
			else
			{
				KeSetTimer(&this->m_kTimer, this->m_DueTime, &this->m_kDpc);
			}

			return (*this);
		}
		~CTimer()
		{
			if (!p_dd)
				return;
			LOG_DEBUG("Begin Timer Release\r\n");
			KeSetTimer(&this->m_kTimer, m_DueTime, NULL);
			KeCancelTimer(&m_kTimer);
			if (this->m_period != 0)
			{
				KeFlushQueuedDpcs();
			}
			LOG_DEBUG("Release Timer\r\n");
		}
	private:
		void set_rel()
		{
			p_dd = false;
			KeSetTimer(&this->m_kTimer, m_DueTime, NULL);
			KeCancelTimer(&m_kTimer);
			if (this->m_period != 0)
			{
				KeFlushQueuedDpcs();
			}
		}
	};
};