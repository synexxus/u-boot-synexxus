/*
 * Copyright (C) 2010-2013 Freescale Semiconductor, Inc.
 *   Extended by John Reep (C) 2014, Synexxus, Inc. <info@synexxus.com>
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/iomux.h>
#include <asm/arch/sys_proto.h>
#include <malloc.h>
#include <asm/arch/mx6-pins.h>
#include <asm/errno.h>
#include <asm/gpio.h>
#include <asm/imx-common/iomux-v3.h>
#include <asm/imx-common/mxc_i2c.h>
#include <asm/imx-common/sata.h>
#include <asm/imx-common/boot_mode.h>
#include <asm/imx-common/video.h>
#include <mmc.h>
#include <fsl_esdhc.h>
#include <micrel.h>
#include <miiphy.h>
#include <ipu_pixfmt.h>
#include <asm/arch/crm_regs.h>
#include <asm/arch/mxc_hdmi.h>
#include <i2c.h>
#include <input.h>
#include <netdev.h>
#include <splash.h>
#include <linux/fb.h>

DECLARE_GLOBAL_DATA_PTR;

#define GP_USB_OTG_PWR	IMX_GPIO_NR(4, 15)		// Aristeus: GPIO4_IO15 OR [KEY_ROW4__USB_OTG_PWR]

#define UART_PAD_CTRL  (PAD_CTL_PUS_100K_UP |			\
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm |			\
	PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define USDHC_PAD_CTRL (PAD_CTL_PUS_47K_UP |			\
	PAD_CTL_SPEED_LOW | PAD_CTL_DSE_80ohm |			\
	PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define ENET_PAD_CTRL  (PAD_CTL_PUS_100K_UP |			\
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm | PAD_CTL_HYS)

#define SPI_PAD_CTRL (PAD_CTL_HYS | PAD_CTL_SPEED_MED |		\
	PAD_CTL_DSE_40ohm     | PAD_CTL_SRE_FAST)

#define BUTTON_PAD_CTRL (PAD_CTL_PUS_100K_UP |			\
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm | PAD_CTL_HYS)

#define I2C_PAD_CTRL	(PAD_CTL_PUS_100K_UP |			\
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm | PAD_CTL_HYS |	\
	PAD_CTL_ODE | PAD_CTL_SRE_FAST)

#define WEAK_PULLUP	(PAD_CTL_PUS_100K_UP |			\
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm | PAD_CTL_HYS |	\
	PAD_CTL_SRE_SLOW)

#define WEAK_PULLDOWN	(PAD_CTL_PUS_100K_DOWN |		\
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm |			\
	PAD_CTL_HYS | PAD_CTL_SRE_SLOW)

#define OUTPUT_40OHM (PAD_CTL_SPEED_MED|PAD_CTL_DSE_40ohm)

/**
	It appears CONFIG_DDR_MB is linked to DDR_MB=1024 found in boards.cfg in u-boot main
 */
int dram_init(void)
{
	gd->ram_size = ((ulong)CONFIG_DDR_MB * 1024 * 1024);

	return 0;
}

/*
	Modified Comment 01/28/2014
	SABRELITE, Nitrogen6x, and Aristeus all use SD3_DAT6/7 for UART1
	
	This efficiently allows the terminal to be used and code reuse
 */
static iomux_v3_cfg_t const uart1_pads[] = {
	MX6_PAD_SD3_DAT6__UART1_RX_DATA | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX6_PAD_SD3_DAT7__UART1_TX_DATA | MUX_PAD_CTRL(UART_PAD_CTRL),
};

/*
	Modified 01/28/2014 for Aristeus 
	UART2 is confgiured from the original of the SABRELITE/NITROGEN6X
 */
static iomux_v3_cfg_t const uart2_pads[] = {
	// original commented
	//#ifdef CONFIG_ARISTEUS
	MX6_PAD_GPIO_7__UART2_TX_DATA | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX6_PAD_GPIO_8__UART2_RX_DATA | MUX_PAD_CTRL(UART_PAD_CTRL),
	//#else
	//MX6_PAD_EIM_D26__UART2_TX_DATA | MUX_PAD_CTRL(UART_PAD_CTRL),
	//MX6_PAD_EIM_D27__UART2_RX_DATA | MUX_PAD_CTRL(UART_PAD_CTRL),
	//#endif
};

//#ifdef CONFIG_ARISTEUS
/*
	Added 01/28/2014 for Aristeus 
	UART3 is an added UART interface communication port
 */
static iomux_v3_cfg_t const uart3_pads[] = {

	MX6_PAD_EIM_D24__UART3_TX_DATA | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX6_PAD_EIM_D25__UART3_RX_DATA| MUX_PAD_CTRL(UART_PAD_CTRL),
};
//#endif

#define PC MUX_PAD_CTRL(I2C_PAD_CTRL)

/* Modified 01/28/2014 for Aristeus
   I2C1: 
		Known Communication Devices:
		ADV7343 	(configuration required)
		SMSC2517i 	(No configuration required)
		LVDS_I2C	(LVDS EEPROM for display configuration [EDID])
*/
static struct i2c_pads_info i2c_pad_info0 = {
	.scl = {
		.i2c_mode = MX6_PAD_EIM_D21__I2C1_SCL | PC,
		.gpio_mode = MX6_PAD_EIM_D21__GPIO3_IO21 | PC,
		.gp = IMX_GPIO_NR(3, 21)
	},
	.sda = {
		.i2c_mode = MX6_PAD_EIM_D28__I2C1_SDA | PC,
		.gpio_mode = MX6_PAD_EIM_D28__GPIO3_IO28 | PC,
		.gp = IMX_GPIO_NR(3, 28)
	}
};

/* Modified 01/28/2014 for Aristeus
   I2C2: 
		Known Communication Devices:
		Outboard on COM Express Header
		PEX8606 PCIe Switch
		PMIC_I2C (MMPF0100)
		HDMI_I2C	(HDMI EEPROM for EDID)
*/
static struct i2c_pads_info i2c_pad_info1 = {
	.scl = {
		.i2c_mode = MX6_PAD_KEY_COL3__I2C2_SCL | PC,
		.gpio_mode = MX6_PAD_KEY_COL3__GPIO4_IO12 | PC,
		.gp = IMX_GPIO_NR(4, 12)
	},
	.sda = {
		.i2c_mode = MX6_PAD_KEY_ROW3__I2C2_SDA | PC,
		.gpio_mode = MX6_PAD_KEY_ROW3__GPIO4_IO13 | PC,
		.gp = IMX_GPIO_NR(4, 13)
	}
};

/* Modified 01/28/2014 for Aristeus
   I2C2: 
		Known Communication Devices:
		Outboard on COM Express Header (SMBus)
		VGA_I2C		(VGA EEPROM for EDID - needed to P&P configure ADV7343)
*/
static struct i2c_pads_info i2c_pad_info2 = {
	.scl = {
		.i2c_mode = MX6_PAD_GPIO_5__I2C3_SCL | PC,
		.gpio_mode = MX6_PAD_GPIO_5__GPIO1_IO05 | PC,
		.gp = IMX_GPIO_NR(1, 5)
	},
	.sda = {
		.i2c_mode = MX6_PAD_GPIO_16__I2C3_SDA | PC,
		.gpio_mode = MX6_PAD_GPIO_16__GPIO7_IO11 | PC,
		.gp = IMX_GPIO_NR(7, 11)
	}
};

/* Modified 01/29/2014
	SD3 goes out to the COM Express Type 6 Header
	This can be interfaced to an SD or microSD card slot
*/
static iomux_v3_cfg_t const usdhc3_pads[] = {
	MX6_PAD_SD3_CLK__SD3_CLK   | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_CMD__SD3_CMD   | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_DAT0__SD3_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_DAT1__SD3_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_DAT2__SD3_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_DAT3__SD3_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_DAT5__GPIO7_IO00    | MUX_PAD_CTRL(NO_PAD_CTRL), /* CD */
	/* Could support WP detection with
	 MX6_PAD_SD3_DAT4_GPIO7_IO01 | MUX_PAD_CTRL(NO_PAD_CTRL) 
	*/
};

/* Modified 01/29/1024
	SD4 goes to a microSD card slot on the COM Express module
*/
static iomux_v3_cfg_t const usdhc4_pads[] = {
	MX6_PAD_SD4_CLK__SD4_CLK   | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD4_CMD__SD4_CMD   | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD4_DAT0__SD4_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD4_DAT1__SD4_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD4_DAT2__SD4_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD4_DAT3__SD4_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_NANDF_D6__GPIO2_IO06    | MUX_PAD_CTRL(NO_PAD_CTRL), /* CD */
	/* Could support WP detection with
	 MX6_PAD_NANDF_D7_GPIO2_IO07 | MUX_PAD_CTRL(NO_PAD_CTRL) 
	*/
};

/* Added SD2 support on Aristeus.  Not supported on SABRELITE or NITROGEN6X
	SD2 is an onboard microSD slot

	Added: 01/29/2014
	SD2 is supported on the COM Express through the standard interfaces
*/
//#ifdef CONFIG_ARISTEUS
static iomux_v3_cfg_t const usdhc2_pads[] = {
	MX6_PAD_SD2_CLK__SD2_CLK   | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD2_CMD__SD2_CMD   | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD2_DAT0__SD2_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD2_DAT1__SD2_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD2_DAT2__SD2_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD2_DAT3__SD2_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_GPIO_4__GPIO1_IO04    | MUX_PAD_CTRL(NO_PAD_CTRL), /* CD */
	/* Could support WP detection with
	 MX6_PAD_GPIO_2__GPIO1_IO02 | MUX_PAD_CTRL(NO_PAD_CTRL) 
	*/
};
//#endif

/*
	
 */
static iomux_v3_cfg_t const enet_pads1[] = {
	MX6_PAD_ENET_MDIO__ENET_MDIO		| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET_MDC__ENET_MDC		| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_TXC__RGMII_TXC	| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_TD0__RGMII_TD0	| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_TD1__RGMII_TD1	| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_TD2__RGMII_TD2	| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_TD3__RGMII_TD3	| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_TX_CTL__RGMII_TX_CTL	| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET_REF_CLK__ENET_TX_CLK	| MUX_PAD_CTRL(ENET_PAD_CTRL),
	/* pin 35 - 1 (PHY_AD2) on reset */
	MX6_PAD_RGMII_RXC__GPIO6_IO30		| MUX_PAD_CTRL(NO_PAD_CTRL),
	/* pin 32 - 1 - (MODE0) all */
	MX6_PAD_RGMII_RD0__GPIO6_IO25		| MUX_PAD_CTRL(NO_PAD_CTRL),
	/* pin 31 - 1 - (MODE1) all */
	MX6_PAD_RGMII_RD1__GPIO6_IO27		| MUX_PAD_CTRL(NO_PAD_CTRL),
	/* pin 28 - 1 - (MODE2) all */
	MX6_PAD_RGMII_RD2__GPIO6_IO28		| MUX_PAD_CTRL(NO_PAD_CTRL),
	/* pin 27 - 1 - (MODE3) all */
	MX6_PAD_RGMII_RD3__GPIO6_IO29		| MUX_PAD_CTRL(NO_PAD_CTRL),
	/* pin 33 - 1 - (CLK125_EN) 125Mhz clockout enabled */
	MX6_PAD_RGMII_RX_CTL__GPIO6_IO24	| MUX_PAD_CTRL(NO_PAD_CTRL),
	/* pin 42 PHY nRST */
	//#if defined(CONFIG_ARISTEUS)
	MX6_PAD_ENET_CRS_DV__GPIO1_IO25		| MUX_PAD_CTRL(NO_PAD_CTRL),
	#define _RGMII_nRST_GP_ 1
	#define _RGMII_nRST_IO_	25
	//#elif defined(CONFIG_SABRELITE)
	//MX6_PAD_EIM_D23__GPIO3_IO23		| MUX_PAD_CTRL(NO_PAD_CTRL),
	//#define _RGMII_nRST_GP_ 3
	//#define _RGMII_nRST_IO_	23
	//#else
	//MX6_PAD_ENET_RXD0__GPIO1_IO27		| MUX_PAD_CTRL(NO_PAD_CTRL),
	//#define _RGMII_nRST_GP_ 1
	//#define _RGMII_nRST_IO_	27
	//#endif
};

static iomux_v3_cfg_t const enet_pads2[] = {
	MX6_PAD_RGMII_RXC__RGMII_RXC	| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_RD0__RGMII_RD0	| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_RD1__RGMII_RD1	| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_RD2__RGMII_RD2	| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_RD3__RGMII_RD3	| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_RX_CTL__RGMII_RX_CTL	| MUX_PAD_CTRL(ENET_PAD_CTRL),
};

//#if defined(CONFIG_ARISTEUS)
/* 
	Moves USB_OTG_ID to ENET_RX_ER pad and USB_H1_OC
 */
static iomux_v3_cfg_t const misc_pads[] = {
	MX6_PAD_ENET_RX_ER__USB_OTG_ID		| MUX_PAD_CTRL(WEAK_PULLUP),
	MX6_PAD_KEY_COL4__USB_OTG_OC		| MUX_PAD_CTRL(WEAK_PULLUP),
	MX6_PAD_GPIO_3__USB_H1_OC		| MUX_PAD_CTRL(WEAK_PULLUP),
	/* OTG Power enable */
	MX6_PAD_KEY_ROW4__USB_OTG_PWR		| MUX_PAD_CTRL(OUTPUT_40OHM),
};


//#if defined(CONFIG_ARISTEUS)
/* Added on 01/30/2014
	Aristeus supports the android buttons but in different locations
 */
static iomux_v3_cfg_t const button_pads[] = {
	/* Menu */
	MX6_PAD_NANDF_D1__GPIO2_IO01	| MUX_PAD_CTRL(BUTTON_PAD_CTRL),
	/* Back */
	MX6_PAD_NANDF_D2__GPIO2_IO02	| MUX_PAD_CTRL(BUTTON_PAD_CTRL),
	/* Labelled Search (mapped to Power under Android) */
	MX6_PAD_NANDF_D3__GPIO2_IO03	| MUX_PAD_CTRL(BUTTON_PAD_CTRL),
	/* Home */
	MX6_PAD_NANDF_D0__GPIO2_IO00	| MUX_PAD_CTRL(BUTTON_PAD_CTRL),
	/* Volume Down */
	MX6_PAD_NANDF_RB0__GPIO6_IO10	| MUX_PAD_CTRL(BUTTON_PAD_CTRL),
	/* Volume Up */
	MX6_PAD_NANDF_WP_B__GPIO6_IO09	| MUX_PAD_CTRL(BUTTON_PAD_CTRL),
};
//#else
//#endif

/*
	FUNCTION: setup_iomux_enet(void)
	
	INPUT: void
	OUTPUT:
	DESCRIPTION: initial setup for ENET descriptors and called by board_eth_init() function
 */
static void setup_iomux_enet(void)
{
//	imx_iomux_v3_setup_multiple_pads(enet_pads1, ARRAY_SIZE(enet_pads1));	/* sets remainder*/
//	udelay(20);
	gpio_direction_output(IMX_GPIO_NR(_RGMII_nRST_GP_, _RGMII_nRST_IO_), 0); 
	gpio_direction_output(IMX_GPIO_NR(6, 30), 1);	/* RGMII_RXCLK */
	gpio_direction_output(IMX_GPIO_NR(6, 25), 1);	/* RGMII_RD0 */
	gpio_direction_output(IMX_GPIO_NR(6, 27), 1);	/* RGMII_RD1 */
	gpio_direction_output(IMX_GPIO_NR(6, 28), 1);	/* RGMII_RD2 */
	gpio_direction_output(IMX_GPIO_NR(6, 29), 1);	/* RGMII_RD3 */
	imx_iomux_v3_setup_multiple_pads(enet_pads1, ARRAY_SIZE(enet_pads1));
	gpio_direction_output(IMX_GPIO_NR(6, 24), 1);	/* RGMII_RX_CTL */

	/* Need delay 10ms according to KSZ9031 spec */
	udelay(1000 * 10);
	gpio_set_value(IMX_GPIO_NR(_RGMII_nRST_GP_, _RGMII_nRST_IO_), 1);	/* Remove from Reset */

	/* Configure for RGMII*/
	imx_iomux_v3_setup_multiple_pads(enet_pads2, ARRAY_SIZE(enet_pads2));
	udelay(100);
}

/*
	Goes to USB HUB RESET_B -- Managed to keep them the same
	
 */
iomux_v3_cfg_t const usb_pads[] = {
	MX6_PAD_GPIO_17__GPIO7_IO12 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

/* Updated on 01/30/2014*/
static void setup_iomux_uart(void)
{
	imx_iomux_v3_setup_multiple_pads(uart1_pads, ARRAY_SIZE(uart1_pads));
	imx_iomux_v3_setup_multiple_pads(uart2_pads, ARRAY_SIZE(uart2_pads));

//	#ifdef CONFIG_ARISTEUS
	imx_iomux_v3_setup_multiple_pads(uart3_pads, ARRAY_SIZE(uart3_pads));
	// Consider a downstream flag for UART3 or just use CONFIG_ARISTEUS?
//	#endif
}

#ifdef CONFIG_USB_EHCI_MX6
int board_ehci_hcd_init(int port)
{
	imx_iomux_v3_setup_multiple_pads(usb_pads, ARRAY_SIZE(usb_pads));

	/* Reset USB hub */
	gpio_direction_output(IMX_GPIO_NR(7, 12), 0);
	mdelay(2);
	gpio_set_value(IMX_GPIO_NR(7, 12), 1);

	return 0;
}

int board_ehci_power(int port, int on)
{
	if (port)
		return 0;
	gpio_set_value(GP_USB_OTG_PWR, on);
	return 0;
}
#endif

#ifdef CONFIG_FSL_ESDHC
//#if defined(CONFIG_ARISTEUS)
struct fsl_esdhc_cfg usdhc_cfg[3] = {
	{USDHC3_BASE_ADDR},
	{USDHC4_BASE_ADDR},
	{USDHC2_BASE_ADDR},
};
//#else
//struct fsl_esdhc_cfg usdhc_cfg[2] = {
//	{USDHC3_BASE_ADDR},
//	{USDHC4_BASE_ADDR},
//};
//#endif

//#if defined(CONFIG_ARISTEUS)
int board_mmc_getcd(struct mmc *mmc)
{
	struct fsl_esdhc_cfg *cfg = (struct fsl_esdhc_cfg *)mmc->priv;
	int ret = 0;

	switch(cfg->esdhc_base){
		case USDHC3_BASE_ADDR:
			gpio_direction_input(IMX_GPIO_NR(7, 0));
			ret = !gpio_get_value(IMX_GPIO_NR(7, 0));
			break;
		case USDHC4_BASE_ADDR:
			gpio_direction_input(IMX_GPIO_NR(2, 6));
			ret = !gpio_get_value(IMX_GPIO_NR(2, 6));
			break;
		case USDHC2_BASE_ADDR:
			// This is also the actual SD2_CD_B signal group! (ALT6)
			gpio_direction_input(IMX_GPIO_NR(1, 4));
			ret = !gpio_get_value(IMX_GPIO_NR(1, 4));
			break;
	}
	return ret;
}


int board_mmc_init(bd_t *bis)
{
	s32 status = 0;
	u32 index = 0;

//#if defined(CONFIG_ARISTEUS)
	usdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC3_CLK);
	usdhc_cfg[1].sdhc_clk = mxc_get_clock(MXC_ESDHC4_CLK);
	usdhc_cfg[2].sdhc_clk = mxc_get_clock(MXC_ESDHC2_CLK);

	usdhc_cfg[0].max_bus_width = 4;
	usdhc_cfg[1].max_bus_width = 4;
	usdhc_cfg[2].max_bus_width = 4;


	for (index = 0; index < CONFIG_SYS_FSL_USDHC_NUM; ++index) {
		switch (index) {
		case 0:
			imx_iomux_v3_setup_multiple_pads(
				usdhc3_pads, ARRAY_SIZE(usdhc3_pads));
			break;
		case 1:
		       imx_iomux_v3_setup_multiple_pads(
			       usdhc4_pads, ARRAY_SIZE(usdhc4_pads));
		       break;
		case 2:
		       imx_iomux_v3_setup_multiple_pads(
			       usdhc2_pads, ARRAY_SIZE(usdhc2_pads));
			break;
		default:
		       printf("Warning: you configured more USDHC controllers"
			       "(%d) then supported by the board (%d)\n",
			       index + 1, CONFIG_SYS_FSL_USDHC_NUM);
		       return status;
		}

		status |= fsl_esdhc_initialize(bis, &usdhc_cfg[index]);
	}

	return status;
}
#endif

#ifdef CONFIG_MXC_SPI
iomux_v3_cfg_t const ecspi1_pads[] = {
	/* SS1 */
	// why not? EIM_D19__ECSPI1_SS1
	MX6_PAD_EIM_D19__GPIO3_IO19   | MUX_PAD_CTRL(SPI_PAD_CTRL),
	MX6_PAD_EIM_D17__ECSPI1_MISO | MUX_PAD_CTRL(SPI_PAD_CTRL),
	MX6_PAD_EIM_D18__ECSPI1_MOSI | MUX_PAD_CTRL(SPI_PAD_CTRL),
	MX6_PAD_EIM_D16__ECSPI1_SCLK | MUX_PAD_CTRL(SPI_PAD_CTRL),
	/* SS0 */
	MX6_PAD_EIM_EB2__GPIO2_IO30 | MUX_PAD_CTRL(SPI_PAD_CTRL),
	
};


iomux_v3_cfg_t const ecspi2_pads[] = {
	/* SS0 */
	MX6_PAD_EIM_RW__GPIO2_IO26   | MUX_PAD_CTRL(SPI_PAD_CTRL),
	MX6_PAD_EIM_OE__ECSPI2_MISO | MUX_PAD_CTRL(SPI_PAD_CTRL),
	MX6_PAD_EIM_CS1__ECSPI2_MOSI | MUX_PAD_CTRL(SPI_PAD_CTRL),
	MX6_PAD_EIM_CS0__ECSPI2_SCLK | MUX_PAD_CTRL(SPI_PAD_CTRL),
};

void setup_spi(void)
{
	imx_iomux_v3_setup_multiple_pads(ecspi1_pads,
					 ARRAY_SIZE(ecspi1_pads));

	imx_iomux_v3_setup_multiple_pads(ecspi2_pads,
					 ARRAY_SIZE(ecspi2_pads));

}
#endif

int board_phy_config(struct phy_device *phydev)
{

	/* min rx data delay */
	//ksz9031_phy_extended_write(phydev,
	//		MII_KSZ9031_EXT_RGMII_RX_DATA_SKEW, 0x0);
	/* min tx data delay */
	//ksz9031_phy_extended_write(phydev,
	//		MII_KSZ9031_EXT_RGMII_TX_DATA_SKEW, 0x0);
	/* max rx/tx clock delay, min rx/tx control */
	//ksz9031_phy_extended_write(phydev,
	//		MII_KSZ9031_EXT_RGMII_CLOCK_SKEW, 0xf0f0);

	phy_write(phydev, MDIO_DEVAD_NONE, MII_CTRL1000, 0x1c00);

	/* control data pad skew - devaddr = 0x02, register = 0x04 */
	ksz9031_phy_extended_write(phydev, 0x02,
				   MII_KSZ9031_EXT_RGMII_CTRL_SIG_SKEW,
				   MII_KSZ9031_MOD_DATA_NO_POST_INC, 0x0000);
	/* rx data pad skew - devaddr = 0x02, register = 0x05 */
	ksz9031_phy_extended_write(phydev, 0x02,
				   MII_KSZ9031_EXT_RGMII_RX_DATA_SKEW,
				   MII_KSZ9031_MOD_DATA_NO_POST_INC, 0x0000);
	/* tx data pad skew - devaddr = 0x02, register = 0x05 */
	ksz9031_phy_extended_write(phydev, 0x02,
				   MII_KSZ9031_EXT_RGMII_TX_DATA_SKEW,
				   MII_KSZ9031_MOD_DATA_NO_POST_INC, 0x0000);
	/* gtx and rx clock pad skew - devaddr = 0x02, register = 0x08 */
	ksz9031_phy_extended_write(phydev, 0x02,
				   MII_KSZ9031_EXT_RGMII_CLOCK_SKEW,
				   MII_KSZ9031_MOD_DATA_NO_POST_INC, 0x03FF);

//#else
	/* min rx data delay */
//	ksz9021_phy_extended_write(phydev,
//			MII_KSZ9021_EXT_RGMII_RX_DATA_SKEW, 0x0);
	/* min tx data delay */
//	ksz9021_phy_extended_write(phydev,
//			MII_KSZ9021_EXT_RGMII_TX_DATA_SKEW, 0x0);
	/* max rx/tx clock delay, min rx/tx control */
//	ksz9021_phy_extended_write(phydev,
//			MII_KSZ9021_EXT_RGMII_CLOCK_SKEW, 0xf0f0);
//#endif
	if (phydev->drv->config)
		phydev->drv->config(phydev);
	return 0;
}

/*
	Modified to this point!
 */
int board_eth_init(bd_t *bis)
{

#if defined(CONFIG_FEC_MXC)
	uint32_t base = IMX_FEC_BASE;
	struct mii_dev *bus = NULL;
	struct phy_device *phydev = NULL;
	int ret;
#endif

	setup_iomux_enet();
//puts("Board eth init called\n");
#ifdef CONFIG_FEC_MXC
//puts("fec get miibus\n");
	bus = fec_get_miibus(base, -1);
	if (!bus)
		return 0;
	/* scan phy 4,5,6,7 */
	phydev = phy_find_by_mask(bus, (0xf << 4), PHY_INTERFACE_MODE_RGMII);
	if (!phydev) {
		free(bus);
		return 0;
	}
	printf("using phy at %d\n", phydev->addr);
	ret  = fec_probe(bis, -1, base, bus, phydev);
	if (ret) {
		printf("FEC MXC: %s:failed\n", __func__);
		free(phydev);
		free(bus);
	}

#endif

#ifdef CONFIG_MV_UDC
	/* For otg ethernet*/
	usb_eth_initialize(bis);
#endif
	return 0;
}

/* New Function as of 2014.09 */
int splash_screen_prepare(void)
{
	char *env_loadsplash;

	if (!getenv("splashimage") || !getenv("splashsize")) {
		return -1;
	}

	env_loadsplash = getenv("loadsplash");
	if (env_loadsplash == NULL) {
		printf("Environment variable loadsplash not found!\n");
		return -1;
	}

	if (run_command_list(env_loadsplash, -1, 0)) {
		printf("failed to run loadsplash %s\n\n", env_loadsplash);
		return -1;
	}

	return 0;
}

static void setup_buttons(void)
{
	imx_iomux_v3_setup_multiple_pads(button_pads,
					 ARRAY_SIZE(button_pads));
}

#if defined(CONFIG_VIDEO_IPUV3)

static iomux_v3_cfg_t const backlight_pads[] = {
	/* Backlight PWM for LVDS_BKLT_CTRL*/
	//MX6_PAD_SD1_DAT3__PWM1_OUT
	MX6_PAD_SD1_DAT3__GPIO1_IO21 | MUX_PAD_CTRL(NO_PAD_CTRL),
#define LVDS_BACKLIGHT_GP IMX_GPIO_NR(1, 21)
#define RGB_BACKLIGHT_GP IMX_GPIO_NR(1, 21)		// There is no actual RGB but configure for backwards compatibility performance [can be the same as LVDS just for repurpose]

	/* LVDS Backlight Enable Signal to the COM Express header */
	MX6_PAD_NANDF_CS3__GPIO6_IO16 | MUX_PAD_CTRL(NO_PAD_CTRL),
#define LVDS_BACKLIGHT_EN IMX_GPIO_NR(6,16)

	/* LVDS Panel Enable Power signal*/
	MX6_PAD_NANDF_CS1__GPIO6_IO14 | MUX_PAD_CTRL(NO_PAD_CTRL),
#define LVDS_PANEL_EN	IMX_GPIO_NR(6,14)

};

static iomux_v3_cfg_t const rgb_pads[] = {
	MX6_PAD_DI0_DISP_CLK__IPU1_DI0_DISP_CLK,
	MX6_PAD_DI0_PIN15__IPU1_DI0_PIN15,
	MX6_PAD_DI0_PIN2__IPU1_DI0_PIN02,
	MX6_PAD_DI0_PIN3__IPU1_DI0_PIN03,
	MX6_PAD_DI0_PIN4__GPIO4_IO20,
	MX6_PAD_DISP0_DAT0__IPU1_DISP0_DATA00,
	MX6_PAD_DISP0_DAT1__IPU1_DISP0_DATA01,
	MX6_PAD_DISP0_DAT2__IPU1_DISP0_DATA02,
	MX6_PAD_DISP0_DAT3__IPU1_DISP0_DATA03,
	MX6_PAD_DISP0_DAT4__IPU1_DISP0_DATA04,
	MX6_PAD_DISP0_DAT5__IPU1_DISP0_DATA05,
	MX6_PAD_DISP0_DAT6__IPU1_DISP0_DATA06,
	MX6_PAD_DISP0_DAT7__IPU1_DISP0_DATA07,
	MX6_PAD_DISP0_DAT8__IPU1_DISP0_DATA08,
	MX6_PAD_DISP0_DAT9__IPU1_DISP0_DATA09,
	MX6_PAD_DISP0_DAT10__IPU1_DISP0_DATA10,
	MX6_PAD_DISP0_DAT11__IPU1_DISP0_DATA11,
	MX6_PAD_DISP0_DAT12__IPU1_DISP0_DATA12,
	MX6_PAD_DISP0_DAT13__IPU1_DISP0_DATA13,
	MX6_PAD_DISP0_DAT14__IPU1_DISP0_DATA14,
	MX6_PAD_DISP0_DAT15__IPU1_DISP0_DATA15,
	MX6_PAD_DISP0_DAT16__IPU1_DISP0_DATA16,
	MX6_PAD_DISP0_DAT17__IPU1_DISP0_DATA17,
	MX6_PAD_DISP0_DAT18__IPU1_DISP0_DATA18,
	MX6_PAD_DISP0_DAT19__IPU1_DISP0_DATA19,
	MX6_PAD_DISP0_DAT20__IPU1_DISP0_DATA20,
	MX6_PAD_DISP0_DAT21__IPU1_DISP0_DATA21,
	MX6_PAD_DISP0_DAT22__IPU1_DISP0_DATA22,
	MX6_PAD_DISP0_DAT23__IPU1_DISP0_DATA23,
};

static void do_enable_hdmi(struct display_info_t const *dev)
{
	imx_enable_hdmi_phy();
}

static int detect_i2c(struct display_info_t const *dev)
{
	return ((0 == i2c_set_bus_num(dev->bus))
		&&
		(0 == i2c_probe(dev->addr)));
}

static void enable_lvds(struct display_info_t const *dev)
{
	struct iomuxc *iomux = (struct iomuxc *)
				IOMUXC_BASE_ADDR;
	u32 reg = readl(&iomux->gpr[2]);
	reg |= IOMUXC_GPR2_DATA_WIDTH_CH0_24BIT;
	writel(reg, &iomux->gpr[2]);
	gpio_direction_output(LVDS_BACKLIGHT_GP, 1);
}

static void enable_lvds_jeida(struct display_info_t const *dev)
{
	struct iomuxc *iomux = (struct iomuxc *)
				IOMUXC_BASE_ADDR;
	u32 reg = readl(&iomux->gpr[2]);
	reg |= IOMUXC_GPR2_DATA_WIDTH_CH0_24BIT
	     |IOMUXC_GPR2_BIT_MAPPING_CH0_JEIDA;
	writel(reg, &iomux->gpr[2]);
	gpio_direction_output(LVDS_BACKLIGHT_GP, 1);
}

static void enable_rgb(struct display_info_t const *dev)
{
	imx_iomux_v3_setup_multiple_pads(
		rgb_pads,
		 ARRAY_SIZE(rgb_pads));
	gpio_direction_output(RGB_BACKLIGHT_GP, 1);
	/* RGB pads on the Aristeus go to a programmable ADV7343 which requires I2C programming */
}


struct display_info_t const displays[] = {{
	.bus	= 1,
	.addr	= 0x50,
	.pixfmt	= IPU_PIX_FMT_RGB24,
	.detect	= detect_i2c,
	.enable	= do_enable_hdmi,
	.mode	= {
		.name           = "HDMI",
		.refresh        = 60,
		.xres           = 1024,
		.yres           = 768,
		.pixclock       = 15385,
		.left_margin    = 220,
		.right_margin   = 40,
		.upper_margin   = 21,
		.lower_margin   = 7,
		.hsync_len      = 60,
		.vsync_len      = 10,
		.sync           = FB_SYNC_EXT,
		.vmode          = FB_VMODE_NONINTERLACED
} }, {
	.bus	= 0,
	.addr	= 0,
	.pixfmt	= IPU_PIX_FMT_RGB24,
	.detect	= NULL,
	.enable	= enable_lvds_jeida,
	.mode	= {
		.name           = "LDB-WXGA",
		.refresh        = 60,
		.xres           = 1280,
		.yres           = 800,
		.pixclock       = 14065,
		.left_margin    = 40,
		.right_margin   = 40,
		.upper_margin   = 3,
		.lower_margin   = 80,
		.hsync_len      = 10,
		.vsync_len      = 10,
		.sync           = FB_SYNC_EXT,
		.vmode          = FB_VMODE_NONINTERLACED
} }, {
	.bus	= 2,
	.addr	= 0x4,
	.pixfmt	= IPU_PIX_FMT_LVDS666,
	.detect	= detect_i2c,
	.enable	= enable_lvds,
	.mode	= {
		.name           = "Hannstar-XGA",
		.refresh        = 60,
		.xres           = 1024,
		.yres           = 768,
		.pixclock       = 15385,
		.left_margin    = 220,
		.right_margin   = 40,
		.upper_margin   = 21,
		.lower_margin   = 7,
		.hsync_len      = 60,
		.vsync_len      = 10,
		.sync           = FB_SYNC_EXT,
		.vmode          = FB_VMODE_NONINTERLACED
} },  {
	.bus	= 2,
	.addr	= 0x4,
	.pixfmt	= IPU_PIX_FMT_LVDS888,
	.detect	= detect_i2c,
	.enable	= enable_lvds,
	.mode	= {
		.name           = "Hannstar-XGA8",
		.refresh        = 60,
		.xres           = 1024,
		.yres           = 768,
		.pixclock       = 15385,
		.left_margin    = 220,
		.right_margin   = 40,
		.upper_margin   = 21,
		.lower_margin   = 7,
		.hsync_len      = 60,
		.vsync_len      = 10,
		.sync           = FB_SYNC_EXT,
		.vmode          = FB_VMODE_NONINTERLACED
} },   {
	.bus	= 2,
	.addr	= 0x4,
	.pixfmt	= IPU_PIX_FMT_RGB666,
	.detect	= detect_i2c,
	.enable	= enable_lvds,
	.mode	= {
		.name           = "Hannstar-XGA18",
		.refresh        = 60,
		.xres           = 1024,
		.yres           = 768,
		.pixclock       = 15385,
		.left_margin    = 220,
		.right_margin   = 40,
		.upper_margin   = 21,
		.lower_margin   = 7,
		.hsync_len      = 60,
		.vsync_len      = 10,
		.sync           = FB_SYNC_EXT,
		.vmode          = FB_VMODE_NONINTERLACED
} }, {
	.bus	= 0,
	.addr	= 0,
	.pixfmt	= IPU_PIX_FMT_RGB24,
	.detect	= NULL,
	.enable	= enable_lvds,
	.mode	= {
		.name           = "LDB-WXGA-S",
		.refresh        = 60,
		.xres           = 1280,
		.yres           = 800,
		.pixclock       = 14065,
		.left_margin    = 40,
		.right_margin   = 40,
		.upper_margin   = 3,
		.lower_margin   = 80,
		.hsync_len      = 10,
		.vsync_len      = 10,
		.sync           = FB_SYNC_EXT,
		.vmode          = FB_VMODE_NONINTERLACED
} }, {
	.bus	= 2,
	.addr	= 0x4,
	.pixfmt	= IPU_PIX_FMT_LVDS666,
	.detect	= detect_i2c,
	.enable	= enable_lvds,
	.mode	= {
		.name           = "LG-9.7",
		.refresh        = 60,
		.xres           = 1024,
		.yres           = 768,
		.pixclock       = 15385, /* ~65MHz */
		.left_margin    = 480,
		.right_margin   = 260,
		.upper_margin   = 16,
		.lower_margin   = 6,
		.hsync_len      = 250,
		.vsync_len      = 10,
		.sync           = FB_SYNC_EXT,
		.vmode          = FB_VMODE_NONINTERLACED
} }, {
	.bus	= 2,
	.addr	= 0x38,
	.pixfmt	= IPU_PIX_FMT_LVDS666,
	.detect	= detect_i2c,
	.enable	= enable_lvds,
	.mode	= {
		.name           = "wsvga-lvds",
		.refresh        = 60,
		.xres           = 1024,
		.yres           = 600,
		.pixclock       = 15385,
		.left_margin    = 220,
		.right_margin   = 40,
		.upper_margin   = 21,
		.lower_margin   = 7,
		.hsync_len      = 60,
		.vsync_len      = 10,
		.sync           = FB_SYNC_EXT,
		.vmode          = FB_VMODE_NONINTERLACED
} }, {
	.bus	= 2,
	.addr	= 0x41,
	.pixfmt	= IPU_PIX_FMT_LVDS666,
	.detect	= detect_i2c,
	.enable	= enable_lvds,
	.mode	= {
		.name           = "amp1024x600",
		.refresh        = 60,
		.xres           = 1024,
		.yres           = 600,
		.pixclock       = 15385,
		.left_margin    = 220,
		.right_margin   = 40,
		.upper_margin   = 21,
		.lower_margin   = 7,
		.hsync_len      = 60,
		.vsync_len      = 10,
		.sync           = FB_SYNC_EXT,
		.vmode          = FB_VMODE_NONINTERLACED
} }, {
	.bus	= 0,
	.addr	= 0,
	.pixfmt	= IPU_PIX_FMT_LVDS666,
	.detect	= 0,
	.enable	= enable_lvds,
	.mode	= {
		.name           = "wvga-lvds",
		.refresh        = 57,
		.xres           = 800,
		.yres           = 480,
		.pixclock       = 15385,
		.left_margin    = 220,
		.right_margin   = 40,
		.upper_margin   = 21,
		.lower_margin   = 7,
		.hsync_len      = 60,
		.vsync_len      = 10,
		.sync           = FB_SYNC_EXT,
		.vmode          = FB_VMODE_NONINTERLACED
} }, {
	.bus	= 2,
	.addr	= 0x48,
	.pixfmt	= IPU_PIX_FMT_RGB666,
	.detect	= detect_i2c,
	.enable	= enable_rgb,
	.mode	= {
		.name           = "wvga-rgb",
		.refresh        = 57,
		.xres           = 800,
		.yres           = 480,
		.pixclock       = 37037,
		.left_margin    = 40,
		.right_margin   = 60,
		.upper_margin   = 10,
		.lower_margin   = 10,
		.hsync_len      = 20,
		.vsync_len      = 10,
		.sync           = 0,
		.vmode          = FB_VMODE_NONINTERLACED
} }, {
	.bus	= 2,
	.addr	= 0x10,
	.pixfmt	= IPU_PIX_FMT_RGB666,
	.detect	= detect_i2c,
	.enable	= enable_rgb,
	.mode	= {
		.name           = "fusion7",
		.refresh        = 60,
		.xres           = 800,
		.yres           = 480,
		.pixclock       = 33898,
		.left_margin    = 96,
		.right_margin   = 24,
		.upper_margin   = 3,
		.lower_margin   = 10,
		.hsync_len      = 72,
		.vsync_len      = 7,
		.sync           = 0x40000002,
		.vmode          = FB_VMODE_NONINTERLACED
} }, {
	.bus	= 0,
	.addr	= 0,
	.pixfmt	= IPU_PIX_FMT_RGB666,
	.detect	= detect_i2c,
	.enable	= enable_rgb,
	.mode	= {
		.name           = "svga",
		.refresh        = 60,
		.xres           = 800,
		.yres           = 600,
		.pixclock       = 15385,
		.left_margin    = 220,
		.right_margin   = 40,
		.upper_margin   = 21,
		.lower_margin   = 7,
		.hsync_len      = 60,
		.vsync_len      = 10,
		.sync           = 0,
		.vmode          = FB_VMODE_NONINTERLACED
} }, {
	.bus	= 2,
	.addr	= 0x48,
	.pixfmt	= IPU_PIX_FMT_RGB24,
	.detect	= detect_i2c,
	.enable	= enable_rgb,
	.mode	= {
		.name           = "qvga",
		.refresh        = 60,
		.xres           = 320,
		.yres           = 240,
		.pixclock       = 37037,
		.left_margin    = 38,
		.right_margin   = 37,
		.upper_margin   = 16,
		.lower_margin   = 15,
		.hsync_len      = 30,
		.vsync_len      = 3,
		.sync           = 0,
		.vmode          = FB_VMODE_NONINTERLACED
} } };

size_t display_count = ARRAY_SIZE(displays);


int board_cfb_skip(void)
{
	return 0 != getenv("novideo");
}

/*
int board_video_skip(void)
{
	int i;
	int ret;
	char const *panel;

	panel = getenv("panel");
	if (!panel) {
		for (i = 0; i < ARRAY_SIZE(displays); i++) {
			struct display_info_t const *dev = displays+i;
			if (dev->detect && dev->detect(dev)) {
				panel = dev->mode.name;
				printf("auto-detected panel %s\n", panel);
				break;
			}
		}
		if (!panel) {
			panel = displays[0].mode.name;
			printf("No panel detected: default to %s\n", panel);
			i = 0;
		}
	} else {
		for (i = 0; i < ARRAY_SIZE(displays); i++) {
			if (!strcmp(panel, displays[i].mode.name))
				break;
		}
	}
	if (i < ARRAY_SIZE(displays)) {
		ret = ipuv3_fb_init(&displays[i].mode, 0,
				    displays[i].pixfmt);
		if (!ret) {
			displays[i].enable(displays+i);
			printf("Display: %s (%ux%u)\n",
			       displays[i].mode.name,
			       displays[i].mode.xres,
			       displays[i].mode.yres);
		} else {
			printf("LCD %s cannot be configured: %d\n",
			       displays[i].mode.name, ret);
		}
	} else {
		printf("unsupported panel %s\n", panel);
		ret = -EINVAL;
	}

	if (!ret)
		splash_screen_prepare();

	return (0 != ret);
}
*/

static void setup_display(void)
{
	struct mxc_ccm_reg *mxc_ccm = (struct mxc_ccm_reg *)CCM_BASE_ADDR;
	struct iomuxc *iomux = (struct iomuxc *)IOMUXC_BASE_ADDR;
	int reg;

	enable_ipu_clock();
	imx_setup_hdmi();
	/* Turn on LDB0,IPU,IPU DI0 clocks */
	reg = __raw_readl(&mxc_ccm->CCGR3);
	reg |=  MXC_CCM_CCGR3_LDB_DI0_MASK;
	writel(reg, &mxc_ccm->CCGR3);

	/* set LDB0, LDB1 clk select to 011/011 */
	reg = readl(&mxc_ccm->cs2cdr);
	reg &= ~(MXC_CCM_CS2CDR_LDB_DI0_CLK_SEL_MASK
		 |MXC_CCM_CS2CDR_LDB_DI1_CLK_SEL_MASK);
	reg |= (3<<MXC_CCM_CS2CDR_LDB_DI0_CLK_SEL_OFFSET)
	      |(3<<MXC_CCM_CS2CDR_LDB_DI1_CLK_SEL_OFFSET);
	writel(reg, &mxc_ccm->cs2cdr);

	reg = readl(&mxc_ccm->cscmr2);
	reg |= MXC_CCM_CSCMR2_LDB_DI0_IPU_DIV;
	writel(reg, &mxc_ccm->cscmr2);

	reg = readl(&mxc_ccm->chsccdr);
	reg |= (CHSCCDR_CLK_SEL_LDB_DI0
		<<MXC_CCM_CHSCCDR_IPU1_DI0_CLK_SEL_OFFSET);
	writel(reg, &mxc_ccm->chsccdr);

	reg = IOMUXC_GPR2_BGREF_RRMODE_EXTERNAL_RES
	     |IOMUXC_GPR2_DI1_VS_POLARITY_ACTIVE_HIGH
	     |IOMUXC_GPR2_DI0_VS_POLARITY_ACTIVE_LOW
	     |IOMUXC_GPR2_BIT_MAPPING_CH1_SPWG
	     |IOMUXC_GPR2_DATA_WIDTH_CH1_18BIT
	     |IOMUXC_GPR2_BIT_MAPPING_CH0_SPWG
	     |IOMUXC_GPR2_DATA_WIDTH_CH0_18BIT
	     |IOMUXC_GPR2_LVDS_CH1_MODE_DISABLED
	     |IOMUXC_GPR2_LVDS_CH0_MODE_ENABLED_DI0;
	writel(reg, &iomux->gpr[2]);

	reg = readl(&iomux->gpr[3]);
	reg = (reg & ~(IOMUXC_GPR3_LVDS0_MUX_CTL_MASK
			|IOMUXC_GPR3_HDMI_MUX_CTL_MASK))
	    | (IOMUXC_GPR3_MUX_SRC_IPU1_DI0
	       <<IOMUXC_GPR3_LVDS0_MUX_CTL_OFFSET);
	writel(reg, &iomux->gpr[3]);

	/* backlights off until needed */
	imx_iomux_v3_setup_multiple_pads(backlight_pads,
					 ARRAY_SIZE(backlight_pads));
	gpio_direction_input(LVDS_BACKLIGHT_GP);
	gpio_direction_input(RGB_BACKLIGHT_GP);
}
#endif

/* v2014.09 added */
static iomux_v3_cfg_t const init_pads[] = {
	//NEW_PAD_CTRL(MX6_PAD_GPIO_0__CCM_CLKO1, OUTPUT_40OHM),	/* SGTL5000 sys_mclk */
	NEW_PAD_CTRL(MX6_PAD_NANDF_CS2__CCM_CLKO2, OUTPUT_40OHM),	/* J5 - Camera MCLK */

	/* wl1271 pads on nitrogen6x */
	/* WL12XX_WL_IRQ_GP */
	//NEW_PAD_CTRL(MX6_PAD_NANDF_CS1__GPIO6_IO14, WEAK_PULLDOWN),
	/* WL12XX_WL_ENABLE_GP */
	//NEW_PAD_CTRL(MX6_PAD_NANDF_CS2__GPIO6_IO15, OUTPUT_40OHM),
	/* WL12XX_BT_ENABLE_GP */
	//NEW_PAD_CTRL(MX6_PAD_NANDF_CS3__GPIO6_IO16, OUTPUT_40OHM),
	/* USB otg power */
	//NEW_PAD_CTRL(MX6_PAD_EIM_D22__GPIO3_IO22, OUTPUT_40OHM),
	//NEW_PAD_CTRL(MX6_PAD_NANDF_D5__GPIO2_IO05, OUTPUT_40OHM),
	//NEW_PAD_CTRL(MX6_PAD_NANDF_WP_B__GPIO6_IO09, OUTPUT_40OHM),
	//NEW_PAD_CTRL(MX6_PAD_GPIO_8__GPIO1_IO08, OUTPUT_40OHM),
	//NEW_PAD_CTRL(MX6_PAD_GPIO_6__GPIO1_IO06, OUTPUT_40OHM),
};

static unsigned gpios_out_low[] = {
	/* Disable wl1271 */
	//IMX_GPIO_NR(6, 15),	/* disable wireless */
	//IMX_GPIO_NR(6, 16), 	/* disable bluetooth */
	IMX_GPIO_NR(4, 15),	/* disable USB otg power */
	//IMX_GPIO_NR(2, 5),	/* ov5640 mipi camera reset */
	//IMX_GPIO_NR(1, 8),	/* ov5642 reset */
};


static unsigned gpios_out_high[] = {
	//IMX_GPIO_NR(1, 6),	/* ov5642 powerdown */
	//IMX_GPIO_NR(6, 9),	/* ov5640 mipi camera power down */
};

static void set_gpios(unsigned *p, int cnt, int val)
{
	int i;

	for (i = 0; i < cnt; i++)
		gpio_direction_output(*p++, val);
}

int board_early_init_f(void)
{
	setup_iomux_uart();

	set_gpios(gpios_out_high, ARRAY_SIZE(gpios_out_high), 1);
	set_gpios(gpios_out_low, ARRAY_SIZE(gpios_out_low), 0);

	imx_iomux_v3_setup_multiple_pads(init_pads, ARRAY_SIZE(init_pads));
	setup_buttons();

#if defined(CONFIG_VIDEO_IPUV3)
	setup_display();
#endif
	return 0;
}

/*
 * Do not overwrite the console
 * Use always serial for U-Boot console
 */
int overwrite_console(void)
{
	return 1;
}

int board_init(void)
{
	struct iomuxc_base_regs *const iomuxc_regs
		= (struct iomuxc_base_regs *)IOMUXC_BASE_ADDR;

	clrsetbits_le32(&iomuxc_regs->gpr[1],
			IOMUXC_GPR1_OTG_ID_MASK,
			IOMUXC_GPR1_OTG_ID_GPIO1);

	imx_iomux_v3_setup_multiple_pads(misc_pads, ARRAY_SIZE(misc_pads));

	/* address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM + 0x100;

#ifdef CONFIG_MXC_SPI
	setup_spi();
#endif
	setup_i2c(0, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pad_info0);
	setup_i2c(1, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pad_info1);
	setup_i2c(2, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pad_info2);

#ifdef CONFIG_CMD_SATA
	setup_sata();
#endif

	return 0;
}

static char const *board_type = "uninitialized";

/**
 *	Function: checkboard()
 *
 *	Inputs: void
 *	outputs: int
 *	Description: 
 *	  U-Boot Configuration defined function through CONFIG_DISPLAY_BOARDINFO
 *
 */
int checkboard(void)
{
	puts("Board: Aristeus\n");
	return 0;
}

struct button_key {
	char const	*name;
	unsigned	gpnum;
	char		ident;
};

/* Updated for Aristeus Keys as well*/
static struct button_key const buttons[] = {
	{"back",	IMX_GPIO_NR(2, 2),	'B'},
	{"home",	IMX_GPIO_NR(2, 0),	'H'},
	{"menu",	IMX_GPIO_NR(2, 1),	'M'},
	{"search",	IMX_GPIO_NR(2, 3),	'S'},
	{"volup",	IMX_GPIO_NR(6, 9),	'V'},
	{"voldown",	IMX_GPIO_NR(6, 10),	'v'},
};

/*
 * generate a null-terminated string containing the buttons pressed
 * returns number of keys pressed
 */
static int read_keys(char *buf)
{
	int i, numpressed = 0;
	for (i = 0; i < ARRAY_SIZE(buttons); i++) {
		if (!gpio_get_value(buttons[i].gpnum))
			buf[numpressed++] = buttons[i].ident;
	}
	buf[numpressed] = '\0';
	return numpressed;
}

static int do_kbd(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char envvalue[ARRAY_SIZE(buttons)+1];
	int numpressed = read_keys(envvalue);
	setenv("keybd", envvalue);
	return numpressed == 0;
}

U_BOOT_CMD(
	kbd, 1, 1, do_kbd,
	"Tests for keypresses, sets 'keybd' environment variable",
	"Returns 0 (true) to shell if key is pressed."
);

#ifdef CONFIG_PREBOOT
static char const kbd_magic_prefix[] = "key_magic";
static char const kbd_command_prefix[] = "key_cmd";

static void preboot_keys(void)
{
	int numpressed;
	char keypress[ARRAY_SIZE(buttons)+1];
	numpressed = read_keys(keypress);
	if (numpressed) {
		char *kbd_magic_keys = getenv("magic_keys");
		char *suffix;
		/*
		 * loop over all magic keys
		 */
		for (suffix = kbd_magic_keys; *suffix; ++suffix) {
			char *keys;
			char magic[sizeof(kbd_magic_prefix) + 1];
			sprintf(magic, "%s%c", kbd_magic_prefix, *suffix);
			keys = getenv(magic);
			if (keys) {
				if (!strcmp(keys, keypress))
					break;
			}
		}
		if (*suffix) {
			char cmd_name[sizeof(kbd_command_prefix) + 1];
			char *cmd;
			sprintf(cmd_name, "%s%c", kbd_command_prefix, *suffix);
			cmd = getenv(cmd_name);
			if (cmd) {
				setenv("preboot", cmd);
				return;
			}
		}
	}
}
#endif

#ifdef CONFIG_CMD_BMODE
static const struct boot_mode board_boot_modes[] = {
	/* 4 bit bus width */
	/* ordered as CFG1[7:0], CFG2[7:0], CFG3[7:0], CFG4[7:0]*/
	{"mmc0",	MAKE_CFGVAL(0x40, 0x30, 0x00, 0x00)},	//SD3
	{"mmc1",	MAKE_CFGVAL(0x40, 0x38, 0x00, 0x00)},	//SD4
	{"mmc2",	MAKE_CFGVAL(0x40, 0x28, 0x00, 0x00)},	//SD2
	{NULL,		0},
};
#endif

int misc_init_r(void)
{
#ifdef CONFIG_PREBOOT
	preboot_keys();
#endif

#ifdef CONFIG_CMD_BMODE
	add_board_boot_modes(board_boot_modes);
#endif
	return 0;
}

int board_late_init(void)
{
	int cpurev = get_cpu_rev();
	setenv("cpu",get_imx_type((cpurev & 0xFF000) >> 12));
	if (0 == getenv("board"))
		setenv("board",board_type);
	return 0;
}
