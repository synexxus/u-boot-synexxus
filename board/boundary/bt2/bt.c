/*
 * Copyright (C) 2010-2013 Freescale Semiconductor, Inc.
 * Copyright (C) 2013, Boundary Devices <info@boundarydevices.com>
 *
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
#include <asm/imx-common/boot_mode.h>
#include <asm/imx-common/fbpanel.h>
#include <asm/imx-common/iomux-v3.h>
#include <asm/imx-common/mxc_i2c.h>
#include <asm/imx-common/sata.h>
#include <asm/imx-common/spi.h>
#include <mmc.h>
#include <fsl_esdhc.h>
#include <micrel.h>
#include <miiphy.h>
#include <netdev.h>
#include <linux/fb.h>
#include <ipu_pixfmt.h>
#include <asm/arch/crm_regs.h>
#include <asm/arch/mxc_hdmi.h>
#include <i2c.h>
#include <input.h>
#include <netdev.h>
#include <splash.h>
#include <usb/ehci-fsl.h>

DECLARE_GLOBAL_DATA_PTR;

#define AUD_PAD_CTRL  (PAD_CTL_PUS_100K_UP |			\
	PAD_CTL_SPEED_LOW | PAD_CTL_DSE_40ohm |			\
	PAD_CTL_HYS | PAD_CTL_SRE_FAST)

#define BUTTON_PAD_CTRL (PAD_CTL_PUS_100K_UP |			\
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm | PAD_CTL_HYS)

#define CEC_PAD_CTRL    (PAD_CTL_PUS_22K_UP |                   \
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm | PAD_CTL_HYS |   \
	PAD_CTL_ODE)

#define CSI_PAD_CTRL  (PAD_CTL_PUS_100K_UP |			\
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm |			\
	PAD_CTL_HYS | PAD_CTL_SRE_FAST)

#define ENET_PAD_CTRL  (PAD_CTL_PUS_100K_UP |			\
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm | PAD_CTL_HYS)

#define I2C_PAD_CTRL	(PAD_CTL_PUS_100K_UP |			\
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm | PAD_CTL_HYS |	\
	PAD_CTL_ODE | PAD_CTL_SRE_FAST)

#define OUTPUT_40OHM (PAD_CTL_SPEED_MED|PAD_CTL_DSE_40ohm)

#define OUTPUT_40OHM_PULLDN	(PAD_CTL_PUS_100K_DOWN |	\
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm |			\
	PAD_CTL_SRE_SLOW)

#define OUTPUT_40OHM_PULLUP	(PAD_CTL_PUS_100K_UP |		\
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm |			\
	PAD_CTL_SRE_SLOW)

#define RGB_PAD_CTRL	PAD_CTL_DSE_120ohm

#define SPI_PAD_CTRL (PAD_CTL_HYS | PAD_CTL_SPEED_MED |		\
	PAD_CTL_DSE_40ohm     | PAD_CTL_SRE_FAST)

#define UART_PAD_CTRL  (PAD_CTL_PUS_100K_UP |			\
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm |			\
	PAD_CTL_HYS | PAD_CTL_SRE_FAST)

#define USDHC1_PAD_CTRL (PAD_CTL_PUS_47K_UP |			\
	PAD_CTL_SPEED_LOW | PAD_CTL_DSE_80ohm |			\
	PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define USDHC2_PAD_CTRL	USDHC1_PAD_CTRL

#define USDHC3_PAD_CTRL (PAD_CTL_PUS_47K_UP |			\
	PAD_CTL_SPEED_LOW | PAD_CTL_DSE_40ohm |			\
	PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define USDHC4_PAD_CTRL	USDHC3_PAD_CTRL

#define WEAK_PULLDN	(PAD_CTL_PUS_100K_DOWN |		\
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm |			\
	PAD_CTL_HYS | PAD_CTL_SRE_SLOW)

#define WEAK_PULLUP	(PAD_CTL_PUS_100K_UP |			\
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm |			\
	PAD_CTL_HYS | PAD_CTL_SRE_SLOW)

/*
 *
 */
static const iomux_v3_cfg_t init_pads[] = {
	/* Aud5 - GS2971 audio input */
	IOMUX_PAD_CTRL(KEY_ROW1__AUD5_RXD, AUD_PAD_CTRL),	/* pin J3 - AOUT1/2 */
	IOMUX_PAD_CTRL(DISP0_DAT14__AUD5_RXC, AUD_PAD_CTRL),	/* pin J4 - ACLK*/
	IOMUX_PAD_CTRL(DISP0_DAT13__AUD5_RXFS, AUD_PAD_CTRL),	/* pin H4 - WCLK*/

	/* ECSPI1 */
	IOMUX_PAD_CTRL(EIM_D17__ECSPI1_MISO, SPI_PAD_CTRL),
	IOMUX_PAD_CTRL(EIM_D18__ECSPI1_MOSI, SPI_PAD_CTRL),
	IOMUX_PAD_CTRL(EIM_D16__ECSPI1_SCLK, SPI_PAD_CTRL),
#define GP_ECSPI1_NOR_CS	IMX_GPIO_NR(3, 19)
	IOMUX_PAD_CTRL(EIM_D19__GPIO3_IO19, WEAK_PULLUP),	/* SS1 */

	/* ECSPI3 - GS2971 */
	IOMUX_PAD_CTRL(DISP0_DAT2__ECSPI3_MISO, SPI_PAD_CTRL),	/* pin E7 - SDOUT */
	IOMUX_PAD_CTRL(DISP0_DAT1__ECSPI3_MOSI, SPI_PAD_CTRL),	/* pin E8 - SDIN */
	IOMUX_PAD_CTRL(DISP0_DAT0__ECSPI3_SCLK, SPI_PAD_CTRL),	/* pin F8 - SCLK */
#define GP_ECSPI3_GS2971_CS	IMX_GPIO_NR(4, 24)
	IOMUX_PAD_CTRL(DISP0_DAT3__GPIO4_IO24, WEAK_PULLUP),	/* 0 - pin F7 - CS0 */

	/* ecspi3 - gs2971, SDI on CSI1 */
#if defined(CONFIG_MX6S) || defined(CONFIG_MX6DL)
	/* Dualite/Solo doesn't have IPU2 */
	IOMUX_PAD_CTRL(EIM_DA9__IPU1_CSI1_DATA00, CSI_PAD_CTRL),	/* GPIO3[9] */
	IOMUX_PAD_CTRL(EIM_DA8__IPU1_CSI1_DATA01, CSI_PAD_CTRL),	/* GPIO3[8] */
	IOMUX_PAD_CTRL(EIM_DA7__IPU1_CSI1_DATA02, CSI_PAD_CTRL),	/* GPIO3[7] */
	IOMUX_PAD_CTRL(EIM_DA6__IPU1_CSI1_DATA03, CSI_PAD_CTRL),	/* GPIO3[6] */
	IOMUX_PAD_CTRL(EIM_DA5__IPU1_CSI1_DATA04, CSI_PAD_CTRL),	/* GPIO3[5] */
	IOMUX_PAD_CTRL(EIM_DA4__IPU1_CSI1_DATA05, CSI_PAD_CTRL),	/* GPIO3[4] */
	IOMUX_PAD_CTRL(EIM_DA3__IPU1_CSI1_DATA06, CSI_PAD_CTRL),	/* GPIO3[3] */
	IOMUX_PAD_CTRL(EIM_DA2__IPU1_CSI1_DATA07, CSI_PAD_CTRL),	/* GPIO3[2] */
	IOMUX_PAD_CTRL(EIM_DA1__IPU1_CSI1_DATA08, CSI_PAD_CTRL),	/* GPIO3[1] */
	IOMUX_PAD_CTRL(EIM_DA0__IPU1_CSI1_DATA09, CSI_PAD_CTRL),	/* GPIO3[0] */
	IOMUX_PAD_CTRL(EIM_EB1__IPU1_CSI1_DATA10, CSI_PAD_CTRL),	/* GPIO2[29] */
	IOMUX_PAD_CTRL(EIM_EB0__IPU1_CSI1_DATA11, CSI_PAD_CTRL),	/* GPIO2[28] */
	IOMUX_PAD_CTRL(EIM_A17__IPU1_CSI1_DATA12, CSI_PAD_CTRL),	/* GPIO2[21] */
	IOMUX_PAD_CTRL(EIM_A18__IPU1_CSI1_DATA13, CSI_PAD_CTRL),	/* GPIO2[20] */
	IOMUX_PAD_CTRL(EIM_A19__IPU1_CSI1_DATA14, CSI_PAD_CTRL),	/* GPIO2[19] */
	IOMUX_PAD_CTRL(EIM_A20__IPU1_CSI1_DATA15, CSI_PAD_CTRL),	/* GPIO2[18] */
	IOMUX_PAD_CTRL(EIM_A21__IPU1_CSI1_DATA16, CSI_PAD_CTRL),	/* GPIO2[17] */
	IOMUX_PAD_CTRL(EIM_A22__IPU1_CSI1_DATA17, CSI_PAD_CTRL),	/* GPIO2[16] */
	IOMUX_PAD_CTRL(EIM_A23__IPU1_CSI1_DATA18, CSI_PAD_CTRL),	/* GPIO6[6] */
	IOMUX_PAD_CTRL(EIM_A24__IPU1_CSI1_DATA19, CSI_PAD_CTRL),	/* GPIO2[30] */
	IOMUX_PAD_CTRL(EIM_A16__IPU1_CSI1_PIXCLK, CSI_PAD_CTRL),	/* GPIO2[22] */
#else
	IOMUX_PAD_CTRL(EIM_DA9__IPU2_CSI1_DATA00, CSI_PAD_CTRL),	/* GPIO3[9] */
	IOMUX_PAD_CTRL(EIM_DA8__IPU2_CSI1_DATA01, CSI_PAD_CTRL),	/* GPIO3[8] */
	IOMUX_PAD_CTRL(EIM_DA7__IPU2_CSI1_DATA02, CSI_PAD_CTRL),	/* GPIO3[7] */
	IOMUX_PAD_CTRL(EIM_DA6__IPU2_CSI1_DATA03, CSI_PAD_CTRL),	/* GPIO3[6] */
	IOMUX_PAD_CTRL(EIM_DA5__IPU2_CSI1_DATA04, CSI_PAD_CTRL),	/* GPIO3[5] */
	IOMUX_PAD_CTRL(EIM_DA4__IPU2_CSI1_DATA05, CSI_PAD_CTRL),	/* GPIO3[4] */
	IOMUX_PAD_CTRL(EIM_DA3__IPU2_CSI1_DATA06, CSI_PAD_CTRL),	/* GPIO3[3] */
	IOMUX_PAD_CTRL(EIM_DA2__IPU2_CSI1_DATA07, CSI_PAD_CTRL),	/* GPIO3[2] */
	IOMUX_PAD_CTRL(EIM_DA1__IPU2_CSI1_DATA08, CSI_PAD_CTRL),	/* GPIO3[1] */
	IOMUX_PAD_CTRL(EIM_DA0__IPU2_CSI1_DATA09, CSI_PAD_CTRL),	/* GPIO3[0] */
	IOMUX_PAD_CTRL(EIM_EB1__IPU2_CSI1_DATA10, CSI_PAD_CTRL),	/* GPIO2[29] */
	IOMUX_PAD_CTRL(EIM_EB0__IPU2_CSI1_DATA11, CSI_PAD_CTRL),	/* GPIO2[28] */
	IOMUX_PAD_CTRL(EIM_A17__IPU2_CSI1_DATA12, CSI_PAD_CTRL),	/* GPIO2[21] */
	IOMUX_PAD_CTRL(EIM_A18__IPU2_CSI1_DATA13, CSI_PAD_CTRL),	/* GPIO2[20] */
	IOMUX_PAD_CTRL(EIM_A19__IPU2_CSI1_DATA14, CSI_PAD_CTRL),	/* GPIO2[19] */
	IOMUX_PAD_CTRL(EIM_A20__IPU2_CSI1_DATA15, CSI_PAD_CTRL),	/* GPIO2[18] */
	IOMUX_PAD_CTRL(EIM_A21__IPU2_CSI1_DATA16, CSI_PAD_CTRL),	/* GPIO2[17] */
	IOMUX_PAD_CTRL(EIM_A22__IPU2_CSI1_DATA17, CSI_PAD_CTRL),	/* GPIO2[16] */
	IOMUX_PAD_CTRL(EIM_A23__IPU2_CSI1_DATA18, CSI_PAD_CTRL),	/* GPIO6[6] */
	IOMUX_PAD_CTRL(EIM_A24__IPU2_CSI1_DATA19, CSI_PAD_CTRL),	/* GPIO2[30] */
	IOMUX_PAD_CTRL(EIM_A16__IPU2_CSI1_PIXCLK, CSI_PAD_CTRL),	/* GPIO2[22] - pin A8 */
#endif

	/* Not used, but MUST be in GPIO mode */
	IOMUX_PAD_CTRL(EIM_DA10__GPIO3_IO10, WEAK_PULLUP),	/* IPU2_CSI1_DATA_EN not used (pin B5 stat2) */
	IOMUX_PAD_CTRL(EIM_DA11__GPIO3_IO11, WEAK_PULLUP),	/* HSYNC - pin A5 stat0 */
	IOMUX_PAD_CTRL(EIM_DA12__GPIO3_IO12, WEAK_PULLUP),	/* VSYNC - pin A6 stat1 */

#define GP_GS2971_STANDBY	IMX_GPIO_NR(5, 0)
	IOMUX_PAD_CTRL(EIM_WAIT__GPIO5_IO00, OUTPUT_40OHM_PULLUP), /* 1 - pin K2 - Standby */
#define GP_GS2971_RESET		IMX_GPIO_NR(3, 13)
	IOMUX_PAD_CTRL(EIM_DA13__GPIO3_IO13, OUTPUT_40OHM_PULLDN), /* 0 - pin C7 - reset */
#define GP_GS2971_RC_BYPASS	IMX_GPIO_NR(4, 27)
	IOMUX_PAD_CTRL(DISP0_DAT6__GPIO4_IO27, OUTPUT_40OHM),	/* 0 - pin G3 - RC bypass - output is buffered(low) */
#define GP_GS2971_IOPROC_EN	IMX_GPIO_NR(4, 28)
	IOMUX_PAD_CTRL(DISP0_DAT7__GPIO4_IO28, OUTPUT_40OHM),	/* 0 - pin H8 - io(A/V) processor enable */
#define GP_GS2971_AUDIO_EN	IMX_GPIO_NR(4, 29)
	IOMUX_PAD_CTRL(DISP0_DAT8__GPIO4_IO29, OUTPUT_40OHM),	/* 0 - pin H3 - Audio Enable */
#define GP_GS2971_TIM_861	IMX_GPIO_NR(4, 30)
	IOMUX_PAD_CTRL(DISP0_DAT9__GPIO4_IO30, OUTPUT_40OHM),	/* 0 - pin H5 - TIM861 timing format, 0-use HSYNC/VSYNC */
#define GP_GS2971_SW_EN		IMX_GPIO_NR(4, 31)
	IOMUX_PAD_CTRL(DISP0_DAT10__GPIO4_IO31, OUTPUT_40OHM),	/* 0 - pin D7 - SW_EN - line lock enable */
#define GP_GS2971_DVB_ASI	IMX_GPIO_NR(5, 5)
	IOMUX_PAD_CTRL(DISP0_DAT11__GPIO5_IO05, WEAK_PULLUP),	/* pin G8 i/o DVB_ASI */
#define GP_GS2971_SMPTE_BYPASS	IMX_GPIO_NR(2, 24)
	IOMUX_PAD_CTRL(EIM_CS1__GPIO2_IO24, WEAK_PULLUP),	/* pin G7 - i/o SMPTE bypass */
#define GP_GS2971_DVI_LOCK	IMX_GPIO_NR(3, 14)
	IOMUX_PAD_CTRL(EIM_DA14__GPIO3_IO14, WEAK_PULLUP),	/* pin B6 - stat3 - DVI_LOCK */
#define GP_GS2971_DATA_ERR	IMX_GPIO_NR(3, 15)
	IOMUX_PAD_CTRL(EIM_DA15__GPIO3_IO15, WEAK_PULLUP),	/* pin C6 - stat5 - DATA error */
#define GP_GS2971_LB_CONT	IMX_GPIO_NR(3, 20)
	IOMUX_PAD_CTRL(EIM_D20__GPIO3_IO20, WEAK_PULLUP),	/* pin A3 - LB control - float, analog input */
#define GP_GS2971_Y_1ANC	IMX_GPIO_NR(4, 26)
	IOMUX_PAD_CTRL(DISP0_DAT5__GPIO4_IO26, WEAK_PULLUP),	/* pin C5 - stat4 - 1ANC - Y signal detect */

	/* ENET pads that don't change for PHY reset */
	IOMUX_PAD_CTRL(ENET_MDIO__ENET_MDIO, ENET_PAD_CTRL),
	IOMUX_PAD_CTRL(ENET_MDC__ENET_MDC, ENET_PAD_CTRL),
	IOMUX_PAD_CTRL(RGMII_TXC__RGMII_TXC, ENET_PAD_CTRL),
	IOMUX_PAD_CTRL(RGMII_TD0__RGMII_TD0, ENET_PAD_CTRL),
	IOMUX_PAD_CTRL(RGMII_TD1__RGMII_TD1, ENET_PAD_CTRL),
	IOMUX_PAD_CTRL(RGMII_TD2__RGMII_TD2, ENET_PAD_CTRL),
	IOMUX_PAD_CTRL(RGMII_TD3__RGMII_TD3, ENET_PAD_CTRL),
	IOMUX_PAD_CTRL(RGMII_TX_CTL__RGMII_TX_CTL, ENET_PAD_CTRL),
	IOMUX_PAD_CTRL(ENET_REF_CLK__ENET_TX_CLK, ENET_PAD_CTRL),
	/* pin 42 PHY nRST */
#define GP_ENET_PHY_RESET	IMX_GPIO_NR(1, 27)
	IOMUX_PAD_CTRL(ENET_RXD0__GPIO1_IO27, OUTPUT_40OHM_PULLDN),
#define GPIRQ_ENET_PHY		IMX_GPIO_NR(1, 28)
	IOMUX_PAD_CTRL(ENET_TX_EN__GPIO1_IO28, WEAK_PULLUP),	/* Micrel RGMII Phy Interrupt */

	/* Hogs - Power control, high is off */
	/* Pull-up so that reset will leave high */
#define GP_PWR_J1	IMX_GPIO_NR(5, 9)
	IOMUX_PAD_CTRL(DISP0_DAT15__GPIO5_IO09, WEAK_PULLUP),
#define GP_PWR_J2	IMX_GPIO_NR(4, 25)
	IOMUX_PAD_CTRL(DISP0_DAT4__GPIO4_IO25, WEAK_PULLUP),
#define GP_PWR_J3	IMX_GPIO_NR(2, 23)
	IOMUX_PAD_CTRL(EIM_CS0__GPIO2_IO23, WEAK_PULLUP),
#define GP_PWR_J4	IMX_GPIO_NR(2, 25)
	IOMUX_PAD_CTRL(EIM_OE__GPIO2_IO25, WEAK_PULLUP),
#define GP_PWR_J5V	IMX_GPIO_NR(2, 27)
	IOMUX_PAD_CTRL(EIM_LBA__GPIO2_IO27, WEAK_PULLUP),	/* J6 new, J7 old board */
#define GP_PWR_J12V	IMX_GPIO_NR(2, 26)
	IOMUX_PAD_CTRL(EIM_RW__GPIO2_IO26, WEAK_PULLUP),	/* J7 new, J6 old board */

	/* Hogs - TW6869 mic bias */
#define GP_TW6869_MIC_BIAS_EN	IMX_GPIO_NR(3, 29)
	IOMUX_PAD_CTRL(EIM_D29__GPIO3_IO29, OUTPUT_40OHM_PULLDN),

	/* Hogs - Dry Contact */
	/* J92 pins */
#define GP_J92_PIN7	IMX_GPIO_NR(3, 31)
	IOMUX_PAD_CTRL(EIM_D31__GPIO3_IO31, WEAK_PULLUP),	/* OUT_1 - Dry contact to J92 pin 7 */
#define GP_J92_PIN9	IMX_GPIO_NR(1, 8)
	IOMUX_PAD_CTRL(GPIO_8__GPIO1_IO08, WEAK_PULLDN),	/* OUT_2 - Dry contact to J92 pin 9 */
#define GP_J92_PIN10	IMX_GPIO_NR(5, 22)
	IOMUX_PAD_CTRL(CSI0_DAT4__GPIO5_IO22, WEAK_PULLUP),	/* GPI_1 - J92 - pin 10 */
#define GP_J92_PIN12	IMX_GPIO_NR(5, 23)
	IOMUX_PAD_CTRL(CSI0_DAT5__GPIO5_IO23, WEAK_PULLUP),	/* GPI_2 - J92 - pin 12 */

	/* Hogs - Test point */
#define GP_TP77			IMX_GPIO_NR(5, 6)
	IOMUX_PAD_CTRL(DISP0_DAT12__GPIO5_IO06, WEAK_PULLUP),
#define GP_TP81			IMX_GPIO_NR(1, 3)
	IOMUX_PAD_CTRL(GPIO_3__GPIO1_IO03, WEAK_PULLUP),
#define GP_TP82			IMX_GPIO_NR(1, 0)
	IOMUX_PAD_CTRL(GPIO_0__GPIO1_IO00, WEAK_PULLUP),
	IOMUX_PAD_CTRL(GPIO_19__GPIO4_IO05, WEAK_PULLUP),
	IOMUX_PAD_CTRL(NANDF_CS2__GPIO6_IO15, WEAK_PULLDN),
	IOMUX_PAD_CTRL(NANDF_CS3__GPIO6_IO16, WEAK_PULLDN),
	IOMUX_PAD_CTRL(NANDF_WP_B__GPIO6_IO09, WEAK_PULLUP),
	IOMUX_PAD_CTRL(NANDF_D0__GPIO2_IO00, WEAK_PULLUP),
	IOMUX_PAD_CTRL(GPIO_18__GPIO7_IO13, OUTPUT_40OHM),

	/* i2c1 rtc rv4162 */
#define GPIRQ_RTC_RV4162	IMX_GPIO_NR(4, 11)
	IOMUX_PAD_CTRL(KEY_ROW2__GPIO4_IO11, WEAK_PULLUP),

	/* I2C3_Adv7391 */
#define GP_ADV7391_RESET	IMX_GPIO_NR(4, 20)
	IOMUX_PAD_CTRL(DI0_PIN4__GPIO4_IO20, OUTPUT_40OHM_PULLDN),	/* Adv7391 reset */

	/* i2c3 max-7w - GPS */
	IOMUX_PAD_CTRL(CSI0_DAT12__GPIO5_IO30, WEAK_PULLUP),
	IOMUX_PAD_CTRL(CSI0_DAT13__GPIO5_IO31, WEAK_PULLUP),
#define GP_GPS_HEART_BEAT	IMX_GPIO_NR(2, 30)
	IOMUX_PAD_CTRL(EIM_EB2__GPIO2_IO30, WEAK_PULLUP),
#define GPIRQ_GPS		IMX_GPIO_NR(2, 31)
	IOMUX_PAD_CTRL(EIM_EB3__GPIO2_IO31, WEAK_PULLUP),
#define GP_GPS_RESET		IMX_GPIO_NR(4, 10)
	IOMUX_PAD_CTRL(KEY_COL2__GPIO4_IO10, WEAK_PULLDN),

	/* hdmi_cec */
	IOMUX_PAD_CTRL(EIM_A25__HDMI_TX_CEC_LINE, CEC_PAD_CTRL),

	/*
	 * PCIe - tw6869 dedicated,
	 * VIN1-4 used,
	 * AIN1-2 amplified, AIN3-4 not amplified
	 */
#define GP_PCIE_RESET		IMX_GPIO_NR(4, 8)
	IOMUX_PAD_CTRL(KEY_COL1__GPIO4_IO08, OUTPUT_40OHM),

	/* regulator usgotg */
#define GP_REG_USBOTG		IMX_GPIO_NR(3, 22)
	IOMUX_PAD_CTRL(EIM_D22__GPIO3_IO22, OUTPUT_40OHM_PULLDN),

	/* reg_wlan_en */
#define GP_REG_WLAN_EN	IMX_GPIO_NR(6, 7)
	IOMUX_PAD_CTRL(NANDF_CLE__GPIO6_IO07, OUTPUT_40OHM_PULLDN),

	/* UART1 - J1 */
	IOMUX_PAD_CTRL(CSI0_DAT10__UART1_TX_DATA, UART_PAD_CTRL),
	IOMUX_PAD_CTRL(CSI0_DAT11__UART1_RX_DATA, UART_PAD_CTRL),

	/* UART2 for debug */
	IOMUX_PAD_CTRL(EIM_D26__UART2_TX_DATA, UART_PAD_CTRL),
	IOMUX_PAD_CTRL(EIM_D27__UART2_RX_DATA, UART_PAD_CTRL),

	/* UART3 - J2 */
	IOMUX_PAD_CTRL(EIM_D24__UART3_TX_DATA, UART_PAD_CTRL),
	IOMUX_PAD_CTRL(EIM_D25__UART3_RX_DATA, UART_PAD_CTRL),

	/* UART4 - J3 */
	IOMUX_PAD_CTRL(KEY_COL0__UART4_TX_DATA, UART_PAD_CTRL),
	IOMUX_PAD_CTRL(KEY_ROW0__UART4_RX_DATA, UART_PAD_CTRL),

	/* UART5 - J4 */
	IOMUX_PAD_CTRL(CSI0_DAT14__UART5_TX_DATA, UART_PAD_CTRL),
	IOMUX_PAD_CTRL(CSI0_DAT15__UART5_RX_DATA, UART_PAD_CTRL),

	/* USBH1 */
	IOMUX_PAD_CTRL(EIM_D30__USB_H1_OC, WEAK_PULLUP),
#define GP_USB_HUB_RESET	IMX_GPIO_NR(7, 12)
	IOMUX_PAD_CTRL(GPIO_17__GPIO7_IO12, OUTPUT_40OHM),	/* USB Hub Reset for USB2512 4 port hub */
	/*
	 * port1 - 10/100 ethernet using XR22800 on J90
	 * port2 - usb connector on J26
	 * port3 - usb connector on J25
	 * port4 - usb connector on J96
	 */

	/* USBOTG - J80 */
	IOMUX_PAD_CTRL(GPIO_1__USB_OTG_ID, WEAK_PULLUP),
	IOMUX_PAD_CTRL(KEY_COL4__USB_OTG_OC, WEAK_PULLUP),

	/* USDHC1: Full size SD card holder - J88 */
	IOMUX_PAD_CTRL(SD1_CLK__SD1_CLK, USDHC1_PAD_CTRL),
	IOMUX_PAD_CTRL(SD1_CMD__SD1_CMD, USDHC1_PAD_CTRL),
	IOMUX_PAD_CTRL(SD1_DAT0__SD1_DATA0, USDHC1_PAD_CTRL),
	IOMUX_PAD_CTRL(SD1_DAT1__SD1_DATA1, USDHC1_PAD_CTRL),
	IOMUX_PAD_CTRL(SD1_DAT2__SD1_DATA2, USDHC1_PAD_CTRL),
	IOMUX_PAD_CTRL(SD1_DAT3__SD1_DATA3, USDHC1_PAD_CTRL),
#define GP_USDHC1_CD		IMX_GPIO_NR(1, 4)
	IOMUX_PAD_CTRL(GPIO_4__GPIO1_IO04, WEAK_PULLUP),
#define GP_USDHC1_WP		IMX_GPIO_NR(1, 2)
	IOMUX_PAD_CTRL(GPIO_2__GPIO1_IO02, WEAK_PULLUP),

	/* USDHC2:  Full size sd - J87 */
	IOMUX_PAD_CTRL(SD2_CLK__SD2_CLK, USDHC2_PAD_CTRL),
	IOMUX_PAD_CTRL(SD2_CMD__SD2_CMD, USDHC2_PAD_CTRL),
	IOMUX_PAD_CTRL(SD2_DAT0__SD2_DATA0, USDHC2_PAD_CTRL),
	IOMUX_PAD_CTRL(SD2_DAT1__SD2_DATA1, USDHC2_PAD_CTRL),
	IOMUX_PAD_CTRL(SD2_DAT2__SD2_DATA2, USDHC2_PAD_CTRL),
	IOMUX_PAD_CTRL(SD2_DAT3__SD2_DATA3, USDHC2_PAD_CTRL),
#define GP_USDHC2_CD		IMX_GPIO_NR(3, 23)
	IOMUX_PAD_CTRL(EIM_D23__GPIO3_IO23, WEAK_PULLUP),
#define GP_USDHC2_WP		IMX_GPIO_NR(4, 15)
	IOMUX_PAD_CTRL(KEY_ROW4__GPIO4_IO15, WEAK_PULLUP),

	/* USDHC3 - eMMC */
	IOMUX_PAD_CTRL(SD3_CLK__SD3_CLK, USDHC3_PAD_CTRL),
	IOMUX_PAD_CTRL(SD3_CMD__SD3_CMD, USDHC3_PAD_CTRL),
	IOMUX_PAD_CTRL(SD3_DAT0__SD3_DATA0, USDHC3_PAD_CTRL),
	IOMUX_PAD_CTRL(SD3_DAT1__SD3_DATA1, USDHC3_PAD_CTRL),
	IOMUX_PAD_CTRL(SD3_DAT2__SD3_DATA2, USDHC3_PAD_CTRL),
	IOMUX_PAD_CTRL(SD3_DAT3__SD3_DATA3, USDHC3_PAD_CTRL),
	IOMUX_PAD_CTRL(SD3_DAT4__SD3_DATA4, USDHC3_PAD_CTRL),
	IOMUX_PAD_CTRL(SD3_DAT5__SD3_DATA5, USDHC3_PAD_CTRL),
	IOMUX_PAD_CTRL(SD3_DAT6__SD3_DATA6, USDHC3_PAD_CTRL),
	IOMUX_PAD_CTRL(SD3_DAT7__SD3_DATA7, USDHC3_PAD_CTRL),
#define GP_EMMC_RESET	IMX_GPIO_NR(7, 8)
	IOMUX_PAD_CTRL(SD3_RST__GPIO7_IO08, OUTPUT_40OHM),	/* eMMC reset */

	/* USDHC4 - Broadcom GB863021 */
	IOMUX_PAD_CTRL(SD4_CLK__SD4_CLK, USDHC4_PAD_CTRL),
	IOMUX_PAD_CTRL(SD4_CMD__SD4_CMD, USDHC4_PAD_CTRL),
	IOMUX_PAD_CTRL(SD4_DAT0__SD4_DATA0, USDHC4_PAD_CTRL),
	IOMUX_PAD_CTRL(SD4_DAT1__SD4_DATA1, USDHC4_PAD_CTRL),
	IOMUX_PAD_CTRL(SD4_DAT2__SD4_DATA2, USDHC4_PAD_CTRL),
	IOMUX_PAD_CTRL(SD4_DAT3__SD4_DATA3, USDHC4_PAD_CTRL),

#define GPIRQ_WL	IMX_GPIO_NR(6, 10)
	IOMUX_PAD_CTRL(NANDF_RB0__GPIO6_IO10, WEAK_PULLUP),
};

static const iomux_v3_cfg_t enet_pads1[] = {
	/* pin 35 - 1 (PHY_AD2) on reset */
#define GP_PHY_AD2		IMX_GPIO_NR(6, 30)
	IOMUX_PAD_CTRL(RGMII_RXC__GPIO6_IO30, OUTPUT_40OHM),
	/* pin 32 - 1 - (MODE0) all */
#define GP_PHY_MODE0		IMX_GPIO_NR(6, 25)
	IOMUX_PAD_CTRL(RGMII_RD0__GPIO6_IO25, OUTPUT_40OHM),
	/* pin 31 - 1 - (MODE1) all */
#define GP_PHY_MODE1		IMX_GPIO_NR(6, 27)
	IOMUX_PAD_CTRL(RGMII_RD1__GPIO6_IO27, OUTPUT_40OHM),
	/* pin 28 - 1 - (MODE2) all */
#define GP_PHY_MODE2		IMX_GPIO_NR(6, 28)
	IOMUX_PAD_CTRL(RGMII_RD2__GPIO6_IO28, OUTPUT_40OHM),
	/* pin 27 - 1 - (MODE3) all */
#define GP_PHY_MODE3		IMX_GPIO_NR(6, 29)
	IOMUX_PAD_CTRL(RGMII_RD3__GPIO6_IO29, OUTPUT_40OHM),
	/* pin 33 - 1 - (CLK125_EN) 125Mhz clockout enabled */
#define GP_PHY_CLK125		IMX_GPIO_NR(6, 24)
	IOMUX_PAD_CTRL(RGMII_RX_CTL__GPIO6_IO24, OUTPUT_40OHM),
};

static const iomux_v3_cfg_t enet_pads2[] = {
	IOMUX_PAD_CTRL(RGMII_RXC__RGMII_RXC, ENET_PAD_CTRL),
	IOMUX_PAD_CTRL(RGMII_RD0__RGMII_RD0, ENET_PAD_CTRL),
	IOMUX_PAD_CTRL(RGMII_RD1__RGMII_RD1, ENET_PAD_CTRL),
	IOMUX_PAD_CTRL(RGMII_RD2__RGMII_RD2, ENET_PAD_CTRL),
	IOMUX_PAD_CTRL(RGMII_RD3__RGMII_RD3, ENET_PAD_CTRL),
	IOMUX_PAD_CTRL(RGMII_RX_CTL__RGMII_RX_CTL, ENET_PAD_CTRL),
};

/*
 *
 */
static struct i2c_pads_info i2c_pads[] = {
	/* I2C1, SGTL5000 */
	I2C_PADS_INFO_ENTRY(I2C1, EIM_D21, 3, 21, EIM_D28, 3, 28, I2C_PAD_CTRL),
	/* I2C2 Camera, MIPI */
	I2C_PADS_INFO_ENTRY(I2C2, KEY_COL3, 4, 12, KEY_ROW3, 4, 13, I2C_PAD_CTRL),
	/* I2C3, J15 - RGB connector */
	I2C_PADS_INFO_ENTRY(I2C3, GPIO_5, 1, 05, GPIO_16, 7, 11, I2C_PAD_CTRL),
};

/* Adv7391 uses DSIP0_DAT16-23 */
static const iomux_v3_cfg_t rgb_pads[] = {
	IOMUX_PAD_CTRL(DI0_DISP_CLK__IPU1_DI0_DISP_CLK, RGB_PAD_CTRL),
	IOMUX_PAD_CTRL(DI0_PIN2__IPU1_DI0_PIN02, RGB_PAD_CTRL),		/* HSYNC */
	IOMUX_PAD_CTRL(DI0_PIN3__IPU1_DI0_PIN03, RGB_PAD_CTRL),		/* VSYNC */
	IOMUX_PAD_CTRL(DISP0_DAT16__IPU1_DISP0_DATA16, RGB_PAD_CTRL),
	IOMUX_PAD_CTRL(DISP0_DAT17__IPU1_DISP0_DATA17, RGB_PAD_CTRL),
	IOMUX_PAD_CTRL(DISP0_DAT18__IPU1_DISP0_DATA18, RGB_PAD_CTRL),
	IOMUX_PAD_CTRL(DISP0_DAT19__IPU1_DISP0_DATA19, RGB_PAD_CTRL),
	IOMUX_PAD_CTRL(DISP0_DAT20__IPU1_DISP0_DATA20, RGB_PAD_CTRL),
	IOMUX_PAD_CTRL(DISP0_DAT21__IPU1_DISP0_DATA21, RGB_PAD_CTRL),
	IOMUX_PAD_CTRL(DISP0_DAT22__IPU1_DISP0_DATA22, RGB_PAD_CTRL),
	IOMUX_PAD_CTRL(DISP0_DAT23__IPU1_DISP0_DATA23, RGB_PAD_CTRL),
};

static const iomux_v3_cfg_t rgb_gpio_pads[] = {
	IOMUX_PAD_CTRL(DI0_DISP_CLK__GPIO4_IO16, WEAK_PULLUP),
	IOMUX_PAD_CTRL(DI0_PIN2__GPIO4_IO18, WEAK_PULLUP),
	IOMUX_PAD_CTRL(DI0_PIN3__GPIO4_IO19, WEAK_PULLUP),
	IOMUX_PAD_CTRL(DISP0_DAT16__GPIO5_IO10, WEAK_PULLUP),
	IOMUX_PAD_CTRL(DISP0_DAT17__GPIO5_IO11, WEAK_PULLUP),
	IOMUX_PAD_CTRL(DISP0_DAT18__GPIO5_IO12, WEAK_PULLUP),
	IOMUX_PAD_CTRL(DISP0_DAT19__GPIO5_IO13, WEAK_PULLUP),
	IOMUX_PAD_CTRL(DISP0_DAT20__GPIO5_IO14, WEAK_PULLUP),
	IOMUX_PAD_CTRL(DISP0_DAT21__GPIO5_IO15, WEAK_PULLUP),
	IOMUX_PAD_CTRL(DISP0_DAT22__GPIO5_IO16, WEAK_PULLUP),
	IOMUX_PAD_CTRL(DISP0_DAT23__GPIO5_IO17, WEAK_PULLUP),
};

int dram_init(void)
{
	gd->ram_size = ((ulong)CONFIG_DDR_MB * 1024 * 1024);
//	printf("%s:%p *%p=0x%lx\n", __func__, gd, &gd->ram_size, gd->ram_size);
	return 0;
}

#ifdef CONFIG_USB_EHCI_MX6
int board_ehci_hcd_init(int port)
{
	/* Reset USB hub */
	if (port) {
		gpio_set_value(GP_USB_HUB_RESET, 0);
		mdelay(2);
		gpio_set_value(GP_USB_HUB_RESET, 1);
	}
	return 0;
}

int board_ehci_power(int port, int on)
{
	if (port)
		return 0;
	gpio_set_value(GP_REG_USBOTG, on);
	return 0;
}

#endif

#ifdef CONFIG_FSL_ESDHC
static struct fsl_esdhc_cfg usdhc_cfg[] = {
	{.esdhc_base = USDHC1_BASE_ADDR, .max_bus_width = 4},
	{.esdhc_base = USDHC2_BASE_ADDR, .max_bus_width = 4},
	{.esdhc_base = USDHC3_BASE_ADDR, .max_bus_width = 8},
};

int board_mmc_getcd(struct mmc *mmc)
{
	struct fsl_esdhc_cfg *cfg = (struct fsl_esdhc_cfg *)mmc->priv;
	int gp_cd = (cfg->esdhc_base == USDHC1_BASE_ADDR) ? GP_USDHC1_CD :
			GP_USDHC2_CD;

	if (cfg->esdhc_base == USDHC3_BASE_ADDR)
		return 1;	/* eMMC always present */
	return !gpio_get_value(gp_cd);
}

int board_mmc_init(bd_t *bis)
{
	int ret;
	u32 index = 0;

	usdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC_CLK);
	usdhc_cfg[1].sdhc_clk = mxc_get_clock(MXC_ESDHC2_CLK);
	usdhc_cfg[2].sdhc_clk = mxc_get_clock(MXC_ESDHC3_CLK);

	for (index = 0; index < CONFIG_SYS_FSL_USDHC_NUM; ++index) {
		switch (index) {
		case 0:
		case 1:
			break;
		case 2:
			gpio_set_value(GP_EMMC_RESET, 1); /* release reset */
			break;
		default:
			printf("Warning: you configured more USDHC controllers"
				"(%d) then supported by the board (%d)\n",
				index + 1, CONFIG_SYS_FSL_USDHC_NUM);
			return -EINVAL;
		}

		ret = fsl_esdhc_initialize(bis, &usdhc_cfg[index]);
		if (ret)
			return ret;
	}
	return 0;
}
#endif

int board_spi_cs_gpio(unsigned bus, unsigned cs)
{
	return (bus == 0 && cs == 0) ? GP_ECSPI1_NOR_CS : -1;
}

int board_phy_config(struct phy_device *phydev)
{
	/* min rx data delay */
	ksz9021_phy_extended_write(phydev,
			MII_KSZ9021_EXT_RGMII_RX_DATA_SKEW, 0x0);
	/* min tx data delay */
	ksz9021_phy_extended_write(phydev,
			MII_KSZ9021_EXT_RGMII_TX_DATA_SKEW, 0x0);
	/* max rx/tx clock delay, min rx/tx control */
	ksz9021_phy_extended_write(phydev,
			MII_KSZ9021_EXT_RGMII_CLOCK_SKEW, 0xf0f0);
	if (phydev->drv->config)
		phydev->drv->config(phydev);

	return 0;
}

static void setup_iomux_enet(void)
{
	gpio_direction_output(GP_ENET_PHY_RESET, 0); /* PHY rst */
	gpio_direction_output(GP_PHY_AD2, 1);
	gpio_direction_output(GP_PHY_MODE0, 1);
	gpio_direction_output(GP_PHY_MODE1, 1);
	gpio_direction_output(GP_PHY_MODE2, 1);
	gpio_direction_output(GP_PHY_MODE3, 1);
	gpio_direction_output(GP_PHY_CLK125, 1);
	SETUP_IOMUX_PADS(enet_pads1);

	/* Need delay 10ms according to KSZ9021 spec */
	udelay(1000 * 10);
	gpio_set_value(GP_ENET_PHY_RESET, 1); /* PHY reset */

	SETUP_IOMUX_PADS(enet_pads2);
	udelay(100);	/* Wait 100 us before using mii interface */
}

int board_eth_init(bd_t *bis)
{
	uint32_t base = IMX_FEC_BASE;
	struct mii_dev *bus = NULL;
	struct phy_device *phydev = NULL;
	int ret;

	setup_iomux_enet();

#ifdef CONFIG_FEC_MXC
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

#ifdef CONFIG_CI_UDC
	/* For otg ethernet*/
	usb_eth_initialize(bis);
#endif
	return 0;
}


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

#if defined(CONFIG_VIDEO_IPUV3)

static const struct display_info_t displays[] = {
	/* hdmi */
	IMX_VD50_1280_720M_60(HDMI, 1, 1),
	IMX_VD50_1920_1080M_60(HDMI, 0, 1),
	IMX_VD50_1024_768M_60(HDMI, 0, 1),
};

int board_cfb_skip(void)
{
	return NULL != getenv("novideo");
}
#endif


static const unsigned short gpios_out_low[] = {
	GP_GS2971_RESET,
	GP_GS2971_RC_BYPASS,
	GP_GS2971_IOPROC_EN,
	GP_GS2971_AUDIO_EN,
	GP_GS2971_TIM_861,
	GP_GS2971_SW_EN,
	GP_GS2971_DVB_ASI,
	GP_ENET_PHY_RESET,
	GP_TW6869_MIC_BIAS_EN,
	GP_J92_PIN9,
	GP_ADV7391_RESET,
	GP_PCIE_RESET,
	GP_REG_USBOTG,		/* disable USB otg power */
	GP_REG_WLAN_EN,
	GP_GPS_RESET,
	GP_USB_HUB_RESET,	/* disable hub */
	GP_EMMC_RESET,		/* hold in reset */
};

static const unsigned short gpios_out_high[] = {
	GP_ECSPI1_NOR_CS,	/* SS1 of spi nor */
	GP_ECSPI3_GS2971_CS,
	GP_GS2971_STANDBY,
	GP_PWR_J1,
	GP_PWR_J2,
	GP_PWR_J3,
	GP_PWR_J4,
	GP_PWR_J5V,
	GP_PWR_J12V,
	GP_J92_PIN7,
};

static const unsigned short gpios_in[] = {
	GP_GS2971_SMPTE_BYPASS,
	GP_GS2971_DVI_LOCK,
	GP_GS2971_DATA_ERR,
	GP_GS2971_LB_CONT,
	GP_GS2971_Y_1ANC,
	GPIRQ_ENET_PHY,
	GP_J92_PIN10,
	GP_J92_PIN12,
	GP_TP77,
	GP_TP81,
	GP_TP82,
	GPIRQ_RTC_RV4162,
	GP_GPS_HEART_BEAT,
	GPIRQ_GPS,
	GP_USDHC1_CD,
	GP_USDHC1_WP,
	GP_USDHC2_CD,
	GP_USDHC2_WP,
	GPIRQ_WL,
};

static void set_gpios_in(const unsigned short *p, int cnt)
{
	int i;

	for (i = 0; i < cnt; i++)
		gpio_direction_input(*p++);
}

static void set_gpios(const unsigned short *p, int cnt, int val)
{
	int i;

	for (i = 0; i < cnt; i++)
		gpio_direction_output(*p++, val);
}

int board_early_init_f(void)
{
	set_gpios_in(gpios_in, ARRAY_SIZE(gpios_in));
	set_gpios(gpios_out_high, ARRAY_SIZE(gpios_out_high), 1);
	set_gpios(gpios_out_low, ARRAY_SIZE(gpios_out_low), 0);
	SETUP_IOMUX_PADS(init_pads);
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
	int i;
	struct i2c_pads_info *p = i2c_pads + i2c_get_info_entry_offset();
	struct iomuxc *const iomuxc_regs = (struct iomuxc *)IOMUXC_BASE_ADDR;

	clrsetbits_le32(&iomuxc_regs->gpr[1],
			IOMUXC_GPR1_OTG_ID_MASK,
			IOMUXC_GPR1_OTG_ID_GPIO1);

	/* address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM + 0x100;
	for (i = 0; i < 3; i++) {
	        setup_i2c(i, CONFIG_SYS_I2C_SPEED, 0x7f, p);
		p += I2C_PADS_INFO_ENTRY_SPACING;
	}

#ifdef CONFIG_CMD_SATA
	setup_sata();
#endif

#ifdef CONFIG_CMD_FBPANEL
	fbp_setup_display(displays, ARRAY_SIZE(displays));
#endif
	return 0;
}


int checkboard(void)
{
	puts("Board: BT2\n");
	return 0;
}

#ifdef CONFIG_CMD_BMODE
static const struct boot_mode board_boot_modes[] = {
	/* 4 bit bus width */
	{"mmc0",	MAKE_CFGVAL(0x40, 0x30, 0x00, 0x00)},
	{"mmc1",	MAKE_CFGVAL(0x40, 0x38, 0x00, 0x00)},
	{NULL,		0},
};
#endif

int misc_init_r(void)
{
#ifdef CONFIG_CMD_BMODE
	add_board_boot_modes(board_boot_modes);
#endif
	return 0;
}

int board_late_init(void)
{
	int cpurev = get_cpu_rev();

	setenv("cpu", get_imx_type((cpurev & 0xFF000) >> 12));
	if (!getenv("board"))
		setenv("board", "bt2");
	if (!getenv("uboot_defconfig"))
		setenv("uboot_defconfig", CONFIG_DEFCONFIG);
	return 0;
}