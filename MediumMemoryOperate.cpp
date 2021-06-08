#include "MediumMemoryOperate.h"
#include "PageMemoryOperate.h"

MediumMemoryOperate MediumMemoryOperate::MediumInstence;

Bridge* MediumMemoryOperate::GetOneBridge(BridgeList& bridgelist, size_t BridgeByteSize)
{
	Bridge* bridge = bridgelist.GetListBegin();
	while (bridge != bridgelist.GetListEnd())//bridge������bridgelist	
	{
		if (bridge->BriegeListNode != nullptr)
			return bridge;
		else
			bridge = bridge->ListNext;
	}

	 

	//��ʱ����ʾbridge�ǿյģ���PageMemory��ȡbridge����
	Bridge* NewBridge = PageMemoryOperate::GetPageInstence()->NewBridge(CalculateSize::GetPageNum(BridgeByteSize));
 
	//��bridge�жβ�����
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


//���λ�ȡ����ڴ����

size_t MediumMemoryOperate::GetOrderByRangeObject(void*& ListStart, void*& ListEnd, size_t NodeNum, size_t BridgeByteSize)
{
	size_t ListIndex = CalculateSize::ListIndex(BridgeByteSize);
	BridgeList& Mbridgelist = bridgelist[ListIndex];


	std::unique_lock<std::mutex> ListLock(Mbridgelist.BridgeListMutex);


	Bridge* Mbridge = GetOneBridge(Mbridgelist, BridgeByteSize);
	  //�˴����һ��Mbridge����

	//��Mbridge�а�˳���ȡ����
	size_t BridegBatchSize = 0;
	void* PrevNode = nullptr;//ǰһ������Ļ�ȡ
	void* CurNode = Mbridge->BriegeListNode;//��CurNode������Mbridge
	for (size_t i = 0; i < NodeNum; ++i)
	{
		PrevNode = CurNode;
		CurNode = NextObject(CurNode);
		++BridegBatchSize;
		if (CurNode == nullptr)//CurNodeΪ�գ������˳�����
			break;
	}

	ListStart = Mbridge->BriegeListNode;
	ListEnd = PrevNode;

	Mbridge->BriegeListNode = CurNode;
	Mbridge->BridgeUseCount += BridegBatchSize;

	//����Mbridge�Ƿ�Ϊ�գ��յ��ź��棬�ǿ���ǰ��
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
		//WBridge��ȫ�������ͷź�PageMemory��WBridge���գ�����PageMemory�ϲ�

		if (--WBridge->BridgeUseCount == 0)
		{
			Mbridgelist.EraseNode(WBridge);
			PageMemoryOperate::GetPageInstence()->ReleaseBridgeToPageMemory(WBridge);
		}



		ListStart = NextNode;
	}


}