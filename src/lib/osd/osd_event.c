/*
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "osd/osd_event.h"
#include "osd/osd_dialog.h"
#include "key.h"
#include "pd.h"
#include "lcd.h"

int osd_event_init(void)
{
#ifdef CONFIG_DRIVER_PD
	pd_Init();
#endif
	key_Init();
	return 0;
}

int osd_event_get(void)
{
	int event = OSDE_NONE;

#ifdef CONFIG_DRIVER_PD
	osd_dialog_k *kdlg;
	int pde;
	dot_t p;
	
	//handle gui event when stimulated by pointing device like touch screen
	pde = pd_GetEvent(&p);
	if(pde != PDE_NONE) {
		kdlg = (osd_dialog_k *) osd_GetActiveDialog();
		event = osd_dlg_react(kdlg, pde, &p);
	}
#endif

	//handle keyboard event
	if(event == OSDE_NONE) {
		event = key_GetKey();
	}
	
	return event;
}

int osd_event_try(const rect_t *rc, const dot_t *pp)
{
	struct lcd_s *lcd = lcd_get(NULL);
	int fw, fh;
	rect_t r;
	
	lcd_get_font(lcd, &fw, &fh);
	rect_copy(&r, rc);
	rect_zoom(&r, fw, fh);
	return rect_have(&r, pp);
}
