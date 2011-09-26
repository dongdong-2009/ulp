#include "lib/nest.h"
#include "priv/mcamos.h"

void main(void)
{
	nest_init();
	nest_message("\nPower Conditioning - fixture\n");
	nest_message("IAR C Version v%x.%x, Compile Date: %s,%s\n", (__VER__ >> 24),((__VER__ >> 12) & 0xfff),  __TIME__, __DATE__);

	while(1){
		//wait for dut insertion
		nest_message("please press the start switch\n");
		nest_wait_plug_in();

		//send can message, remote not connected, so can controller try to resend again and again
		struct mcamos_s m = {
			.can = &can1,
		};
		nest_can_sel(DW_CAN);
		mcamos_init_ex(&m);
		mcamos_dnload_ex(0x0F000000, (char *) &m, sizeof(m));

		nest_message("please release the start switch to restart test routine\n");
		while(! cncb_detect(0)) {
			//flash all cncb outputs
			nest_message("flash sig1~6, bat/ign/lsd/led_f/led_r/led_p\n");
			for(int i = SIG1; i <= LSD; i ++) {
				cncb_signal(i, TOGGLE);
			}

			//handling serial command
			nest_mdelay(300);
		}
	}
} // main
