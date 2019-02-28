================================================================================
    Hook概述：有关这个项目的使用方法
===============================================================================

1.在需要地方包含#include <HookEntry.h>
2.调用HookEntry函数
3.所有你自己的实现写在MainHook中


=====Hook文头文件========
//HOOK 框架，支持x86和x64
#include <HookEngine/DetoursHeader.h>

//IRP HOOK，支持x86和x64
#include <HookEngine/irphook/IRPHook.h>
#include <HookEngine/irphook/IRPHookEx.h>