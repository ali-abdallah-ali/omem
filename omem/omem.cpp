/*
 OMEM © 2024 by ALI ABDALLAH is licensed under CC BY-NC-SA 4.0.
 To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
*/

#include "pch.h"
#include "framework.h"
#include "omem.h"

struct _omem
{
	int _Hdr;
	size_t _Size;
	void* _Block;
	void* _Owner;
	_omem_lock_permission _LockPerm;
	bool _Performnace;
	ULONG_PTR _PerfPageCount;
	ULONG_PTR* _PerfPageArrayPtr;

	void init()
	{
		_Hdr = _OMEM_SIG;
		_Size = 0;
		_Block = nullptr;
		_Owner = nullptr;
		_LockPerm = _omem_lock_permission_owner;
		_Performnace = false;
		_PerfPageCount = 0;
		_PerfPageArrayPtr = nullptr;
	}
};

_omem_scopelock::_omem_scopelock()
{
	omem = nullptr;
}

_omem_scopelock::~_omem_scopelock()
{
	if (omem != nullptr)
	{
		omem_lock(omem->_Block, omem->_Owner);
	}
	omem = nullptr;
}

_omem* omem_gethdr(void* _Block)
{
	if (_Block != nullptr)
	{
		MEMORY_BASIC_INFORMATION meminfo = { 0 };
		if (VirtualQuery(_Block, &meminfo, sizeof(meminfo)) == sizeof(meminfo))
		{
			_omem* omem = (_omem*)meminfo.AllocationBase;
			if (omem->_Hdr == _OMEM_SIG)
			{
				return omem;
			}
		}
	}

	return nullptr;
}

void* operator new(size_t _Size)
{
	return omem_alloc(_Size);
}
void* operator new[](size_t _Size)
{
	return omem_alloc(_Size);
}
void operator delete(void* _Block)
{
	omem_free(_Block);
}
void operator delete[](void* _Block)
{
	omem_free(_Block);
}

void* omem_alloc(size_t _Size, void* _Owner/*= nullptr*/)
{
	return omem_alloc_base(_Size, _Owner, false);
}

bool enable_setlockpages()
{
	HANDLE Token = NULL;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &Token))
	{
		LUID Luid = { 0 };
		if (LookupPrivilegeValue(NULL, SE_LOCK_MEMORY_NAME, &Luid))
		{
			TOKEN_PRIVILEGES priv = { 0 };
			priv.PrivilegeCount = 1;
			priv.Privileges->Attributes = SE_PRIVILEGE_ENABLED;
			priv.Privileges->Luid = Luid;
			if (AdjustTokenPrivileges(Token, FALSE, &priv, 0, NULL, NULL))
			{
				CloseHandle(Token);
				return true;
			}
		}
	}

	return false;
}

void* omem_performance_alloc(size_t _Size, void* _Owner/*= nullptr*/)
{
	if (enable_setlockpages())
	{
		return omem_alloc_base(_Size, _Owner, true);
	}

	return nullptr;
}

ULONG_PTR* alloc_physicalmem(DWORD dwNumofPages, void* pPage)
{
	ULONG_PTR* pPageArr = (ULONG_PTR*)HeapAlloc(GetProcessHeap(), 0, dwNumofPages * sizeof(ULONG_PTR));
	if (pPageArr != nullptr)
	{
		ULONG_PTR ulNumofPages = dwNumofPages;
		if (AllocateUserPhysicalPages(GetCurrentProcess(), &ulNumofPages, pPageArr))
		{
			if (ulNumofPages == dwNumofPages)
			{
				if (MapUserPhysicalPages(pPage, ulNumofPages, pPageArr))
				{
					return pPageArr;
				}
				else
				{
					FreeUserPhysicalPages(GetCurrentProcess(), &ulNumofPages, pPageArr);
				}
			}
			else
			{
				FreeUserPhysicalPages(GetCurrentProcess(), &ulNumofPages, pPageArr);
			}
		}
		else
		{
			HeapFree(GetProcessHeap(), 0, pPageArr);
		}
	}

	return nullptr;
}

void* omem_alloc_base(size_t _Size, void* _Owner, bool _performance)
{
	void* pData = nullptr;
	SYSTEM_INFO sysinfo = { 0 };
	GetSystemInfo(&sysinfo);
	DWORD dwPageSize = sysinfo.dwPageSize;

	//	_____________
	//	|	hdr		|
	//	|___________|
	//	|	data	|
	//	|___________|
	//	|protection	|
	//	|area		|
	//	|___________|

	DWORD dwFlags = MEM_COMMIT;
	if (_performance)
	{
		dwFlags = MEM_RESERVE | MEM_PHYSICAL;
	}

	DWORD dwNumofPages = (DWORD)(2/*hdr + protection*/ + ((_Size / dwPageSize) + ((_Size % dwPageSize) ? 1 : 0)));
	void* pPage = VirtualAlloc(NULL, dwNumofPages * dwPageSize, dwFlags, PAGE_READWRITE);

	ULONG_PTR* pPageArr = nullptr;
	if (_performance)
	{
		if (pPage != nullptr)
		{
			pPageArr = alloc_physicalmem(dwNumofPages, pPage);
			if (pPageArr == nullptr)
			{
				VirtualFree(pPage, 0, MEM_RELEASE);
				pPage = nullptr;
			}
		}
	}

	if (pPage != nullptr)
	{
		MEMORY_BASIC_INFORMATION meminfo = { 0 };
		if (VirtualQuery(pPage, &meminfo, sizeof(meminfo)) == sizeof(meminfo))
		{
			DWORD dwProtect = 0;
			void* pPageEnd = (void*)((DWORD_PTR)pPage + meminfo.RegionSize);
			if (VirtualProtect((void*)((DWORD_PTR)pPageEnd - dwPageSize), dwPageSize, PAGE_NOACCESS, &dwProtect))
			{
				pData = (void*)((DWORD_PTR)pPageEnd - dwPageSize - _Size);
				_omem* omem = (_omem*)meminfo.AllocationBase;
				omem->init();

				omem->_Block = pData;
				omem->_Owner = _Owner;
				omem->_Size = _Size;

				if (_performance)
				{
					omem->_Performnace = true;
					omem->_PerfPageCount = dwNumofPages;
					omem->_PerfPageArrayPtr = pPageArr;
				}
			}
		}
	}

	return pData;
}

bool omem_free(void* _Block, void* _Owner/*= nullptr*/)
{
	_omem* omem = omem_gethdr(_Block);
	if (omem != nullptr)
	{
		if (omem->_Owner == _Owner)
		{
			if (omem->_Performnace)
			{
				ULONG_PTR perfpagecount = omem->_PerfPageCount;
				ULONG_PTR* perfpagearrayptr = omem->_PerfPageArrayPtr;
				MapUserPhysicalPages(omem, perfpagecount, NULL);
				FreeUserPhysicalPages(GetCurrentProcess(), &perfpagecount,perfpagearrayptr);
				HeapFree(GetProcessHeap(), 0, perfpagearrayptr);
			}

			VirtualFree(omem, 0, MEM_RELEASE);
			return true;
		}
	}

	return false;
}

bool omem_lock(void* _Block, void* _Owner/*= nullptr*/)
{
	_omem* omem = omem_gethdr(_Block);
	if (omem != nullptr)
	{
		bool allowed = false;
		if (omem->_LockPerm == _omem_lock_permission_owner && omem->_Owner == _Owner)
		{
			allowed = true;
		}
		else if (omem->_LockPerm == _omem_lock_permission_any)
		{
			allowed = true;
		}

		if (allowed)
		{
			DWORD dwProtect = 0;
			if (VirtualProtect(omem->_Block, omem->_Size, PAGE_READONLY, &dwProtect))
			{
				return true;
			}
		}
	}

	return false;
}

bool omem_unlock(void* _Block, void* _Owner /*= nullptr)*/)
{
	_omem* omem = omem_gethdr(_Block);
	if (omem != nullptr)
	{
		bool allowed = false;
		if (omem->_LockPerm == _omem_lock_permission_owner && omem->_Owner == _Owner)
		{
			allowed = true;
		}
		else if (omem->_LockPerm == _omem_lock_permission_any)
		{
			allowed = true;
		}

		if (allowed)
		{
			DWORD dwProtect = 0;
			if (VirtualProtect(omem->_Block, omem->_Size, PAGE_READWRITE, &dwProtect))
			{
				return true;
			}
		}
	}

	return false;
}


bool omem_unlock(void* _Block, _omem_scopelock& scopelock, void* _Owner /*= nullptr)*/)
{
	_omem* omem = omem_gethdr(_Block);
	if (omem != nullptr)
	{
		if (omem_unlock(_Block, _Owner))
		{
			scopelock.omem = omem;

			return true;
		}
	}

	return false;
}

bool omem_setowner(void* _Block, void* _Owner)
{
	_omem* omem = omem_gethdr(_Block);
	if (omem != nullptr)
	{
		omem->_Owner = _Owner;
		return true;
	}

	return false;
}

bool omem_setlockpermission(void* _Block, void* _Owner, _omem_lock_permission _Mode)
{
	_omem* omem = omem_gethdr(_Block);
	if (omem != nullptr)
	{
		if (omem->_Owner == _Owner)
		{
			omem->_LockPerm = _Mode;
			return true;
		}
	}

	return false;
}