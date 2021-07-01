#pragma once

#include "Together.h"


//优先从ThreadMemory获取，再考虑从MediumMemory获取

//设计成单例模式 保证全局只有一份实例
class MediumMemoryOperate
{
public:
	static MediumMemoryOperate* GetMediumInstence()
	{
		return &MediumInstence;
	}

	//PageMemoryOperate从Bridge 获取一个对象
	Bridge* GetOneBridge(BridgeList& bridgelist, size_t BridgeByteSize);

	//ThreadMemory从MediumMemoryOperate获取NodeNum个对象
	size_t GetOrderByRangeObject(void*& ListStart, void*& ListEnd, size_t NodeNum, size_t BridgeByteSize);


	//释放一些对象给BridgeList
	void ReleaseListMemoryToBridge(void* ListStart, size_t ListSize);

private:
	BridgeList bridgelist[NumList];

private:
	MediumMemoryOperate(){}//函数只说明不实现，防止产生默认构造函数  

	MediumMemoryOperate(MediumMemoryOperate&) = delete;
	static MediumMemoryOperate MediumInstence;
};