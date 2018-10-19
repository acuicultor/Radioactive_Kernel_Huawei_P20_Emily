


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_DFS_OPTIMIZE

/*****************************************************************************
  1 ͷ�ļ�����
*****************************************************************************/

#include "oal_util.h"
#include "oam_ext_if.h"
#include "frw_ext_if.h"
#include "hal_commom_ops.h"
#include "dmac_ext_if.h"
#include "dmac_radar.h"
#include "dmac_device.h"
#include "dmac_resource.h"
#include "dmac_chan_mgmt.h"

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
#ifndef WIN32
#include <linux/timer.h>
#include <linux/timex.h>
#include <linux/rtc.h>
#endif
#endif /* #if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE) */

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_RADAR_C
/*****************************************************************************
  2 ȫ�ֱ�������
*****************************************************************************/
#define RADAR_FILTER_FCC_CRAZY_REPORT_DET
#define RADAR_FILTER_NORMAL_PULSE_TIMER

oal_uint8 g_auc_rptmem_np_fcc[]   = {5,8,8,8,7,5};
oal_uint8 g_auc_rptmem_np_etsi[]  = {5,5,5,7,5};

/*****************************************************************************
  3 ����ʵ��
*****************************************************************************/

OAL_STATIC oal_uint32 dmac_radar_get_pulse_margin(hal_dfs_radar_type_enum_uint8 en_radar_type, oal_uint32 ul_min_pri)
{
    oal_uint32              ul_pulse_margin = RADAR_PULSE_MARGIN_ETSI;

    if ( (HAL_DFS_RADAR_TYPE_FCC == en_radar_type) )
    {
        if ( OAL_TRUE == CHECK_RADAR_FCC_TYPE0_PRI(ul_min_pri)
            || OAL_TRUE == CHECK_RADAR_FCC_TYPE4_PRI(ul_min_pri)
            || OAL_TRUE == CHECK_RADAR_FCC_TYPE3_PRI(ul_min_pri))
        {
            ul_pulse_margin = RADAR_PULSE_MARGIN_ETSI;
        }
        else
        {
            ul_pulse_margin = RADAR_PULSE_MARGIN_FCC;
        }
    }
    else
    {
        ul_pulse_margin = RADAR_PULSE_MARGIN_ETSI;
    }

    return ul_pulse_margin;
}


OAL_STATIC oal_uint32 dmac_radar_check_pulse_period(hal_dfs_radar_type_enum_uint8 en_radar_type,
                oal_uint32 *pul_pri, oal_uint8 uc_cnt, oal_uint32 ul_min_pri)
{
    oal_uint8               uc_loop  = 0;
    oal_uint32              ul_tmp   = 0;
    oal_uint32              ul_times = 0;
    oal_uint32              ul_pulse_margin = 0;
    oal_uint8               uc_diff_num = 0;
    oal_bool_enum_uint8     en_big_period_limit = OAL_TRUE;
    oal_uint32              ul_tmp_max   = ul_min_pri;
    oal_uint32              ul_tmp_min   = ul_min_pri;

    ul_pulse_margin = dmac_radar_get_pulse_margin(en_radar_type, ul_min_pri);

    if ((HAL_DFS_RADAR_TYPE_FCC == en_radar_type && OAL_TRUE == CHECK_RADAR_FCC_TYPE0_PRI(ul_min_pri))
        || (HAL_DFS_RADAR_TYPE_KOREA == en_radar_type && OAL_ABSOLUTE_SUB(ul_min_pri, 333) <= 2))
    {
        en_big_period_limit = OAL_FALSE;
    }

    /* �жϼ���������� */
    for (uc_loop = 0; uc_loop < uc_cnt; uc_loop++)
    {
        ul_times = (pul_pri[uc_loop] + (ul_min_pri >> 1)) / ul_min_pri;
        ul_tmp   = pul_pri[uc_loop] / ul_times;

        if ( ul_tmp > ul_tmp_max )
        {
            ul_tmp_max = ul_tmp;
        }
        if ( ul_tmp < ul_tmp_min )
        {
            ul_tmp_min = ul_tmp;
        }

        if (OAL_ABSOLUTE_SUB(ul_min_pri, ul_tmp) > ul_pulse_margin)
        {
            uc_diff_num++;
            //return RADAR_PULSE_NO_PERIOD;
        }
        else
        {
            if (ul_times > MAX_PULSE_TIMES && OAL_TRUE == en_big_period_limit)
            {
                return RADAR_PULSE_BIG_PERIOD;
            }
        }
    }

    if ( OAL_ABSOLUTE_SUB(ul_tmp_max, ul_tmp_min) > ul_pulse_margin )
    {
        return RADAR_PULSE_NO_PERIOD;
    }

    /*���4��PRI�У�������Ϊmin_PRI�ı�����һ�����ɱ�������ʶ��Ϊ���ȼ������*/
    if(1 == uc_diff_num)
    {
        return RADAR_PULSE_ONE_DIFF_PRI;
    }
    else if(uc_diff_num > 1)
    {
        return RADAR_PULSE_NO_PERIOD;
    }

    return RADAR_PULSE_NORMAL_PERIOD;
}


OAL_STATIC oal_uint32 dmac_radar_analysis_pulse_info(
                                hal_radar_pulse_info_stru                *pst_pulse_info,
                                oal_uint32                                ul_analysis_cnt,
                                oal_uint8                                 uc_type,
                                dmac_radar_pulse_analysis_result_stru    *pst_result)
{
    oal_uint32                          ul_loop             = 0;
    oal_bool_enum_uint8                 en_first            = OAL_TRUE;
    oal_uint32                          ul_pre_timestamp    = 0;
    hal_pulse_info_stru                *pst_info            = OAL_PTR_NULL;
    oal_uint32                          uc_stagger_cnt      = 0;
    oal_uint16                          us_min_duration     = 0;
    oal_uint16                          us_max_duration     = 0;
    oal_uint32                          ul_min_pri          = 0xFFFFF;
    oal_uint32                          ul_max_pri          = 0xFFFFF;
    oal_uint16                          us_max_power        = 0;
    oal_uint16                          us_min_power        = 0;
    oal_uint8                           uc_index            = 0;
    oal_uint32                          ul_sum_power        = 0;
    oal_uint32                          ul_sum_duration     = 0;

    if (pst_pulse_info->ul_pulse_cnt >= ul_analysis_cnt)
    {
        ul_loop = pst_pulse_info->ul_pulse_cnt - ul_analysis_cnt;
    }

    pst_result->uc_begin = ul_loop;

    /* ���������࣬ͳ��stagger������� */
    for (; ul_loop < pst_pulse_info->ul_pulse_cnt; ul_loop++)
    {
        pst_info = &(pst_pulse_info->ast_pulse_info[ul_loop]);

        if (uc_type != pst_info->uc_type)
        {
            continue;
        }

        if (pst_info->us_duration <= STAGGER_MAX_DURATION)
        {
            uc_stagger_cnt++;
        }

        if (OAL_TRUE == en_first)
        {
            us_min_duration = pst_info->us_duration;
            us_max_power    = pst_info->us_power;
            us_min_power    = pst_info->us_power;
            ul_sum_power    += pst_info->us_power;
            us_max_duration = pst_info->us_duration;
            ul_sum_duration += pst_info->us_duration;

            pst_result->uc_begin = ul_loop;

            en_first        = OAL_FALSE;
        }
        else
        {
            us_min_duration = (pst_info->us_duration >= us_min_duration) ? us_min_duration : pst_info->us_duration;
            us_max_duration = (pst_info->us_duration <= us_max_duration) ? us_max_duration : pst_info->us_duration;

            us_max_power    = (pst_info->us_power < us_max_power) ? us_max_power : pst_info->us_power;
            us_min_power    = (pst_info->us_power > us_min_power) ? us_min_power : pst_info->us_power;
            ul_sum_power    += pst_info->us_power;
            ul_sum_duration += pst_info->us_duration;

            pst_result->aul_pri[uc_index] = (pst_info->ul_timestamp >= ul_pre_timestamp) ? (pst_info->ul_timestamp - ul_pre_timestamp)
                                  : (pst_info->ul_timestamp + 0xFFFFF - ul_pre_timestamp);
            ul_min_pri = (pst_result->aul_pri[uc_index] > ul_min_pri) ? ul_min_pri : pst_result->aul_pri[uc_index];
            ul_max_pri = (pst_result->aul_pri[uc_index] <= ul_max_pri) ? ul_max_pri : pst_result->aul_pri[uc_index];
            uc_index++;
        }

        ul_pre_timestamp = pst_info->ul_timestamp;

    }

    pst_result->uc_stagger_cnt  = uc_stagger_cnt;
    pst_result->us_min_duration = us_min_duration;
    pst_result->us_avrg_power   = ul_sum_power/(uc_index+1);
    pst_result->uc_pri_cnt      = uc_index;
    pst_result->ul_min_pri      = ul_min_pri;
    pst_result->us_max_power    = us_max_power;
    pst_result->us_duration_diff = us_max_duration - us_min_duration;
    pst_result->ul_pri_diff     = ul_max_pri - ul_min_pri;
    pst_result->us_power_diff   = us_max_power - us_min_power;
    pst_result->us_avrg_duration = ul_sum_duration/(uc_index+1);

    if (pst_result->uc_begin > 1 && 5 == ul_analysis_cnt)
    {
        ul_pre_timestamp = pst_pulse_info->ast_pulse_info[pst_result->uc_begin - 1].ul_timestamp;
        pst_result->ul_extra_pri = (pst_pulse_info->ast_pulse_info[pst_result->uc_begin].ul_timestamp >= ul_pre_timestamp) ?
            (pst_pulse_info->ast_pulse_info[pst_result->uc_begin].ul_timestamp - ul_pre_timestamp)
          : (pst_pulse_info->ast_pulse_info[pst_result->uc_begin].ul_timestamp + 0xFFFFF - ul_pre_timestamp);
    }

    return OAL_SUCC;
}

#ifndef WIN32
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
OAL_STATIC oal_void dmac_radar_get_sys_time(oal_bool_enum_uint8 en_log_switch)
{
    oal_uint32 ul_curr_timestamp;
    ul_curr_timestamp = (oal_uint32)OAL_TIME_GET_STAMP_MS();
    if(en_log_switch)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY,"Curren time : %u ms.", ul_curr_timestamp);
    }
}

#else

OAL_STATIC oal_void dmac_radar_get_sys_time(oal_bool_enum_uint8 en_log_switch)
{
    struct timex  txc;
    struct rtc_time tm;
    do_gettimeofday(&(txc.time));
    rtc_time_to_tm(txc.time.tv_sec,&tm);
    if(en_log_switch)
    {
        OAL_IO_PRINT("UTC time :%d-%d-%d %d:%d:%d\n",
            tm.tm_year+1900,tm.tm_mon+1, tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec);
    }
}
#endif
#endif



OAL_STATIC oal_void dmac_radar_pulse_log(
                        hal_radar_pulse_info_stru               *pst_pulse_info,
                        dmac_radar_pulse_analysis_result_stru   *pst_result,
                        oal_bool_enum_uint8          en_log_switch,
                        oal_uint8                    uc_type)
{
    oal_uint8                      uc_loop  = 0;
    oal_uint8                      uc_begin = pst_result->uc_begin;
    oal_uint32                     ul_pri;
    oal_uint8                      uc_idx   = 0;

    OAM_WARNING_LOG4(0, OAM_SF_DFS, "{dmac_radar_pulse_log::[DFS]min_duration: %d, min_intvl: %d, mean_power: %d, begin: %d!",
           pst_result->us_min_duration,
           pst_result->ul_min_pri,
           pst_result->us_avrg_power,
           pst_result->uc_begin);

    for (uc_loop = uc_begin; uc_loop < pst_pulse_info->ul_pulse_cnt; uc_loop++)
    {
        if ( uc_type != pst_pulse_info->ast_pulse_info[uc_loop].uc_type)
        {
            continue;
        }

        OAM_WARNING_LOG4(0, OAM_SF_DFS, "{dmac_radar_pulse_log::duration: %d, power: %d, intvl: %u, pulse_type: %d",
             pst_pulse_info->ast_pulse_info[uc_loop].us_duration,
             pst_pulse_info->ast_pulse_info[uc_loop].us_power,
             pst_result->aul_pri[uc_idx++],
             pst_pulse_info->ast_pulse_info[uc_loop].uc_type);
    }

    if (OAL_FALSE == en_log_switch)
    {
        return;
    }

    OAL_IO_PRINT("\n{min_duration: %d, min_intvl: %d, mean_power: %d, begin: %d}\r\n",
                   pst_result->us_min_duration,
                   pst_result->ul_min_pri,
                   pst_result->us_avrg_power,
                   pst_result->uc_begin);

    uc_idx = 0;

    if( 1 == en_log_switch || RADAR_CHIRP_PULSE_TYPE == uc_type)
    {
        for (uc_loop = uc_begin; uc_loop < pst_pulse_info->ul_pulse_cnt; uc_loop++)
        {
            if ( uc_type != pst_pulse_info->ast_pulse_info[uc_loop].uc_type)
            {
                continue;
            }

            OAL_IO_PRINT("{duration: %d, power: %d, intvl: %u, pulse_type: %d}\r\n",
                         pst_pulse_info->ast_pulse_info[uc_loop].us_duration,
                         pst_pulse_info->ast_pulse_info[uc_loop].us_power,
                         pst_result->aul_pri[uc_idx++],
                         pst_pulse_info->ast_pulse_info[uc_loop].uc_type);
        }
    }
    else if( 2 == en_log_switch && RADAR_CHIRP_PULSE_TYPE != uc_type)
    {
        if ( uc_begin >= 3)
        {
            uc_begin = uc_begin -3;
        }
        else
        {
            uc_begin = 0;
        }

        for (uc_loop = uc_begin; uc_loop < pst_pulse_info->ul_pulse_cnt; uc_loop++)
        {

            if(uc_loop >= pst_result->uc_begin)
            {
                ul_pri = pst_result->aul_pri[uc_loop - pst_result->uc_begin];
            }
            else
            {
                ul_pri = (pst_pulse_info->ast_pulse_info[uc_loop+1].ul_timestamp >= pst_pulse_info->ast_pulse_info[uc_loop].ul_timestamp) ?
            (pst_pulse_info->ast_pulse_info[uc_loop+1].ul_timestamp - pst_pulse_info->ast_pulse_info[uc_loop].ul_timestamp)
            : (pst_pulse_info->ast_pulse_info[uc_loop+1].ul_timestamp + 0xFFFFF - pst_pulse_info->ast_pulse_info[uc_loop].ul_timestamp);
            }

            OAL_IO_PRINT("{duration: %d, power: %d, intvl: %u, pulse_type: %d}\r\n",
                         pst_pulse_info->ast_pulse_info[uc_loop].us_duration,
                         pst_pulse_info->ast_pulse_info[uc_loop].us_power,
                         ul_pri,
                         pst_pulse_info->ast_pulse_info[uc_loop].uc_type);
        }
    }
}


OAL_STATIC oal_bool_enum_uint8 dmac_radar_check_chirp_pri(hal_dfs_radar_type_enum_uint8 en_radar_type, oal_uint32 ul_pri)
{
    oal_uint32              ul_min_pri;

    if (HAL_DFS_RADAR_TYPE_ETSI == en_radar_type)
    {
        ul_min_pri = MIN_ETSI_CHIRP_PRI;
    }
    else if (HAL_DFS_RADAR_TYPE_FCC == en_radar_type)
    {
        ul_min_pri = MIN_FCC_CHIRP_PRI;
    }
    else if (HAL_DFS_RADAR_TYPE_MKK == en_radar_type)
    {
        ul_min_pri = MIN_MKK_CHIRP_PRI;
    }
    else
    {
        ul_min_pri = 0;
    }

    if (ul_pri <= ul_min_pri || 0 == ul_min_pri)
    {
        return OAL_FALSE;
    }

    return OAL_TRUE;
}



OAL_STATIC oal_uint32 dmac_radar_check_chrip_pulse_num(dmac_radar_pulse_analysis_result_stru *pst_result,
    hal_dfs_radar_filter_stru *pst_dfs_radar, oal_bool_enum_uint8 en_log_switch)
{
    oal_uint8                      uc_loop;
    oal_uint32                     ul_sum_pri;
    oal_uint8                      uc_idx;
    oal_uint8                      uc_pulse_num;

    /*һ���ж������600ms�������������5��ʶ��Ϊ��*/
    for ( uc_loop = 0 ; uc_loop < pst_result->uc_pri_cnt; uc_loop++ )
    {
        ul_sum_pri = 0;
        uc_pulse_num = 1;
        for ( uc_idx = uc_loop ; uc_idx < pst_result->uc_pri_cnt; uc_idx++ )
        {
            uc_pulse_num++;
            ul_sum_pri += pst_result->aul_pri[uc_idx];

            if (ul_sum_pri > MIN_FCC_TOW_TIMES_INT_PRI*1000 )
            {
                break;
            }
        }

        if ( (uc_pulse_num >= MAX_FCC_CHIRP_PULSE_CNT_IN_600US) &&
            (ul_sum_pri * MAX_FCC_CHIRP_PULSE_CNT_IN_600US <= MIN_FCC_TOW_TIMES_INT_PRI*1000 * uc_pulse_num) )
        {
            if(en_log_switch)
            {
                OAL_IO_PRINT("dmac_radar_analysis_chrip_pulse_num::loop=%d,idx=%d,sum_pri=%d,pulse_num=%d\n",
                    uc_loop, uc_idx, ul_sum_pri, uc_pulse_num);
            }
            return OAL_FALSE;
        }

    }

    return OAL_TRUE;
}


OAL_STATIC oal_uint32 dmac_radar_check_chrip_duration(dmac_radar_pulse_analysis_result_stru *pst_result,
    hal_radar_pulse_info_stru *pst_pulse_info, oal_bool_enum_uint8 en_log_switch)
{
    oal_uint8           uc_idx;

    /*chirp PRI 1000~2000, Ҫ��duration����<=4*/
    for(uc_idx = 0; uc_idx + 1 < pst_result->uc_pri_cnt ; uc_idx++)
    {
        if(OAL_TRUE == CHECK_RADAR_FCC_TYPE5_PRI(pst_result->aul_pri[uc_idx])
            && (OAL_ABSOLUTE_SUB(pst_pulse_info->ast_pulse_info[uc_idx].us_duration,
            pst_pulse_info->ast_pulse_info[uc_idx+1].us_duration) > RADAR_FCC_CHIRP_PULSE_DURATION_MARGIN))
        {
            if(en_log_switch)
            {
                OAL_IO_PRINT("dmac_radar_check_chrip_duration::pri=%d, idx=%d, duration1=%d, duration2=%d\n",
                    pst_result->aul_pri[uc_idx],
                    uc_idx,
                    pst_pulse_info->ast_pulse_info[uc_idx].us_duration,
                    pst_pulse_info->ast_pulse_info[uc_idx+1].us_duration);
            }
            return OAL_FALSE;
        }
    }

    return OAL_TRUE;
}


OAL_STATIC oal_uint32 dmac_radar_check_eq_duration_num(dmac_radar_pulse_analysis_result_stru *pst_result,
    hal_radar_pulse_info_stru *pst_pulse_info, oal_bool_enum_uint8 en_log_switch)
{
    oal_uint8           uc_idx;
    oal_uint8           uc_sub_idx;
    oal_uint8           uc_eq_num = 0;
    oal_uint8           uc_pulse_num = pst_result->uc_pri_cnt + 1;

    /*�ж�������ȵ�duration����*/
    for ( uc_idx = 0 ; uc_idx < uc_pulse_num - 1; uc_idx++ )
    {
        for ( uc_sub_idx = uc_idx + 1 ; uc_sub_idx < uc_pulse_num; uc_sub_idx++ )
        {
            if ( OAL_ABSOLUTE_SUB(pst_pulse_info->ast_pulse_info[uc_idx].us_duration,
                pst_pulse_info->ast_pulse_info[uc_sub_idx].us_duration) <= RADAR_FCC_CHIRP_PULSE_DURATION_MARGIN )
            {
                uc_eq_num++;
            }
            else
            {
                break;
            }

            if(en_log_switch)
            {
                OAL_IO_PRINT("dmac_radar_check_eq_duration_num::dur_idx[%d]=%u, dur_subidx[%d]=%u, uc_eq_num=%d\n",
                    uc_idx,pst_pulse_info->ast_pulse_info[uc_idx].us_duration,
                    uc_sub_idx,pst_pulse_info->ast_pulse_info[uc_sub_idx].us_duration,
                    uc_eq_num);
            }
        }
        uc_idx += uc_eq_num;

        if ( uc_eq_num >= MAX_FCC_CHIRP_EQ_DURATION_NUM)
        {
            return OAL_FALSE;
        }
        else
        {
            uc_eq_num = 0;
        }
    }

    return OAL_TRUE;
}


OAL_STATIC oal_bool_enum_uint8 dmac_radar_check_min_chirp_pulse_num(hal_dfs_radar_type_enum_uint8 en_radar_type, oal_uint8 uc_cnt)
{
    oal_uint32              ul_pri_num;

    if (HAL_DFS_RADAR_TYPE_ETSI == en_radar_type)
    {
        ul_pri_num = MIN_ETSI_CHIRP_PRI_NUM;
    }
    else if (HAL_DFS_RADAR_TYPE_FCC == en_radar_type)
    {
        ul_pri_num = MIN_FCC_CHIRP_PRI_NUM;
    }
    else if (HAL_DFS_RADAR_TYPE_MKK == en_radar_type)
    {
        ul_pri_num = MIN_MKK_CHIRP_PRI_NUM;
    }
    else
    {
        ul_pri_num = 0;
    }

    if (uc_cnt < ul_pri_num)
    {
        return OAL_FALSE;
    }

    return OAL_TRUE;
}


OAL_STATIC oal_bool_enum_uint8 dmac_radar_check_normal_pulse_num(oal_uint8 uc_cnt, hal_dfs_radar_type_enum_uint8 en_radar_type)
{
    if ( uc_cnt < MAX_RADAR_NORMAL_PULSE_ANA_CNT - 1)
    {
        return OAL_FALSE;
    }

    return OAL_TRUE;
}
#ifdef RADAR_FILTER_NORMAL_PULSE_TIMER
OAL_STATIC oal_bool_enum_uint8 dmac_radar_check_normal_pulse_timer_irq_num(hal_dfs_radar_type_enum_uint8 en_radar_type, oal_uint8 uc_radar_type_num,
        oal_uint8 uc_normal_pulse_irq_cnt, oal_uint8 uc_normal_pulse_irq_cnt_old)
{
    if (OAL_TRUE == CHECK_RADAR_ETSI_TYPE3_HW(en_radar_type, uc_radar_type_num)
        && OAL_TRUE == CHECK_RADAR_ETSI_TYPE3_IRQ_NUM(uc_normal_pulse_irq_cnt, uc_normal_pulse_irq_cnt_old))
    {
        return OAL_FALSE;
    }

    if (OAL_TRUE == CHECK_RADAR_ETSI_TYPE2_HW(en_radar_type, uc_radar_type_num)
        && OAL_TRUE == CHECK_RADAR_ETSI_TYPE2_IRQ_NUM(uc_normal_pulse_irq_cnt, uc_normal_pulse_irq_cnt_old))
    {
        return OAL_FALSE;
    }

    if (OAL_TRUE == CHECK_RADAR_FCC_TYPE4_HW(en_radar_type, uc_radar_type_num)
        && OAL_TRUE == CHECK_RADAR_FCC_TYPE4_IRQ_NUM(uc_normal_pulse_irq_cnt, uc_normal_pulse_irq_cnt_old))
    {
        return OAL_FALSE;
    }

    if (OAL_TRUE == CHECK_RADAR_FCC_TYPE3_HW(en_radar_type, uc_radar_type_num)
        && OAL_TRUE == CHECK_RADAR_FCC_TYPE3_IRQ_NUM(uc_normal_pulse_irq_cnt, uc_normal_pulse_irq_cnt_old))
    {
        return OAL_FALSE;
    }

    return  OAL_TRUE;
}
#endif

OAL_STATIC oal_bool_enum_uint8 dmac_radar_check_pulse_power(oal_uint16 us_power, hal_dfs_radar_type_enum_uint8 en_radar_type,
        oal_uint32 ul_min_pri, oal_uint16 us_min_duration)
{
    if (HAL_DFS_RADAR_TYPE_FCC == en_radar_type
        && (OAL_TRUE == CHECK_RADAR_FCC_TYPE0_PRI(ul_min_pri)
        || OAL_TRUE == CHECK_RADAR_FCC_TYPE0_PRI(ul_min_pri>>1))
        && us_power >= MIN_RADAR_PULSE_POWER_FCC_TYPE0)
    {
        return OAL_TRUE;
    }

    if (HAL_DFS_RADAR_TYPE_ETSI == en_radar_type
       && OAL_TRUE == CHECK_RADAR_ETSI_SHORT_PULSE(us_min_duration)
       && us_power >= MIN_RADAR_PULSE_POWER_ETSI_STAGGER)
    {
        return OAL_TRUE;
    }

    if (us_power < MIN_RADAR_PULSE_POWER)
    {
        return OAL_FALSE;
    }

    return OAL_TRUE;
}



OAL_STATIC oal_uint32 dmac_radar_check_fcc_chirp(dmac_radar_pulse_analysis_result_stru *pst_result,
                                    hal_radar_pulse_info_stru *pst_pulse_info,
                                    hal_dfs_radar_filter_stru *pst_dfs_radar,
                                    oal_uint32 ul_delta_time_for_pulse)
{
    /*����600ms�������������5�����ж�*/
    if( OAL_FALSE == dmac_radar_check_chrip_pulse_num(pst_result, pst_dfs_radar, pst_dfs_radar->en_log_switch))
    {
        pst_dfs_radar->uc_chirp_cnt = 0;
        OAM_WARNING_LOG0(0, OAM_SF_DFS, "{dmac_radar_pulse_log::[DFS]uc_chirp_cnt set 0 due to more than 5 irq in 600ms.!");
        return OAL_FALSE;
    }

    /*��������durationһ��*/
    if (OAL_FALSE == dmac_radar_check_chrip_duration(pst_result, pst_pulse_info, pst_dfs_radar->en_log_switch)
        && OAL_TRUE == pst_dfs_radar->st_dfs_pulse_check_filter.en_fcc_chirp_duration_diff)
    {
        pst_dfs_radar->uc_chirp_cnt = 0;
        if(pst_dfs_radar->en_log_switch)
        {
            OAL_IO_PRINT("dmac_radar_chirp_filter::uc_chirp_cnt set 0 due to duration diff.\n");
        }
        OAM_WARNING_LOG0(0, OAM_SF_DFS, "{dmac_radar_pulse_log::[DFS]uc_chirp_cnt set 0 due to duration diff.!");
        return OAL_FALSE;
    }

    /*���duration����*/
    if (OAL_FALSE == dmac_radar_check_eq_duration_num(pst_result, pst_pulse_info, pst_dfs_radar->en_log_switch)
        && OAL_TRUE == pst_dfs_radar->st_dfs_pulse_check_filter.en_fcc_chirp_eq_duration_num)
    {
        pst_dfs_radar->uc_chirp_cnt = 0;
        if(pst_dfs_radar->en_log_switch)
        {
            OAL_IO_PRINT("dmac_radar_chirp_filter::uc_chirp_cnt set 0 due to same duration num.\n");
        }
        OAM_WARNING_LOG0(0, OAM_SF_DFS, "{dmac_radar_pulse_log::[DFS]uc_chirp_cnt set 0 due to same duration num.!");
        return OAL_FALSE;
    }

    /*��鹦��һ����*/
    if ( OAL_FALSE == CHECK_RADAR_FCC_CHIRP_POW_DIFF(pst_result->us_power_diff)
        && OAL_TRUE == pst_dfs_radar->st_dfs_pulse_check_filter.en_fcc_chirp_pow_diff)
    {
        pst_dfs_radar->uc_chirp_cnt = 0;
        if(pst_dfs_radar->en_log_switch)
        {
            OAL_IO_PRINT("dmac_radar_chirp_filter::uc_chirp_cnt set 0 due to power diff.\n");
        }
        OAM_WARNING_LOG0(0, OAM_SF_DFS, "{dmac_radar_pulse_log::[DFS]uc_chirp_cnt set 0 due to power diff.!");
        return OAL_FALSE;
    }

    /*�Ե�һ���жϲ�����ʱ����*/
    if ( pst_dfs_radar->uc_chirp_cnt_total <= 1)
    {
        return OAL_TRUE;
    }

    /*����ʱ����<600ms���ж�*/
    /*����ʱ��������6s���жϣ��������жϼ���*/
    if (ul_delta_time_for_pulse < MIN_FCC_TOW_TIMES_INT_PRI
        || ul_delta_time_for_pulse > MAX_FCC_TOW_TIMES_INT_PRI)
    {
        pst_dfs_radar->uc_chirp_cnt = 0;
        if(pst_dfs_radar->en_log_switch)
        {
            OAL_IO_PRINT("dmac_radar_chirp_filter::uc_chirp_cnt set 0 due to delta_time_for_pulse=%u.\n", ul_delta_time_for_pulse);
        }
        OAM_WARNING_LOG1(0, OAM_SF_DFS, "{dmac_radar_pulse_log::[DFS]uc_chirp_cnt set 0 due to delta_time_for_pulse=%u.!",
           ul_delta_time_for_pulse);
        return OAL_FALSE;
    }

    return OAL_TRUE;
}


OAL_STATIC oal_bool_enum_uint8 dmac_radar_chirp_filter(hal_dfs_radar_filter_stru *pst_dfs_radar, hal_radar_pulse_info_stru *pst_pulse_info)
{
    oal_uint32                                    ul_timestamp          = 0;
    oal_uint32                                    ul_delta_time         = 0;
    oal_uint32                                    ul_delta_time_for_pulse  = 0;
    dmac_radar_pulse_analysis_result_stru         st_result;

    ul_timestamp  = (oal_uint32)OAL_TIME_GET_STAMP_MS();

    ul_delta_time = (oal_uint32)OAL_TIME_GET_RUNTIME(pst_dfs_radar->ul_last_burst_timestamp_for_chirp, ul_timestamp);
    ul_delta_time_for_pulse = (oal_uint32)OAL_TIME_GET_RUNTIME(pst_dfs_radar->ul_last_timestamp_for_chirp_pulse, ul_timestamp);

    OAL_MEMZERO(&st_result, OAL_SIZEOF(dmac_radar_pulse_analysis_result_stru));
    dmac_radar_analysis_pulse_info(pst_pulse_info, pst_pulse_info->ul_pulse_cnt, RADAR_CHIRP_PULSE_TYPE, &st_result);

    dmac_radar_pulse_log(pst_pulse_info, &st_result, pst_dfs_radar->en_log_switch, RADAR_CHIRP_PULSE_TYPE);

    /*ͳ��12s�����жϸ���*/
    if ( 0 != pst_dfs_radar->ul_last_burst_timestamp_for_chirp
        && (HAL_DFS_RADAR_TYPE_FCC == pst_dfs_radar->en_radar_type
      || HAL_DFS_RADAR_TYPE_MKK == pst_dfs_radar->en_radar_type))
    {
        pst_dfs_radar->uc_chirp_cnt_total++;
    }

#ifdef RADAR_FILTER_FCC_CRAZY_REPORT_DET
    if(OAL_TRUE == pst_dfs_radar->en_crazy_report_cnt)
    {
        pst_dfs_radar->uc_chirp_cnt_for_crazy_report_det++;
    }
#endif

    /*��������������*/
    if (OAL_FALSE == dmac_radar_check_min_chirp_pulse_num(pst_dfs_radar->en_radar_type, st_result.uc_pri_cnt))
    {
        pst_dfs_radar->uc_chirp_cnt = 0;
        return OAL_TRUE;
    }

    /* �������power */
    if (OAL_FALSE == dmac_radar_check_pulse_power(st_result.us_max_power, pst_dfs_radar->en_radar_type, st_result.ul_min_pri, st_result.us_min_duration))
    {
        pst_dfs_radar->uc_chirp_cnt = 0;
        return OAL_TRUE;
    }

    /* ���chirp����С��� */
    if (OAL_FALSE == dmac_radar_check_chirp_pri(pst_dfs_radar->en_radar_type, st_result.ul_min_pri))
    {
        pst_dfs_radar->uc_chirp_cnt = 0;
        return OAL_TRUE;
    }

    if (HAL_DFS_RADAR_TYPE_ETSI == pst_dfs_radar->en_radar_type)
    {
        if (0 != pst_dfs_radar->ul_last_burst_timestamp_for_chirp &&
            ul_delta_time < pst_dfs_radar->ul_chirp_time_threshold)
        {
            return OAL_TRUE;
        }

        /* ���chirp������ */
        if (RADAR_PULSE_NORMAL_PERIOD != dmac_radar_check_pulse_period(pst_dfs_radar->en_radar_type, st_result.aul_pri, st_result.uc_pri_cnt, st_result.ul_min_pri))
        {
            return OAL_TRUE;
        }

        pst_dfs_radar->ul_last_burst_timestamp_for_chirp = ul_timestamp;

        return OAL_FALSE;
    }
    else if (HAL_DFS_RADAR_TYPE_FCC == pst_dfs_radar->en_radar_type
      || HAL_DFS_RADAR_TYPE_MKK == pst_dfs_radar->en_radar_type)
    {
        pst_dfs_radar->ul_last_timestamp_for_chirp_pulse = ul_timestamp;

        /*12s�״�ĵ�һ���ж�*/
        if (ul_delta_time >= pst_dfs_radar->ul_chirp_time_threshold
         || pst_dfs_radar->uc_chirp_cnt_total == 0)
        {
            pst_dfs_radar->ul_last_burst_timestamp_for_chirp    = ul_timestamp;
            pst_dfs_radar->uc_chirp_cnt                         = 1;
            pst_dfs_radar->uc_chirp_cnt_total                   = 1;

            /*У���������ʲ��������*/
            dmac_radar_check_fcc_chirp(&st_result, pst_pulse_info, pst_dfs_radar, ul_delta_time_for_pulse);

            if(pst_dfs_radar->en_log_switch)
            {
                OAL_IO_PRINT("dmac_radar_chirp_filter::begin::chirp_cnt = %d.\n", pst_dfs_radar->uc_chirp_cnt);
            }

            OAM_WARNING_LOG1(0, OAM_SF_DFS, "{dmac_radar_pulse_log::[DFS]begin::chirp_cnt = %d!",
                   pst_dfs_radar->uc_chirp_cnt);

            return OAL_TRUE;
        }

        /*У���������ʲ��������*/
        if (OAL_FALSE == dmac_radar_check_fcc_chirp(&st_result, pst_pulse_info, pst_dfs_radar, ul_delta_time_for_pulse) )
        {
            return OAL_TRUE;
        }

        pst_dfs_radar->uc_chirp_cnt++;
        pst_dfs_radar->ul_last_timestamp_for_chirp_pulse = ul_timestamp;

        if(pst_dfs_radar->en_log_switch)
        {
            OAL_IO_PRINT("dmac_radar_chirp_filter::uc_chirp_cnt = %d\n", pst_dfs_radar->uc_chirp_cnt);
        }
        OAM_WARNING_LOG1(0, OAM_SF_DFS, "{dmac_radar_pulse_log::[DFS]uc_chirp_cnt=%d.!", pst_dfs_radar->uc_chirp_cnt);

        return OAL_TRUE;
    }
    else
    {
        return OAL_TRUE;
    }
}


OAL_STATIC oal_bool_enum_uint8 dmac_radar_check_normal_pulse_pri(hal_dfs_radar_type_enum_uint8 en_radar_type, oal_uint32 ul_pri)
{
    if (ul_pri < MIN_RADAR_PULSE_PRI)
    {
        return OAL_FALSE;
    }

    if (HAL_DFS_RADAR_TYPE_ETSI == en_radar_type)
    {
        /* ETSI�·�chirp�״���С���巶Χ��� */
        if (OAL_FALSE == CHECK_RADAR_ETSI_TYPE1_PRI(ul_pri)
         && OAL_FALSE == CHECK_RADAR_ETSI_TYPE2_PRI(ul_pri)
         && OAL_FALSE == CHECK_RADAR_ETSI_TYPE3_PRI(ul_pri)
         && OAL_FALSE == CHECK_RADAR_ETSI_TYPE5_PRI(ul_pri)
         && OAL_FALSE == CHECK_RADAR_ETSI_TYPE6_PRI(ul_pri))
        {
            return OAL_FALSE;
        }
    }

    return OAL_TRUE;
}


OAL_STATIC oal_bool_enum_uint8 dmac_radar_check_stagger_pulse_mode(oal_uint8 uc_stagger_cnt)
{
    if (uc_stagger_cnt >= MAX_RADAR_STAGGER_NUM)
    {
        return OAL_TRUE;
    }

    return OAL_FALSE;
}



OAL_STATIC oal_bool_enum_uint8 dmac_radar_check_stagger_min_duration(oal_uint16 us_min_duration)
{
    if (us_min_duration >= MIN_RADAR_STAGGER_DURATION )
    {
        return OAL_TRUE;
    }

    return OAL_FALSE;
}



OAL_STATIC oal_bool_enum_uint8 dmac_radar_check_stagger_avrg_duration(oal_uint16 us_min_duration)
{
    if (us_min_duration <= MEAN_RADAR_STAGGER_DURATION )
    {
        return OAL_TRUE;
    }

    return OAL_FALSE;
}



OAL_STATIC oal_bool_enum_uint8 dmac_radar_check_stagger_type6_duration_diff(oal_uint16 us_duration_diff, oal_uint32 ul_min_pri)
{
    if (OAL_TRUE == CHECK_RADAR_ETSI_TYPE6_PRI(ul_min_pri)
        && OAL_FALSE == CHECK_RADAR_ETSI_TYPE6_DURATION_DIFF(us_duration_diff))
    {
        return OAL_FALSE;
    }

    return OAL_TRUE;
}



OAL_STATIC oal_bool_enum_uint8 dmac_radar_check_stagger_type6_extra_pri(oal_uint32 ul_extra_pri, oal_uint32 ul_min_pri)
{
    if (OAL_TRUE == CHECK_RADAR_ETSI_TYPE6_PRI(ul_min_pri)
        && (0 != ul_extra_pri)
        && (ul_extra_pri <= ul_min_pri - 100))
    {
        return OAL_FALSE;
    }

    return OAL_TRUE;
}



OAL_STATIC oal_bool_enum_uint8 dmac_radar_check_normal_min_duration(oal_uint16 us_min_duration, oal_uint32 ul_min_pri,
    hal_dfs_radar_type_enum_uint8 en_radar_type)
{
    /*FCC type2, duration[1~5us] should <=10us*/
    if((HAL_DFS_RADAR_TYPE_FCC == en_radar_type
        || HAL_DFS_RADAR_TYPE_MKK == en_radar_type)
        && OAL_TRUE == CHECK_RADAR_FCC_TYPE2_PRI_SMALL(ul_min_pri)
        && us_min_duration > MAX_RADAR_NORMAL_DURATION_FCC_TYPE2)
    {
        return OAL_FALSE;
    }

    /*FCC type3/4, duration [4us~22us] */
    if((HAL_DFS_RADAR_TYPE_FCC == en_radar_type
        || HAL_DFS_RADAR_TYPE_MKK == en_radar_type)
        && OAL_TRUE == CHECK_RADAR_FCC_TYPE4_PRI_SMALL(ul_min_pri)
        && OAL_FALSE == CHECK_RADAR_FCC_TYPE4_DURATION(us_min_duration))
    {
        return OAL_FALSE;
    }

    if ((us_min_duration >= MIN_RADAR_NORMAL_DURATION && HAL_DFS_RADAR_TYPE_MKK != en_radar_type)
        || (us_min_duration >= MIN_RADAR_NORMAL_DURATION_MKK && HAL_DFS_RADAR_TYPE_MKK == en_radar_type))
    {
        return OAL_TRUE;
    }

    return OAL_FALSE;
}


OAL_STATIC oal_bool_enum_uint8 dmac_radar_check_normal_duration_diff(oal_uint16 us_duration_diff, oal_uint32 ul_min_pri,
    hal_dfs_radar_type_enum_uint8 en_radar_type)
{
    if((HAL_DFS_RADAR_TYPE_FCC == en_radar_type)
        && (OAL_TRUE == CHECK_RADAR_FCC_TYPE4_PRI(ul_min_pri)
        || OAL_TRUE == CHECK_RADAR_FCC_TYPE2_PRI(ul_min_pri))
        && OAL_FALSE == CHECK_RADAR_FCC_TYPE4_DURATION_DIFF(us_duration_diff))
    {
        return OAL_FALSE;
    }

    return OAL_TRUE;
}


OAL_STATIC dmac_radar_stagger_type_enum_uint8 dmac_radar_get_stagger_pri_type(oal_uint32 ul_t1, oal_uint32 ul_t2)
{
    if (OAL_TRUE == CHECK_RADAR_ETSI_TYPE5_PRI(ul_t1)
     && OAL_TRUE == CHECK_RADAR_ETSI_TYPE5_PRI(ul_t2))
    {
        return DMAC_RADAR_STAGGER_TYPE5;
    }

    if (OAL_TRUE == CHECK_RADAR_ETSI_TYPE6_PRI(ul_t1)
     && OAL_TRUE == CHECK_RADAR_ETSI_TYPE6_PRI(ul_t2))
    {
        return DMAC_RADAR_STAGGER_TYPE6;
    }

    return DMAC_RADAR_STAGGER_TYPE_INVALID;
}


OAL_STATIC oal_bool_enum_uint8 dmac_radar_check_pps_by_type(oal_uint32 ul_t1, oal_uint32 ul_t2, dmac_radar_stagger_type_enum_uint8 en_type)
{
    oal_uint32      ul_min_pps_diff;
    oal_uint32      ul_max_pps_diff;
    oal_uint32      ul_tmp;
    oal_uint32      ul_product;

    if (DMAC_RADAR_STAGGER_TYPE5 == en_type)
    {
        ul_min_pps_diff = RADAR_ETSI_TYPE5_MIN_PPS_DIFF;
        ul_max_pps_diff = RADAR_ETSI_TYPE5_MAX_PPS_DIFF;
    }
    else if (DMAC_RADAR_STAGGER_TYPE6 == en_type)
    {
        ul_min_pps_diff = RADAR_ETSI_TYPE6_MIN_PPS_DIFF;
        ul_max_pps_diff = RADAR_ETSI_TYPE6_MAX_PSS_DIFF;
    }
    else
    {
        return OAL_FALSE;
    }

    ul_tmp     = OAL_ABSOLUTE_SUB(ul_t1, ul_t2) * RADAR_ONE_SEC_IN_US;
    ul_product = ul_t1 * ul_t2;

    if (ul_min_pps_diff * ul_product < ul_tmp && ul_tmp < ul_max_pps_diff * ul_product)
    {
        return OAL_TRUE;
    }

    return OAL_FALSE;
}


OAL_STATIC oal_bool_enum_uint8 dmac_radar_check_pps(oal_uint32 ul_t1, oal_uint32 ul_t2)
{
    dmac_radar_stagger_type_enum_uint8          en_type;

    /* T1��T2�����ͬһ��PRI������ */
    if (OAL_ABSOLUTE_SUB(ul_t1, ul_t2) <= RADAR_PULSE_MARGIN_ETSI) //RADAR_PULSE_MARGIN)
    {
        return OAL_TRUE;
    }

    en_type = dmac_radar_get_stagger_pri_type(ul_t1, ul_t2);

    if (DMAC_RADAR_STAGGER_TYPE_INVALID == en_type)
    {
        return OAL_FALSE;
    }

    return dmac_radar_check_pps_by_type(ul_t1, ul_t2, en_type);
}


OAL_STATIC oal_bool_enum_uint8 dmac_radar_check_stagger_sum(oal_uint32 ul_dt1,
                                                                         oal_uint32 ul_dt2,
                                                                         oal_uint32 ul_dt3,
                                                                         oal_uint32 ul_t4)
{
    /*dT3=dT1����dT3=dT2, ��dt1, dt2�������������PPSҪ�������״��ź�*/
    //if ((OAL_ABSOLUTE_SUB(ul_dt1, ul_dt3) <= STAGGER_PULSE_MARGIN
     //|| OAL_ABSOLUTE_SUB(ul_dt3, ul_dt2) <= STAGGER_PULSE_MARGIN)
     //&&(OAL_TRUE == dmac_radar_check_pps(ul_dt1, ul_dt2)))
    //{
        //return OAL_TRUE;
    //}


    /* DT3��DT1������Type5��Type6 PRI��Χ�ڣ�ֱ�ӹ��� */
    if ((CHECK_RADAR_ETSI_TYPE5_PRI(ul_dt3) != OAL_TRUE || CHECK_RADAR_ETSI_TYPE5_PRI(ul_dt1) != OAL_TRUE)
      && (CHECK_RADAR_ETSI_TYPE6_PRI(ul_dt3) != OAL_TRUE || CHECK_RADAR_ETSI_TYPE6_PRI(ul_dt1) != OAL_TRUE))
    {
        return OAL_FALSE;
    }

    /*���T1��T2������T2, T3֮���pps������Ҫ������Ϊ����*/
    if(OAL_FALSE == dmac_radar_check_pps(ul_dt1, ul_dt2)
        ||  OAL_FALSE == dmac_radar_check_pps(ul_dt2, ul_dt3))
    {
        return OAL_FALSE;
    }

    if (OAL_ABSOLUTE_SUB(ul_t4, ul_dt1 + ul_dt2) <= TWO_TIMES_STAGGER_PULSE_MARGIN
     || OAL_ABSOLUTE_SUB(ul_t4, ul_dt1 + ul_dt3) <= TWO_TIMES_STAGGER_PULSE_MARGIN
     || OAL_ABSOLUTE_SUB(ul_t4, ul_dt2 + ul_dt3) <= TWO_TIMES_STAGGER_PULSE_MARGIN
     || OAL_ABSOLUTE_SUB(ul_t4, ul_dt1 + ul_dt2 + ul_dt3) <= THREE_TIMES_STAGGER_PULSE_MARGIN
     || OAL_ABSOLUTE_SUB(ul_t4, TWO_TIMES(ul_dt1) + ul_dt2 + ul_dt3) <= THREE_TIMES_STAGGER_PULSE_MARGIN
     || OAL_ABSOLUTE_SUB(ul_t4, ul_dt1 + TWO_TIMES(ul_dt2) + ul_dt3) <= THREE_TIMES_STAGGER_PULSE_MARGIN
     || OAL_ABSOLUTE_SUB(ul_t4, ul_dt1 + ul_dt2 + TWO_TIMES(ul_dt3)) <= THREE_TIMES_STAGGER_PULSE_MARGIN
     || OAL_ABSOLUTE_SUB(ul_t4, TWO_TIMES(ul_dt1) + TWO_TIMES(ul_dt2) + ul_dt3) <= THREE_TIMES_STAGGER_PULSE_MARGIN
     || OAL_ABSOLUTE_SUB(ul_t4, ul_dt1 + TWO_TIMES(ul_dt2) + TWO_TIMES(ul_dt3)) <= THREE_TIMES_STAGGER_PULSE_MARGIN
     || OAL_ABSOLUTE_SUB(ul_t4, TWO_TIMES(ul_dt1) + ul_dt2 + TWO_TIMES(ul_dt3)) <= THREE_TIMES_STAGGER_PULSE_MARGIN)
    {
        return OAL_TRUE;
    }

    return OAL_FALSE;
}


OAL_STATIC oal_bool_enum_uint8 dmac_radar_check_stagger_T2_T3(oal_uint32 ul_t1,
                                                                            oal_uint32 ul_t2,
                                                                            oal_uint32 ul_t3,
                                                                            oal_uint32 ul_t4)
{
    oal_uint32              ul_dt3;

    if ((ul_t2 < TWO_TIMES(ul_t1)) && (ul_t3 <= 3 * ul_t1) && (ul_t3 > TWO_TIMES(ul_t1)))
    {
        /* T2��T1������Type5��Type6 PRI��Χ�ڣ������״��� */
        if ((CHECK_RADAR_ETSI_TYPE5_PRI(ul_t2) != OAL_TRUE || CHECK_RADAR_ETSI_TYPE5_PRI(ul_t1) != OAL_TRUE)
          && (CHECK_RADAR_ETSI_TYPE6_PRI(ul_t2) != OAL_TRUE || CHECK_RADAR_ETSI_TYPE6_PRI(ul_t1) != OAL_TRUE))
        {
            return OAL_FALSE;
        }

        ul_dt3 = ul_t3 - ul_t1;

        /*dT3=dT2, ��dt1, dt2�������������PPSҪ�������״��ź�*/
        if ((OAL_ABSOLUTE_SUB(ul_dt3, ul_t2) <= STAGGER_PULSE_MARGIN)
         &&(OAL_TRUE == dmac_radar_check_pps(ul_t1, ul_t2)))
        {
            return OAL_TRUE;
        }

        if (OAL_TRUE == dmac_radar_check_stagger_sum(ul_t1, ul_t2, ul_dt3, ul_t4))
        {
            return OAL_TRUE;
        }

        ul_dt3 = ul_t3 - ul_t2;

        /*dT3=dT1, ��dt1, dt2�������������PPSҪ�������״��ź�*/
        if ((OAL_ABSOLUTE_SUB(ul_dt3, ul_t1) <= STAGGER_PULSE_MARGIN)
         &&(OAL_TRUE == dmac_radar_check_pps(ul_t1, ul_t2)))
        {
            return OAL_TRUE;
        }

        if (OAL_TRUE == dmac_radar_check_stagger_sum(ul_t1, ul_t2, ul_dt3, ul_t4))
        {
            return OAL_TRUE;
        }
    }

    return OAL_FALSE;
}


OAL_STATIC oal_bool_enum_uint8 dmac_radar_check_stagger_T3_T4(oal_uint32 ul_t1,
                                                              oal_uint32 ul_t2,
                                                              oal_uint32 ul_t3,
                                                              oal_uint32 ul_t4)
{
    if (ul_t3 < TWO_TIMES(ul_t1) && ul_t4 > TWO_TIMES(ul_t1))
    {
        /* T3��T1������Type5��Type6 PRI��Χ�ڣ�����Ϊ���󱨣�ֱ�ӹ��� */
        if ((CHECK_RADAR_ETSI_TYPE5_PRI(ul_t3) != OAL_TRUE || CHECK_RADAR_ETSI_TYPE5_PRI(ul_t1) != OAL_TRUE)
          && (CHECK_RADAR_ETSI_TYPE6_PRI(ul_t3) != OAL_TRUE || CHECK_RADAR_ETSI_TYPE6_PRI(ul_t1) != OAL_TRUE))
        {
            return OAL_FALSE;
        }

        /*���T1��T2������T2, T3֮���pps������Ҫ������Ϊ����*/
        if(OAL_FALSE == dmac_radar_check_pps(ul_t1, ul_t2)
            ||  OAL_FALSE == dmac_radar_check_pps(ul_t2, ul_t3))
        {
            return OAL_FALSE;
        }

        if (OAL_ABSOLUTE_SUB(ul_t4, ul_t1 + ul_t2) <= TWO_TIMES_STAGGER_PULSE_MARGIN
         || OAL_ABSOLUTE_SUB(ul_t4, ul_t1 + ul_t3) <= TWO_TIMES_STAGGER_PULSE_MARGIN
         || OAL_ABSOLUTE_SUB(ul_t4, ul_t2 + ul_t3) <= TWO_TIMES_STAGGER_PULSE_MARGIN
         || OAL_ABSOLUTE_SUB(ul_t4, ul_t1 + ul_t2 + ul_t3) <= THREE_TIMES_STAGGER_PULSE_MARGIN
         || OAL_ABSOLUTE_SUB(ul_t4, TWO_TIMES(ul_t1) + ul_t2 + ul_t3) <= FOUR_TIMES_STAGGER_PULSE_MARGIN
         || OAL_ABSOLUTE_SUB(ul_t4, ul_t1 + TWO_TIMES(ul_t2) + ul_t3) <= FOUR_TIMES_STAGGER_PULSE_MARGIN
         || OAL_ABSOLUTE_SUB(ul_t4, ul_t1 + ul_t2 + TWO_TIMES(ul_t3)) <= FOUR_TIMES_STAGGER_PULSE_MARGIN)
        {
            return OAL_TRUE;
        }
    }

    return OAL_FALSE;
}


OAL_STATIC oal_bool_enum_uint8 dmac_radar_check_stagger_type6(oal_uint32 *pul_pri, oal_uint32 *pul_pri_sort)
{
    oal_uint32  ul_t1, ul_t2, ul_t3, ul_t4;
    oal_uint32  ul_T1, ul_T2, ul_T3, ul_T4;
    oal_uint8   uc_loop;

    ul_t1 = pul_pri[0];
    ul_t2 = pul_pri[1];
    ul_t3 = pul_pri[2];
    ul_t4 = pul_pri[3];

    ul_T1 = pul_pri_sort[0];
    ul_T2 = pul_pri_sort[1];
    ul_T3 = pul_pri_sort[2];
    ul_T4 = pul_pri_sort[3];

    /*1. T1~T4������ͬ��Ϊ��*/
    for (uc_loop = 1; uc_loop < 4; uc_loop++)
    {
       if (OAL_ABSOLUTE_SUB(pul_pri_sort[uc_loop], pul_pri_sort[uc_loop-1]) <= STAGGER_PULSE_MARGIN)
       {
           break;
       }
    }

    if ( 4 == uc_loop && OAL_ABSOLUTE_SUB(ul_T4, ul_T1 + ul_T2 + ul_T3) <= THREE_TIMES_STAGGER_PULSE_MARGIN)
    {
        return OAL_TRUE;
    }

    /*2.���T2=T3=T4, ��T2>2T1�����״��������*/
    if (OAL_ABSOLUTE_SUB(ul_T2, ul_T3) <= STAGGER_PULSE_MARGIN
        && OAL_ABSOLUTE_SUB(ul_T2, ul_T4) <= STAGGER_PULSE_MARGIN
        && ul_T2 > TWO_TIMES(ul_T1))
    {
        return OAL_TRUE;
    }

    /*3.���T1=T2, T3=T4, ��t1=t3, t2=t4, ��T3<3T1�������״��źţ���������*/
    if (OAL_ABSOLUTE_SUB(ul_T1, ul_T2) <= STAGGER_PULSE_MARGIN
        && OAL_ABSOLUTE_SUB(ul_T3, ul_T4) <= STAGGER_PULSE_MARGIN
        && OAL_ABSOLUTE_SUB(ul_t1, ul_t3) <= STAGGER_PULSE_MARGIN
        && OAL_ABSOLUTE_SUB(ul_t2, ul_t4) <= STAGGER_PULSE_MARGIN
        && ul_T3 < THREE_TIMES(ul_T1))
    {
        return OAL_TRUE;
    }

    /*4. ���t1=t4��(a)T4<2T1 ���� (b)T4��T1, T2, T3����������ͬ��Ts֮�ͣ������״��������*/
    if ( OAL_ABSOLUTE_SUB(ul_t1, ul_t4) <= STAGGER_PULSE_MARGIN)
    {
        if(ul_T4 < TWO_TIMES(ul_T1))
        {
            return OAL_TRUE;
        }
        /*���1== uc_loop, ��T1=T2, �Ƚ�T4��T1+T3;���2==uc_loop, ��T1!=T2, T2=T3,�Ƚ�T4��T1+T3*/
        if ( OAL_ABSOLUTE_SUB(ul_T4, ul_T1 + ul_T3) <= TWO_TIMES_STAGGER_PULSE_MARGIN
            && uc_loop <= 2)
        {
            return OAL_TRUE;
        }
    }

    /*5. ���T3=T4, ��1)T3=T1+T2 or 2)T3<2T1�������״��������*/
    if (OAL_ABSOLUTE_SUB(ul_T3, ul_T4) <= STAGGER_PULSE_MARGIN
        && (OAL_ABSOLUTE_SUB(ul_T3, ul_T1 + ul_T2) <= TWO_TIMES_STAGGER_PULSE_MARGIN
        || ul_T3 < THREE_TIMES(ul_T1)))
    {
        return OAL_TRUE;
    }

    return OAL_FALSE;

}

OAL_STATIC dmac_radar_stagger_period_enum_uint8 dmac_radar_check_stagger_pri_equal(oal_uint32 *pul_pri,
                                        oal_uint8 uc_cnt, oal_uint8 *puc_pri_idx)
{
    oal_uint8           uc_loop     = 0;
    oal_bool_enum_uint8 en_eq_bool  = OAL_FALSE;
    oal_uint32          ul_t1       = pul_pri[0];
    oal_uint8           uc_eq_idx   = 0;
    oal_uint8           uc_basic_pri_idx = 0;

    for (uc_loop = 1; uc_loop < uc_cnt; uc_loop++)
    {
        if (OAL_ABSOLUTE_SUB(pul_pri[uc_loop], pul_pri[uc_loop-1]) <= STAGGER_PULSE_MARGIN)
        {
            en_eq_bool = OAL_TRUE;
            /*��¼��ͬPRI��idx*/
            uc_eq_idx = uc_loop-1;
            break;
        }
    }

    /* ��������ͬ��PRI��ֱ�ӷ��� */
    if (OAL_FALSE == en_eq_bool)
    {
        return DMAC_RADAR_STAGGER_PERIOD_NOT_DEC;
    }

    /***************************************************************
       �жϴ�T2,T3,T4;
       �������С��2T1, ��
       1)����stagger��PRI��Χ�ڣ�
       ����2)��T1��pps���첻�ڷ�Χ�ڣ�
       ����3)��ͬ��PRI���ڻ�����Ԫ��������2��
       �����ж�Ϊ�״��ź�
    ***************************************************************/
    for (uc_loop = 1; uc_loop < uc_cnt; uc_loop++)
    {
        if (pul_pri[uc_loop] < TWO_TIMES(ul_t1))
        {
            if(OAL_FALSE == CHECK_RADAR_ETSI_STAGGER_PRI(pul_pri[uc_loop])
            || OAL_FALSE == dmac_radar_check_pps(ul_t1, pul_pri[uc_loop]))
            {
                return DMAC_RADAR_STAGGER_PERIOD_PRI_ERR;
            }
            else
            {
                uc_basic_pri_idx = uc_loop;
            }
        }
    }

    /*���ԭʼ������������������Ԫ��ͬ����ʶ��Ϊ��*/
    if (1 == OAL_ABSOLUTE_SUB(puc_pri_idx[uc_eq_idx], puc_pri_idx[uc_eq_idx +1])
        && uc_eq_idx +1 <= uc_basic_pri_idx)
    {
        return DMAC_RADAR_STAGGER_PERIOD_PRI_ERR;
    }

    return DMAC_RADAR_STAGGER_PERIOD_PRI_EQUAL;
}


OAL_STATIC void dmac_radar_pri_sort(oal_uint32 *pul_pri, oal_uint32 uc_cnt, oal_uint8 *puc_pri_idx)
{
    oal_uint8           uc_loop      = 0;
    oal_uint8           uc_index     = 0;
    oal_uint8           uc_min_index = 0;
    oal_uint32          ul_pri_tmp   = 0;
    oal_uint32          ul_pri_idx_tmp   = 0;

    for (uc_loop = 1; uc_loop < uc_cnt; uc_loop++)
    {
        uc_min_index = uc_loop - 1;
        for (uc_index = uc_loop; uc_index < uc_cnt; uc_index++)
        {
            if (pul_pri[uc_index] < pul_pri[uc_min_index])
            {
                uc_min_index = uc_index;
            }
        }

        ul_pri_tmp            = pul_pri[uc_loop-1];
        pul_pri[uc_loop-1]    = pul_pri[uc_min_index];
        pul_pri[uc_min_index] = ul_pri_tmp;

        ul_pri_idx_tmp            = puc_pri_idx[uc_loop-1];
        puc_pri_idx[uc_loop-1]    = puc_pri_idx[uc_min_index];
        puc_pri_idx[uc_min_index] = ul_pri_idx_tmp;
    }
}



OAL_STATIC oal_bool_enum_uint8 dmac_radar_check_pri_absub(oal_uint32 *pul_pri, oal_uint8 uc_cnt)
{
    oal_uint8           uc_loop      = 0;
    oal_uint32          ul_absub_val = 0;

    for (uc_loop = 0; uc_loop < uc_cnt -1; uc_loop++)
    {
        ul_absub_val = OAL_ABSOLUTE_SUB(pul_pri[uc_loop+1], pul_pri[uc_loop]);
        if (OAL_FALSE == CHECK_STAGGER_PRI_ABSUB(ul_absub_val))
        {
            return OAL_FALSE;
        }
    }
    return OAL_TRUE;
}


OAL_STATIC oal_bool_enum_uint8 dmac_radar_check_stagger_period(oal_uint32 *pul_pri, oal_uint8 uc_cnt)

{
    oal_uint32                              aul_pri[MAX_RADAR_NORMAL_PULSE_ANA_CNT] = {0};
    oal_uint8                               auc_pri_idx[MAX_RADAR_NORMAL_PULSE_ANA_CNT - 1] = {0, 1, 2, 3};
    oal_uint32                              ul_t1, ul_t2, ul_t3, ul_t4;
    dmac_radar_stagger_period_enum_uint8    en_stagger_period_ret;

    oal_memcopy((oal_uint32 *)aul_pri, (oal_uint32 *)pul_pri, uc_cnt * OAL_SIZEOF(oal_uint32));

    /* PRI���� */
    dmac_radar_pri_sort(aul_pri, uc_cnt, auc_pri_idx);

    /* ���ֵ����Сֵ֮�䱶������MAX_STAGGER_PULSE_TIMES������Ϊ���� */
    if (aul_pri[3] > MAX_STAGGER_PULSE_TIMES * aul_pri[0])
    {
        return OAL_FALSE;
    }

    /*staggerģʽ�£���������T2-T1, T3-T2,T4-T3����1)<=4;2)>50��������֮һ*/
    if(OAL_FALSE == dmac_radar_check_pri_absub(aul_pri, uc_cnt))
    {
        return OAL_FALSE;
    }

    /*���type6������֮��2500>T1>=833*/
    if(OAL_TRUE == CHECK_RADAR_ETSI_TYPE6_PRI(aul_pri[0]))
    {
        if (OAL_FALSE == dmac_radar_check_stagger_type6(pul_pri, aul_pri))
        {
            return OAL_FALSE;
        }
    }

    /* ������ͬ��pri����PRI���Ϸ�ΧҪ�������staggerģʽ������ */
    en_stagger_period_ret = dmac_radar_check_stagger_pri_equal(aul_pri, uc_cnt, auc_pri_idx);
    if (DMAC_RADAR_STAGGER_PERIOD_PRI_EQUAL == en_stagger_period_ret)
    {
        return OAL_TRUE;
    }

    /* ������ͬ��PRI����PRI�����Ϸ�ΧҪ������Ϊ���� */
    if (DMAC_RADAR_STAGGER_PERIOD_PRI_ERR == en_stagger_period_ret)
    {
        return OAL_FALSE;
    }

    ul_t1 = aul_pri[0];
    ul_t2 = aul_pri[1];
    ul_t3 = aul_pri[2];
    ul_t4 = aul_pri[3];

    /* T4 <2*T1����T2>2*T1, ��Ϊ���� */
    if (ul_t4 < TWO_TIMES(ul_t1) || ul_t2 > TWO_TIMES(ul_t1))
    {
        return OAL_FALSE;
    }

    /* T3 < 2*T1, T4>2*T1 */
    if (OAL_TRUE == dmac_radar_check_stagger_T3_T4(ul_t1, ul_t2, ul_t3, ul_t4))
    {
        return OAL_TRUE;
    }

    /* T2 < 2 * T1, 2*T1< T3 <= 3 * T1 */
    if (OAL_TRUE == dmac_radar_check_stagger_T2_T3(ul_t1, ul_t2, ul_t3, ul_t4))
    {
        return OAL_TRUE;
    }

    return OAL_FALSE;
}


OAL_STATIC oal_bool_enum_uint8 dmac_radar_stagger_filter(
                                    hal_dfs_radar_type_enum_uint8           en_radar_type,
                                    dmac_radar_pulse_analysis_result_stru  *pst_result)
{
    /* ��ETSI�״ֱ�ӹ��� */
    if (HAL_DFS_RADAR_TYPE_ETSI != en_radar_type)
    {
        return OAL_TRUE;
    }

    /* ��СPRI����staggerģʽ��������Χ�ڣ�ֱ�ӹ��� */
    if (OAL_FALSE == CHECK_RADAR_ETSI_STAGGER_PRI(pst_result->ul_min_pri))
    {
        return OAL_TRUE;
    }

    /* ����Ƿ���staggerģʽ */
    if (OAL_FALSE == dmac_radar_check_stagger_pulse_mode(pst_result->uc_stagger_cnt))
    {
        return OAL_TRUE;
    }

    /* ��������С����*/
    if (OAL_FALSE == dmac_radar_check_stagger_min_duration(pst_result->us_min_duration))
    {
        return OAL_TRUE;
    }

    /* ���ƽ������*/
    if (OAL_FALSE == dmac_radar_check_stagger_avrg_duration(pst_result->us_avrg_duration))
    {
        return OAL_TRUE;
    }

    /* ���type6 stagger�����*/
    if (OAL_FALSE == dmac_radar_check_stagger_type6_duration_diff(pst_result->us_duration_diff, pst_result->ul_min_pri))
    {
        return OAL_TRUE;
    }

    /* ���type6 stagger���һ��pri*/
    if (OAL_FALSE == dmac_radar_check_stagger_type6_extra_pri(pst_result->ul_extra_pri, pst_result->ul_min_pri))
    {
        return OAL_TRUE;
    }

    /* staggerģʽ�����Լ�� */
    if (OAL_TRUE == dmac_radar_check_stagger_period(pst_result->aul_pri, pst_result->uc_pri_cnt))
    {
        return OAL_FALSE;
    }

    return OAL_TRUE;
}


OAL_STATIC oal_uint8 dmac_radar_get_max_radar_pulse_ant_cnt(hal_dfs_radar_type_enum_uint8 en_radar_type,
                                                        oal_uint8 uc_radar_det_num,
                                                        hal_radar_pulse_info_stru *pst_pulse_info)
{
    oal_uint8                           uc_max_radar_pulse_ant_cnt;
    oal_uint8                           uc_loop = 0;
    hal_pulse_info_stru                *pst_info1            = OAL_PTR_NULL;
    hal_pulse_info_stru                *pst_info2            = OAL_PTR_NULL;
    oal_uint32                          ul_pri;
    oal_uint8                           uc_first_pulse_idx   = 0;
    oal_uint32                          ul_min_pri          = 0;
    oal_uint8                           uc_begin = 0;

    /*��������ѡ���������*/
    if ( HAL_DFS_RADAR_TYPE_FCC == en_radar_type && 0 == uc_radar_det_num)
    {
        return MAX_RADAR_NORMAL_PULSE_ANA_CNT;
    }

    uc_first_pulse_idx = uc_begin;

    /* �ж�buf�Ƿ��������burst*/
    for (uc_loop = pst_pulse_info->ul_pulse_cnt - 1; uc_loop >= uc_begin + 1; uc_loop--)
    {
        pst_info2 = &(pst_pulse_info->ast_pulse_info[uc_loop]);
        pst_info1 = &(pst_pulse_info->ast_pulse_info[uc_loop-1]);

        ul_pri = (pst_info2->ul_timestamp >= pst_info1->ul_timestamp) ? (pst_info2->ul_timestamp - pst_info1->ul_timestamp)
                                  : (pst_info2->ul_timestamp + 0xFFFFF - pst_info1->ul_timestamp);

        if ( uc_loop == pst_pulse_info->ul_pulse_cnt - 1 )
        {
            ul_min_pri = ul_pri;
        }

        if ( ul_pri < ul_min_pri)
        {
            ul_min_pri = ul_pri;
        }

        if ( ul_pri > ul_min_pri * MAX_PULSE_TIMES )
        {
            uc_first_pulse_idx = uc_loop;
            break;
        }
    }

    uc_max_radar_pulse_ant_cnt = pst_pulse_info->ul_pulse_cnt - uc_first_pulse_idx;

    /*check pulse num>=5*/
    uc_max_radar_pulse_ant_cnt = (uc_max_radar_pulse_ant_cnt < MAX_RADAR_NORMAL_PULSE_ANA_CNT)?
        MAX_RADAR_NORMAL_PULSE_ANA_CNT: uc_max_radar_pulse_ant_cnt;

    return uc_max_radar_pulse_ant_cnt;
}

 

OAL_STATIC oal_bool_enum_uint8 dmac_radar_normal_pulse_filter(hal_dfs_radar_filter_stru *pst_dfs_radar, hal_radar_pulse_info_stru *pst_pulse_info, oal_uint8 uc_radar_det_num)
{
    dmac_radar_pulse_analysis_result_stru      st_result;
    oal_uint32                                 ul_ret;
    oal_uint8                                  uc_max_pulse_num = 0;

    OAL_MEMZERO(&st_result, OAL_SIZEOF(dmac_radar_pulse_analysis_result_stru));

    /*��ȡӲ�������������*/
    uc_max_pulse_num = dmac_radar_get_max_radar_pulse_ant_cnt(pst_dfs_radar->en_radar_type, uc_radar_det_num, pst_pulse_info);

    if ( pst_dfs_radar->en_log_switch )
    {
        OAL_IO_PRINT("dmac_radar_normal_pulse_filter:max_pulse_num=%d\n", uc_max_pulse_num);
    }

    if ( HAL_DFS_RADAR_TYPE_FCC == pst_dfs_radar->en_radar_type )
    {
        dmac_radar_analysis_pulse_info(pst_pulse_info, uc_max_pulse_num, RADAR_NORMAL_PULSE_TYPE, &st_result);
    }
    else
    {
        dmac_radar_analysis_pulse_info(pst_pulse_info, MAX_RADAR_NORMAL_PULSE_ANA_CNT, RADAR_NORMAL_PULSE_TYPE, &st_result);
    }

    /*���ETSI type2/3, ��stagger��һ������8������Ĺ���*/
    if (HAL_DFS_RADAR_TYPE_ETSI == pst_dfs_radar->en_radar_type
        && (OAL_TRUE == CHECK_RADAR_ETSI_TYPE3_PRI(st_result.ul_min_pri)
         || ((OAL_TRUE == CHECK_RADAR_ETSI_TYPE2_PRI(st_result.ul_min_pri)
         && st_result.ul_pri_diff <= RADAR_PULSE_MARGIN_ETSI*4))))
    {
        OAL_MEMZERO(&st_result, OAL_SIZEOF(dmac_radar_pulse_analysis_result_stru));
        dmac_radar_analysis_pulse_info(pst_pulse_info, uc_max_pulse_num, RADAR_NORMAL_PULSE_TYPE, &st_result);
    }

    pst_dfs_radar->ul_min_pri = st_result.ul_min_pri;

    dmac_radar_pulse_log(pst_pulse_info, &st_result, pst_dfs_radar->en_log_switch, RADAR_NORMAL_PULSE_TYPE);

    /* ���������� */
    if (OAL_FALSE == dmac_radar_check_normal_pulse_num(st_result.uc_pri_cnt, pst_dfs_radar->en_radar_type))
    {
        return OAL_TRUE;
    }

    /* �������power */
    if (OAL_FALSE == dmac_radar_check_pulse_power(st_result.us_avrg_power, pst_dfs_radar->en_radar_type, st_result.ul_min_pri, st_result.us_min_duration))
    {
        return OAL_TRUE;
    }

    /* ��������С����*/
    if (OAL_FALSE == dmac_radar_check_normal_min_duration(st_result.us_min_duration, st_result.ul_min_pri,
                    pst_dfs_radar->en_radar_type))
    {
        return OAL_TRUE;
    }

    /* ���duration diff */
    if (OAL_FALSE == dmac_radar_check_normal_duration_diff(st_result.us_duration_diff, st_result.ul_min_pri,
                    pst_dfs_radar->en_radar_type)
        && OAL_TRUE == pst_dfs_radar->st_dfs_pulse_check_filter.en_fcc_type4_duration_diff)
    {
        return OAL_TRUE;
    }

    /* ���PRI */
    if (OAL_FALSE == dmac_radar_check_normal_pulse_pri(pst_dfs_radar->en_radar_type, st_result.ul_min_pri))
    {
        return OAL_TRUE;
    }

    /********************************************************************
       �����Լ��:
       �߱������ԣ��Ҽ��֮��ı�����ϵ������MAX_PULSE_TIMES���򲻹���
       �߱������ԣ��Ҽ��֮��ı�����ϵ����MAX_PULSE_TIMES����ֱ�ӹ���
       ���߱������ԣ������staggerģʽ����
    *********************************************************************/
    ul_ret = dmac_radar_check_pulse_period(pst_dfs_radar->en_radar_type, st_result.aul_pri, st_result.uc_pri_cnt, st_result.ul_min_pri);
    if (RADAR_PULSE_NORMAL_PERIOD == ul_ret)
    {
        return OAL_FALSE;
    }
    else if (RADAR_PULSE_BIG_PERIOD <= ul_ret)
    {
        return OAL_TRUE;
    }
    else
    {
        return dmac_radar_stagger_filter(pst_dfs_radar->en_radar_type, &st_result);
    }
}


oal_uint32 dmac_radar_filter_timeout(oal_void *p_arg)
{
    frw_event_mem_stru          *pst_event_mem;
    hal_dfs_radar_filter_stru   *pst_dfs_radar;
    dmac_device_stru            *pst_dmac_device;
    hal_to_dmac_device_stru     *pst_hal_device;
    frw_event_stru              *pst_event;
    mac_device_stru             *pst_mac_device;

    pst_event_mem = (frw_event_mem_stru*)p_arg;

    pst_event = frw_get_event_stru(pst_event_mem);

    pst_mac_device = mac_res_get_dev(pst_event->st_event_hdr.uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_WARNING_LOG0(0, OAM_SF_DFS, "{dmac_radar_filter_timeout::pst_mac_device is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_dmac_device = dmac_res_get_mac_dev(pst_mac_device->uc_device_id);
    pst_hal_device  = pst_dmac_device->past_hal_device[0];
    pst_dfs_radar   = &(pst_hal_device->st_dfs_radar_filter);

    pst_dfs_radar->en_chirp_timeout = OAL_TRUE;

    dmac_dfs_radar_detect_event(pst_event_mem);

    return OAL_TRUE;
}


OAL_STATIC oal_bool_enum_uint8 dmac_radar_filter_timer(mac_device_stru *pst_mac_device,
                                            hal_radar_det_event_stru *pst_radar_det_info,
                                            hal_dfs_radar_filter_stru *pst_dfs_radar)
{
    frw_event_mem_stru         *pst_event_mem;
    frw_event_stru             *pst_event;

    if (HAL_DFS_RADAR_TYPE_FCC != pst_dfs_radar->en_radar_type
          && HAL_DFS_RADAR_TYPE_MKK != pst_dfs_radar->en_radar_type)
    {
        return OAL_TRUE;
    }

    /*��һ��FCC chirp�ж�ʱʹ�ܶ�ʱ��*/
    if ( 1 != pst_dfs_radar->uc_chirp_cnt_total )
    {
        return OAL_TRUE;
    }
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    pst_event_mem = FRW_EVENT_ALLOC_LARGE(OAL_SIZEOF(hal_radar_det_event_stru));
#else
    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(hal_radar_det_event_stru));
#endif /* #if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) */
    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_WARNING_LOG0(0, OAM_SF_DFS, "{dmac_radar_filter_timer::alloc pst_event_mem failed.}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event = frw_get_event_stru(pst_event_mem);

    /* ��д�¼�ͷ */
    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_DMAC_MISC,
                       HAL_EVENT_DMAC_MISC_RADAR_DETECTED,
                       OAL_SIZEOF(hal_radar_det_event_stru),
                       FRW_EVENT_PIPELINE_STAGE_0,
                       pst_mac_device->uc_chip_id,
                       pst_mac_device->uc_device_id,
                       0);

    oal_memcopy(pst_event->auc_event_data, pst_radar_det_info, OAL_SIZEOF(hal_radar_det_event_stru));

    /* ����12s��ʱ��ʱ��*/
    pst_dfs_radar->st_timer.ul_timeout = pst_dfs_radar->ul_chirp_time_threshold;

    FRW_TIMER_CREATE_TIMER(&(pst_dfs_radar->st_timer),
                          dmac_radar_filter_timeout,
                          pst_dfs_radar->st_timer.ul_timeout,
                          (oal_void *)pst_event_mem,
                          OAL_FALSE,
                          OAM_MODULE_ID_DMAC,
                          pst_dfs_radar->st_timer.ul_core_id);

    /* �ͷ��¼��ڴ� */
    FRW_EVENT_FREE(pst_event_mem);

    return OAL_TRUE;

}

#ifdef RADAR_FILTER_NORMAL_PULSE_TIMER
oal_uint32 dmac_radar_normal_pulse_det_timeout(oal_void *p_arg)
{
    frw_event_mem_stru          *pst_event_mem;
    hal_dfs_radar_filter_stru   *pst_dfs_radar;
    dmac_device_stru            *pst_dmac_device;
    hal_to_dmac_device_stru     *pst_hal_device;
    frw_event_stru              *pst_event;
    mac_device_stru             *pst_mac_device;

    pst_event_mem = (frw_event_mem_stru*)p_arg;

    pst_event = frw_get_event_stru(pst_event_mem);

    pst_mac_device = mac_res_get_dev(pst_event->st_event_hdr.uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_WARNING_LOG0(0, OAM_SF_DFS, "{dmac_radar_normal_pulse_det_timeout::pst_mac_device is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_dmac_device = dmac_res_get_mac_dev(pst_mac_device->uc_device_id);
    pst_hal_device  = pst_dmac_device->past_hal_device[0];
    pst_dfs_radar   = &(pst_hal_device->st_dfs_radar_filter);

    pst_dfs_radar->st_dfs_normal_pulse_det.en_timeout = OAL_FALSE;

    if ( 0 == pst_dfs_radar->st_dfs_normal_pulse_det.uc_irq_cnt)
    {
        pst_dfs_radar->st_dfs_normal_pulse_det.ul_period_cnt++;
        return OAL_TRUE;
    }
    else
    {
        pst_dfs_radar->st_dfs_normal_pulse_det.ul_period_cnt = 0;
    }

    /*��ʱ�����ڼ�⵽�״�򴥷��״����¼�*/
    if (pst_dfs_radar->st_dfs_normal_pulse_det.uc_radar_cnt)
    {
        pst_dfs_radar->st_dfs_normal_pulse_det.en_timeout = OAL_TRUE;
        dmac_dfs_radar_detect_event(pst_event_mem);
    }
    else
    {
        pst_dfs_radar->st_dfs_normal_pulse_det.uc_radar_cnt = 0;
        pst_dfs_radar->st_dfs_normal_pulse_det.uc_irq_cnt_old = pst_dfs_radar->st_dfs_normal_pulse_det.uc_irq_cnt;
        pst_dfs_radar->st_dfs_normal_pulse_det.uc_irq_cnt = 0;
    }

    return OAL_TRUE;
}

oal_bool_enum_uint8 dmac_radar_normal_pulse_det_timer(mac_device_stru *pst_mac_device,
                                            hal_radar_det_event_stru *pst_radar_det_info,
                                            hal_dfs_radar_filter_stru *pst_dfs_radar)
{
    frw_event_mem_stru         *pst_event_mem;
    frw_event_stru             *pst_event;

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    pst_event_mem = FRW_EVENT_ALLOC_LARGE(OAL_SIZEOF(hal_radar_det_event_stru));
#else
    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(hal_radar_det_event_stru));
#endif /* #if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) */
    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_WARNING_LOG0(0, OAM_SF_DFS, "{dmac_radar_normal_pulse_det_timer::alloc pst_event_mem failed.}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event = frw_get_event_stru(pst_event_mem);

    /* ��д�¼�ͷ */
    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_DMAC_MISC,
                       HAL_EVENT_DMAC_MISC_RADAR_DETECTED,
                       OAL_SIZEOF(hal_radar_det_event_stru),
                       FRW_EVENT_PIPELINE_STAGE_0,
                       pst_mac_device->uc_chip_id,
                       pst_mac_device->uc_device_id,
                       0);

    oal_memcopy(pst_event->auc_event_data, pst_radar_det_info, OAL_SIZEOF(hal_radar_det_event_stru));

    /* ����200m�����Լ�ⶨʱ��*/
    if(pst_dfs_radar->en_log_switch)
    {
        pst_dfs_radar->st_dfs_normal_pulse_det.st_timer.ul_timeout = 200;
    }
    else
    {
        pst_dfs_radar->st_dfs_normal_pulse_det.st_timer.ul_timeout = 50;
    }

    FRW_TIMER_CREATE_TIMER(&(pst_dfs_radar->st_dfs_normal_pulse_det.st_timer),
                          dmac_radar_normal_pulse_det_timeout,
                          pst_dfs_radar->st_dfs_normal_pulse_det.st_timer.ul_timeout,
                          (oal_void *)pst_event_mem,
                          OAL_TRUE,
                          OAM_MODULE_ID_DMAC,
                          pst_dfs_radar->st_dfs_normal_pulse_det.st_timer.ul_core_id);

    /* �ͷ��¼��ڴ� */
    FRW_EVENT_FREE(pst_event_mem);

    return OAL_TRUE;

}

#endif

#ifdef RADAR_FILTER_FCC_CRAZY_REPORT_DET

oal_uint32 dmac_radar_disable_chirp_det_timeout(oal_void *p_arg)
{
    hal_to_dmac_device_stru   *pst_hal_device;

    pst_hal_device = (hal_to_dmac_device_stru*)p_arg;

    /*�ر�chirp 1min��ʹ��chirp���*/
    hal_radar_enable_chirp_det(pst_hal_device, 1);

    dmac_radar_crazy_report_det_timer(pst_hal_device);

    return OAL_TRUE;
}


oal_uint32 dmac_radar_crazy_report_det_timeout(oal_void *p_arg)
{
    hal_dfs_radar_filter_stru *pst_dfs_radar;
    hal_to_dmac_device_stru   *pst_hal_device;
    oal_uint8                 uc_max_chirp_cnt_in_crazy_report_det;

    pst_hal_device = (hal_to_dmac_device_stru*)p_arg;
    pst_dfs_radar   = &(pst_hal_device->st_dfs_radar_filter);

    uc_max_chirp_cnt_in_crazy_report_det = (HAL_DFS_RADAR_TYPE_FCC == pst_dfs_radar->en_radar_type)?
            MAX_IRQ_CNT_IN_CHIRP_CRAZY_REPORT_DET_FCC: MAX_IRQ_CNT_IN_CHIRP_CRAZY_REPORT_DET_ETSI;

    /*���20s�ڼ�⵽�жϸ�������40������ر�chirp���*/
    if ( pst_dfs_radar->uc_chirp_cnt_for_crazy_report_det >= uc_max_chirp_cnt_in_crazy_report_det )
    {
         /*�ر�chirp���*/
        hal_radar_enable_chirp_det(pst_hal_device, OAL_FALSE);

        if(pst_dfs_radar->en_log_switch)
        {
            OAL_IO_PRINT("dmac_radar_crazy_report:crazy_cnt[%d] in 20s:turn off chirp det and wait %ds.\n",
                    pst_dfs_radar->uc_chirp_cnt_for_crazy_report_det,
                    pst_dfs_radar->st_timer_disable_chirp_det.ul_timeout/1000);
        }
        OAM_WARNING_LOG2(0, OAM_SF_DFS, "{dmac_radar_crazy_report:crazy_cnt[%d] in 20s>=%d:turn off chirp det and wait %ds.}\r\n",
                    pst_dfs_radar->uc_chirp_cnt_for_crazy_report_det,
                    pst_dfs_radar->st_timer_disable_chirp_det.ul_timeout/1000);
    }
    else
    {
        /*ʹ��chirp���*/
        hal_radar_enable_chirp_det(pst_hal_device, OAL_TRUE);
    }

    pst_dfs_radar->uc_chirp_cnt_for_crazy_report_det = 0;
    pst_dfs_radar->en_crazy_report_cnt               = OAL_FALSE;

    FRW_TIMER_CREATE_TIMER(&(pst_dfs_radar->st_timer_disable_chirp_det),
                          dmac_radar_disable_chirp_det_timeout,
                          pst_dfs_radar->st_timer_disable_chirp_det.ul_timeout,
                          (oal_void *)pst_hal_device,
                          OAL_FALSE,
                          OAM_MODULE_ID_DMAC,
                          pst_hal_device->ul_core_id);

    return OAL_TRUE;
}


oal_bool_enum_uint8 dmac_radar_crazy_report_det_timer(hal_to_dmac_device_stru *pst_hal_device)
{
    hal_dfs_radar_filter_stru       *pst_dfs_radar;

    pst_dfs_radar   = &(pst_hal_device->st_dfs_radar_filter);

    /*ʹ��20s���жϸ���ͳ��*/
    pst_dfs_radar->en_crazy_report_cnt = OAL_TRUE;

    /* ����20s��ʱ��ʱ��*/
    FRW_TIMER_CREATE_TIMER(&(pst_dfs_radar->st_timer_crazy_report_det),
                          dmac_radar_crazy_report_det_timeout,
                          pst_dfs_radar->st_timer_crazy_report_det.ul_timeout,
                          (oal_void *)pst_hal_device,
                          OAL_FALSE,
                          OAM_MODULE_ID_DMAC,
                          pst_hal_device->ul_core_id);

    return OAL_TRUE;
}

#endif

oal_bool_enum_uint8 dmac_radar_filter(mac_device_stru *pst_mac_device, hal_radar_det_event_stru *pst_radar_det_info)
{
    oal_uint32                       ul_timestamp   = 0;
    oal_uint32                       ul_delta_time  = 0;
    dmac_device_stru                *pst_dmac_device;
    hal_to_dmac_device_stru         *pst_hal_device;
    hal_dfs_radar_filter_stru       *pst_dfs_radar;
    oal_uint8                        uc_ret = OAL_TRUE;

    pst_dmac_device = dmac_res_get_mac_dev(pst_mac_device->uc_device_id);
    pst_hal_device  = pst_dmac_device->past_hal_device[0];
    pst_dfs_radar   = &(pst_hal_device->st_dfs_radar_filter);
    pst_dfs_radar->st_timer.ul_core_id = pst_mac_device->ul_core_id;
    pst_dfs_radar->st_dfs_normal_pulse_det.st_timer.ul_core_id = pst_mac_device->ul_core_id;

#ifndef WIN32
    dmac_radar_get_sys_time(pst_dfs_radar->en_log_switch);
#endif

#ifdef RADAR_FILTER_FCC_CRAZY_REPORT_DET
    /*FCC crazy report ��ʱ��ʹ��*/
    if ( RADAR_TYPE_CHIRP == pst_radar_det_info->uc_radar_type
        && OAL_TRUE == pst_dfs_radar->en_crazy_report_is_enabled
        && 0 == pst_dfs_radar->ul_last_burst_timestamp_for_chirp)
    {
        dmac_radar_crazy_report_det_timer(pst_hal_device);
    }
#endif

    /*FCC chirp�״��ⳬʱ*/
    if (OAL_TRUE == pst_dfs_radar->en_chirp_timeout)
    {

        OAM_WARNING_LOG2(0, OAM_SF_DFS, "{dmac_radar_pulse_log::[DFS]chirp_cnt_total: %d, chirp_cnt: %d!",
               pst_dfs_radar->uc_chirp_cnt_total, pst_dfs_radar->uc_chirp_cnt);

        if(pst_dfs_radar->en_log_switch)
        {
            OAL_IO_PRINT("dmac_radar_filter_timeout::chirp_cnt_total =%d, chirp_cnt=%d\n",
                pst_dfs_radar->uc_chirp_cnt_total, pst_dfs_radar->uc_chirp_cnt);
        }
        else
        {
            OAL_IO_PRINT("dmac_radar_filter::12s done\n");
        }

        pst_dfs_radar->en_chirp_timeout = OAL_FALSE;

        if (pst_dfs_radar->uc_chirp_cnt >= pst_dfs_radar->ul_chirp_cnt_threshold
            && OAL_TRUE == CHECK_RADAR_FCC_CHIRP_TOTAL_CNT(pst_dfs_radar->uc_chirp_cnt_total))
        {
            pst_hal_device->uc_radar_type = RADAR_TYPE_CHIRP;
            return OAL_FALSE;
        }
        else
        {
            return OAL_TRUE;
        }
    }

#ifdef RADAR_FILTER_NORMAL_PULSE_TIMER
    /*normal pulse*/
    if (OAL_TRUE == pst_dfs_radar->st_dfs_normal_pulse_det.en_timeout)
    {
        pst_dfs_radar->st_dfs_normal_pulse_det.en_timeout = OAL_FALSE;

        if(OAL_FALSE == dmac_radar_check_normal_pulse_timer_irq_num(pst_dfs_radar->en_radar_type,
            pst_hal_device->uc_radar_type, pst_dfs_radar->st_dfs_normal_pulse_det.uc_irq_cnt,
            pst_dfs_radar->st_dfs_normal_pulse_det.uc_irq_cnt_old))
        {
            uc_ret = OAL_FALSE;
        }

        if(pst_dfs_radar->en_log_switch)
        {
            OAL_IO_PRINT("dmac_radar_filter:radar_type=%d, normal_pulse_irq[new:%d][old:%d].\n", pst_hal_device->uc_radar_type,
                pst_dfs_radar->st_dfs_normal_pulse_det.uc_irq_cnt,
                pst_dfs_radar->st_dfs_normal_pulse_det.uc_irq_cnt_old);
        }
        OAM_WARNING_LOG3(0, OAM_SF_DFS, "{dmac_radar_filter:radar_type=%d, normal_pulse_irq[new:%d][old:%d].",
                pst_hal_device->uc_radar_type,
                pst_dfs_radar->st_dfs_normal_pulse_det.uc_irq_cnt,
                pst_dfs_radar->st_dfs_normal_pulse_det.uc_irq_cnt_old);

        pst_dfs_radar->st_dfs_normal_pulse_det.uc_radar_cnt = 0;
        pst_dfs_radar->st_dfs_normal_pulse_det.uc_irq_cnt_old = pst_dfs_radar->st_dfs_normal_pulse_det.uc_irq_cnt;
        pst_dfs_radar->st_dfs_normal_pulse_det.uc_irq_cnt = 0;

        return uc_ret;
    }
#endif

    /*FCC�ڲ�����һ���жϺ�ʹ�������Լ�ⶨʱ��*/
#ifdef RADAR_FILTER_NORMAL_PULSE_TIMER
    if(OAL_TRUE == CHECK_RADAR_ETSI_TYPE23_OR_FCC_TYPE34_HW(pst_dfs_radar->en_radar_type, pst_radar_det_info->uc_radar_type)
        && OAL_TRUE == pst_dfs_radar->st_dfs_normal_pulse_det.en_is_enabled)
    {
        if(OAL_FALSE == pst_dfs_radar->st_dfs_normal_pulse_det.en_timer_start)
        {
            dmac_radar_normal_pulse_det_timer(pst_mac_device, pst_radar_det_info, pst_dfs_radar);
            pst_dfs_radar->st_dfs_normal_pulse_det.en_timer_start = OAL_TRUE;
        }
        pst_dfs_radar->st_dfs_normal_pulse_det.uc_irq_cnt++;
    }
#endif

    if (RADAR_TYPE_CHIRP == pst_radar_det_info->uc_radar_type)
    {
        uc_ret  = dmac_radar_chirp_filter(pst_dfs_radar, &pst_radar_det_info->st_radar_pulse_info);

        dmac_radar_filter_timer(pst_mac_device, pst_radar_det_info, pst_dfs_radar);
        pst_hal_device->uc_radar_type = RADAR_TYPE_CHIRP;

        return uc_ret;
    }
    else
    {
        ul_timestamp  = (oal_uint32)OAL_TIME_GET_STAMP_MS();
        ul_delta_time = (oal_uint32)OAL_TIME_GET_RUNTIME(pst_dfs_radar->ul_last_burst_timestamp, ul_timestamp);

        if (0 != pst_dfs_radar->ul_last_burst_timestamp &&
            ul_delta_time < pst_dfs_radar->ul_time_threshold)
        {
            /* ��������״��жϼ����С������Ϊ��һ���µ�Burst��ֱ�ӷ��� */
            return OAL_TRUE;
        }

        if (OAL_TRUE == dmac_radar_normal_pulse_filter(pst_dfs_radar, &pst_radar_det_info->st_radar_pulse_info, pst_radar_det_info->uc_radar_type))
        {
            return OAL_TRUE;
        }

        /* �µ�Burst����������ʱ��� */
        pst_dfs_radar->ul_last_burst_timestamp = ul_timestamp;

#ifdef RADAR_FILTER_NORMAL_PULSE_TIMER
        if(OAL_TRUE == CHECK_RADAR_ETSI_TYPE23_OR_FCC_TYPE34_HW(pst_dfs_radar->en_radar_type, pst_radar_det_info->uc_radar_type)
            && OAL_TRUE == pst_dfs_radar->st_dfs_normal_pulse_det.en_is_enabled)
        {
            pst_hal_device->uc_radar_type = pst_radar_det_info->uc_radar_type;

            /*FCC type6���������Լ��*/
            if(HAL_DFS_RADAR_TYPE_FCC == pst_dfs_radar->en_radar_type
                && OAL_TRUE == CHECK_RADAR_FCC_TYPE6_PRI(pst_dfs_radar->ul_min_pri))
            {
                return OAL_FALSE;
            }
            pst_dfs_radar->st_dfs_normal_pulse_det.uc_radar_cnt++;
            return OAL_TRUE;
        }
#endif
        pst_hal_device->uc_radar_type = pst_radar_det_info->uc_radar_type;
        return OAL_FALSE;
    }
}

#endif /* _PRE_WLAN_FEATURE_DFS_OPTIMIZE */


#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

