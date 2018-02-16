#
# arch/arm/cpu/armv8/rcar_gen3/Makefile
# 	This file defines board-related files for make.
#
# Copyright (C) 2015-2018 Renesas Electoronics Corporation
#
# SPDX-License-Identifier:	GPL-2.0+
#

obj-y += cpu_info.o

obj-$(CONFIG_R8A7795) += lowlevel_init.o cpu_info-r8a7795.o \
				pfc.o pfc-r8a7795.o pfc-r8a7795_es.o prr_depend.o \
				board.o
obj-$(CONFIG_R8A7796X) += lowlevel_init.o cpu_info-r8a7796.o \
				pfc.o pfc-r8a7796.o prr_depend.o \
				board.o
obj-$(CONFIG_R8A77990) += lowlevel_init.o cpu_info-r8a77990.o \
				pfc.o pfc-r8a77990.o prr_depend.o \
				board.o
obj-$(CONFIG_R8A77995) += lowlevel_init.o cpu_info-r8a77995.o \
				pfc.o pfc-r8a77995.o prr_depend.o \
				board.o
