// CMyMemPool.cpp : 定义控制台应用程序的入口点。
//

#include <Windows.h>
#include "MemPoolpp.h"

#define DATA_BLOCK_LEN 1500

int main(int argc, CHAR* argv[])
{
	CMemPool myPool1(DATA_BLOCK_LEN, 10, 10);

	cout<<"myPool1 block size = "<<myPool1.BlockSize()<<endl;
	cout<<"myPool1 allocated block num = "<<myPool1.Allocated()<<endl;
	cout<<"myPool1 available block num = "<<myPool1.Available()<<endl<<endl;

	std::vector<void*> ptrs;
	for (int i = 0; i < 10; ++i)
	{
		ptrs.push_back(myPool1.Get());
	}

	myPool1.Get();

	int iavilable = 0;
	for (std::vector<void*>::iterator it = ptrs.begin(); it != ptrs.end(); ++it)
	{
		myPool1.Release(*it);
		++iavilable;
		cout<<"myPool1 available block num = "<<myPool1.Available()<<endl;
	}

	CMemPool myPool2(DATA_BLOCK_LEN, 5, 10);

	cout<<endl<<"myPool2 block size = "<<myPool2.BlockSize()<<endl;
	cout<<"myPool2 allocated block num = "<<myPool2.Allocated()<<endl;
	cout<<"myPool2 available block num = "<<myPool2.Available()<<endl;

	int iWait;
	cin>>iWait;

	return 0;
}