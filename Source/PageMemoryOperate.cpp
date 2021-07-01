#include "PageMemoryOperate.h"

PageMemoryOperate PageMemoryOperate::PageInstence;


//申请比较大的对象时候，直接从系统申请
Bridge* PageMemoryOperate::RequestLargePageObject(size_t PageSize)
{
	assert(PageSize > MaxByte);

	PageSize = CalculateSize::GetListRoundUp(PageSize, PageShift); //对齐
	size_t PNumPage = PageSize >> PageShift;
	if (PNumPage < NumPage)
	{
		Bridge* PBridge = NewBridge(PNumPage);
		PBridge->BridgeObjectSize = PageSize;
		return PBridge;
	}
	else
	{
		void* pointer = VirtualAlloc(0, PNumPage << PageShift,
			MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

		if (pointer == nullptr)
			throw std::bad_alloc();

		Bridge* Rbridge = new Bridge;
		Rbridge->BridgeNumPage = PNumPage;
		Rbridge->BridgePageId = (PageIndexType)pointer >> PageShift;
		Rbridge->BridgeObjectSize = PNumPage << PageShift;

		GetIndexBridgeMap[Rbridge->BridgePageId] = Rbridge;

		return Rbridge;
	}
}

void PageMemoryOperate::ReleaseLargePageObject(void* pointer, Bridge* bridge)
{
	size_t FNumPage = bridge->BridgeObjectSize >> PageShift;
	if (FNumPage < NumPage) //FNumPage小于128页时候，释放bridge
	{
		bridge->BridgeObjectSize = 0;
		ReleaseBridgeToPageMemory(bridge);
	}
	else
	{
		GetIndexBridgeMap.erase(FNumPage);
		delete bridge;
		VirtualFree(pointer, 0, MEM_RELEASE);
	}
}

Bridge* PageMemoryOperate::NewBridge(size_t Num)
{

	
	std::unique_lock<std::mutex> NLock(GetMutex);
	//加锁的原因：全局加锁可以保证某一刻只有一个线程到PageMemory申请到bridge
	return GetNewBridge(Num);
}



Bridge* PageMemoryOperate::GetNewBridge(size_t Num)
{
	assert(Num < NumPage);
	if (!GetBridgeList[Num].JudgeEmpty())
		return GetBridgeList[Num].PopNodeFront();

	for (size_t i = Num + 1; i < NumPage; ++i)
	{
		if (!GetBridgeList[i].JudgeEmpty())
		{
			Bridge* bridge = GetBridgeList[i].PopNodeFront();
			Bridge* bridgeList = new Bridge;

			bridgeList->BridgePageId = bridge->BridgePageId;
			bridgeList->BridgeNumPage = Num;
			bridge->BridgePageId = bridge->BridgePageId + Num;
			bridge->BridgeNumPage = bridge->BridgeNumPage - Num;

	 

			for (size_t i = 0; i < Num; ++i)
				GetIndexBridgeMap[bridgeList->BridgePageId + i] = bridgeList;

	 

			GetBridgeList[bridge->BridgeNumPage].InsertNodeFront(bridge);
			return bridgeList;
		}
	}

	Bridge* bridge = new Bridge;
	//此时表明BridgeList里无合适bridge,从而直接向系统申请128页内存
 
#ifdef _WIN32
	void* pointer = VirtualAlloc(0, (NumPage - 1)*(1 << PageShift), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	//  brk
#endif


	bridge->BridgePageId = (PageIndexType)pointer >> PageShift;
	bridge->BridgeNumPage = NumPage - 1;

	for (size_t i = 0; i < bridge->BridgeNumPage; ++i)
		GetIndexBridgeMap[bridge->BridgePageId + i] = bridge;

	GetBridgeList[bridge->BridgeNumPage].InsertNodeFront(bridge);   
	return GetNewBridge(Num);
}

//得到从对象指向bridge的映射关系
Bridge* PageMemoryOperate::SetMapObjectToBridge(void* object)
{
	//统计页码
	PageIndexType index = (PageIndexType)object >> PageShift;
	auto sIt = GetIndexBridgeMap.find(index);
	if (sIt != GetIndexBridgeMap.end())
	{
		return sIt->second;
	}
	else
	{
		assert(false);
		return nullptr;
	}
}

void PageMemoryOperate::ReleaseBridgeToPageMemory(Bridge* CurBridge)
{ 
	std::unique_lock<std::mutex> RLock(GetMutex);//全局锁保证一次只有一个线程从ThreadMemory归还数据



	//Bridge释放内存页数大于128，不合并，直接向系统释放内存
	if (CurBridge->BridgeNumPage >= NumPage)
	{
		void* pointer = (void*)(CurBridge->BridgePageId << PageShift);

		GetIndexBridgeMap.erase(CurBridge->BridgePageId);//删除page到bridge的映射
		VirtualFree(pointer, 0, MEM_RELEASE);
		delete CurBridge;
		return;
	}


	// 向前合并page
	while (1)
	{


		PageIndexType PageCurIndex = CurBridge->BridgePageId;
		PageIndexType PagePreIndex = PageCurIndex - 1;
		auto PageIt = GetIndexBridgeMap.find(PagePreIndex);

		// 无法找到映射关系，退出
		if (PageIt == GetIndexBridgeMap.end())
			break;

		// 前一个bridge不空闲BridgeUseCount
		if (PageIt->second->BridgeUseCount != 0)//BridgeUseCount
			break;

		Bridge* PrevBridge = PageIt->second;

		//大于128页，就不进行合并
		if (CurBridge->BridgeNumPage + PrevBridge->BridgeNumPage > NumPage - 1)
			break;

		//删除链表的PrevBridge
		GetBridgeList[PrevBridge->BridgeNumPage].EraseNode(PrevBridge);

		// 合并链表
		PrevBridge->BridgeNumPage += CurBridge->BridgeNumPage;
		//调整BridgePageId指向bridge的映射关系
		for (PageIndexType i = 0; i < CurBridge->BridgeNumPage; ++i)
		{
			GetIndexBridgeMap[CurBridge->BridgePageId + i] = PrevBridge;
		}
		delete CurBridge;

		// 接着向前合并链表
		CurBridge = PrevBridge;
	}


	//向后合并
	while (1)
	{


		PageIndexType CurPageIndex = CurBridge->BridgePageId;
		PageIndexType NextPageIndex = CurPageIndex + CurBridge->BridgeNumPage;

		auto bridgeIt = GetIndexBridgeMap.find(NextPageIndex);

		if (bridgeIt == GetIndexBridgeMap.end())
			break;

		if (bridgeIt->second->BridgeUseCount != 0)
			break;

		Bridge* NextBridge = bridgeIt->second;

		///////大于128页，就不进行合并
		if (CurBridge->BridgeNumPage + NextBridge->BridgeNumPage >= NumPage - 1)
			break;

		GetBridgeList[NextBridge->BridgeNumPage].EraseNode(NextBridge);

		CurBridge->BridgeNumPage += NextBridge->BridgeNumPage;
		////调整BridgePageId指向bridge的映射关系
		for (PageIndexType i = 0; i < NextBridge->BridgeNumPage; ++i)
		{
			GetIndexBridgeMap[NextBridge->BridgePageId + i] = CurBridge;
		}

		delete NextBridge;
	}

	// 最后将合并好的Bridge插入到BridgeList链的前部位置
	GetBridgeList[CurBridge->BridgeNumPage].InsertNodeFront(CurBridge);
}
