#include "PageMemoryOperate.h"

PageMemoryOperate PageMemoryOperate::PageInstence;


//����Ƚϴ�Ķ���ʱ��ֱ�Ӵ�ϵͳ����
Bridge* PageMemoryOperate::RequestLargePageObject(size_t PageSize)
{
	assert(PageSize > MaxByte);

	PageSize = CalculateSize::GetListRoundUp(PageSize, PageShift); //����
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
	if (FNumPage < NumPage) //FNumPageС��128ҳʱ���ͷ�bridge
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
	//������ԭ��ȫ�ּ������Ա�֤ĳһ��ֻ��һ���̵߳�PageMemory���뵽bridge
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
	//��ʱ����BridgeList���޺���bridge,�Ӷ�ֱ����ϵͳ����128ҳ�ڴ�
 
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

//�õ��Ӷ���ָ��bridge��ӳ���ϵ
Bridge* PageMemoryOperate::SetMapObjectToBridge(void* object)
{
	//ͳ��ҳ��
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
	std::unique_lock<std::mutex> RLock(GetMutex);//ȫ������֤һ��ֻ��һ���̴߳�ThreadMemory�黹����



	//Bridge�ͷ��ڴ�ҳ������128�����ϲ���ֱ����ϵͳ�ͷ��ڴ�
	if (CurBridge->BridgeNumPage >= NumPage)
	{
		void* pointer = (void*)(CurBridge->BridgePageId << PageShift);

		GetIndexBridgeMap.erase(CurBridge->BridgePageId);//ɾ��page��bridge��ӳ��
		VirtualFree(pointer, 0, MEM_RELEASE);
		delete CurBridge;
		return;
	}


	// ��ǰ�ϲ�page
	while (1)
	{


		PageIndexType PageCurIndex = CurBridge->BridgePageId;
		PageIndexType PagePreIndex = PageCurIndex - 1;
		auto PageIt = GetIndexBridgeMap.find(PagePreIndex);

		// �޷��ҵ�ӳ���ϵ���˳�
		if (PageIt == GetIndexBridgeMap.end())
			break;

		// ǰһ��bridge������BridgeUseCount
		if (PageIt->second->BridgeUseCount != 0)//BridgeUseCount
			break;

		Bridge* PrevBridge = PageIt->second;

		//����128ҳ���Ͳ����кϲ�
		if (CurBridge->BridgeNumPage + PrevBridge->BridgeNumPage > NumPage - 1)
			break;

		//ɾ�������PrevBridge
		GetBridgeList[PrevBridge->BridgeNumPage].EraseNode(PrevBridge);

		// �ϲ�����
		PrevBridge->BridgeNumPage += CurBridge->BridgeNumPage;
		//����BridgePageIdָ��bridge��ӳ���ϵ
		for (PageIndexType i = 0; i < CurBridge->BridgeNumPage; ++i)
		{
			GetIndexBridgeMap[CurBridge->BridgePageId + i] = PrevBridge;
		}
		delete CurBridge;

		// ������ǰ�ϲ�����
		CurBridge = PrevBridge;
	}


	//���ϲ�
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

		///////����128ҳ���Ͳ����кϲ�
		if (CurBridge->BridgeNumPage + NextBridge->BridgeNumPage >= NumPage - 1)
			break;

		GetBridgeList[NextBridge->BridgeNumPage].EraseNode(NextBridge);

		CurBridge->BridgeNumPage += NextBridge->BridgeNumPage;
		////����BridgePageIdָ��bridge��ӳ���ϵ
		for (PageIndexType i = 0; i < NextBridge->BridgeNumPage; ++i)
		{
			GetIndexBridgeMap[NextBridge->BridgePageId + i] = CurBridge;
		}

		delete NextBridge;
	}

	// ��󽫺ϲ��õ�Bridge���뵽BridgeList����ǰ��λ��
	GetBridgeList[CurBridge->BridgeNumPage].InsertNodeFront(CurBridge);
}
