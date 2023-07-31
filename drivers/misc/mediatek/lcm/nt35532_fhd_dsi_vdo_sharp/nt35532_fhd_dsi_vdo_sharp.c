// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef BUILD_LK
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/string.h>
#include <linux/wait.h>
#include <linux/gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of_gpio.h>
#include <asm-generic/gpio.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/types.h>
#include <linux/vmalloc.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#endif
#endif
#include "lcm_drv.h"

#ifndef BUILD_LK
static unsigned int GPIO_LCD_RST; /* GPIO45: panel reset pin for control */
static unsigned int GPIO_LCD_PWR_EN; /* GPIO158: panel 1.8V for control */
static unsigned int GPIO_LCD_PWR2_EN; /* GPIO159: panel 2.8V for control */
static unsigned int GPIO_LCD_PWR; /* GPIO166: panel power enable */

static void lcm_request_gpio_control(struct device *dev)
{
	GPIO_LCD_RST = of_get_named_gpio(dev->of_node, "gpio_lcd_rst", 0);
	gpio_request(GPIO_LCD_RST, "GPIO_LCD_RST");
	pr_notice("[KE/LCM] GPIO_LCD_RST = 0x%x\n", GPIO_LCD_RST);

	GPIO_LCD_PWR = of_get_named_gpio(dev->of_node, "gpio_lcd_pwr", 0);
	gpio_request(GPIO_LCD_PWR, "GPIO_LCD_PWR");
	pr_notice("[KE/LCM] GPIO_LCD_PWR = 0x%x\n", GPIO_LCD_PWR);

	GPIO_LCD_PWR_EN = of_get_named_gpio(dev->of_node, "gpio_lcd_pwr_en", 0);
	gpio_request(GPIO_LCD_PWR_EN, "GPIO_LCD_PWR_EN");
	pr_notice("[KE/LCM] GPIO_LCD_PWR_EN = 0x%x\n", GPIO_LCD_PWR_EN);

	GPIO_LCD_PWR2_EN = of_get_named_gpio(dev->of_node, "gpio_lcd_pwr2_en",
					     0);
	gpio_request(GPIO_LCD_PWR2_EN, "GPIO_LCD_PWR2_EN");
	pr_notice("[KE/LCM] GPIO_LCD_PWR2_EN = 0x%x\n", GPIO_LCD_PWR2_EN);
}

static int lcm_driver_probe(struct device *dev, void const *data)
{
	pr_notice("[Kernel/LCM] %s\n", __func__);
	lcm_request_gpio_control(dev);

	return 0;
}

static const struct of_device_id lcm_platform_of_match[] = {
	{
		.compatible = "sharp,nt35532",
		.data = 0,
	}, {
		/* sentinel */
	}
};

MODULE_DEVICE_TABLE(of, platform_of_match);

static int lcm_platform_probe(struct platform_device *pdev)
{
	const struct of_device_id *id;

	id = of_match_node(lcm_platform_of_match, pdev->dev.of_node);
	if (!id)
		return -ENODEV;

	return lcm_driver_probe(&pdev->dev, id->data);
}

static struct platform_driver lcm_driver = {
	.probe = lcm_platform_probe,
	.driver = {
		.name = "nt35532_fhd_dsi_vdo_sharp",
		.owner = THIS_MODULE,
		.of_match_table = lcm_platform_of_match,
	},
};

static int __init lcm_init(void)
{
	if (platform_driver_register(&lcm_driver)) {
		pr_notice("LCM: failed to register this driver!\n");
		return -ENODEV;
	}

	return 0;
}

static void __exit lcm_exit(void)
{
	platform_driver_unregister(&lcm_driver);
}

late_initcall(lcm_init);
module_exit(lcm_exit);
MODULE_AUTHOR("mediatek");
MODULE_DESCRIPTION("LCM display subsystem driver");
MODULE_LICENSE("GPL");
#endif

/* ----------------------------------------------------------------- */
/* Local Constants */
/* ----------------------------------------------------------------- */
#define FRAME_WIDTH			(1080)
#define FRAME_HEIGHT		(1920)
#define GPIO_OUT_ONE		1
#define GPIO_OUT_ZERO		0

#define REGFLAG_DELAY		0xFE
#define REGFLAG_END_OF_TABLE	0xFF

#define LCM_DSI_CMD_MODE	0
#define LCM_ID_NT35532		0x80

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/* ----------------------------------------------------------------- */
/* Local Variables */
/* ----------------------------------------------------------------- */
static struct LCM_UTIL_FUNCS lcm_util = {0};
#define SET_RESET_PIN(v)	(lcm_util.set_reset_pin((v)))
#define UDELAY(n)		(lcm_util.udelay(n))
#define MDELAY(n)		(lcm_util.mdelay(n))

/* ----------------------------------------------------------------- */
/* Local Functions */
/* ----------------------------------------------------------------- */
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update) \
		lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update) \
		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)	lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums) \
		lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg	lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size) \
		lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

#ifdef BUILD_LK
#define LCM_LOGI(string, args...)  dprintf(CRITICAL, "[LK/"LOG_TAG"]"string, \
					   ##args)
#define LCM_LOGD(string, args...)  dprintf(INFO, "[LK/"LOG_TAG"]"string, ##args)
#else
#define LCM_LOGI(fmt, args...)  pr_notice("[KERNEL/"LOG_TAG"]"fmt, ##args)
#define LCM_LOGD(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
#endif

struct LCM_setting_table {
	unsigned char cmd;
	unsigned char count;
	unsigned char para_list[64];
};

static void lcm_set_gpio_output(unsigned int GPIO, unsigned int output)
{
#ifdef BUILD_LK
	mt_set_gpio_mode(GPIO, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO, output);
#else
	gpio_set_value(GPIO, output);
#endif
}

static struct LCM_setting_table lcm_initialization_setting[] = {
	/* Select page command*/
	{0xFF, 1, { 0x00 } },
	/* Non-reload command */
	{0xFB, 1, { 0x01 } },
	{REGFLAG_DELAY, 23, { } },

	/* Select page command*/
	{0xFF, 1, { 0x00 } },
	{0xD3, 1, { 0x08 } },
	{0xD4, 1, { 0x0E } },

	/* Sleep out*/
	{0x11, 0, { } },
	{REGFLAG_DELAY, 120, { } },

	/* Display on */
	{0x29, 0, { } },
	{REGFLAG_DELAY, 20, { } },

	/* Set LED light */
	{0x51, 1, { 0xFF } },
	/* Set LED on */
	{0x53, 1, { 0x2C } },
	/* Set CABC on */
	{0x55, 1, { 0x03 } },
};

static struct LCM_setting_table lcm_suspend_setting[] = {
	/* Turn off backlight */
	{0x53, 1, { 0x00 } },
	{REGFLAG_DELAY, 1, { } },

	/* Display off */
	{0x28, 0, { } },
	{REGFLAG_DELAY, 20, { } },

	/* Sleep in */
	{0x10, 0, { } },
	{REGFLAG_DELAY, 120, { } },

	/* Select page command */
	{0xFF, 1, { 0x00 } },
	{REGFLAG_DELAY, 1, { } },
};

static void push_table(struct LCM_setting_table *table, unsigned int count,
		       unsigned char force_update)
{
	unsigned int i;

	for (i = 0; i < count; i++) {
		unsigned int cmd;

		cmd = table[i].cmd;
		switch (cmd) {
		case REGFLAG_DELAY:
			MDELAY(table[i].count);
			break;

		case REGFLAG_END_OF_TABLE:
			break;

		default:
			dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list,
					force_update);
		}
	}
}

/* ----------------------------------------------------------------- */
/*  LCM Driver Implementations */
/* ----------------------------------------------------------------- */
static void lcm_set_util_funcs(const struct LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(struct LCM_UTIL_FUNCS));
}

static void lcm_get_params(struct LCM_PARAMS *params)
{
	memset(params, 0, sizeof(struct LCM_PARAMS));

	params->type   = LCM_TYPE_DSI;

	params->width  = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode   = CMD_MODE;
#else
	params->dsi.mode   = SYNC_PULSE_VDO_MODE;
#endif

	params->dsi.LANE_NUM				= LCM_FOUR_LANE;
	params->dsi.data_format.color_order	= LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq	= LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding		= LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format		= LCM_DSI_FORMAT_RGB888;

	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;

	params->dsi.vertical_sync_active	= 2;
	params->dsi.vertical_backporch		= 6;
	params->dsi.vertical_frontporch		= 14;
	params->dsi.vertical_active_line	= FRAME_HEIGHT;

	params->dsi.horizontal_sync_active	= 8;
	params->dsi.horizontal_backporch	= 16;
	params->dsi.horizontal_frontporch	= 72;
	params->dsi.horizontal_active_pixel	= FRAME_WIDTH;

	params->dsi.PLL_CLOCK		= 430;
}

static void lcm_init_lcm(void)
{
	pr_notice("[Kernel/LCM] %s\n", __func__);
	push_table(lcm_initialization_setting,
		   sizeof(lcm_initialization_setting) /
		   sizeof(struct LCM_setting_table), 1);
}

static void lcm_init_power(void)
{
#ifdef BUILD_LK
	printf("[LK/LCM] %s enter\n", __func__);

	lcm_set_gpio_output(GPIO_LCD_PWR, GPIO_OUT_ONE);
	MDELAY(20);

	lcm_set_gpio_output(GPIO_LCD_PWR_EN, GPIO_OUT_ONE);
	MDELAY(10);
	lcm_set_gpio_output(GPIO_LCD_PWR2_EN, GPIO_OUT_ONE);
	MDELAY(10);

	lcm_set_gpio_output(GPIO_LCD_RST, GPIO_OUT_ONE);
	MDELAY(30);
	lcm_set_gpio_output(GPIO_LCD_RST, GPIO_OUT_ZERO);
	MDELAY(2);
	lcm_set_gpio_output(GPIO_LCD_RST, GPIO_OUT_ONE);
	MDELAY(5);
#else
	pr_notice("[KERNEL/LCM] %s enter\n", __func__);
#endif
}

static void lcm_resume(void)
{
	pr_notice("[Kernel/LCM] %s\n", __func__);
	lcm_init_lcm();
}

static void lcm_resume_power(void)
{
	pr_notice("[Kernel/LCM] %s\n", __func__);
	lcm_set_gpio_output(GPIO_LCD_PWR, GPIO_OUT_ONE);
	MDELAY(20);

	lcm_set_gpio_output(GPIO_LCD_PWR_EN, GPIO_OUT_ONE);
	MDELAY(10);

	lcm_set_gpio_output(GPIO_LCD_PWR2_EN, GPIO_OUT_ONE);
	MDELAY(10);

	lcm_set_gpio_output(GPIO_LCD_RST, GPIO_OUT_ONE);
	MDELAY(10);
}

static void lcm_suspend(void)
{
	pr_notice("[Kernel/LCM] %s\n", __func__);
	push_table(lcm_suspend_setting,
		   sizeof(lcm_suspend_setting) /
		   sizeof(struct LCM_setting_table), 1);
}

static void lcm_suspend_power(void)
{
	pr_notice("[Kernel/LCM] %s\n", __func__);
	lcm_set_gpio_output(GPIO_LCD_RST, GPIO_OUT_ZERO);
	MDELAY(10);

	lcm_set_gpio_output(GPIO_LCD_PWR2_EN, GPIO_OUT_ZERO);
	MDELAY(10);

	lcm_set_gpio_output(GPIO_LCD_PWR_EN, GPIO_OUT_ZERO);
	MDELAY(10);
	lcm_set_gpio_output(GPIO_LCD_PWR, GPIO_OUT_ZERO);
}

struct LCM_DRIVER nt35532_fhd_dsi_vdo_sharp_lcm_drv = {
	.name			= "nt35532_fhd_dsi_vdo_sharp_lcm_drv",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init_lcm,
	.init_power     = lcm_init_power,
	.resume			= lcm_resume,
	.resume_power	= lcm_resume_power,
	.suspend		= lcm_suspend,
	.suspend_power	= lcm_suspend_power,
};

