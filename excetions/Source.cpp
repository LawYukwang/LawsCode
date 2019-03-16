#include<stdlib.h> 
#include<crtdbg.h> 
#include <iostream> 
// �ڴ�й¶������ 
#define _CRTDBG_MAP_ALLOC 
#ifdef _DEBUG 
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__) 
#endif
// �Զ����쳣�� 
class MyExcepction 
{ 
public:
	// ���캯��,����Ϊ������� 
	MyExcepction(int errorId) 
	{ 
		// ������캯����������Ϣ 
		std::cout << "MyExcepction is called" << std::endl; 
		m_errorId = errorId; 
	}
	// �������캯�� 
	MyExcepction( MyExcepction& myExp) 
	{ 
		// ����������캯����������Ϣ 
		std::cout << "copy construct is called" << std::endl; 
		this->m_errorId = myExp.m_errorId; 
	}
	~MyExcepction() 
	{ 
		// �������������������Ϣ 
		std::cout << "~MyExcepction is called" << std::endl; 
	}
	// ��ȡ������ 
	int getErrorId() 
	{ 
		return m_errorId; 
	}
private: 
	// ������ 
	int m_errorId; 
};

int main(int argc, char* argv[]) 
{ 
	// �ڴ�й¶������ 
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	// ���Ըı������,�Ա��׳���ͬ���쳣���в��� 
	int throwErrorCode = 110;
	std::cout << " input test code :" << std::endl; 
	std::cin >> throwErrorCode;
	try 
	{ 
		if ( throwErrorCode == 110) 
		{ 
			MyExcepction myStru(110);
			// �׳�����ĵ�ַ -> ��catch( MyExcepction* pMyExcepction) ���� 
			// ����ö���ĵ�ַ�׳���catch���,������ö���Ŀ������캯�� 
			// ����ַ���ᳫ������,����Ƶ���ص��øö���Ĺ��캯���򿽱����캯�� 
			// catch���ִ�н�����,myStru�ᱻ������ 
			throw &myStru; 
		} 
		else if ( throwErrorCode == 119 ) 
		{ 
			MyExcepction myStru(119);
			// �׳�����,�����ͨ���������캯������һ����ʱ�Ķ��󴫳���catch 
			// ��catch( MyExcepction myExcepction) ���� 
			// ��catch����л��ٴε���ͨ���������캯��������ʱ���������ﴫ��ȥ�Ķ��� 
			// throw������myStru�ᱻ������ 
			throw myStru; 
		} 
		else if ( throwErrorCode == 120 ) 
		{ 
			// ���ᳫ�������׳����� 
			// �������Ļ�,���catch( MyExcepction* pMyExcepction)�в�ִ��delete������ᷢ���ڴ�й¶
			// ��catch( MyExcepction* pMyExcepction) ���� 
			MyExcepction * pMyStru = new MyExcepction(120); 
			throw pMyStru; 
		} 
		else 
		{ 
			// ֱ�Ӵ����¶����׳� 
			// �൱�ڴ�������ʱ�Ķ��󴫵ݸ���catch��� 
			// ��catch����ʱͨ���������캯���ٴδ�����ʱ������մ��ݹ�ȥ�Ķ��� 
			// throw���������δ�������ʱ����ᱻ������ 
			throw MyExcepction(throwErrorCode); 
		} 
	} 
	catch( MyExcepction* pMyExcepction) 
	{ 
		// �������䱻ִ����Ϣ 
		std::cout << "ִ���� catch( MyExcepction* pMyExcepction) " << std::endl;
		// ���������Ϣ 
		std::cout << "error Code : " << pMyExcepction->getErrorId()<< std::endl;
		// �쳣�׳����¶��󲢷Ǵ����ں���ջ�ϣ����Ǵ�����ר�õ��쳣ջ��,����Ҫ����delete 
		//delete pMyExcepction; 
	} 
	catch ( MyExcepction myExcepction) 
	{ 
		// �������䱻ִ����Ϣ 
		std::cout << "ִ���� catch ( MyExcepction myExcepction) " << std::endl;
		// ���������Ϣ 
		std::cout << "error Code : " << myExcepction.getErrorId()<< std::endl; 
	} 
	catch(...) 
	{ 
		// �������䱻ִ����Ϣ 
		std::cout << "ִ���� catch(...) " << std::endl;
		// ������,�����׳����ϼ� 
		throw ; 
	}
	// ��ͣ 
	int temp; 
	std::cin >> temp;
	return 0; 
} 