/*
 * 	king@2013 initial version
 */

static const int sinTableQ15[257] = {
	0x00000000, 0x000000c9, 0x00000192, 0x0000025b, 0x00000324, 0x000003ed, 0x000004b6, 0x0000057f, \
	0x00000647, 0x00000710, 0x000007d9, 0x000008a2, 0x0000096a, 0x00000a33, 0x00000afb, 0x00000bc3, \
	0x00000c8b, 0x00000d53, 0x00000e1b, 0x00000ee3, 0x00000fab, 0x00001072, 0x00001139, 0x00001201, \
	0x000012c8, 0x0000138e, 0x00001455, 0x0000151b, 0x000015e2, 0x000016a8, 0x0000176d, 0x00001833, \
	0x000018f8, 0x000019bd, 0x00001a82, 0x00001b47, 0x00001c0b, 0x00001ccf, 0x00001d93, 0x00001e56, \
	0x00001f19, 0x00001fdc, 0x0000209f, 0x00002161, 0x00002223, 0x000022e5, 0x000023a6, 0x00002467, \
	0x00002528, 0x000025e8, 0x000026a8, 0x00002767, 0x00002826, 0x000028e5, 0x000029a3, 0x00002a61, \
	0x00002b1f, 0x00002bdc, 0x00002c98, 0x00002d55, 0x00002e11, 0x00002ecc, 0x00002f87, 0x00003041, \
	0x000030fb, 0x000031b5, 0x0000326e, 0x00003326, 0x000033de, 0x00003496, 0x0000354d, 0x00003604, \
	0x000036ba, 0x0000376f, 0x00003824, 0x000038d8, 0x0000398c, 0x00003a40, 0x00003af2, 0x00003ba5, \
	0x00003c56, 0x00003d07, 0x00003db8, 0x00003e68, 0x00003f17, 0x00003fc5, 0x00004073, 0x00004121, \
	0x000041ce, 0x0000427a, 0x00004325, 0x000043d0, 0x0000447a, 0x00004524, 0x000045cd, 0x00004675, \
	0x0000471c, 0x000047c3, 0x00004869, 0x0000490f, 0x000049b4, 0x00004a58, 0x00004afb, 0x00004b9e, \
	0x00004c3f, 0x00004ce1, 0x00004d81, 0x00004e21, 0x00004ebf, 0x00004f5e, 0x00004ffb, 0x00005097, \
	0x00005133, 0x000051ce, 0x00005269, 0x00005302, 0x0000539b, 0x00005433, 0x000054ca, 0x00005560, \
	0x000055f5, 0x0000568a, 0x0000571d, 0x000057b0, 0x00005842, 0x000058d4, 0x00005964, 0x000059f3, \
	0x00005a82, 0x00005b10, 0x00005b9d, 0x00005c29, 0x00005cb4, 0x00005d3e, 0x00005dc7, 0x00005e50, \
	0x00005ed7, 0x00005f5e, 0x00005fe3, 0x00006068, 0x000060ec, 0x0000616f, 0x000061f1, 0x00006271, \
	0x000062f2, 0x00006371, 0x000063ef, 0x0000646c, 0x000064e8, 0x00006563, 0x000065dd, 0x00006657, \
	0x000066cf, 0x00006746, 0x000067bd, 0x00006832, 0x000068a6, 0x00006919, 0x0000698c, 0x000069fd, \
	0x00006a6d, 0x00006adc, 0x00006b4a, 0x00006bb8, 0x00006c24, 0x00006c8f, 0x00006cf9, 0x00006d62, \
	0x00006dca, 0x00006e30, 0x00006e96, 0x00006efb, 0x00006f5f, 0x00006fc1, 0x00007023, 0x00007083, \
	0x000070e2, 0x00007141, 0x0000719e, 0x000071fa, 0x00007255, 0x000072af, 0x00007307, 0x0000735f, \
	0x000073b5, 0x0000740b, 0x0000745f, 0x000074b2, 0x00007504, 0x00007555, 0x000075a5, 0x000075f4, \
	0x00007641, 0x0000768e, 0x000076d9, 0x00007723, 0x0000776c, 0x000077b4, 0x000077fa, 0x00007840, \
	0x00007884, 0x000078c7, 0x00007909, 0x0000794a, 0x0000798a, 0x000079c8, 0x00007a05, 0x00007a42, \
	0x00007a7d, 0x00007ab6, 0x00007aef, 0x00007b26, 0x00007b5d, 0x00007b92, 0x00007bc5, 0x00007bf8, \
	0x00007c29, 0x00007c5a, 0x00007c89, 0x00007cb7, 0x00007ce3, 0x00007d0f, 0x00007d39, 0x00007d62, \
	0x00007d8a, 0x00007db0, 0x00007dd6, 0x00007dfa, 0x00007e1d, 0x00007e3f, 0x00007e5f, 0x00007e7f, \
	0x00007e9d, 0x00007eba, 0x00007ed5, 0x00007ef0, 0x00007f09, 0x00007f21, 0x00007f38, 0x00007f4d, \
	0x00007f62, 0x00007f75, 0x00007f87, 0x00007f97, 0x00007fa7, 0x00007fb5, 0x00007fc2, 0x00007fce, \
	0x00007fd8, 0x00007fe1, 0x00007fe9, 0x00007ff0, 0x00007ff6, 0x00007ffa, 0x00007ffd, 0x00007fff, \
	0x00008000,
};

void sin_cos_q15(int theta, int *pSinVal, int *pCosVal)
{
	int theta_cov;
	int diff;
	int theta_in;
	int sector;

	theta_in = theta + 0x00008000;
	sector = (((theta_in >> 14) + 2) & 0x00000003) + 1;

	switch(sector) {
	case 1:
		theta_in = theta >> 6;
		diff = theta - (theta_in << 6);
		*pSinVal = ((diff * (sinTableQ15[theta_in + 1] - sinTableQ15[theta_in])) >> 6) + sinTableQ15[theta_in];
		*pCosVal = ((diff * (sinTableQ15[255 - theta_in] - sinTableQ15[256 - theta_in])) >> 6) + sinTableQ15[256 - theta_in];
		break;
	case 2:
		theta_cov = 0x00008000 - theta;
		theta_in = theta_cov >> 6;
		diff = theta_cov - (theta_in << 6);
		*pSinVal = ((diff * (sinTableQ15[theta_in + 1] - sinTableQ15[theta_in])) >> 6) + sinTableQ15[theta_in];
		*pCosVal = -(((diff * (sinTableQ15[255 - theta_in] - sinTableQ15[256 - theta_in])) >> 6) + sinTableQ15[256 - theta_in]);
		break;
	case 3:
		theta_cov = 0x00008000 + theta;
		theta_in = theta_cov >> 6;
		diff = theta_cov - (theta_in << 6);
		*pSinVal = -(((diff * (sinTableQ15[theta_in + 1] - sinTableQ15[theta_in])) >> 6) + sinTableQ15[theta_in]);
		*pCosVal = -(((diff * (sinTableQ15[255 - theta_in] - sinTableQ15[256 - theta_in])) >> 6) + sinTableQ15[256 - theta_in]);
		break;
	case 4:
		theta_cov = -theta;
		theta_in = theta_cov >> 6;
		diff = theta_cov - (theta_in << 6);
		*pSinVal = -(((diff * (sinTableQ15[theta_in + 1] - sinTableQ15[theta_in])) >> 6) + sinTableQ15[theta_in]);
		*pCosVal = ((diff * (sinTableQ15[255 - theta_in] - sinTableQ15[256 - theta_in])) >> 6) + sinTableQ15[256 - theta_in];
		break;
	default:
		//error
		break;
	}

}

/*
 * 	Clarke Transformation
 * 	alpha = a
 * 	beta = 2*b/sqrt(3) + a/sqrt(3)
 * 	2/sqrt(3) = 18918.6136208056 = 0x000049E6(Q14)
 * 	1/sqrt(3) = 9459.30681040282 = 0x000024F3(Q14)
 */

void clarke_q15(int Ia, int Ib, int *pIalpha, int *pIbeta)
{
	*pIalpha = Ia;
	*pIbeta = (0x000024F3 * Ia + 0x000049E6 * Ib) >> 14;
}

/*
 * 	d = alpha*cos(theta) + beta*sin(theta)
 * 	q = -alpha*sin(theta) + beta*cos(theta)
 * 	theta : [-0x8000~0x8000] -- [-180~180]
 */

void park_q15(int Ialpha, int Ibeta, int *pId, int *pIq, int SinVal, int CosVal)
{
	*pId = (Ialpha * CosVal + Ibeta * SinVal) >> 15;
	*pIq = (Ibeta * CosVal - Ialpha * SinVal) >> 15;
}

/*
 * 	alpha=d*cos(theta) - q*sin(theta)
 * 	beta=d*sin(theta) + q*cos(theta)
 * 	theta : [-0x8000~0x8000] -- [-180~180]
 */

void inv_park_q15(int Id, int Iq, int *pIalpha, int *pIbeta, int SinVal, int CosVal)
{
	*pIalpha = (Id * CosVal - Iq * SinVal) >> 15;
	*pIbeta = (Id * SinVal + Iq * CosVal) >> 15;
}
