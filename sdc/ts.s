/********************************************************************
Copyright 2010-2017 K.C. Wang, <kwang@eecs.wsu.edu>
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************/
	.text
.code 32
.global reset_handler
.global vectors_start, vectors_end
.global int_off, int_on, lock, unlock, getcsr
	
reset_handler:
  /* set SVC stack to HIGH END of proc[0].kstack[] */
  LDR r0, =proc      // r0 points to proc's
  LDR r1, =procsize  // r1 -> procsize
  LDR r2, [r1,#0]    // r2 = procsize
  ADD r0, r0, r2     // r0 -> high end of proc[0]
  MOV sp, r0
	
  /* go in IRQ mode to set IRQ stack and enable IRQ interrupts */
  MSR cpsr, #0x12
  LDR sp, =irq_stack_top  // set IRQ stack poiner

  /* go in ABT mode to set ABT stack */
  MSR cpsr, #0x17
  LDR sp, =abt_stack_top

  /* go in UND mode to set UND stack */
  MSR cpsr, #0x1B
  LDR sp, =und_stack_top

  /* go back in SVC mode */
  MSR cpsr, #0x13      // write to cspr, so in SVC mode now

  // set previous mode of SVC mode to USER mode with interrupts ON
  MSR spsr, #0x10

  /* copy vector table to address 0 */
  BL copy_vectors

  BL main
	
  B .
	
.align 4

irq_handler:        
  sub	lr, lr, #4  
  stmfd	sp!, {r0-r12, lr}   // save all Umode regs in kstack
  bl	IRQ_handler         // call IRQ_handler() in C
  ldmfd	sp!, {r0-r12, pc}^  // pop from kstack but restore Umode SR

int_off:          // SR = int_off()
  MRS r0, cpsr
  mov r1, r0
  ORR r1, r1, #0x80
  MSR cpsr, r1    // return value in r0 = original cpsr
  mov pc,lr	

int_on:           // int_on(SR);  SR in r0
  MSR cpsr, r0
  mov pc,lr	


unlock:
  MRS r0, cpsr
  BIC r0, r0, #0x80
  MSR cpsr, r0
  mov pc,lr	

lock:
  MRS r0, cpsr
  ORR r0, r0, #0x80
  MSR cpsr, r0
  mov pc,lr	

getcsr:
   mrs r0, cpsr
   mov pc, lr

svc_entry:
dummy:	
vectors_start:
  LDR PC, reset_handler_addr
  LDR PC, undef_handler_addr
  LDR PC, svc_handler_addr
  LDR PC, prefetch_abort_handler_addr
  LDR PC, data_abort_handler_addr
  B .
  LDR PC, irq_handler_addr
  LDR PC, fiq_handler_addr

reset_handler_addr:          .word reset_handler
undef_handler_addr:          .word undef_handler
svc_handler_addr:            .word svc_entry
prefetch_abort_handler_addr: .word dummy
data_abort_handler_addr:     .word dummy
irq_handler_addr:            .word irq_handler
fiq_handler_addr:            .word fiq_handler

vectors_end:

