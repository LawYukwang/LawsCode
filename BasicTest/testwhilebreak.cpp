#include <iostream>

int main()
{
	int a = 0;
	int b;
	while (true)
	{
		b = 1;
		while (true)
		{
			if((b % 3) == 0)
				break;
			b++;
		}
		a++;
		if(a == 99)
			break;
	}
}