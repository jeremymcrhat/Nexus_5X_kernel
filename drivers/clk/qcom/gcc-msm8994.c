/* Copyright (c) 2013-2014, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/ctype.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <dt-bindings/clock/msm-clocks-8994.h>

#include "common.h"
#include "clk-regmap.h"
#include "clk-pll.h"
#include "clk-rcg.h"
#include "clk-branch.h"
#include "reset.h"
#include "vdd-level-8994.h"

enum {
    P_XO,
    P_GPLL0,
    P_GPLL4,
};

static const struct parent_map gcc_xo_gpll0_map[] = {
    { P_XO, 0 },
    { P_GPLL0, 1 },
};

static const char * const gcc_xo_gpll0[] = {
    "xo",
    "gpll0_vote",
};

static const struct parent_map gcc_xo_gpll0_gpll4_map[] = {
    { P_XO, 0 },
    { P_GPLL0, 1 },
    { P_GPLL4, 2 },
};

static const char * const gcc_xo_gpll0_gpll4[] = {
    "xo",
    "gpll0_vote",
    "gpll4_vote",
};

#define F(f, s, h, m, n) { (f), (s), (2 * (h) - 1), (m), (n) }

static struct clk_fixed_factor xo = {
    .mult = 1,
    .div = 1,
    .hw.init = &(struct clk_init_data){
        .name = "xo",
        .parent_names = (const char *[]){ "xo_board" },
        .num_parents = 1,
        .ops = &clk_fixed_factor_ops,
    },
};

static struct clk_pll gpll0 = {
    .status_reg = 0x0000,
    .status_bit = 30,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "gpll0",
        .parent_names = (const char *[]){ "xo" },
        .num_parents = 1,
        .ops = &clk_pll_ops,
    },
};

static struct clk_regmap gpll0_vote = {
    .enable_reg = 0x1480,
    .enable_mask = BIT(0),
    .hw.init = &(struct clk_init_data){
        .name = "gpll0_vote",
        .parent_names = (const char *[]){ "gpll0" },
        .num_parents = 1,
        .ops = &clk_pll_vote_ops,
    },
};

static struct clk_pll gpll4 = {
    .status_reg = 0x1DC0,
    .status_bit = 30,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "gpll4",
        .parent_names = (const char *[]){ "xo" },
        .num_parents = 1,
        .ops = &clk_pll_ops,
    },
};

static struct clk_regmap gpll4_vote = {
    .enable_reg = 0x1480,
    .enable_mask = BIT(4),
    .hw.init = &(struct clk_init_data){
        .name = "gpll4_vote",
        .parent_names = (const char *[]){ "gpll4" },
        .num_parents = 1,
        .ops = &clk_pll_vote_ops,
    },
};

static struct freq_tbl ftbl_ufs_axi_clk_src[] = {
    F(50000000, P_GPLL0, 12, 0, 0),
    F(100000000, P_GPLL0, 6, 0, 0),
    F(150000000, P_GPLL0, 4, 0, 0),
    F(171430000, P_GPLL0, 3.5,  0, 0),
    { }
};

static struct freq_tbl ftbl_ufs_axi_clk_src_v2[] = {
    F(50000000, P_GPLL0, 12, 0, 0),
    F(100000000, P_GPLL0, 6, 0, 0),
    F(150000000, P_GPLL0, 4, 0, 0),
    F(171430000, P_GPLL0, 3.5, 0, 0),
    F(200000000, P_GPLL0, 3, 0, 0),
    F(240000000, P_GPLL0, 2.5, 0, 0),
    { }
};

static struct clk_rcg2 ufs_axi_clk_src = {
    .cmd_rcgr = 0x1D68,
    .mnd_width = 8,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_ufs_axi_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "ufs_axi_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct freq_tbl ftbl_usb30_master_clk_src[] = {
    F(19200000, P_XO, 1, 0, 0),
    F(125000000, P_GPLL0, 1, 5, 24),
    { }
};

static struct clk_rcg2 usb30_master_clk_src = {
    .cmd_rcgr = 0x03D4,
    .mnd_width = 8,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_usb30_master_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "usb30_master_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct freq_tbl ftbl_blsp_i2c_apps_clk_src[] = {
    F(19200000, P_XO, 1, 0, 0),
    F(50000000, P_GPLL0, 12, 0, 0),
    { }
};

static struct clk_rcg2 blsp1_qup1_i2c_apps_clk_src = {
    .cmd_rcgr = 0x0660,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_blsp_i2c_apps_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "blsp1_qup1_i2c_apps_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct freq_tbl ftbl_blspqup_spi_apps_clk_src_v2[] = {
    F(960000, P_XO, 10, 1, 2),
    F(4800000, P_XO, 4, 0, 0),
    F(9600000, P_XO, 2, 0, 0),
    F(15000000, P_GPLL0, 10, 1, 4),
    F(19200000, P_XO, 1, 0, 0),
    F(24000000, P_GPLL0, 12.5, 1, 2),
    F(25000000, P_GPLL0, 12, 1, 2),
    F(48000000, P_GPLL0, 12.5, 0, 0),
    F(50000000, P_GPLL0, 12, 0, 0),
    { }
};

static struct freq_tbl ftbl_blsp1_qup1_spi_apps_clk_src[] = {
    F(960000, P_XO, 10, 1, 2),
    F(4800000, P_XO, 4, 0, 0),
    F(9600000, P_XO, 2, 0, 0),
    F(15000000, P_GPLL0, 10, 1, 4),
    F(19200000, P_XO, 1, 0, 0),
    F(24000000, P_GPLL0, 12.5, 1, 2),
    F(25000000, P_GPLL0, 12, 1, 2),
    F(48000000, P_GPLL0, 12.5, 0, 0),
    F(50000000, P_GPLL0, 12, 0, 0),
    { }
};

static struct clk_rcg2 blsp1_qup1_spi_apps_clk_src = {
    .cmd_rcgr = 0x064C,
    .mnd_width = 8,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_blsp1_qup1_spi_apps_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "blsp1_qup1_spi_apps_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct clk_rcg2 blsp1_qup2_i2c_apps_clk_src = {
    .cmd_rcgr = 0x06E0,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_blsp_i2c_apps_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "blsp1_qup2_i2c_apps_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct freq_tbl ftbl_blsp1_qup2_spi_apps_clk_src[] = {
    F(960000, P_XO, 10, 1, 2),
    F(4800000, P_XO, 4, 0, 0),
    F(9600000, P_XO, 2, 0, 0),
    F(15000000, P_GPLL0, 10, 1, 4),
    F(19200000, P_XO, 1, 0, 0),
    F(24000000, P_GPLL0, 12.5, 1, 2),
    F(25000000, P_GPLL0, 12, 1, 2),
    F(42860000, P_GPLL0, 14, 0, 0),
    F(46150000, P_GPLL0, 13, 0, 0),
    { }
};

static struct clk_rcg2 blsp1_qup2_spi_apps_clk_src = {
    .cmd_rcgr = 0x06CC,
    .mnd_width = 8,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_blsp1_qup2_spi_apps_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "blsp1_qup2_spi_apps_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct clk_rcg2 blsp1_qup3_i2c_apps_clk_src = {
    .cmd_rcgr = 0x0760,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_blsp_i2c_apps_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "blsp1_qup3_i2c_apps_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct freq_tbl ftbl_blsp1_qup3_spi_apps_clk_src[] = {
    F(960000, P_XO, 10, 1, 2),
    F(4800000, P_XO, 4, 0, 0),
    F(9600000, P_XO, 2, 0, 0),
    F(15000000, P_GPLL0, 10, 1, 4),
    F(19200000, P_XO, 1, 0, 0),
    F(24000000, P_GPLL0, 12.5, 1, 2),
    F(25000000, P_GPLL0, 12, 1, 2),
    F(42860000, P_GPLL0, 14, 0, 0),
    F(44440000, P_GPLL0, 13.5, 0, 0),
    { }
};

static struct clk_rcg2 blsp1_qup3_spi_apps_clk_src = {
    .cmd_rcgr = 0x074C,
    .mnd_width = 8,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_blsp1_qup3_spi_apps_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "blsp1_qup3_spi_apps_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct clk_rcg2 blsp1_qup4_i2c_apps_clk_src = {
    .cmd_rcgr = 0x07E0,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_blsp_i2c_apps_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "blsp1_qup4_i2c_apps_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct freq_tbl ftbl_blsp1_qup4_spi_apps_clk_src[] = {
    F(960000, P_XO, 10, 1, 2),
    F(4800000, P_XO, 4, 0, 0),
    F(9600000, P_XO, 2, 0, 0),
    F(15000000, P_GPLL0, 10, 1, 4),
    F(19200000, P_XO, 1, 0, 0),
    F(24000000, P_GPLL0, 12.5, 1, 2),
    F(25000000, P_GPLL0, 12, 1, 2),
    F(42860000, P_GPLL0, 14, 0, 0),
    F(44440000, P_GPLL0, 13.5, 0, 0),
    { }
};

static struct clk_rcg2 blsp1_qup4_spi_apps_clk_src = {
    .cmd_rcgr = 0x07CC,
    .mnd_width = 8,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_blsp1_qup4_spi_apps_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "blsp1_qup4_spi_apps_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct clk_rcg2 blsp1_qup5_i2c_apps_clk_src = {
    .cmd_rcgr = 0x0860,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_blsp_i2c_apps_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "blsp1_qup5_i2c_apps_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct freq_tbl ftbl_blsp1_qup5_spi_apps_clk_src[] = {
    F(960000, P_XO, 10, 1, 2),
    F(4800000, P_XO, 4, 0, 0),
    F(9600000, P_XO, 2, 0, 0),
    F(15000000, P_GPLL0, 10, 1, 4),
    F(19200000, P_XO, 1, 0, 0),
    F(24000000, P_GPLL0, 12.5, 1, 2),
    F(25000000, P_GPLL0, 12, 1, 2),
    F(40000000, P_GPLL0, 15, 0, 0),
    F(42860000, P_GPLL0, 14, 0, 0),
    { }
};

static struct clk_rcg2 blsp1_qup5_spi_apps_clk_src = {
    .cmd_rcgr = 0x084C,
    .mnd_width = 8,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_blsp1_qup5_spi_apps_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "blsp1_qup5_spi_apps_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct clk_rcg2 blsp1_qup6_i2c_apps_clk_src = {
    .cmd_rcgr = 0x08E0,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_blsp_i2c_apps_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "blsp1_qup6_i2c_apps_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct freq_tbl ftbl_blsp1_qup6_spi_apps_clk_src[] = {
    F(960000, P_XO, 10, 1, 2),
    F(4800000, P_XO, 4, 0, 0),
    F(9600000, P_XO, 2, 0, 0),
    F(15000000, P_GPLL0, 10, 1, 4),
    F(19200000, P_XO, 1, 0, 0),
    F(24000000, P_GPLL0, 12.5, 1, 2),
    F(25000000, P_GPLL0, 12, 1, 2),
    F(41380000, P_GPLL0, 14.5, 0, 0),
    F(42860000, P_GPLL0, 14, 0, 0),
    { }
};

static struct clk_rcg2 blsp1_qup6_spi_apps_clk_src = {
    .cmd_rcgr = 0x08CC,
    .mnd_width = 8,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_blsp1_qup6_spi_apps_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "blsp1_qup6_spi_apps_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct freq_tbl ftbl_blsp_uart_apps_clk_src[] = {
    F(3686400, P_GPLL0, 1, 96, 15625),
    F(7372800, P_GPLL0, 1, 192, 15625),
    F(14745600, P_GPLL0, 1, 384, 15625),
    F(16000000, P_GPLL0, 5, 2, 15),
    F(19200000, P_XO, 1, 0, 0),
    F(24000000, P_GPLL0, 5, 1, 5),
    F(32000000, P_GPLL0, 1, 4, 75),
    F(40000000, P_GPLL0, 15, 0, 0),
    F(46400000, P_GPLL0, 1, 29, 375),
    F(48000000, P_GPLL0, 12.5, 0, 0),
    F(51200000, P_GPLL0, 1, 32, 375),
    F(56000000, P_GPLL0, 1, 7, 75),
    F(58982400, P_GPLL0, 1, 1536, 15625),
    F(60000000, P_GPLL0, 10, 0, 0),
    F(63160000, P_GPLL0, 9.5, 0, 0),
    { }
};

static struct clk_rcg2 blsp1_uart1_apps_clk_src = {
    .cmd_rcgr = 0x068C,
    .mnd_width = 16,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_blsp_uart_apps_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "blsp1_uart1_apps_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct clk_rcg2 blsp1_uart2_apps_clk_src = {
    .cmd_rcgr = 0x070C,
    .mnd_width = 16,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_blsp_uart_apps_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "blsp1_uart2_apps_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct clk_rcg2 blsp1_uart3_apps_clk_src = {
    .cmd_rcgr = 0x078C,
    .mnd_width = 16,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_blsp_uart_apps_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "blsp1_uart3_apps_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct clk_rcg2 blsp1_uart4_apps_clk_src = {
    .cmd_rcgr = 0x080C,
    .mnd_width = 16,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_blsp_uart_apps_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "blsp1_uart4_apps_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct clk_rcg2 blsp1_uart5_apps_clk_src = {
    .cmd_rcgr = 0x088C,
    .mnd_width = 16,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_blsp_uart_apps_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "blsp1_uart5_apps_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct clk_rcg2 blsp1_uart6_apps_clk_src = {
    .cmd_rcgr = 0x090C,
    .mnd_width = 16,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_blsp_uart_apps_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "blsp1_uart6_apps_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct clk_rcg2 blsp2_qup1_i2c_apps_clk_src = {
    .cmd_rcgr = 0x09A0,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_blsp_i2c_apps_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "blsp1_uart6_apps_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct freq_tbl ftbl_blsp2_qup1_spi_apps_clk_src[] = {
    F(960000, P_XO, 10, 1, 2),
    F(4800000, P_XO, 4, 0, 0),
    F(9600000, P_XO, 2, 0, 0),
    F(15000000, P_GPLL0, 10, 1, 4),
    F(19200000, P_XO, 1, 0, 0),
    F(24000000, P_GPLL0, 12.5, 1, 2),
    F(25000000, P_GPLL0, 12, 1, 2),
    F(42860000, P_GPLL0, 14, 0, 0),
    F(44440000, P_GPLL0, 13.5, 0, 0),
    { }
};

static struct clk_rcg2 blsp2_qup1_spi_apps_clk_src = {
    .cmd_rcgr = 0x098C,
    .mnd_width = 8,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_blsp2_qup1_spi_apps_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "blsp2_qup1_spi_apps_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct clk_rcg2 blsp2_qup2_i2c_apps_clk_src = {
    .cmd_rcgr = 0x0A20,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_blsp_i2c_apps_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "blsp2_qup2_i2c_apps_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct freq_tbl ftbl_blsp2_qup2_spi_apps_clk_src[] = {
    F(960000, P_XO, 10, 1, 2),
    F(4800000, P_XO, 4, 0, 0),
    F(9600000, P_XO, 2, 0, 0),
    F(15000000, P_GPLL0, 10, 1, 4),
    F(19200000, P_XO, 1, 0, 0),
    F(24000000, P_GPLL0, 12.5, 1, 2),
    F(25000000, P_GPLL0, 12, 1, 2),
    F(42860000, P_GPLL0, 14, 0, 0),
    F(44440000, P_GPLL0, 13.5, 0, 0),
    { }
};

static struct clk_rcg2 blsp2_qup2_spi_apps_clk_src = {
    .cmd_rcgr = 0x0A0C,
    .mnd_width = 8,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_blsp2_qup2_spi_apps_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "blsp2_qup2_spi_apps_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct clk_rcg2 blsp2_qup3_i2c_apps_clk_src = {
    .cmd_rcgr = 0x0AA0,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_blsp_i2c_apps_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "blsp2_qup3_i2c_apps_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct freq_tbl ftbl_blsp2_qup3_spi_apps_clk_src[] = {
    F(960000, P_XO, 10, 1, 2),
    F(4800000, P_XO, 4, 0, 0),
    F(9600000, P_XO, 2, 0, 0),
    F(15000000, P_GPLL0, 10, 1, 4),
    F(19200000, P_XO, 1, 0, 0),
    F(24000000, P_GPLL0, 12.5, 1, 2),
    F(25000000, P_GPLL0, 12, 1, 2),
    F(42860000, P_GPLL0, 14, 0, 0),
    F(48000000, P_GPLL0, 12.5, 0, 0),
    { }
};

static struct clk_rcg2 blsp2_qup3_spi_apps_clk_src = {
    .cmd_rcgr = 0x0A8C,
    .mnd_width = 8,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_blsp2_qup3_spi_apps_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "blsp2_qup3_spi_apps_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct clk_rcg2 blsp2_qup4_i2c_apps_clk_src = {
    .cmd_rcgr = 0x0B20,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_blsp_i2c_apps_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "blsp2_qup4_i2c_apps_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct freq_tbl ftbl_blsp2_qup4_spi_apps_clk_src[] = {
    F(960000, P_XO, 10, 1, 2),
    F(4800000, P_XO, 4, 0, 0),
    F(9600000, P_XO, 2, 0, 0),
    F(15000000, P_GPLL0, 10, 1, 4),
    F(19200000, P_XO, 1, 0, 0),
    F(24000000, P_GPLL0, 12.5, 1, 2),
    F(25000000, P_GPLL0, 12, 1, 2),
    F(42860000, P_GPLL0, 14, 0, 0),
    F(48000000, P_GPLL0, 12.5, 0, 0),
    { }
};

static struct clk_rcg2 blsp2_qup4_spi_apps_clk_src = {
    .cmd_rcgr = 0x0B0C,
    .mnd_width = 8,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_blsp2_qup4_spi_apps_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "blsp2_qup4_spi_apps_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct clk_rcg2 blsp2_qup5_i2c_apps_clk_src = {
    .cmd_rcgr = 0x0BA0,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_blsp_i2c_apps_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "blsp2_qup5_i2c_apps_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct freq_tbl ftbl_blsp2_qup5_spi_apps_clk_src[] = {
    F(960000, P_XO, 10, 1, 2),
    F(4800000, P_XO, 4, 0, 0),
    F(9600000, P_XO, 2, 0, 0),
    F(15000000, P_GPLL0, 10, 1, 4),
    F(19200000, P_XO, 1, 0, 0),
    F(24000000, P_GPLL0, 12.5, 1, 2),
    F(25000000, P_GPLL0, 12, 1, 2),
    F(48000000, P_GPLL0, 12.5, 0, 0),
    F(50000000, P_GPLL0, 12, 0, 0),
    { }
};

static struct clk_rcg2 blsp2_qup5_spi_apps_clk_src = {
    .cmd_rcgr = 0x0B8C,
    .mnd_width = 8,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_blsp2_qup5_spi_apps_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "blsp2_qup5_spi_apps_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct clk_rcg2 blsp2_qup6_i2c_apps_clk_src = {
    .cmd_rcgr = 0x0C20,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_blsp_i2c_apps_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "blsp2_qup6_i2c_apps_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct freq_tbl ftbl_blsp2_qup6_spi_apps_clk_src[] = {
    F(960000, P_XO, 10, 1, 2),
    F(4800000, P_XO, 4, 0, 0),
    F(9600000, P_XO, 2, 0, 0),
    F(15000000, P_GPLL0, 10, 1, 4),
    F(19200000, P_XO, 1, 0, 0),
    F(24000000, P_GPLL0, 12.5, 1, 2),
    F(25000000, P_GPLL0, 12, 1, 2),
    F(44440000, P_GPLL0, 13.5, 0, 0),
    F(48000000, P_GPLL0, 12.5, 0, 0),
    { }
};

static struct clk_rcg2 blsp2_qup6_spi_apps_clk_src = {
    .cmd_rcgr = 0x0C0C,
    .mnd_width = 8,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_blsp2_qup6_spi_apps_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "blsp2_qup6_spi_apps_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct clk_rcg2 blsp2_uart1_apps_clk_src = {
    .cmd_rcgr = 0x09CC,
    .mnd_width = 16,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_blsp_uart_apps_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "blsp2_uart1_apps_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct clk_rcg2 blsp2_uart2_apps_clk_src = {
    .cmd_rcgr = 0x0A4C,
    .mnd_width = 16,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_blsp_uart_apps_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "blsp2_uart2_apps_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct clk_rcg2 blsp2_uart3_apps_clk_src = {
    .cmd_rcgr = 0x0ACC,
    .mnd_width = 16,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_blsp_uart_apps_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "blsp2_uart3_apps_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct clk_rcg2 blsp2_uart4_apps_clk_src = {
    .cmd_rcgr = 0x0B4C,
    .mnd_width = 16,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_blsp_uart_apps_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "blsp2_uart4_apps_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct clk_rcg2 blsp2_uart5_apps_clk_src = {
    .cmd_rcgr = 0x0BCC,
    .mnd_width = 16,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_blsp_uart_apps_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "blsp2_uart5_apps_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct clk_rcg2 blsp2_uart6_apps_clk_src = {
    .cmd_rcgr = 0x0C4C,
    .mnd_width = 16,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_blsp_uart_apps_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "blsp2_uart6_apps_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct freq_tbl ftbl_gp1_clk_src[] = {
    F(19200000, P_XO, 1, 0, 0),
    F(100000000, P_GPLL0, 6, 0, 0),
    F(200000000, P_GPLL0, 3, 0, 0),
    { }
};

static struct clk_rcg2 gp1_clk_src = {
    .cmd_rcgr = 0x1904,
    .mnd_width = 8,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_gp1_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "gp1_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct freq_tbl ftbl_gp2_clk_src[] = {
    F(19200000, P_XO, 1, 0, 0),
    F(100000000, P_GPLL0, 6, 0, 0),
    F(200000000, P_GPLL0, 3, 0, 0),
    { }
};

static struct clk_rcg2 gp2_clk_src = {
    .cmd_rcgr = 0x1944,
    .mnd_width = 8,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_gp2_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "gp2_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct freq_tbl ftbl_gp3_clk_src[] = {
    F(19200000, P_XO, 1, 0, 0),
    F(100000000, P_GPLL0, 6, 0, 0),
    F(200000000, P_GPLL0, 3, 0, 0),
    { }
};

static struct clk_rcg2 gp3_clk_src = {
    .cmd_rcgr = 0x1984,
    .mnd_width = 8,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_gp3_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "gp3_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct freq_tbl ftbl_pcie_0_aux_clk_src[] = {
    F(1011000, P_XO, 1, 1, 19),
    { }
};

static struct clk_rcg2 pcie_0_aux_clk_src = {
    .cmd_rcgr = 0x1B00,
    .mnd_width = 8,
    .hid_width = 5,
    .freq_tbl = ftbl_pcie_0_aux_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "pcie_0_aux_clk_src",
        .parent_names = (const char *[]){ "xo" },
        .num_parents = 1,
        .ops = &clk_rcg2_ops,
    },
};

static struct freq_tbl ftbl_pcie_pipe_clk_src[] = {
    F(125000000, P_XO, 1, 0, 0),
    { }
};

static struct clk_rcg2 pcie_0_pipe_clk_src = {
    .cmd_rcgr = 0x1ADC,
    .hid_width = 5,
    .freq_tbl = ftbl_pcie_pipe_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "pcie_0_pipe_clk_src",
        .parent_names = (const char *[]){ "xo" },
        .num_parents = 1,
        .ops = &clk_rcg2_ops,
    },
};

static struct freq_tbl ftbl_pcie_1_aux_clk_src[] = {
    F(1011000, P_XO, 1, 1, 19),
    { }
};

static struct clk_rcg2 pcie_1_aux_clk_src = {
    .cmd_rcgr = 0x1B80,
    .mnd_width = 8,
    .hid_width = 5,
    .freq_tbl = ftbl_pcie_1_aux_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "pcie_1_aux_clk_src",
        .parent_names = (const char *[]){ "xo" },
        .num_parents = 1,
        .ops = &clk_rcg2_ops,
    },
};

static struct clk_rcg2 pcie_1_pipe_clk_src = {
    .cmd_rcgr = 0x1B5C,
    .hid_width = 5,
    .freq_tbl = ftbl_pcie_pipe_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "pcie_1_pipe_clk_src",
        .parent_names = (const char *[]){ "xo" },
        .num_parents = 1,
        .ops = &clk_rcg2_ops,
    },
};

static struct freq_tbl ftbl_pdm2_clk_src[] = {
    F(60000000, P_GPLL0, 10, 0, 0),
    { }
};

static struct clk_rcg2 pdm2_clk_src = {
    .cmd_rcgr = 0x0CD0,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_gp3_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "pdm2_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct freq_tbl ftbl_sdcc1_apps_clk_src[] = {
    F(144000, P_XO, 16, 3, 25),
    F(400000, P_XO, 12, 1, 4),
    F(20000000, P_GPLL0, 15, 1, 2),
    F(25000000, P_GPLL0, 12, 1, 2),
    F(50000000, P_GPLL0, 12, 0, 0),
    F(100000000, P_GPLL0, 6, 0, 0),
    F(192000000, P_GPLL4, 2, 0, 0),
    F(384000000, P_GPLL4, 1, 0, 0),
    { }
};

static struct clk_rcg2 sdcc1_apps_clk_src = {
    .cmd_rcgr = 0x04D0,
    .mnd_width = 8,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_gpll4_map,
    .freq_tbl = ftbl_sdcc1_apps_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "sdcc1_apps_clk_src",
        .parent_names = gcc_xo_gpll0_gpll4,
        .num_parents = 3,
        .ops = &clk_rcg2_ops,
    },
};

static struct freq_tbl ftbl_sdcc2_4_apps_clk_src[] = {
    F(144000, P_XO, 16, 3, 25),
    F(400000, P_XO, 12, 1, 4),
    F(20000000, P_GPLL0, 15, 1, 2),
    F(25000000, P_GPLL0, 12, 1, 2),
    F(50000000, P_GPLL0, 12, 0, 0),
    F(100000000, P_GPLL0, 6, 0, 0),
    F(200000000, P_GPLL0, 3, 0, 0),
    { }
};

static struct clk_rcg2 sdcc2_apps_clk_src = {
    .cmd_rcgr = 0x0510,
    .mnd_width = 8,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_sdcc2_4_apps_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "sdcc2_apps_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct clk_rcg2 sdcc3_apps_clk_src = {
    .cmd_rcgr = 0x0550,
    .mnd_width = 8,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_sdcc2_4_apps_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "sdcc3_apps_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct clk_rcg2 sdcc4_apps_clk_src = {
    .cmd_rcgr = 0x0590,
    .mnd_width = 8,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_sdcc2_4_apps_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "sdcc4_apps_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct freq_tbl ftbl_tsif_ref_clk_src[] = {
    F(105500, P_XO, 1, 1, 182),
    { }
};

static struct clk_rcg2 tsif_ref_clk_src = {
    .cmd_rcgr = 0x0D90,
    .mnd_width = 8,
    .hid_width = 5,
    .freq_tbl = ftbl_tsif_ref_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "tsif_ref_clk_src",
        .parent_names = (const char *[]){ "xo" },
        .num_parents = 1,
        .ops = &clk_rcg2_ops,
    },
};

static struct freq_tbl ftbl_usb30_mock_utmi_clk_src[] = {
    F(19200000, P_XO, 1, 0, 0),
    F(60000000, P_GPLL0, 10, 0, 0),
    { }
};

static struct clk_rcg2 usb30_mock_utmi_clk_src = {
    .cmd_rcgr = 0x03E8,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_usb30_mock_utmi_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "usb30_mock_utmi_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

static struct freq_tbl ftbl_usb3_phy_aux_clk_src[] = {
    F(1200000, P_XO, 16, 0, 0),
    { }
};

static struct clk_rcg2 usb3_phy_aux_clk_src = {
    .cmd_rcgr = 0x1414,
    .hid_width = 5,
    .freq_tbl = ftbl_usb3_phy_aux_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "usb3_phy_aux_clk_src",
        .parent_names = (const char *[]){ "xo" },
        .num_parents = 1,
        .ops = &clk_rcg2_ops,
    },
};

static struct freq_tbl ftbl_usb_hs_system_clk_src[] = {
    F(75000000, P_GPLL0, 8, 0, 0),
    { }
};

static struct clk_rcg2 usb_hs_system_clk_src = {
    .cmd_rcgr = 0x0490,
    .hid_width = 5,
    .parent_map = gcc_xo_gpll0_map,
    .freq_tbl = ftbl_usb_hs_system_clk_src,
    .clkr.hw.init = &(struct clk_init_data){
        .name = "usb_hs_system_clk_src",
        .parent_names = gcc_xo_gpll0,
        .num_parents = 2,
        .ops = &clk_rcg2_ops,
    },
};

// TODO, removed all gate cloks, all reset clocks, clock gcc_bam_dma_ahb_clk and gcc_blsp1_ahb_clk

static struct clk_branch gcc_blsp1_qup1_i2c_apps_clk = {
    .halt_reg = 0x0648,
    .clkr = {
        .enable_reg = 0x0648,
        .enable_mask = BIT(0),
        .hw.init = &(struct clk_init_data){
            .name = "gcc_blsp1_qup1_i2c_apps_clk",
            .parent_names = (const char *[]){
                "blsp1_qup1_i2c_apps_clk_src",
            },
            .num_parents = 1,
            .flags = CLK_SET_RATE_PARENT,
            .ops = &clk_branch2_ops,
        },
    },
};

static struct clk_branch gcc_blsp1_qup1_spi_apps_clk = {
    .halt_reg = 0x0644,
    .clkr = {
        .enable_reg = 0x0644,
        .enable_mask = BIT(0),
        .hw.init = &(struct clk_init_data){
            .name = "gcc_blsp1_qup1_spi_apps_clk",
            .parent_names = (const char *[]){
                "blsp1_qup1_spi_apps_clk_src",
            },
            .num_parents = 1,
            .flags = CLK_SET_RATE_PARENT,
            .ops = &clk_branch2_ops,
        },
    },
};

static struct clk_branch gcc_blsp1_qup2_i2c_apps_clk = {
    .halt_reg = 0x06C8,
    .clkr = {
        .enable_reg = 0x06C8,
        .enable_mask = BIT(0),
        .hw.init = &(struct clk_init_data){
            .name = "gcc_blsp1_qup2_i2c_apps_clk",
            .parent_names = (const char *[]){
                "blsp1_qup2_i2c_apps_clk_src",
            },
            .num_parents = 1,
            .flags = CLK_SET_RATE_PARENT,
            .ops = &clk_branch2_ops,
        },
    },
};

static struct clk_branch gcc_blsp1_qup2_spi_apps_clk = {
    .halt_reg = 0x06C4,
    .clkr = {
        .enable_reg = 0x06C4,
        .enable_mask = BIT(0),
        .hw.init = &(struct clk_init_data){
            .name = "gcc_blsp1_qup2_spi_apps_clk",
            .parent_names = (const char *[]){
                "blsp1_qup2_spi_apps_clk_src",
            },
            .num_parents = 1,
            .flags = CLK_SET_RATE_PARENT,
            .ops = &clk_branch2_ops,
        },
    },
};

static struct clk_branch gcc_blsp1_qup3_i2c_apps_clk = {
    .halt_reg = 0x0748,
    .clkr = {
        .enable_reg = 0x0748,
        .enable_mask = BIT(0),
        .hw.init = &(struct clk_init_data){
            .name = "gcc_blsp1_qup3_i2c_apps_clk",
            .parent_names = (const char *[]){
                "blsp1_qup3_i2c_apps_clk_src",
            },
            .num_parents = 1,
            .flags = CLK_SET_RATE_PARENT,
            .ops = &clk_branch2_ops,
        },
    },
};

static struct clk_branch gcc_blsp1_qup3_spi_apps_clk = {
    .halt_reg = 0x0744,
    .clkr = {
        .enable_reg = 0x0744,
        .enable_mask = BIT(0),
        .hw.init = &(struct clk_init_data){
            .name = "gcc_blsp1_qup3_spi_apps_clk",
            .parent_names = (const char *[]){
                "blsp1_qup3_spi_apps_clk_src",
            },
            .num_parents = 1,
            .flags = CLK_SET_RATE_PARENT,
            .ops = &clk_branch2_ops,
        },
    },
};

static struct clk_branch gcc_blsp1_qup4_i2c_apps_clk = {
    .halt_reg = 0x07C8,
    .clkr = {
        .enable_reg = 0x07C8,
        .enable_mask = BIT(0),
        .hw.init = &(struct clk_init_data){
            .name = "gcc_blsp1_qup4_i2c_apps_clk",
            .parent_names = (const char *[]){
                "blsp1_qup4_i2c_apps_clk_src",
            },
            .num_parents = 1,
            .flags = CLK_SET_RATE_PARENT,
            .ops = &clk_branch2_ops,
        },
    },
};

static struct clk_branch gcc_blsp1_qup4_spi_apps_clk = {
    .halt_reg = 0x07C4,
    .clkr = {
        .enable_reg = 0x07C4,
        .enable_mask = BIT(0),
        .hw.init = &(struct clk_init_data){
            .name = "gcc_blsp1_qup4_spi_apps_clk",
            .parent_names = (const char *[]){
                "blsp1_qup4_spi_apps_clk_src",
            },
            .num_parents = 1,
            .flags = CLK_SET_RATE_PARENT,
            .ops = &clk_branch2_ops,
        },
    },
};

static struct clk_branch gcc_blsp1_qup5_i2c_apps_clk = {
    .halt_reg = 0x0848,
    .clkr = {
        .enable_reg = 0x0848,
        .enable_mask = BIT(0),
        .hw.init = &(struct clk_init_data){
            .name = "gcc_blsp1_qup5_i2c_apps_clk",
            .parent_names = (const char *[]){
                "blsp1_qup5_i2c_apps_clk_src",
            },
            .num_parents = 1,
            .flags = CLK_SET_RATE_PARENT,
            .ops = &clk_branch2_ops,
        },
    },
};

static struct clk_branch gcc_blsp1_qup5_spi_apps_clk = {
    .halt_reg = 0x0844,
    .clkr = {
        .enable_reg = 0x0844,
        .enable_mask = BIT(0),
        .hw.init = &(struct clk_init_data){
            .name = "gcc_blsp1_qup5_spi_apps_clk",
            .parent_names = (const char *[]){
                "blsp1_qup5_spi_apps_clk_src",
            },
            .num_parents = 1,
            .flags = CLK_SET_RATE_PARENT,
            .ops = &clk_branch2_ops,
        },
    },
};

static struct clk_branch gcc_blsp1_qup6_i2c_apps_clk = {
    .halt_reg = 0x08C8,
    .clkr = {
        .enable_reg = 0x08C8,
        .enable_mask = BIT(0),
        .hw.init = &(struct clk_init_data){
            .name = "gcc_blsp1_qup6_i2c_apps_clk",
            .parent_names = (const char *[]){
                "blsp1_qup6_i2c_apps_clk_src",
            },
            .num_parents = 1,
            .flags = CLK_SET_RATE_PARENT,
            .ops = &clk_branch2_ops,
        },
    },
};

static struct clk_branch gcc_blsp1_qup6_spi_apps_clk = {
    .halt_reg = 0x08C4,
    .clkr = {
        .enable_reg = 0x08C4,
        .enable_mask = BIT(0),
        .hw.init = &(struct clk_init_data){
            .name = "gcc_blsp1_qup6_spi_apps_clk",
            .parent_names = (const char *[]){
                "blsp1_qup6_spi_apps_clk_src",
            },
            .num_parents = 1,
            .flags = CLK_SET_RATE_PARENT,
            .ops = &clk_branch2_ops,
        },
    },
};

static struct clk_branch gcc_blsp1_uart1_apps_clk = {
    .halt_reg = 0x0684,
    .clkr = {
        .enable_reg = 0x0684,
        .enable_mask = BIT(0),
        .hw.init = &(struct clk_init_data){
            .name = "gcc_blsp1_uart1_apps_clk",
            .parent_names = (const char *[]){
                "blsp1_uart1_apps_clk_src",
            },
            .num_parents = 1,
            .flags = CLK_SET_RATE_PARENT,
            .ops = &clk_branch2_ops,
        },
    },
};

static struct clk_branch gcc_blsp1_uart2_apps_clk = {
    .halt_reg = 0x0704,
    .clkr = {
        .enable_reg = 0x0704,
        .enable_mask = BIT(0),
        .hw.init = &(struct clk_init_data){
            .name = "gcc_blsp1_uart2_apps_clk",
            .parent_names = (const char *[]){
                "blsp1_uart2_apps_clk_src",
            },
            .num_parents = 1,
            .flags = CLK_SET_RATE_PARENT,
            .ops = &clk_branch2_ops,
        },
    },
};

static struct clk_branch gcc_blsp1_uart3_apps_clk = {
    .halt_reg = 0x0784,
    .clkr = {
        .enable_reg = 0x0784,
        .enable_mask = BIT(0),
        .hw.init = &(struct clk_init_data){
            .name = "gcc_blsp1_uart3_apps_clk",
            .parent_names = (const char *[]){
                "blsp1_uart3_apps_clk_src",
            },
            .num_parents = 1,
            .flags = CLK_SET_RATE_PARENT,
            .ops = &clk_branch2_ops,
        },
    },
};

static struct clk_branch gcc_blsp1_uart4_apps_clk = {
    .halt_reg = 0x0804,
    .clkr = {
        .enable_reg = 0x0804,
        .enable_mask = BIT(0),
        .hw.init = &(struct clk_init_data){
            .name = "gcc_blsp1_uart4_apps_clk",
            .parent_names = (const char *[]){
                "blsp1_uart4_apps_clk_src",
            },
            .num_parents = 1,
            .flags = CLK_SET_RATE_PARENT,
            .ops = &clk_branch2_ops,
        },
    },
};

static struct clk_branch gcc_blsp1_uart5_apps_clk = {
    .halt_reg = 0x0884,
    .clkr = {
        .enable_reg = 0x0884,
        .enable_mask = BIT(0),
        .hw.init = &(struct clk_init_data){
            .name = "gcc_blsp1_uart5_apps_clk",
            .parent_names = (const char *[]){
                "blsp1_uart5_apps_clk_src",
            },
            .num_parents = 1,
            .flags = CLK_SET_RATE_PARENT,
            .ops = &clk_branch2_ops,
        },
    },
};

static struct clk_branch gcc_blsp1_uart6_apps_clk = {
    .halt_reg = 0x0904,
    .clkr = {
        .enable_reg = 0x0904,
        .enable_mask = BIT(0),
        .hw.init = &(struct clk_init_data){
            .name = "gcc_blsp1_uart6_apps_clk",
            .parent_names = (const char *[]){
                "blsp1_uart6_apps_clk_src",
            },
            .num_parents = 1,
            .flags = CLK_SET_RATE_PARENT,
            .ops = &clk_branch2_ops,
        },
    },
};

// TODO removed gcc_blsp2_ahb_clk

static struct clk_branch gcc_blsp2_qup1_i2c_apps_clk = {
    .halt_reg = 0x0988,
    .clkr = {
        .enable_reg = 0x0988,
        .enable_mask = BIT(0),
        .hw.init = &(struct clk_init_data){
            .name = "gcc_blsp2_qup1_i2c_apps_clk",
            .parent_names = (const char *[]){
                "blsp2_qup1_i2c_apps_clk_src",
            },
            .num_parents = 1,
            .flags = CLK_SET_RATE_PARENT,
            .ops = &clk_branch2_ops,
        },
    },
};

static struct clk_branch gcc_blsp2_qup1_spi_apps_clk = {
    .halt_reg = 0x0984,
    .clkr = {
        .enable_reg = 0x0984,
        .enable_mask = BIT(0),
        .hw.init = &(struct clk_init_data){
            .name = "gcc_blsp2_qup1_spi_apps_clk",
            .parent_names = (const char *[]){
                "blsp2_qup1_spi_apps_clk_src",
            },
            .num_parents = 1,
            .flags = CLK_SET_RATE_PARENT,
            .ops = &clk_branch2_ops,
        },
    },
};

static struct clk_branch gcc_blsp2_qup2_i2c_apps_clk = {
    .halt_reg = 0x0A08,
    .clkr = {
        .enable_reg = 0x0A08,
        .enable_mask = BIT(0),
        .hw.init = &(struct clk_init_data){
            .name = "gcc_blsp2_qup2_i2c_apps_clk",
            .parent_names = (const char *[]){
                "blsp2_qup2_i2c_apps_clk_src",
            },
            .num_parents = 1,
            .flags = CLK_SET_RATE_PARENT,
            .ops = &clk_branch2_ops,
        },
    },
};

static struct clk_branch gcc_blsp2_qup2_spi_apps_clk = {
    .halt_reg = 0x0A04,
    .clkr = {
        .enable_reg = 0x0A04,
        .enable_mask = BIT(0),
        .hw.init = &(struct clk_init_data){
            .name = "gcc_blsp2_qup2_spi_apps_clk",
            .parent_names = (const char *[]){
                "blsp2_qup2_spi_apps_clk_src",
            },
            .num_parents = 1,
            .flags = CLK_SET_RATE_PARENT,
            .ops = &clk_branch2_ops,
        },
    },
};

static struct clk_branch gcc_blsp2_qup3_i2c_apps_clk = {
    .halt_reg = 0x0A88,
    .clkr = {
        .enable_reg = 0x0A88,
        .enable_mask = BIT(0),
        .hw.init = &(struct clk_init_data){
            .name = "gcc_blsp2_qup3_i2c_apps_clk",
            .parent_names = (const char *[]){
                "blsp2_qup3_i2c_apps_clk_src",
            },
            .num_parents = 1,
            .flags = CLK_SET_RATE_PARENT,
            .ops = &clk_branch2_ops,
        },
    },
};

static struct clk_branch gcc_blsp2_qup3_spi_apps_clk = {
    .halt_reg = 0x0A84,
    .clkr = {
        .enable_reg = 0x0A84,
        .enable_mask = BIT(0),
        .hw.init = &(struct clk_init_data){
            .name = "gcc_blsp2_qup3_spi_apps_clk",
            .parent_names = (const char *[]){
                "blsp2_qup3_spi_apps_clk_src",
            },
            .num_parents = 1,
            .flags = CLK_SET_RATE_PARENT,
            .ops = &clk_branch2_ops,
        },
    },
};

static struct clk_branch gcc_blsp2_qup4_i2c_apps_clk = {
    .halt_reg = 0x0B08,
    .clkr = {
        .enable_reg = 0x0B08,
        .enable_mask = BIT(0),
        .hw.init = &(struct clk_init_data){
            .name = "gcc_blsp2_qup4_i2c_apps_clk",
            .parent_names = (const char *[]){
                "blsp2_qup4_i2c_apps_clk_src",
            },
            .num_parents = 1,
            .flags = CLK_SET_RATE_PARENT,
            .ops = &clk_branch2_ops,
        },
    },
};

static struct clk_branch gcc_blsp2_qup4_spi_apps_clk = {
    .halt_reg = 0x0B04,
    .clkr = {
        .enable_reg = 0x0B04,
        .enable_mask = BIT(0),
        .hw.init = &(struct clk_init_data){
            .name = "gcc_blsp2_qup4_spi_apps_clk",
            .parent_names = (const char *[]){
                "blsp2_qup4_spi_apps_clk_src",
            },
            .num_parents = 1,
            .flags = CLK_SET_RATE_PARENT,
            .ops = &clk_branch2_ops,
        },
    },
};

static struct clk_branch gcc_blsp2_qup5_i2c_apps_clk = {
    .halt_reg = 0x0B88,
    .clkr = {
        .enable_reg = 0x0B88,
        .enable_mask = BIT(0),
        .hw.init = &(struct clk_init_data){
            .name = "gcc_blsp2_qup5_i2c_apps_clk",
            .parent_names = (const char *[]){
                "blsp2_qup5_i2c_apps_clk_src",
            },
            .num_parents = 1,
            .flags = CLK_SET_RATE_PARENT,
            .ops = &clk_branch2_ops,
        },
    },
};

static struct clk_branch gcc_blsp2_qup5_spi_apps_clk = {
    .halt_reg = 0x0B84,
    .clkr = {
        .enable_reg = 0x0B84,
        .enable_mask = BIT(0),
        .hw.init = &(struct clk_init_data){
            .name = "gcc_blsp2_qup5_spi_apps_clk",
            .parent_names = (const char *[]){
                "blsp2_qup5_spi_apps_clk_src",
            },
            .num_parents = 1,
            .flags = CLK_SET_RATE_PARENT,
            .ops = &clk_branch2_ops,
        },
    },
};

static struct clk_branch gcc_blsp2_qup6_i2c_apps_clk = {
    .halt_reg = 0x0C08,
    .clkr = {
        .enable_reg = 0x0C08,
        .enable_mask = BIT(0),
        .hw.init = &(struct clk_init_data){
            .name = "gcc_blsp2_qup6_i2c_apps_clk",
            .parent_names = (const char *[]){
                "blsp2_qup6_i2c_apps_clk_src",
            },
            .num_parents = 1,
            .flags = CLK_SET_RATE_PARENT,
            .ops = &clk_branch2_ops,
        },
    },
};

static struct clk_branch gcc_blsp2_qup6_spi_apps_clk = {
    .halt_reg = 0x0C04,
    .clkr = {
        .enable_reg = 0x0C04,
        .enable_mask = BIT(0),
        .hw.init = &(struct clk_init_data){
            .name = "gcc_blsp2_qup6_spi_apps_clk",
            .parent_names = (const char *[]){
                "blsp2_qup6_spi_apps_clk_src",
            },
            .num_parents = 1,
            .flags = CLK_SET_RATE_PARENT,
            .ops = &clk_branch2_ops,
        },
    },
};

static struct clk_branch gcc_blsp2_uart1_apps_clk = {
    .halt_reg = 0x09C4,
    .clkr = {
        .enable_reg = 0x09C4,
        .enable_mask = BIT(0),
        .hw.init = &(struct clk_init_data){
            .name = "gcc_blsp2_uart1_apps_clk",
            .parent_names = (const char *[]){
                "blsp2_uart1_apps_clk_src",
            },
            .num_parents = 1,
            .flags = CLK_SET_RATE_PARENT,
            .ops = &clk_branch2_ops,
        },
    },
};

static struct clk_branch gcc_blsp2_uart2_apps_clk = {
    .halt_reg = 0x0A44,
    .clkr = {
        .enable_reg = 0x0A44,
        .enable_mask = BIT(0),
        .hw.init = &(struct clk_init_data){
            .name = "gcc_blsp2_uart2_apps_clk",
            .parent_names = (const char *[]){
                "blsp2_uart2_apps_clk_src",
            },
            .num_parents = 1,
            .flags = CLK_SET_RATE_PARENT,
            .ops = &clk_branch2_ops,
        },
    },
};

static struct clk_branch gcc_blsp2_uart3_apps_clk = {
    .halt_reg = 0x0AC4,
    .clkr = {
        .enable_reg = 0x0AC4,
        .enable_mask = BIT(0),
        .hw.init = &(struct clk_init_data){
            .name = "gcc_blsp2_uart3_apps_clk",
            .parent_names = (const char *[]){
                "blsp2_uart3_apps_clk_src",
            },
            .num_parents = 1,
            .flags = CLK_SET_RATE_PARENT,
            .ops = &clk_branch2_ops,
        },
    },
};

static struct clk_branch gcc_blsp2_uart4_apps_clk = {
    .halt_reg = 0x0B44,
    .clkr = {
        .enable_reg = 0x0B44,
        .enable_mask = BIT(0),
        .hw.init = &(struct clk_init_data){
            .name = "gcc_blsp2_uart4_apps_clk",
            .parent_names = (const char *[]){
                "blsp2_uart4_apps_clk_src",
            },
            .num_parents = 1,
            .flags = CLK_SET_RATE_PARENT,
            .ops = &clk_branch2_ops,
        },
    },
};

static struct clk_branch gcc_blsp2_uart4_apps_clk = {
    .halt_reg = 0x0B44,
    .clkr = {
        .enable_reg = 0x0B44,
        .enable_mask = BIT(0),
        .hw.init = &(struct clk_init_data){
            .name = "gcc_blsp2_uart4_apps_clk",
            .parent_names = (const char *[]){
                "blsp2_uart4_apps_clk_src",
            },
            .num_parents = 1,
            .flags = CLK_SET_RATE_PARENT,
            .ops = &clk_branch2_ops,
        },
    },
};

//weitermachen!!

static struct branch_clk gcc_blsp2_uart5_apps_clk = {
    .cbcr_reg = BLSP2_UART5_APPS_CBCR,
    .has_sibling = 0,
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_blsp2_uart5_apps_clk",
        .parent = &blsp2_uart5_apps_clk_src.c,
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_blsp2_uart5_apps_clk.c),
    },
};

static struct branch_clk gcc_blsp2_uart6_apps_clk = {
    .cbcr_reg = BLSP2_UART6_APPS_CBCR,
    .has_sibling = 0,
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_blsp2_uart6_apps_clk",
        .parent = &blsp2_uart6_apps_clk_src.c,
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_blsp2_uart6_apps_clk.c),
    },
};

static struct local_vote_clk gcc_boot_rom_ahb_clk = {
    .cbcr_reg = BOOT_ROM_AHB_CBCR,
    .vote_reg = APCS_CLOCK_BRANCH_ENA_VOTE,
    .en_mask = BIT(10),
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_boot_rom_ahb_clk",
        .ops = &clk_ops_vote,
        CLK_INIT(gcc_boot_rom_ahb_clk.c),
    },
};

static struct branch_clk gcc_gp1_clk = {
    .cbcr_reg = GP1_CBCR,
    .has_sibling = 0,
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_gp1_clk",
        .parent = &gp1_clk_src.c,
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_gp1_clk.c),
    },
};

static struct branch_clk gcc_gp2_clk = {
    .cbcr_reg = GP2_CBCR,
    .has_sibling = 0,
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_gp2_clk",
        .parent = &gp2_clk_src.c,
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_gp2_clk.c),
    },
};

static struct branch_clk gcc_gp3_clk = {
    .cbcr_reg = GP3_CBCR,
    .has_sibling = 0,
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_gp3_clk",
        .parent = &gp3_clk_src.c,
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_gp3_clk.c),
    },
};

static struct branch_clk gcc_lpass_q6_axi_clk = {
    .cbcr_reg = LPASS_Q6_AXI_CBCR,
    .has_sibling = 1,
    .base = &virt_base,
    .halt_check = DELAY,
    .c = {
        .dbg_name = "gcc_lpass_q6_axi_clk",
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_lpass_q6_axi_clk.c),
    },
};

static struct branch_clk gcc_mss_q6_bimc_axi_clk = {
    .cbcr_reg = MSS_Q6_BIMC_AXI_CBCR,
    .has_sibling = 0,
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_mss_q6_bimc_axi_clk",
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_mss_q6_bimc_axi_clk.c),
    },
};

static struct branch_clk gcc_pcie_0_aux_clk = {
    .cbcr_reg = PCIE_0_AUX_CBCR,
    .has_sibling = 0,
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_pcie_0_aux_clk",
        .parent = &pcie_0_aux_clk_src.c,
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_pcie_0_aux_clk.c),
    },
};

static struct branch_clk gcc_pcie_0_cfg_ahb_clk = {
    .cbcr_reg = PCIE_0_CFG_AHB_CBCR,
    .has_sibling = 1,
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_pcie_0_cfg_ahb_clk",
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_pcie_0_cfg_ahb_clk.c),
    },
};

static struct branch_clk gcc_pcie_0_mstr_axi_clk = {
    .cbcr_reg = PCIE_0_MSTR_AXI_CBCR,
    .has_sibling = 1,
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_pcie_0_mstr_axi_clk",
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_pcie_0_mstr_axi_clk.c),
    },
};

static struct branch_clk gcc_pcie_0_pipe_clk = {
    .cbcr_reg = PCIE_0_PIPE_CBCR,
    .bcr_reg = PCIE_PHY_0_PHY_BCR,
    .has_sibling = 0,
    .base = &virt_base,
    .halt_check = DELAY,
    .c = {
        .dbg_name = "gcc_pcie_0_pipe_clk",
        .parent = &pcie_0_pipe_clk_src.c,
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_pcie_0_pipe_clk.c),
    },
};

static struct branch_clk gcc_pcie_0_slv_axi_clk = {
    .cbcr_reg = PCIE_0_SLV_AXI_CBCR,
    .has_sibling = 1,
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_pcie_0_slv_axi_clk",
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_pcie_0_slv_axi_clk.c),
    },
};

static struct branch_clk gcc_pcie_1_aux_clk = {
    .cbcr_reg = PCIE_1_AUX_CBCR,
    .has_sibling = 0,
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_pcie_1_aux_clk",
        .parent = &pcie_1_aux_clk_src.c,
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_pcie_1_aux_clk.c),
    },
};

static struct branch_clk gcc_pcie_1_cfg_ahb_clk = {
    .cbcr_reg = PCIE_1_CFG_AHB_CBCR,
    .has_sibling = 1,
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_pcie_1_cfg_ahb_clk",
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_pcie_1_cfg_ahb_clk.c),
    },
};

static struct branch_clk gcc_pcie_1_mstr_axi_clk = {
    .cbcr_reg = PCIE_1_MSTR_AXI_CBCR,
    .has_sibling = 1,
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_pcie_1_mstr_axi_clk",
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_pcie_1_mstr_axi_clk.c),
    },
};

static struct branch_clk gcc_pcie_1_pipe_clk = {
    .cbcr_reg = PCIE_1_PIPE_CBCR,
    .bcr_reg = PCIE_PHY_1_PHY_BCR,
    .has_sibling = 0,
    .base = &virt_base,
    .halt_check = DELAY,
    .c = {
        .dbg_name = "gcc_pcie_1_pipe_clk",
        .parent = &pcie_1_pipe_clk_src.c,
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_pcie_1_pipe_clk.c),
    },
};

static struct branch_clk gcc_pcie_1_slv_axi_clk = {
    .cbcr_reg = PCIE_1_SLV_AXI_CBCR,
    .has_sibling = 1,
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_pcie_1_slv_axi_clk",
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_pcie_1_slv_axi_clk.c),
    },
};

static struct branch_clk gcc_pdm2_clk = {
    .cbcr_reg = PDM2_CBCR,
    .has_sibling = 0,
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_pdm2_clk",
        .parent = &pdm2_clk_src.c,
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_pdm2_clk.c),
    },
};

static struct branch_clk gcc_pdm_ahb_clk = {
    .cbcr_reg = PDM_AHB_CBCR,
    .has_sibling = 1,
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_pdm_ahb_clk",
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_pdm_ahb_clk.c),
    },
};

static struct local_vote_clk gcc_prng_ahb_clk = {
    .cbcr_reg = PRNG_AHB_CBCR,
    .vote_reg = APCS_CLOCK_BRANCH_ENA_VOTE,
    .en_mask = BIT(13),
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_prng_ahb_clk",
        .ops = &clk_ops_vote,
        CLK_INIT(gcc_prng_ahb_clk.c),
    },
};

static struct branch_clk gcc_sdcc1_ahb_clk = {
    .cbcr_reg = SDCC1_AHB_CBCR,
    .has_sibling = 1,
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_sdcc1_ahb_clk",
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_sdcc1_ahb_clk.c),
    },
};

static struct branch_clk gcc_sdcc1_apps_clk = {
    .cbcr_reg = SDCC1_APPS_CBCR,
    .has_sibling = 0,
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_sdcc1_apps_clk",
        .parent = &sdcc1_apps_clk_src.c,
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_sdcc1_apps_clk.c),
    },
};

static struct branch_clk gcc_sdcc2_ahb_clk = {
    .cbcr_reg = SDCC2_AHB_CBCR,
    .has_sibling = 1,
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_sdcc2_ahb_clk",
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_sdcc2_ahb_clk.c),
    },
};

static struct branch_clk gcc_sdcc2_apps_clk = {
    .cbcr_reg = SDCC2_APPS_CBCR,
    .has_sibling = 0,
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_sdcc2_apps_clk",
        .parent = &sdcc2_apps_clk_src.c,
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_sdcc2_apps_clk.c),
    },
};

static struct branch_clk gcc_sdcc3_ahb_clk = {
    .cbcr_reg = SDCC3_AHB_CBCR,
    .has_sibling = 1,
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_sdcc3_ahb_clk",
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_sdcc3_ahb_clk.c),
    },
};

static struct branch_clk gcc_sdcc3_apps_clk = {
    .cbcr_reg = SDCC3_APPS_CBCR,
    .has_sibling = 0,
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_sdcc3_apps_clk",
        .parent = &sdcc3_apps_clk_src.c,
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_sdcc3_apps_clk.c),
    },
};

static struct branch_clk gcc_sdcc4_ahb_clk = {
    .cbcr_reg = SDCC4_AHB_CBCR,
    .has_sibling = 1,
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_sdcc4_ahb_clk",
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_sdcc4_ahb_clk.c),
    },
};

static struct branch_clk gcc_sdcc4_apps_clk = {
    .cbcr_reg = SDCC4_APPS_CBCR,
    .has_sibling = 0,
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_sdcc4_apps_clk",
        .parent = &sdcc4_apps_clk_src.c,
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_sdcc4_apps_clk.c),
    },
};

static struct branch_clk gcc_sys_noc_ufs_axi_clk = {
    .cbcr_reg = SYS_NOC_UFS_AXI_CBCR,
    .has_sibling = 1,
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_sys_noc_ufs_axi_clk",
        .parent = &ufs_axi_clk_src.c,
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_sys_noc_ufs_axi_clk.c),
    },
};

static struct branch_clk gcc_sys_noc_usb3_axi_clk = {
    .cbcr_reg = SYS_NOC_USB3_AXI_CBCR,
    .has_sibling = 1,
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_sys_noc_usb3_axi_clk",
        .parent = &usb30_master_clk_src.c,
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_sys_noc_usb3_axi_clk.c),
    },
};

static struct branch_clk gcc_tsif_ahb_clk = {
    .cbcr_reg = TSIF_AHB_CBCR,
    .has_sibling = 1,
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_tsif_ahb_clk",
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_tsif_ahb_clk.c),
    },
};

static struct branch_clk gcc_tsif_ref_clk = {
    .cbcr_reg = TSIF_REF_CBCR,
    .has_sibling = 0,
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_tsif_ref_clk",
        .parent = &tsif_ref_clk_src.c,
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_tsif_ref_clk.c),
    },
};

static struct branch_clk gcc_ufs_ahb_clk = {
    .cbcr_reg = UFS_AHB_CBCR,
    .has_sibling = 1,
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_ufs_ahb_clk",
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_ufs_ahb_clk.c),
    },
};

static struct branch_clk gcc_ufs_axi_clk = {
    .cbcr_reg = UFS_AXI_CBCR,
    .has_sibling = 0,
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_ufs_axi_clk",
        .parent = &ufs_axi_clk_src.c,
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_ufs_axi_clk.c),
    },
};

static struct branch_clk gcc_ufs_rx_cfg_clk = {
    .cbcr_reg = UFS_RX_CFG_CBCR,
    .has_sibling = 1,
    .max_div = 16,
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_ufs_rx_cfg_clk",
        .parent = &ufs_axi_clk_src.c,
        .ops = &clk_ops_branch,
        .rate = 1,
        CLK_INIT(gcc_ufs_rx_cfg_clk.c),
    },
};

static struct branch_clk gcc_ufs_rx_symbol_0_clk = {
    .cbcr_reg = UFS_RX_SYMBOL_0_CBCR,
    .has_sibling = 1,
    .base = &virt_base,
    .halt_check = DELAY,
    .c = {
        .dbg_name = "gcc_ufs_rx_symbol_0_clk",
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_ufs_rx_symbol_0_clk.c),
    },
};

static struct branch_clk gcc_ufs_rx_symbol_1_clk = {
    .cbcr_reg = UFS_RX_SYMBOL_1_CBCR,
    .has_sibling = 1,
    .base = &virt_base,
    .halt_check = DELAY,
    .c = {
        .dbg_name = "gcc_ufs_rx_symbol_1_clk",
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_ufs_rx_symbol_1_clk.c),
    },
};

static struct branch_clk gcc_ufs_tx_cfg_clk = {
    .cbcr_reg = UFS_TX_CFG_CBCR,
    .has_sibling = 1,
    .max_div = 16,
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_ufs_tx_cfg_clk",
        .parent = &ufs_axi_clk_src.c,
        .ops = &clk_ops_branch,
        .rate = 1,
        CLK_INIT(gcc_ufs_tx_cfg_clk.c),
    },
};

static struct branch_clk gcc_ufs_tx_symbol_0_clk = {
    .cbcr_reg = UFS_TX_SYMBOL_0_CBCR,
    .has_sibling = 1,
    .base = &virt_base,
    .halt_check = DELAY,
    .c = {
        .dbg_name = "gcc_ufs_tx_symbol_0_clk",
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_ufs_tx_symbol_0_clk.c),
    },
};

static struct branch_clk gcc_ufs_tx_symbol_1_clk = {
    .cbcr_reg = UFS_TX_SYMBOL_1_CBCR,
    .has_sibling = 1,
    .base = &virt_base,
    .halt_check = DELAY,
    .c = {
        .dbg_name = "gcc_ufs_tx_symbol_1_clk",
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_ufs_tx_symbol_1_clk.c),
    },
};

static struct branch_clk gcc_usb2_hs_phy_sleep_clk = {
    .cbcr_reg = USB2_HS_PHY_SLEEP_CBCR,
    .bcr_reg = USB2_HS_PHY_ONLY_BCR,
    .has_sibling = 1,
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_usb2_hs_phy_sleep_clk",
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_usb2_hs_phy_sleep_clk.c),
    },
};

static struct branch_clk gcc_usb30_master_clk = {
    .cbcr_reg = USB30_MASTER_CBCR,
    .bcr_reg = USB_30_BCR,
    .has_sibling = 0,
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_usb30_master_clk",
        .parent = &usb30_master_clk_src.c,
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_usb30_master_clk.c),
        .depends = &gcc_sys_noc_usb3_axi_clk.c,
    },
};

static struct branch_clk gcc_usb30_mock_utmi_clk = {
    .cbcr_reg = USB30_MOCK_UTMI_CBCR,
    .has_sibling = 0,
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_usb30_mock_utmi_clk",
        .parent = &usb30_mock_utmi_clk_src.c,
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_usb30_mock_utmi_clk.c),
    },
};

static struct branch_clk gcc_usb30_sleep_clk = {
    .cbcr_reg = USB30_SLEEP_CBCR,
    .has_sibling = 1,
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_usb30_sleep_clk",
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_usb30_sleep_clk.c),
    },
};

static struct branch_clk gcc_usb3_phy_aux_clk = {
    .cbcr_reg = USB3_PHY_AUX_CBCR,
    .has_sibling = 0,
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_usb3_phy_aux_clk",
        .parent = &usb3_phy_aux_clk_src.c,
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_usb3_phy_aux_clk.c),
    },
};

static struct gate_clk gcc_usb3_phy_pipe_clk = {
    .en_reg = USB3_PHY_PIPE_CBCR,
    .en_mask = BIT(0),
    .delay_us = 50,
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_usb3_phy_pipe_clk",
        .ops = &clk_ops_gate,
        CLK_INIT(gcc_usb3_phy_pipe_clk.c),
    },
};

static struct reset_clk gcc_usb3phy_phy_reset = {
    .reset_reg = USB3PHY_PHY_BCR,
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_usb3phy_phy_reset",
        .ops = &clk_ops_rst,
        CLK_INIT(gcc_usb3phy_phy_reset.c),
    },
};

static struct branch_clk gcc_usb_hs_ahb_clk = {
    .cbcr_reg = USB_HS_AHB_CBCR,
    .has_sibling = 1,
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_usb_hs_ahb_clk",
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_usb_hs_ahb_clk.c),
    },
};

static struct branch_clk gcc_usb_hs_system_clk = {
    .cbcr_reg = USB_HS_SYSTEM_CBCR,
    .bcr_reg = USB_HS_BCR,
    .has_sibling = 0,
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_usb_hs_system_clk",
        .parent = &usb_hs_system_clk_src.c,
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_usb_hs_system_clk.c),
    },
};

static struct branch_clk gcc_usb_phy_cfg_ahb2phy_clk = {
    .cbcr_reg = USB_PHY_CFG_AHB2PHY_CBCR,
    .has_sibling = 1,
    .base = &virt_base,
    .c = {
        .dbg_name = "gcc_usb_phy_cfg_ahb2phy_clk",
        .ops = &clk_ops_branch,
        CLK_INIT(gcc_usb_phy_cfg_ahb2phy_clk.c),
    },
};

static struct mux_clk gcc_debug_mux;
static struct clk_ops clk_ops_debug_mux;
static struct clk_mux_ops gcc_debug_mux_ops;

static struct measure_clk_data debug_mux_priv = {
    .cxo = &gcc_xo.c,
    .plltest_reg = PLLTEST_PAD_CFG,
    .plltest_val = 0x51A00,
    .xo_div4_cbcr = GCC_XO_DIV4_CBCR,
    .ctl_reg = CLOCK_FRQ_MEASURE_CTL,
    .status_reg = CLOCK_FRQ_MEASURE_STATUS,
    .base = &virt_base,
};

static int gcc_set_mux_sel(struct mux_clk *clk, int sel)
{
    u32 regval;

    /* Zero out CDIV bits in top level debug mux register */
    regval = readl_relaxed(GCC_REG_BASE(GCC_DEBUG_CLK_CTL));
    regval &= ~BM(15, 12);
    writel_relaxed(regval, GCC_REG_BASE(GCC_DEBUG_CLK_CTL));

    /*
     * RPM clocks use the same GCC debug mux. Don't reprogram
     * the mux (selection) register.
     */
    if (sel == 0xFFFF)
        return 0;
    mux_reg_ops.set_mux_sel(clk, sel);

    return 0;
}

static struct mux_clk gcc_debug_mux = {
    .priv = &debug_mux_priv,
    .ops = &gcc_debug_mux_ops,
    .en_mask = BIT(16),
    .mask = 0x3FF,
    .base = &virt_dbgbase,
    MUX_REC_SRC_LIST(
        &debug_mmss_clk.c,
        &debug_rpm_clk.c,
        &debug_cpu_clk.c,
    ),
    MUX_SRC_LIST(
        { &debug_cpu_clk.c, 0x016A },
        { &debug_mmss_clk.c, 0x002b },
        { &debug_rpm_clk.c, 0xffff },
        { &gcc_sys_noc_usb3_axi_clk.c, 0x0006 },
        { &gcc_mss_q6_bimc_axi_clk.c, 0x0031 },
        { &gcc_usb30_master_clk.c, 0x0050 },
        { &gcc_usb30_sleep_clk.c, 0x0051 },
        { &gcc_usb30_mock_utmi_clk.c, 0x0052 },
        { &gcc_usb3_phy_aux_clk.c, 0x0053 },
        { &gcc_usb3_phy_pipe_clk.c, 0x0054 },
        { &gcc_sys_noc_ufs_axi_clk.c, 0x0058 },
        { &gcc_usb_hs_system_clk.c, 0x0060 },
        { &gcc_usb_hs_ahb_clk.c, 0x0061 },
        { &gcc_usb2_hs_phy_sleep_clk.c, 0x0063 },
        { &gcc_usb_phy_cfg_ahb2phy_clk.c, 0x0064 },
        { &gcc_sdcc1_apps_clk.c, 0x0068 },
        { &gcc_sdcc1_ahb_clk.c, 0x0069 },
        { &gcc_sdcc2_apps_clk.c, 0x0070 },
        { &gcc_sdcc2_ahb_clk.c, 0x0071 },
        { &gcc_sdcc3_apps_clk.c, 0x0078 },
        { &gcc_sdcc3_ahb_clk.c, 0x0079 },
        { &gcc_sdcc4_apps_clk.c, 0x0080 },
        { &gcc_sdcc4_ahb_clk.c, 0x0081 },
        { &gcc_blsp1_ahb_clk.c, 0x0088 },
        { &gcc_blsp1_qup1_spi_apps_clk.c, 0x008a },
        { &gcc_blsp1_qup1_i2c_apps_clk.c, 0x008b },
        { &gcc_blsp1_uart1_apps_clk.c, 0x008c },
        { &gcc_blsp1_qup2_spi_apps_clk.c, 0x008e },
        { &gcc_blsp1_qup2_i2c_apps_clk.c, 0x0090 },
        { &gcc_blsp1_uart2_apps_clk.c, 0x0091 },
        { &gcc_blsp1_qup3_spi_apps_clk.c, 0x0093 },
        { &gcc_blsp1_qup3_i2c_apps_clk.c, 0x0094 },
        { &gcc_blsp1_uart3_apps_clk.c, 0x0095 },
        { &gcc_blsp1_qup4_spi_apps_clk.c, 0x0098 },
        { &gcc_blsp1_qup4_i2c_apps_clk.c, 0x0099 },
        { &gcc_blsp1_uart4_apps_clk.c, 0x009a },
        { &gcc_blsp1_qup5_spi_apps_clk.c, 0x009c },
        { &gcc_blsp1_qup5_i2c_apps_clk.c, 0x009d },
        { &gcc_blsp1_uart5_apps_clk.c, 0x009e },
        { &gcc_blsp1_qup6_spi_apps_clk.c, 0x00a1 },
        { &gcc_blsp1_qup6_i2c_apps_clk.c, 0x00a2 },
        { &gcc_blsp1_uart6_apps_clk.c, 0x00a3 },
        { &gcc_blsp2_ahb_clk.c, 0x00a8 },
        { &gcc_blsp2_qup1_spi_apps_clk.c, 0x00aa },
        { &gcc_blsp2_qup1_i2c_apps_clk.c, 0x00ab },
        { &gcc_blsp2_uart1_apps_clk.c, 0x00ac },
        { &gcc_blsp2_qup2_spi_apps_clk.c, 0x00ae },
        { &gcc_blsp2_qup2_i2c_apps_clk.c, 0x00b0 },
        { &gcc_blsp2_uart2_apps_clk.c, 0x00b1 },
        { &gcc_blsp2_qup3_spi_apps_clk.c, 0x00b3 },
        { &gcc_blsp2_qup3_i2c_apps_clk.c, 0x00b4 },
        { &gcc_blsp2_uart3_apps_clk.c, 0x00b5 },
        { &gcc_blsp2_qup4_spi_apps_clk.c, 0x00b8 },
        { &gcc_blsp2_qup4_i2c_apps_clk.c, 0x00b9 },
        { &gcc_blsp2_uart4_apps_clk.c, 0x00ba },
        { &gcc_blsp2_qup5_spi_apps_clk.c, 0x00bc },
        { &gcc_blsp2_qup5_i2c_apps_clk.c, 0x00bd },
        { &gcc_blsp2_uart5_apps_clk.c, 0x00be },
        { &gcc_blsp2_qup6_spi_apps_clk.c, 0x00c1 },
        { &gcc_blsp2_qup6_i2c_apps_clk.c, 0x00c2 },
        { &gcc_blsp2_uart6_apps_clk.c, 0x00c3 },
        { &gcc_pdm_ahb_clk.c, 0x00d0 },
        { &gcc_pdm2_clk.c, 0x00d2 },
        { &gcc_prng_ahb_clk.c, 0x00d8 },
        { &gcc_bam_dma_ahb_clk.c, 0x00e0 },
        { &gcc_tsif_ahb_clk.c, 0x00e8 },
        { &gcc_tsif_ref_clk.c, 0x00e9 },
        { &gcc_boot_rom_ahb_clk.c, 0x00f8 },
        { &gcc_lpass_q6_axi_clk.c, 0x0160 },
        { &gcc_pcie_0_slv_axi_clk.c, 0x01e8 },
        { &gcc_pcie_0_mstr_axi_clk.c, 0x01e9 },
        { &gcc_pcie_0_cfg_ahb_clk.c, 0x01ea },
        { &gcc_pcie_0_aux_clk.c, 0x01eb },
        { &gcc_pcie_0_pipe_clk.c, 0x01ec },
        { &gcc_pcie_1_slv_axi_clk.c, 0x01f0 },
        { &gcc_pcie_1_mstr_axi_clk.c, 0x01f1 },
        { &gcc_pcie_1_cfg_ahb_clk.c, 0x01f2 },
        { &gcc_pcie_1_aux_clk.c, 0x01f3 },
        { &gcc_pcie_1_pipe_clk.c, 0x01f4 },
        { &gcc_ufs_axi_clk.c, 0x0230 },
        { &gcc_ufs_ahb_clk.c, 0x0231 },
        { &gcc_ufs_tx_cfg_clk.c, 0x0232 },
        { &gcc_ufs_rx_cfg_clk.c, 0x0233 },
        { &gcc_ufs_tx_symbol_0_clk.c, 0x0234 },
        { &gcc_ufs_tx_symbol_1_clk.c, 0x0235 },
        { &gcc_ufs_rx_symbol_0_clk.c, 0x0236 },
        { &gcc_ufs_rx_symbol_1_clk.c, 0x0237 },
    ),
    .c = {
        .dbg_name = "gcc_debug_mux",
        .ops = &clk_ops_debug_mux,
        .flags = CLKFLAG_NO_RATE_CACHE | CLKFLAG_MEASURE,
        CLK_INIT(gcc_debug_mux.c),
    },
};

static struct clk_lookup gcc_clocks_8994_v1[] = {
    CLK_LIST(gcc_bam_dma_ahb_clk),
};

static struct clk_lookup gcc_clocks_8994_common[] = {
    CLK_LIST(gcc_xo),
    CLK_LIST(gcc_xo_a_clk),
    CLK_LIST(gpll0),
    CLK_LIST(gpll0_ao),
    CLK_LIST(P_GPLL0),
    CLK_LIST(gpll4),
    CLK_LIST(gpll4_out_main),
    CLK_LIST(ufs_axi_clk_src),
    CLK_LIST(usb30_master_clk_src),
    CLK_LIST(blsp1_qup1_i2c_apps_clk_src),
    CLK_LIST(blsp1_qup1_spi_apps_clk_src),
    CLK_LIST(blsp1_qup2_i2c_apps_clk_src),
    CLK_LIST(blsp1_qup2_spi_apps_clk_src),
    CLK_LIST(blsp1_qup3_i2c_apps_clk_src),
    CLK_LIST(blsp1_qup3_spi_apps_clk_src),
    CLK_LIST(blsp1_qup4_i2c_apps_clk_src),
    CLK_LIST(blsp1_qup4_spi_apps_clk_src),
    CLK_LIST(blsp1_qup5_i2c_apps_clk_src),
    CLK_LIST(blsp1_qup5_spi_apps_clk_src),
    CLK_LIST(blsp1_qup6_i2c_apps_clk_src),
    CLK_LIST(blsp1_qup6_spi_apps_clk_src),
    CLK_LIST(blsp1_uart1_apps_clk_src),
    CLK_LIST(blsp1_uart2_apps_clk_src),
    CLK_LIST(blsp1_uart3_apps_clk_src),
    CLK_LIST(blsp1_uart4_apps_clk_src),
    CLK_LIST(blsp1_uart5_apps_clk_src),
    CLK_LIST(blsp1_uart6_apps_clk_src),
    CLK_LIST(blsp2_qup1_i2c_apps_clk_src),
    CLK_LIST(blsp2_qup1_spi_apps_clk_src),
    CLK_LIST(blsp2_qup2_i2c_apps_clk_src),
    CLK_LIST(blsp2_qup2_spi_apps_clk_src),
    CLK_LIST(blsp2_qup3_i2c_apps_clk_src),
    CLK_LIST(blsp2_qup3_spi_apps_clk_src),
    CLK_LIST(blsp2_qup4_i2c_apps_clk_src),
    CLK_LIST(blsp2_qup4_spi_apps_clk_src),
    CLK_LIST(blsp2_qup5_i2c_apps_clk_src),
    CLK_LIST(blsp2_qup5_spi_apps_clk_src),
    CLK_LIST(blsp2_qup6_i2c_apps_clk_src),
    CLK_LIST(blsp2_qup6_spi_apps_clk_src),
    CLK_LIST(blsp2_uart1_apps_clk_src),
    CLK_LIST(blsp2_uart2_apps_clk_src),
    CLK_LIST(blsp2_uart3_apps_clk_src),
    CLK_LIST(blsp2_uart4_apps_clk_src),
    CLK_LIST(blsp2_uart5_apps_clk_src),
    CLK_LIST(blsp2_uart6_apps_clk_src),
    CLK_LIST(gp1_clk_src),
    CLK_LIST(gp2_clk_src),
    CLK_LIST(gp3_clk_src),
    CLK_LIST(pcie_0_aux_clk_src),
    CLK_LIST(pcie_0_pipe_clk_src),
    CLK_LIST(pcie_1_aux_clk_src),
    CLK_LIST(pcie_1_pipe_clk_src),
    CLK_LIST(pdm2_clk_src),
    CLK_LIST(sdcc1_apps_clk_src),
    CLK_LIST(sdcc2_apps_clk_src),
    CLK_LIST(sdcc3_apps_clk_src),
    CLK_LIST(sdcc4_apps_clk_src),
    CLK_LIST(tsif_ref_clk_src),
    CLK_LIST(usb30_mock_utmi_clk_src),
    CLK_LIST(usb3_phy_aux_clk_src),
    CLK_LIST(usb_hs_system_clk_src),
    CLK_LIST(gcc_pcie_phy_0_reset),
    CLK_LIST(gcc_pcie_phy_1_reset),
    CLK_LIST(gcc_qusb2_phy_reset),
    CLK_LIST(gcc_usb3_phy_reset),
    CLK_LIST(gpll0_out_mmsscc),
    CLK_LIST(gpll0_out_msscc),
    CLK_LIST(pcie_0_phy_ldo),
    CLK_LIST(pcie_1_phy_ldo),
    CLK_LIST(ufs_phy_ldo),
    CLK_LIST(usb_ss_phy_ldo),
    CLK_LIST(gcc_blsp1_ahb_clk),
    CLK_LIST(gcc_blsp1_qup1_i2c_apps_clk),
    CLK_LIST(gcc_blsp1_qup1_spi_apps_clk),
    CLK_LIST(gcc_blsp1_qup2_i2c_apps_clk),
    CLK_LIST(gcc_blsp1_qup2_spi_apps_clk),
    CLK_LIST(gcc_blsp1_qup3_i2c_apps_clk),
    CLK_LIST(gcc_blsp1_qup3_spi_apps_clk),
    CLK_LIST(gcc_blsp1_qup4_i2c_apps_clk),
    CLK_LIST(gcc_blsp1_qup4_spi_apps_clk),
    CLK_LIST(gcc_blsp1_qup5_i2c_apps_clk),
    CLK_LIST(gcc_blsp1_qup5_spi_apps_clk),
    CLK_LIST(gcc_blsp1_qup6_i2c_apps_clk),
    CLK_LIST(gcc_blsp1_qup6_spi_apps_clk),
    CLK_LIST(gcc_blsp1_uart1_apps_clk),
    CLK_LIST(gcc_blsp1_uart2_apps_clk),
    CLK_LIST(gcc_blsp1_uart3_apps_clk),
    CLK_LIST(gcc_blsp1_uart4_apps_clk),
    CLK_LIST(gcc_blsp1_uart5_apps_clk),
    CLK_LIST(gcc_blsp1_uart6_apps_clk),
    CLK_LIST(gcc_blsp2_ahb_clk),
    CLK_LIST(gcc_blsp2_qup1_i2c_apps_clk),
    CLK_LIST(gcc_blsp2_qup1_spi_apps_clk),
    CLK_LIST(gcc_blsp2_qup2_i2c_apps_clk),
    CLK_LIST(gcc_blsp2_qup2_spi_apps_clk),
    CLK_LIST(gcc_blsp2_qup3_i2c_apps_clk),
    CLK_LIST(gcc_blsp2_qup3_spi_apps_clk),
    CLK_LIST(gcc_blsp2_qup4_i2c_apps_clk),
    CLK_LIST(gcc_blsp2_qup4_spi_apps_clk),
    CLK_LIST(gcc_blsp2_qup5_i2c_apps_clk),
    CLK_LIST(gcc_blsp2_qup5_spi_apps_clk),
    CLK_LIST(gcc_blsp2_qup6_i2c_apps_clk),
    CLK_LIST(gcc_blsp2_qup6_spi_apps_clk),
    CLK_LIST(gcc_blsp2_uart1_apps_clk),
    CLK_LIST(gcc_blsp2_uart2_apps_clk),
    CLK_LIST(gcc_blsp2_uart3_apps_clk),
    CLK_LIST(gcc_blsp2_uart4_apps_clk),
    CLK_LIST(gcc_blsp2_uart5_apps_clk),
    CLK_LIST(gcc_blsp2_uart6_apps_clk),
    CLK_LIST(gcc_boot_rom_ahb_clk),
    CLK_LIST(gcc_gp1_clk),
    CLK_LIST(gcc_gp2_clk),
    CLK_LIST(gcc_gp3_clk),
    CLK_LIST(gcc_lpass_q6_axi_clk),
    CLK_LIST(gcc_mss_q6_bimc_axi_clk),
    CLK_LIST(gcc_pcie_0_aux_clk),
    CLK_LIST(gcc_pcie_0_cfg_ahb_clk),
    CLK_LIST(gcc_pcie_0_mstr_axi_clk),
    CLK_LIST(gcc_pcie_0_pipe_clk),
    CLK_LIST(gcc_pcie_0_slv_axi_clk),
    CLK_LIST(gcc_pcie_1_aux_clk),
    CLK_LIST(gcc_pcie_1_cfg_ahb_clk),
    CLK_LIST(gcc_pcie_1_mstr_axi_clk),
    CLK_LIST(gcc_pcie_1_pipe_clk),
    CLK_LIST(gcc_pcie_1_slv_axi_clk),
    CLK_LIST(gcc_pdm2_clk),
    CLK_LIST(gcc_pdm_ahb_clk),
    CLK_LIST(gcc_prng_ahb_clk),
    CLK_LIST(gcc_sdcc1_ahb_clk),
    CLK_LIST(gcc_sdcc1_apps_clk),
    CLK_LIST(gcc_sdcc2_ahb_clk),
    CLK_LIST(gcc_sdcc2_apps_clk),
    CLK_LIST(gcc_sdcc3_ahb_clk),
    CLK_LIST(gcc_sdcc3_apps_clk),
    CLK_LIST(gcc_sdcc4_ahb_clk),
    CLK_LIST(gcc_sdcc4_apps_clk),
    CLK_LIST(gcc_sys_noc_ufs_axi_clk),
    CLK_LIST(gcc_sys_noc_usb3_axi_clk),
    CLK_LIST(gcc_tsif_ahb_clk),
    CLK_LIST(gcc_tsif_ref_clk),
    CLK_LIST(gcc_ufs_ahb_clk),
    CLK_LIST(gcc_ufs_axi_clk),
    CLK_LIST(gcc_ufs_rx_cfg_clk),
    CLK_LIST(gcc_ufs_rx_symbol_0_clk),
    CLK_LIST(gcc_ufs_rx_symbol_1_clk),
    CLK_LIST(gcc_ufs_tx_cfg_clk),
    CLK_LIST(gcc_ufs_tx_symbol_0_clk),
    CLK_LIST(gcc_ufs_tx_symbol_1_clk),
    CLK_LIST(gcc_usb2_hs_phy_sleep_clk),
    CLK_LIST(gcc_usb30_master_clk),
    CLK_LIST(gcc_usb30_mock_utmi_clk),
    CLK_LIST(gcc_usb30_sleep_clk),
    CLK_LIST(gcc_usb3_phy_aux_clk),
    CLK_LIST(gcc_usb3_phy_pipe_clk),
    CLK_LIST(gcc_usb3phy_phy_reset),
    CLK_LIST(gcc_usb_hs_ahb_clk),
    CLK_LIST(gcc_usb_hs_system_clk),
    CLK_LIST(gcc_usb_phy_cfg_ahb2phy_clk),
};

static void msm_gcc_8994v2_fixup(void)
{
    ufs_axi_clk_src.freq_tbl = ftbl_ufs_axi_clk_src_v2;
    ufs_axi_clk_src.c.fmax[VDD_DIG_NOMINAL] = 200000000;
    ufs_axi_clk_src.c.fmax[VDD_DIG_HIGH] = 240000000;

    blsp1_qup1_spi_apps_clk_src.freq_tbl = ftbl_blspqup_spi_apps_clk_src_v2;
    blsp1_qup2_spi_apps_clk_src.freq_tbl = ftbl_blspqup_spi_apps_clk_src_v2;
    blsp1_qup3_spi_apps_clk_src.freq_tbl = ftbl_blspqup_spi_apps_clk_src_v2;
    blsp1_qup4_spi_apps_clk_src.freq_tbl = ftbl_blspqup_spi_apps_clk_src_v2;
    blsp1_qup5_spi_apps_clk_src.freq_tbl = ftbl_blspqup_spi_apps_clk_src_v2;
    blsp1_qup6_spi_apps_clk_src.freq_tbl = ftbl_blspqup_spi_apps_clk_src_v2;
    blsp2_qup1_spi_apps_clk_src.freq_tbl = ftbl_blspqup_spi_apps_clk_src_v2;
    blsp2_qup2_spi_apps_clk_src.freq_tbl = ftbl_blspqup_spi_apps_clk_src_v2;
    blsp2_qup3_spi_apps_clk_src.freq_tbl = ftbl_blspqup_spi_apps_clk_src_v2;
    blsp2_qup4_spi_apps_clk_src.freq_tbl = ftbl_blspqup_spi_apps_clk_src_v2;
    blsp2_qup5_spi_apps_clk_src.freq_tbl = ftbl_blspqup_spi_apps_clk_src_v2;
    blsp2_qup6_spi_apps_clk_src.freq_tbl = ftbl_blspqup_spi_apps_clk_src_v2;

    blsp1_qup1_spi_apps_clk_src.c.fmax[VDD_DIG_NOMINAL] = 50000000;
    blsp1_qup2_spi_apps_clk_src.c.fmax[VDD_DIG_NOMINAL] = 50000000;
    blsp1_qup3_spi_apps_clk_src.c.fmax[VDD_DIG_NOMINAL] = 50000000;
    blsp1_qup4_spi_apps_clk_src.c.fmax[VDD_DIG_NOMINAL] = 50000000;
    blsp1_qup5_spi_apps_clk_src.c.fmax[VDD_DIG_NOMINAL] = 50000000;
    blsp1_qup6_spi_apps_clk_src.c.fmax[VDD_DIG_NOMINAL] = 50000000;
    blsp2_qup1_spi_apps_clk_src.c.fmax[VDD_DIG_NOMINAL] = 50000000;
    blsp2_qup2_spi_apps_clk_src.c.fmax[VDD_DIG_NOMINAL] = 50000000;
    blsp2_qup3_spi_apps_clk_src.c.fmax[VDD_DIG_NOMINAL] = 50000000;
    blsp2_qup4_spi_apps_clk_src.c.fmax[VDD_DIG_NOMINAL] = 50000000;
    blsp2_qup5_spi_apps_clk_src.c.fmax[VDD_DIG_NOMINAL] = 50000000;
    blsp2_qup6_spi_apps_clk_src.c.fmax[VDD_DIG_NOMINAL] = 50000000;

    blsp1_qup1_spi_apps_clk_src.c.fmax[VDD_DIG_HIGH] = 0;
    blsp1_qup2_spi_apps_clk_src.c.fmax[VDD_DIG_HIGH] = 0;
    blsp1_qup3_spi_apps_clk_src.c.fmax[VDD_DIG_HIGH] = 0;
    blsp1_qup4_spi_apps_clk_src.c.fmax[VDD_DIG_HIGH] = 0;
    blsp1_qup5_spi_apps_clk_src.c.fmax[VDD_DIG_HIGH] = 0;
    blsp1_qup6_spi_apps_clk_src.c.fmax[VDD_DIG_HIGH] = 0;
    blsp2_qup1_spi_apps_clk_src.c.fmax[VDD_DIG_HIGH] = 0;
    blsp2_qup2_spi_apps_clk_src.c.fmax[VDD_DIG_HIGH] = 0;
    blsp2_qup3_spi_apps_clk_src.c.fmax[VDD_DIG_HIGH] = 0;
    blsp2_qup4_spi_apps_clk_src.c.fmax[VDD_DIG_HIGH] = 0;
    blsp2_qup5_spi_apps_clk_src.c.fmax[VDD_DIG_HIGH] = 0;
    blsp2_qup6_spi_apps_clk_src.c.fmax[VDD_DIG_HIGH] = 0;
}

static int msm_gcc_8994_probe(struct platform_device *pdev)
{
    struct resource *res;
    struct clk *tmp_clk;
    int ret;
    const char *compat = NULL;
    int compatlen = 0;
    bool is_v2 = false;

    res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "cc_base");
    if (!res) {
        dev_err(&pdev->dev, "Failed to get CC base.\n");
        return -EINVAL;
    }
    virt_base = devm_ioremap(&pdev->dev, res->start, resource_size(res));
    if (!virt_base) {
        dev_err(&pdev->dev, "Failed to map in CC registers.\n");
        return -ENOMEM;
    }

    vdd_dig.regulator[0] = devm_regulator_get(&pdev->dev, "vdd_dig");
    if (IS_ERR(vdd_dig.regulator[0])) {
        if (!(PTR_ERR(vdd_dig.regulator[0]) == -EPROBE_DEFER))
            dev_err(&pdev->dev, "Unable to get vdd_dig regulator!");
        return PTR_ERR(vdd_dig.regulator[0]);
    }

    tmp_clk = gcc_xo.c.parent = devm_clk_get(&pdev->dev, "xo");
    if (IS_ERR(tmp_clk)) {
        if (!(PTR_ERR(tmp_clk) == -EPROBE_DEFER))
            dev_err(&pdev->dev, "Unable to get xo clock!");
        return PTR_ERR(tmp_clk);
    }

    tmp_clk = gcc_xo_a_clk.c.parent = devm_clk_get(&pdev->dev, "xo_a_clk");
    if (IS_ERR(tmp_clk)) {
        if (!(PTR_ERR(tmp_clk) == -EPROBE_DEFER))
            dev_err(&pdev->dev, "Unable to get xo_a_clk clock!");
        return PTR_ERR(tmp_clk);
    }

    /* Perform revision specific fixes */
    compat = of_get_property(pdev->dev.of_node, "compatible", &compatlen);
    if (!compat || (compatlen <= 0))
        return -EINVAL;
    is_v2 = !strcmp(compat, "qcom,gcc-8994v2");
    if (is_v2)
        msm_gcc_8994v2_fixup();

    /* register common clock table */
    ret = of_msm_clock_register(pdev->dev.of_node, gcc_clocks_8994_common,
                    ARRAY_SIZE(gcc_clocks_8994_common));
    if (ret)
        return ret;

    if (!is_v2) {
        /* register v1 specific clocks */
        ret = of_msm_clock_register(pdev->dev.of_node,
            gcc_clocks_8994_v1, ARRAY_SIZE(gcc_clocks_8994_v1));
        if (ret)
            return ret;
    }

    dev_info(&pdev->dev, "Registered GCC clocks.\n");
    return 0;
}

static struct of_device_id msm_clock_gcc_match_table[] = {
    { .compatible = "qcom,gcc-8994" },
    { .compatible = "qcom,gcc-8994v2" },
    {}
};

static struct platform_driver msm_clock_gcc_driver = {
    .probe = msm_gcc_8994_probe,
    .driver = {
        .name = "qcom,gcc-8994",
        .of_match_table = msm_clock_gcc_match_table,
        .owner = THIS_MODULE,
    },
};

int __init msm_gcc_8994_init(void)
{
    return platform_driver_register(&msm_clock_gcc_driver);
}
arch_initcall(msm_gcc_8994_init);

/* ======== Clock Debug Controller ======== */
static struct clk_lookup msm_clocks_measure_8994[] = {
    CLK_LIST(debug_mmss_clk),
    CLK_LIST(debug_rpm_clk),
    CLK_LIST(debug_cpu_clk),
    CLK_LOOKUP_OF("measure", gcc_debug_mux, "debug"),
};

static struct of_device_id msm_clock_debug_match_table[] = {
    { .compatible = "qcom,cc-debug-8994" },
    {}
};

static int msm_clock_debug_8994_probe(struct platform_device *pdev)
{
    struct resource *res;
    int ret;

    clk_ops_debug_mux = clk_ops_gen_mux;
    clk_ops_debug_mux.get_rate = measure_get_rate;

    gcc_debug_mux_ops = mux_reg_ops;
    gcc_debug_mux_ops.set_mux_sel = gcc_set_mux_sel;

    res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "cc_base");
    if (!res) {
        dev_err(&pdev->dev, "Failed to get CC base.\n");
        return -EINVAL;
    }
    virt_dbgbase = devm_ioremap(&pdev->dev, res->start, resource_size(res));
    if (!virt_dbgbase) {
        dev_err(&pdev->dev, "Failed to map in CC registers.\n");
        return -ENOMEM;
    }

    debug_mmss_clk.dev = &pdev->dev;
    debug_mmss_clk.clk_id = "debug_mmss_clk";
    debug_rpm_clk.dev = &pdev->dev;
    debug_rpm_clk.clk_id = "debug_rpm_clk";
    debug_cpu_clk.dev = &pdev->dev;
    debug_cpu_clk.clk_id = "debug_cpu_clk";

    ret = of_msm_clock_register(pdev->dev.of_node,
                    msm_clocks_measure_8994,
                    ARRAY_SIZE(msm_clocks_measure_8994));
    if (ret)
        return ret;

    dev_info(&pdev->dev, "Registered debug mux.\n");
    return ret;
}

static struct platform_driver msm_clock_debug_driver = {
    .probe = msm_clock_debug_8994_probe,
    .driver = {
        .name = "qcom,cc-debug-8994",
        .of_match_table = msm_clock_debug_match_table,
        .owner = THIS_MODULE,
    },
};

int __init msm_clock_debug_8994_init(void)
{
    return platform_driver_register(&msm_clock_debug_driver);
}
late_initcall(msm_clock_debug_8994_init);
