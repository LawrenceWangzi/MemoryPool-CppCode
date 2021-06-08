#pragma once

#include "Together.h"

//����ģʽ
//MediumMemory��ȡbridge�����Ǵ�ͬһ��PageMemory�������л�ȡ
//�Ӷ�PageMemoryΪ����ģʽ
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
	Bridge* NewBridge(size_t Num);//��pageΪ��λ��ȡbridge����

	//�õ��Ӷ���ָ��bridge��ӳ���ϵ
	Bridge* SetMapObjectToBridge(void* object);

	//PageCache����bridge�ͷŵĿռ䣬�ϲ����ڵ�bridge
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