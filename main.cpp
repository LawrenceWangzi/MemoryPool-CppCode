#include "Together.h"
#include "ConcurrentAllocateMemory.h"

#define SIZE 16

void MainMemoryMalloc(size_t MallocNumTimes, size_t MallocNumWorks, size_t MallocRounds)
{
	std::vector<std::thread> VarThread(MallocNumWorks);
	size_t MallocMemoryCostTime = 0;
	size_t FreeMemoryCostTime = 0;
	for (size_t i = 0; i < MallocNumWorks; ++i)
	{
		VarThread[i] = std::thread([&, i]() {
			std::vector<void*> vec;
			vec.reserve(MallocNumTimes);
			for (size_t j = 0; j < MallocRounds; ++j)
			{
				size_t BeginOne = clock();
				for (size_t k = 0; k < MallocNumTimes; k++)
				{
					vec.push_back(malloc(SIZE));
				}
				size_t EndOne = clock();
				size_t BeginTwo = clock();
				for (size_t k = 0; k < MallocNumTimes; k++)
				{
					free(vec[k]);
				}
				size_t EndTwo = clock();
				vec.clear();
				MallocMemoryCostTime += EndOne - BeginOne;
				FreeMemoryCostTime += EndTwo - BeginTwo;
			}
		});
	}
	for (auto& thread : VarThread)
	{
		thread.join();
	}

	printf("%u threads concurrent run %u rounds，every round run malloc %u times: cost time：%u ms\n", MallocNumWorks, MallocRounds, MallocNumTimes, MallocMemoryCostTime);
	printf("%u threads concurrent run %u rounds，every round run free %u times: cost time：%u ms\n", MallocNumWorks, MallocRounds, MallocNumTimes, FreeMemoryCostTime);
	printf("%u threads concurrent run malloc and free %u times，totally cost time：%u ms\n", MallocNumWorks, MallocNumWorks*MallocRounds*MallocNumTimes, MallocMemoryCostTime + FreeMemoryCostTime);
}


//每轮申请内存的 释放次数、线程个数、运行轮次
void MainConcurrentMemoryMalloc(size_t MallocNumTimes, size_t MallocNumWorks, size_t MallocRounds)
{
	std::vector<std::thread> VarThread(MallocNumWorks);
	size_t MallocMemoryCostTime = 0;
	size_t FreeMemoryCostTime = 0;
	for (size_t k = 0; k < MallocNumWorks; ++k)
	{
		VarThread[k] = std::thread([&]() {
			std::vector<void*> vec;
			vec.reserve(MallocNumTimes);
			for (size_t j = 0; j < MallocRounds; ++j)
			{
				size_t BeginOne = clock();
				for (size_t i = 0; i < MallocNumTimes; i++)
				{
					vec.push_back(ConcurrentAllocateMemory(SIZE));
				}
				size_t EndOne = clock();
				size_t BeginTwo = clock();
				for (size_t i = 0; i < MallocNumTimes; i++)
				{
					ConcurrentMemoryRelease(vec[i]);
				}
				size_t EndTwo = clock();
				vec.clear();
				MallocMemoryCostTime += EndOne - BeginOne;
				FreeMemoryCostTime += EndTwo - BeginTwo;
			}
		});
	}
	for (auto& thread : VarThread)
	{
		thread.join();
	}


	printf("%u threads concurrent run %u rounds，every round concurrent alloc %u times: cost time：%u ms\n", MallocNumWorks, MallocRounds, MallocNumTimes, MallocMemoryCostTime);
		printf("%u threads concurrent run %u rounds，every round concurrent dealloc %u times: cost time：%u ms\n",
		MallocNumWorks, MallocRounds, MallocNumTimes, FreeMemoryCostTime);
	printf("%u threads concurrent alloc&dealloc %u times，totally cost time：%u ms\n",
		MallocNumWorks, MallocNumWorks*MallocRounds*MallocNumTimes, MallocMemoryCostTime + FreeMemoryCostTime);
}


int main()
{
	cout << "############################################################" << endl;

	// MainMemoryMalloc(10, 10000, 10);
	cout << endl;
	MainConcurrentMemoryMalloc(10, 10000, 10);
	cout << endl << endl;

	cout <<"############################################################" << endl;

	system("pause");
	return 0;
}/**/