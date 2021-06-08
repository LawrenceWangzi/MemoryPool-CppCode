#pragma once

#include "Together.h"

class ThreadMemoryOperate
{
private:
	ChangeList ThreadChangeList[NumList];//自由链表

public:

	void* RequestMemory(size_t MemorySize);//申请内存对象
	void ReleaseMemory(void* pointer, size_t MemorySize);//释放内存对象


	void* GetObjectFromMedium(size_t MediumIndex, size_t MemorySize);//从Medium获取对象

	void ChangeListSuperLong(ChangeList* CTlist, size_t MemorySize);//链表长度超长，回收内存到Medium
};

//静态的，线程线程
//每个thread有个自带指针, 用(_declspec (thread))，使用时每次都是线程自己的指针，因此，可以不加锁

_declspec (thread) static ThreadMemoryOperate* ThreadList = nullptr;