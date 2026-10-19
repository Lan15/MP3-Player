/**
 * \file 	HardFault_Regdef_CM3.h
 * \author 	Thomas Barth	- Hochschule Darmstadt thomas.barth@h-da.de
 * \date 	03.03.2020
 *
 * \brief Register definitions for ARM cortex M3 fault handling
 *
 *
 * \copyright Copyright ©2020
 * Department of electrical engineering and information technology, Hochschule Darmstadt - University of applied sciences (h_da). All Rights Reserved.
 * Permission to use, copy, modify, and distribute this software and its documentation for educational, and research purposes in the context of non-commercial
 * (unless permitted by h_da) and official h_da projects, is hereby granted for enrolled students of h_da, provided that the above copyright notice,
 * this paragraph and the following paragraph appear in all copies, modifications, and distributions.
 * Contact Prof.Dr.-Ing. Peter Fromm, peter.fromm@h-da.de, Birkenweg 8 64295 Darmstadt - GERMANY for commercial requests.
 *
 * \warning This software is a PROTOTYPE version and is not designed or intended for use in production, especially not for safety-critical applications!
 * The user represents and warrants that it will NOT use or redistribute the Software for such purposes.
 * This prototype is for research purposes only. This software is provided "AS IS," without a warranty of any kind.
 */
#ifndef HARDFAULT_REGDEF_CORTEXM3_H_
#define HARDFAULT_REGDEF_CORTEXM3_H_
    
#include "global.h"    

//-------------------------------------------------------------------- [Symbols]

#define ADR_SCB_SCR		((uint32_t*)0xE000ED10) 	/**< \brief Address of the System Control Register */
#define ADR_SCB_CCR		((uint32_t*)0xE000ED14)		/**< \brief Address of the Configuration and Control Register */
#define ADR_SCB_SHPR1 	((uint32_t*)0xE000ED18)		/**< \brief Address of the System Handler Priority register 1 */
#define ADR_SCB_SHPR2 	((uint32_t*)0xE000ED1C)		/**< \brief Address of the System Handler Priority register 2 */
#define ADR_SCB_SHPR3 	((uint32_t*)0xE000ED20)		/**< \brief Address of the System Handler Priority register 3 */
#define ADR_SCB_SHCRS 	((uint32_t*)0xE000ED24)		/**< \brief Address of the System Handler Control and State Register */
#define ADR_SCB_CFSR 	((uint32_t*)0xE000ED28)		/**< \brief Address of the Configurable Fault Status Register */
#define ADR_SCB_HFSR 	((uint32_t*)0xE000ED2C)		/**< \brief Address of the HardFault Status Register */
#define ADR_SCB_MMFAR 	((uint32_t*)0xE000ED34)		/**< \brief Address of the MemManage Fault Address Register */
#define ADR_SCB_BFAR 	((uint32_t*)0xE000ED38)		/**< \brief Address of the BusFault Address Register */

//-------------------------------------------------------------------- [Types]

/**
 * \brief System Control Register
 *
 * The CCR controls entry to Thread mode and enables the handlers for NMI, hard fault and faults escalated by FAULTMASK to ignore BusFaults
 * trapping of divide by zero and unaligned accesses access to the STIR by unprivileged software, see Software Trigger Interrupt Register. */
typedef union{

	unsigned int u32:32;				/**< \brief U32 access*/
	struct{
		/** \brief [0] reserved*/
		unsigned int reserved_1:1;

		/** \brief [1] Indicates sleep-on-exit when returning from Handler mode to Thread mode
		 *
		 * 0 = do not sleep when returning to Thread mode
		 * 1 = enter sleep, or deep sleep, on return from an ISR.
		 * Setting this bit to 1 enables an interrupt driven application to avoid returning to an empty main application.*/
		unsigned int SLEEPONEXIT:1;

		/** \brief [2] Controls whether the processor uses sleep or deep sleep as its low power mode
		 *
		 *	0 = sleep
		 *	1 = deep sleep. */
		unsigned int SLEEPDEEP:1;

		/** \brief [3] reserved*/
		unsigned int reserved_2:1;

		/** \brief [4] Send Event on Pending bit
		 *
		 * 0 = only enabled interrupts or events can wakeup the processor, disabled interrupts are excluded
		 * 1 = enabled events and all interrupts, including disabled interrupts, can wakeup the processor.
		 * When an event or interrupt enters pending state, the event signal wakes up the processor from WFE.
		 * If the processor is not waiting for an event, the event is registered and affects the next WFE.
		 * The processor also wakes up on execution of an SEV instruction or an external event.*/
		unsigned int SEVONPEND:1;

		/** \brief [31:5] reserved*/
		unsigned int reserved_3:27;
	}B;									/**< \brief Fieldwise access*/
}CORTEX_SCR_t;

/**
 * \brief Configuration and Control Register
 *
 * The CCR controls entry to Thread mode and enables the handlers for NMI, hard fault and faults escalated by FAULTMASK to ignore BusFaults
 * trapping of divide by zero and unaligned accesses access to the STIR by unprivileged software, see Software Trigger Interrupt Register. */
typedef union{

	unsigned int u32:32;				/**< \brief U32 access*/
	struct{

		/** \brief [0] Indicates how the processor enters Thread mode
		 *
		 *   0 = processor can enter Thread mode only when no exception is active
		 *   1 = processor can enter Thread mode from any level under the control of an EXC_RETURN value     */
		unsigned int NONBASETHRDENA:1;

		/** \brief [1] Enables unprivileged software access to the STIR, see Software Trigger Interrupt Register
		 *
		 *	0 = disable
		 *	1 = enable.     */
		unsigned int USERSETMPEND:1;

		/** \brief [2] reserved*/
		unsigned int reserved_1:1;

		/** \brief [3] Enables unaligned access traps:
		 *
		 *	0 = do not trap unaligned halfword and word accesses
		 *	1 = trap unaligned halfword and word accesses.
		 * If this bit is set to 1, an unaligned access generates a UsageFault.
		 * Unaligned LDM, STM, LDRD, and STRD instructions always fault irrespective of whether UNALIGN_TRP is set to 1.*/
		unsigned int UNALIGN_TRP:1;

		/** \brief [4] Enables faulting or halting when the processor executes an SDIV or UDIV instruction with a divisor of 0
		 *
		 *	0 = do not trap divide by 0
		 *	1 = trap divide by 0.
		 * When this bit is set to 0, a divide by zero returns a quotient of 0.*/
		unsigned int DIV_0_TRP:1;

		/** \brief [7:5] reserved*/
		unsigned int reserved_3:3;

		/** \brief [8] Enables handlers with priority -1 or -2 to ignore data BusFaults caused by load and store instructions.
		 *
		 * This applies to the hard fault, NMI, and FAULTMASK escalated handlers:
		 *	0 = data bus faults caused by load and store instructions cause a lock-up
		 *	1 = handlers running at priority -1 and -2 ignore data bus faults caused by load and store instructions.
		 * Set this bit to 1 only when the handler and its data are in absolutely safe memory.
		 * The normal use of this bit is to probe system devices and bridges to detect control path problems and fix them.   */
		unsigned int BFHFNMIGN:1;

		/** \brief [9] Indicates stack alignment on exception entry:
		 *
		 *	0 = 4-byte aligned
		 *	1 = 8-byte aligned.
		 * On exception entry, the processor uses bit[9] of the stacked PSR to indicate the stack alignment.
		 * On return from the exception it uses this stacked bit to restore the correct stack alignment. */
		unsigned int STKALIGN:1;

		/** \brief [31:10] reserved*/
		unsigned int reserved_22:22;
	}B;									/**< \brief Fieldwise access*/
}CORTEX_CCR_t;

/** \brief System Handler Priority register 1
 *
 * control the priority of exception handlers. */
typedef union{
	unsigned int u32:32;				/**< \brief U32 access*/
	struct{
		uint8_t	PRI_4;					/**< \brief [7:0] 	Priority of system handler 4, MemManage*/
		uint8_t PRI_5;					/**< \brief [15:8] 	Priority of system handler 5, BusFault*/
		uint8_t PRI_6;					/**< \brief [23:16] Priority of system handler 6, UsageFault*/
		uint8_t reserved;				/**< \brief [31:24] Reserved*/
	}B;									/**< \brief Fieldwise access*/
}CORTEX_SHPR1_t;

/** \brief System Handler Priority register 2
 *
 * control the priority of exception handlers. */
typedef union{
	unsigned int u32:32;				/**< \brief U32 access*/
	struct{
		uint32_t reserved:24;			/**< \brief [23:0] 	Reserved*/
		uint8_t  PRI_11;				/**< \brief [31:24]	Priority of system handler 11, SVCall*/
	}B;									/**< \brief Fieldwise access*/
}CORTEX_SHPR2_t;

/** \brief System Handler Priority register 3
 *
 * control the priority of exception handlers.  */
typedef union{
	unsigned int u32:32;				/**< \brief U32 access*/
	struct{
		uint16_t reserved;				/**< \brief [15:0]  Reserved*/
		uint8_t	PRI_14;					/**< \brief [23:16] Priority of system handler 14, PendSV*/
		uint8_t PRI_15;					/**< \brief [31:24] Priority of system handler 15, SysTick exception*/
	}B;									/**< \brief Fieldwise access*/
}CORTEX_SHPR3_t;

/** \brief System Handler Control and State Register
 *
 * The SHCSR enables the system handlers, and indicates the pending status of the BusFault, MemManage fault, and SVC exceptions
 * the active status of the system handlers. */
typedef union{
	unsigned int u32:32;				/**< \brief U32 access*/
	struct{
		unsigned int MEMFAULTACT:1;		/**< \brief [0]		MemManage exception active bit, reads as 1 if exception is active*/
		unsigned int BUSFAULTACT:1;		/**< \brief [1]		BusFault exception active bit, reads as 1 if exception is active*/
		unsigned int reseved_0:1;		/**< \brief [2]		Reserved*/
		unsigned int USGFAULTACT:1;		/**< \brief [3]		UsageFault exception active bit, reads as 1 if exception is active*/
		unsigned int reserved_1:3;		/**< \brief [6:4]	Reserved*/
		unsigned int SVCALLACT:1;		/**< \brief [7]		SVCall active bit, reads as 1 if SVC call is active*/
		unsigned int MONITORACT:1;		/**< \brief [8]		Debug monitor active bit, reads as 1 if Debug monitor is active*/
		unsigned int reserved_2:1;		/**< \brief [9]		Reserved*/
		unsigned int PENDSVACT:1;		/**< \brief [10]	PendSV exception active bit, reads as 1 if exception is active*/
		unsigned int SYSTICKACT:1;		/**< \brief [11]	SysTick exception active bit, reads as 1 if exception is active*/
		unsigned int USGFAULTPENDED:1;	/**< \brief [12]	UsageFault exception pending bit, reads as 1 if exception is pending*/
		unsigned int MEMFAULTPENDED:1;	/**< \brief [13]	MemManage exception pending bit, reads as 1 if exception is pending*/
		unsigned int BUSFAULTPENDED:1;	/**< \brief [14]	BusFault exception pending bit, reads as 1 if exception is pending*/
		unsigned int SVCALLPENDED:1;	/**< \brief [15]	SVCall pending bit, reads as 1 if exception is pending*/
		unsigned int MEMFAULTENA:1;		/**< \brief [16]	MemManage enable bit, set to 1 to enable*/
		unsigned int BUSFAULTENA:1;		/**< \brief [17]	BusFault enable bit, set to 1 to enable*/
		unsigned int USGFAULTENA:1;		/**< \brief [18] 	UsageFault enable bit, set to 1 to enable*/
		unsigned int reserved_3:13;		/**< \brief [31:19]	Reserved */
	}B;									/**< \brief Fieldwise access*/
}CORTEX_SHCRS_t;


#pragma pack(1)

/** \brief Configurable Fault Status Register
 *
 * The CFSR indicates the cause of a MemManage fault, BusFault, or UsageFault. */
 typedef union{
	unsigned int u32:32;				/**< \brief U32 access*/
	struct{
		union{
			unsigned int u8:8;			/**< \brief U8 access*/
			struct {
				/** \brief [0] Instruction access violation flag
				 *
				 * 0 = no instruction access violation fault
				 * 1 = the processor attempted an instruction fetch from a location that does not permit execution.
				 * This fault occurs on any access to an XN region, even when the MPU is disabled or not present.
				 * When this bit is 1, the PC value stacked for the exception return points to the faulting instruction.
				 * The processor has not written a fault address to the MMAR.*/
				unsigned int IACCVIOL:1;

				/** \brief [1] Data access violation flag
				 *
				 * 0 = no data access violation fault
				 * 1 = the processor attempted a load or store at a location that does not permit the operation.
				 * When this bit is 1, the PC value stacked for the exception return points to the faulting instruction.
				 * The processor has loaded the MMAR with the address of the attempted access. */
				unsigned int DACCVIOL:1;

				/** \brief [2]	-	Reserved*/
				unsigned int reserved_1:1;

				/** \brief [3] MemManage fault on unstacking for a return from exception
				 *
				 * 0 = no unstacking fault
				 * 1 = unstack for an exception return has caused one or more access violations.
				 * 	This fault is chained to the handler. This means that when this bit is 1, the original return stack is still present.
				 * The processor has not adjusted the SP from the failing return, and has not performed a new save.
				 * The processor has not written a fault address to the MMAR.*/
				unsigned int MUNSTKERR:1;

				/** \brief [4] MemManage fault on stacking for exception entry
				 *
				 * 0 = no stacking fault
				 * 1 = stacking for an exception entry has caused one or more access violations.
				 * When this bit is 1, the SP is still adjusted but the values in the context area on the stack might be incorrect.
				 * The processor has not written a fault address to the MMAR.*/
				unsigned int MSTKERR:1;

				/** \brief [6:5]	-	Reserved*/
				unsigned int reserved_2:2;

				/** \brief [7] MemManage Fault Address Register (MMFAR) valid flag
				 *
				 * 0 = value in MMAR is not a valid fault address
				 * 1 = MMAR holds a valid fault address.
				 * If a MemManage fault occurs and is escalated to a HardFault because of priority, the HardFault handler must set this bit to 0.
				 * This prevents problems on return to a stacked active MemManage fault handler whose MMAR value has been overwritten.*/
				unsigned int MMARVALID:1;
			}  B;				/**< \brief Fieldwise access*/
		}MMFSR;							/**< \brief MemManage Fault Status Register */
		union{
			unsigned int u8:8;			/**< \brief U8 access*/
			struct{
				/** \brief [0] Instruction bus error
				 *
				 * 0 = no instruction bus error
				 * 1 = instruction bus error.
				 * The processor detects the instruction bus error on prefetching an instruction, but it sets the IBUSERR flag to 1
				 * only if it attempts to issue the faulting instruction.
				 * When the processor sets this bit is 1, it does not write a fault address to the BFAR.*/
				unsigned int IBUSERR:1;

				/** \brief [1] Precise data bus error
				 *
				 * 0 = no precise data bus error
				 * 1 = a data bus error has occurred, and the PC value stacked for the exception return points to the instruction
				 * that caused the fault.
				 * When the processor sets this bit is 1, it writes the faulting address to the BFAR.*/
				unsigned int PRECISERR:1;

				/** \brief [2] Imprecise data bus error
				 *
				 * 0 = no imprecise data bus error
				 * 1 = a data bus error has occurred, but the return address in the stack frame is not related to the instruction
				 * that caused the error.
				 * When the processor sets this bit to 1, it does not write a fault address to the BFAR.
				 * This is an asynchronous fault. Therefore, if it is detected when the priority of the current process is higher
				 * than the BusFault priority, the BusFault becomes pending and becomes active only when the processor returns from
				 * all higher priority processes. If a precise fault occurs before the processor enters the handler for the imprecise BusFault,
				 * the handler detects both IMPRECISERR set to 1 and one of the precise fault status bits set to 1.*/
				unsigned int IMPRECISERR:1;

				/** \brief [3] BusFault on unstacking for a return from exception
				 *
				 * 0 = no unstacking fault
				 * 1 = unstack for an exception return has caused one or more BusFaults.
				 * This fault is chained to the handler. This means that when the processor sets this bit to 1,
				 * the original return stack is still present. The processor does not adjust the SP from the failing return,
				 * does not performed a new save, and does not write a fault address to the BFAR.*/
				unsigned int UNSTKERR:1;

				/** \brief [4] BusFault on stacking for exception entry
				 *
				 * 0 = no stacking fault
				 * 1 = stacking for an exception entry has caused one or more BusFaults.
				 * When the processor sets this bit to 1, the SP is still adjusted but the values in the context area on the stack might be incorrect.
				 * The processor does not write a fault address to the BFAR.*/
				unsigned int STKERR:1;

				/** \brief [6:5]	-	Reserved.*/
				unsigned int reserved:2;

				/** \brief [7] BusFault Address Register (BFAR) valid flag
				 *
				 * 0 = value in BFAR is not a valid fault address
				 * 1 = BFAR holds a valid fault address.
				 * The processor sets this bit to 1 after a BusFault where the address is known. Other faults can set this bit to 0,
				 * such as a MemManage fault occurring later.
				 * If a BusFault occurs and is escalated to a hard fault because of priority, the hard fault handler must set this bit to 0.
				 * This prevents problems if returning to a stacked active BusFault handler whose BFAR value has been overwritten.*/
				unsigned int BFARVALID:1;
			}B;				/**< \brief Fieldwise access*/
		}BFSR;							/**< \brief BusFault Status Register*/
		union{
			unsigned int u16:16;		/**< \brief U16 access*/
			struct{
				/** \brief [0] Undefined instruction UsageFault
				 *
				 * 0 = no undefined instruction UsageFault
				 * 1 = the processor has attempted to execute an undefined instruction.
				 * When this bit is set to 1, the PC value stacked for the exception return points to the undefined instruction.
				 * An undefined instruction is an instruction that the processor cannot decode.	*/
				unsigned int UNDEFINSTR:1;

				/** \brief [1] Invalid state UsageFault
				 *
				 * 0 = no invalid state UsageFault
				 * 1 = the processor has attempted to execute an instruction that makes illegal use of the EPSR.
				 * When this bit is set to 1, the PC value stacked for the exception return points to the instruction that attempted
				 * the illegal use of the EPSR.
				 * This bit is not set to 1 if an undefined instruction uses the EPSR.*/
				unsigned int INVSTATE:1;

				/** \brief [2] Invalid PC load UsageFault, caused by an invalid PC load by EXC_RETURN
				 *
				 * 0 = no invalid PC load UsageFault
				 * 1 = the processor has attempted an illegal load of EXC_RETURN to the PC, as a result of an invalid context,
				 * or an invalid EXC_RETURN value.
				 * When this bit is set to 1, the PC value stacked for the exception return points to the instruction that tried to
				 * perform the illegal load of the PC.*/
				unsigned int INVPC:1;

				/** \brief [3] No coprocessor UsageFault. The processor does not support coprocessor instructions
				 *
				 * 0 = no UsageFault caused by attempting to access a coprocessor
				 * 1 = the processor has attempted to access a coprocessor.*/
				unsigned int NOCP:1;

				/** \brief [7:4]	-	Reserved.*/
				unsigned int reserved_1:4;

				/** \brief [8] Unaligned access UsageFault
				 *
				 * 0 = no unaligned access fault, or unaligned access trapping not enabled
				 * 1 = the processor has made an unaligned memory access.
				 * Enable trapping of unaligned accesses by setting the UNALIGN_TRP bit in the CCR to 1, see Configuration and Control Register.
				 * Unaligned LDM, STM, LDRD, and STRD instructions always fault irrespective of the setting of UNALIGN_TRP.*/
				unsigned int UNALIGNED:1;

				/** \brief [9] Divide by zero UsageFault
				 *
				 * 0 = no divide by zero fault, or divide by zero trapping not enabled
				 * 1 = the processor has executed an SDIV or UDIV instruction with a divisor of 0.
				 * When the processor sets this bit to 1, the PC value stacked for the exception return points to the instruction that performed
				 * the divide by zero.
				 * Enable trapping of divide by zero by setting the DIV_0_TRP bit in the CCR to 1, see Configuration and Control Register.*/
				unsigned int DIVBYZERO:1;

				/** \brief [15:10]	-	Reserved.*/
				unsigned int reserved_2:6;
			}B;				/**< \brief Fieldwise access*/
		}UFSR;							/**< \brief UsageFault Status Register */
	} B;		/**< \brief Fieldwise access*/
 }CORTEX_CFSR_t;

#pragma pack()

/** \brief HardFault Status Register
 *
 * The HFSR gives information about events that activate the HardFault handler.
 * This register is read, write to clear. This means that bits in the register read normally, but writing 1 to any bit clears that bit to 0.*/
 typedef union{
	unsigned int u32:32;				/**< \brief U32 access*/
	struct{
		/** \brief [0]	-	Reserved.*/
		unsigned int reserved_1:1;

		/** \brief [1] Indicates a BusFault on a vector table read during exception processing
		 *
		 * 0 = no BusFault on vector table read
		 * 1 = BusFault on vector table read.
		 * This error is always handled by the hard fault handler.
		 * When this bit is set to 1, the PC value stacked for the exception return points to the instruction that was preempted by the exception.*/
		unsigned int VECTTBL:1;

		/** \brief [29:2]	-	Reserved.*/
		unsigned int reserved_2:28;

		/** \brief	[30] Indicates a forced hard fault
		 *
		 * generated by escalation of a fault with configurable priority that cannot be handles, either because of priority or because
		 * it is disabled:
		 * 0 = no forced HardFault
		 * 1 = forced HardFault.
		 * When this bit is set to 1, the HardFault handler must read the other fault status registers to find the cause of the fault.*/
		unsigned int FORCED:1;

		/** [31] Reserved for Debug use.
		 *
		 * When writing to the register you must write 0 to this bit, otherwise behavior is Unpredictable.*/
		unsigned int DEBUGEVT:1;
	}B;									/**< \brief Fieldwise access*/
 }CORTEX_HFSR_t;


/** \brief MemManage Fault Address Register
 *
 * The MMFAR contains the address of the location that generated a MemManage fault.
 * When an unaligned access faults, the address is the actual address that faulted.
 * Because a single read or write instruction can be split into multiple aligned accesses,
 * the fault address can be any address in the range of the requested access size. */
typedef union{
	unsigned int u32:32;				/**< \brief U32 access*/
	struct{
		/** When the MMARVALID bit of the MMFSR is set to 1,
		 * this field holds the address of the location that generated the MemManage fault*/
		unsigned int ADDRESS:32;
	}B;									/**< \brief Fieldwise access*/
}CORTEX_MMFAR_t;

/** \brief BusFault Address Register
 *
 * The BFAR contains the address of the location that generated a BusFault.
 * When an unaligned access faults the address in the BFAR is the one requested by the instruction,
 * even if it is not the address of the fault.*/
typedef union{
	unsigned int u32:32;				/**< \brief U32 access*/
	struct{
		/** When the BFARVALID bit of the BFSR is set to 1,
		 * this field holds the address of the location that generated the BusFault*/
		unsigned int ADDRESS:32;
	}B;									/**< \brief Fieldwise access*/
}CORTEX_BFAR_t;


#endif /* HARDFAULT_REGDEF_CORTEXM3_H_ */
