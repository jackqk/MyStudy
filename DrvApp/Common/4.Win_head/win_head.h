#pragma once
#pragma warning(push)
#pragma warning(disable:4005)
#pragma warning(disable:4201)
#pragma warning(disable:4091)
namespace ddk
{
	namespace ntos {
		#include "kdbgblock.h"
		//PKDDEBUGGER_DATA64 get_kdblock();	δ�㶨
		namespace build_7600
		{
			#include "win7.h"
		};
		namespace build_7601
		{
			#include "win7_sp1.h"
		};
		namespace build_9200
		{
			#include "win8_9200.h"
		};
		namespace build_9600
		{
			#include "win8_9600.h"
		};
		namespace build_10586
		{
			#include "win10_10586.h"
		};
		namespace build_14393
		{
			#include "win10_14393.h"
		};
		namespace build_15063
		{
			#include "win10_15063.h"
		};
	}
};
#pragma warning(pop)