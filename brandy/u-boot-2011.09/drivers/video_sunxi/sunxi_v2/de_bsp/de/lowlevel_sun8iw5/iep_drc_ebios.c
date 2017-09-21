#include "iep_drc_ebios.h"

static volatile __iep_drc_dev_t *drc_dev[2];

#define ____SEPARATOR_GLOABL____

s32 DRC_EBIOS_Set_Reg_Base(u32 sel, u32 base)
{

  drc_dev[sel] = (__iep_drc_dev_t *)base;
  return 0;

}

u32 DRC_EBIOS_Get_Reg_Base(u32 sel)
{
  u32 ret = 0;

  ret = (u32)drc_dev[sel];
  return ret;

}

s32 DRC_EBIOS_Enable(u32 sel)
{

  drc_dev[sel]->gnectl.bits.en = 1;
  return 0;

}

s32 DRC_EBIOS_Disable(u32 sel)
{
    drc_dev[sel]->gnectl.bits.en = 0;
  return 0;

}

s32 DRC_EBIOS_Set_Mode(u32 sel, u32 mod)
{

  drc_dev[sel]->gnectl.bits.mod = mod;
  return 0;

}

s32 DRC_EBIOS_Bist_Enable(u32 sel, u32 en)
{

  drc_dev[sel]->gnectl.bits.bist_en = en;
  return 0;

}

s32 DRC_EBIOS_Set_Reg_Refresh_Edge(u32 sel, u32 falling)
{

  drc_dev[sel]->gnectl.bits.sync_edge = falling;
  return 0;

}
s32 DRC_EBIOS_Set_Csc_Coeff(u32 sel, u32 mod)
{

  if(mod==2)   
  {
    drc_dev[sel]->cscygcoff[0].bits.csc_yg_coff = 0x4a7;
    drc_dev[sel]->cscygcoff[1].bits.csc_yg_coff = 0x000;
    drc_dev[sel]->cscygcoff[2].bits.csc_yg_coff = 0x72c;
    drc_dev[sel]->cscygcon.bits.csc_yg_con =      0x307d;
    drc_dev[sel]->cscurcoff[0].bits.csc_ur_coff = 0x4a7;
    drc_dev[sel]->cscurcoff[1].bits.csc_ur_coff = 0x1f25;
    drc_dev[sel]->cscurcoff[2].bits.csc_ur_coff = 0x1ddd;
    drc_dev[sel]->cscurcon.bits.csc_ur_con =      0x4cf;
    drc_dev[sel]->cscvbcoff[0].bits.csc_vb_coff = 0x4a7;
    drc_dev[sel]->cscvbcoff[1].bits.csc_vb_coff = 0x875;
    drc_dev[sel]->cscvbcoff[2].bits.csc_vb_coff = 0x000;
    drc_dev[sel]->cscvbcon.bits.csc_vb_con =      0x2dea;
  }
  else if(mod==3)
  {
#if 0
    drc_dev[sel]->cscygcoff[0].bits.csc_yg_coff = 0x0c00;
    drc_dev[sel]->cscygcoff[1].bits.csc_yg_coff = 0x1c00;
    drc_dev[sel]->cscygcoff[2].bits.csc_yg_coff = 0x1c00;
    drc_dev[sel]->cscygcon.bits.csc_yg_con =      0x0000;
    drc_dev[sel]->cscurcoff[0].bits.csc_ur_coff = 0x0000;
    drc_dev[sel]->cscurcoff[1].bits.csc_ur_coff = 0x0400;
    drc_dev[sel]->cscurcoff[2].bits.csc_ur_coff = 0x0000;
    drc_dev[sel]->cscurcon.bits.csc_ur_con =      0x0000;
    drc_dev[sel]->cscvbcoff[0].bits.csc_vb_coff = 0x0000;
    drc_dev[sel]->cscvbcoff[1].bits.csc_vb_coff = 0x0000;
    drc_dev[sel]->cscvbcoff[2].bits.csc_vb_coff = 0x0400;
    drc_dev[sel]->cscvbcon.bits.csc_vb_con =      0x0000;
#else
    drc_dev[sel]->cscygcoff[0].bits.csc_yg_coff = 0x0400;
    drc_dev[sel]->cscygcoff[1].bits.csc_yg_coff = 0x0000;
    drc_dev[sel]->cscygcoff[2].bits.csc_yg_coff = 0x067B;
    drc_dev[sel]->cscygcon.bits.csc_yg_con =   0x330A;
    drc_dev[sel]->cscurcoff[0].bits.csc_ur_coff = 0x0400;
    drc_dev[sel]->cscurcoff[1].bits.csc_ur_coff = 0x1E59;
    drc_dev[sel]->cscurcoff[2].bits.csc_ur_coff = 0x1E1B;
    drc_dev[sel]->cscurcon.bits.csc_ur_con =   0x0719;
    drc_dev[sel]->cscvbcoff[0].bits.csc_vb_coff = 0x0400;
    drc_dev[sel]->cscvbcoff[1].bits.csc_vb_coff = 0x06B8;
    drc_dev[sel]->cscvbcoff[2].bits.csc_vb_coff = 0x0000;
    drc_dev[sel]->cscvbcon.bits.csc_vb_con =   0x328F;

#endif
  }
  else   
  {
    drc_dev[sel]->cscygcoff[0].bits.csc_yg_coff = 0x400;
    drc_dev[sel]->cscygcoff[1].bits.csc_yg_coff = 0x000;
    drc_dev[sel]->cscygcoff[2].bits.csc_yg_coff = 0x000;
    drc_dev[sel]->cscygcon.bits.csc_yg_con =    0x000;
    drc_dev[sel]->cscurcoff[0].bits.csc_ur_coff = 0x000;
    drc_dev[sel]->cscurcoff[1].bits.csc_ur_coff = 0x400;
    drc_dev[sel]->cscurcoff[2].bits.csc_ur_coff = 0x000;
    drc_dev[sel]->cscurcon.bits.csc_ur_con =      0x000;
    drc_dev[sel]->cscvbcoff[0].bits.csc_vb_coff = 0x000;
    drc_dev[sel]->cscvbcoff[1].bits.csc_vb_coff = 0x000;
    drc_dev[sel]->cscvbcoff[2].bits.csc_vb_coff = 0x400;
    drc_dev[sel]->cscvbcon.bits.csc_vb_con =      0x000;
  }
  return 0;

}

s32 DRC_EBIOS_Set_Display_Size(u32 sel, u32 width, u32 height)
{

  drc_dev[sel]->drcsize.bits.disp_w = width - 1;
  drc_dev[sel]->drcsize.bits.disp_h = height - 1;
  return 0;

}

s32 DRC_EBIOS_Win_Enable(u32 sel, u32 en)
{

  drc_dev[sel]->drcctl.bits.win_en = en;
  return 0;

}

u32 DRC_EBIOS_Set_Win_Para(u32 sel, u32 top, u32 bot, u32 left, u32 right)
{
  drc_dev[sel]->drc_wp0.bits.win_left = left;
  drc_dev[sel]->drc_wp0.bits.win_top = top;
  drc_dev[sel]->drc_wp1.bits.win_right = right;
  drc_dev[sel]->drc_wp1.bits.win_bottom = bot;
  return 0;

}

#define ____SEPARATOR_DRC____

s32 DRC_EBIOS_Drc_Cfg_Rdy(u32 sel)
{

  drc_dev[sel]->drcctl.bits.dbrdy_ctl = 1;
  drc_dev[sel]->drcctl.bits.db_en = 1;
  return 0;

}

u32 DRC_EBIOS_Drc_Set_Lgc_Addr(u32 sel, u32 addr)
{
#if defined(CONFIG_ARCH_SUN8IW1P1) || defined(CONFIG_ARCH_SUN8IW3P1) || defined(CONFIG_ARCH_SUN6I) || defined(CONFIG_ARCH_SUN8IW5P1)
  drc_dev[sel]->drclgc_addr.bits.lgc_addr = addr-0x40000000;
#else
  drc_dev[sel]->ext_drclgc_addr.bits.lgc_addr = addr;
#endif
  return 0;

}

u32 DRC_EBIOS_Drc_Set_Lgc_Autoload_Disable(u32 sel, u32 disable)
{

  drc_dev[sel]->drc_set.bits.gain_autoload_dis = disable;
  return 0;

}

u32 DRC_EBIOS_Drc_Adjust_Enable(u32 sel, u32 en)
{

  drc_dev[sel]->drc_set.bits.adjust_en = en;
  return 0;

}

u32 DRC_EBIOS_Drc_Set_Adjust_Para(u32 sel, u32 abslumperval, u32 abslumshf)
{

  drc_dev[sel]->drc_set.bits.lgc_abslumshf = abslumshf;
  drc_dev[sel]->drc_set.bits.lgc_abslumperval = abslumperval;
  return 0;

}

u32 DRC_EBIOS_Drc_Set_Spa_Coeff(u32 sel, u8 spatab[IEP_DRC_SPA_TAB_LEN])
{

  drc_dev[sel]->drcspacoff[0].bits.spa_coff0 = spatab[0];
  drc_dev[sel]->drcspacoff[0].bits.spa_coff1 = spatab[1];
  drc_dev[sel]->drcspacoff[0].bits.spa_coff2 = spatab[2];
  drc_dev[sel]->drcspacoff[1].bits.spa_coff0 = spatab[3];
  drc_dev[sel]->drcspacoff[1].bits.spa_coff1 = spatab[4];
  drc_dev[sel]->drcspacoff[1].bits.spa_coff2 = spatab[5];
  drc_dev[sel]->drcspacoff[2].bits.spa_coff0 = spatab[6];
  drc_dev[sel]->drcspacoff[2].bits.spa_coff1 = spatab[7];
  drc_dev[sel]->drcspacoff[2].bits.spa_coff2 = spatab[8];

  return 0;

}

u32 DRC_EBIOS_Drc_Set_Int_Coeff(u32 sel,  u8 inttab[IEP_DRC_INT_TAB_LEN])
{
  u32 i;

  for(i=0;i<IEP_DRC_INT_TAB_LEN/4;i++)
  {
    drc_dev[sel]->drcintcoff[i].bits.inten_coff0 = inttab[4*i];
    drc_dev[sel]->drcintcoff[i].bits.inten_coff1 = inttab[4*i+1];
    drc_dev[sel]->drcintcoff[i].bits.inten_coff2 = inttab[4*i+2];
    drc_dev[sel]->drcintcoff[i].bits.inten_coff3 = inttab[4*i+3];
  }

  return 0;

}

/*u32 DRC_EBIOS_Drc_Set_Lgc_Coeff(u32 sel,  u16 lgctab[IEP_DRC_INT_TAB_LEN])
{
  return 0;

}*/

u32 DRC_EBIOS_Drc_Set_Mode(u32 sel, u32 mode)
{
  if(mode == 0)
  {
    drc_dev[sel]->drcctl.bits.hsv_mode_en = 0;
  }
  else
  {
    drc_dev[sel]->drcctl.bits.hsv_mode_en = 1;
  }

  return 0;
}

#define ____SEPARATOR_LH____

u32 DRC_EBIOS_Lh_Set_Mode(u32 sel, u32 mod)
{

  drc_dev[sel]->lhctl.bits.lh_mod = mod; 
  return 0;

}

u32 DRC_EBIOS_Lh_Clr_Rec(u32 sel)
{

  drc_dev[sel]->lhctl.bits.lh_rec_clr = 1;
  return 0;

}

u32 DRC_EBIOS_Lh_Set_Thres(u32 sel, u8 thres[])
{

  drc_dev[sel]->lhthr0.bits.lh_thres_val1 = thres[0];
  drc_dev[sel]->lhthr0.bits.lh_thres_val2 = thres[1];
  drc_dev[sel]->lhthr0.bits.lh_thres_val3 = thres[2];
  drc_dev[sel]->lhthr0.bits.lh_thres_val4 = thres[3];
  drc_dev[sel]->lhthr1.bits.lh_thres_val5 = thres[4];
  drc_dev[sel]->lhthr1.bits.lh_thres_val6 = thres[5];
  drc_dev[sel]->lhthr1.bits.lh_thres_val7 = thres[6];
  return 0;

}

u32 DRC_EBIOS_Lh_Get_Sum_Rec(u32 sel, u32 *sum)
{
  u32 i;

  for(i=0;i<IEP_LH_INTERVAL_NUM;i++)
  {
    *sum++ = drc_dev[sel]->lhslum[i].bits.lh_lum_data;
  }
  return 0;

}

u32 DRC_EBIOS_Lh_Get_Cnt_Rec(u32 sel, u32 *cnt)
{
  u32 i;

  for(i=0;i<IEP_LH_INTERVAL_NUM;i++)
  {
    *cnt++ = drc_dev[sel]->lhscnt[i].bits.lh_cnt_data;
  }

  return 0;
}

