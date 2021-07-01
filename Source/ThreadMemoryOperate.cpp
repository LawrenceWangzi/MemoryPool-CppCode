#include "ThreadMemoryOperate.h"
#include "MediumMemoryOperate.h"



//从Medium获取缓存对象，因为每次申请内存需要加锁，因此每次申请相对多一点内存，减少加锁次数，提高效率。
void* ThreadMemoryOperate::GetObjectFromMedium(size_t MediumIndex, size_t MemorySize)
{

	ChangeList* GetThreadChangeList = &ThreadChangeList[MediumIndex];
	 


	//申请对象大小与内存块数量大小成负相关，申请次数与数量呈正相关
	size_t MaxListSize = GetThreadChangeList->sMaxSize();
	size_t NumListToMove = min(CalculateSize::GetSizeNum(MemorySize), MaxListSize);

	void* MemoryStart = nullptr, *MemoryEnd = nullptr;//MemoryStart和MemoryEnd表示取出内存的首尾标记位



	size_t GetMemoryBatchSize = MediumMemoryOperate::GetMediumInstence()->GetOrderByRangeObject(MemoryStart, MemoryEnd, NumListToMove, MemorySize);
	//GetMemoryBatchSize表示获取内存的多少
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

//链表长度超长，回收内存到Medium
void ThreadMemoryOperate::ChangeListSuperLong(ChangeList* CTlist, size_t MemorySize)
{

	void* MemoryStart = CTlist->PopOrderByRange();
	MediumMemoryOperate::GetMediumInstence()->ReleaseListMemoryToBridge(MemoryStart, MemorySize);
}


void* ThreadMemoryOperate::RequestMemory(size_t MemorySize)//申请内存对象
{
	size_t ListIndex = CalculateSize::ListIndex(MemorySize);//获取链对象的位置
	ChangeList* CList = &ThreadChangeList[ListIndex];


	//可变链表为空时，到MediumMemory处获取对象并取多个，减少加锁次数
	

	if (!CList->JudgeEmpty())//如果ThreadMemory非空，可以取其内存
	{
		return CList->PopObject();
	}
	


	else
	{
		// 从MediumMemory处获取对象
		return GetObjectFromMedium(ListIndex, CalculateSize::ListRoundUp(MemorySize));
	}
}

void ThreadMemoryOperate::ReleaseMemory(void* pointer, size_t MemorySize)
{
	size_t ListIndex = CalculateSize::ListIndex(MemorySize);
	ChangeList* CList = &ThreadChangeList[ListIndex];
	CList->PushObject(pointer);

	//满足某个条件时(释放回一个批量的对象)，释放回中心缓存
	if (CList->sSize() >= CList->sMaxSize())
	{
		ChangeListSuperLong(CList, MemorySize);
	}
}


