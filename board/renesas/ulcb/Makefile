#
# board/renesas/ulcb/Makefile
#
# Copyright (C) 2017-2020 Renesas Electronics Corporation
#
# SPDX-License-Identifier: GPL-2.0+
#

ifdef CONFIG_SPL_BUILD
obj-y	:= ../rcar-common/gen3-spl.o
else
obj-y	:= ulcb.o cpld.o ../rcar-common/common.o ../rcar-common/board_detect.o
endif
