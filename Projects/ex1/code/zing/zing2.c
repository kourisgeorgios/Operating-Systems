#include <stdio.h>
#include <unistd.h>

void zing(void)
{
	printf("Hello humans of %s\n", getlogin());
}

