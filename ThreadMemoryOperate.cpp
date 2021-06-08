#include "ThreadMemoryOperate.h"
#include "MediumMemoryOperate.h"



//��Medium��ȡ���������Ϊÿ�������ڴ���Ҫ���������ÿ��������Զ�һ���ڴ棬���ټ������������Ч�ʡ�
void* ThreadMemoryOperate::GetObjectFromMedium(size_t MediumIndex, size_t MemorySize)
{

	ChangeList* GetThreadChangeList = &ThreadChangeList[MediumIndex];
	 


	//��������С���ڴ��������С�ɸ���أ���������������������
	size_t MaxListSize = GetThreadChangeList->sMaxSize();
	size_t NumListToMove = min(CalculateSize::GetSizeNum(MemorySize), MaxListSize);

	void* MemoryStart = nullptr, *MemoryEnd = nullptr;//MemoryStart��MemoryEnd��ʾȡ���ڴ����β���λ



	size_t GetMemoryBatchSize = MediumMemoryOperate::GetMediumInstence()->GetOrderByRangeObject(MemoryStart, MemoryEnd, NumListToMove, MemorySize);
	//GetMemoryBatchSize��ʾ��ȡ�ڴ�Ķ���
	if (GetMemoryBatchSize > 1)
	{
		GetThreadChangeList->PushOrderByRange(NextObject(MemoryStart), MemoryEnd, GetMemoryBatchSize - 1);
	}

	if (GetMemoryBatchSize >= GetThreadChangeList->sMaxSize())
	{
		GetThreadChangeList->SetListMaxSize(MaxListSize + 1);
	}

	return MemoryStart;
}

//�����ȳ����������ڴ浽Medium
void ThreadMemoryOperate::ChangeListSuperLong(ChangeList* CTlist, size_t MemorySize)
{

	void* MemoryStart = CTlist->PopOrderByRange();
	MediumMemoryOperate::GetMediumInstence()->ReleaseListMemoryToBridge(MemoryStart, MemorySize);
}


void* ThreadMemoryOperate::RequestMemory(size_t MemorySize)//�����ڴ����
{
	size_t ListIndex = CalculateSize::ListIndex(MemorySize);//��ȡ�������λ��
	ChangeList* CList = &ThreadChangeList[ListIndex];


	//�ɱ�����Ϊ��ʱ����MediumMemory����ȡ����ȡ��������ټ�������
	

	if (!CList->JudgeEmpty())//���ThreadMemory�ǿգ�����ȡ���ڴ�
	{
		return CList->PopObject();
	}
	


	else
	{
		// ��MediumMemory����ȡ����
		return GetObjectFromMedium(ListIndex, CalculateSize::ListRoundUp(MemorySize));
	}
}

void ThreadMemoryOperate::ReleaseMemory(void* pointer, size_t MemorySize)
{
	size_t ListIndex = CalculateSize::ListIndex(MemorySize);
	ChangeList* CList = &ThreadChangeList[ListIndex];
	CList->PushObject(pointer);

	//����ĳ������ʱ(�ͷŻ�һ�������Ķ���)���ͷŻ����Ļ���
	if (CList->sSize() >= CList->sMaxSize())
	{
		ChangeListSuperLong(CList, MemorySize);
	}
}


