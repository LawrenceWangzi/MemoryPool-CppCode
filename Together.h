#pragma once

#include <iostream>
#include <thread>
#include <mutex>
#include <map>
#include <unordered_map>
#include <vector>
#include <stdlib.h>
#include <algorithm>
#include <assert.h>

using std::cout;
using std::endl;

#include <Windows.h>

const size_t MaxByte = 64 * 1024; //ThreadMemoryOperate ����������ڴ�����
const size_t NumList = 184; //���ݶ�������������������Ԫ�ظ���	
const size_t PageShift = 12;
const size_t NumPage = 129;


inline static void*& NextObject(void* next)//��ȡ����ͷ�˸����ĸ��ֽ�
{
	return *((void**)next);   // // ��next����ǿ������ת����void**���ͣ����������ת��void *����
}

//���һ��ChangeList���������࣬
//������������࣬������Ҫ��������󣬲��ø��������ӿڽ��в���			
//��һ���������������������
class ChangeList
{
private:
	void* CList = nullptr; // ����Ĭ��ֵ
	size_t CSize = 0;  //ͳ������������	
	size_t CMaxSize = 1;

public:

	void PushObject(void* objcet)
	{
		NextObject(objcet) = CList;
		CList = objcet;
		++CSize;
	}

	void PushOrderByRange(void* Start, void* End, size_t num)
	{
		NextObject(End) = CList;
		CList = Start;
		CSize += num;
	}

	void* PopObject() //��������	
	{
		void* object = CList;
		CList = NextObject(object);
		--CSize;

		return object;
	}

	void* PopOrderByRange()
	{
		CSize = 0;
		void* OneListNode = CList;
		CList = nullptr;

		return OneListNode;
	}
	
	bool JudgeEmpty()//��ն���
	{
		return CList == nullptr;
	}

	size_t sSize()//���ض������
	{
		return CSize;
	}

	size_t sMaxSize()
	{
		return CMaxSize;
	}

	void SetListMaxSize(size_t ListMaxSize)
	{
		CMaxSize = ListMaxSize;
	}
};

//ͳ��ChangeList�����Сλ�õ���
class CalculateSize
{
public:
	//�õ�ChangeList�����λ��
	inline static size_t GetListIndex(size_t ListSize, size_t adjust)
	{
		size_t AdjustNum = 1 << adjust;  //����ʵ�ֵķ���
		return ((ListSize + AdjustNum - 1) >> adjust) - 1;
	}

	inline static size_t GetListRoundUp(size_t ListSize, size_t adjust)
	{
		size_t AdjustNum = 1 << adjust;
		return (ListSize + AdjustNum - 1)&~(AdjustNum - 1);
	}

public:
	// ������12%���ҵ�����Ƭ�˷�
	// [1,128]				8byte���� ChangeList[0,16)
	// [129,1024]			16byte���� ChangeList[16,72)
	// [1025,8*1024]		128byte���� ChangeList[72,128)
	// [8*1024+1,64*1024]	1024byte���� ChangeList[128,184)

	inline static size_t ListIndex(size_t ListSize)
	{
		assert(ListSize <= MaxByte);

		// ÿ������������������
		static int ListGroupNumArray[4] = { 16, 56, 56, 56 };
		if (ListSize <= 128)
		{
			return GetListIndex(ListSize, 3);
		}
		else if (ListSize <= 1024)
		{
			return GetListIndex(ListSize - 128, 4) + ListGroupNumArray[0];
		}
		else if (ListSize <= 8192)
		{
			return GetListIndex(ListSize - 1024, 7) + ListGroupNumArray[0] + ListGroupNumArray[1];
		}
		else//С�ڵ��� 65536
		{
			return GetListIndex(ListSize - 8 * 1024, 10) + ListGroupNumArray[0] + ListGroupNumArray[1] + ListGroupNumArray[2];
		}
	}

	// ���ݶ����������ȡ������
	static inline size_t ListRoundUp(size_t ListSize)
	{
		assert(ListSize <= MaxByte);

		if (ListSize <= 128){
			return GetListRoundUp(ListSize, 3);
		}
		else if (ListSize <= 1024){
			return GetListRoundUp(ListSize, 4);
		}
		else if (ListSize <= 8192){
			return GetListRoundUp(ListSize, 7);
		}
		else {//<= 65536
			return GetListRoundUp(ListSize, 10);
		}
	}

	//�����MediumMemoryOperate�����ڴ����ThreadMemoryOperate�е��ڴ�������  
	static size_t GetSizeNum(size_t SizeNum)
	{
		if (SizeNum == 0)
			return 0;

		int GetNum = (int)(MaxByte / SizeNum);
		if (GetNum < 2)
			GetNum = 2;

		if (GetNum > 512)
			GetNum = 512;

		return GetNum;
	}
	//��PageSizeͳ��Medium���浽Page�����ȡ��Bridge����Ĵ�С

	static size_t GetPageNum(size_t PageSize)
	{
		size_t PageNum = GetSizeNum(PageSize);
		size_t NumPages = PageNum*PageSize;
		NumPages >>= PageShift;
		if (NumPages == 0)
			NumPages = 1;
		return NumPages;
	}
};
//����Ϊ�˼���WIN32���͵�ϵͳ��������ϵͳ
#ifdef _WIN32
	typedef size_t PageIndexType;
#else
	typedef long long PageIndexType;
#endif //_WIN32


	//Bridge����Ϊ�ṹ����Ա����д��Ԫ����������һ���������ã����Ի����ڴ浽Page,Ҳ�ɷ����ڴ�
struct Bridge
{
	PageIndexType BridgePageId = 0;//ҳ��
	size_t BridgeNumPage = 0;//ҳ����

	Bridge* ListPrev = nullptr; 
	Bridge* ListNext = nullptr;

	void* BriegeListNode = nullptr;//�������Ӷ���Ŀɱ��������ж����ǿ�
	size_t BridgeObjectSize = 0;//Bridge�����С

	size_t BridgeUseCount = 0;//����Bridge����,
};

 
//�������������� ChangeList��Bridge�������˫���ȡ��
class BridgeList
{
public:
	Bridge* BridgeHead;
	std::mutex BridgeListMutex;

public:
	BridgeList()
	{
		BridgeHead = new Bridge;
		BridgeHead->ListNext = BridgeHead;
		BridgeHead->ListPrev = BridgeHead;
	}

	~BridgeList()//�ͷ������ڴ�	
	{
		Bridge * CurNode = BridgeHead->ListNext;
		while (CurNode != BridgeHead)
		{
			Bridge* NextNode = CurNode->ListNext;
			delete CurNode;
			CurNode = NextNode;
		}
		delete BridgeHead;
		BridgeHead = nullptr;
	}
	//Ϊ�˷�ֹǳ����������delete ������ֵ���캯�����������캯����

	BridgeList(const BridgeList&) = delete;
	BridgeList& operator=(const BridgeList&) = delete;

	//����ҿ�
	Bridge* GetListBegin()//���ص�һ�����ݵ�ָ��
	{
		return BridgeHead->ListNext;
	}

	Bridge* GetListEnd()//���һ������һ��ָ��
	{
		return BridgeHead;
	}

	bool JudgeEmpty()
	{
		return BridgeHead->ListNext == BridgeHead;
	}


	//��NewBridge���뵽 CurNodeǰ��
	void InsertNode(Bridge* CurNode, Bridge* NewBridge)
	{
		Bridge* BridgePrev = CurNode->ListPrev;


		BridgePrev->ListNext = NewBridge;
		NewBridge->ListNext = CurNode;

		NewBridge->ListPrev = BridgePrev;
		CurNode->ListPrev = NewBridge;
	}

	
	void EraseNode(Bridge* CurNode)////ɾ��ָ����� ����û���ͷ��ڴ�  
	{
		Bridge* BridgePrev = CurNode->ListPrev;
		Bridge* BridgeNext = CurNode->ListNext;

		BridgePrev->ListNext = BridgeNext;
		BridgeNext->ListPrev = BridgePrev;
	}

	//β������
	void InsertNodeEnd(Bridge* NewBridge)
	{
		InsertNode(GetListEnd(), NewBridge);
	}

	//ͷ������
	void InsertNodeFront(Bridge* NewBridge)
	{
		InsertNode(GetListBegin(), NewBridge);
	}

	//β��ɾ��
	Bridge* PopNodeEnd()//ʵ���ǽ�β��λ�õĽڵ��ó���
	{
		Bridge* EndNode = BridgeHead->ListPrev;
		EraseNode(EndNode);

		return EndNode;
	}

	//ͷ��ɾ������ͷ��λ��ȡ�����
	Bridge* PopNodeFront()//
	{
		Bridge* FrontNode = BridgeHead->ListNext;
		EraseNode(FrontNode);

		return FrontNode;
	}

	void Lock()
	{
		BridgeListMutex.lock();
	}

	void Unlock()
	{
		BridgeListMutex.unlock();
	}
};