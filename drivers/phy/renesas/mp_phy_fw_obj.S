/* SPDX-License-Identifier: GPL-2.0
 *
 * MP-PHY firmware blob linked in at build time.
 */

.section .rodata

.global mp_phy_fw_obj
.type   mp_phy_fw_obj, @object
.global mp_phy_fw_obj_size
.type   mp_phy_fw_obj_size, @object

mp_phy_fw_obj:
.align 4
.incbin CONFIG_PHY_R8A78000_MP_PHY_FW

mp_phy_fw_obj_end:

mp_phy_fw_obj_size:
.int mp_phy_fw_obj_end - mp_phy_fw_obj
