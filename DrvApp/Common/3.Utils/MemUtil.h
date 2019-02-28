#pragma once
namespace ddk::util {
	typedef struct _MM_SYSTEM_MAP
	{
		PMDL pMdl;
		PVOID pMappedAddress;
	}MM_SYSTEM_MAP, *PMM_SYSTEM_MAP;

	typedef struct _MM_LOCKED_MEM
	{
		PVOID OrgBuffer;
		PVOID MappedBuffer;
		PMDL MapMdl;
	}MM_LOCKED_MEM, *PMM_LOCKED_MEM;

	class MemUtil {
	public:
		static void *MmMemMatch(const void *BaseAddr, SIZE_T TotalSize, 
			const void *Pattern,SIZE_T PatternSize) {
			//从BaseAddr开始到TotalSize结束，寻找大小为PatternSize的Patter
			//成功返回地址，不成功返回NULL
			if (!BaseAddr || !Pattern || PatternSize > TotalSize)
				return NULL;
			char *pBase = (char *)BaseAddr;
			for (int i = 0; i < TotalSize - PatternSize; i++)
			{
				if (!memcmp(&pBase[i], Pattern, PatternSize))
					return &pBase[i];
			}
			return NULL;
		}

		static NTSTATUS MmSearch(IN PUCHAR adresseBase, IN PUCHAR adresseMaxMin, 
			IN PUCHAR pattern, OUT PUCHAR *addressePattern,IN SIZE_T longueur)
		{
			for (*addressePattern = adresseBase;
				(adresseMaxMin > adresseBase) ? (*addressePattern <= adresseMaxMin) : (*addressePattern >= adresseMaxMin);
				*addressePattern += (adresseMaxMin > adresseBase) ? 1 : -1)
				if (RtlEqualMemory(pattern, *addressePattern, longueur))
					return STATUS_SUCCESS;
			*addressePattern = NULL;
			return STATUS_NOT_FOUND;
		}

		static NTSTATUS MmGenericPointerSearch(OUT PUCHAR *addressePointeur, IN PUCHAR adresseBase,
			IN PUCHAR adresseMaxMin, IN PUCHAR pattern, IN SIZE_T longueur, IN LONG offsetTo)
		{
			NTSTATUS status = MmSearch(adresseBase,
				adresseMaxMin,
				pattern,
				addressePointeur,
				longueur);
			if (NT_SUCCESS(status))
			{
				*addressePointeur += offsetTo;
#ifdef _AMD64_
				*addressePointeur += sizeof(LONG) + *(PLONG)(*addressePointeur);
#else
				*addressePointeur = *(PUCHAR *)(*addressePointeur);
#endif

				if (!*addressePointeur)
					status = STATUS_INVALID_HANDLE;
			}
			return status;
		}
		////////////下面未整理/////////////
		static BOOL MmMapSystemMemoryWritable(PVOID Address, SIZE_T _MapSize, PMM_SYSTEM_MAP pMap)
		{
			auto pMdl = IoAllocateMdl(Address, static_cast<ULONG>(_MapSize), FALSE, FALSE,
				nullptr);
			if (!pMdl)
			{
				return FALSE;
			}
			auto exit_mdl = std::experimental::make_scope_exit([&]() {IoFreeMdl(pMdl); });
			MmBuildMdlForNonPagedPool(pMdl);
			__try
			{
				MmProbeAndLockPages(pMdl, KernelMode, IoModifyAccess);
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				return FALSE;
			}
			exit_mdl.release();

			auto pMapAddr = (PVOID)MmMapLockedPages(pMdl, KernelMode);
			if (pMapAddr == NULL)
			{
				return FALSE;
			}
			pMap->pMappedAddress = pMapAddr;
			pMap->pMdl = pMdl;
			return TRUE;
		}
		static void MmFreeMap(PMM_SYSTEM_MAP pMap)
		{
			MmUnmapLockedPages(pMap->pMappedAddress, pMap->pMdl);
			IoFreeMdl(pMap->pMdl);
		}

		////////////////下面先注释//////////////////////
		//static bool MiAllocateMemorySpecialAddress(PVOID _TargetAddress, SIZE_T _Size)
		//{
		//	auto _RealAllocateSize = ddk::util::CommonUtil::AlignSize(_Size, PAGE_SIZE);
		//	for (auto i = 0ull; i<_RealAllocateSize; i += PAGE_SIZE)
		//	{
		//		auto pNewVa = reinterpret_cast<PVOID>((PUCHAR)_TargetAddress + i);
		//		if (MmIsAccessibleAddress(pNewVa))
		//		{
		//			return false;
		//		}
		//	}
		//	MM_LOCKED_MEM mm = {};
		//	if (!MmAllocateLockedMemory(_RealAllocateSize, &mm))
		//	{
		//		return false;
		//	}
		//	for (auto i = 0ull; i<_RealAllocateSize; i += PAGE_SIZE)
		//	{
		//		auto pNewMapVa = reinterpret_cast<PVOID>((PUCHAR)_TargetAddress + i);
		//		auto pNewOrgVa = reinterpret_cast<PVOID>((PUCHAR)mm.MappedBuffer + i);
		//		if (!MiMapAddressToMapAddress(pNewMapVa, pNewOrgVa))
		//		{
		//			if (i == 0)
		//			{
		//				//只有第一次映射才能释放
		//				MmFreeLockedMemory(&mm);
		//			}
		//			return false;
		//		}
		//	}
		//	return true;
		//}
		//
		static PVOID MmAllocMemoryForHook(PVOID _target, size_t _size)
		{
			PVOID imageBase = nullptr;
			PVOID alloc_address = nullptr;
			RtlPcToFileHeader(_target, &imageBase);
			if (!imageBase)
			{
				//临近内存？？
				alloc_address = PVOID((PUCHAR)_target + PAGE_SIZE);
			}
		//	else
		//	{
		//		alloc_address = PVOID((PUCHAR)imageBase + ddk::util::CommonUtil::AlignSize(ddk::util::PEUtil::LdrGetImageSize(imageBase), PAGE_SIZE) + PAGE_SIZE);
		//	}

		//	auto MaxBase = (DWORD64)alloc_address + 0x0FFFFFFFFULL;
		//	while ((DWORD64)alloc_address < MaxBase)
		//	{
		//		alloc_address = PVOID((ULONG_PTR)alloc_address & 0xFFFFFFFFFFFFF000ULL);
		//		if (MiAllocateMemorySpecialAddress(alloc_address, _size))
		//		{
		//			return alloc_address;
		//		}
		//		alloc_address = PVOID((PUCHAR)alloc_address + PAGE_SIZE);
		//	}
			return nullptr;
		}
	};
}