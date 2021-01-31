#pragma once
#include <stdlib.h> //malloc free
#include <mutex> // std::mutex
#include <assert.h> // assert

#ifdef _DEBUG
	#ifndef xPrintf
		#include <stdio.h>
		#define xPrintf(...) printf(__VA_ARGS__)
	#endif // !xPrintf
#else
	#ifndef xPrintf
		#define xPrintf(...)
	#endif // !xPrintf
#endif // _DEBUG

template<class T, size_t PoolSize>
class ObjectPool
{
private:
	class NodeHeader
	{
	public:
		NodeHeader* Next;
		int Id;
		char RefCnt;
		bool InPool;
	private:
		char pad[2];
	};
public:
	ObjectPool()
	{
		_buf = nullptr;
		Init();
	}
	~ObjectPool()
	{
		if (_buf)
			delete[] _buf;
	}


	void* AllocObjMem(size_t size)
	{
		std::lock_guard<std::mutex> lock(_mutex);

		NodeHeader* ret = nullptr;
		if (nullptr == _header) {// if no space
			ret = (NodeHeader*)new char[sizeof(T) + sizeof(NodeHeader)];

			ret->InPool = false;
			ret->Id = -1;
			ret->RefCnt = 1;
			ret->Next = nullptr;
		}
		else {
			ret = _header;
			_header = _header->Next;

			assert(0 == ret->RefCnt);
			ret->RefCnt = 1;
		}

		// xPrintf("AllocObjMem: %p, id=%d, size=%d\n", ret, ret->Id, size);

		return (char*)ret + sizeof(NodeHeader);
	}

	void FreeObjMem(void* p)
	{
		NodeHeader* node = (NodeHeader*)((char*)p - sizeof(NodeHeader));

		assert(1 == node->RefCnt);

		if (node->InPool) {
			std::lock_guard<std::mutex> lock(_mutex);

			if (--node->RefCnt != 0) {
				return;
			}
			node->Next = _header;
			_header = node;
		}
		else {
			if (--node->RefCnt != 0) {
				return;
			}
			delete[] node;
		}
	}
private:
	void Init()
	{
		assert(nullptr == _buf);
		if (_buf)	return;

		size_t oneTotalSize = sizeof(T) + sizeof(NodeHeader);
		size_t totalPoolSize = PoolSize * oneTotalSize;
		_buf = new char[totalPoolSize];

		// 初始化对象池
		_header = (NodeHeader*)_buf;
		_header->InPool = true;
		_header->Id = 0;
		_header->RefCnt = 0;
		_header->Next = nullptr;

		// 遍历对象池, 初始化内存块
		NodeHeader* tempPre = _header;
		for (int i = 1; i < PoolSize; i++) {
			NodeHeader* temp = (NodeHeader*)(_buf + i * oneTotalSize);
			temp->InPool = true;
			temp->Id = i;
			temp->RefCnt = 0;
			temp->Next = nullptr;

			tempPre->Next = temp;
			tempPre = temp;
		}
	}

	NodeHeader* _header;
	char* _buf;
	std::mutex _mutex;
};



template<class T, size_t PoolSize>
class IObjectPool
{
public:
	void* operator new(size_t size)
	{
		return	ObjectPool().AllocObjMem(size);
	}

	void operator delete(void* p)
	{
		ObjectPool().FreeObjMem(p);
	}

	template<typename ...Args>
	static T* CreateObject(Args ...args)
	{
		T* obj = new T(args...);
		return obj;
	}

	static void DestoryObject(T* obj)
	{
		delete obj;
	}

private:
	typedef ObjectPool<T, PoolSize> ClassTPool;
	static ClassTPool& ObjectPool()
	{
		// static Object pool instance
		static ClassTPool pool;
		return pool;
	}
};
