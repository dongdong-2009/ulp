/*
*	sky@2011 initial version
*/
#include "stdio.h"
#include "stdlib.h"
#include "sis_code.h"
#include "stm32f10x.h"
#include "stdio.h"
#include "stdlib.h"
#define countof(a)  (sizeof(a)/sizeof(*(a)));
uint16_t message[300];
char messageb[19];
unsigned short int size;
int cishu2=0,size2=0;
char manchester_valu(char *mes)
{
	int b[38],s1=100;
	b[0]=100,b[1]=200;
	int i,j=2;
	for(i=1;i<19;i++) {
		if(mes[i]==mes[i-1])
		{
			b[j]=b[j-1]+s1;
			b[j+1]=b[j]+s1;
			j=j+2;
		}

		if(mes[i]!=mes[i-1])
		{
			j=j-1;
			b[j]=b[j]+s1;
			b[j+1]=b[j]+s1;
			j=j+2;
		}
	}
	if (messageb[18]==0) {
		size=j;
	}
	else {
		size =j-1;
	}

	for(i=0;i<size;i++) {
		message[size2+i]=b[i]+4200*cishu2;
	}

	size2+=size;
	cishu2++;
	return 0;
}

void value_change(int x)
{
	int j;
	for(j=0;j<19;j++) {
		if((x>>j)&(1<<0)) {
			messageb[j]=1;
		}
		else
			messageb[j]=0;
	}
}

