#pragma once

#include "Together.h"

class ThreadMemoryOperate
{
private:
	ChangeList ThreadChangeList[NumList];//��������

public:

	void* RequestMemory(size_t MemorySize);//�����ڴ����
	void ReleaseMemory(void* pointer, size_t MemorySize);//�ͷ��ڴ����


	void* GetObjectFromMedium(size_t MediumIndex, size_t MemorySize);//��Medium��ȡ����

	void ChangeListSuperLong(ChangeList* CTlist, size_t MemorySize);//�����ȳ����������ڴ浽Medium
};

//��̬�ģ��߳��߳�
//ÿ��thread�и��Դ�ָ��, ��(_declspec (thread))��ʹ��ʱÿ�ζ����߳��Լ���ָ�룬��ˣ����Բ�����

_declspec (thread) static ThreadMemoryOperate* ThreadList = nullptr;