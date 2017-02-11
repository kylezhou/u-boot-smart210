/*
 * (C) Copyright 2009 Samsung Electronics
 * Minkyu Kang <mk7.kang@samsung.com>
 * Heungjun Kim <riverful.kim@samsung.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __ASM_ARM_ARCH_DMC_H_
#define __ASM_ARM_ARCH_DMC_H_

#ifndef __ASSEMBLY__
struct s5pc100_dmc {
	unsigned int	concontrol;
	unsigned int	memcontrol;
	unsigned int	memconfig0;
	unsigned int	memconfig1;
	unsigned int	directcmd;
	unsigned int	prechconfig;
	unsigned int	phycontrol0;
	unsigned int	phycontrol1;
	unsigned int	phycontrol2;
	unsigned char	res1[0x04];
	unsigned int	pwrdnconfig;
	unsigned char	res2[0x04];
	unsigned int	timingaref;
	unsigned int	timingrow;
	unsigned int	timingdata;
	unsigned int	timingpower;
	unsigned int	phystatus0;
	unsigned int	phystatus1;
	unsigned int	chip0status;
	unsigned int	chip1status;
	unsigned int	arefstatus;
	unsigned int	mrstatus;
	unsigned int	phytest0;
	unsigned int	phytest1;
	/* QoS registers are ignored here */
};

struct s5pc110_dmc {
	unsigned int	concontrol;
	unsigned int	memcontrol;
	unsigned int	memconfig0;
	unsigned int	memconfig1;
	unsigned int	directcmd;
	unsigned int	prechconfig;
	unsigned int	phycontrol0;
	unsigned int	phycontrol1;
	unsigned char	res1[0x08];
	unsigned int	pwrdnconfig;
	unsigned char	res2[0x04];
	unsigned int	timingaref;
	unsigned int	timingrow;
	unsigned int	timingdata;
	unsigned int	timingpower;
	unsigned int	phystatus;
	unsigned char	res3[0x04];
	unsigned int	chip0status;
	unsigned int	chip1status;
	unsigned int	arefstatus;
	unsigned int	mrstatus;
	unsigned int	phytest0;
	unsigned int	phytest1;
};
#endif

#endif
