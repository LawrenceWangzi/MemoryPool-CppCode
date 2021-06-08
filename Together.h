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

const size_t MaxByte = 64 * 1024; //ThreadMemoryOperate 可以申请的内存上限
const size_t NumList = 184; //根据对齐规则推算出来得数组元素个数	
const size_t PageShift = 12;
const size_t NumPage = 129;


inline static void*& NextObject(void* next)//抢取对象头八个或四个字节
{
	return *((void**)next);   // // 对next变量强制类型转换成void**类型，结果解引用转成void *类型
}

//设计一个ChangeList对象链表类，
//利用这个链表类，生成需要的链表对象，并用各个函数接口进行操作			
//让一个链表类来处理链表对象
class ChangeList
{
private:
	void* CList = nullptr; // 给个默认值
	size_t CSize = 0;  //统计链表对象个数	
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

	void* PopObject() //弹出对象	
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
	
	bool JudgeEmpty()//清空对象
	{
		return CList == nullptr;
	}

	size_t sSize()//返回对象个数
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

//统计ChangeList对象大小位置的类
class CalculateSize
{
public:
	//得到ChangeList对象的位置
	inline static size_t GetListIndex(size_t ListSize, size_t adjust)
	{
		size_t AdjustNum = 1 << adjust;  //库里实现的方法
		return ((ListSize + AdjustNum - 1) >> adjust) - 1;
	}

	inline static size_t GetListRoundUp(size_t ListSize, size_t adjust)
	{
		size_t AdjustNum = 1 << adjust;
		return (ListSize + AdjustNum - 1)&~(AdjustNum - 1);
	}

public:
	// 控制在12%左右的内碎片浪费
	// [1,128]				8byte对齐 ChangeList[0,16)
	// [129,1024]			16byte对齐 ChangeList[16,72)
	// [1025,8*1024]		128byte对齐 ChangeList[72,128)
	// [8*1024+1,64*1024]	1024byte对齐 ChangeList[128,184)

	inline static size_t ListIndex(size_t ListSize)
	{
		assert(ListSize <= MaxByte);

		// 每区间包含的链对象个数
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
		else//小于等于 65536
		{
			return GetListIndex(ListSize - 8 * 1024, 10) + ListGroupNumArray[0] + ListGroupNumArray[1] + ListGroupNumArray[2];
		}
	}

	// 根据对齐规则，向上取整个数
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

	//计算从MediumMemoryOperate分配内存对象到ThreadMemoryOperate中的内存对象个数  
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
	//从PageSize统计Medium缓存到Page缓存获取的Bridge对象的大小

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
//以下为了兼容WIN32类型的系统或者其他系统
#ifdef _WIN32
	typedef size_t PageIndexType;
#else
	typedef long long PageIndexType;
#endif //_WIN32


	//Bridge定义为结构体可以避免编写友元函数，其是一个桥梁作用，可以回收内存到Page,也可分配内存
struct Bridge
{
	PageIndexType BridgePageId = 0;//页码
	size_t BridgeNumPage = 0;//页数量

	Bridge* ListPrev = nullptr; 
	Bridge* ListNext = nullptr;

	void* BriegeListNode = nullptr;//用以链接对象的可变链表，无有对象即是空
	size_t BridgeObjectSize = 0;//Bridge对象大小

	size_t BridgeUseCount = 0;//对象Bridge计数,
};

 
//基本功能类似于 ChangeList，Bridge链表可以双向读取，
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

	~BridgeList()//释放链表内存	
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
	//为了防止浅拷贝，利用delete 封死赋值构造函数、拷贝构造函数；

	BridgeList(const BridgeList&) = delete;
	BridgeList& operator=(const BridgeList&) = delete;

	//左闭右开
	Bridge* GetListBegin()//返回的一个数据的指针
	{
		return BridgeHead->ListNext;
	}

	Bridge* GetListEnd()//最后一个的下一个指针
	{
		return BridgeHead;
	}

	bool JudgeEmpty()
	{
		return BridgeHead->ListNext == BridgeHead;
	}


	//将NewBridge插入到 CurNode前面
	void InsertNode(Bridge* CurNode, Bridge* NewBridge)
	{
		Bridge* BridgePrev = CurNode->ListPrev;


		BridgePrev->ListNext = NewBridge;
		NewBridge->ListNext = CurNode;

		NewBridge->ListPrev = BridgePrev;
		CurNode->ListPrev = NewBridge;
	}

	
	void EraseNode(Bridge* CurNode)////删除指定结点 但是没有释放内存  
	{
		Bridge* BridgePrev = CurNode->ListPrev;
		Bridge* BridgeNext = CurNode->ListNext;

		BridgePrev->ListNext = BridgeNext;
		BridgeNext->ListPrev = BridgePrev;
	}

	//尾部插入
	void InsertNodeEnd(Bridge* NewBridge)
	{
		InsertNode(GetListEnd(), NewBridge);
	}

	//头部插入
	void InsertNodeFront(Bridge* NewBridge)
	{
		InsertNode(GetListBegin(), NewBridge);
	}

	//尾部删除
	Bridge* PopNodeEnd()//实际是将尾部位置的节点拿出来
	{
		Bridge* EndNode = BridgeHead->ListPrev;
		EraseNode(EndNode);

		return EndNode;
	}

	//头部删除，从头部位置取出结点
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