#include "lib/nest.h"
#include "priv/mcamos.h"
#include "time.h"

static time_t fixture_timer;
static char fixture_led_on = 0;

void leds_set(int on)
{
	for(int i = SIG1; i <= SIG6; i ++)
		cncb_signal(i, (on)?SIG_LO:SIG_HI);
	for(int i = BAT; i <= LED_P; i ++)
		cncb_signal(i, (on)?LSD_ON:LSD_OFF);
}


void main(void)
{
	nest_init();
	nest_message("\nPower Conditioning - fixture\n");
	nest_message("IAR C Version v%x.%x, Compile Date: %s,%s\n", (__VER__ >> 24),((__VER__ >> 12) & 0xfff),  __TIME__, __DATE__);

	while(1){
		fixture_timer = 0;
		fixture_led_on = 1;
		leds_set(fixture_led_on);
		nest_update();
		//start test???
		while(! cncb_detect(0)) {
			nest_update();
			if(time_left(fixture_timer) < 0) {
				fixture_timer = time_get(3000);
				fixture_led_on = !fixture_led_on;
				leds_set(fixture_led_on);
			}
		}
	}
} // main
