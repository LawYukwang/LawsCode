#include <iostream>
#include <queue>
#include <string>

using namespace std;

queue<int> intArray;
queue<int*> intpArray;
queue<string> stringArray;

int main()
{
	char cc[2] ={'a','a'};
	int input;
	//��������ֱ�ӱ��棬��ʱpushֱ������һ�ο�������ʵ����ջ�ϵ��ڴ濽��
	for(int i = 0; i < 10; i++)
	{
		input = i;
		intArray.push(input);
	}
	//��������ָ�룬��ȻpushҲ����һ�ο�����������ʵ������ǳ������ֻ�ǿ�����ָ���ֵ��û��Ϊqueue�����ָ�뿪���µĿռ䣻
	for(int j = 0; j < 10; j++)
	{
		int * inputP = new int[1];
		*inputP = j;
		intpArray.push(inputP);
		delete inputP;
	}
	//�����ʵ����pushҲ��һ�ο������칤����
	for(char c = 65; c < 75; c++)
	{	
		cc[1] = c;
		string str = cc;
		stringArray.push(str);
	}
	system("pause");
}