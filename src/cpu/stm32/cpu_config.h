/*miaofng@2013-1-5 initial version
*/
#ifndef __CPU_CONFIG_H__
#define __CPU_CONFIG_H__

#ifdef CONFIG_STM32F10X_LD
	#define STM32F10X_LD
#endif

#ifdef CONFIG_STM32F10X_MD
	#define STM32F10X_MD
#endif

#ifdef CONFIG_STM32F10X_HD
	#define STM32F10X_HD
#endif

#ifdef CONFIG_STM32F10X_CL
	#define STM32F10X_CL
#endif

#ifdef CONFIG_STM32F10X_XL
	#define STM32F10X_XL
#endif

#ifdef CONFIG_USE_STDPERIPH_DRIVER
	#define USE_STDPERIPH_DRIVER
	#ifdef CONFIG_HSE_VALUE
		#define HSE_Value CONFIG_HSE_VALUE
	#endif
#endif

#ifdef CONFIG_USE_STDPERIPH_V35
	#define SystemFrequency SystemCoreClock
	#define USE_STDPERIPH_DRIVER
	#ifdef CONFIG_HSE_VALUE
		#define HSE_VALUE CONFIG_HSE_VALUE
	#endif
#endif

#endif