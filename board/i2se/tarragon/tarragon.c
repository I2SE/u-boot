/*
 * Copyright (C) 2017-2018 I2SE GmbH
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <asm/arch/clock.h>
#include <asm/arch/iomux.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/crm_regs.h>
#include <asm/arch/mx6-pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/gpio.h>
#include <asm/imx-common/iomux-v3.h>
#include <asm/imx-common/boot_mode.h>
#include <asm/io.h>
#include <common.h>
#include <fdt_support.h>
#include <fsl_esdhc.h>
#include <fuse.h>
#include <miiphy.h>
#include <linux/sizes.h>
#include <mmc.h>
#include <netdev.h>
#include <usb.h>
#include <usb/ehci-ci.h>

DECLARE_GLOBAL_DATA_PTR;

#define UART_PAD_CTRL  (PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED |		\
	PAD_CTL_DSE_40ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define USDHC_PAD_CTRL (PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_22K_UP  | PAD_CTL_SPEED_LOW |		\
	PAD_CTL_DSE_80ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define ENET_PAD_CTRL  (PAD_CTL_PUS_100K_UP | PAD_CTL_PUE |     \
	PAD_CTL_SPEED_HIGH   |                                  \
	PAD_CTL_DSE_48ohm   | PAD_CTL_SRE_FAST)

#define MDIO_PAD_CTRL  (PAD_CTL_PUS_100K_UP | PAD_CTL_PUE |     \
	PAD_CTL_DSE_48ohm   | PAD_CTL_SRE_FAST | PAD_CTL_ODE)

#define ENET_CLK_PAD_CTRL  (PAD_CTL_DSE_40ohm   | PAD_CTL_SRE_FAST)

#define OTG_ID_PAD_CTRL (PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_47K_UP  | PAD_CTL_SPEED_LOW |		\
	PAD_CTL_DSE_80ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define SPI_PAD_CTRL (PAD_CTL_HYS |				\
	PAD_CTL_SPEED_MED |		\
	PAD_CTL_DSE_40ohm | PAD_CTL_SRE_FAST)

int dram_init(void)
{
	gd->ram_size = imx_ddr_size();

	return 0;
}

static iomux_v3_cfg_t const uart4_pads[] = {
	MX6_PAD_LCD_CLK__UART4_DTE_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX6_PAD_LCD_ENABLE__UART4_DTE_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static void setup_iomux_uart(void)
{
	imx_iomux_v3_setup_multiple_pads(uart4_pads, ARRAY_SIZE(uart4_pads));
}

#ifdef CONFIG_FSL_ESDHC

static iomux_v3_cfg_t const usdhc2_emmc_pads[] = {
	MX6_PAD_NAND_RE_B__USDHC2_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_NAND_WE_B__USDHC2_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_NAND_DATA00__USDHC2_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_NAND_DATA01__USDHC2_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_NAND_DATA02__USDHC2_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_NAND_DATA03__USDHC2_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_NAND_DATA04__USDHC2_DATA4 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_NAND_DATA05__USDHC2_DATA5 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_NAND_DATA06__USDHC2_DATA6 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_NAND_DATA07__USDHC2_DATA7 | MUX_PAD_CTRL(USDHC_PAD_CTRL),

	/* RST_B */
	MX6_PAD_NAND_ALE__GPIO4_IO10 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

/* we default to 4-bit bus width and no-1v8 to be compatible with V0R3 */
static struct fsl_esdhc_cfg usdhc2_emmc_cfg = { USDHC2_BASE_ADDR, 0, 4, 0, 0 };

#define USDHC2_RST_GPIO	IMX_GPIO_NR(4, 10)

int board_mmc_get_env_dev(int devno)
{
	return 0;
}

int board_mmc_getcd(struct mmc *mmc)
{
	/* eMMC is always present */
	return 1;
}

/*
 * According to the board_mmc_init() the following map is done:
 * (U-Boot device node)    (Physical Port)
 * mmc0                    USDHC2
 */
int board_mmc_init(bd_t *bis)
{
	int ret;

	imx_iomux_v3_setup_multiple_pads(usdhc2_emmc_pads, ARRAY_SIZE(usdhc2_emmc_pads));

	gpio_direction_output(USDHC2_RST_GPIO, 0);
	udelay(500);
	gpio_direction_output(USDHC2_RST_GPIO, 1);

	usdhc2_emmc_cfg.sdhc_clk = mxc_get_clock(MXC_ESDHC2_CLK);
	ret = fsl_esdhc_initialize(bis, &usdhc2_emmc_cfg);

#ifndef CONFIG_SPL_BUILD
	if (ret) {
		printf("Warning: failed to initialize mmc dev\n");
	}
#endif

	return ret;
}
#endif

#ifdef CONFIG_USB_EHCI_MX6
#define USB_OTHERREGS_OFFSET	0x800
#define UCTRL_PWR_POL		(1 << 9)

static iomux_v3_cfg_t const usb_otg_pads[] = {
	MX6_PAD_SD1_DATA0__ANATOP_OTG1_ID | MUX_PAD_CTRL(OTG_ID_PAD_CTRL),
};

/* At default the 3v3 enables the MIC2026 for VBUS power */
static void setup_usb(void)
{
	imx_iomux_v3_setup_multiple_pads(usb_otg_pads,
					 ARRAY_SIZE(usb_otg_pads));
}

int board_usb_phy_mode(int port)
{
	if (port == 1)
		return USB_INIT_HOST;
	else
		return usb_phy_mode(port);
}

int board_ehci_hcd_init(int port)
{
	u32 *usbnc_usb_ctrl;

	if (port > 1)
		return -EINVAL;

	usbnc_usb_ctrl = (u32 *)(USB_BASE_ADDR + USB_OTHERREGS_OFFSET +
				 port * 4);

	/* Set Power polarity */
	setbits_le32(usbnc_usb_ctrl, UCTRL_PWR_POL);

	return 0;
}
#endif

#ifdef CONFIG_FEC_MXC

#define ENET1_PHY_RST_GPIO	IMX_GPIO_NR(5, 6)

static iomux_v3_cfg_t const fec1_pads[] = {
	/* MDIO bus connects both PHYs for ENET1 and ENET2 */
	MX6_PAD_GPIO1_IO06__ENET1_MDIO | MUX_PAD_CTRL(MDIO_PAD_CTRL),
	MX6_PAD_GPIO1_IO07__ENET1_MDC | MUX_PAD_CTRL(ENET_PAD_CTRL),

	/* ENET1 RMII */
	MX6_PAD_ENET1_TX_DATA0__ENET1_TDATA00 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET1_TX_DATA1__ENET1_TDATA01 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET1_TX_CLK__ENET1_REF_CLK1 | MUX_PAD_CTRL(ENET_CLK_PAD_CTRL),
	MX6_PAD_ENET1_TX_EN__ENET1_TX_EN | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET1_RX_DATA0__ENET1_RDATA00 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET1_RX_DATA1__ENET1_RDATA01 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET1_RX_EN__ENET1_RX_EN | MUX_PAD_CTRL(ENET_PAD_CTRL),

	/* ENET1 PHY reset */
	MX6_PAD_SNVS_TAMPER6__GPIO5_IO06 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

int board_eth_init(bd_t *bis)
{
	imx_iomux_v3_setup_multiple_pads(fec1_pads, ARRAY_SIZE(fec1_pads));

	/* reset PHY */
	gpio_direction_output(ENET1_PHY_RST_GPIO, 0);
	udelay(200);
	gpio_set_value(ENET1_PHY_RST_GPIO, 1);

	/* give PHY some time to get out of the reset */
	udelay(10000);

	return fecmxc_initialize_multi(bis, 0,
				       CONFIG_FEC_MXC_PHYADDR, IMX_FEC_BASE);
}

#ifdef CONFIG_OF_BOARD_SETUP
int ft_board_setup(void *blob, bd_t *bd)
{
	uint8_t enetaddr[6];
	u32 mac = 0;

	enetaddr[0] = 0x00;
	enetaddr[1] = 0x01;
	enetaddr[2] = 0x87;

	/* Read QCA7000 MAINS MAC from OCOTP_GP1 */
	fuse_read(4, 6, &mac);

	if (mac != 0) {
		enetaddr[3] = (mac >> 16) & 0xff;
		enetaddr[4] = (mac >>  8) & 0xff;
		enetaddr[5] =  mac        & 0xff;

		fdt_find_and_setprop(blob,
		                     "spi3/ethernet@0",
		                     "local-mac-address", enetaddr, 6, 1);
	}

	/* Read QCA7000 CP MAC from OCOTP_GP2 */
	fuse_read(4, 7, &mac);

	if (mac != 0) {
		enetaddr[3] = (mac >> 16) & 0xff;
		enetaddr[4] = (mac >>  8) & 0xff;
		enetaddr[5] =  mac        & 0xff;

		fdt_find_and_setprop(blob,
		                     "spi1/ethernet@0",
		                     "local-mac-address", enetaddr, 6, 1);
	}

	return 0;
}
#endif

static int setup_fec(int fec_id)
{
	struct iomuxc *const iomuxc_regs = (struct iomuxc *)IOMUXC_BASE_ADDR;
	int ret;

	/*
	 * Use 50M anatop loopback REF_CLK1 for ENET1,
	 * clear gpr1[13], set gpr1[17].
	 */
	clrsetbits_le32(&iomuxc_regs->gpr[1], IOMUX_GPR1_FEC1_MASK,
	                IOMUX_GPR1_FEC1_CLOCK_MUX1_SEL_MASK);

	ret = enable_fec_anatop_clock(fec_id, ENET_50MHZ);
	if (ret)
		return ret;

	enable_enet_clk(1);

	return 0;
}
#endif

static iomux_v3_cfg_t const boardvariant_pads[] = {
	MX6_PAD_NAND_CLE__GPIO4_IO15 | MUX_PAD_CTRL(NO_PAD_CTRL),
	MX6_PAD_NAND_CE0_B__GPIO4_IO13 | MUX_PAD_CTRL(NO_PAD_CTRL),
	MX6_PAD_NAND_CE1_B__GPIO4_IO14 | MUX_PAD_CTRL(NO_PAD_CTRL),
	MX6_PAD_NAND_DQS__GPIO4_IO16 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

#define BOARD_VARIANT_GPIO0	IMX_GPIO_NR(4, 15)
#define BOARD_VARIANT_GPIO1	IMX_GPIO_NR(4, 13)
#define BOARD_VARIANT_GPIO2	IMX_GPIO_NR(4, 14)
#define BOARD_VARIANT_GPIO3	IMX_GPIO_NR(4, 16)

static enum board_variants {
	TARRAGON_DEFAULT,
	TARRAGON_MASTER,
	TARRAGON_SLAVE,
	TARRAGON_MICRO,
	/* insert new variants before this line */
	TARRAGON_UNKNOWN,
} board_variant = TARRAGON_UNKNOWN;

static const char * const board_variants_names[] = {
	"Default",
	"Master",
	"Slave",
	"Micro",
	/* insert new variant names before this line */
	"Unknown",
};

static const char * const board_variants_default_dtb_names[] = {
	NULL,
	"imx6ull-tarragon-master.dtb",
	"imx6ull-tarragon-slave.dtb",
	"imx6ull-tarragon-micro.dtb",
	/* insert new variant names before this line */
	NULL
};

static void boardvariants_init(void)
{
	imx_iomux_v3_setup_multiple_pads(boardvariant_pads, ARRAY_SIZE(boardvariant_pads));

	gpio_direction_input(BOARD_VARIANT_GPIO0);
	gpio_direction_input(BOARD_VARIANT_GPIO1);
	gpio_direction_input(BOARD_VARIANT_GPIO2);
	gpio_direction_input(BOARD_VARIANT_GPIO3);

	/* Reverse GPIO order, because schematics have a special assignment */
	board_variant =
		gpio_get_value(BOARD_VARIANT_GPIO3) << 0 |
		gpio_get_value(BOARD_VARIANT_GPIO2) << 1 |
		gpio_get_value(BOARD_VARIANT_GPIO1) << 2 |
		gpio_get_value(BOARD_VARIANT_GPIO0) << 3;

	/* in case hardware is populated wrong or variant is not (yet) known */
	if (board_variant > TARRAGON_UNKNOWN)
		board_variant = TARRAGON_UNKNOWN;
}

static iomux_v3_cfg_t const qca7000_cp_pads[] = {
	MX6_PAD_UART4_RX_DATA__ECSPI2_SS0 | MUX_PAD_CTRL(SPI_PAD_CTRL),
	MX6_PAD_UART4_TX_DATA__ECSPI2_SCLK | MUX_PAD_CTRL(SPI_PAD_CTRL),
	MX6_PAD_UART5_RX_DATA__ECSPI2_MISO | MUX_PAD_CTRL(SPI_PAD_CTRL),
	MX6_PAD_UART5_TX_DATA__ECSPI2_MOSI | MUX_PAD_CTRL(SPI_PAD_CTRL),

	/* RST pin */
	MX6_PAD_LCD_DATA12__GPIO3_IO17 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static iomux_v3_cfg_t const qca7000_mains_pads[] = {
	MX6_PAD_ENET2_RX_ER__ECSPI4_SS0 | MUX_PAD_CTRL(SPI_PAD_CTRL),
	MX6_PAD_ENET2_TX_DATA1__ECSPI4_SCLK | MUX_PAD_CTRL(SPI_PAD_CTRL),
	MX6_PAD_ENET2_TX_CLK__ECSPI4_MISO | MUX_PAD_CTRL(SPI_PAD_CTRL),
	MX6_PAD_ENET2_TX_EN__ECSPI4_MOSI | MUX_PAD_CTRL(SPI_PAD_CTRL),

	/* RST pin */
	MX6_PAD_SNVS_TAMPER7__GPIO5_IO07 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

#define QCA7000_CP_RST_GPIO	IMX_GPIO_NR(3, 17)
#define QCA7000_MAINS_RST_GPIO	IMX_GPIO_NR(5, 7)

static void qca7000_init(void)
{
	imx_iomux_v3_setup_multiple_pads(qca7000_cp_pads, ARRAY_SIZE(qca7000_cp_pads));
	imx_iomux_v3_setup_multiple_pads(qca7000_mains_pads, ARRAY_SIZE(qca7000_mains_pads));

	/* De-assert QCA7000 resets */
	gpio_direction_output(QCA7000_CP_RST_GPIO, 1);
	gpio_direction_output(QCA7000_MAINS_RST_GPIO, 1);
}

static iomux_v3_cfg_t const motor_1_drv_pads[] = {
	MX6_PAD_LCD_DATA02__GPIO3_IO07 | MUX_PAD_CTRL(NO_PAD_CTRL),
	MX6_PAD_LCD_DATA03__GPIO3_IO08 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

#define MOTOR_1_DRV_IN1	IMX_GPIO_NR(3, 7)
#define MOTOR_1_DRV_IN2	IMX_GPIO_NR(3, 8)

static void motor_1_drv_init(void)
{
	imx_iomux_v3_setup_multiple_pads(motor_1_drv_pads, ARRAY_SIZE(motor_1_drv_pads));

	/* start charging caps */
	gpio_direction_output(MOTOR_1_DRV_IN1, 1);
	gpio_direction_output(MOTOR_1_DRV_IN2, 0);
}

static iomux_v3_cfg_t const led_pads[] = {
	MX6_PAD_LCD_DATA09__GPIO3_IO14 | MUX_PAD_CTRL(NO_PAD_CTRL),
	MX6_PAD_LCD_DATA10__GPIO3_IO15 | MUX_PAD_CTRL(NO_PAD_CTRL),
	MX6_PAD_LCD_DATA14__GPIO3_IO19 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

#define LED_GREEN	IMX_GPIO_NR(3, 14)
#define LED_YELLOW	IMX_GPIO_NR(3, 15)
#define LED_RED		IMX_GPIO_NR(3, 19)

static void led_init(void)
{
	imx_iomux_v3_setup_multiple_pads(led_pads, ARRAY_SIZE(led_pads));

	/* enable red LED to indicate a running bootloader */
	gpio_direction_output(LED_GREEN, 0);
	gpio_direction_output(LED_YELLOW, 0);
	gpio_direction_output(LED_RED, 1);
}

int board_early_init_f(void)
{
	setup_iomux_uart();

	boardvariants_init();

	qca7000_init();

	motor_1_drv_init();

	led_init();

	return 0;
}

int board_init(void)
{
	/* Address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM + 0x100;

#ifdef	CONFIG_FEC_MXC
	setup_fec(CONFIG_FEC_ENET_DEV);
#endif

#ifdef CONFIG_USB_EHCI_MX6
	setup_usb();
#endif

	return 0;
}

int board_late_init(void)
{
	setenv("board_variant", board_variants_names[board_variant]);

	set_wdog_reset((struct wdog_regs *)WDOG1_BASE_ADDR);

	return 0;
}

int checkboard(void)
{
	printf("Board: I2SE Tarragon (%s)\n", board_variants_names[board_variant]);

	switch (get_boot_device()) {
	case SD2_BOOT:
		puts("Bootmode: SD card\n");
		break;
	case MMC2_BOOT:
		puts("Bootmode: eMMC\n");
		break;
	default:
		/* no output */
		;
	}

	return 0;
}

int misc_init_r(void)
{
	char *s;

	/* guess DT blob if not already set in environment */
	if (!getenv("fdt_file") && board_variants_default_dtb_names[board_variant] != NULL)
		setenv("fdt_file", board_variants_default_dtb_names[board_variant]);

	/* print serial number if available */
	s = getenv("serial#");
	if (s && s[0]) {
		puts("Serial: ");
		puts(s);
		puts("\n");
	}

	return 0;
}

#ifdef CONFIG_SPL_BUILD
#include <libfdt.h>
#include <spl.h>
#include <asm/arch/mx6-ddr.h>

static struct mx6ul_iomux_grp_regs mx6_grp_ioregs = {
	.grp_addds = 0x00000030,
	.grp_ddrmode_ctl = 0x00020000,
	.grp_b0ds = 0x00000030,
	.grp_ctlds = 0x00000030,
	.grp_b1ds = 0x00000030,
	.grp_ddrpke = 0x00000000,
	.grp_ddrmode = 0x00020000,
	.grp_ddr_type = 0x000c0000,
};

static struct mx6ul_iomux_ddr_regs mx6_ddr_ioregs = {
	.dram_dqm0 = 0x00000030,
	.dram_dqm1 = 0x00000030,
	.dram_ras = 0x00000030,
	.dram_cas = 0x00000030,
	.dram_odt0 = 0x00000030,
	.dram_odt1 = 0x00000030,
	.dram_sdba2 = 0x00000000,
	.dram_sdclk_0 = 0x00000030,
	.dram_sdqs0 = 0x00000030,
	.dram_sdqs1 = 0x00000030,
	.dram_reset = 0x00000030,
};

static struct mx6_mmdc_calibration mx6_mmcd_calib = {
	.p0_mpwldectrl0 = 0x00000004,
	.p0_mpdgctrl0 = 0x00000000,
	.p0_mprddlctl = 0x40404040,
	.p0_mpwrdlctl = 0x40404040,
};

struct mx6_ddr_sysinfo ddr_sysinfo = {
	.dsize = 0,
	.cs_density = 20,
	.ncs = 1,
	.cs1_mirror = 0,
	.rtt_wr = 2,
	.rtt_nom = 1,		/* RTT_Nom = RZQ/2 */
	.walat = 1,		/* Write additional latency */
	.ralat = 5,		/* Read additional latency */
	.mif3_mode = 3,		/* Command prediction working mode */
	.bi_on = 1,		/* Bank interleaving enabled */
	.sde_to_rst = 0x10,	/* 14 cycles, 200us (JEDEC default) */
	.rst_to_cke = 0x23,	/* 33 cycles, 500us (JEDEC default) */
	.ddr_type = DDR_TYPE_DDR3,
};

static struct mx6_ddr3_cfg mem_ddr = {
	.mem_speed = 800,
	.density = 4,
	.width = 16,
	.banks = 8,
	.rowaddr = 15,
	.coladdr = 10,
	.pagesz = 2,
	.trcd = 1375,
	.trcmin = 4875,
	.trasmin = 3500,
};

static void ccgr_init(void)
{
	struct mxc_ccm_reg *ccm = (struct mxc_ccm_reg *)CCM_BASE_ADDR;

	writel(0xFFFFFFFF, &ccm->CCGR0);
	writel(0xFFFFFFFF, &ccm->CCGR1);
	writel(0xFFFFFFFF, &ccm->CCGR2);
	writel(0xFFFFFFFF, &ccm->CCGR3);
	writel(0xFFFFFFFF, &ccm->CCGR4);
	writel(0xFFFFFFFF, &ccm->CCGR5);
	writel(0xFFFFFFFF, &ccm->CCGR6);
	writel(0xFFFFFFFF, &ccm->CCGR7);
}

static void spl_dram_init(void)
{
	mx6ul_dram_iocfg(mem_ddr.width, &mx6_ddr_ioregs, &mx6_grp_ioregs);
	mx6_dram_cfg(&ddr_sysinfo, &mx6_mmcd_calib, &mem_ddr);
}

void board_init_f(ulong dummy)
{
	/* setup AIPS and disable watchdog */
	arch_cpu_init();

	ccgr_init();

	/* iomux */
	board_early_init_f();

	/* setup GP timer */
	timer_init();

	/* UART clocks enabled and gd valid - init serial console */
	preloader_console_init();

	/* DDR initialization */
	spl_dram_init();

	/* Clear the BSS. */
	memset(__bss_start, 0, __bss_end - __bss_start);

	/* load/boot image from boot device */
	board_init_r(NULL, 0);
}
#endif
