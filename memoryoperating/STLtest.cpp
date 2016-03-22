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
	//基本类型直接保存，此时push直接做了一次拷贝，事实上是栈上的内存拷贝
	for(int i = 0; i < 10; i++)
	{
		input = i;
		intArray.push(input);
	}
	//基本类型指针，虽然push也做了一次拷贝工作，但实际上是浅拷贝，只是拷贝了指针的值，没有为queue里面的指针开辟新的空间；
	for(int j = 0; j < 10; j++)
	{
		int * inputP = new int[1];
		*inputP = j;
		intpArray.push(inputP);
		delete inputP;
	}
	//类对象实例，push也做一次拷贝构造工作；
	for(char c = 65; c < 75; c++)
	{	
		cc[1] = c;
		string str = cc;
		stringArray.push(str);
	}
	system("pause");
}