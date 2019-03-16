#include <time.h>
#include <ctime>
#include <Windows.h>
#include <string>
#include <vector>

int main()
{
	char chr[] = "(255,255,128)";
char chArry[] = "one two   three,four * five";


	clock_t start, end;

	start = clock();
	Sleep(999);
	end = clock();
	int exetime = int(end - start);
	double miao = exetime/CLK_TCK;
	miao = exetime / CLOCKS_PER_SEC;

	system("pause");
	return 0;
}