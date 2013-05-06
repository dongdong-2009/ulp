/*
 * 	king@2013 initial version
 */

#ifndef __PMSMDPRIV_H
#define __PMSMDPRIV_H

struct pmsmd_priv_s {

	struct {
		int Du; //offset of U Phase
		int Dv;
		int Dw;
		int Da; //offset of all
		int Gu; //gain of U Phase
		int Gv;
		int Gw;
		int Ga; //gain of all
	} I;

	int deadtime; //us?
	int crp_multi;

};

#endif	/* __PMSMDPRIV_H */
