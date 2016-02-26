//最简单的创建多线程实例
#include <stdio.h>
#include <windows.h>
//子线程函数
DWORD WINAPI ThreadFun(LPVOID pM)
{
	printf("子线程的线程ID号为：%d\n子线程输出Hello World\n", GetCurrentThreadId());
	return 0;
}
//主函数，所谓主函数其实就是主线程执行的函数。
int main()
{
	printf("     最简单的创建多线程实例\n");
	printf(" -- by MoreWindows( http://blog.csdn.net/MoreWindows ) --\n\n");

	HANDLE handle = CreateThread(NULL, 0, ThreadFun, NULL, 0, NULL);
	WaitForSingleObject(handle, INFINITE);
	system("pause");
	return 0;
}
//_beginthreadex源码整理By MoreWindows( http://blog.csdn.net/MoreWindows )
/* 
_MCRTIMP uintptr_t __cdecl _beginthreadex(
	void *security,
	unsigned stacksize,
	unsigned (__CLR_OR_STD_CALL * initialcode) (void *),
	void * argument,
	unsigned createflag,
	unsigned *thrdaddr
)
{
	_ptiddata ptd;          //pointer to per-thread data 见注1
	uintptr_t thdl;         //thread handle 线程句柄
	unsigned long err = 0L; //Return from GetLastError()
	unsigned dummyid;    //dummy returned thread ID 线程ID号
	
	// validation section 检查initialcode是否为NULL
	_VALIDATE_RETURN(initialcode != NULL, EINVAL, 0);

	//Initialize FlsGetValue function pointer
	__set_flsgetvalue();
	
	//Allocate and initialize a per-thread data structure for the to-be-created thread.
	//相当于new一个_tiddata结构，并赋给_ptiddata指针。
	if ( (ptd = (_ptiddata)_calloc_crt(1, sizeof(struct _tiddata))) == NULL )
		goto error_return;

	// Initialize the per-thread data
	//初始化线程的_tiddata块即CRT数据区域 见注2
	_initptd(ptd, _getptd()->ptlocinfo);
	
	//设置_tiddata结构中的其它数据，这样这块_tiddata块就与线程联系在一起了。
	ptd->_initaddr = (void *) initialcode; //线程函数地址
	ptd->_initarg = argument;              //传入的线程参数
	ptd->_thandle = (uintptr_t)(-1);
	
#if defined (_M_CEE) || defined (MRTDLL)
	if(!_getdomain(&(ptd->__initDomain))) //见注3
	{
		goto error_return;
	}
#endif  // defined (_M_CEE) || defined (MRTDLL)
	
	// Make sure non-NULL thrdaddr is passed to CreateThread
	if ( thrdaddr == NULL )//判断是否需要返回线程ID号
		thrdaddr = &dummyid;

	// Create the new thread using the parameters supplied by the caller.
	//_beginthreadex()最终还是会调用CreateThread()来向系统申请创建线程
	if ( (thdl = (uintptr_t)CreateThread(
					(LPSECURITY_ATTRIBUTES)security,
					stacksize,
					_threadstartex,
					(LPVOID)ptd,
					createflag,
					(LPDWORD)thrdaddr))
		== (uintptr_t)0 )
	{
		err = GetLastError();
		goto error_return;
	}

	//Good return
	return(thdl); //线程创建成功,返回新线程的句柄.
	
	//Error return
error_return:
	//Either ptd is NULL, or it points to the no-longer-necessary block
	//calloc-ed for the _tiddata struct which should now be freed up.
	//回收由_calloc_crt()申请的_tiddata块
	_free_crt(ptd);
	// Map the error, if necessary.
	// Note: this routine returns 0 for failure, just like the Win32
	// API CreateThread, but _beginthread() returns -1 for failure.
	//校正错误代号(可以调用GetLastError()得到错误代号)
	if ( err != 0L )
		_dosmaperr(err);
	return( (uintptr_t)0 ); //返回值为NULL的效句柄
}
讲解下部分代码：

注1．_ptiddataptd;中的_ptiddata是个结构体指针。在mtdll.h文件被定义：

      typedefstruct_tiddata * _ptiddata

微软对它的注释为Structure for each thread's data。这是一个非常大的结构体，有很多成员。本文由于篇幅所限就不列出来了。

 

注2．_initptd(ptd, _getptd()->ptlocinfo);微软对这一句代码中的getptd()的说明为：

// return address of per-thread CRT data 

      _ptiddata __cdecl_getptd(void);

对_initptd()说明如下：

// initialize a per-thread CRT data block 

      void__cdecl_initptd(_Inout_ _ptiddata _Ptd,_In_opt_ pthreadlocinfo _Locale);

注释中的CRT （C Runtime Library）即标准C运行库。
注3．if(!_getdomain(&(ptd->__initDomain)))中的_getdomain()函数代码可以在thread.c文件中找到，
其主要功能是初始化COM环境。
由上面的源代码可知，_beginthreadex()函数在创建新线程时会分配并初始化一个_tiddata块。
这个_tiddata块自然是用来存放一些需要线程独享的数据。事实上新线程运行时会首先将_tiddata块与自己进一步关联起来。
然后新线程调用标准C运行库函数如strtok()时就会先取得_tiddata块的地址再将需要保护的数据存入_tiddata块中。
这样每个线程就只会访问和修改自己的数据而不会去篡改其它线程的数据了。
因此，如果在代码中有使用标准C运行库中的函数时，尽量使用_beginthreadex()来代替CreateThread()。
相信阅读到这里时，你会对这句简短的话有个非常深刻的印象，如果有面试官问起，你也可以流畅准确的回答了^_^。
*/