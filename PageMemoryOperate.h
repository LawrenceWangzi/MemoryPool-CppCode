#pragma once

#include "Together.h"

//单例模式
//MediumMemory获取bridge对象都是从同一个PageMemory的数组中获取
//从而PageMemory为单例模式
class PageMemoryOperate
{
public:
	static PageMemoryOperate* GetPageInstence()
	{
		return &PageInstence;
	}

	Bridge* RequestLargePageObject(size_t PageSize);
	void ReleaseLargePageObject(void* pointer, Bridge* bridge);

	Bridge* GetNewBridge(size_t Num);
	Bridge* NewBridge(size_t Num);//以page为单位获取bridge对象

	//得到从对象指向bridge的映射关系
	Bridge* SetMapObjectToBridge(void* object);

	//PageCache回收bridge释放的空间，合并相邻的bridge
	void ReleaseBridgeToPageMemory(Bridge* bridge);

private:
	BridgeList GetBridgeList[NumPage];

	std::unordered_map<PageIndexType, Bridge*> GetIndexBridgeMap;

	std::mutex GetMutex;
private:
	PageMemoryOperate(){}

	PageMemoryOperate(const PageMemoryOperate&) = delete;
	static PageMemoryOperate PageInstence;
};