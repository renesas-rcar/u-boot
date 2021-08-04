#ifndef __ASM_ARCH_RMOBILE_H
#define __ASM_ARCH_RMOBILE_H

#if defined(CONFIG_ARCH_RMOBILE)
#if defined(CONFIG_SH73A0)
#include <asm/arch/sh73a0.h>
#elif defined(CONFIG_R8A7740)
#include <asm/arch/r8a7740.h>
#elif defined(CONFIG_R8A7790)
#include <asm/arch/r8a7790.h>
#elif defined(CONFIG_R8A7791)
#include <asm/arch/r8a7791.h>
#elif defined(CONFIG_R8A7792)
#include <asm/arch/r8a7792.h>
#elif defined(CONFIG_R8A7793)
#include <asm/arch/r8a7793.h>
#elif defined(CONFIG_R8A7794)
#include <asm/arch/r8a7794.h>
#elif defined(CONFIG_RCAR_GEN3)
#include <asm/arch/rcar-gen3-base.h>
#elif defined(CONFIG_RCAR_GEN4)
#include <asm/arch/rcar-gen4-base.h>
#elif defined(CONFIG_R7S72100)
#else
#error "SOC Name not defined"
#endif
#endif /* CONFIG_ARCH_RMOBILE */

/* PRR CPU IDs */
#define RMOBILE_CPU_TYPE_SH73A0		0x37
#define RMOBILE_CPU_TYPE_R8A7740	0x40
#define RMOBILE_CPU_TYPE_R8A7790	0x45
#define RMOBILE_CPU_TYPE_R8A7791	0x47
#define RMOBILE_CPU_TYPE_R8A7792	0x4A
#define RMOBILE_CPU_TYPE_R8A7793	0x4B
#define RMOBILE_CPU_TYPE_R8A7794	0x4C
#define RMOBILE_CPU_TYPE_R8A7795	0x4F
#define RMOBILE_CPU_TYPE_R8A7796	0x52
#define RMOBILE_CPU_TYPE_R8A77965	0x55
#define RMOBILE_CPU_TYPE_R8A77970	0x54
#define RMOBILE_CPU_TYPE_R8A77980	0x56
#define RMOBILE_CPU_TYPE_R8A77990	0x57
#define RMOBILE_CPU_TYPE_R8A77995	0x58
#define RMOBILE_CPU_TYPE_R8A779A0	0x59

/* EEPROM BOARD IDs */
#define BOARD_TYPE_SALVATOR_X		0x00
#define BOARD_TYPE_STARTER_KIT		0x02
#define BOARD_TYPE_EAGLE		0x03
#define BOARD_TYPE_SALVATOR_XS		0x04
#define BOARD_TYPE_DRAAK		0x07
#define BOARD_TYPE_CONDOR		0x06
#define BOARD_TYPE_EBISU		0x08
#define BOARD_TYPE_STARTER_KIT_PRE	0x0B
#define BOARD_TYPE_EBISU_4D		0x0D
#define BOARD_TYPE_CONDOR_I		0x10
#define BOARD_TYPE_UNKNOWN		0x1F

#ifndef __ASSEMBLY__
u32 rmobile_get_cpu_type(void);
u32 rmobile_get_cpu_rev_integer(void);
u32 rmobile_get_cpu_rev_fraction(void);
int rcar_get_board_type(int busnum, int chip_addr, u32 *type);
bool is_rcar_gen3_board(const char *board_name);
#endif /* __ASSEMBLY__ */

#endif /* __ASM_ARCH_RMOBILE_H */
