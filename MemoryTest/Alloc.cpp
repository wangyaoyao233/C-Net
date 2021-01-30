#include "Alloc.h"
#include "MemoryMgr.hpp"

void* operator new(size_t size)
{
	return	MemoryMgr::Instance().AllocMem(size);
}

void* operator new[](size_t size)
{
	return	MemoryMgr::Instance().AllocMem(size);
}

void operator delete(void* p)
{
	MemoryMgr::Instance().FreeMem(p);
}

void operator delete[](void* p)
{
	MemoryMgr::Instance().FreeMem(p);
}

void* mem_alloc(size_t size)
{
	return malloc(size);
}
void mem_free(void* p)
{
	free(p);
}
