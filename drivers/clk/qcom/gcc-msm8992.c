/* Copyright (c) 2014-2015, The Linux Foundation. All rights reserved.
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
#include <linux/bitops.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/clk-provider.h>
#include <linux/regmap.h>
#include <linux/reset-controller.h>

#include <dt-bindings/clock/msm-clocks-8992.h>
#include <dt-bindings/clock/qcom,gcc-msm8992.h>

#include "common.h"
#include "clk-regmap.h"
#include "clk-alpha-pll.h"
#include "clk-rcg.h"
#include "clk-branch.h"
#include "reset.h"
#include "gdsc.h"

/*      
 * Bit manipulation macros
 */     
#define BM(msb, lsb)    (((((uint32_t)-1) << (31-msb)) >> (31-msb+lsb)) << lsb)
#define BVAL(msb, lsb, val)     (((val) << lsb) & BM(msb, lsb))

#define GCC_REG_BASE(x) (void __iomem *)(virt_base + (x))

#define CLK_LIST(_c) { .clk = &(&_c)->c, .of_idx = clk_##_c }

#define P_XO_source_val 0
#define P_GPLL0_source_val 1
#define gpll4_out_main_source_val 5
#define pcie_pipe_source_val 2

#define FIXDIV(div) (div ? (2 * (div) - 1) : (0))


#define F(f, s, h, m, n) { (f), (s), (2 * (h) - 1), (m), (n) }

enum {
        P_XO,
        P_GPLL0,
        P_GPLL2,
        P_GPLL3,
        P_GPLL1,
        P_GPLL2_EARLY,
        P_GPLL0_EARLY_DIV,
        P_SLEEP_CLK,
        P_GPLL4,
        P_AUD_REF_CLK,
        P_GPLL1_EARLY_DIV
};


static const struct parent_map gcc_sleep_clk_map[] = {
        { P_SLEEP_CLK, 5 }
};

static const char * const gcc_sleep_clk[] = {
        "sleep_clk"
};

static const struct parent_map gcc_xo_gpll0_map[] = {
        { P_XO, 0 },
        { P_GPLL0, 1 }
};

static const char * const gcc_xo_gpll0[] = {
        "xo",
        "gpll0"
};

static const struct parent_map gcc_xo_sleep_clk_map[] = {
        { P_XO, 0 },
        { P_SLEEP_CLK, 5 }
};

static const char * const gcc_xo_sleep_clk[] = {
        "xo",
        "sleep_clk"
};

static const struct parent_map gcc_xo_gpll0_gpll0_early_div_map[] = {
        { P_XO, 0 },
        { P_GPLL0, 1 },
        { P_GPLL0_EARLY_DIV, 6 }
};


static const char * const gcc_xo_gpll0_gpll0_early_div[] = {
        "xo",
        "gpll0",
        "gpll0_early_div"
};

static const struct parent_map gcc_xo_gpll0_gpll4_map[] = {
        { P_XO, 0 },
        { P_GPLL0, 1 },
        { P_GPLL4, 5 }
};

static const char * const gcc_xo_gpll0_gpll4[] = {
        "xo",
        "gpll0",
        "gpll4"
};

static const struct parent_map gcc_xo_gpll0_aud_ref_clk_map[] = {
        { P_XO, 0 },
        { P_GPLL0, 1 },
        { P_AUD_REF_CLK, 2 }
};

static const char * const gcc_xo_gpll0_aud_ref_clk[] = {
        "xo",
        "gpll0",
        "aud_ref_clk"
};

static const struct parent_map gcc_xo_gpll0_sleep_clk_gpll0_early_div_map[] = {
        { P_XO, 0 },
        { P_GPLL0, 1 },
        { P_SLEEP_CLK, 5 },
        { P_GPLL0_EARLY_DIV, 6 }
};

static const char * const gcc_xo_gpll0_sleep_clk_gpll0_early_div[] = {
        "xo",
        "gpll0",
        "sleep_clk",
        "gpll0_early_div"
};

static const struct parent_map gcc_xo_gpll0_gpll4_gpll0_early_div_map[] = {
        { P_XO, 0 },
        { P_GPLL0, 1 },
        { P_GPLL4, 5 },
        { P_GPLL0_EARLY_DIV, 6 }
};

static const char * const gcc_xo_gpll0_gpll4_gpll0_early_div[] = {
        "xo",
        "gpll0",
        "gpll4",
        "gpll0_early_div"
};

static const struct parent_map gcc_xo_gpll0_gpll2_gpll3_gpll0_early_div_map[] = {
        { P_XO, 0 },
        { P_GPLL0, 1 },
        { P_GPLL2, 2 },
        { P_GPLL3, 3 },
        { P_GPLL0_EARLY_DIV, 6 }
};

static const char * const gcc_xo_gpll0_gpll2_gpll3_gpll0_early_div[] = {
        "xo",
        "gpll0",
        "gpll2",
        "gpll3",
        "gpll0_early_div"
};

static const struct parent_map gcc_xo_gpll0_gpll1_early_div_gpll1_gpll4_gpll0_early_div_map[] = {
        { P_XO, 0 },
        { P_GPLL0, 1 },
        { P_GPLL1_EARLY_DIV, 3 },
        { P_GPLL1, 4 },
        { P_GPLL4, 5 },
        { P_GPLL0_EARLY_DIV, 6 }
};

static const char * const gcc_xo_gpll0_gpll1_early_div_gpll1_gpll4_gpll0_early_div[] = {
        "xo",
        "gpll0",
        "gpll1_early_div",
        "gpll1",
        "gpll4",
        "gpll0_early_div"
};

static const struct parent_map gcc_xo_gpll0_gpll2_gpll3_gpll1_gpll2_early_gpll0_early_div_map[] = {
        { P_XO, 0 },
        { P_GPLL0, 1 },
        { P_GPLL2, 2 },
        { P_GPLL3, 3 },
        { P_GPLL1, 4 },
        { P_GPLL2_EARLY, 5 },
        { P_GPLL0_EARLY_DIV, 6 }
};

static const char * const gcc_xo_gpll0_gpll2_gpll3_gpll1_gpll2_early_gpll0_early_div[] = {
        "xo",
        "gpll0",
        "gpll2",
        "gpll3",
        "gpll1",
        "gpll2_early",
        "gpll0_early_div"
};


static const struct parent_map gcc_xo_gpll0_gpll2_gpll3_gpll1_gpll4_gpll0_early_div_map[] = {
        { P_XO, 0 },
        { P_GPLL0, 1 },
        { P_GPLL2, 2 },
        { P_GPLL3, 3 },
        { P_GPLL1, 4 },
        { P_GPLL4, 5 },
        { P_GPLL0_EARLY_DIV, 6 }
};

static const char * const gcc_xo_gpll0_gpll2_gpll3_gpll1_gpll4_gpll0_early_div[] = {
        "xo",
        "gpll0",
        "gpll2",
        "gpll3",
        "gpll1",
        "gpll4",
        "gpll0_early_div"
};


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

static struct clk_alpha_pll gpll0_early = {
        .offset = 0x00000,
        .clkr = {
                .enable_reg = 0x52000,
                .enable_mask = BIT(0),
                .hw.init = &(struct clk_init_data){
                        .name = "gpll0_early",
                        .parent_names = (const char *[]){ "xo" },
                        .num_parents = 1,
                        .ops = &clk_alpha_pll_ops,
                },
        },
};

static struct clk_fixed_factor gpll0_early_div = {
        .mult = 1,
        .div = 2,
        .hw.init = &(struct clk_init_data){
                .name = "gpll0_early_div",
                .parent_names = (const char *[]){ "gpll0_early" },
                .num_parents = 1,
                .ops = &clk_fixed_factor_ops,
        },
};

static struct clk_alpha_pll_postdiv gpll0 = {
        .offset = 0x00000,
        .clkr.hw.init = &(struct clk_init_data){
                .name = "gpll0",
                .parent_names = (const char *[]){ "gpll0_early" },
                .num_parents = 1,
                .ops = &clk_alpha_pll_postdiv_ops,
        },
};


static struct clk_alpha_pll gpll4_early = {
        .offset = 0x77000,
        .clkr = {
                .enable_reg = 0x52000,
                .enable_mask = BIT(4),
                .hw.init = &(struct clk_init_data){
                        .name = "gpll4_early",
                        .parent_names = (const char *[]){ "xo" },
                        .num_parents = 1,
                        .ops = &clk_alpha_pll_ops,
                },
        },
};

static struct clk_alpha_pll_postdiv gpll4 = {
        .offset = 0x77000,
        .clkr.hw.init = &(struct clk_init_data){
                .name = "gpll4",
                .parent_names = (const char *[]){ "gpll4_early" },
                .num_parents = 1,
                .ops = &clk_alpha_pll_postdiv_ops,
        },
};



#define GPLL0_MODE                                       (0x0000)
#define SYS_NOC_USB3_AXI_CBCR                            (0x03FC)
#define MSS_Q6_BIMC_AXI_CBCR                             (0x0284)
#define USB_30_BCR                                       (0x03C0)
#define USB30_MASTER_CBCR                                (0x03C8)
#define USB30_SLEEP_CBCR                                 (0x03CC)
#define USB30_MOCK_UTMI_CBCR                             (0x03D0)
#define USB30_MASTER_CMD_RCGR                            (0x03D4)
#define USB30_MOCK_UTMI_CMD_RCGR                         (0x03E8)
#define USB3_PHY_BCR                                     (0x1400)
#define USB3PHY_PHY_BCR                                  (0x1404)
#define USB3_PHY_AUX_CBCR                                (0x1408)
#define USB3_PHY_PIPE_CBCR                               (0x140C)
#define USB3_PHY_AUX_CMD_RCGR                            (0x1414)
#define USB_HS_BCR                                       (0x0480)
#define USB_HS_SYSTEM_CBCR                               (0x0484)
#define USB_HS_AHB_CBCR                                  (0x0488)
#define USB_HS_SYSTEM_CMD_RCGR                           (0x0490)
#define USB2_HS_PHY_SLEEP_CBCR                           (0x04AC)
#define USB2_HS_PHY_ONLY_BCR                             (0x04B0)
#define QUSB2_PHY_BCR                                    (0x04B8)
#define USB_PHY_CFG_AHB2PHY_CBCR                         (0x1A84)
#define SDCC1_APPS_CMD_RCGR                              (0x04D0)
#define SDCC1_APPS_CBCR                                  (0x04C4)
#define SDCC1_AHB_CBCR                                   (0x04C8)
#define SDCC2_APPS_CMD_RCGR                              (0x0510)
#define SDCC2_APPS_CBCR                                  (0x0504)
#define SDCC2_AHB_CBCR                                   (0x0508)
#define SDCC3_APPS_CMD_RCGR                              (0x0550)
#define SDCC3_APPS_CBCR                                  (0x0544)
#define SDCC3_AHB_CBCR                                   (0x0548)
#define SDCC4_APPS_CMD_RCGR                              (0x0590)
#define SDCC4_APPS_CBCR                                  (0x0584)
#define SDCC4_AHB_CBCR                                   (0x0588)
#define BLSP1_AHB_CBCR                                   (0x05C4)
#define BLSP1_QUP1_SPI_APPS_CBCR                         (0x0644)
#define BLSP1_QUP1_I2C_APPS_CBCR                         (0x0648)
#define BLSP1_QUP1_I2C_APPS_CMD_RCGR                     (0x0660)
#define BLSP1_QUP2_I2C_APPS_CMD_RCGR                     (0x06E0)
#define BLSP1_QUP3_I2C_APPS_CMD_RCGR                     (0x0760)
#define BLSP1_QUP4_I2C_APPS_CMD_RCGR                     (0x07E0)
#define BLSP1_QUP5_I2C_APPS_CMD_RCGR                     (0x0860)
#define BLSP1_QUP6_I2C_APPS_CMD_RCGR                     (0x08E0)
#define BLSP2_QUP1_I2C_APPS_CMD_RCGR                     (0x09A0)
#define BLSP2_QUP2_I2C_APPS_CMD_RCGR                     (0x0A20)
#define BLSP2_QUP3_I2C_APPS_CMD_RCGR                     (0x0AA0)
#define BLSP2_QUP4_I2C_APPS_CMD_RCGR                     (0x0B20)
#define BLSP2_QUP5_I2C_APPS_CMD_RCGR                     (0x0BA0)
#define BLSP2_QUP6_I2C_APPS_CMD_RCGR                     (0x0C20)
#define BLSP1_QUP1_SPI_APPS_CMD_RCGR                     (0x064C)
#define BLSP1_UART1_APPS_CBCR                            (0x0684)
#define BLSP1_UART1_APPS_CMD_RCGR                        (0x068C)
#define BLSP1_QUP2_SPI_APPS_CBCR                         (0x06C4)
#define BLSP1_QUP2_I2C_APPS_CBCR                         (0x06C8)
#define BLSP1_QUP2_SPI_APPS_CMD_RCGR                     (0x06CC)
#define BLSP1_UART2_APPS_CBCR                            (0x0704)
#define BLSP1_UART2_APPS_CMD_RCGR                        (0x070C)
#define BLSP1_QUP3_SPI_APPS_CBCR                         (0x0744)
#define BLSP1_QUP3_I2C_APPS_CBCR                         (0x0748)
#define BLSP1_QUP3_SPI_APPS_CMD_RCGR                     (0x074C)
#define BLSP1_UART3_APPS_CBCR                            (0x0784)
#define BLSP1_UART3_APPS_CMD_RCGR                        (0x078C)
#define BLSP1_QUP4_SPI_APPS_CBCR                         (0x07C4)
#define BLSP1_QUP4_I2C_APPS_CBCR                         (0x07C8)
#define BLSP1_QUP4_SPI_APPS_CMD_RCGR                     (0x07CC)
#define BLSP1_UART4_APPS_CBCR                            (0x0804)
#define BLSP1_UART4_APPS_CMD_RCGR                        (0x080C)
#define BLSP1_QUP5_SPI_APPS_CBCR                         (0x0844)
#define BLSP1_QUP5_I2C_APPS_CBCR                         (0x0848)
#define BLSP1_QUP5_SPI_APPS_CMD_RCGR                     (0x084C)
#define BLSP1_UART5_APPS_CBCR                            (0x0884)
#define BLSP1_UART5_APPS_CMD_RCGR                        (0x088C)
#define BLSP1_QUP6_SPI_APPS_CBCR                         (0x08C4)
#define BLSP1_QUP6_I2C_APPS_CBCR                         (0x08C8)
#define BLSP1_QUP6_SPI_APPS_CMD_RCGR                     (0x08CC)
#define BLSP1_UART6_APPS_CBCR                            (0x0904)
#define BLSP1_UART6_APPS_CMD_RCGR                        (0x090C)
#define BLSP2_AHB_CBCR                                   (0x0944)
#define BLSP2_QUP1_SPI_APPS_CBCR                         (0x0984)
#define BLSP2_QUP1_I2C_APPS_CBCR                         (0x0988)
#define BLSP2_QUP1_SPI_APPS_CMD_RCGR                     (0x098C)
#define BLSP2_UART1_APPS_CBCR                            (0x09C4)
#define BLSP2_UART1_APPS_CMD_RCGR                        (0x09CC)
#define BLSP2_QUP2_SPI_APPS_CBCR                         (0x0A04)
#define BLSP2_QUP2_I2C_APPS_CBCR                         (0x0A08)
#define BLSP2_QUP2_SPI_APPS_CMD_RCGR                     (0x0A0C)
#define BLSP2_UART2_APPS_CBCR                            (0x0A44)
#define BLSP2_UART2_APPS_CMD_RCGR                        (0x0A4C)
#define BLSP2_QUP3_SPI_APPS_CBCR                         (0x0A84)
#define BLSP2_QUP3_I2C_APPS_CBCR                         (0x0A88)
#define BLSP2_QUP3_SPI_APPS_CMD_RCGR                     (0x0A8C)
#define BLSP2_UART3_APPS_CBCR                            (0x0AC4)
#define BLSP2_UART3_APPS_CMD_RCGR                        (0x0ACC)
#define BLSP2_QUP4_SPI_APPS_CBCR                         (0x0B04)
#define BLSP2_QUP4_I2C_APPS_CBCR                         (0x0B08)
#define BLSP2_QUP4_SPI_APPS_CMD_RCGR                     (0x0B0C)
#define BLSP2_UART4_APPS_CBCR                            (0x0B44)
#define BLSP2_UART4_APPS_CMD_RCGR                        (0x0B4C)
#define BLSP2_QUP5_SPI_APPS_CBCR                         (0x0B84)
#define BLSP2_QUP5_I2C_APPS_CBCR                         (0x0B88)
#define BLSP2_QUP5_SPI_APPS_CMD_RCGR                     (0x0B8C)
#define BLSP2_UART5_APPS_CBCR                            (0x0BC4)
#define BLSP2_UART5_APPS_CMD_RCGR                        (0x0BCC)
#define BLSP2_QUP6_SPI_APPS_CBCR                         (0x0C04)
#define BLSP2_QUP6_I2C_APPS_CBCR                         (0x0C08)
#define BLSP2_QUP6_SPI_APPS_CMD_RCGR                     (0x0C0C)
#define BLSP2_UART6_APPS_CBCR                            (0x0C44)
#define BLSP2_UART6_APPS_CMD_RCGR                        (0x0C4C)
#define PDM_AHB_CBCR                                     (0x0CC4)
#define PDM2_CBCR                                        (0x0CCC)
#define PDM2_CMD_RCGR                                    (0x0CD0)
#define PRNG_AHB_CBCR                                    (0x0D04)
#define TSIF_AHB_CBCR                                    (0x0D84)
#define TSIF_REF_CBCR                                    (0x0D88)
#define TSIF_REF_CMD_RCGR                                (0x0D90)
#define BOOT_ROM_AHB_CBCR                                (0x0E04)
#define GCC_XO_DIV4_CBCR                                 (0x10C8)
#define APCS_GPLL_ENA_VOTE                               (0x1480)
#define APCS_CLOCK_BRANCH_ENA_VOTE                       (0x1484)
#define GCC_DEBUG_CLK_CTL                                (0x1880)
#define CLOCK_FRQ_MEASURE_CTL                            (0x1884)
#define CLOCK_FRQ_MEASURE_STATUS                         (0x1888)
#define PLLTEST_PAD_CFG                                  (0x188C)
#define GP1_CBCR                                         (0x1900)
#define GP1_CMD_RCGR                                     (0x1904)
#define GP2_CBCR                                         (0x1940)
#define GP2_CMD_RCGR                                     (0x1944)
#define GP3_CBCR                                         (0x1980)
#define GP3_CMD_RCGR                                     (0x1984)
#define GPLL4_MODE                                       (0x1DC0)
#define PCIE_0_SLV_AXI_CBCR                              (0x1AC8)
#define PCIE_0_MSTR_AXI_CBCR                             (0x1ACC)
#define PCIE_0_CFG_AHB_CBCR                              (0x1AD0)
#define PCIE_0_AUX_CBCR                                  (0x1AD4)
#define PCIE_0_PIPE_CBCR                                 (0x1AD8)
#define PCIE_0_PIPE_CMD_RCGR                             (0x1ADC)
#define PCIE_0_AUX_CMD_RCGR                              (0x1B00)
#define PCIE_PHY_0_PHY_BCR                               (0x1B14)
#define PCIE_PHY_0_BCR                                   (0x1B18)
#define PCIE_0_PHY_LDO_EN                                (0x1E00)
#define USB_SS_PHY_LDO_EN                                (0x1E08)

#if 0
static struct pll_vote_clk gpll0 = {
	.en_reg = (void __iomem *)APCS_GPLL_ENA_VOTE,
	.en_mask = BIT(0),
	.status_reg = (void __iomem *)GPLL0_MODE,
	.status_mask = BIT(30),
	.soft_vote = &soft_vote_gpll0,
	.soft_vote_mask = PLL_SOFT_VOTE_PRIMARY,
	.base = &virt_base,
	.c = {
		.rate = 600000000,
		.parent = &P_XO.c,
		.dbg_name = "gpll0",
		.ops = &clk_ops_pll_acpu_vote,
		CLK_INIT(gpll0.c),
	},
};

static struct pll_vote_clk gpll0_ao = {
	.en_reg = (void __iomem *)APCS_GPLL_ENA_VOTE,
	.en_mask = BIT(0),
	.status_reg = (void __iomem *)GPLL0_MODE,
	.status_mask = BIT(30),
	.soft_vote = &soft_vote_gpll0,
	.soft_vote_mask = PLL_SOFT_VOTE_ACPU,
	.base = &virt_base,
	.c = {
		.rate = 600000000,
		.parent = &P_XO_a_clk.c,
		.dbg_name = "gpll0_ao",
		.ops = &clk_ops_pll_acpu_vote,
		CLK_INIT(gpll0_ao.c),
	},
};

DEFINE_EXT_CLK(P_GPLL0, &gpll0.c);

static struct pll_vote_clk gpll4 = {
	.en_reg = (void __iomem *)APCS_GPLL_ENA_VOTE,
	.en_mask = BIT(4),
	.status_reg = (void __iomem *)GPLL4_MODE,
	.status_mask = BIT(30),
	.base = &virt_base,
	.c = {
		.rate = 1376000000,
		.parent = &P_XO.c,
		.dbg_name = "gpll4",
		.ops = &clk_ops_pll_vote,
		VDD_DIG_FMAX_MAP3(LOWER, 400000000, LOW, 800000000,
				  NOMINAL, 1600000000),
		CLK_INIT(gpll4.c),
	},
};
DEFINE_FIXED_SLAVE_DIV_CLK(gpll4_out_main, 4, &gpll4.c);
#endif


static const struct freq_tbl ftbl_blsp_uart_apps_clk_src[] = {
	F(   3686400, P_GPLL0,    1,   96, 15625),
	F(   7372800, P_GPLL0,    1,  192, 15625),
	F(  14745600, P_GPLL0,    1,  384, 15625),
	F(  16000000, P_GPLL0,    5,    2,    15),
	F(  19200000,         P_XO,    1,    0,     0),
	F(  24000000, P_GPLL0,    5,    1,     5),
	F(  32000000, P_GPLL0,    1,    4,    75),
	F(  40000000, P_GPLL0,   15,    0,     0),
	F(  46400000, P_GPLL0,    1,   29,   375),
	F(  48000000, P_GPLL0, 12.5,    0,     0),
	F(  51200000, P_GPLL0,    1,   32,   375),
	F(  56000000, P_GPLL0,    1,    7,    75),
	F(  58982400, P_GPLL0,    1, 1536, 15625),
	F(  60000000, P_GPLL0,   10,    0,     0),
	F(  63160000, P_GPLL0,  9.5,    0,     0),
	{ }
};

static struct clk_rcg2 blsp1_uart1_apps_clk_src = {
	.cmd_rcgr = BLSP1_UART1_APPS_CMD_RCGR,
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
	.cmd_rcgr = BLSP1_UART2_APPS_CMD_RCGR,
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


static struct clk_branch gcc_blsp1_uart1_apps_clk = {
	//.cbcr_reg = BLSP1_UART1_APPS_CBCR,
        .halt_reg = BLSP1_UART1_APPS_CBCR,
        .clkr = {
                .enable_reg = BLSP1_UART1_APPS_CBCR,
                .enable_mask = BIT(0),
                .hw.init = &(struct clk_init_data){
                        .name = "gcc_blsp1_uart1_apps_clk",
                        .parent_names = (const char *[]){ "blsp1_uart1_apps_clk_src" },
                        .num_parents = 1,
                        .flags = CLK_SET_RATE_PARENT,
                        .ops = &clk_branch2_ops,
                },
        },

};

static struct clk_branch gcc_blsp1_uart2_apps_clk = {
	//.cbcr_reg = BLSP1_UART2_APPS_CBCR,
        .halt_reg = BLSP1_UART2_APPS_CBCR,
        .clkr = {
                .enable_reg = BLSP1_UART2_APPS_CBCR,
                .enable_mask = BIT(0),
                .hw.init = &(struct clk_init_data){
                        .name = "gcc_blsp1_uart2_apps_clk",
                        .parent_names = (const char *[]){ "blsp1_uart2_apps_clk_src" },
                        .num_parents = 1,
                        .flags = CLK_SET_RATE_PARENT,
                        .ops = &clk_branch2_ops,
                },
        },
};

static struct clk_rcg2 blsp2_uart1_apps_clk_src = {
        //.cmd_rcgr = 0x2700c,
	.cmd_rcgr = BLSP2_UART1_APPS_CBCR,
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
        //.cmd_rcgr = 0x2900c,
        .cmd_rcgr = BLSP2_UART2_APPS_CBCR,
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



static struct clk_hw *gcc_msm8992_hws[] = {
        &xo.hw,
        &gpll0_early_div.hw,
        //&ufs_tx_cfg_clk_src.hw,
        //&ufs_rx_cfg_clk_src.hw,
        //&ufs_ice_core_postdiv_clk_src.hw,
};

#if 0
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

static struct clk_lookup msm_clocks_gcc_8992[] = {
	CLK_LIST(P_XO),
	CLK_LIST(P_XO_a_clk),
	CLK_LIST(gpll0),
	CLK_LIST(gpll0_ao),
	CLK_LIST(P_GPLL0),
	CLK_LIST(gpll4),
	CLK_LIST(gpll4_out_main),
	CLK_LIST(blsp1_qup6_spi_apps_clk_src),
	CLK_LIST(blsp1_uart1_apps_clk_src),
	CLK_LIST(blsp1_uart2_apps_clk_src),
	CLK_LIST(blsp2_uart1_apps_clk_src),
	CLK_LIST(blsp2_uart2_apps_clk_src),
	CLK_LIST(gp1_clk_src),
	CLK_LIST(gp2_clk_src),
	CLK_LIST(gp3_clk_src),
	CLK_LIST(pdm2_clk_src),
	CLK_LIST(tsif_ref_clk_src),
	CLK_LIST(usb_hs_system_clk_src),
	CLK_LIST(gpll0_out_mmsscc),
	CLK_LIST(gpll0_out_msscc),
	CLK_LIST(usb_ss_phy_ldo),
	CLK_LIST(gcc_blsp1_ahb_clk),
	CLK_LIST(gcc_blsp1_uart1_apps_clk),
	CLK_LIST(gcc_blsp1_uart2_apps_clk),
	CLK_LIST(gcc_blsp2_ahb_clk),
	CLK_LIST(gcc_blsp2_uart1_apps_clk),
	CLK_LIST(gcc_blsp2_uart2_apps_clk),
	CLK_LIST(gcc_boot_rom_ahb_clk),
	CLK_LIST(gcc_gp1_clk),
	CLK_LIST(gcc_gp2_clk),
	CLK_LIST(gcc_gp3_clk),
	CLK_LIST(gcc_mss_q6_bimc_axi_clk),
	CLK_LIST(gcc_pdm2_clk),
	CLK_LIST(gcc_pdm_ahb_clk),
	CLK_LIST(gcc_prng_ahb_clk),
	CLK_LIST(gcc_sys_noc_usb3_axi_clk),
	CLK_LIST(gcc_tsif_ahb_clk),
	CLK_LIST(gcc_tsif_ref_clk),
};

#endif

static struct clk_regmap *gcc_msm8992_clocks[] = {
        [GPLL0_EARLY] = &gpll0_early.clkr,
        [GPLL0] = &gpll0.clkr,
        [GPLL4_EARLY] = &gpll4_early.clkr,
        [GPLL4] = &gpll4.clkr,
        //[SYSTEM_NOC_CLK_SRC] = &system_noc_clk_src.clkr,
        //[CONFIG_NOC_CLK_SRC] = &config_noc_clk_src.clkr,
        //[PERIPH_NOC_CLK_SRC] = &periph_noc_clk_src.clkr,
        [BLSP1_UART1_APPS_CLK_SRC] = &blsp1_uart1_apps_clk_src.clkr,
        [BLSP1_UART2_APPS_CLK_SRC] = &blsp1_uart2_apps_clk_src.clkr,
        [BLSP2_UART1_APPS_CLK_SRC] = &blsp2_uart1_apps_clk_src.clkr,
        [BLSP2_UART2_APPS_CLK_SRC] = &blsp2_uart2_apps_clk_src.clkr,
	[GCC_BLSP1_UART1_APPS_CLK] = &gcc_blsp1_uart1_apps_clk.clkr,
	[GCC_BLSP1_UART2_APPS_CLK] = &gcc_blsp1_uart2_apps_clk.clkr,
};



//TODO - fix me.   These addresses are direct copy from 8996
static const struct qcom_reset_map gcc_msm8996_resets[] = {
        [GCC_SYSTEM_NOC_BCR] = { 0x4000 },
        [GCC_CONFIG_NOC_BCR] = { 0x5000 },
        [GCC_PERIPH_NOC_BCR] = { 0x6000 },
};

//TODO - verify these, from msm8996
static const struct regmap_config gcc_msm8992_regmap_config = {
        .reg_bits       = 32,
        .reg_stride     = 4,
        .val_bits       = 32,
        .max_register   = 0x8f010,
        .fast_io        = true,
};

//TODO - verify these , from msm8996
static const struct qcom_cc_desc gcc_msm8992_desc = {
        .config = &gcc_msm8992_regmap_config,
        .clks = gcc_msm8992_clocks,
        .num_clks = ARRAY_SIZE(gcc_msm8992_clocks),
        .resets = NULL, //gcc_msm8996_resets,
        .num_resets = 0, //ARRAY_SIZE(gcc_msm8996_resets),
        .gdscs = NULL, //gcc_msm8992_gdscs,
        .num_gdscs = 0, //ARRAY_SIZE(gcc_msm8992_gdscs),
};


static int gcc_msm8992_probe(struct platform_device *pdev)
{
	struct clk *clk;
	struct device *dev = &pdev->dev;
	int i;
	struct regmap *regmap;
	int ret = 0;
printk(" %s - qcom_cc_mapp \n", __func__);
WARN_ON(1);

	regmap = qcom_cc_map(pdev, &gcc_msm8992_desc);
	if (IS_ERR(regmap)) {
		dev_err(&pdev->dev, "Error mapping gcc\n");
		return PTR_ERR(regmap);
	}

	/*
         * Set the HMSS_AHB_CLK_SLEEP_ENA bit to allow the hmss_ahb_clk to be
         * turned off by hardware during certain apps low power modes.
         */
	//TODO - not sure if this is needed
        //regmap_update_bits(regmap, 0x52008, BIT(21), BIT(21));

printk(" %s - devm clk reg \n", __func__);
        for (i = 0; i < ARRAY_SIZE(gcc_msm8992_hws); i++) {
		dev_info(&pdev->dev, " devm registering clock #%d \n", i);
                clk = devm_clk_register(dev, gcc_msm8992_hws[i]);
                if (IS_ERR(clk)) {
			dev_err(&pdev->dev, " Error clk register #%d \n", i);
                        return PTR_ERR(clk);
		}
        }

printk(" %s - really probe \n", __func__);
	ret = qcom_cc_really_probe(pdev, &gcc_msm8992_desc, regmap);	

	if (ret != 0)
		dev_err(&pdev->dev, "Error cc_really probe\n");

	dev_info(&pdev->dev, "Registered GCC clocks.\n");
	return ret;
}

static struct of_device_id msm_clock_gcc_match_table[] = {
	{ .compatible = "qcom,gcc-8992" },
	{}
};
MODULE_DEVICE_TABLE(of, gcc_msm8992_match_table);

static struct platform_driver gcc_msm8992_driver = {
	.probe = gcc_msm8992_probe,
	.driver = {
		.name = "gcc-msm8992",
		.of_match_table = msm_clock_gcc_match_table,
		.owner = THIS_MODULE,
	},
};

static int __init msm_gcc_8992_init(void)
{
	printk("--> %s \n", __func__);
	return platform_driver_register(&gcc_msm8992_driver);
}
core_initcall(msm_gcc_8992_init);

static void __exit gcc_msm8992_exit(void)
{
	platform_driver_unregister(&gcc_msm8992_driver);
}
module_exit(gcc_msm8992_exit);


MODULE_DESCRIPTION("QCOM GCC MSM8992 Driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:gcc-msm8992");
