#pragma once

#include "Together.h"
#include "ThreadMemoryOperate.h"
#include "PageMemoryOperate.h"

//�������ã��ĸ��߳�����֮����Ҫ�ڴ�͵�������ӿ�

//�µ��߳����ˣ�����ӿھͿ��Է����ڴ���߳�
static inline void* ConcurrentAllocateMemory(size_t ThreadSize)
{
	if (ThreadSize > MaxByte)
	{//ThreadSize > MaxByte ���Ǵ���64K��ֱ�ӻ�ȡϵͳ�ڴ�
		
		Bridge* CBridge = PageMemoryOperate::GetPageInstence()->RequestLargePageObject(ThreadSize);
		void* pointer = (void*)(CBridge->BridgePageId << PageShift);
		return pointer;
	}
	else
	{	//�״ξ�new ThreadMemoryOperate���д�������
		//�Ժ��ʹ���Ѿ�����ThreadList�Ķ���
		if (ThreadList == nullptr) 
		{
			ThreadList = new ThreadMemoryOperate;
		}

		return ThreadList->RequestMemory(ThreadSize);
	}
}



static inline void ConcurrentMemoryRelease(void* pointer)//�ͷŻ�ȡ�����ڴ�
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