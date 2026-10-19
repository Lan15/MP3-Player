/**
 * \file 	HardFault_Handler.c
 * \author 	Thomas Barth	- Hochschule Darmstadt thomas.barth@h-da.de
 * \date 	03.03.2020
 *
 * \brief ARM Cortex-M3 Hard Fault
 *
 * The register definitions are based on http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0552a/Bhcjabhi.html
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

//-------------------------------------------------------------------- [includes]

#include "HardFault_Handler.h"
#include <project.h>
#include "HardFault_Regdef_CM3.h"


//-------------------------------------------------------------------- [References]

/**
 * Reference to the interrupt vector table
 */
extern cyisraddress     CyRamVectors[];

//-------------------------------------------------------------------- [Defines]

//User Code to stop the system. Implementation depends on the architecture
#define FAULTHANDLER_stop(a)   asm("bkpt")

//-------------------------------------------------------------------- [Types]

/** \brief Fault Detail structure
 *
 * This structure is used to save all relevant information after an fault occurred */
typedef struct{
    struct{
        uint32_t r0;					/**< \brief CPU register R0. */
        uint32_t r1;					/**< \brief CPU register R1. */
        uint32_t r2;					/**< \brief CPU register R2. */
        uint32_t r3;					/**< \brief CPU register R3. */
        uint32_t r12;					/**< \brief CPU register R12. */
        uint32_t lr;    				/**< \brief Link register. */
        uint32_t pc;    				/**< \brief Program counter. */
        uint32_t psr;   				/**< \brief Program status register. */
    }CPU;
	struct{
		CORTEX_SHCRS_t 	SHCRS;			/**< \brief System Handler Control and State Register. */
		CORTEX_CFSR_t	CFSR;			/**< \brief Configurable Fault Status Register. */
		CORTEX_HFSR_t	HFSR;			/**< \brief HardFault Status Register. */
		CORTEX_MMFAR_t	MMFAR;			/**< \brief MemManage Fault Address Register. */
		CORTEX_HFSR_t	BFAR;			/**< \brief BusFault Address Register. */
    }Faults;
}FaultDetail_t;

//-------------------------------------------------------------------- [local variables]

/** \brief contains information about the last fault */
static volatile FaultDetail_t   FaultDetail={};

//-------------------------------------------------------------------- [local functions]

/** \brief Safe fault details
  *
  * This function gathers information about any fault
  */
void FAULTHANDLER_SaveDetails(void){

    //safe fault details
    FaultDetail.Faults.SHCRS.u32=*ADR_SCB_SHCRS;
    FaultDetail.Faults.CFSR.u32=*ADR_SCB_CFSR;
    FaultDetail.Faults.HFSR.u32=*ADR_SCB_HFSR;
    FaultDetail.Faults.MMFAR.u32=*ADR_SCB_MMFAR;
    FaultDetail.Faults.BFAR.u32=*ADR_SCB_BFAR;
}

/** \brief Hard Fault handler
  *
  * This function should be set as a hard fault "ISR"
  * It will gather information about the fault, which can be used for debugging
  * By calling FAULTHANDLER__getRegistersFromStack
  */
__attribute__((naked)) void FAULTHANDLER_HardFault_hdl(void){
    __asm volatile
    (
        " tst lr, #4                                                          \n"
        " ite eq                                                              \n"
        " mrseq r0, msp                                                       \n"
        " mrsne r0, psp                                                       \n"
        " ldr r1, [r0, #24]                                                   \n"
        " ldr r2, handler2_address_const                                      \n"
        " bx r2                                                               \n"
        " handler2_address_const: .word FAULTHANDLER__getRegistersFromStack   \n"
    );
}

/** \brief ISR for faults other than the hard fault */
static void FAULTHANDLER_fault_hdl(void){
    FAULTHANDLER_SaveDetails();
    FAULTHANDLER_stop(0);
}

/**
 * Gets the CPU registers which have been stored on the stack after an hard fault and stores them in RAM.
 * @param pulFaultStackAddress	Pointer to the stack in which the error information is stored
 */
void FAULTHANDLER__getRegistersFromStack( uint32_t *pulFaultStackAddress ){

    //safe CPU details from the stack
    FaultDetail.CPU.r0 = pulFaultStackAddress[ 0 ];
    FaultDetail.CPU.r1 = pulFaultStackAddress[ 1 ];
    FaultDetail.CPU.r2 = pulFaultStackAddress[ 2 ];
    FaultDetail.CPU.r3 = pulFaultStackAddress[ 3 ];

    FaultDetail.CPU.r12 = pulFaultStackAddress[ 4 ];
    FaultDetail.CPU.lr =  pulFaultStackAddress[ 5 ];
    FaultDetail.CPU.pc =  pulFaultStackAddress[ 6 ];
    FaultDetail.CPU.psr = pulFaultStackAddress[ 7 ];

    //safe additional details
    FAULTHANDLER_SaveDetails();

    /* When the following line is hit, the variables contain the register values. */
    FAULTHANDLER_stop(0);

    //supress warnings
    (void)FaultDetail;
}

//-------------------------------------------------------------------- [global functions]

void FAULTHANDLER_Install(void){
    //install fault handlers
    CyRamVectors[ CY_INT_HARD_FAULT_IRQN ]  = ( cyisraddress ) FAULTHANDLER_HardFault_hdl;       /* Hard Fault Interrupt        */
    CyRamVectors[ CY_INT_NMI_IRQN ]         = ( cyisraddress ) FAULTHANDLER_fault_hdl;           /* Non Maskable Interrupt      */
    CyRamVectors[ CY_INT_MEM_MANAGE_IRQN ]  = ( cyisraddress ) FAULTHANDLER_fault_hdl;           /* Memory Management Interrupt */
    CyRamVectors[ CY_INT_BUS_FAULT_IRQN ]   = ( cyisraddress ) FAULTHANDLER_fault_hdl;           /* Bus Fault Interrupt         */
    CyRamVectors[ CY_INT_USAGE_FAULT_IRQN ] = ( cyisraddress ) FAULTHANDLER_fault_hdl;           /* Usage Fault Interrupt       */
}



/* [] END OF FILE */
