#pragma once

#include "Together.h"


//���ȴ�ThreadMemory��ȡ���ٿ��Ǵ�MediumMemory��ȡ

//��Ƴɵ���ģʽ ��֤ȫ��ֻ��һ��ʵ��
class MediumMemoryOperate
{
public:
	static MediumMemoryOperate* GetMediumInstence()
	{
		return &MediumInstence;
	}

	//PageMemoryOperate��Bridge ��ȡһ������
	Bridge* GetOneBridge(BridgeList& bridgelist, size_t BridgeByteSize);

	//ThreadMemory��MediumMemoryOperate��ȡNodeNum������
	size_t GetOrderByRangeObject(void*& ListStart, void*& ListEnd, size_t NodeNum, size_t BridgeByteSize);


	//�ͷ�һЩ�����BridgeList
	void ReleaseListMemoryToBridge(void* ListStart, size_t ListSize);

private:
	BridgeList bridgelist[NumList];

private:
	MediumMemoryOperate(){}//����ֻ˵����ʵ�֣���ֹ����Ĭ�Ϲ��캯��  

	MediumMemoryOperate(MediumMemoryOperate&) = delete;
	static MediumMemoryOperate MediumInstence;
};