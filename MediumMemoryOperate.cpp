#include "MediumMemoryOperate.h"
#include "PageMemoryOperate.h"

MediumMemoryOperate MediumMemoryOperate::MediumInstence;

Bridge* MediumMemoryOperate::GetOneBridge(BridgeList& bridgelist, size_t BridgeByteSize)
{
	Bridge* bridge = bridgelist.GetListBegin();
	while (bridge != bridgelist.GetListEnd())//bridge存在于bridgelist	
	{
		if (bridge->BriegeListNode != nullptr)
			return bridge;
		else
			bridge = bridge->ListNext;
	}

	 

	//此时，表示bridge是空的，到PageMemory获取bridge对象
	Bridge* NewBridge = PageMemoryOperate::GetPageInstence()->NewBridge(CalculateSize::GetPageNum(BridgeByteSize));
 
	//将bridge切段并链接
	char* CurBridge = (char*)(NewBridge->BridgePageId << PageShift);
	char* EndBridge = CurBridge + (NewBridge->BridgeNumPage << PageShift);
	NewBridge->BriegeListNode = CurBridge;
	NewBridge->BridgeObjectSize = BridgeByteSize;

	while (CurBridge + BridgeByteSize < EndBridge)
	{
		char* NextBridge = CurBridge + BridgeByteSize;
		NextObject(CurBridge) = NextBridge;
		CurBridge = NextBridge;
	}
	NextObject(CurBridge) = nullptr;

	bridgelist.InsertNodeFront(NewBridge);

	return NewBridge;
}


//依次获取多个内存对象

size_t MediumMemoryOperate::GetOrderByRangeObject(void*& ListStart, void*& ListEnd, size_t NodeNum, size_t BridgeByteSize)
{
	size_t ListIndex = CalculateSize::ListIndex(BridgeByteSize);
	BridgeList& Mbridgelist = bridgelist[ListIndex];


	std::unique_lock<std::mutex> ListLock(Mbridgelist.BridgeListMutex);


	Bridge* Mbridge = GetOneBridge(Mbridgelist, BridgeByteSize);
	  //此处获得一个Mbridge对象

	//从Mbridge中按顺序获取对象
	size_t BridegBatchSize = 0;
	void* PrevNode = nullptr;//前一个对象的获取
	void* CurNode = Mbridge->BriegeListNode;//用CurNode来遍历Mbridge
	for (size_t i = 0; i < NodeNum; ++i)
	{
		PrevNode = CurNode;
		CurNode = NextObject(CurNode);
		++BridegBatchSize;
		if (CurNode == nullptr)//CurNode为空，马上退出遍历
			break;
	}

	ListStart = Mbridge->BriegeListNode;
	ListEnd = PrevNode;

	Mbridge->BriegeListNode = CurNode;
	Mbridge->BridgeUseCount += BridegBatchSize;

	//根据Mbridge是否为空，空的排后面，非空排前面
	if (Mbridge->BriegeListNode == nullptr)
	{
		Mbridgelist.EraseNode(Mbridge);
		Mbridgelist.InsertNodeEnd(Mbridge);
	}



	return BridegBatchSize;
}

void MediumMemoryOperate::ReleaseListMemoryToBridge(void* ListStart, size_t ListSize)
{
	size_t ListIndex = CalculateSize::ListIndex(ListSize);
	BridgeList& Mbridgelist = bridgelist[ListIndex];


	std::unique_lock<std::mutex> ListLock(Mbridgelist.BridgeListMutex);

	while (ListStart)
	{
		void* NextNode = NextObject(ListStart);



		Bridge* WBridge = PageMemoryOperate::GetPageInstence()->SetMapObjectToBridge(ListStart);
		NextObject(ListStart) = WBridge->BriegeListNode;
		WBridge->BriegeListNode = ListStart;
		//WBridge的全部对象释放后，PageMemory将WBridge回收，并做PageMemory合并

		if (--WBridge->BridgeUseCount == 0)
		{
			Mbridgelist.EraseNode(WBridge);
			PageMemoryOperate::GetPageInstence()->ReleaseBridgeToPageMemory(WBridge);
		}



		ListStart = NextNode;
	}


}