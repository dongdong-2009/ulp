/*
*	miaofng@2011 initial version
*	- to emulate video DMA operation
*	- currently only single video frame is suported
*/

#include "config.h"
#include "stm32f10x.h"

struct vp_priv_s = {
	int camera; //camera fd
	int fb_poll; //result of vp_hw_poll, indicates which fb is ready
	int fb_size;

	unsigned char *fb;
};

static int vp_hw_open(struct vp_priv_s *priv);
static int vp_hw_capture(struct vp_priv_s *priv);
static int vp_hw_poll(struct vp_priv_s *priv);


#if CONFIG_BOARD_HY_SMART == 1
#define FIFO_CS_PIN     GPIO_Pin_6   /* FIFOÆ¬Ñ¡ */
#define FIFO_WRST_PIN   GPIO_Pin_7   /* FIFOÐ´µØÖ·¸´Î» */
#define FIFO_RRST_PIN   GPIO_Pin_0   /* FIFO¶ÁµØÖ·¸´Î» */
#define FIFO_RCLK_PIN   GPIO_Pin_1   /* FIFO¶ÁÊ±ÖÓ */
#define FIFO_WE_PIN     GPIO_Pin_3   /* FIFOÐ´ÔÊÐí */

#define FIFO_CS_H()     GPIOD->BSRR =FIFO_CS_PIN	  /* GPIO_SetBits(GPIOD , FIFO_CS_PIN)   */
#define FIFO_CS_L()     GPIOD->BRR  =FIFO_CS_PIN	  /* GPIO_ResetBits(GPIOD , FIFO_CS_PIN) */

#define FIFO_WRST_H()   GPIOB->BSRR =FIFO_WRST_PIN	  /* GPIO_SetBits(GPIOB , FIFO_WRST_PIN)   */
#define FIFO_WRST_L()   GPIOB->BRR  =FIFO_WRST_PIN	  /* GPIO_ResetBits(GPIOB , FIFO_WRST_PIN) */

#define FIFO_RRST_H()   GPIOE->BSRR =FIFO_RRST_PIN	  /* GPIO_SetBits(GPIOE , FIFO_RRST_PIN)   */
#define FIFO_RRST_L()   GPIOE->BRR  =FIFO_RRST_PIN	  /* GPIO_ResetBits(GPIOE , FIFO_RRST_PIN) */

#define FIFO_RCLK_H()   GPIOE->BSRR =FIFO_RCLK_PIN	  /* GPIO_SetBits(GPIOE , FIFO_RCLK_PIN)   */
#define FIFO_RCLK_L()   GPIOE->BRR  =FIFO_RCLK_PIN	  /* GPIO_ResetBits(GPIOE , FIFO_RCLK_PIN) */

//inverted by 74lvc1g00 (2 input nand gate)
#define FIFO_WE_H()     GPIOD->BRR  =FIFO_WE_PIN
#define FIFO_WE_L()     GPIOD->BSRR =FIFO_WE_PIN	  /* GPIO_SetBits(GPIOD , FIFO_WE_PIN)   */

int vp_xclk_set(int hz)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP ;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	RCC_MCOConfig(RCC_MCO_HSE);
}

/*this var is used to sync main prog with isr*/
static char vp_vsync;

static inline int vp_hw_open(struct vp_priv_s *priv)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOE, ENABLE);

	/* FIFO_RCLK : PE1 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	/* FIFO_RRST : PE0 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	/* FIFO_CS : PD6 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	/* FIFO_WEN : PD3 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	/* FIFO_WRST : PB7 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* FIFO D[0-7] */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	/*vsync irq init*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	EXTI_InitTypeDef EXTI_InitStructure;
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource0);
	EXTI_InitStructure.EXTI_Line = EXTI_Line0;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising ;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);
	EXTI_GenerateSWInterrupt(EXTI_Line0);

	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
	NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	FIFO_CS_L();
	FIFO_WE_H(); //DISABLE WRITE
	vp_vsync = -1;
	return 0;
}

static inline int vp_hw_capture(struct vp_priv_s *priv)
{
	vp_vsync = 0;
	return 0;
}

static inline int vp_hw_poll(struct vp_priv_s *priv)
{
	int i;
	priv->poll = 0;
	if(vp_vsync == 2) {
		priv->poll = 1;

		FIFO_RRST_L();
		FIFO_RCLK_L();
		FIFO_RCLK_H();
		FIFO_RRST_H();
		FIFO_RCLK_L();
		FIFO_RCLK_H();

		//copy&convert video dat from fifo to fb
		for(i = 0; i < priv->fb_size; i ++) {
			FIFO_RCLK_L();
			*fb ++ = GPIOC->IDR;
			FIFO_RCLK_H();
		}
	}
	return 0;
}

void EXTI0_IRQHandler(void)
{
	if ( EXTI_GetITStatus(EXTI_Line0) != RESET ) {
		if( vp_vsync == 0 ) {
			FIFO_WRST_L();  //FIFO RESET
			FIFO_WRST_H();
			FIFO_WE_L(); //ENABLE WRITE
			vp_vsync = 1;
		}
		else if( vp_vsync == 1 ) {
			FIFO_WE_H(); //DISABLE WRITE
			vp_vsync = 2;
		}

		EXTI_ClearITPendingBit(EXTI_Line0);
	}
}
#endif

int vp_open(int fd, const void *pcfg)
{
	const struct vp_cfg_s *vp_cfg = pcfg;
	struct vp_priv_s *priv = sys_malloc(sizeof(struct vp_priv_s));
	assert(priv != NULL);
	dev_priv_set(fd, priv);

	//try to open camera device
	const char *camera = "camera0";
	if(pcfg->camera != NULL)
		camera = pcfg->camera;
	priv->camera = dev_open(camera);
	assert(priv->camera != 0); //we must get a camera device here

	//vp port init
	vp_hw_open(priv);
	return 0;
}

int vp_ioctl(int fd, int request, va_list args)
{
	struct vp_priv_s *priv = dev_priv_get(fd);
	assert(priv->camera != 0);

	int ret = 0;
	switch (request) {
	case VIDEO_FORMAT:
		long fmt =  va_arg(args, long);
		int width = va_arg(args, int);
		int height = va_arg(args, int);
		ret = dev_ioctl(priv->camera, fmt, width, height);
		break;
	case VIDEO_BUFFER:
		priv->fb_size = va_arg(args, int);
		va_arg(args, int);
		priv->fb = va_arg(args, void *);
		break;
	case VIDEO_CAPTURE:
		ret = vp_hw_capture(priv);
		break;
	case VIDEO_POLL:
		void **pfb = va_arg(args, void **);
		ret = vp_hw_poll(priv);
		*pfb = (priv->poll) ? priv->fb : NULL;
		break;
	default:
		ret = -1;
	}

	return ret;
}

static int vp_close(int fd)
{
	struct vp_priv_s *priv = dev_priv_get(fd);
	if(priv->camera)
		dev_close(priv->camera);
	return 0;
}

static int vp_init(int fd, const void *pcfg)
{
	return dev_class_register(DEV_CLASS_VIDEO, fd);
}

static const struct drv_ops_s vp_ops = {
	.init = vp_init,
	.open = vp_open,
	.ioctl = vp_ioctl,
	.read = NULL,
	.write = NULL,
	.close = vp_close,
};

static struct driver_s vp_driver = {
	.name = "vp_fifo_stm32",
	.ops = &vp_ops,
};

static void __driver_init(void)
{
	drv_register(&vp_driver);
}
driver_init(__driver_init);


