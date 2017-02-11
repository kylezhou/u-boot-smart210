/*
 * Lowlevel setup for EXYNOS5 based board
 *
 * Copyright (C) 2013 Samsung Electronics
 * Rajeshwari Shinde <rajeshwari.s@samsung.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <config.h>
#include <debug_uart.h>
#include <asm/arch/cpu.h>
#include <asm/arch/dmc.h>
#include <asm/arch/power.h>
#include <asm/arch/clock.h>
#include <asm/arch/periph.h>
#include <asm/arch/pinmux.h>
//#include <asm/arch/system.h>
#include <asm/armv7.h>
#include "common_setup.h"
//#include "exynos5_setup.h"

void system_clock_init(void)
{	
	u32 val = 0;
	
	struct s5pc110_clock *const clock = (struct s5pc110_clock *)samsung_get_base_clock();
	
	/* 1.设置PLL锁定值 */
	writel(0xFFFF, &clock->apll_lock);
	writel(0xFFFF, &clock->mpll_lock);
	writel(0xFFFF, &clock->epll_lock);
	writel(0xFFFF, &clock->vpll_lock);
	
	/* 2.设置PLL的PMS值(使用芯片手册推荐的值)，并使能PLL */
	/*	 	P	  	    M   		  S		     EN	*/
	writel((3  << 8) | (125 << 16) | (1 << 0) | (1 << 31), &clock->apll_con);	/* FOUT_APLL = 1000MHz */
	writel((12 << 8) | (667 << 16) | (1 << 0) | (1 << 31), &clock->mpll_con); 	/* FOUT_MPLL = 667MHz */
	writel((3  << 8) | (48  << 16) | (2 << 0) | (1 << 31), &clock->epll_con);	/* FOUT_EPLL = 96MHz */
	writel((6  << 8) | (108 << 16) | (3 << 0) | (1 << 31), &clock->vpll_con);	/* FOUT_VPLL = 54MHz */
	
	/* 3.等待PLL锁定 */
	while (!(readl(&clock->apll_con) & (1 << 29)));
	while (!(readl(&clock->mpll_con) & (1 << 29)));
	while (!(readl(&clock->apll_con) & (1 << 29)));
	while (!(readl(&clock->epll_con) & (1 << 29)));
	while (!(readl(&clock->vpll_con) & (1 << 29)));

	/* 
	** 4.设置系统时钟源，选择PLL为时钟输出 */
	/* MOUT_MSYS = SCLKAPLL = FOUT_APLL = 1000MHz
	** MOUT_DSYS = SCLKMPLL = FOUT_MPLL = 667MHz
	** MOUT_PSYS = SCLKMPLL = FOUT_MPLL = 667MHz
	** ONENAND = HCLK_PSYS
	*/
	writel((1 << 0) | (1 << 4) | (1 << 8) | (1 << 12), &clock->src0);
	
	/* 4.设置其他模块的时钟源 */

	/* 6.设置系统时钟分频值 */
	val = 	(0 << 0)  |	/* APLL_RATIO = 0, freq(ARMCLK) = MOUT_MSYS / (APLL_RATIO + 1) = 1000MHz */
			(4 << 4)  |	/* A2M_RATIO = 4, freq(A2M) = SCLKAPLL / (A2M_RATIO + 1) = 200MHz */
			(4 << 8)  |	/* HCLK_MSYS_RATIO = 4, freq(HCLK_MSYS) = ARMCLK / (HCLK_MSYS_RATIO + 1) = 200MHz */
			(1 << 12) |	/* PCLK_MSYS_RATIO = 1, freq(PCLK_MSYS) = HCLK_MSYS / (PCLK_MSYS_RATIO + 1) = 100MHz */
			(3 << 16) | /* HCLK_DSYS_RATIO = 3, freq(HCLK_DSYS) = MOUT_DSYS / (HCLK_DSYS_RATIO + 1) = 166MHz */
			(1 << 20) | /* PCLK_DSYS_RATIO = 1, freq(PCLK_DSYS) = HCLK_DSYS / (PCLK_DSYS_RATIO + 1) = 83MHz */
			(4 << 24) |	/* HCLK_PSYS_RATIO = 4, freq(HCLK_PSYS) = MOUT_PSYS / (HCLK_PSYS_RATIO + 1) = 133MHz */
			(1 << 28);	/* PCLK_PSYS_RATIO = 1, freq(PCLK_PSYS) = HCLK_PSYS / (PCLK_PSYS_RATIO + 1) = 66MHz */
	writel(val, &clock->div0);
	
	/* 7.设置其他模块的时钟分频值 */
}

void mem_ctrl_init(void)
{
	struct s5pc110_dmc *const dmc0 = (struct s5pc110_dmc *)S5PC110_DMC0_BASE;
	struct s5pc110_dmc *const dmc1 = (struct s5pc110_dmc *)S5PC110_DMC1_BASE;
	
	/* DMC0 */
	writel(0x00101000, &dmc0->phycontrol0);
	writel(0x00101002, &dmc0->phycontrol0);			/* DLL on */
	writel(0x00000086, &dmc0->phycontrol1);
	writel(0x00101003, &dmc0->phycontrol0);			/* DLL start */
	
	while ((readl(&dmc0->phystatus) & 0x7) != 0x7); /* wait DLL locked */

	writel(0x0FFF2350, &dmc0->concontrol);			/* Auto Refresh Counter should be off */
	writel(0x00202430, &dmc0->memcontrol);			/* Dynamic power down should be off */
	writel(0x20E01323, &dmc0->memconfig0);
	
	writel(0xFF000000, &dmc0->prechconfig);
	writel(0xFFFF00FF, &dmc0->pwrdnconfig);
	
	writel(0x00000618, &dmc0->timingaref);			/* 7.8us * 200MHz = 1560 = 0x618  */
	writel(0x19233309, &dmc0->timingrow);
	writel(0x23240204, &dmc0->timingdata);
	writel(0x09C80232, &dmc0->timingpower);
	
	writel(0x07000000, &dmc0->directcmd);			/* NOP */
	writel(0x01000000, &dmc0->directcmd);			/* PALL */
	writel(0x00020000, &dmc0->directcmd);			/* EMRS2 */
	writel(0x00030000, &dmc0->directcmd);			/* EMRS3 */
	writel(0x00010400, &dmc0->directcmd);			/* EMRS enable DLL */
	writel(0x00000542, &dmc0->directcmd);			/* DLL reset */
	writel(0x01000000, &dmc0->directcmd); 			/* PALL */
	writel(0x05000000, &dmc0->directcmd);			/* auto refresh */
	writel(0x05000000, &dmc0->directcmd);			/* auto refresh */
	writel(0x00000442, &dmc0->directcmd);			/* DLL unreset */
	writel(0x00010780, &dmc0->directcmd);			/* OCD default */
	writel(0x00010400, &dmc0->directcmd);			/* OCD exit */
	
	writel(0x0FF02030, &dmc0->concontrol);			/* auto refresh on */
	writel(0xFFFF00FF, &dmc0->pwrdnconfig);
	writel(0x00202400, &dmc0->memcontrol);

	/* DMC1 */
	writel(0x00101000, &dmc1->phycontrol0);
	writel(0x00101002, &dmc1->phycontrol0);			/* DLL on */
	writel(0x00000086, &dmc1->phycontrol1);
	writel(0x00101003, &dmc1->phycontrol0);			/* DLL start */

	while ((readl(&dmc1->phystatus) & 0x7) != 0x7); /* wait DLL locked */

	writel(0x0FFF2350, &dmc1->concontrol);			/* Auto Refresh Counter should be off */
	writel(0x00202430, &dmc1->memcontrol);			/* Dynamic power down should be off */
	writel(0x40E01323, &dmc1->memconfig0);
	
	writel(0xFF000000, &dmc1->prechconfig);
	writel(0xFFFF00FF, &dmc1->pwrdnconfig);
	
	writel(0x00000618, &dmc1->timingaref);			/* 7.8us * 200MHz = 1560 = 0x618  */
	writel(0x19233309, &dmc1->timingrow);
	writel(0x23240204, &dmc1->timingdata);
	writel(0x09C80232, &dmc1->timingpower);
	
	writel(0x07000000, &dmc1->directcmd);			/* NOP */
	writel(0x01000000, &dmc1->directcmd);			/* PALL */
	writel(0x00020000, &dmc1->directcmd);			/* EMRS2 */
	writel(0x00030000, &dmc1->directcmd);			/* EMRS3 */
	writel(0x00010400, &dmc1->directcmd);			/* EMRS enable DLL */
	writel(0x00000542, &dmc1->directcmd);			/* DLL reset */
	writel(0x01000000, &dmc1->directcmd); 			/* PALL */
	writel(0x05000000, &dmc1->directcmd);			/* auto refresh */
	writel(0x05000000, &dmc1->directcmd);			/* auto refresh */
	writel(0x00000442, &dmc1->directcmd);			/* DLL unreset */
	writel(0x00010780, &dmc1->directcmd);			/* OCD default */
	writel(0x00010400, &dmc1->directcmd);			/* OCD exit */
	
	writel(0x0FF02030, &dmc1->concontrol);			/* auto refresh on */
	writel(0xFFFF00FF, &dmc1->pwrdnconfig);
	writel(0x00202400, &dmc1->memcontrol);
}

void do_lowlevel_init(void)
{
	arch_cpu_init();

	system_clock_init();

	mem_ctrl_init();

}
