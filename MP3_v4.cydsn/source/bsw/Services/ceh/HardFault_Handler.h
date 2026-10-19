/**
 * \file 	HardFault_Handler.h
 * \author 	Thomas Barth	- Hochschule Darmstadt thomas.barth@h-da.de
 * \date 	03.03.2020
 *
 * \brief ARM Hard Fault Interrupt Handler
 *
 * The ARM Cortex-M core implements a set of fault exceptions. Each exception relates to an error condition. If the error occurs, the ARM Cortex-M core stops executing the current instruction, and branches to the exception's handler function. This mechanism is just like that used for interrupts, where the ARM Cortex-M core branches to an interrupt handler when it accepts an interrupt.
 * The CMSIS names for the fault handlers are as follows:
 * 
 * UsageFault_Handler()
 * BusFault_Handler()
 * MemMang_Handler()
 * HardFault_Handler()
 * The exact circumstances under which the ARM Cortex-M core calls each of these handlers is out of scope of this document. See the ARM Cortex-M literature from ARM, and various other sources, if you are interested in the details. For the purpose of this document it is enough to say that, if your application ends up in one of these handlers, then something has gone wrong. Hard faults are the most common fault type, as other fault types that are not enabled individually will be escalated to become a hard fault.
 * Despite the numerous RTOS support requests from people explaining that, when using the RTOS kernel, their application ends up in the hard fault handler, when the issue has been worked through, it is always shown that the cause of the hardware fault is not the kernel, but one of the following:
 * 
 * A misunderstanding of interrupt priorities on the ARM Cortex-M core (easy to do!), or a misunderstanding of how to use the FreeRTOS interrupt nesting model. The "Running the RTOS on a ARM Cortex-M Core" documentation page is provided specifically to assist with this.
 * A general RTOS user error. The FAQ "My Application Does Not Run - What Could Be Wrong" page is provided specifically to assist with this.
 * A bug in the application code.
 * Hard fault debugging should start by ensuring the software application follows the guidelines provided on the two pages linked to in the first two bullet points above. If, after that, hard faults still persist, then it will be necessary to determine the system state at the time the fault occurred. Debuggers do not always make that easy, so the rest of this page describes a software technique than can be used for the purpose.
 * 
 * 
 * Determining Which Exception Handler is Executing
 * 
 * It is common for interrupt vector tables to install the same handler for every interrupt/exception source. The default handlers are declared as weak symbols to allow the application writer to install their own handler simply by implementing a function with the correct name. If an interrupt occurs for which the application writer has not provided their own handler then the default handler will execute.
 * Default interrupt handlers are typically implemented as an infinite loop. If an application ends up in such a default handler it is first necessary to determine which interrupt is actually executing.
 * 
 * The code snippet below demonstrates how to add a few instructions to a default infinite loop handler to load the number of the executing interrupt into register 2 (r2) before the infinite loop is entered.
 * 
 * Interrupt numbers read from the NVIC in this way are relative to the start of the vector table, in which entries for system exceptions (such as the hard fault) appear before entries for peripheral interrupts. If r2 contains the value 3 then, a hard fault exception is being handled. If r2 contains a value equal to or greater than 16, then a peripheral interrupt is being handled - and the interrupting peripheral can be determined by subtracting 16 from the interrupt number.
 * 
 * Debugging a ARM Cortex-M Hard Fault
 * 
 * The stack frame of the fault handler contains the state of the ARM Cortex-M registers at the time that the fault occurred. The code below shows how to read the register values from the stack into C variables. Once this is done, the values of the variables can be inspected in a debugger just as an other variable.
 * First, a very short assembly function is defined to determine which stack was being used when the fault occurred. Once this is done, the fault handler assembly code passes a pointer to the stack into a C function called FAULTHANDLER_getRegistersFromStack().
 * 
 * FAULTHANDLER_getRegistersFromStack() copies the register values from the stack into the C variables, then sits in a loop. The variables are named to indicate the register value that they hold. Other registers will not have changed since the fault occurred, and can be viewed directly in the debugger's CPU register window.
 * 
 * 
 * Using the Register Values
 * 
 * [See also "Handling Imprecise Faults" below]
 * 
 * The first register of interest is the program counter. In the code above, the variable pc contains the program counter value. When the fault is a precise fault, the pc holds the address of the instruction that was executing when the hard fault (or other fault) occurred. When the fault is an imprecise fault, then additional steps are required to find the address of the instruction that caused the fault.
 * 
 * To find the instruction at the address held in the pc variable, either...
 * 
 * Open an assembly code window in the debugger, and manually enter the address to view the assembly instructions at that address, or
 * Open the break point window in the debugger, and manually define an execution or access break point at that address. With the break point set, restart the application to see which line of code the instruction relates to.
 * Knowing the instruction that was being executed when the fault occurred allows you to know which other register values are also of interest. For example, if the instruction was using the value of R7 as an address, then the value of R7 needs to be know. Further, examining the assembly code, and the C code that generated the assembly code, will show what R7 actually holds (it might be the value of a variable, for example).
 * 
 * Handling Imprecise Faults
 * 
 * ARM Cortex-M faults can be precise or imprecise. If the IMPRECISERR bit (bit 2) is set in the BusFault Status Register (or BFSR, which is byte accessible at address 0xE000ED29) is set then the fault is imprecise.
 * It is harder to determine the cause of an imprecise fault because the fault will not necessarily occur concurrently with the instruction that caused the fault. For example, if writes to memory are cached then there might be a delay between an assembly instruction initiating a write to memory and the write to memory actually occurring. If such a delayed write operation is invalid (for example, a write is being attempted to an invalid memory location) then an imprecise fault will occur, and the program counter value obtained using the code above will not be the address of the assembly instruction that initiated the write operation.
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
 
#ifndef FAULTHANDLER_H
#define FAULTHANDLER_H

/** \brief Initialize the fault handling system by setting the handlers */
void FAULTHANDLER_Install(void);
    
#endif /*_FAULT_HANDLER_H_*/
/* [] END OF FILE */
