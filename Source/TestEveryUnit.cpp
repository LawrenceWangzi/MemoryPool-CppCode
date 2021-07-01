#include "Together.h"
#include "PageMemoryOperate.h"
#include "ConcurrentAllocateMemory.h"

//µ•‘™≤‚ ‘
void TestCalculateSize()//≤‚ ‘CalculateSize
{



	cout << CalculateSize::ListRoundUp(10) << endl;
	cout << CalculateSize::ListRoundUp(1025) << endl;
	cout << CalculateSize::ListRoundUp(1024 * 8 + 1) << endl;

	cout << CalculateSize::GetPageNum(16) << endl;
	cout << CalculateSize::GetPageNum(1024) << endl;
	cout << CalculateSize::GetPageNum(1024 * 8) << endl;
	cout << CalculateSize::GetPageNum(1024 * 64) << endl;

}

void AllocateMemory(size_t Num)//…Í«Îƒ⁄¥Ê
{
	size_t BeginOne = clock();
	std::vector<void*> vec;
	for (size_t i = 0; i < Num; ++i)
	{
		vec.push_back(ConcurrentAllocateMemory(10));
	}



	for (size_t i = 0; i < Num; ++i)
	{
		ConcurrentMemoryRelease(vec[i]);
		cout << vec[i] << endl;
	}
	vec.clear();
	size_t EndOne = clock();

	size_t BeginTwo = clock();
	cout << endl << endl;
	for (size_t i = 0; i < Num; ++i)
	{
		vec.push_back(ConcurrentAllocateMemory(10));
	}

	for (size_t i = 0; i < Num; ++i)
	{
		ConcurrentMemoryRelease(vec[i]);
		cout << vec[i] << endl;
	}
	vec.clear();
	size_t EndTwo = clock();

	cout << EndOne - BeginOne << endl;
	cout << EndTwo - BeginTwo << endl;
}

void TestThreadMemoryOperate()//≤‚ ‘ThreadMemoryOperate
{
	std::thread threadOne(AllocateMemory, 1000000);


	threadOne.join();


}

void TestMediumMemoryOperate()//≤‚ ‘MediumMemoryOperate
{
	std::vector<void*> vec;
	for (size_t i = 0; i < 8; ++i)
	{
		vec.push_back(ConcurrentAllocateMemory(10));
	}

	for (size_t i = 0; i < 8; ++i)
	{

		cout << vec[i] << endl;
	}
}

void TestPageMemoryOperate()//≤‚ ‘PageMemoryOperate
{
	PageMemoryOperate::GetPageInstence()->NewBridge(2);
}

void TestConcurrentAllocateMemoryRelease()//≤‚ ‘ConcurrentAllocateMemoryRelease
{
	size_t Num = 2;
	std::vector<void*> vec;
	for (size_t i = 0; i < Num; ++i)
	{
		void* pointer = ConcurrentAllocateMemory(99999);
		vec.push_back(pointer);

	}

	for (size_t i = 0; i < Num; ++i)
	{
		ConcurrentMemoryRelease(vec[i]);
	}
	cout << "TestConcurrentAllocateMemoryRelease" << endl;
}

void AllocateLarge()//∏ﬂ≤¢∑¢ƒ⁄¥Ê¥¶¿Ì
{
	void* poinerOne = ConcurrentAllocateMemory(65 << PageShift);
	void* poinerTwo = ConcurrentAllocateMemory(129 << PageShift);

	ConcurrentMemoryRelease(poinerOne);
	ConcurrentMemoryRelease(poinerTwo);
}

/*
int main()
{
	TestCalculateSize();//	//
	TestThreadMemoryOperate();//	//
	TestMediumMemoryOperate();//	
	TestPageMemoryOperate();
	TestConcurrentAllocateMemoryRelease();
	AllocateLarge();//	//
	system("pause");
	return 0;
}*/