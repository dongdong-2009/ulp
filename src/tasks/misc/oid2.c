#include "config.h"
#include "ulp/sys.h"

int main(void)
{	
	sys_init();
	while(1) {
		sys_update();
	}
}
