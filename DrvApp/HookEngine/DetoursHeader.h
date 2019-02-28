#pragma once
#include <vector>
//
//用到在包含，放到预编译头里没用
//
// Distorm v3.3 X86/X64
#include "mem_util.h"
#include "distorm/distorm.h"
#include "asmjit/asmjit/src/asmjit/base.h"
#include "Intrinsic.h"
#include "AsmGen.h"
#include "Reassembler.h"
#include "Detours.h"
#include "DetoursInternal.h"

// TODO:  在此处引用程序需要的其他头文件
#ifdef _WIN64
namespace hook = Detours::X64;
#else
namespace hook = Detours::X86;
#endif
