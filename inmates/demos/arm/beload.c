#include <inmate.h>
#include <gic.h>
#include <asm/sysregs.h>

typedef void (**user_handler)(void);

static void handle_IRQ(unsigned int irqn)
{
	(*(user_handler)0xfffc)();
}

void inmate_main(void)
{
	map_range((void*)0x49000000, 0x77000000, MAP_CACHED);
	map_range((void*)0x4a000000, 0x01000000, MAP_UNCACHED);
	map_range((void*)0x488f0000, 0x10000, MAP_CACHED);
	memset((void *)0x488f1000, 0, 0x9000);
	memset((void *)0x488fc000, 0, 0x4000);

	map_range((void *)0x01000000, 0x00c80000, MAP_UNCACHED);
	map_range((void *)0x01900000, 0x02000000-0x01900000, MAP_UNCACHED);

	irq_init(handle_IRQ);

        asm("mov.w r0, #0x49000000 ; blx r0");
	halt();
}

void vector_unused(void);
void arch_mmu_enable_secondary(void);

void __attribute__((naked)) vector_unused(void)
{
	asm("mov r0, #0xfff0; mov sp, r0");

	arch_mmu_enable_secondary();

        asm("mov.w r0, #0x49000000 ; blx r0");
	halt();
}
