/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Test configuration for Orange Pi Zero (H2+ quad-core Cortex-A7, 256MB RAM)
 *
 * Copyright (c) Siemens AG, 2014-2016
 *
 * Authors:
 *  Jan Kiszka <jan.kiszka@siemens.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <jailhouse/types.h>
#include <jailhouse/cell-config.h>

struct {
	struct jailhouse_system header;
	__u64 cpus[1];
	struct jailhouse_memory mem_regions[18];
	struct jailhouse_irqchip irqchips[1];
	struct jailhouse_pci_device pci_devices[0];
} __attribute__((packed)) config = {
	.header = {
		.signature = JAILHOUSE_SYSTEM_SIGNATURE,
		.revision = JAILHOUSE_CONFIG_REVISION,
		.architecture = JAILHOUSE_ARM,
		.flags = JAILHOUSE_SYS_VIRTUAL_DEBUG_CONSOLE,
		.hypervisor_memory = {
			.phys_start = 0x48000000,
			.size = 0x800000 - 0x100000, /* -1MB (PSCI) */
		},
		.debug_console = {
			.address = 0x01c28000,
			.size = 0x1000,
			.type = JAILHOUSE_CON_TYPE_8250,
			.flags = JAILHOUSE_CON_ACCESS_MMIO |
				 JAILHOUSE_CON_REGDIST_4,
		},
		.platform_info = {
			.pci_mmconfig_base = 0x2000000,
			.pci_mmconfig_end_bus = 0,
			.pci_is_virtual = 1,
			.arm = {
				.gic_version = 2,
				.gicd_base = 0x01c81000,
				.gicc_base = 0x01c82000,
				.gich_base = 0x01c84000,
				.gicv_base = 0x01c86000,
				.maintenance_irq = 25,
			},
		},
		.root_cell = {
			.name = "Orange-Pi0",

			.cpu_set_size = sizeof(config.cpus),
			.num_memory_regions = ARRAY_SIZE(config.mem_regions),
			.num_irqchips = ARRAY_SIZE(config.irqchips),
			.num_pci_devices = ARRAY_SIZE(config.pci_devices),

			.vpci_irq_base = 108,
		},
	},

	.cpus = {
		0xf,
	},

	.mem_regions = {
		/* IVSHMEM shared memory regions (demo) */
		{
			.phys_start = 0x488f0000,
			.virt_start = 0x488f0000,
			.size = 0x1000,
			.flags = JAILHOUSE_MEM_READ,
		},
		{
			.phys_start = 0x488f1000,
			.virt_start = 0x488f1000,
			.size = 0x9000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE,
		},
		{
			.phys_start = 0x488fa000,
			.virt_start = 0x488fa000,
			.size = 0x2000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE,
		},
		{
			.phys_start = 0x488fc000,
			.virt_start = 0x488fc000,
			.size = 0x4000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE,
		},
		/* IVSHMEM shared memory region */
		JAILHOUSE_SHMEM_NET_REGIONS(0x48900000, 0),
		/* MMIO 1 (permissive) */ {
			.phys_start = 0x01c00000,
			.virt_start = 0x01c00000,
			.size =          0x20000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO,
		},
		/* clock (permissive) */ {
			.phys_start = 0x01c20000,
			.virt_start = 0x01c20000,
			.size =            0x800,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO | JAILHOUSE_MEM_IO_32,
		},
		/* pinctrl PA */ {
			.phys_start = 0x01c20800,
			.virt_start = 0x01c20800,
			.size =             0x24,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO | JAILHOUSE_MEM_IO_32,
		},
		/* pinctrl rest */ {
			.phys_start = 0x01c20824,
			.virt_start = 0x01c20824,
			.size =            0x3dc,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO | JAILHOUSE_MEM_IO_32,
		},
		/* watchdog */ {
			.phys_start = 0x01c20ca0,
			.virt_start = 0x01c20ca0,
			.size =             0x20,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO | JAILHOUSE_MEM_IO_32,
		},
		/* MMIO 2 (permissive) */ {
			.phys_start = 0x01c28000,
			.virt_start = 0x01c28000,
			.size =           0x9000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO,
		},
		/* RTC */ {
			.phys_start = 0x01f00000,
			.virt_start = 0x01f00000,
			.size =            0x400,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO | JAILHOUSE_MEM_IO_32,
		},
		/* MMIO 3 (permissive) */ {
			.phys_start = 0x01f01000,
			.virt_start = 0x01f01000,
			.size =           0x3000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO,
		},
		/* RAM */ {
			.phys_start = 0x40000000,
			.virt_start = 0x40000000,
			.size =        0x8000000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE,
		},
		/* RAM */ {
			.phys_start = 0x48800000,
			.virt_start = 0x48800000,
			.size =       0x77800000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE,
		},
	},

	.irqchips = {
		/* GIC */ {
			.address = 0x01c81000,
			.pin_base = 32,
			.pin_bitmap = {
				0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff
			},
		},
	},
};
