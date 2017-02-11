#
# (C) Copyright 2000, 2001, 2002
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# (C) Copyright 2008
# Guennadi Liakhovetki, DENX Software Engineering, <lg@denx.de>
#
# SPDX-License-Identifier:	GPL-2.0+
#

obj-y	:= lowlevel_init.o
obj-$(CONFIG_SAMSUNG_ONENAND)	+= onenand.o

ifdef CONFIG_SPL_BUILD
obj-y += spl_boot.o
obj-y += spl_lowlevel_init.o
hostprogs-y := tools/mksmart210spl
HOSTCFLAGS_mksmart210spl.o := -pedantic
always := $(hostprogs-y)
else
obj-y += smart210.o
endif

