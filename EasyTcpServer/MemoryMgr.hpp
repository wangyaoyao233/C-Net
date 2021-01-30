#pragma once
#include <stdlib.h> // malloc(), free()
#include <assert.h> // assert
#include <mutex>	// std::mutex

#ifdef _DEBUG
	#include <stdio.h>
	#define xPrintf(...) printf(__VA_ARGS__)
#else
	#define xPrintf(...)
#endif // _DEBUG


#define MAX_MEMORY_SIZE 128

class MemoryBlock;
class MemoryAlloc;


// memory block class
class MemoryBlock
{
public:
	int Id; // block id
	int RefCnt; // reference count
	MemoryAlloc* MeAlloc; // 所属内存块
	MemoryBlock* Next;
	bool InPool;
private:
	char Pad[3];
};

// memory pool
class MemoryAlloc
{
public:
	MemoryAlloc()
	{
		_size = 0;
		_blockNum = 0;
		_header = nullptr;
		_buf = nullptr;
	}
	~MemoryAlloc()
	{
		if (_buf) {
			free(_buf);
		}
	}
	void* AllocMem(size_t size)
	{
		std::lock_guard<std::mutex> lock(_mutex);

		if (!_buf) {
			this->Init();
		}
		MemoryBlock* ret = nullptr;	
		if (nullptr == _header) {// if no space
			ret = (MemoryBlock*)malloc(_size + sizeof(MemoryBlock));

			ret->InPool = false;
			ret->Id = -1;
			ret->RefCnt = 1;
			ret->MeAlloc = nullptr;
			ret->Next = nullptr;
		}
		else {
			ret = _header;
			_header = _header->Next;

			assert(0 == ret->RefCnt);
			ret->RefCnt = 1;
		}

		//xPrintf("AllocMem: %p, id=%d, size=%d\n", ret, ret->Id, size);

		return (char*)ret + sizeof(MemoryBlock);
	}

	void FreeMem(void* p)
	{
		MemoryBlock* block = (MemoryBlock*)((char*)p - sizeof(MemoryBlock));

		assert(1 == block->RefCnt);

		if (block->InPool) {
			std::lock_guard<std::mutex> lock(_mutex);

			if (--block->RefCnt != 0) {
				return;
			}
			block->Next = _header;
			_header = block;
		}
		else {
			if (--block->RefCnt != 0) {
				return;
			}
			free(block);
		}
	}

	void Init()
	{
		xPrintf("MemoryAlloc<%d>, block num<%d> init..\n", _size, _blockNum);

		assert(nullptr == _buf);
		if (_buf)	return;
		// 向系统申请池内存
		size_t oneTotalSize = _size + sizeof(MemoryBlock);
		size_t buffSize = oneTotalSize * _blockNum; // cal pool size
		_buf = (char*)malloc(buffSize);

		// 初始化内存池
		_header = (MemoryBlock*)_buf;
		_header->InPool = true;
		_header->Id = 0;
		_header->RefCnt = 0;
		_header->MeAlloc = this;
		_header->Next = nullptr;

		// 遍历内存池, 初始化内存块
		MemoryBlock* tempPre = _header;
		for (int i = 1; i < _blockNum; i++) {
			MemoryBlock* temp = (MemoryBlock*)(_buf + i * oneTotalSize);
			temp->InPool = true;
			temp->Id = i;
			temp->RefCnt = 0;
			temp->MeAlloc = this;
			temp->Next = nullptr;

			tempPre->Next = temp;
			tempPre = temp;
		}

	}

protected:
	size_t _size;
	size_t _blockNum;
	MemoryBlock* _header;
	char* _buf;
	std::mutex _mutex;
};

template<size_t size, size_t blockNum>
class MemoryAlloctor : public MemoryAlloc
{
public:
	MemoryAlloctor()
	{
		const size_t n = sizeof(void*);

		// memory allignment
		_size = (size / n) * n + (size % n ? n : 0);
		_blockNum = blockNum;
	}
};

// memory manager
class MemoryMgr
{
public:
	static MemoryMgr& Instance()
	{
		static MemoryMgr mgr;
		return mgr;
	}

	void* AllocMem(size_t size)
	{
		if (size <= MAX_MEMORY_SIZE) {
			return _sizeMeAlloc[size]->AllocMem(size);
		}
		else {
			MemoryBlock* ret = (MemoryBlock*)malloc(size + sizeof(MemoryBlock));

			ret->InPool = false;
			ret->Id = -1;
			ret->RefCnt = 1;
			ret->MeAlloc = nullptr;
			ret->Next = nullptr;

			//xPrintf("AllocMem: %p, id=%d, size=%d\n", ret, ret->Id, size);

			return (char*)ret + sizeof(MemoryBlock);
		}
	}

	void FreeMem(void* p)
	{
		MemoryBlock* block = (MemoryBlock*)((char*)p - sizeof(MemoryBlock));

		//xPrintf("FreeMem: %p, id=%d\n", block, block->Id);

		if (block->InPool) {
			block->MeAlloc->FreeMem(p);
		}
		else {
			if (--block->RefCnt != 0) {
				return;
			}
			free(block);
		}
		
	}

	void AddRef(void* p)
	{
		MemoryBlock* block = (MemoryBlock*)((char*)p - sizeof(MemoryBlock));
		++block->RefCnt;
	}


private:
	MemoryMgr() 
	{
		InitSize(0, 64, &_mem64);
		InitSize(65, 128, &_mem128);
		//InitSize(129, 256, &_mem256);
		//InitSize(257, 512, &_mem512);
		//InitSize(513, 1024, &_mem1024);

		xPrintf("MemoryMgr..\n");
	}
	~MemoryMgr() {}

	// 初始化内存池映射数组
	void InitSize(int begin, int end, MemoryAlloc* m)
	{
		for (int i = begin; i <= end; i++) {
			_sizeMeAlloc[i] = m;
		}
	}


	MemoryAlloc* _sizeMeAlloc[MAX_MEMORY_SIZE + 1];
private:
	MemoryAlloctor<64, 1000000> _mem64;
	MemoryAlloctor<128, 1000000> _mem128;
	//MemoryAlloctor<256, 100000> _mem256;
	//MemoryAlloctor<512, 100000> _mem512;
	MemoryAlloctor<1024, 100000> _mem1024;
};
