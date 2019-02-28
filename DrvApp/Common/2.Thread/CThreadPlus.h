#pragma once

//线程模式所有CPU运行
namespace ddk
{
	class CThreadPlus;
	class _ddkPadPlus
	{
	public:
		virtual void _Go() = 0;
		virtual void _Release() = 0;
		static VOID _launch_callback(IN PVOID _Data)
		{
			//DBG_PRINT("_launch_callback\r\n");
			auto p_this = reinterpret_cast<ddk::_ddkPadPlus*>(_Data);
			__try
			{
				//DBG_PRINT("do callback\r\n");
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
	class _LaunchPadPlus :public _ddkPadPlus
	{	// template class for launching threads
	public:
		template<class _Other> inline
			_LaunchPadPlus(_Other&& _Tgt)
			: _MyTarget(_STD forward<_Other>(_Tgt))
		{	// construct from target
		}
		virtual void _Release() {
			//	DBG_PRINT("free myself\r\n");
			delete this;
		};
		virtual void _Go()
		{	// run the thread function object
			//	DBG_PRINT("_GO GO\r\n");
			bool set_cpu = p_thread->switcher();
			DBG_PRINT("cpu = %d\r\n", KeGetCurrentProcessorNumber());
			_Run(this);
			if (set_cpu)
			{
				KeRevertToUserAffinityThread();
			}
		}
		void _Launch(CThreadPlus *thr)
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

		static void _Run(_LaunchPadPlus *_Ln) _NOEXCEPT	// enforces termination
		{	// construct local unique_ptr and call function object within
			//	DBG_PRINT("_Run\r\n");
			_Target _Local(_STD forward<_Target>(_Ln->_MyTarget));
			_Execute(*_Local,
				std::make_integer_sequence<size_t,
				std::tuple_size<typename _Target::element_type>::value>());
		}
		_Target _MyTarget;
		CThreadPlus *p_thread;
	};

	template<class _Target> inline
		void _Launch(CThreadPlus *_Thr, _Target&& _Tg)
	{	// launch a new thread
		auto _Launcher = new ddk::_LaunchPadPlus<_Target>(std::forward<_Target>(_Tg));
		_Launcher->_Launch(_Thr);
	}

	class CThreadPlus
	{
	public:
		CThreadPlus()
		{
			is_joinable = true;
			object = nullptr;
			h_thread = nullptr;
			tid = 0;
			set_cpu = ULONG(-1);
		}
		~CThreadPlus()
		{
			if (is_joinable)
			{
				//结束线程
				//ddk::util::syscall("ZwTerminateThread",h_thread);
			}
			if (h_thread)
			{
				ZwClose(h_thread);
			}
			if (object)
			{
				ObDereferenceObject(object);
			}

		}
		template<class _Fn,
			class... _Args,
			class = typename std::enable_if<
			!std::is_same<typename std::decay<_Fn>::type, CThreadPlus>::value>::type>
			explicit CThreadPlus(_Fn&& _Fx, _Args&&... _Ax)
		{	// construct with _Fx(_Ax...)
			is_joinable = true;
			object = nullptr;
			h_thread = nullptr;
			tid = 0;
			set_cpu = ULONG(-1);
			ddk::_Launch(this,
				std::make_unique<std::tuple<std::decay_t<_Fn>, std::decay_t<_Args>...> >(
					std::forward<_Fn>(_Fx), std::forward<_Args>(_Ax)...));
		}
		template<class _Fn,
			class... _Args,
			class = typename std::enable_if<
			!std::is_same<typename std::decay<_Fn>::type, CThreadPlus>::value>::type>
			explicit CThreadPlus(ULONG cpu_number, _Fn&& _Fx, _Args&&... _Ax)
		{	// construct with _Fx(_Ax...)
			is_joinable = true;
			object = nullptr;
			h_thread = nullptr;
			tid = 0;
			set_cpu = cpu_number;
			ddk::_Launch(this,
				std::make_unique<std::tuple<std::decay_t<_Fn>, std::decay_t<_Args>...> >(
					std::forward<_Fn>(_Fx), std::forward<_Args>(_Ax)...));
		}
		void join()
		{
			if (is_joinable)
			{
				if (object)
				{
					KeWaitForSingleObject(object, Executive, KernelMode, FALSE, NULL);
				}
			}
		}
		void detach()
		{
			is_joinable = false;
		}
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
			h_thread = thread;
			object = ThreadObject;
			tid = PsGetThreadId(object);
		}
		HANDLE get_native_handle() {
			return h_thread;
		}
		HANDLE get_id() {
			return tid;
		}
		bool switcher()
		{
			if (set_cpu != ULONG(-1))
			{
				KeSetSystemAffinityThread(static_cast<KAFFINITY>(1ull << set_cpu));
				return true;
			}
			return false;
		}
		CThreadPlus & operator =(CThreadPlus &_thr)
		{
			is_joinable = true;
			object = nullptr;
			h_thread = nullptr;
			tid = 0;
			this->set_handle(_thr.get_native_handle());
			_thr.set_rel();
			return (*this);
		}
		void set_rel()
		{
			h_thread = nullptr;
		}
	private:
		HANDLE tid;
		PETHREAD object;
		HANDLE h_thread;
		bool is_joinable;
		ULONG set_cpu;
	};

};

