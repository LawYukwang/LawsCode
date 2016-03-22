#ifndef _MEM_POOL_H
#define _MEM_POOL_H

#include <vector>
#include <iostream>

using namespace std;

/*
	���ڴ���з���̶���С���ڴ��

	�����Ŀ���Ǽ����ڴ�����ٶȣ����Ҽ������ظ�������ͬ
	�ڴ�ʱ�������ڴ���Ƭ�������ڷ�����Ӧ�ó����С�
*/

class CMemPool
{
public:

	//������СΪblockSize���ڴ�飬�ڴ����ĿΪԤ�������ĿpreAlloc
	CMemPool(std::size_t blockSize, int preAlloc = 0, int maxAlloc = 0);

	~CMemPool();

	//��ȡһ���ڴ�顣����ڴ����û���㹻���ڴ�飬����Զ������µ��ڴ��
	//���������ڴ����Ŀ�ﵽ�����ֵ����᷵��һ���쳣
	void* Get();

	//�ͷŵ�ǰ�ڴ�飬��������ڴ��
	void Release(void* ptr);

	//�����ڴ���С
	std::size_t BlockSize() const;

	//�����ڴ�����ڴ����Ŀ
	int Allocated() const;

	//�����ڴ���п��õ��ڴ����Ŀ
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