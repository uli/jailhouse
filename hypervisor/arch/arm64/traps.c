/*
 * Jailhouse AArch64 support
 *
 * Copyright (C) 2015 Huawei Technologies Duesseldorf GmbH
 *
 * Authors:
 *  Antonios Motakis <antonios.motakis@huawei.com>
 *  Dmitry Voytik <dmitry.voytik@huawei.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <jailhouse/control.h>
#include <jailhouse/printk.h>
#include <asm/control.h>
#include <asm/entry.h>
#include <asm/gic.h>
#include <asm/mmio.h>
#include <asm/psci.h>
#include <asm/smccc.h>
#include <asm/sysregs.h>
#include <asm/traps.h>
#include <asm/processor.h>
#include <asm/irqchip.h>

void arch_skip_instruction(struct trap_context *ctx)
{
	u64 pc;

	arm_read_sysreg(ELR_EL2, pc);
	pc += ESR_IL(ctx->esr) ? 4 : 2;
	arm_write_sysreg(ELR_EL2, pc);
}

static enum trap_return handle_hvc(struct trap_context *ctx)
{
	unsigned long *regs = ctx->regs;
	unsigned long code = regs[0];

	if (ESR_ISS(ctx->esr) != JAILHOUSE_HVC_CODE)
		return TRAP_FORBIDDEN;

	regs[0] = hypercall(code, regs[1], regs[2]);

	if (code == JAILHOUSE_HC_DISABLE && regs[0] == 0) {
		paging_map_all_per_cpu(this_cpu_id(), true);
		arch_shutdown_self(per_cpu(this_cpu_id()));
	}

	return TRAP_HANDLED;
}

static enum trap_return handle_sysreg(struct trap_context *ctx)
{
	u32 esr = ctx->esr;
	u32 rt  = (esr >> 5) & 0x1f;

	/* All handled registers are write-only. */
	if (esr & 1)
		return TRAP_UNHANDLED;

	if (ESR_MATCH_MSR_MRS(esr, 3, 0, 12, 11, 5) && /* ICC_SGI1R */
	    gicv3_handle_sgir_write(rt == 31 ? 0 : ctx->regs[rt])) {
		arch_skip_instruction(ctx);
		return TRAP_HANDLED;
	}

	return TRAP_UNHANDLED;
}

static enum trap_return handle_iabt(struct trap_context *ctx)
{
	unsigned long hpfar, hdfar;

	if (this_cpu_data()->sdei_event) {
		this_cpu_data()->sdei_event = false;
		arm_write_sysreg(VTCR_EL2, VTCR_CELL);
		isb();

		arch_handle_sgi(SGI_EVENT, 1);

		return TRAP_HANDLED;
	}

	arm_read_sysreg(HPFAR_EL2, hpfar);
	arm_read_sysreg(FAR_EL2, hdfar);

	panic_printk("FATAL: instruction abort at 0x%lx\n",
		     (hpfar << 8) | (hdfar & 0xfff));
	return TRAP_FORBIDDEN;
}

static void dump_regs(struct trap_context *ctx)
{
	unsigned char i;
	u64 pc;

	arm_read_sysreg(ELR_EL2, pc);
	panic_printk(" pc: %016llx   lr: %016lx spsr: %08llx     EL%lld\n"
		     " sp: %016llx  elr: %016llx  esr: %02llx %01llx %07llx\n",
		     pc, ctx->regs[30], ctx->spsr, SPSR_EL(ctx->spsr),
		     ctx->sp, ctx->elr, ESR_EC(ctx->esr), ESR_IL(ctx->esr),
		     ESR_ISS(ctx->esr));
	for (i = 0; i < NUM_USR_REGS - 1; i++)
		panic_printk("%sx%d: %016lx%s", i < 10 ? " " : "", i,
			     ctx->regs[i], i % 3 == 2 ? "\n" : "  ");
	panic_printk("\n");
}

/* TODO: move this function to an arch-independent code if other architectures
 * will need it.
 */
static void dump_mem(unsigned long start, unsigned long stop)
{
	unsigned long caddr = start & ~0x1f;

	if (stop <= start)
		return;
	printk("(0x%016lx - 0x%016lx):", start, stop);
	for (;;) {
		printk("\n%04lx: ", caddr & 0xffe0);
		do {
			if (caddr >= start)
				printk("%08x ", *(unsigned int *)caddr);
			else
				printk("         ");
			caddr += 4;
		} while ((caddr & 0x1f) && caddr < stop);
		if (caddr >= stop)
			break;
	}
	printk("\n");
}

static void dump_hyp_stack(const struct trap_context *ctx)
{
	panic_printk("Hypervisor stack before exception ");
	dump_mem(ctx->sp, (unsigned long)this_cpu_data()->stack +
					sizeof(this_cpu_data()->stack));
}

static void fill_trap_context(struct trap_context *ctx, union registers *regs)
{
	arm_read_sysreg(SPSR_EL2, ctx->spsr);
	switch (SPSR_EL(ctx->spsr)) {	/* exception level */
	case 0:
		arm_read_sysreg(SP_EL0, ctx->sp); break;
	case 1:
		arm_read_sysreg(SP_EL1, ctx->sp); break;
	case 2:
		/* SP_EL2 is not accessible in EL2. To obtain SP value before
		 * the excepton we can use the regs parameter. regs is located
		 * on the stack (see handle_vmexit in exception.S) */
		ctx->sp = (u64)(regs) + 16 * 16; break;
	default:
		ctx->sp = 0; break;	/* should never happen */
	}
	arm_read_sysreg(ESR_EL2, ctx->esr);
	arm_read_sysreg(ELR_EL2, ctx->elr);
	ctx->regs = regs->usr;
}

static enum trap_return arch_handle_smc(struct trap_context *ctx)
{
	unsigned long *regs = ctx->regs;

	if (SMCCC_IS_CONV_64(regs[0]))
		return TRAP_FORBIDDEN;

	if (IS_PSCI_UBOOT(regs[0])) {
		regs[0] = psci_dispatch(ctx);
		arch_skip_instruction(ctx);
		return TRAP_HANDLED;
	}

	return handle_smc(ctx);
}

static const trap_handler trap_handlers[0x40] =
{
	[ESR_EC_HVC64]		= handle_hvc,
	[ESR_EC_SMC32]		= arch_handle_smc,
	[ESR_EC_SMC64]		= handle_smc,
	[ESR_EC_SYS64]		= handle_sysreg,
	[ESR_EC_IABT_LOW]	= handle_iabt,
	[ESR_EC_DABT_LOW]	= arch_handle_dabt,
};

void arch_handle_trap(union registers *guest_regs)
{
	struct trap_context ctx;
	trap_handler handler;
	int ret = TRAP_UNHANDLED;

	fill_trap_context(&ctx, guest_regs);

	handler = trap_handlers[ESR_EC(ctx.esr)];
	if (handler)
		ret = handler(&ctx);

	if (ret == TRAP_UNHANDLED || ret == TRAP_FORBIDDEN) {
		panic_printk("\nFATAL: %s (exception class 0x%02llx)\n",
			     (ret == TRAP_UNHANDLED ? "unhandled trap" :
						      "forbidden access"),
			     ESR_EC(ctx.esr));
		panic_printk("Cell state before exception:\n");
		dump_regs(&ctx);
		panic_park();
	}
}

void arch_el2_abt(union registers *regs)
{
	struct trap_context ctx;

	fill_trap_context(&ctx, regs);
	panic_printk("\nFATAL: Unhandled HYP exception: "
		     "synchronous abort from EL2\n");
	dump_regs(&ctx);
	dump_hyp_stack(&ctx);
	panic_stop();
}
