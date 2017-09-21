/*
**********************************************************************************************************************
*
*						           the Embedded Secure Bootloader System
*
*
*						       Copyright(C), 2006-2014, Allwinnertech Co., Ltd.
*                                           All Rights Reserved
*
* File    :
*
* By      :
*
* Version : V2.00
*
* Date	  :
*
* Descript:
**********************************************************************************************************************
*/
#include "common.h"
#include "asm/io.h"
#include "asm/armv7.h"
#include "asm/arch/cpu.h"
#include "asm/arch/ccmu.h"
#include "asm/arch/timer.h"
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
static int clk_set_divd(void)
{
	unsigned int reg_val;

	reg_val = readl(CCM_CPU_L2_AXI_CTRL);
	reg_val &= ~(0x03 << 8);
	reg_val |=  (0x01 << 8);
	reg_val |=  (0x01 << 0);
	writel(reg_val, CCM_CPU_L2_AXI_CTRL);
	
	reg_val = readl(CCM_AHB1_APB1_CTRL);;
	reg_val &= ~((0x03 << 12) | (0x03 << 8) |(0x03 << 4));
	reg_val |=  (0x02 << 12);
	reg_val |=  (1 << 4);
	reg_val |=  (1 << 8);

	writel(reg_val, CCM_AHB1_APB1_CTRL);

	return 0;
}
/*******************************************************************************
*函数名称: set_pll
*函数原型：void set_pll( void )
*函数功能: 调整CPU频率
*入口参数: void
*返 回 值: void
*备    注:
*******************************************************************************/
void set_pll( void )
{
    unsigned int reg_val;
    unsigned int i;

    reg_val = readl(CCM_CPU_L2_AXI_CTRL);
    reg_val &= ~(0x01 << 16);
    reg_val |=  (0x01 << 16);
	reg_val |=  (0x01 << 0);
    writel(reg_val, CCM_CPU_L2_AXI_CTRL);
    for(i=0; i<0x400; i++);
    reg_val = (0x10 << 8) |(0x01 << 4)|(0x01<<31);
    writel(reg_val, CCM_PLL1_CPUX_CTRL);
#ifndef CONFIG_FPGA
	do
	{
		reg_val = readl(CCM_PLL1_CPUX_CTRL);
	}
	while(!(reg_val & (0x1 << 28)));
#endif
    clk_set_divd();
	writel(readl(CCM_AHB1_RESET_CTRL)  | (1 << 6), CCM_AHB1_RESET_CTRL);
	for(i=0;i<100;i++);
	writel(readl(CCM_AHB1_GATE0_CTRL) | (1 << 6), CCM_AHB1_GATE0_CTRL);
	writel(7, (0x01c20000+0x20));
	writel(0x80000000, CCM_MBUS_RESET_CTRL);      
	writel(0x81000002, CCM_MBUS_SCLK_CTRL0); 
	writel(readl(CCM_PLL6_MOD_CTRL) | (1U << 31), CCM_PLL6_MOD_CTRL);

    reg_val = readl(CCM_CPU_L2_AXI_CTRL);
    reg_val &= ~(0x03 << 16);
    reg_val |=  (0x02 << 16);
    writel(reg_val, CCM_CPU_L2_AXI_CTRL);
	__usdelay(1000);
	CP15DMB;
	CP15ISB;
	writel(readl(CCM_APB1_GATE0_CTRL)		|	(1 << 5), CCM_APB1_GATE0_CTRL);
	writel(readl(SUNXI_RPRCM_BASE + 0x28)   | 		0x01, SUNXI_RPRCM_BASE + 0x28);
    return  ;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
void reset_pll( void )
{
	writel(0x00010000, CCM_CPU_L2_AXI_CTRL);
	writel(0x00001000, CCM_PLL1_CPUX_CTRL);
	writel(0x00001010, CCM_AHB1_APB1_CTRL);

	return ;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
void set_gpio_gate(void)
{
	writel(readl(CCM_APB1_GATE0_CTRL)		|	(1 << 5), CCM_APB1_GATE0_CTRL);
	writel(readl(SUNXI_RPRCM_BASE + 0x28)	|		0x01, SUNXI_RPRCM_BASE + 0x28);
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
void set_ccmu_normal(void)
{
	writel(7, CCM_SECURITY_REG);
}

