#pragma once

#include "Common.h"
#include "ThreadCache.h"
#include "PageCache.h"

//�������ã��ĸ��߳�����֮����Ҫ�ڴ�͵�������ӿ�
static inline void* ConcurrentAlloc(size_t size)
{
	if (size > MAX_BYTES)//����һ�����ֵ 64k�����Լ���ϵͳ�л�ȡ������ʹ���ڴ��
	{
		//return malloc(size);
		Span* span = PageCache::GetInstence()->AllocBigPageObj(size);
		void* ptr = (void*)(span->_pageid << PAGE_SHIFT);
		return ptr;
	}
	else
	{
		if (tlslist == nullptr)//��һ�������Լ��������������ģ��Ϳ���ֱ��ʹ�õ�ǰ�����õ��ڴ��
		{
			tlslist = new ThreadCache;
		}

		return tlslist->Allocate(size);
	}
}

//static inline void ConcurrentFree(void* ptr, size_t size)//����ͷ�
//{
//	if (size > MAX_BYTES)
//	{
//		free(ptr);
//	}
//	else
//	{
//		tlslist->Deallocate(ptr, size);
//	}
//}

static inline void ConcurrentFree(void* ptr)//����ͷ�
{
	Span* span = PageCache::GetInstence()->MapObjectToSpan(ptr);
	size_t size = span->_objsize;
	if (size > MAX_BYTES)
	{
		//free(ptr);
		PageCache::GetInstence()->FreeBigPageObj(ptr, span);
	}
	else
	{
		tlslist->Deallocate(ptr, size);
	}
}