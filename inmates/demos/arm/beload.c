#include <inmate.h>
#include <gic.h>
#include <asm/sysregs.h>
#define JAILHOUSE
#include "fixed_addr.h"
#define xstr(s) str(s)
#define str(s) #s

typedef void (**user_handler)(void);

static void handle_IRQ(unsigned int irqn)
{
	(*(user_handler)AWBM_IRQ_HANDLER_VECTOR)();
}

void inmate_main(void)
{
	map_range((void*)AWBM_BASE_ADDR, 0x77000000, MAP_CACHED);
	map_range((void*)AWBM_UNCACHED_ADDR, 0x01000000, MAP_UNCACHED);
	map_range((void*)JAILHOUSE_COMM_BASE, 0x10000, MAP_CACHED);
	memset((void *)SDL_EVENT_BUFFER_ADDR, 0, 0x9000);
	memset((void *)LIBC_CALL_BUFFER_ADDR, 0, 0x4000);

	map_range((void *)0x01000000, 0x00c80000, MAP_UNCACHED);
	map_range((void *)0x01900000, 0x02000000-0x01900000, MAP_UNCACHED);

	irq_init(handle_IRQ);

        asm("mov r0, #" xstr(AWBM_BASE_ADDR) " ; blx r0");
	halt();
}

void vector_unused(void);
void vector_svc(void);

void arch_mmu_enable_secondary(void);

void __attribute__((naked)) vector_unused(void)
{
	asm("mov r0, #" xstr(AWBM_DABT_HANDLER_VECTOR) "; mov sp, r0");

	arch_mmu_enable_secondary();

        asm("mov r0, #" xstr(AWBM_BASE_ADDR) " ; blx r0");
	halt();
}

void __attribute__((naked)) vector_svc(void)
{
	// Save regs, then jump to user SVC handler.
	asm("push {r0-r7,ip,lr}; mov r0, #" xstr(GDBSTUB_SVC_HANDLER_VECTOR) " ; ldr r0, [r0] ; bx r0");
}

void vector_dabt(void);
void __attribute__ ((naked)) vector_dabt(void)
{
	// Switch to system mode, re-enable interrupts, then jump to user
	// data abort handler.
	asm("cps #0x1f ; mov r0, #" xstr(AWBM_DABT_HANDLER_VECTOR) " ; ldr r0, [r0] ; cpsie if ; bx r0");
}
