/* START LIBRARY DESCRIPTION **************************************************
typesplus.lib
*.
*.COPYRIGHT (c) 2005 Delphi Corporation, All Rights Reserved
*.
*.Security Classification: none
*.
*.Description: This library contains global type and macro definitions.
*.
*.Functions: none
*.
*.Language: Z-World Dynamic C ( ANSII C with extensions and variations )
*.
*.Target Environment: Rabbit Semiconductor, Rabbit 3000 microprocessor used in
*.                    RCM3200 core module.
*.
*.Notes:
*. > This library should be considered the origin of other definitions. Thus,
*.   it does NOT "use" any other libraries.
*.
*.*****************************************************************************
*.Revision:
*.
*.DATE     USERID  DESCRIPTION
*.-------  ------  ------------------------------------------------------------
*.07APR05  qz78ws  > Original release.
*.
END DESCRIPTION ***************************************************************/


/*** BeginHeader */
#ifndef TYPESPLUS_LIB
#define TYPESPLUS_LIB
/*** EndHeader */


/*** BeginHeader */
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/* Type Specifiers                                                          */
/*                                                                          */
/* The following type specifiers are always guaranteed to have the same size*/
/* and sign properties regardless of the machine or compiler.               */
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
// 8-bits
//typedef          char INT8,   INT8_T,   int8_t;
typedef          char SINT8,  SINT8_T,  sint8_t;
typedef unsigned char UINT8,  UINT8_T,  uint8_t;

// 16-bits
//typedef          short INT16,   INT16_T,   int16_t;
typedef          short SINT16,  SINT16_T,  sint16_t;
typedef unsigned short UINT16,  UINT16_T,  uint16_t;

// 32-bits
//typedef          long  INT32,   INT32_T,   int32_t;
typedef          long  SINT32,  SINT32_T,  sint32_t;
typedef unsigned long  UINT32,  UINT32_T,  uint32_t;
typedef          float SFLOAT32, SFLOAT32_T, sfloat32_t;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/* Type Limits                                                              */
/*                                                                          */
/* These constants define the magnitude limits and sizes for the integral   */
/* types.                                                                   */
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
// 8-bits
#define BITS_UINT8            (8)
#define  MIN_UINT8           (0u)   /*0x0 0000 0000*/
#define  MAX_UINT8         (255u)   /*0x0 0000 00FF*/

#define BITS_SINT8            (8)
#define  MIN_SINT8         (-128)   /*0x0 0000 0080*/
#define  MAX_SINT8          (127)   /*0x0 0000 007F*/

// 16-bits
#define BITS_UINT16          (16)
#define  MIN_UINT16           (0)   /*0x0 0000 0000*/
#define  MAX_UINT16       (65535)   /*0x0 0000 FFFF*/

#define BITS_SINT16          (16)
#define  MIN_SINT16      (-32768)   /*0x0 0000 8000*/
#define  MAX_SINT16       (32767)   /*0x0 0000 7FFF*/

// 32-bits
#define BITS_UINT32          (32)
#define  MIN_UINT32           (0)   /*0x0 0000 0000*/
#define  MAX_UINT32  (4294967295)   /*0x0 FFFF FFFF*/

#define BITS_SINT32          (32)
#define  MIN_SINT32 (-2147483648)   /*0x0 8000 0000*/
#define  MAX_SINT32  (2147483647)   /*0x0 7FFF FFFF*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/* Boolean                                                                  */
/*                                                                          */
/* An enumeration is not used so there is no question as to how large a     */
/* boolean is.                                                              */
/*                                                                          */
/* NOTE - C defines logical expressions as having a value of 1 if true and  */
/* 0 if false.  The test part of conditional operators define true as non-  */
/* zero.  Thus, functions may return non-zero values as true.               */
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
typedef UINT8 BOOL, BOOL_T, bool, bool_t;
typedef UINT8 HWBOOL, HWBOOL_T, hwbool_t;

#define HWTRUE  (1)

#define HWFALSE (0)

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/* Data Direction Types                                                     */
/*                                                                          */
/* Most data direction registers are configured the same way.  This         */
/* definition allows a data direction register to be referenced in a uniform*/
/* manner.                                                                  */
/*                                                                          */
/* CAUTION:                                                                 */
/* Compiler-dependent options (i.e. -Xenum-is-short) will dictate bitfield  */
/* sizes and structure packing if "enum'ed" types are used in defining data */
/* types.                                                                   */
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
#define DD_INPUT  (0)
#define DD_OUTPUT (1)

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/* Conversion Constants                                                     */
/*                                                                          */
/* These make it convenient to convert to different units.                  */
/* Usage example:                                                           */
/*    Freq_Hz  = 4.5 * MHZ;                                                 */
/*    Time_Sec = 6.25 * MS;                                                 */
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
#define MHZ    (1000000)
#define KHZ    (1000)

#define MS     (1 / 1000)
#define US     (1 / 1000000)

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/* Member Access Definitions                                                */
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
#define OFFSETOF(type, field) ((size_t) &((type *) 0)->field)

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/* Bit Definitions                                                          */
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
#define BIT31  (0x80000000)
#define BIT30  (0x40000000)
#define BIT29  (0x20000000)
#define BIT28  (0x10000000)
#define BIT27  (0x08000000)
#define BIT26  (0x04000000)
#define BIT25  (0x02000000)
#define BIT24  (0x01000000)
#define BIT23  (0x00800000)
#define BIT22  (0x00400000)
#define BIT21  (0x00200000)
#define BIT20  (0x00100000)
#define BIT19  (0x00080000)
#define BIT18  (0x00040000)
#define BIT17  (0x00020000)
#define BIT16  (0x00010000)
#define BIT15  (0x00008000)
#define BIT14  (0x00004000)
#define BIT13  (0x00002000)
#define BIT12  (0x00001000)
#define BIT11  (0x00000800)
#define BIT10  (0x00000400)
#define BIT9   (0x00000200)
#define BIT8   (0x00000100)
#define BIT7   (0x00000080)
#define BIT6   (0x00000040)
#define BIT5   (0x00000020)
#define BIT4   (0x00000010)
#define BIT3   (0x00000008)
#define BIT2   (0x00000004)
#define BIT1   (0x00000002)
#define BIT0   (0x00000001)

#define BPOS31 (31)
#define BPOS30 (30)
#define BPOS29 (29)
#define BPOS28 (28)
#define BPOS27 (27)
#define BPOS26 (26)
#define BPOS25 (25)
#define BPOS24 (24)
#define BPOS23 (23)
#define BPOS22 (22)
#define BPOS21 (21)
#define BPOS20 (20)
#define BPOS19 (19)
#define BPOS18 (18)
#define BPOS17 (17)
#define BPOS16 (16)
#define BPOS15 (15)
#define BPOS14 (14)
#define BPOS13 (13)
#define BPOS12 (12)
#define BPOS11 (11)
#define BPOS10 (10)
#define BPOS9  (9)
#define BPOS8  (8)
#define BPOS7  (7)
#define BPOS6  (6)
#define BPOS5  (5)
#define BPOS4  (4)
#define BPOS3  (3)
#define BPOS2  (2)
#define BPOS1  (1)
#define BPOS0  (0)

#define Bin8(h,g,f,e,d,c,b,a) ((UINT8) \
   (h *  BIT7 + g *  BIT6 + f *  BIT5 + e *  BIT4 + d *  BIT3 + c *  BIT2 + b *  BIT1 + a *  BIT0))

#define Bin8Reflect(h,g,f,e,d,c,b,a) ((UINT8) \
   (h *  BIT0 + g *  BIT1 + f *  BIT2 + e *  BIT3 + d *  BIT4 + c *  BIT5 + b *  BIT6 + a *  BIT7))

#define Bin16(p,o,n,m,l,k,j,i,h,g,f,e,d,c,b,a) ((UINT16) \
   (p * BIT15 + o * BIT14 + n * BIT13 + m * BIT12 + l * BIT11 + k * BIT10 + j *  BIT9 + i *  BIT8 + \
    h *  BIT7 + g *  BIT6 + f *  BIT5 + e *  BIT4 + d *  BIT3 + c *  BIT2 + b *  BIT1 + a *  BIT0))

#define Bin16Reflect(p,o,n,m,l,k,j,i,h,g,f,e,d,c,b,a) ((UINT16) \
   (p *  BIT0 + o *  BIT1 + n *  BIT2 + m *  BIT3 + l *  BIT4 + k *  BIT5 + j *  BIT6 + i *  BIT7 + \
    h *  BIT8 + g *  BIT9 + f * BIT10 + e * BIT11 + d * BIT12 + c * BIT12 + b * BIT14 + a * BIT15))

#define Bin32(F,E,D,C,B,A,z,y,x,w,v,u,t,s,r,q,p,o,n,m,l,k,j,i,h,g,f,e,d,c,b,a) ((UINT32) \
   (F * BIT31 + E * BIT30 + D * BIT29 + C * BIT28 + B * BIT27 + A * BIT26 + z * BIT25 + y * BIT24 + \
    x * BIT23 + w * BIT22 + v * BIT21 + u * BIT20 + t * BIT19 + s * BIT18 + r * BIT17 + q * BIT16 + \
    p * BIT15 + o * BIT14 + n * BIT13 + m * BIT12 + l * BIT11 + k * BIT10 + j *  BIT9 + i *  BIT8 + \
    h *  BIT7 + g *  BIT6 + f *  BIT5 + e *  BIT4 + d *  BIT3 + c *  BIT2 + b *  BIT1 + a *  BIT0))

#define Bin32Reflect(F,E,D,C,B,A,z,y,x,w,v,u,t,s,r,q,p,o,n,m,l,k,j,i,h,g,f,e,d,c,b,a) ((UINT32) \
   (F *  BIT0 + E *  BIT1 + D *  BIT2 + C *  BIT3 + B *  BIT4 + A *  BIT5 + z *  BIT6 + y *  BIT7 + \
    x *  BIT8 + w *  BIT9 + v * BIT10 + u * BIT11 + t * BIT12 + s * BIT13 + r * BIT14 + q * BIT15 + \
    p * BIT16 + o * BIT17 + n * BIT18 + m * BIT19 + l * BIT20 + k * BIT21 + j * BIT22 + i * BIT23 + \
    h * BIT24 + g * BIT25 + f * BIT26 + e * BIT27 + d * BIT28 + c * BIT29 + b * BIT30 + a * BIT31))

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/* Hardware I/O Macro Definitions                                           */
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
#define BITCLEAR(register, mask)           register &= ~mask;
#define BITSET(register, mask)             register |= mask;
#define LOOPUNTIL_BITCLEAR(register, mask) while (register & mask);
#define LOOPUNTIL_BITSET(register, mask)   while (!(register & mask));

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/* Variable Manipulation Macro Definitions                                  */
/*                                                                          */
/* The LROTATE and RROTATE macros perform a bitwise rotate of "var". "rnum" */
/* contains the number of bits to rotate. These macros will work for any    */
/* size variable. "rnum must be less than the number of bits in the         */
/* variable.                                                                */
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
#define LROTATE(var, rnum)\
   var = (((var & (~(-1 << rnum)) << (sizeof(var) * 8 - rnum)) \
                                 >> ((sizeof(var)) * 8 - rnum)) | (var << rnum))

#define RROTATE(var, rnum)\
   var = (((var & (~(-1 << rnum))) << ((sizeof(var)) * 8 - rnum)) | (var >> rnum))
/*** EndHeader */


/*** BeginHeader */
#endif   // TYPESPLUS_LIB
/*** EndHeader */