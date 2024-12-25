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

	void init()
	{
		_Hdr = _OMEM_SIG;
		_Size = 0;
		_Block = nullptr;
		_Owner = nullptr;
		_LockPerm = _omem_lock_permission_owner;
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

	DWORD dwNumofPages = (DWORD)(2/*hdr + protection*/ + ((_Size / dwPageSize) + ((_Size % dwPageSize) / (_Size % dwPageSize))));
	void* pPage = VirtualAlloc(NULL, dwNumofPages * dwPageSize, MEM_COMMIT | MEM_TOP_DOWN, PAGE_EXECUTE_READWRITE);
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