//��򵥵Ĵ������߳�ʵ��
#include <stdio.h>
#include <windows.h>
//���̺߳���
DWORD WINAPI ThreadFun(LPVOID pM)
{
	printf("���̵߳��߳�ID��Ϊ��%d\n���߳����Hello World\n", GetCurrentThreadId());
	return 0;
}
//����������ν��������ʵ�������߳�ִ�еĺ�����
int main()
{
	printf("     ��򵥵Ĵ������߳�ʵ��\n");
	printf(" -- by MoreWindows( http://blog.csdn.net/MoreWindows ) --\n\n");

	HANDLE handle = CreateThread(NULL, 0, ThreadFun, NULL, 0, NULL);
	WaitForSingleObject(handle, INFINITE);
	system("pause");
	return 0;
}
//_beginthreadexԴ������By MoreWindows( http://blog.csdn.net/MoreWindows )
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
	_ptiddata ptd;          //pointer to per-thread data ��ע1
	uintptr_t thdl;         //thread handle �߳̾��
	unsigned long err = 0L; //Return from GetLastError()
	unsigned dummyid;    //dummy returned thread ID �߳�ID��
	
	// validation section ���initialcode�Ƿ�ΪNULL
	_VALIDATE_RETURN(initialcode != NULL, EINVAL, 0);

	//Initialize FlsGetValue function pointer
	__set_flsgetvalue();
	
	//Allocate and initialize a per-thread data structure for the to-be-created thread.
	//�൱��newһ��_tiddata�ṹ��������_ptiddataָ�롣
	if ( (ptd = (_ptiddata)_calloc_crt(1, sizeof(struct _tiddata))) == NULL )
		goto error_return;

	// Initialize the per-thread data
	//��ʼ���̵߳�_tiddata�鼴CRT�������� ��ע2
	_initptd(ptd, _getptd()->ptlocinfo);
	
	//����_tiddata�ṹ�е��������ݣ��������_tiddata������߳���ϵ��һ���ˡ�
	ptd->_initaddr = (void *) initialcode; //�̺߳�����ַ
	ptd->_initarg = argument;              //������̲߳���
	ptd->_thandle = (uintptr_t)(-1);
	
#if defined (_M_CEE) || defined (MRTDLL)
	if(!_getdomain(&(ptd->__initDomain))) //��ע3
	{
		goto error_return;
	}
#endif  // defined (_M_CEE) || defined (MRTDLL)
	
	// Make sure non-NULL thrdaddr is passed to CreateThread
	if ( thrdaddr == NULL )//�ж��Ƿ���Ҫ�����߳�ID��
		thrdaddr = &dummyid;

	// Create the new thread using the parameters supplied by the caller.
	//_beginthreadex()���ջ��ǻ����CreateThread()����ϵͳ���봴���߳�
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
	return(thdl); //�̴߳����ɹ�,�������̵߳ľ��.
	
	//Error return
error_return:
	//Either ptd is NULL, or it points to the no-longer-necessary block
	//calloc-ed for the _tiddata struct which should now be freed up.
	//������_calloc_crt()�����_tiddata��
	_free_crt(ptd);
	// Map the error, if necessary.
	// Note: this routine returns 0 for failure, just like the Win32
	// API CreateThread, but _beginthread() returns -1 for failure.
	//У���������(���Ե���GetLastError()�õ��������)
	if ( err != 0L )
		_dosmaperr(err);
	return( (uintptr_t)0 ); //����ֵΪNULL��Ч���
}
�����²��ִ��룺

ע1��_ptiddataptd;�е�_ptiddata�Ǹ��ṹ��ָ�롣��mtdll.h�ļ������壺

      typedefstruct_tiddata * _ptiddata

΢�������ע��ΪStructure for each thread's data������һ���ǳ���Ľṹ�壬�кܶ��Ա����������ƪ�����޾Ͳ��г����ˡ�

 

ע2��_initptd(ptd, _getptd()->ptlocinfo);΢�����һ������е�getptd()��˵��Ϊ��

// return address of per-thread CRT data 

      _ptiddata __cdecl_getptd(void);

��_initptd()˵�����£�

// initialize a per-thread CRT data block 

      void__cdecl_initptd(_Inout_ _ptiddata _Ptd,_In_opt_ pthreadlocinfo _Locale);

ע���е�CRT ��C Runtime Library������׼C���п⡣
ע3��if(!_getdomain(&(ptd->__initDomain)))�е�_getdomain()�������������thread.c�ļ����ҵ���
����Ҫ�����ǳ�ʼ��COM������
�������Դ�����֪��_beginthreadex()�����ڴ������߳�ʱ����䲢��ʼ��һ��_tiddata�顣
���_tiddata����Ȼ���������һЩ��Ҫ�̶߳�������ݡ���ʵ�����߳�����ʱ�����Ƚ�_tiddata�����Լ���һ������������
Ȼ�����̵߳��ñ�׼C���п⺯����strtok()ʱ�ͻ���ȡ��_tiddata��ĵ�ַ�ٽ���Ҫ���������ݴ���_tiddata���С�
����ÿ���߳̾�ֻ����ʺ��޸��Լ������ݶ�����ȥ�۸������̵߳������ˡ�
��ˣ�����ڴ�������ʹ�ñ�׼C���п��еĺ���ʱ������ʹ��_beginthreadex()������CreateThread()��
�����Ķ�������ʱ����������̵Ļ��и��ǳ���̵�ӡ����������Թ�������Ҳ��������׼ȷ�Ļش���^_^��
*/