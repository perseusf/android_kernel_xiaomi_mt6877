/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef __KPD_H__
#define __KPD_H__

#include <linux/ioctl.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/atomic.h>
#include <hal_kpd.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#ifdef CONFIG_LONG_PRESS_MODE_EN
#include <linux/jiffies.h>
#endif

struct keypad_dts_data {
	u32 kpd_key_debounce;
	u32 kpd_sw_pwrkey;
	u32 kpd_hw_pwrkey;
	u32 kpd_sw_rstkey;
	u32 kpd_hw_rstkey;
	u32 kpd_use_extend_type;
	u32 kpd_hw_map_num;
	u32 kpd_hw_init_map[KPD_NUM_KEYS];
	u32 kpd_hw_dl_key1;
	u32 kpd_hw_dl_key2;
	u32 kpd_hw_dl_key3;
	u32 kpd_hw_recovery_key;
	u32 kpd_hw_factory_key;
	u32 boot_mode;
};

extern struct keypad_dts_data kpd_dts_data;
extern int kpd_klog_en;
#ifdef CONFIG_LONG_PRESS_MODE_EN
extern atomic_t vol_down_long_press_flag;
#endif

#define KPD_DEBUG	1
#define SET_KPD_KCOL		_IO('k', 29)
#define KPD_SAY		"kpd: "

#if KPD_DEBUG
#define kpd_print(fmt, arg...)	do { \
	if (kpd_klog_en) \
		pr_debug(KPD_SAY fmt, ##arg); \
	} while (0)
#define kpd_info(fmt, arg...)	do { \
	if (kpd_klog_en) \
		pr_info(KPD_SAY fmt, ##arg); \
	} while (0)
#define kpd_notice(fmt, arg...)	 \
			pr_notice(KPD_SAY fmt, ##arg)

#else
#define kpd_print(fmt, arg...)	do {} while (0)
#define kpd_info(fmt, arg...)	do {} while (0)
#endif

#endif				/* __KPD_H__ */
