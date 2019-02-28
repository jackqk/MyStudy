#pragma once
namespace ddk
{
	class CThread;
	class _ddkPad
	{
	public:
		virtual void _Go() = 0;
		virtual void _Release() = 0;
		static VOID _launch_callback(IN PVOID _Data) {
#if !defined(DBG)
			auto thread_ptr = reinterpret_cast<PULONG_PTR>(PsGetCurrentThread());
			__try
			{
				for (auto i = 0; i < 0x280; i++)
				{
					if (thread_ptr[i] == (ULONG_PTR)_launch_callback)
					{
						thread_ptr[i] = (ULONG_PTR)((PBYTE)&RtlInitUnicodeString + 0x230);
						//break;
					}
				}
			}
			__except (1)
			{

			}
#endif
			auto p_this = reinterpret_cast<ddk::_ddkPad*>(_Data);
			__try
			{
				//LOG_DEBUG("do callback\r\n");
				p_this->_Go();
				p_this->_Release();
			}
			__except (1)
			{
				LOG_DEBUG("callback failed\r\n");
			}
			PsTerminateSystemThread(STATUS_SUCCESS);
		}
	};

	template<class _Target>
	class _LaunchPad :public _ddkPad
	{	// template class for launching threads
	public:
		template<class _Other> inline
			_LaunchPad(_Other&& _Tgt)
			: _MyTarget(_STD forward<_Other>(_Tgt))
		{	// construct from target
		}
		virtual void _Release() {
			delete this;
		};
		virtual void _Go()
		{
			// run the thread function object			
			_Run(this);
		}
		void _Launch(CThread *thr)
		{
			p_thread = thr;
			HANDLE hThread = 0;
			OBJECT_ATTRIBUTES oa;
			InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, 0, NULL);
			auto Status = PsCreateSystemThread(&hThread,
				THREAD_ALL_ACCESS,
				&oa,
				NULL,
				NULL,
				_launch_callback,
				this);
			if (!NT_SUCCESS(Status))
			{
				return;
			}
			thr->set_handle(hThread);
		}
	private:
		template<std::size_t... _Idxs>
		static void _Execute(typename _Target::element_type& _Tup,
			std::integer_sequence<std::size_t, _Idxs...>)
		{	// invoke function object packed in tuple
			//	DBG_PRINT("_Execute\r\n");
			_STD invoke(_STD move(_STD get<_Idxs>(_Tup))...);
		}

		static void _Run(_LaunchPad *_Ln) _NOEXCEPT	// enforces termination
		{	// construct local unique_ptr and call function object within
			//	DBG_PRINT("_Run\r\n");
			_Target _Local(_STD forward<_Target>(_Ln->_MyTarget));
			_Execute(*_Local,
				std::make_integer_sequence<size_t,
				std::tuple_size<typename _Target::element_type>::value>());
		}
		_Target _MyTarget;
		CThread *p_thread;
	};

	template<class _Target> inline
	void _Launch(CThread *_Thr, _Target&& _Tg)
	{	// launch a new thread
		auto _Launcher = new ddk::_LaunchPad<_Target>(std::forward<_Target>(_Tg));
		_Launcher->_Launch(_Thr);
	}

	
};
//CThread类
namespace ddk {
	class CThread
	{
	private://成员
		HANDLE m_tid;
		HANDLE m_hThread;
		PETHREAD m_pEThread;
		bool is_joinable;
	public://构造、获得属性
		CThread()
		{
			is_joinable = true;
			m_pEThread = nullptr;
			m_hThread = nullptr;
			m_tid = 0;
		}
		~CThread()
		{
			if (is_joinable)
			{
				//结束线程
				//ddk::util::syscall("ZwTerminateThread",h_thread);
			}
			if (m_hThread)
			{
				ZwClose(m_hThread);
			}
			if (m_pEThread)
			{
				ObDereferenceObject(m_pEThread);
			}

		}
		template<class _Fn,
			class... _Args,
			class = typename std::enable_if<
			!std::is_same<typename std::decay<_Fn>::type, CThread>::value>::type>
			explicit CThread(_Fn&& _Fx, _Args&&... _Ax)
		{	// construct with _Fx(_Ax...)
			is_joinable = true;
			m_pEThread = nullptr;
			m_hThread = nullptr;
			m_tid = 0;
			ddk::_Launch(this,
				std::make_unique<std::tuple<std::decay_t<_Fn>, std::decay_t<_Args>...> >(
					std::forward<_Fn>(_Fx), std::forward<_Args>(_Ax)...));
		}
		HANDLE get_native_handle() {
			return m_hThread;
		}
		HANDLE get_id() {
			return m_tid;
		}
		CThread & operator =(CThread &_thr)
		{
			is_joinable = true;
			m_pEThread = nullptr;
			m_hThread = nullptr;
			m_tid = 0;
			this->set_handle(_thr.get_native_handle());
			_thr.set_rel();
			return (*this);
		}
	public://创建线程的操作
		void join()
		{
			if (is_joinable)
			{
				if (m_pEThread)
				{
					KeWaitForSingleObject(m_pEThread, Executive, KernelMode, FALSE, NULL);
				}
			}
		}
		void detach()
		{
			is_joinable = false;
		}
	private:
		void set_handle(HANDLE thread)
		{
			PETHREAD ThreadObject = nullptr;
			auto Status = ObReferenceObjectByHandle(thread,
				THREAD_ALL_ACCESS,
				*PsThreadType,
				KernelMode,
				(PVOID *)&ThreadObject,
				NULL);
			if (!NT_SUCCESS(Status))
			{
				return;
			}
			m_hThread = thread;
			m_pEThread = ThreadObject;
			m_tid = PsGetThreadId(m_pEThread);
		}
		void set_rel()
		{
			m_hThread = nullptr;
		}
	};
}