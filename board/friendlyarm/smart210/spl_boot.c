/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <config.h>

#include <asm/arch/clock.h>
#include <asm/arch/clk.h>
//#include <asm/arch/dmc.h>
#include <asm/arch/periph.h>
#include <asm/arch/pinmux.h>
#include <asm/arch/power.h>
//#include <asm/arch/spl.h>
//#include <asm/arch/spi.h>

#include "common_setup.h"
//#include "clock_init.h"

DECLARE_GLOBAL_DATA_PTR;

/* Index into irom ptr table */
enum index {
	SDMMC_INDEX,
	EMMC_INDEX,
};

/* IROM Function Pointers Table */
u32 irom_ptr_table[] = {
	[SDMMC_INDEX] = 0xD0037F98,	/* iROM Function Pointer-SDMMC boot */
	[EMMC_INDEX] = 0xD0037F9C,	/* iROM Function Pointer-EMMC boot*/
	};

typedef unsigned int (*copy_sd_mmc_to_mem) (unsigned int channel, unsigned int start_block, unsigned short num_blocks, unsigned int *target, unsigned int init);

unsigned int get_irom_func(int index)
{
	return *(unsigned int *)irom_ptr_table[index];
}

enum {
	BOOT_NAND,
	BOOT_ONENAND,
	BOOT_SDMMC,
	BOOT_NOR,
	BOOT_UARTUSB
};

/* OM registers are located at 0xE000_0004 */
#define OMR_OFFSET 0x04
unsigned int get_boot_mode(void)
{
	unsigned int om = readl(samsung_get_base_pro_id() + OMR_OFFSET);
	om = om & 0x3e; // mask out OM[5:1]
	switch(om) {
	case 0x0: // 512B 4-cycle
	case 0x2: // 2KB 5-cycle
	case 0x4: // 4KB 5-cycle 8-bit ECC
	case 0x6: // 4KB 5-cycle 16-bit ECC
		return BOOT_NAND;
	case 0x8: // OneNAND Mux
		return BOOT_ONENAND;
	case 0xc: // SD/MMC
		return BOOT_SDMMC;
	case 0x14: // NOR
		return BOOT_NOR;
	case 0x10: // UART->USB
		return BOOT_UARTUSB;
	}
	return BOOT_SDMMC;
}

/*
* Copy U-Boot from mmc to RAM:
* COPY_BL2_FNPTR_ADDR: Address in iRAM, which Contains
* Pointer to API (Data transfer from mmc to ram)
*/
#define SDMMC_BASE 0xD0037488
void copy_uboot_to_ram(void)
{
	unsigned int bootmode;

	copy_sd_mmc_to_mem copy_bl2 = NULL;
	u32 start_block = 0, num_blocks = 0, channel = 0;
	u32 ch = *(volatile unsigned int *)SDMMC_BASE;
	
	bootmode = get_boot_mode();

	switch (bootmode) {
	case BOOT_SDMMC:
		if(ch == 0xEB200000) channel = 2;
		start_block = BL2_START_OFFSET;
		num_blocks = BL2_SIZE_BLOC_COUNT;
		copy_bl2 = (copy_sd_mmc_to_mem)get_irom_func(SDMMC_INDEX);
		break;

	default:
		break;
	}

	if (copy_bl2)
		copy_bl2(channel, start_block, num_blocks, (unsigned int *)CONFIG_SYS_TEXT_BASE, 0);
}

void memzero(void *s, size_t n)
{
	char *ptr = s;
	size_t i;

	for (i = 0; i < n; i++)
		*ptr++ = '\0';
}

/**
 * Set up the U-Boot global_data pointer
 *
 * This sets the address of the global data, and sets up basic values.
 *
 * @param gdp   Value to give to gd
 */
static void setup_global_data(gd_t *gdp)
{
	gd = gdp;
	memzero((void *)gd, sizeof(gd_t));
	gd->flags |= GD_FLG_RELOC;
	gd->baudrate = CONFIG_BAUDRATE;
	gd->have_console = 1;
}

void power_exit_wakeup(void)
{
}

void board_init_f(unsigned long bootflag)
{
	__aligned(8) gd_t local_gd;
	__attribute__((noreturn)) void (*uboot)(void);

	setup_global_data(&local_gd);

	if (do_lowlevel_init())
		power_exit_wakeup();

	copy_uboot_to_ram();

	/* Jump to U-Boot image */
	uboot = (void *)CONFIG_SYS_TEXT_BASE;
	(*uboot)();
	/* Never returns Here */
}

/* Place Holders */
void board_init_r(gd_t *id, ulong dest_addr)
{
	/* Function attribute is no-return */
	/* This Function never executes */
	while (1)
		;
}
