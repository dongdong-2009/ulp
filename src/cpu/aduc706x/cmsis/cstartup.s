;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Part one of the system initialization code,
;; contains low-level
;; initialization.
;;
;; Copyright 2007 IAR Systems. All rights reserved.
;;
;; $Revision: 14520 $
;;
#include "config.h"

        MODULE  ?cstartup

        ;; Forward declaration of sections.
        SECTION IRQ_STACK:DATA:NOROOT(3)
        SECTION FIQ_STACK:DATA:NOROOT(3)
        SECTION SVC_STACK:DATA:NOROOT(3)
        SECTION ABT_STACK:DATA:NOROOT(3)
        SECTION UND_STACK:DATA:NOROOT(3)
        SECTION CSTACK:DATA:NOROOT(3)

;
; The module in this file are included in the libraries, and may be
; replaced by any user-defined modules that define the PUBLIC symbol
; __iar_program_start or a user defined start symbol.
;
; To override the cstartup defined in the library, simply add your
; modified version to the workbench project.

        SECTION .intvec:CODE:NOROOT(2)

        PUBLIC  __vector
        PUBLIC  __vector_0x14
        PUBLIC  __iar_program_start
        EXTERN  Undefined_Handler
        EXTERN  SWI_Handler
        EXTERN  Prefetch_Handler
        EXTERN  Abort_Handler

        ARM
__vector:
        ; All default exception handlers (except reset) are
        ; defined as weak symbol definitions.
        ; If a handler is defined by the application it will take precedence.
        LDR     PC,Reset_Addr           ; Reset
        LDR     PC,Undefined_Addr       ; Undefined instructions
        LDR     PC,SWI_Addr             ; Software interrupt (SWI/SVC)
        LDR     PC,Prefetch_Addr        ; Prefetch abort
        LDR     PC,Abort_Addr           ; Data abort
__vector_0x14:
        DCD     0                       ; RESERVED
        LDR     PC,IRQ_Addr             ; IRQ
        LDR     PC,FIQ_Addr             ; FIQ

Reset_Addr:     DCD   __iar_program_start
Undefined_Addr: DCD   Undefined_Handler
SWI_Addr:       DCD   SWI_Handler
Prefetch_Addr:  DCD   Prefetch_Handler
Abort_Addr:     DCD   Abort_Handler
IRQ_Addr:       DCD   IRQ_Handler
FIQ_Addr:       DCD   FIQ_Handler


; --------------------------------------------------
; ?cstartup -- low-level system initialization code.
;
; After a reser execution starts here, the mode is ARM, supervisor
; with interrupts disabled.
;



        SECTION .text:CODE:NOROOT(2)

;        PUBLIC  ?cstartup
        EXTERN  ?main
        REQUIRE __vector

        ARM

__iar_program_start:
?cstartup:

;
; Add initialization needed before setup of stackpointers here.
;

;
; Initialize the stack pointers.
; The pattern below can be used for any of the exception stacks:
; FIQ, IRQ, SVC, ABT, UND, SYS.
; The USR mode uses the same stack as SYS.
; The stack segments must be defined in the linker command file,
; and be declared above.
;


; --------------------
; Mode, correspords to bits 0-5 in CPSR

MODE_MSK DEFINE 0x1F            ; Bit mask for mode bits in CPSR

USR_MODE DEFINE 0x10            ; User mode
FIQ_MODE DEFINE 0x11            ; Fast Interrupt Request mode
IRQ_MODE DEFINE 0x12            ; Interrupt Request mode
SVC_MODE DEFINE 0x13            ; Supervisor mode
ABT_MODE DEFINE 0x17            ; Abort mode
UND_MODE DEFINE 0x1B            ; Undefined Instruction mode
SYS_MODE DEFINE 0x1F            ; System mode


        MRS     r0, cpsr                ; Original PSR value

        ;; Set up the SVC stack pointer.
        MSR     cpsr_c, r0              ; Change the mode
        LDR     sp, =SFE(SVC_STACK)     ; End of SVC_STACK

        ;; Set up the interrupt stack pointer.

        BIC     r0, r0, #MODE_MSK       ; Clear the mode bits
        ORR     r0, r0, #IRQ_MODE       ; Set IRQ mode bits
        MSR     cpsr_c, r0              ; Change the mode
        LDR     sp, =SFE(IRQ_STACK)     ; End of IRQ_STACK

        ;; Set up the fast interrupt stack pointer.

        BIC     r0, r0, #MODE_MSK       ; Clear the mode bits
        ORR     r0, r0, #FIQ_MODE       ; Set FIR mode bits
        MSR     cpsr_c, r0              ; Change the mode
        LDR     sp, =SFE(FIQ_STACK)     ; End of FIQ_STACK

        ;; Set up the ABT stack pointer.
        BIC     r0, r0, #MODE_MSK       ; Clear the mode bits
        ORR     r0, r0, #ABT_MODE       ; Set abort mode bits
        MSR     cpsr_c, r0              ; Change the mode
        LDR     sp, =SFE(ABT_STACK)     ; End of ABT_STACK

        ;; Set up the UND stack pointer.
        BIC     r0, r0, #MODE_MSK       ; Clear the mode bits
        ORR     r0, r0, #UND_MODE       ; Set UND mode bits
        MSR     cpsr_c, r0              ; Change the mode
        LDR     sp, =SFE(UND_STACK)     ; End of UND_STACK

        ;; Set up the normal stack pointer.

        BIC     r0 ,r0, #MODE_MSK       ; Clear the mode bits
        ORR     r0 ,r0, #SYS_MODE       ; Set System mode bits
        MSR     cpsr_c, r0              ; Change the mode
        LDR     sp, =SFE(CSTACK)        ; End of CSTACK

#ifdef __ARMVFP__
        ;; Enable the VFP coprocessor.

        MOV     r0, #0x40000000         ; Set EN bit in VFP
        FMXR    fpexc, r0               ; FPEXC, clear others.

;
; Disable underflow exceptions by setting flush to zero mode.
; For full IEEE 754 underflow compliance this code should be removed
; and the appropriate exception handler installed.
;

        MOV     r0, #0x01000000         ; Set FZ bit in VFP
        FMXR    fpscr, r0               ; FPSCR, clear others.
#endif

;
; Add more initialization here
;
; Enable IRQ and FIQ interrupts
        mrs     r0,cpsr                             ; Original PSR value
;        bic     r0,r0,#0xC0
        orr     r0,r0,#IRQ_MODE                     ; Set IRQ mode bits
        msr     cpsr_c,r0

; Continue to ?main for C-level initialization.

        LDR     r0, =?main
        BX      r0

Mode_USR DEFINE      0x10
Mode_FIQ DEFINE      0x11
Mode_IRQ DEFINE      0x12
Mode_SVC DEFINE      0x13
Mode_ABT DEFINE      0x17
Mode_UND DEFINE      0x1B
Mode_SYS DEFINE      0x1F

I_Bit DEFINE         0x80    ; when I bit is set, IRQ is disabled 
F_Bit DEFINE         0x40    ; when F bit is set, FIQ is disabled 

FIQVEC                         DEFINE       0xFFFF011C
IRQVEC                         DEFINE       0xFFFF001C
FIQSTAN                        DEFINE       0xFFFF013C
IRQSTAN                        DEFINE       0xFFFF003C
IRQSTA                         DEFINE       0xFFFF0000
FIQSTA                         DEFINE       0xFFFF0100


        PUBWEAK FIQ_Handler
        SECTION .text:CODE:REORDER(2)
	ARM
FIQ_Handler:
;======================== Interrupts disabled!=================================;
;========================     FIQ Mode!       =================================;
	; This routine consumes 28 bytes of irq stack per nested interrupt
	; => 224 (8 x 28) due to the 8 maximum interrupt nesting.

	; This routine consumes 8 bytes of SYS stack per nested interrupt
	; => 64 (8 x 8) due to the 8 maximum interrupt nesting.

	; R12 is banked in FIQ mode is handling of this reqister is different
	; between the IRQ and FIQ handlers

        ; Stack modified registers (potentially callee modified too)
        STMDB   SP!    ,{R0-R3,LR}

        ; Stack the SPSR
        MRS     R0      ,SPSR
        STMDB   SP!    ,{R0}

#ifdef CONFIG_ADUC706X_VECTOR
        ; Retrieve handler for this interrupt
        LDR     R0      ,=FIQVEC
        LDR     R0      ,[R0]
        LDR     R2      ,[R0]

        ; Allow new higher priority FIQs in system mode
        MRS     R0      ,CPSR
        BIC     R0      ,R0     ,#F_Bit
        ORR     R1      ,R0     ,#Mode_SYS
        MSR     CPSR_c  ,R1 
#else
	LDR	R0,	=FIQSTA
	LDR	R0,	[R0]

	MOV	R1,	#1	; mask
	MOV	R3,	#0	; counter 1..19
FIQ_CNT_LOOP:
	LSL	R1,	R1,	#1
	ADD	R3,	R3,	#1
	CMP	R3,	#20
	BCS	FIQ_EXIT

	TST	R1,	R0
	BEQ	FIQ_CNT_LOOP

	;found .. exec it now
	EXTERN	vectors_irq
	ADR	R2,	vectors_irq
	LDR	R2,	[R2, +R3, LSL #2]

        ; change to system mode & disable IRQ/FIQ
        MRS     R0,	CPSR
        ORR     R1,	R0,	#(Mode_SYS | I_Bit | F_Bit)
        MSR     CPSR_c,	R1
#endif


;======================== Interrupts enabled! =================================;
;========================     SYS Mode!       =================================;
        ; We need to store the R12,LR of system mode also
        STMDB   SP!    ,{R12,LR}

        ; Calling interrupt handler in system mode with interrupts enabled
        MOV     LR,PC
        BX      R2

        ; We need to restore the R12,LR of system ,mode also
        LDMIA   SP!    ,{R12,LR}

        ; Change back to FIQ mode, disabling F interrupts 
        MRS     R0      ,CPSR
        BIC     R0      ,R0     ,#Mode_SYS
        ORR     R0      ,R0     ,#(Mode_FIQ | F_Bit)
        MSR     CPSR_c  ,R0 
;======================== Interrupts disabled!=================================;
;========================     FIQ Mode!       =================================;
#ifdef CONFIG_ADUC706X_VECTOR
        ; Clear the highest priority interrupt by writing 0xFF to FIQSTAN
        MOV      R0     ,#0xFF     
        LDR      R1     ,=FIQSTAN
        STR      R0     ,[R1]
#endif

FIQ_EXIT:
        ; Restore the SPSR
        LDMIA   SP!    ,{R0}
        MSR     SPSR_cxsf    ,R0

        ; Restore modified registers 
        LDMIA   SP!    ,{R0-R3,LR}

        ; Return from interrupt
        SUBS    PC     ,LR,#0x0004
;======================== Interrupts enabled! =================================;


        PUBWEAK IRQ_Handler
        SECTION .text:CODE:REORDER(2)
	ARM
IRQ_Handler:
;======================== Interrupts disabled!=================================;
;========================     IRQ Mode!       =================================;
        ; This routine consumes 28 bytes of irq stack per nested interrupt
        ; => 224 (8 x 28) due to the 8 maximum interrupt nesting.

        ; This routine consumes 8 bytes of SYS stack per nested interrupt
        ; => 64 (8 x 8) due to the 8 maximum interrupt nesting.

        ; R12 is banked in FIQ mode is handling of this reqister is different
        ; between the IRQ and FIQ handlers

        ; Stack modified registers (potentially callee modified too)
        STMDB   SP!    ,{R0-R3,R12,LR}

        ; Stack the SPSR
        MRS     R0      ,SPSR
        STMDB   SP!    ,{R0}

#ifdef CONFIG_ADUC706X_VECTOR
        ; Retrieve handler for this interrupt
        LDR     R0      ,=IRQVEC
        LDR     R0      ,[R0]
        LDR     R2      ,[R0]

        ; Allow new higher priority IRQs (and FIQs if enabled) in system mode
        MRS     R0      ,CPSR
        BIC     R0      ,R0     ,#I_Bit
        ORR     R1      ,R0     ,#Mode_SYS
        MSR     CPSR_c  ,R1 
#else
	LDR	R0,	=IRQSTA
	LDR	R0,	[R0]

	MOV	R1,	#1	; mask
	MOV	R3,	#0	; counter 1..19
IRQ_CNT_LOOP:
	LSL	R1,	R1,	#1
	ADD	R3,	R3,	#1
	CMP	R3,	#20
	BCS	IRQ_EXIT

	TST	R1,	R0
	BEQ	IRQ_CNT_LOOP

	;found .. exec it now
	EXTERN	vectors_irq
	ADR	R2,	vectors_irq
	LDR	R2,	[R2, +R3, LSL #2]

        ; change to system mode & disable IRQ
        MRS     R0,	CPSR
        ORR     R1,	R0,	#(Mode_SYS | I_Bit)
        MSR     CPSR_c,	R1
#endif

;======================== Interrupts enabled! =================================;
;========================     SYS Mode!       =================================;
        ; We need to store the R12,LR of system mode
        STMDB   SP!    ,{R12,LR}

        ; Calling interrupt handler in system mode with interrupts enabled
        MOV     LR,PC
        BX      R2

        ; We need to restore the R12,LR of system ,mode also
        LDMIA   SP!    ,{R12,LR}

        ; Change back to IRQ mode, disabling I interrupts 
        MRS     R0      ,CPSR
        BIC     R0      ,R0     ,#Mode_SYS
        ORR     R0      ,R0     ,#(Mode_IRQ | I_Bit)
        MSR     CPSR_c  ,R0 
;======================== Interrupts disabled!=================================;
;========================     IRQ Mode!       =================================;
#ifdef CONFIG_ADUC706X_VECTOR
        ; Clear the highest priority interrupt by writing 0xFF to IRQSTAN
        MOV      R0     ,#0xFF     
        LDR      R1     ,=IRQSTAN
        STR      R0     ,[R1]
#endif

IRQ_EXIT:
        ; Restore the SPSR
        LDMIA   SP!     ,{R0}
        MSR     SPSR_cxsf    ,R0

        ; Restore modified registers 
        LDMIA   SP!    ,{R0-R3,R12,LR}

        ; Return from interrupt
        SUBS    PC     ,LR,#0x0004
;======================== Interrupts enabled! =================================;


        END
