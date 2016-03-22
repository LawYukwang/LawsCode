#ifndef _MEM_POOL_H
#define _MEM_POOL_H

#include <vector>
#include <iostream>

using namespace std;

/*
	在内存池中分配固定大小的内存块

	该类的目的是加速内存分配速度，并且减少因重复分配相同
	内存时产生的内存碎片，比如在服务器应用程序中。
*/

class CMemPool
{
public:

	//创建大小为blockSize的内存块，内存池数目为预分配的数目preAlloc
	CMemPool(std::size_t blockSize, int preAlloc = 0, int maxAlloc = 0);

	~CMemPool();

	//获取一个内存块。如果内存池中没有足够的内存块，则会自动分配新的内存块
	//如果分配的内存块数目达到了最大值，则会返回一个异常
	void* Get();

	//释放当前内存块，将其插入内存池
	void Release(void* ptr);

	//返回内存块大小
	std::size_t BlockSize() const;

	//返回内存池中内存块数目
	int Allocated() const;

	//返回内存池中可用的内存块数目
	int Available() const;

private:
	CMemPool();
	CMemPool(const CMemPool&);
	CMemPool& operator = (const CMemPool&);

	enum
	{
		BLOCK_RESERVE = 32
	};

	typedef std::vector<char*> BlockVec;

	std::size_t m_blockSize;
	int         m_maxAlloc;
	int         m_allocated;
	BlockVec    m_blocks;
};

inline std::size_t CMemPool::BlockSize() const
{
	return m_blockSize;
}


inline int CMemPool::Allocated() const
{
	return m_allocated;
}


inline int CMemPool::Available() const
{
	return (int) m_blocks.size();
}


#endif