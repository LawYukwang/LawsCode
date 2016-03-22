#include "MemPoolpp.h"

CMemPool::CMemPool(std::size_t blockSize, int preAlloc, int maxAlloc):
m_blockSize(blockSize),
m_maxAlloc(maxAlloc),
m_allocated(preAlloc)
{
	if ( preAlloc < 0 || maxAlloc == 0 || maxAlloc < preAlloc )
	{
		cout<<"CMemPool::CMemPool parameter error."<<endl;
	}

	int r = BLOCK_RESERVE;
	if (preAlloc > r)
		r = preAlloc;
	if (maxAlloc > 0 && maxAlloc < r)
		r = maxAlloc;
	m_blocks.reserve(r);
	for (int i = 0; i < preAlloc; ++i)
	{
		m_blocks.push_back(new char[m_blockSize]);
	}
}


CMemPool::~CMemPool()
{
	for (BlockVec::iterator it = m_blocks.begin(); it != m_blocks.end(); ++it)
	{
		delete [] *it;
	}
}


void* CMemPool::Get()
{
	if (m_blocks.empty())
	{
		if (m_maxAlloc == 0 || m_allocated < m_maxAlloc)
		{
			++m_allocated;
			return new char[m_blockSize];
		}
		else
		{
			cout<<"CMemPool::get CMemPool exhausted."<<endl;
			return (void *)NULL;
		}
	}
	else
	{
		char* ptr = m_blocks.back();
		m_blocks.pop_back();
		return ptr;
	}
}


void CMemPool::Release(void* ptr)
{
	m_blocks.push_back(reinterpret_cast<char*>(ptr));
}