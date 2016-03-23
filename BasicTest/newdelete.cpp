#include <iostream>
#include <string.h>

using namespace std;

typedef struct 
{
    char * name;
    int num;
    char sex;
}Student;

int main()
{
	Student pp;
	pp.name = "abc";
	pp.sex = 'f';
	Student *qq;

	//qq->name = "lll";
    Student *p;
    p = new Student; //用new运算符开辟一个存放Student类型的数据指针变量p
	cout << p -> num << endl << p -> sex << endl;
	if(p == NULL)
		cout<<"是空的"<<endl;
    p -> name = "Wang Ming";
    p -> num = 21431;
    p -> sex = 'm';
	Student * q = new Student();
	cout << q -> num << endl << q -> sex << endl;
	if(q == NULL)
		cout<<"是空的"<<endl;
	q->name = new char[88];

    cout << p -> num << endl << p -> sex << endl;
    delete p;
	cout <<  p -> num << endl << p -> sex << endl;
	delete q;
    cout << "Hello world!" << endl;
    return 0;
	system("pause");
}
