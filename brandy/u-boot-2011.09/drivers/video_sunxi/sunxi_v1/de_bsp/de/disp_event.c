#include "disp_event.h"
#include "disp_display.h"
#include "disp_de.h"
#include "disp_video.h"
#include "disp_scaler.h"
#include "disp_iep.h"
#include "disp_lcd.h"

#define VINT_TIME_LEN 100
static unsigned long vint_time_index[2] = {0,0};
static unsigned long vint_time[2][VINT_TIME_LEN];//jiffies
static unsigned long vint_count[2] = {0, 0};
static unsigned long lint_count[2] = {0, 0};

extern __panel_para_t gpanel_info[2];

__s32 bsp_disp_cmd_cache_get(__u32 screen_id)
{
	return  gdisp.screen[screen_id].cache_flag;
}

__s32 bsp_disp_cfg_get(__u32 screen_id)
{
	return gdisp.screen[screen_id].cfg_cnt;
}

__s32 bsp_disp_cmd_cache(__u32 screen_id)
{
#ifdef __LINUX_OSAL__
	unsigned long flags;
	spin_lock_irqsave(&gdisp.screen[screen_id].flag_lock, flags);
#endif
	gdisp.screen[screen_id].cache_flag = TRUE;
#ifdef __LINUX_OSAL__
	spin_unlock_irqrestore(&gdisp.screen[screen_id].flag_lock, flags);
#endif
	return DIS_SUCCESS;
}

__s32 bsp_disp_cmd_submit(__u32 screen_id)
{
#ifdef __LINUX_OSAL__
	unsigned long flags;
	spin_lock_irqsave(&gdisp.screen[screen_id].flag_lock, flags);
#endif
	gdisp.screen[screen_id].cache_flag = FALSE;
#ifdef __LINUX_OSAL__
	spin_unlock_irqrestore(&gdisp.screen[screen_id].flag_lock, flags);
#endif
	return DIS_SUCCESS;
}

__s32 bsp_disp_cfg_start(__u32 screen_id)
{
#ifdef __LINUX_OSAL__
	unsigned long flags;
	spin_lock_irqsave(&gdisp.screen[screen_id].flag_lock, flags);
#endif
	gdisp.screen[screen_id].cfg_cnt++;
#ifdef __LINUX_OSAL__
	spin_unlock_irqrestore(&gdisp.screen[screen_id].flag_lock, flags);
#endif
	return DIS_SUCCESS;
}

__s32 bsp_disp_cfg_finish(__u32 screen_id)
{
#ifdef __LINUX_OSAL__
	unsigned long flags;
	spin_lock_irqsave(&gdisp.screen[screen_id].flag_lock, flags);
#endif
	gdisp.screen[screen_id].cfg_cnt--;
#ifdef __LINUX_OSAL__
	spin_unlock_irqrestore(&gdisp.screen[screen_id].flag_lock, flags);
#endif
	return DIS_SUCCESS;
}

__s32 bsp_disp_vsync_event_enable(__u32 screen_id, __bool enable)
{
	gdisp.screen[screen_id].vsync_event_en = enable;

	return DIS_SUCCESS;
}
__s32 bsp_disp_get_fps(__u32 screen_id)
{
	__u32 pre_time_index, cur_time_index;
	__u32 pre_time, cur_time;
	__u32 fps = 0xff;

	pre_time_index = vint_time_index[screen_id];
	cur_time_index = (pre_time_index == 0)? (VINT_TIME_LEN -1):(pre_time_index-1);

	pre_time = vint_time[screen_id][pre_time_index];
	cur_time = vint_time[screen_id][cur_time_index];

	if(pre_time != cur_time) {
		fps = 1000 * 100 / (cur_time - pre_time);
	}

	return fps;
}

__s32 bsp_disp_get_vint_count(__u32 screen_id)
{
	return vint_count[screen_id];
}

__s32 disp_vint_checkin(__u32 screen_id)
{
#ifdef __LINUX_OSAL__
	vint_time[screen_id][vint_time_index[screen_id]] = jiffies;
#endif
	vint_time_index[screen_id] ++;
	vint_count[screen_id] ++;

	vint_time_index[screen_id] = (vint_time_index[screen_id] >= VINT_TIME_LEN)? 0:vint_time_index[screen_id];

	return 0;
}

__s32 disp_lint_checkin(__u32 screen_id)
{
	lint_count[screen_id] ++;

	return 0;
}

__s32 bsp_disp_get_lint_count(__u32 screen_id)
{
	return lint_count[screen_id];
}


__s32 disp_lcd_set_fps(__u32 screen_id);
void LCD_vbi_event_proc(__u32 screen_id, __u32 tcon_index)
{
	__u32 cur_line = 0, start_delay = 0;
	__u32 i = 0;
/*	__u32 num_scalers;

	num_scalers = bsp_disp_feat_get_num_scalers(); */

	disp_vint_checkin(screen_id);
	disp_lcd_set_fps(screen_id);

	Video_Operation_In_Vblanking(screen_id, tcon_index);

	cur_line = TCON_get_cur_line(screen_id, tcon_index);
	start_delay = TCON_get_start_delay(screen_id, tcon_index);
	if(cur_line > start_delay-4) {
		/* return while not  trigger mode  */
		if(gpanel_info[screen_id].lcd_fresh_mode == 0) {
			return ;
		}
	}

	if(gdisp.screen[screen_id].cache_flag == FALSE && gdisp.screen[screen_id].cfg_cnt == 0) {
		DE_BE_Cfg_Ready(screen_id);
		for(i=0; i<2; i++) {
			if((gdisp.scaler[i].status & SCALER_USED) && (gdisp.scaler[i].screen_index == screen_id))	{

				if(gdisp.scaler[i].b_close == TRUE) {
					Scaler_close(i);
					gdisp.scaler[i].b_close = FALSE;
				}	else {
					DE_SCAL_Set_Reg_Rdy(i);
				}
				gdisp.scaler[i].b_reg_change = FALSE;
			}
		}

		if(DISP_OUTPUT_TYPE_LCD == bsp_disp_get_output_type(screen_id)) {
		}

		gdisp.screen[screen_id].have_cfg_reg = TRUE;

		if(gdisp.init_para.take_effect) {
			gdisp.init_para.take_effect(screen_id);
		}

	}

#if 0
	cur_line = LCDC_get_cur_line(screen_id, tcon_index);

	if(cur_line > 5) {
		DE_INF("%d\n", cur_line);
	}
#endif

	return ;
}

void LCD_line_event_proc(__u32 screen_id)
{
	disp_lint_checkin(screen_id);
	if(gdisp.screen[screen_id].vsync_event_en && gdisp.init_para.vsync_event) {
		gdisp.init_para.vsync_event(screen_id);
	}

	if(gdisp.screen[screen_id].have_cfg_reg) {
		gdisp.init_para.disp_int_process(screen_id);
		gdisp.screen[screen_id].have_cfg_reg = FALSE;
	}
}
