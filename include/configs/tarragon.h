/*
 * Copyright (C) 2017-2018 I2SE GmbH
 *
 * Configuration settings for the I2SE Tarragon board.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __CONFIGS_TARRAGON_H__
#define __CONFIGS_TARRAGON_H__

#include <asm/arch/imx-regs.h>
#include <linux/sizes.h>
#include "mx6_common.h"
#include <asm/imx-common/gpio.h>

/* uncomment for PLUGIN mode support */
/* #define CONFIG_USE_PLUGIN */

#define CONFIG_SECURE_BOOT

#ifdef CONFIG_SECURE_BOOT
#ifndef CONFIG_CSF_SIZE
#define CONFIG_CSF_SIZE 0x4000
#endif
#endif

#define PHYS_SDRAM_SIZE			SZ_512M

/* required for vendor U-Boot to compile successfully */
#undef CONFIG_LDO_BYPASS_CHECK

/* SPL options */
/* We default not support SPL
 * #define CONFIG_SPL_LIBCOMMON_SUPPORT
 * #define CONFIG_SPL_MMC_SUPPORT
 * #include "imx6_spl.h"
*/

#define CONFIG_DISPLAY_CPUINFO
#define CONFIG_DISPLAY_BOARDINFO

/* Size of malloc() pool */
#define CONFIG_SYS_MALLOC_LEN		(16 * SZ_1M)

#define CONFIG_BOARD_EARLY_INIT_F
#define CONFIG_BOARD_LATE_INIT

#define CONFIG_MXC_UART
#define CONFIG_MXC_UART_BASE		UART4_BASE

/* MMC Configs */
#ifdef CONFIG_FSL_USDHC
#define CONFIG_SYS_FSL_ESDHC_ADDR	USDHC2_BASE_ADDR
#define CONFIG_SYS_FSL_USDHC_NUM	1
#define TARRAGON_MMC			0

#define ENV_FDT_FILE			"fdt_file=imx6ull-tarragon-master.dtb\0"

#endif

#define CONFIG_MFG_ENV_SETTINGS \
	"mfgtool_args=setenv bootargs console=${console},${baudrate} " \
		"rdinit=/linuxrc " \
		"g_mass_storage.stall=0 g_mass_storage.removable=1 " \
		"g_mass_storage.file=/fat g_mass_storage.ro=1 " \
		"g_mass_storage.idVendor=0x066F g_mass_storage.idProduct=0x37FF " \
		"g_mass_storage.iSerialNumber=\"\" "\
		"qcaspi.qcaspi_pluggable=1 "\
		"clk_ignore_unused "\
		"\0" \
	"initrd_addr=0x83800000\0" \
	"initrd_high=0xffffffff\0" \
	"bootcmd_mfg=run mfgtool_args;bootz ${loadaddr} ${initrd_addr} ${fdt_addr};\0" \

#define CONFIG_EXTRA_ENV_SETTINGS \
	"update_fw_filename_prefix=emmc.img.\0" \
	"update_fw_filename_suffix=.gz\0" \
	"update_fw_parts=0x6\0" \
	"update_fw_fsize_uncompressed=4000000\0" \
	"gzwrite_wbuf=100000\0" \
	"update_emmc_firmware=" \
		"setexpr i ${update_fw_parts}; setexpr error 0; " \
		"while itest ${i} -gt 0; do " \
			"echo Transferring firmware image part ${i} of ${update_fw_parts}; " \
			"if itest ${i} -le f; then " \
				"setenv j 0${i}; " \
			"else " \
				"setenv j ${i}; " \
			"fi; " \
			"if tftp ${loadaddr} ${update_fw_basedir}${update_fw_filename_prefix}${j}${update_fw_filename_suffix}; then " \
				"setexpr k ${i} - 1; " \
				"setexpr offset ${update_fw_fsize_uncompressed} * ${k}; " \
				"if gzwrite mmc ${mmcdev} ${loadaddr} ${filesize} ${gzwrite_wbuf} ${offset}; then " \
					"setexpr i ${i} - 1; " \
				"else " \
					"setexpr i 0; " \
					"setexpr error 1; " \
				"fi; " \
			"else " \
				"setexpr i 0; " \
				"setexpr error 1; " \
			"fi; " \
		"done; setenv i; setenv j; setenv k; setenv fsize; setenv filesize; setenv offset; " \
		"if test ${error} -eq 1; then " \
			"echo Firmware Update FAILED; " \
		"else " \
			"echo Firmware Update OK; " \
		"fi; setenv error\0" \
	CONFIG_MFG_ENV_SETTINGS \
	"image=zImage\0" \
	"console=ttymxc3\0" \
	"fdt_high=0xffffffff\0" \
	"initrd_high=0xffffffff\0" \
	ENV_FDT_FILE \
	"fdt_addr=0x83000000\0" \
	"ip_dyn=yes\0" \
	"BOOT_ORDER=A B\0" \
	"BOOT_A_LEFT=3\0" \
	"BOOT_B_LEFT=3\0" \
	"mmcdev=0\0" \
	"mmcpart=1\0" \
	"mmcroot=/dev/mmcblk1p1\0" \
	"mmcargs=setenv bootargs ${bootargs} console=${console},${baudrate} panic=1\0" \
	"common_mmcargs=rootfstype=ext4 rootwait\0" \
	"loadimage=ext4load mmc ${mmcdev}:${mmcpart} ${loadaddr} /boot/${image}\0" \
	"loadfdt=ext4load mmc ${mmcdev}:${mmcpart} ${fdt_addr} /boot/${fdt_file}\0" \
	"mmcboot=echo Booting from mmc ...; " \
		"test -n \"${BOOT_ORDER}\" || setenv BOOT_ORDER \"A B\"; " \
		"test -n \"${BOOT_A_LEFT}\" || setenv BOOT_A_LEFT 3; " \
		"test -n \"${BOOT_B_LEFT}\" || setenv BOOT_B_LEFT 3; " \
		"setenv bootargs; " \
		"for BOOT_SLOT in \"${BOOT_ORDER}\"; do " \
			"if test \"x${bootargs}\" != x; then " \
				"; " \
			"elif test x${BOOT_SLOT} = xA; then " \
				"if test ${BOOT_A_LEFT} -gt 0; then " \
					"setexpr BOOT_A_LEFT ${BOOT_A_LEFT} - 1; " \
					"echo \"Found valid slot A, ${BOOT_A_LEFT} attempts remaining\"; " \
					"setenv mmcpart 1; " \
					"setenv mmcroot /dev/mmcblk1p${mmcpart}; " \
					"setenv bootargs ${common_mmcargs} root=${mmcroot} rauc.slot=A; " \
				"fi; " \
			"elif test \"x${BOOT_SLOT}\" = xB; then " \
				"if test ${BOOT_B_LEFT} -gt 0; then " \
					"setexpr BOOT_B_LEFT ${BOOT_B_LEFT} - 1; " \
					"echo \"Found valid slot B, ${BOOT_B_LEFT} attempts remaining\"; " \
					"setenv mmcpart 2; " \
					"setenv mmcroot /dev/mmcblk1p${mmcpart}; " \
					"setenv bootargs ${common_mmcargs} root=${mmcroot} rauc.slot=B; " \
				"fi; " \
			"fi; " \
		"done; " \
		"if test -n \"${bootargs}\"; then " \
			"saveenv; " \
		"else " \
			"echo \"No valid slot found, resetting tries to 3\"; " \
			"setenv BOOT_A_LEFT 3; " \
			"setenv BOOT_B_LEFT 3; " \
			"saveenv; " \
			"reset; " \
		"fi; " \
		"run mmcargs; " \
		"run loadimage; run loadfdt; " \
		"bootz ${loadaddr} - ${fdt_addr}\0" \
	"erase_mmc=mmc erase 0 800\0" \
	"erase_env=mmc erase 800 1000\0" \
	"erase_env1=mmc erase 800 800\0" \
	"erase_env2=mmc erase 1000 800\0" \
	"nfsroot=/\0" \
	"netargs=setenv bootargs console=${console},${baudrate} " \
		"root=/dev/nfs " \
		"ip=dhcp nfsroot=${serverip}:${nfsroot},v3,tcp\0" \
	"netboot=echo Booting from net ...; " \
		"run netargs; " \
		"if test ${ip_dyn} = yes; then " \
			"setenv get_cmd dhcp; " \
		"else " \
			"setenv get_cmd tftp; " \
		"fi; " \
		"${get_cmd} ${image}; " \
		"${get_cmd} ${fdt_addr} ${fdt_file}; " \
		"bootz ${loadaddr} - ${fdt_addr}\0"

#define CONFIG_BOOTCOMMAND \
	"mmc dev ${mmcdev}; " \
	"if mmc rescan; then " \
		"run mmcboot; " \
	"else " \
		"run netboot; " \
	"fi"

/* Miscellaneous configurable options */
#define CONFIG_OF_BOARD_SETUP
#define CONFIG_CMD_MEMTEST
#define CONFIG_SYS_MEMTEST_START	0x80000000
#define CONFIG_SYS_MEMTEST_END		(CONFIG_SYS_MEMTEST_START + 0x8000000)

#define CONFIG_SYS_LOAD_ADDR		CONFIG_LOADADDR
#define CONFIG_SYS_HZ			1000

#define CONFIG_STACKSIZE		SZ_128K

/* Physical Memory Map */
#define CONFIG_NR_DRAM_BANKS		1
#define PHYS_SDRAM			MMDC0_ARB_BASE_ADDR

#define CONFIG_SYS_SDRAM_BASE		PHYS_SDRAM
#define CONFIG_SYS_INIT_RAM_ADDR	IRAM_BASE_ADDR
#define CONFIG_SYS_INIT_RAM_SIZE	IRAM_SIZE

#define CONFIG_SYS_INIT_SP_OFFSET \
	(CONFIG_SYS_INIT_RAM_SIZE - GENERATED_GBL_DATA_SIZE)
#define CONFIG_SYS_INIT_SP_ADDR \
	(CONFIG_SYS_INIT_RAM_ADDR + CONFIG_SYS_INIT_SP_OFFSET)

/* Flash and environment organization */
#define CONFIG_SYS_NO_FLASH

#define CONFIG_ENV_IS_IN_MMC
#define CONFIG_SYS_REDUNDAND_ENVIRONMENT
#define CONFIG_SYS_MMC_ENV_DEV		0
#define CONFIG_ENV_SIZE			SZ_128K
#define CONFIG_ENV_OFFSET		(1 * SZ_1M)
#define CONFIG_ENV_OFFSET_REDUND	(2 * SZ_1M)
#define CONFIG_ENV_OVERWRITE

/* USB Configs */
#define CONFIG_CMD_USB
#ifdef CONFIG_CMD_USB
#define CONFIG_USB_EHCI
#define CONFIG_USB_EHCI_MX6
#define CONFIG_USB_STORAGE
#define CONFIG_EHCI_HCD_INIT_AFTER_RESET
#define CONFIG_USB_HOST_ETHER
#define CONFIG_USB_ETHER_ASIX
#define CONFIG_MXC_USB_PORTSC  (PORT_PTS_UTMI | PORT_PTS_PTW)
#define CONFIG_MXC_USB_FLAGS   0
#define CONFIG_USB_MAX_CONTROLLER_COUNT 2
#endif

#ifdef CONFIG_CMD_NET
#define CONFIG_CMD_PING
#define CONFIG_CMD_DHCP
#define CONFIG_CMD_MII
#define CONFIG_FEC_MXC
#define CONFIG_MII
#define CONFIG_FEC_ENET_DEV		0

#define IMX_FEC_BASE			ENET_BASE_ADDR
#define CONFIG_FEC_MXC_PHYADDR		0x0
#define CONFIG_FEC_XCV_TYPE		RMII
#define CONFIG_PHY_SMSC
#define CONFIG_ETHPRIME			"FEC"

#define CONFIG_PHYLIB
#endif

#define CONFIG_IMX_THERMAL

#define CONFIG_IOMUX_LPSR

#endif
