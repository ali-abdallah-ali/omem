
#include <iostream>
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <chrono>

//include and lib
#include "../omem/omem.h"
#ifdef _DEBUG
#pragma comment( lib,"../x64/Debug/omem.lib")
#else
#pragma comment( lib,"../x64/Release/omem.lib")
#endif

// override new op for compatibility 
#define _OMEM_OP_OVERRIDE


void test_outofboundread();
void test_outofboundwrite();
void test_normalread();
void test_lock_unlock();
void test_performance(int mbsize);


int main()
{
	std::cout << "omem lib usage tests !\n";

	test_normalread();
	//UNCOMMENT: EXCEPTION CALL
	//test_outofboundread();
	//UNCOMMENT: EXCEPTION CALL
	//test_outofboundwrite();

	//////////////

	test_lock_unlock();

	//////////////
	std::cout << "performance test !!\n";
	std::cout << "Please make sure 'Lock pages in memory' policy is enabled for current user\n";
	std::cout << "https://learn.microsoft.com/en-us/previous-versions/windows/it-pro/windows-10/security/threat-protection/security-policy-settings/lock-pages-in-memory\n\n";
	std::cout << "enter size in MB !\n (1~1024)";
	int mbsize = 100;
	std::cin >> mbsize;
	test_performance(mbsize);

	return 0;
}


void test_normalread()
{
	// no owner boundry check
	std::cout << "omem : test read\n";
	int* val = new int[5] {0};
	for (int i = 0; i < 5; i++)
	{
		int n = val[i];
	}
}

void test_outofboundread()
{
	// no owner boundry check
	std::cout << "omem : test out of bound read\n";
	int* val = new int[5] {0};
	for (int i = 0; i < 6; i++)
	{
		int n = val[i];
	}
}

void test_outofboundwrite()
{
	// no owner boundry check
	std::cout << "omem : test out of bound write\n";
	int* val = new int[5];
	for (int i = 0; i < 6; i++)
	{
		val[i] = 1;
	}
}

/////////////////////////////////////
//ownership tests
/////////////////////////////////////

class a {
public:
	a() { m_pData = nullptr; }
	~a() { delete[] m_pData; }

	void alloc()
	{
		m_pData = new int[100];
		omem_setowner(m_pData, this);
	}

	void unlockscope(_omem_scopelock& omemlock)
	{
		omem_unlock(m_pData, omemlock, this);
	}

	void unlock()
	{
		omem_unlock(m_pData, this);
	}

	void lock()
	{
		omem_lock(m_pData, this);
	}

	int* m_pData;
};

class b {
public:
	b() {}
	~b() {}

	void tryread(int* pData)
	{
		int val = pData[50];
	}

	void trywrite(int* pData)
	{
		pData[50] = 1;
	}
};

void test_scopeunlock(a& owner, b& other)
{
	_omem_scopelock omemlock = _omem_scopelock();
	owner.unlockscope(omemlock);
	other.trywrite(owner.m_pData);
}

void test_lock_unlock()
{
	a owner;
	b other;

	owner.alloc();
	owner.lock();
	other.tryread(owner.m_pData);
	//UNCOMMENT: EXCEPTION CALL
	//other.trywrite(owner.m_pData); 

	owner.unlock();
	other.trywrite(owner.m_pData);
	owner.lock();

	//scope limited unlock
	test_scopeunlock(owner, other);
	//UNCOMMENT: EXCEPTION CALL
	//other.trywrite(owner.m_pData);
}

/////////////////////////////////////
//performance tests
/////////////////////////////////////

void test_performance(int mbsize)
{
	mbsize = max(1, min(mbsize, 1024));
	const int sample_size = mbsize * 1024 * 1024;

	std::cout << "Performance : Testing" << std::endl;

	char* pexDataWrite = (char*)omem_performance_alloc(sample_size);
	char* pexDataRead = (char*)omem_performance_alloc(sample_size);
	char* pDataWrite = (char*)new char[sample_size];
	char* pDataRead = (char*)new char[sample_size];

	if (pexDataWrite == nullptr || pexDataRead == nullptr || pDataWrite == nullptr || pDataRead == nullptr)
	{
		std::cout << "Can't allocate memory\n";
		system("pause");
		return;
	}

	auto tmstart = std::chrono::high_resolution_clock::now();
	memcpy(pexDataWrite, pexDataRead, sample_size);
	auto tmstop = std::chrono::high_resolution_clock::now();
	LONGLONG perftime = std::chrono::duration_cast<std::chrono::microseconds>(tmstop - tmstart).count();

	tmstart = std::chrono::high_resolution_clock::now();
	memcpy(pDataWrite, pDataRead, sample_size);
	tmstop = std::chrono::high_resolution_clock::now();
	LONGLONG normtime = std::chrono::duration_cast<std::chrono::microseconds>(tmstop - tmstart).count();

	delete[] pexDataWrite;
	delete[] pexDataRead;
	delete[] pDataWrite;
	delete[] pDataRead;

	std::cout << "performance >> " << perftime << " : normal >> " << normtime << " microseconds" << std::endl;
	system("pause");
}