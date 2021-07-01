#pragma once

#include "Together.h"
#include "ThreadMemoryOperate.h"
#include "PageMemoryOperate.h"

//被动调用，哪个线程来了之后，需要内存就调用这个接口

//新的线程来了，这个接口就可以分配内存给线程
static inline void* ConcurrentAllocateMemory(size_t ThreadSize)
{
	if (ThreadSize > MaxByte)
	{//ThreadSize > MaxByte 即是大于64K，直接获取系统内存
		
		Bridge* CBridge = PageMemoryOperate::GetPageInstence()->RequestLargePageObject(ThreadSize);
		void* pointer = (void*)(CBridge->BridgePageId << PageShift);
		return pointer;
	}
	else
	{	//首次就new ThreadMemoryOperate自行创建对象
		//以后就使用已经存在ThreadList的对象
		if (ThreadList == nullptr) 
		{
			ThreadList = new ThreadMemoryOperate;
		}

		return ThreadList->RequestMemory(ThreadSize);
	}
}



static inline void ConcurrentMemoryRelease(void* pointer)//释放获取到的内存
{
	Bridge* CMBridge = PageMemoryOperate::GetPageInstence()->SetMapObjectToBridge(pointer);
	size_t CMSize = CMBridge->BridgeObjectSize;
	if (CMSize > MaxByte)
	{
		PageMemoryOperate::GetPageInstence()->ReleaseLargePageObject(pointer, CMBridge);
	}
	else
	{
		ThreadList->ReleaseMemory(pointer, CMSize);
	}
}