/*
 OMEM © 2024 by ALI ABDALLAH is licensed under CC BY-NC-SA 4.0.
 To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
*/

#pragma once

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>

#define _OMEM_SIG 'memo'

enum _omem_lock_permission
{
	_omem_lock_permission_any = 0,
	_omem_lock_permission_owner = 1,
};

struct _omem;
struct _omem_scopelock
{
	_omem* omem;
	_omem_scopelock();
	~_omem_scopelock();
};

#ifdef _OMEM_OP_OVERRIDE
void* operator new(size_t _Size);
void* operator new[](size_t _Size);
void operator delete(void* _Block);
void operator delete[](void* _Block);
#endif

void* omem_performance_alloc(size_t _Size, void* _Owner= nullptr);
void* omem_alloc(size_t _Size, void* _Owner = nullptr);
void* omem_alloc_base(size_t _Size, void* _Owner, bool _performance);
bool omem_free(void* _Block, void* _Owner = nullptr);

bool omem_lock(void* _Block, void* _Owner = nullptr);
bool omem_unlock(void* _Block, void* _Owner = nullptr);
bool omem_unlock(void* _Block, _omem_scopelock& scopelock, void* _Owner = nullptr);

bool omem_setowner(void* _Block, void* _Owner);
bool omem_setlockpermission(void* _Block, void* _Owner, _omem_lock_permission _Mode);
