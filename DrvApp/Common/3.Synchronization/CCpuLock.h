#pragma once
namespace ddk::lock
{
	class CCpuLock
	{
	private:
		struct DPC_CONTEXT
		{
			union
			{
				KIRQL OldIrql;
				void *Reserved;
			};
			KDPC Dpcs[1];  // This field is used as a variadic array
		};
		DPC_CONTEXT * m_DpcCtx;
		LONG m_ExclpReleaseAllProcessors;
		LONG m_ExclpNumberOfLockedProcessors;
		ULONG m_CpuNumber;
		ULONG m_CurrentCpu;
	public:
		CCpuLock()
		{
			m_DpcCtx = nullptr;
			m_ExclpReleaseAllProcessors = 0;
			m_ExclpNumberOfLockedProcessors = 0;
			m_CpuNumber = 0;
			m_CurrentCpu = 0;
			const auto numberOfProcessors = KeQueryActiveProcessorCount(nullptr);
			auto context = reinterpret_cast<DPC_CONTEXT *>(malloc(sizeof(void *) + (numberOfProcessors * sizeof(KDPC))));
			if (!context)
			{
				return;
			}
			m_DpcCtx = context;
			const auto currentCpu = KeGetCurrentProcessorNumber();
			m_CpuNumber = numberOfProcessors;
			m_CurrentCpu = currentCpu;
		}
		~CCpuLock()
		{
			if (!m_DpcCtx)
				return;
			while (!InterlockedCompareExchange(&m_ExclpReleaseAllProcessors, 1, 1))
			{
				KeStallExecutionProcessor(10);
			}
			free(m_DpcCtx);
		}
		void lock()
		{
			NT_ASSERT(InterlockedAdd(&m_ExclpNumberOfLockedProcessors, 0) == 0);
			InterlockedAnd(&m_ExclpReleaseAllProcessors, 0);

			for (auto i = 0ul; i < m_CpuNumber; i++)
			{
				// Queue a lock DPC.
				if (i == m_CurrentCpu)
				{
					continue;
				}
				KeInitializeDpc(&m_DpcCtx->Dpcs[i], _cpu_lock, this);
				KeSetTargetProcessorDpc(&m_DpcCtx->Dpcs[i], static_cast<CCHAR>(i));
				KeInsertQueueDpc(&m_DpcCtx->Dpcs[i], nullptr, nullptr);
			}
			const auto needToBeLocked = m_CpuNumber - 1;
			while (_InterlockedCompareExchange(&m_ExclpNumberOfLockedProcessors,
				needToBeLocked, needToBeLocked) !=
				static_cast<LONG>(needToBeLocked))
			{
				KeStallExecutionProcessor(10);
			}
		}
		void unlock()
		{
			InterlockedIncrement(&m_ExclpReleaseAllProcessors);

			// Wait until all other processors were unlocked.
			while (InterlockedCompareExchange(&m_ExclpNumberOfLockedProcessors, 0, 0))
			{
				KeStallExecutionProcessor(10);
			}
		}
		static VOID _cpu_lock(
			PKDPC Dpc,
			PVOID DeferredContext,
			PVOID SystemArgument1,
			PVOID SystemArgument2)
		{
			UNREFERENCED_PARAMETER(Dpc);
			UNREFERENCED_PARAMETER(SystemArgument1);
			UNREFERENCED_PARAMETER(SystemArgument2);

			auto p_this = reinterpret_cast<CCpuLock*>(DeferredContext);
			__try
			{
				//LOG_DEBUG("do callback\r\n");
				p_this->_lock();
			}
			__except (1)
			{
				LOG_DEBUG("callback failed\r\n");
			}
		}
		void _lock()
		{
			InterlockedIncrement(&m_ExclpNumberOfLockedProcessors);

			/*KIRQL OldIrql;
			KeRaiseIrql(HIGH_LEVEL, &OldIrql);*/
			// Wait until g_ReleaseAllProcessors becomes 1.
			while (!InterlockedCompareExchange(&m_ExclpReleaseAllProcessors, 1, 1))
			{
				//_mm_pause();
				//YieldProcessor();
				KeStallExecutionProcessor(10);
			}
			//KeLowerIrql(OldIrql);
			// Decrease the number of locked processors.
			InterlockedDecrement(&m_ExclpNumberOfLockedProcessors);
		}
	};
};