


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_STA_PM

/*****************************************************************************
  1 ͷ�ļ�����
*****************************************************************************/
#include "oal_types.h"
#include "hal_chip.h"
#include "hal_device.h"
#include "hal_device_fsm.h"
#include "pm_extern.h"
#ifdef _PRE_PM_DYN_SET_TBTT_OFFSET
#include "hal_pm.h"
#endif
#include "frw_timer.h"
#include "mac_data.h"
#include "dmac_config.h"
#include "dmac_psm_sta.h"
#include "dmac_psm_ap.h"
#include "dmac_sta_pm.h"
#ifdef _PRE_WLAN_FEATURE_P2P
#include "dmac_p2p.h"
#endif

#include "dmac_csa_sta.h"
#ifdef _PRE_WLAN_FEATURE_NO_FRM_INT
#include "dmac_beacon.h"
#endif
#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_PSM_STA_C

oal_uint32 g_device_wlan_pm_timeout =  DMAC_PSM_TIMER_IDLE_TIMEOUT;
oal_uint32 g_pm_timer_restart_cnt   =  DMAC_PSM_TIMER_IDLE_CNT;
oal_uint32 g_ps_fast_check_cnt      =  DMAC_PSM_TIMER_FAST_CNT;

#ifdef _PRE_WLAN_DOWNLOAD_PM
extern oal_uint8 g_uc_max_powersave_limit;
oal_uint16 g_us_download_rate_limit_pps = 0;
oal_uint32 g_ul_drop_time;
#endif
/*****************************************************************************
  2 ȫ�ֱ�������
*****************************************************************************/


//OAL_STATIC oal_uint8  g_uc_bmap[8] = {1, 2, 4, 8, 16, 32, 64, 128}; /* Bit map */
/*****************************************************************************
  3 ����ʵ��
*****************************************************************************/

oal_uint32  dmac_psm_process_fast_ps_state_change(dmac_vap_stru *pst_dmac_vap, oal_uint8 uc_psm)
{
    mac_sta_pm_handler_stru *pst_mac_sta_pm_handle;
    oal_uint32 ul_retval  = OAL_SUCC;

     pst_mac_sta_pm_handle = &pst_dmac_vap->st_sta_pm_handler;

     /* awake->active ��Ҫ���ͻ���null֡ */
     if (STA_PWR_SAVE_STATE_AWAKE == GET_PM_STATE(pst_mac_sta_pm_handle))
     {
         if((STA_PWR_SAVE_STATE_ACTIVE == uc_psm) && (OAL_FALSE == pst_mac_sta_pm_handle->st_null_wait.en_active_null_wait))
         {

             ul_retval = dmac_send_null_frame_to_ap(pst_dmac_vap, STA_PWR_SAVE_STATE_ACTIVE, OAL_FALSE);

         }
         else
         {
             OAM_WARNING_LOG2(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR, "{dmac_psm_process_fast_ps_state_change::state:awake,send[%d]null frame,active null wait:[%d].}",uc_psm,pst_mac_sta_pm_handle->st_null_wait.en_active_null_wait);
         }
     }

     /* active->doze�ĳ�ʱ��Ҫ����˯��null֡ */
     else if (STA_PWR_SAVE_STATE_ACTIVE == GET_PM_STATE(pst_mac_sta_pm_handle))
     {
         if((STA_PWR_SAVE_STATE_DOZE == uc_psm) && (OAL_FALSE == pst_mac_sta_pm_handle->st_null_wait.en_doze_null_wait))
         {

             ul_retval = dmac_send_null_frame_to_ap(pst_dmac_vap, STA_PWR_SAVE_STATE_DOZE, OAL_FALSE);
         }
         else
         {
             OAM_WARNING_LOG2(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR, "{dmac_psm_process_fast_ps_state_change::state:active,send[%d]null frame,doze null wait:[%d].}",uc_psm,pst_mac_sta_pm_handle->st_null_wait.en_doze_null_wait);
         }
     }

     return ul_retval;

}


oal_void dmac_psm_start_activity_timer(dmac_vap_stru *pst_dmac_vap, mac_sta_pm_handler_stru *pst_sta_pm_handle)
{

    if((OAL_PTR_NULL == pst_dmac_vap) || (OAL_PTR_NULL == pst_sta_pm_handle))
    {
        OAM_ERROR_LOG2(0, OAM_SF_PWR, "{dmac_psm_start_activity_timer has null point:dmac_vap =%p pm_handle=%p}",pst_dmac_vap,pst_sta_pm_handle);
        return;
    }

    if(WLAN_MIB_PWR_MGMT_MODE_PWRSAVE == mac_mib_get_powermanagementmode(&(pst_dmac_vap->st_vap_base_info)))
    {
        pst_sta_pm_handle->ul_activity_timeout = g_device_wlan_pm_timeout;

#ifdef _PRE_WLAN_FEATURE_P2P
        /* �޸Ķ�ʱ��ʱ��ʹ����˯��null֡��ʱ������go activeʱ���� */
        if (IS_P2P_NOA_ENABLED(pst_dmac_vap))
        {
            pst_sta_pm_handle->ul_activity_timeout = (pst_dmac_vap->st_p2p_noa_param.ul_duration / 1000);
        }
        else if (IS_P2P_OPPPS_ENABLED(pst_dmac_vap))
        {
            pst_sta_pm_handle->ul_activity_timeout = (pst_dmac_vap->st_p2p_ops_param.uc_ct_window);
        }
#endif
    }
    else
    {
        pst_sta_pm_handle->ul_activity_timeout = MIN_ACTIVITY_TIME_OUT;
    }

    /* �����³�ʱʱ��ͷǽ����µĳ�ʱʱ�䲻һ����������ʱ�� */
    FRW_TIMER_CREATE_TIMER(&(pst_sta_pm_handle->st_inactive_timer),
                        dmac_psm_alarm_callback,
                        pst_sta_pm_handle->ul_activity_timeout ,
                        pst_dmac_vap,
                        OAL_FALSE,
                        OAM_MODULE_ID_DMAC,
                        pst_dmac_vap->st_vap_base_info.ul_core_id);
}

#ifdef _PRE_WLAN_FEATURE_HISTREAM

oal_bool_enum_uint8 dmac_psm_auto_dtim(dmac_vap_stru *pst_dmac_vap)
{
    mac_user_stru     *pst_mac_user;
    oal_uint32         ul_beacon_period;
    oal_uint8          uc_auto_dtim;

    pst_mac_user = (mac_user_stru *)mac_res_get_dmac_user(pst_dmac_vap->st_vap_base_info.us_assoc_vap_id);
    if (OAL_PTR_NULL == pst_mac_user)
    {
        OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR,
            "{dmac_psm_auto_dtim::pst_dmac_user[%d] null.}", pst_dmac_vap->st_vap_base_info.us_assoc_vap_id);
        return OAL_FALSE;
    }

    /* ���ap��֧��proxy_arp���򲻿�����̬dtim�������ԣ���ֹarp���� */
    if (OAL_FALSE == pst_mac_user->st_cap_info.bit_proxy_arp)
    {
        return OAL_FALSE;
    }
#if 0
    /* ���ap��֧��HISTREAM���򲻿�����̬dtim�������ԣ���ֹ���������� */
    if (OAL_FALSE == pst_mac_user->st_cap_info.bit_histream_cap)
    {
        return OAL_FALSE;
    }
#endif
    uc_auto_dtim      = (oal_uint8)mac_mib_get_dot11dtimperiod(&pst_dmac_vap->st_vap_base_info);
    ul_beacon_period  = mac_mib_get_BeaconPeriod(&pst_dmac_vap->st_vap_base_info);

    if (OAL_TRUE == PM_WLAN_IsHostSleep())
    {
        /* �Զ���������ʹ������dtim��ʱ��������500ms */
        if (pst_dmac_vap->uc_psm_dtim_period * ul_beacon_period >= 500)
        {
            return OAL_FALSE;
        }
        /* host���������Ժ�ÿ8�����ڶ�̬����1 */
        pst_dmac_vap->uc_psm_auto_dtim_cnt++;
        uc_auto_dtim += (pst_dmac_vap->uc_psm_auto_dtim_cnt/8);
    }
    else
    {
        pst_dmac_vap->uc_psm_auto_dtim_cnt = 0;
    }

    if (pst_dmac_vap->uc_psm_dtim_period == uc_auto_dtim)
    {
        return OAL_FALSE;
    }

    pst_dmac_vap->uc_psm_dtim_period = uc_auto_dtim;
    if (pst_dmac_vap->uc_psm_dtim_period != (oal_uint8)mac_mib_get_dot11dtimperiod(&pst_dmac_vap->st_vap_base_info))
    {
        pst_dmac_vap->us_psm_listen_interval = (oal_uint16)pst_dmac_vap->uc_psm_dtim_period;
    }
    else
    {
        pst_dmac_vap->us_psm_listen_interval = (pst_dmac_vap->uc_psm_dtim_period  < DMAC_DEFAULT_DTIM_LISTEN_DIFF) ? ((oal_uint16)(pst_dmac_vap->uc_psm_dtim_period)) : DMAC_DEFAULT_LISTEN_INTERVAL;
    }

    /* ����period�Ĵ�����ֵ */
    hal_set_sta_dtim_period(pst_dmac_vap->pst_hal_vap, pst_dmac_vap->uc_psm_dtim_period);
    hal_set_psm_listen_interval(pst_dmac_vap->pst_hal_vap, pst_dmac_vap->us_psm_listen_interval);

    return OAL_TRUE;
}
#endif //_PRE_WLAN_FEATURE_HISTREAM
#if 0

oal_uint8 dmac_psm_is_tim_dtim_set(dmac_vap_stru *pst_dmac_vap, oal_uint8* puc_tim_elm)
{
    oal_uint8                   uc_len  = 0;
    oal_uint8                   uc_bmap_ctrl = 0;
    oal_uint8                   uc_bmap_offset  = 0;
    oal_uint8                   uc_byte_offset = 0;
    oal_uint8                   uc_bit_offset  = 0;
    oal_uint8                   uc_status      = 0 ;
    oal_uint32                  ul_aid;
    mac_sta_pm_handler_stru     *pst_mac_sta_pm_handle;

    ul_aid = pst_dmac_vap->st_vap_base_info.us_sta_aid;
    pst_mac_sta_pm_handle = (mac_sta_pm_handler_stru *)(pst_dmac_vap->pst_pm_handler);

    /*����Ƿ���TIM*/
    if(puc_tim_elm[0]!= MAC_EID_TIM)
    {
      return uc_status;
    }

    uc_len = puc_tim_elm[1];
    uc_bmap_ctrl = puc_tim_elm[4];

    /* Check if AP's DTIM period has changed */
    if(puc_tim_elm[3] != mac_mib_get_dot11dtimperiod(&(pst_dmac_vap->st_vap_base_info)))
    {
      /* ��������DTIM�Ĵ�����ֵ */
      hal_set_psm_dtim_period(pst_dmac_vap->pst_hal_vap, puc_tim_elm[3], pst_mac_sta_pm_handle->uc_listen_intervl_to_dtim_times,
                                  pst_mac_sta_pm_handle->en_receive_dtim);
    }

    /* Compute the bit map offset, which is given by the MSB 7 bits in the   */
    /* bit map control field of the TIM element.  */
    uc_bmap_offset = (uc_bmap_ctrl & 0xFE);

    /* A DTIM count of zero indicates that the current TIM is a DTIM. The    */
    /* BIT0 of the bit map control field is set (for TIMs with a value of 0  */
    /* in the DTIM count field) when one or more broadcast/multicast frames  */
    /* are buffered at the AP.                                               */
    if(OAL_TRUE == (uc_bmap_ctrl & BIT0))
    {
        uc_status |= DMAC_DTIM_IS_SET;
    }

    /* Traffic Indication Virtual Bit Map within the AP, generates the TIM   */
    /* such that if a station has buffered packets, then the corresponding   */
    /* bit (which can be found from the association ID) is set. The byte     */
    /* offset is obtained by dividing the association ID by '8' and the bit  */
    /* offset is the remainder of the association ID when divided by '8'.    */
    uc_byte_offset = ul_aid >> 3;
    uc_bit_offset  = ul_aid & 0x07;

    /* Bit map offset should always be greater than the computed byte offset */
    /* and the byte offset should always be lesser than the 'maximum' number */
    /* of bytes in the Virtual Bitmap. If either of the above two conditions */
    /* are not satisfied, then the 'status' is returned as is.               */
    if(uc_byte_offset < uc_bmap_offset || uc_byte_offset > uc_bmap_offset + uc_len - 4)
    {
        return uc_status;
    }

    /* The station has buffered packets only if the corresponding bit is set */
    /* in the Virtual Bit Map field. Note: Virtual Bit Map field starts      */
    /* 5 bytes from the start of the TIM element.                            */
    if((puc_tim_elm[5 + uc_byte_offset - uc_bmap_offset] & g_uc_bmap[uc_bit_offset]) != 0)
    {
        uc_status |= DMAC_TIM_IS_SET;
    }
    return uc_status;
}
#endif

oal_void dmac_psm_update_dtime_period(mac_vap_stru *pst_mac_vap, oal_uint8 uc_mib_dtim_period,oal_uint32 ul_beacon_period)
{
    dmac_vap_stru   *pst_dmac_vap;
    oal_uint8        uc_dtim_cnt;

    pst_dmac_vap = MAC_GET_DMAC_VAP(pst_mac_vap);

    if ((WLAN_VAP_MODE_BSS_STA != pst_mac_vap->en_vap_mode) || (0 == ul_beacon_period) || (0 == uc_mib_dtim_period))
    {
        return;
    }

    if(g_uc_max_powersave)
    {
        if(uc_mib_dtim_period*ul_beacon_period<=200)
        {
            if(1==uc_mib_dtim_period)
            {
                pst_dmac_vap->uc_psm_dtim_period = (oal_uint8)(uc_mib_dtim_period*4);
                pst_dmac_vap->us_psm_listen_interval = (oal_uint16)pst_dmac_vap->uc_psm_dtim_period;
                OAM_WARNING_LOG2(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR, "{dmac_psm_update_dtime_period::use dtim*4,beacon[%d],dtim period[%d]}",ul_beacon_period,uc_mib_dtim_period);
            }
            else if(2==uc_mib_dtim_period)
            {
                pst_dmac_vap->uc_psm_dtim_period = (oal_uint8)(uc_mib_dtim_period*2);
                pst_dmac_vap->us_psm_listen_interval = (oal_uint16)pst_dmac_vap->uc_psm_dtim_period;
                OAM_WARNING_LOG2(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR, "{dmac_psm_update_dtime_period::use dtim*2,beacon[%d],dtim period[%d]}",ul_beacon_period,uc_mib_dtim_period);
            }
        }
        else
        {
            pst_dmac_vap->uc_psm_dtim_period = uc_mib_dtim_period;
            pst_dmac_vap->us_psm_listen_interval = (pst_dmac_vap->uc_psm_dtim_period  < DMAC_DEFAULT_DTIM_LISTEN_DIFF) ? ((oal_uint16)(pst_dmac_vap->uc_psm_dtim_period )) : DMAC_DEFAULT_LISTEN_INTERVAL;
        }
        #ifdef _PRE_PM_TBTT_OFFSET_PROBE
        hal_tbtt_offset_probe_resume(pst_dmac_vap->pst_hal_vap);
        #endif
    }
    else
    {
        pst_dmac_vap->uc_psm_dtim_period = uc_mib_dtim_period;
        pst_dmac_vap->us_psm_listen_interval = (pst_dmac_vap->uc_psm_dtim_period  < DMAC_DEFAULT_DTIM_LISTEN_DIFF) ? ((oal_uint16)(pst_dmac_vap->uc_psm_dtim_period )) : DMAC_DEFAULT_LISTEN_INTERVAL;
        #ifdef _PRE_PM_TBTT_OFFSET_PROBE
        hal_tbtt_offset_probe_suspend(pst_dmac_vap->pst_hal_vap);
        #endif
    }
    /* ��������ղ���beaocn˯��ֵ */
    dmac_psm_update_bcn_tout_max_cnt(pst_dmac_vap);

    /* ���ŵ��а�dtim 1˯�߻��Ѳ���tbtt�жϲ����ղ���beaconҲ��ȥ˯ */
    if (OAL_TRUE == dmac_sta_csa_is_in_waiting(&(pst_dmac_vap->st_vap_base_info)))
    {
        pst_dmac_vap->us_psm_listen_interval = 1;
        pst_dmac_vap->uc_bcn_tout_max_cnt    = 0;//���ŵ������У��ղ���beacon��ȥ˯����֤�����ܽ���
    }

    OAM_WARNING_LOG3(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR, "{dmac_psm_update_dtime_period:set psm dtim period[%d],bcn tout max cnt[%d]listen interval[%d]}",
                        pst_dmac_vap->uc_psm_dtim_period,pst_dmac_vap->uc_bcn_tout_max_cnt,pst_dmac_vap->us_psm_listen_interval);

    /* ����period�Ĵ�����ֵ */
    hal_set_sta_dtim_period(pst_dmac_vap->pst_hal_vap, pst_dmac_vap->uc_psm_dtim_period);
    hal_set_psm_listen_interval(pst_dmac_vap->pst_hal_vap, pst_dmac_vap->us_psm_listen_interval);

    /* ����count�Ĵ���,��ֹ����interval �� dtim ��count������ */
    if ((pst_dmac_vap->us_psm_listen_interval != 1) && (pst_dmac_vap->uc_psm_dtim_period != 1))
    {
        hal_get_psm_dtim_count(pst_dmac_vap->pst_hal_vap, &uc_dtim_cnt);

        /* ����count�Ĵ�����ֵ */
        dmac_psm_sync_dtim_count(pst_dmac_vap, uc_dtim_cnt);
    }

}


oal_void dmac_psm_update_keepalive(dmac_vap_stru *pst_dmac_vap)
{
    mac_sta_pm_handler_stru 	   *pst_mac_sta_pm_handle; /* STA����״̬�½ṹ�� */
    oal_uint32                      ul_dtime_period;
    oal_uint32                      ul_keepalive_time;
    oal_uint32                      ul_mib_bcn_period;

    /* ����Beacon interval�޸Ķ�Ӧ��STA������keepalive��DTIM interval */
    pst_mac_sta_pm_handle = &pst_dmac_vap->st_sta_pm_handler;
    if (OAL_FALSE == pst_mac_sta_pm_handle->en_is_fsm_attached)
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR, "{dmac_psm_update_keepalive::pm fsm not attached.}");
    }
    else
    {
        if (0!=pst_dmac_vap->uc_psm_dtim_period)
        {
            ul_dtime_period = pst_dmac_vap->uc_psm_dtim_period;
        }
        else
        {
            ul_dtime_period = mac_mib_get_dot11dtimperiod(&pst_dmac_vap->st_vap_base_info);
        }

        ul_mib_bcn_period = mac_mib_get_BeaconPeriod(&pst_dmac_vap->st_vap_base_info);
        if ((0 != ul_mib_bcn_period)
            && (0 != ul_dtime_period))
        {
            ul_keepalive_time = WLAN_STA_KEEPALIVE_TIME;

        #ifdef _PRE_WLAN_FEATURE_P2P
            if (IS_P2P_CL(&pst_dmac_vap->st_vap_base_info))
            {
                ul_keepalive_time = WLAN_CL_KEEPALIVE_TIME;
            }
        #endif

            pst_mac_sta_pm_handle->ul_ps_keepalive_max_num = ul_keepalive_time /
                                        (ul_mib_bcn_period*(ul_dtime_period));
        }
        else
        {
            OAM_ERROR_LOG2(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR, "{dmac_psm_update_keepalive::beacon[%d],dtim period[%d]}", ul_mib_bcn_period,ul_dtime_period);
        }
    }
}

#if (!defined(HI1102_EDA)) && (!defined(HI110x_EDA))
oal_void dmac_sta_adjust_tbtt_offset(dmac_vap_stru *pst_dmac_vap)
{
    oal_uint16                  us_beacon_timeout_adjust    =   0UL;
    oal_uint16                  us_in_tbtt_adjust           =   0UL;

    PM_Driver_Cbb_Mac_WlanRxBeaconAdjust(&us_in_tbtt_adjust, &us_beacon_timeout_adjust);
    hal_set_beacon_timeout_val(pst_dmac_vap->pst_hal_device, pst_dmac_vap->us_beacon_timeout + us_beacon_timeout_adjust);
    hal_pm_set_tbtt_offset(pst_dmac_vap->pst_hal_vap, 0);
}

oal_uint32 dmac_psm_is_tim_dtim_set(dmac_vap_stru *pst_dmac_vap, oal_uint8* puc_tim_elm)
{
    oal_uint32                  ul_aid;
    oal_uint32                  ul_min_ix;
    oal_uint32                  ul_max_ix;
    oal_uint32                  ul_tim_dtim_present      = 0;
    mac_tim_ie_stru            *pst_tim_ie;

    pst_tim_ie = (mac_tim_ie_stru *)puc_tim_elm;
    ul_aid = WLAN_AID(pst_dmac_vap->st_vap_base_info.us_sta_aid);
    //ul_ix = ul_aid / WLAN_NBBY;
    ul_min_ix =  pst_tim_ie->uc_tim_bitctl &~ 1;
    ul_max_ix =  pst_tim_ie->uc_tim_len + ul_min_ix - 4;

    /*����Ƿ���TIM*/
    if(pst_tim_ie->uc_tim_ie != MAC_EID_TIM)
    {
        return ul_tim_dtim_present;
    }
    /* ��dtim period�Ĵ��� */
    //hal_get_sta_dtim_period(pst_dmac_vap->pst_hal_vap, &ul_dtim_period);

    /* Check if AP's DTIM period has changed */
    if((pst_tim_ie->uc_dtim_period != mac_mib_get_dot11dtimperiod(&pst_dmac_vap->st_vap_base_info)) || (0 == pst_dmac_vap->uc_psm_dtim_period))
    {
        mac_mib_set_dot11dtimperiod(&(pst_dmac_vap->st_vap_base_info), pst_tim_ie->uc_dtim_period);

        dmac_psm_update_dtime_period(&(pst_dmac_vap->st_vap_base_info),pst_tim_ie->uc_dtim_period,mac_mib_get_BeaconPeriod(&(pst_dmac_vap->st_vap_base_info)));

        /* ����count�Ĵ�����ֵ */
        dmac_psm_sync_dtim_count(pst_dmac_vap, pst_tim_ie->uc_dtim_count);

        dmac_psm_update_keepalive(pst_dmac_vap);
    }

    /* DTIM IS SET */
    if(OAL_TRUE == (pst_tim_ie->uc_tim_bitctl & BIT0))
    {
        ul_tim_dtim_present |= DMAC_DTIM_IS_SET;
    }

    /* TIM IS SET */
    if (((ul_aid >> 3) >= ul_min_ix) && ((ul_aid >> 3) <= ul_max_ix) && WLAN_TIM_ISSET(pst_tim_ie->auc_tim_bitmap - ul_min_ix, ul_aid))
    {

        //OAM_WARNING_LOG2(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR, "{dmac_psm_is_tim_dtim_set::aid:%d, min_ix:[%d]}", ul_aid, ul_min_ix);
        //OAM_WARNING_LOG3(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR, "{max_ix:%d, bitmap[0]:[%d], bitmap[1]:[%d]}", ul_max_ix, pst_time_ie->auc_tim_bitmap[0], pst_time_ie->auc_tim_bitmap[1]);
        ul_tim_dtim_present |= DMAC_TIM_IS_SET;
    }
    return ul_tim_dtim_present;
}


oal_void dmac_psm_rf_bcn_common(dmac_vap_stru *pst_dmac_vap)
{
    mac_sta_pm_handler_stru     *pst_mac_sta_pm_handle;
#ifdef _PRE_WLAN_FEATURE_SINGLE_RF_RX_BCN
    hal_to_dmac_device_stru     *pst_hal_device = pst_dmac_vap->pst_hal_device;
#endif

    dmac_sta_adjust_tbtt_offset(pst_dmac_vap);

    pst_mac_sta_pm_handle = &pst_dmac_vap->st_sta_pm_handler;

    /* ����beacon����ͳ�ƣ�ֻ��tbtt�ж����ñ��ʱͳ�� */
    if (pst_mac_sta_pm_handle->en_beacon_counting == OAL_TRUE)
    {
        #ifdef _PRE_WLAN_FEATURE_SINGLE_RF_RX_BCN
        /* ͳ�Ƶ�ͨ����beacon */
        if (pst_hal_device->bit_srb_switch && pst_dmac_vap->pst_hal_vap->st_pm_info.uc_bcn_rf_chain == pst_hal_device->st_cfg_cap_info.uc_single_tx_chain)
        {
            pst_mac_sta_pm_handle->aul_pmDebugCount[PM_MSG_SINGLE_BCN_RX_CNT]++;
        }
        #endif

        pst_mac_sta_pm_handle->aul_pmDebugCount[PM_MSG_PSM_BEACON_CNT]++;
        pst_mac_sta_pm_handle->en_beacon_counting = OAL_FALSE;
        #ifdef _PRE_PM_TBTT_OFFSET_PROBE
        hal_tbtt_offset_probe_beacon_cnt_incr(pst_dmac_vap->pst_hal_vap);
        #endif
    }

    /* �ɹ����յ�beacon, beacon timeout��ʱ�ļ������� */
    pst_dmac_vap->bit_beacon_timeout_times = 0;

    /* Reset beacon wait bitλ */
    pst_mac_sta_pm_handle->en_beacon_frame_wait  = OAL_FALSE;

}


oal_void dmac_psm_process_tim_elm(dmac_vap_stru *pst_dmac_vap, oal_netbuf_stru *pst_netbuf)
{
    dmac_rx_ctl_stru            *pst_rx_ctrl;
    mac_rx_ctl_stru             *pst_rx_info;
    mac_tim_ie_stru             *pst_tim_ie;
    oal_uint16                   us_frame_len;
    oal_uint32                   ul_tim_dtim_present;
    hal_to_dmac_vap_stru        *pst_hal_vap;
    oal_uint8                   *puc_tim_elm = OAL_PTR_NULL;
    oal_uint8                   *puc_payload;
    mac_sta_pm_handler_stru     *pst_mac_sta_pm_handle;
#ifdef _PRE_WLAN_FEATURE_MAC_PARSE_TIM
    oal_uint16                   us_tim_pos = 0;
#endif
    oal_uint64                   ull_beacon_timestamp;
    oal_uint32                   ul_beacon_timestamp_l;
    oal_uint32                   ul_beacon_timestamp_h;
    oal_uint16                   us_beacon_interval;

    pst_mac_sta_pm_handle = &pst_dmac_vap->st_sta_pm_handler;
    if (OAL_FALSE == pst_mac_sta_pm_handle->en_is_fsm_attached)
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR, "{dmac_psm_process_tim_elm::pm fsm not attached.}");
        return;
    }

    pst_hal_vap         = pst_dmac_vap->pst_hal_vap;
    pst_rx_ctrl         = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
    pst_rx_info         = (mac_rx_ctl_stru *)(&(pst_rx_ctrl->st_rx_info));
    us_frame_len        = pst_rx_info->us_frame_len - pst_rx_info->uc_mac_header_len; /* ֡�峤�� */
    puc_payload         = OAL_NETBUF_PAYLOAD(pst_netbuf);

    /* �����ղ���beaocn�󣬼���timestamp%beacon interval */
    if (pst_dmac_vap->bit_beacon_timeout_times > pst_dmac_vap->uc_bcn_tout_max_cnt)
    {
        ul_beacon_timestamp_l = OAL_JOIN_WORD32(puc_payload[0], puc_payload[1], puc_payload[2], puc_payload[3]);
        ul_beacon_timestamp_h = OAL_JOIN_WORD32(puc_payload[4], puc_payload[5], puc_payload[6], puc_payload[7]);
        ull_beacon_timestamp = ((oal_uint64)ul_beacon_timestamp_h << 32) | ul_beacon_timestamp_l;

        us_beacon_interval = OAL_MAKE_WORD16(puc_payload[MAC_TIME_STAMP_LEN], puc_payload[MAC_TIME_STAMP_LEN + 1]);
        OAM_WARNING_LOG4(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR, "{dmac_psm_process_tim_elm::timestamp low[0x%x]high[0x%x],interval[%d],timestamp mod interval[%d]}",
                                   ul_beacon_timestamp_l, ul_beacon_timestamp_h, us_beacon_interval, (oal_uint32)(ull_beacon_timestamp%((oal_uint32)us_beacon_interval<<10)));
    }

    dmac_psm_rf_bcn_common(pst_dmac_vap);

    /* ��������beacon��������0 */
    pst_mac_sta_pm_handle->uc_tbtt_cnt_since_full_bcn = 0;

#ifdef _PRE_WLAN_FEATURE_SINGLE_RF_RX_BCN
    dmac_psm_update_bcn_rf_chain(pst_dmac_vap, pst_rx_ctrl->st_rx_statistic.c_rssi_dbm);
#endif

    puc_tim_elm = mac_find_ie(MAC_EID_TIM, puc_payload + MAC_DEVICE_BEACON_OFFSET, us_frame_len - MAC_DEVICE_BEACON_OFFSET);
    if (OAL_PTR_NULL == puc_tim_elm)
    {
        return;
    }

    pst_tim_ie         = (mac_tim_ie_stru *)puc_tim_elm;
    /* ��timԪ�ز�����\TIM IE����У�� */
    if ((MAC_EID_TIM != pst_tim_ie->uc_tim_ie) || (pst_tim_ie->uc_tim_len < MAC_MIN_TIM_LEN))
    {
        return;
    }

#ifdef _PRE_WLAN_FEATURE_MAC_PARSE_TIM
    /* ÿ�ν���bcn֡ʱ������timƫ�ƣ��������仯�����MAC�Ĵ��� */
    us_tim_pos = (oal_uint16)(puc_tim_elm - puc_payload);
    if (us_tim_pos != pst_dmac_vap->us_tim_pos)
    {
        OAM_WARNING_LOG2(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR, "{dmac_psm_process_tim_elm::set bcn tim pos[%d]to[%d].}",pst_dmac_vap->us_tim_pos,us_tim_pos);
        pst_dmac_vap->us_tim_pos = us_tim_pos;

        hal_mac_set_bcn_tim_pos(pst_hal_vap, us_tim_pos);
    }
#endif

    /* �ǽ���ģʽ������beacon֡���� */
    if ((mac_mib_get_powermanagementmode(&(pst_dmac_vap->st_vap_base_info)) != WLAN_MIB_PWR_MGMT_MODE_PWRSAVE) ||
            (0 == pst_dmac_vap->st_vap_base_info.us_sta_aid))
    {
        return;
    }

#ifdef _PRE_WLAN_FEATURE_HISTREAM
    if (OAL_TRUE == dmac_psm_auto_dtim(pst_dmac_vap))
    {
        /* ����count�Ĵ�����ֵ */
        dmac_psm_sync_dtim_count(pst_dmac_vap, pst_tim_ie->uc_dtim_count);
        dmac_psm_update_keepalive(pst_dmac_vap);
    }
#endif //_PRE_WLAN_FEATURE_HISTREAM

    /* ȡDTIMֵ,��֤˯����APͬ�� �Ȳ���dtim cnt ͬ�� */
    if (pst_hal_vap->uc_dtim_cnt != pst_tim_ie->uc_dtim_count)
    {
        /* ����count�Ĵ�����ֵ */
        dmac_psm_sync_dtim_count(pst_dmac_vap, pst_tim_ie->uc_dtim_count);

        //hal_vap_tsf_get_32bit(pst_hal_vap, &ul_tsf_lo);
        //OAL_IO_PRINT("diff dtim count ap:[%d], sta:[%d],bank3 dtim cnt:[%d], tbtt status:[%d]\r\n", pst_time_ie->uc_dtim_count, pst_hal_vap->uc_dtim_cnt, uc_dtim_cnt, (READL(0X20100658)));
        //OAL_IO_PRINT("diff dtim count ap:[%d], sta:[%d],tbtt status:[%d]\r\n\r\n", pst_time_ie->uc_dtim_count, pst_hal_vap->uc_dtim_cnt, (READL(0X20100658)));
        //OAL_IO_PRINT("tsf timer:[%u]\r\n",ul_tsf_lo);
    }

    if (STA_PWR_SAVE_STATE_DOZE == GET_PM_STATE(pst_mac_sta_pm_handle))
    {
        dmac_pm_sta_post_event(pst_dmac_vap, STA_PWR_EVENT_RX_BEACON, 0, OAL_PTR_NULL);
    }

    ul_tim_dtim_present = dmac_psm_is_tim_dtim_set(pst_dmac_vap, puc_tim_elm);

    if (STA_PWR_SAVE_STATE_AWAKE == GET_PM_STATE(pst_mac_sta_pm_handle))
    {
        /* TIM/DTIM is not set */
        if (0 == ul_tim_dtim_present)
        {
#ifdef _PRE_WLAN_FEATURE_P2P
            dmac_set_ps_poll_rsp_pending(pst_dmac_vap, OAL_FALSE);
#endif
            pst_mac_sta_pm_handle->en_more_data_expected = OAL_FALSE;

            /* ״̬�л��ɹ�����ܽ���TIMԪ�صĺ������� */
            if ((OAL_FALSE == pst_mac_sta_pm_handle->st_null_wait.en_active_null_wait) &&
            (OAL_FALSE == pst_mac_sta_pm_handle->st_null_wait.en_doze_null_wait))
            {
                pst_mac_sta_pm_handle->uc_beacon_fail_doze_trans_cnt = 0;

                if((OAL_TRUE == pst_dmac_vap->st_vap_base_info.st_cap_flag.bit_keepalive)
                  && ((pst_mac_sta_pm_handle->ul_ps_keepalive_cnt) >= (pst_mac_sta_pm_handle->ul_ps_keepalive_max_num)))
                {
                    if(OAL_SUCC != dmac_send_null_frame_to_ap(pst_dmac_vap, STA_PWR_SAVE_STATE_DOZE, OAL_FALSE))
                    {
                        OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR, "{dmac_psm_process_tim_elm::keepalive send null data fail.}");
                    }

                    pst_mac_sta_pm_handle->ul_ps_keepalive_cnt = 0;
                    return;
                }

                dmac_pm_sta_post_event(pst_dmac_vap, STA_PWR_EVENT_NORMAL_SLEEP, 0, OAL_PTR_NULL);

            }
            else
            {
                pst_mac_sta_pm_handle->uc_beacon_fail_doze_trans_cnt++;
                if (DMAC_BEACON_DOZE_TRANS_FAIL_NUM  < pst_mac_sta_pm_handle->uc_beacon_fail_doze_trans_cnt)
                {
                    pst_mac_sta_pm_handle->uc_beacon_fail_doze_trans_cnt = 0;
                    OAM_WARNING_LOG2(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR, "{dmac_psm_process_tim_elm::beacon trans doze fail:reason:active null:[%d],doze null:[%d].}",
                                        pst_mac_sta_pm_handle->st_null_wait.en_active_null_wait, pst_mac_sta_pm_handle->st_null_wait.en_doze_null_wait);
                }
            }
        }

        /* TIM is set */
        else if ((ul_tim_dtim_present & DMAC_TIM_IS_SET) != 0 )
        {
            if (OAL_TRUE == dmac_is_any_legacy_ac_present(pst_dmac_vap))
            {
                pst_mac_sta_pm_handle->en_more_data_expected = OAL_TRUE;

                if (OAL_FALSE == dmac_is_sta_fast_ps_enabled(pst_mac_sta_pm_handle))
                {
                    /* Changes state to STA_ACTIVE and send ACTIVE NULL frame to */
                    /* AP immeditately in case of FAST PS mode                   */
                    dmac_pm_sta_post_event(pst_dmac_vap, STA_PWR_EVENT_TIM, 0, OAL_PTR_NULL);
                }
                else
                {
                    if (OAL_SUCC != dmac_psm_process_fast_ps_state_change(pst_dmac_vap, STA_PWR_SAVE_STATE_ACTIVE))
                    {
                        OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR,"{dmac_psm_process_tim_elm::dmac_psm_process_fast_ps_state_change failed.}");
                    }
                }
            }
            else
            {
                dmac_psm_process_tim_set_sta_prot(pst_dmac_vap);
            }

            pst_mac_sta_pm_handle->aul_pmDebugCount[PM_MSG_TIM_AWAKE]++;
        }

        /* DTIM set, stay in AWAKE mode to recieve all broadcast frames */
        else
        {
            dmac_pm_sta_post_event(pst_dmac_vap, STA_PWR_EVENT_DTIM, 0, OAL_PTR_NULL);
        }
    }
    else
    {
        /* Some APs (For Eg. Buffalo AP) sends TIM element set even after  */
        /* our NULL FRAME(ACTIVE) reception. If the TIM element says more  */
        /* data is on the way, then set the appropriate flag else reset it */
        pst_mac_sta_pm_handle->en_more_data_expected = (ul_tim_dtim_present != 0) ? OAL_TRUE : OAL_FALSE;

		/* staut�ᶪtbtt�жϣ������ʱ��� */
        if (OAL_TRUE == (ul_tim_dtim_present & DMAC_TIM_IS_SET))
        {
            if (STA_PWR_SAVE_STATE_DOZE == GET_PM_STATE(pst_mac_sta_pm_handle))
            {
                dmac_pm_sta_post_event(pst_dmac_vap, STA_PWR_EVENT_RX_BEACON, 0, OAL_PTR_NULL);
            }

            if (OAL_TRUE == dmac_is_any_legacy_ac_present(pst_dmac_vap))
            {
                if (OAL_TRUE == dmac_is_sta_fast_ps_enabled(pst_mac_sta_pm_handle)) /* fast ps ģʽ */
                {
                    dmac_send_null_frame_to_ap(pst_dmac_vap, STA_PWR_SAVE_STATE_ACTIVE, OAL_FALSE);
                }
                else
                {
                    dmac_send_pspoll_to_ap(pst_dmac_vap);                           /* pspoll ģʽ */
                }
            }
            else
            {
                dmac_psm_process_tim_set_sta_prot(pst_dmac_vap);                    /* UAPSDģʽ */
            }
        }
    }

}

#ifdef _PRE_WLAN_FEATURE_NO_FRM_INT

oal_uint32 dmac_bcn_no_frm_event_hander(frw_event_mem_stru *pst_event_mem)
{
    dmac_vap_stru               *pst_dmac_vap;
    frw_event_stru              *pst_event;
    mac_sta_pm_handler_stru     *pst_sta_pm_handler;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(0, OAM_SF_WIFI_BEACON, "{dmac_bcn_no_frm_event_hander:event is null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event = frw_get_event_stru(pst_event_mem);
    pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_event->st_event_hdr.uc_vap_id);

    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_WIFI_BEACON, "{dmac_bcn_no_frm_event_hander:vap is null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��sta������Ч */
    if (hal_device_calc_up_vap_num(pst_dmac_vap->pst_hal_device) != 1)
    {
        return OAL_SUCC;
    }

    /* �����ϴ��յ�����bcn��tbtt cnt��������ʱ����doze������bcn */
    pst_sta_pm_handler = &pst_dmac_vap->st_sta_pm_handler;
    if (pst_sta_pm_handler->uc_tbtt_cnt_since_full_bcn > pst_sta_pm_handler->uc_max_skip_bcn_cnt)
    {
        return OAL_SUCC;
    }

    dmac_psm_rf_bcn_common(pst_dmac_vap);
    dmac_pm_sta_post_event(pst_dmac_vap, STA_PWR_EVENT_NO_PS_FRM, 0, OAL_PTR_NULL);

    /* linkloss������0,����+1 */
    dmac_vap_linkloss_clean(pst_dmac_vap);
    dmac_vap_linkloss_threshold_incr(pst_dmac_vap);

    return OAL_SUCC;
}
#endif
#else

oal_void dmac_psm_process_tim_elm(dmac_vap_stru *pst_dmac_vap, oal_netbuf_stru *pst_netbuf)
{
    mac_sta_pm_handler_stru     *pst_mac_sta_pm_handle;

    pst_mac_sta_pm_handle = &pst_dmac_vap->st_sta_pm_handler;
    if (OAL_FALSE == pst_mac_sta_pm_handle->en_is_fsm_attached)
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR, "{dmac_psm_process_tim_elm::pm fsm not attached.}");
        return;
    }

    /* ����beacon����ͳ�ƣ�ֻ��tbtt�ж����ñ��ʱͳ�� */
    if (pst_mac_sta_pm_handle->en_beacon_counting == OAL_TRUE)
    {
        pst_mac_sta_pm_handle->aul_pmDebugCount[PM_MSG_PSM_BEACON_CNT]++;
        pst_mac_sta_pm_handle->en_beacon_counting = OAL_FALSE;
    }
    dmac_pm_sta_post_event(pst_dmac_vap, STA_PWR_EVENT_BEACON_TIMEOUT, 0, OAL_PTR_NULL);
}
#endif


oal_uint32  dmac_psm_rx_process_data_sta(dmac_vap_stru *pst_dmac_vap, oal_netbuf_stru *pst_buf)
{
    oal_uint8                *puc_dest_addr;      /* Ŀ�ĵ�ַ */
    mac_ieee80211_frame_stru *pst_frame_hdr;
    mac_sta_pm_handler_stru  *pst_mac_sta_pm_handle;

    pst_mac_sta_pm_handle = &pst_dmac_vap->st_sta_pm_handler;
    if (OAL_FALSE == pst_mac_sta_pm_handle->en_is_fsm_attached)
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR, "{dmac_psm_rx_process_data_sta::pm fsm not attached.}");
        return DMAC_RX_FRAME_CTRL_GOON;
    }

    pst_frame_hdr = (mac_ieee80211_frame_stru *)OAL_NETBUF_HEADER(pst_buf);
    puc_dest_addr = pst_frame_hdr->auc_address1;

    if ((mac_mib_get_powermanagementmode(&(pst_dmac_vap->st_vap_base_info)) != WLAN_MIB_PWR_MGMT_MODE_PWRSAVE) ||
        (0 == pst_dmac_vap->st_vap_base_info.us_sta_aid))
    {
        dmac_psm_process_no_powersave(pst_dmac_vap);

        if (!ETHER_IS_MULTICAST(puc_dest_addr))
        {
            //OAM_INFO_LOG2(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_KEEPALIVE, "keepalive rx data:type:[%d],subtype:[%d]", pst_frame_hdr->st_frame_control.bit_type, pst_frame_hdr->st_frame_control.bit_sub_type);
            dmac_psm_sta_incr_activity_cnt(pst_mac_sta_pm_handle);
        }
        return DMAC_RX_FRAME_CTRL_GOON;
    }

#ifdef _PRE_WLAN_DOWNLOAD_PM
    if (STA_PWR_SAVE_STATE_ACTIVE == GET_PM_STATE(pst_mac_sta_pm_handle))
    {
        if (g_us_download_rate_limit_pps && (WLAN_NULL_FRAME != pst_frame_hdr->st_frame_control.bit_sub_type)
           && (WLAN_QOS_NULL_FRAME != pst_frame_hdr->st_frame_control.bit_sub_type))
        {
            dmac_rx_ctl_stru  *pst_cb_ctrl = (dmac_rx_ctl_stru*)oal_netbuf_cb(pst_buf);
            pst_mac_sta_pm_handle->ul_rx_cnt++;

            if((pst_mac_sta_pm_handle->ul_rx_cnt >= g_us_download_rate_limit_pps) && (0 == pst_cb_ctrl->st_rx_info.bit_is_key_frame))
            {
                if(pst_mac_sta_pm_handle->ul_rx_cnt == g_us_download_rate_limit_pps)
                {
                    g_ul_drop_time = (oal_uint32)OAL_TIME_GET_STAMP_MS();
                    OAM_WARNING_LOG2(0,OAM_SF_PWR,"wifi download pm version,rx %d pkts drop timestamp[%d]",pst_mac_sta_pm_handle->ul_rx_cnt,g_ul_drop_time);
                }

                return DMAC_RX_FRAME_CTRL_DROP;
            }
        }
    }
    else
#else
    if (STA_PWR_SAVE_STATE_ACTIVE != GET_PM_STATE(pst_mac_sta_pm_handle))
#endif
    {
        /* �����ʱ����doze״̬���е�awake״̬ */
        if (STA_PWR_SAVE_STATE_DOZE == GET_PM_STATE(pst_mac_sta_pm_handle))
        {
            OAM_WARNING_LOG3(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR,"{dmac_psm_rx_process_data_sta::event[%d] change state doze,frame is multi[%d],bcn wait[%d]}",pst_mac_sta_pm_handle->uc_doze_event,ETHER_IS_MULTICAST(puc_dest_addr),pst_mac_sta_pm_handle->en_beacon_frame_wait);
            dmac_pm_sta_post_event(pst_dmac_vap, STA_PWR_EVENT_RX_DATA, 0, OAL_PTR_NULL);
        }

        /* Handle Multicast/Broadcast frames and switch to DOZE state based on the More Data bit */
        if (OAL_TRUE == ETHER_IS_MULTICAST(puc_dest_addr))
        {
            if (OAL_FALSE == dmac_psm_get_more_data_sta(pst_frame_hdr))
            {
                /* �յ����һ���㲥�鲥,�رչ㲥�鲥��ʱ�ȴ���ʱ�� */
                if (OAL_TRUE == pst_mac_sta_pm_handle->st_mcast_timer.en_is_registerd)
                {
                    FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_mac_sta_pm_handle->st_mcast_timer));
                }

                dmac_pm_sta_post_event(pst_dmac_vap, STA_PWR_EVENT_LAST_MCAST, 0, OAL_PTR_NULL);
            }
            else
            {
                pst_mac_sta_pm_handle->en_more_data_expected = OAL_TRUE;

                /* �յ��������һ���㲥�鲥,������ʱ��,�ٵȴ�һ����ʱ��ʱ�� */
                FRW_TIMER_RESTART_TIMER(&(pst_mac_sta_pm_handle->st_mcast_timer), pst_mac_sta_pm_handle->us_mcast_timeout, OAL_FALSE);
            }
        }

        /* �յ������Ĵ��� */
        else
        {
            if (OAL_FALSE == dmac_psm_get_more_data_sta(pst_frame_hdr))
            {
                pst_mac_sta_pm_handle->en_more_data_expected = OAL_FALSE;
            }
            else
            {
                if (OAL_TRUE == dmac_is_any_legacy_ac_present(pst_dmac_vap))
                {
                    //OAM_INFO_LOG2(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR, "{dmac_psm_rx_process_data_sta::type is %d, sub type is %d\r\n}", pst_frame_hdr->st_frame_control.bit_type,
                    //                                                                            pst_frame_hdr->st_frame_control.bit_sub_type);
                    dmac_pm_sta_post_event(pst_dmac_vap, STA_PWR_EVENT_RX_UCAST, 0, OAL_PTR_NULL);
                }

                pst_mac_sta_pm_handle->en_more_data_expected = OAL_TRUE;
            }
            dmac_process_rx_process_data_sta_prot(pst_dmac_vap, pst_buf);
        }
    }

    return DMAC_RX_FRAME_CTRL_GOON;
}
#if (_PRE_OS_VERSION_RAW == _PRE_OS_VERSION)  && defined (__CC_ARM)
#pragma arm section rwdata = "BTCM", code ="ATCM", zidata = "BTCM", rodata = "ATCM"
#endif


oal_uint8 dmac_psm_tx_process_data_sta(dmac_vap_stru *pst_dmac_vap, mac_tx_ctl_stru *pst_tx_ctl)
{
    oal_uint8                 uc_pwr_mgmt_bit = 0;
    mac_sta_pm_handler_stru  *pst_mac_sta_pm_handle;

    pst_mac_sta_pm_handle = &pst_dmac_vap->st_sta_pm_handler;

    /* �ǽ���ģʽ�µĴ��� */
    if ((mac_mib_get_powermanagementmode(&(pst_dmac_vap->st_vap_base_info)) != WLAN_MIB_PWR_MGMT_MODE_PWRSAVE) ||
        (0 == pst_dmac_vap->st_vap_base_info.us_sta_aid))
    {
        dmac_psm_process_no_powersave(pst_dmac_vap);
        //OAM_INFO_LOG2(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_KEEPALIVE, "keepalive tx data ismcast:[%d],is eapol:[%d]", MAC_GET_CB_IS_MCAST(pst_tx_ctl), MAC_GET_CB_IS_EAPOL(pst_tx_ctl));

        /* Increment activity counter */
        dmac_psm_sta_incr_activity_cnt(pst_mac_sta_pm_handle);
        return uc_pwr_mgmt_bit;
    }

    if (STA_PWR_SAVE_STATE_ACTIVE == GET_PM_STATE(pst_mac_sta_pm_handle))
    {
        if ((WLAN_NULL_FRAME !=  MAC_GET_CB_FRAME_HEADER_ADDR(pst_tx_ctl)->st_frame_control.bit_sub_type)
                &&(WLAN_QOS_NULL_FRAME != MAC_GET_CB_FRAME_HEADER_ADDR(pst_tx_ctl)->st_frame_control.bit_sub_type))
        {
            pst_mac_sta_pm_handle->ul_psm_pkt_cnt++;
        }
    }
    else
    {
        uc_pwr_mgmt_bit = 1;
        /*lint -e731*/
        if (STA_PWR_SAVE_STATE_DOZE == GET_PM_STATE(pst_mac_sta_pm_handle))
        {
            //OAM_INFO_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR, "{dmac_psm_tx_process_data_sta::pm state:[%d] not awake}", STA_GET_PM_STATE(pst_mac_sta_pm_handle));
            dmac_pm_sta_post_event(pst_dmac_vap, STA_PWR_EVENT_TX_DATA, 0, OAL_PTR_NULL);
        }
        /*lint +e731*/
        /* ���͵�һ������/�����pm=0 */
        if (STA_PWR_SAVE_STATE_ACTIVE == dmac_psm_process_tx_process_data_sta_prot(pst_dmac_vap, pst_tx_ctl))
        {
            if(OAL_TRUE == dmac_is_sta_fast_ps_enabled(pst_mac_sta_pm_handle))
            {
                /* ֻ����awake״̬��,tx pm=0 ��data�л���active״̬ */
                if ((STA_PWR_SAVE_STATE_AWAKE == GET_PM_STATE(pst_mac_sta_pm_handle)))
                {
                    uc_pwr_mgmt_bit = 0;
                    pst_mac_sta_pm_handle->en_direct_change_to_active = OAL_TRUE;
                }
            }
        }
    }
    return uc_pwr_mgmt_bit;
}

oal_void dmac_psm_tx_set_power_mgmt_bit(dmac_vap_stru *pst_dmac_vap, mac_tx_ctl_stru *pst_tx_ctl)
{
    oal_uint8                 uc_pwr_mgmt_bit = 0;

    /* ����Ƿ��ǽ���ģʽ */
    uc_pwr_mgmt_bit = dmac_psm_tx_process_data_sta(pst_dmac_vap, pst_tx_ctl);
    if (OAL_TRUE == uc_pwr_mgmt_bit)
    {
        MAC_GET_CB_FRAME_HEADER_ADDR(pst_tx_ctl)->st_frame_control.bit_power_mgmt = OAL_TRUE;
    }

    return;
}

#if (_PRE_OS_VERSION_RAW == _PRE_OS_VERSION)  && defined (__CC_ARM)
#pragma arm section rodata, code, rwdata, zidata  // return to default placement
#endif


oal_uint32 dmac_null_frame_complete_sta(dmac_vap_stru *pst_dmac_vap, oal_uint8 uc_dscr_status, oal_uint8 uc_send_rate_rank, oal_netbuf_stru  *pst_netbuf)
{
    mac_sta_pm_handler_stru             *pst_mac_sta_pm_handle;
    mac_ieee80211_frame_stru            *pst_mac_header;

    pst_mac_header = (mac_ieee80211_frame_stru *)OAL_NETBUF_HEADER(pst_netbuf);
    pst_mac_sta_pm_handle = &pst_dmac_vap->st_sta_pm_handler;
    if (OAL_FALSE == pst_mac_sta_pm_handle->en_is_fsm_attached)
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR, "{dmac_null_frame_complete_sta::pm fsm not attached.}");
        return OAL_FAIL;
    }

    if ((OAL_TRUE == pst_mac_sta_pm_handle->st_null_wait.en_active_null_wait) ||
        (OAL_TRUE == pst_mac_sta_pm_handle->st_null_wait.en_doze_null_wait))
    {
        /* NULL֡���ͳɹ��Ĵ��� */
        if (DMAC_TX_SUCC == uc_dscr_status)
        {
            /* PM��״̬��Ӧ�ô���DOZE״̬������Ǵ�״̬�л���awake״̬ */
            if (STA_PWR_SAVE_STATE_DOZE == GET_PM_STATE(pst_mac_sta_pm_handle))
            {
                //OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR, "{dmac_null_frame_complete_sta::pm state is [%d] not awake}", STA_GET_PM_STATE(pst_mac_sta_pm_handle));
                //OAL_IO_PRINT("dmac_null_frame_complete_sta::event:[%d] change state to doze not awake\r\n", pst_mac_sta_pm_handle->uc_doze_event);
                dmac_pm_sta_post_event(pst_dmac_vap, STA_PWR_EVENT_SEND_NULL_SUCCESS, 0, OAL_PTR_NULL);
            }
            /* null ֡�ش���־���� */
            if (STA_PWR_SAVE_STATE_ACTIVE == pst_mac_header->st_frame_control.bit_power_mgmt)
            {
                pst_mac_sta_pm_handle->uc_active_null_retran_cnt = 0;

                pst_mac_sta_pm_handle->en_ps_back_active_pause = OAL_FALSE;
            }
            else
            {
                pst_mac_sta_pm_handle->uc_doze_null_retran_cnt = 0;

                pst_mac_sta_pm_handle->en_ps_back_doze_pause   = OAL_FALSE;
            }
            dmac_pm_sta_post_event(pst_dmac_vap, STA_PWR_EVENT_SEND_NULL_SUCCESS, OAL_SIZEOF(mac_ieee80211_frame_stru), (oal_uint8 *)pst_mac_header);

        #if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
            /* ��6Mbps���ͳɹ��������ӳɹ��������� */
            if (uc_send_rate_rank == HAL_TX_RATE_RANK_0)
            {
                dmac_psm_inc_null_frm_ofdm_succ(pst_dmac_vap);
            }
            /* ֻ����1Mbps���ͳɹ�������ٳɹ��������� */
            else
            {
                dmac_psm_dec_null_frm_ofdm_succ(pst_dmac_vap);
            }
        #endif
        }
        /* NULL֡���Ͳ��ɹ�,���ش�10��,Ӳ���ش�8��(һ��80��)��,�Զ���Ȼû����Ӧ,�����ʱ��·�쳣,ȥ����liuzhengqi todo */
        else
        {
        #if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
            dmac_psm_dec_null_frm_ofdm_succ(pst_dmac_vap);
        #endif

            if ((MAC_VAP_STATE_UP == pst_dmac_vap->st_vap_base_info.en_vap_state))
            {
                /* If  transmission status is timeout reset the global flags and retransmit the NULL frame */
                if (STA_PWR_SAVE_STATE_ACTIVE == pst_mac_header->st_frame_control.bit_power_mgmt)
                {
                    pst_mac_sta_pm_handle->uc_active_null_retran_cnt++;

                    if (DMAC_TX_SOFT_PSM_BACK == uc_dscr_status)
                    {
                        pst_mac_sta_pm_handle->en_ps_back_active_pause = OAL_TRUE;
                    }

                    pst_mac_sta_pm_handle->st_null_wait.en_active_null_wait = OAL_FALSE;

                    /*���ѵ�null֡�ش�ʧ��,1���´�ap beacon��֪�����������,2����������,���¸�����֡�������ͻ���null֡ */
                    if (pst_mac_sta_pm_handle->uc_active_null_retran_cnt > WLAN_MAX_NULL_SENT_NUM)
                    {
                        OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR, "{dmac_null_frame_complete_sta::sta_pm state[%d],retrans active null data to max cnt}",GET_PM_STATE(pst_mac_sta_pm_handle));
                        pst_mac_sta_pm_handle->uc_active_null_retran_cnt = 0;
                        return OAL_SUCC;
                    }

                }
                else
                {
                    pst_mac_sta_pm_handle->uc_doze_null_retran_cnt++;

                    if (DMAC_TX_SOFT_PSM_BACK == uc_dscr_status)
                    {
                        pst_mac_sta_pm_handle->en_ps_back_doze_pause = OAL_TRUE;
                    }

                    pst_mac_sta_pm_handle->st_null_wait.en_doze_null_wait = OAL_FALSE;

                    if (pst_mac_sta_pm_handle->uc_doze_null_retran_cnt > WLAN_MAX_NULL_SENT_NUM)
                    {
                        OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR, "{dmac_null_frame_complete_sta::sta_pm state[%d],retrans sleep null data to max cnt}",GET_PM_STATE(pst_mac_sta_pm_handle));

                        /* awake״̬�·���˯��null֡ʧ�ܴ�ʱӦ��Ϊkeepalive��null֡,������һ�ε�keepalive�ĵ���ʱ��5s�󴥷� */
                        if ((STA_PWR_SAVE_STATE_AWAKE == GET_PM_STATE(pst_mac_sta_pm_handle)))
                        {
                            pst_mac_sta_pm_handle->ul_ps_keepalive_cnt = (pst_mac_sta_pm_handle->ul_ps_keepalive_max_num) >> 2;
                        }

                        /* acticve->doze��˯��null֡,�ط�WLAN_MAX_NULL_SENT_NUM��,������ʱ���൱���ӳ�һ����ʱ��ʱ���ٷ� */
                        else
                        {
                            dmac_psm_start_activity_timer(pst_dmac_vap,pst_mac_sta_pm_handle);
                        }

                        pst_mac_sta_pm_handle->uc_doze_null_retran_cnt = 0;
                        return OAL_SUCC;
                    }
                }

                /* ��������ܻ��˼�������ش�,������noa���ѵ�ʱ���ʹ�null֡ */
                if (DMAC_TX_SOFT_PSM_BACK != uc_dscr_status)
                {
                    if (OAL_SUCC != dmac_send_null_frame_to_ap(pst_dmac_vap, (oal_uint8)(pst_mac_header->st_frame_control.bit_power_mgmt), OAL_FALSE))
                    {
                        OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR, "{dmac_null_frame_complete_sta::retrans null data fail}");
                    }
                }
            }
            /* ����ǰup,�������ʱpause,���Ͳ��ɹ�,��null֡�ȴ���־λ,��ֹ��Ҳ˯����ȥ */
            else
            {
                OAM_WARNING_LOG3(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR, "{dmac_null_frame_complete_sta::pm[%d]vap state[%d]send null fail[%d]}",pst_mac_header->st_frame_control.bit_power_mgmt,
                                                                                                                 pst_dmac_vap->st_vap_base_info.en_vap_state, uc_dscr_status);
                if (STA_PWR_SAVE_STATE_ACTIVE == pst_mac_header->st_frame_control.bit_power_mgmt)
                {
                    pst_mac_sta_pm_handle->st_null_wait.en_active_null_wait = OAL_FALSE;
                }
                else
                {
                    pst_mac_sta_pm_handle->st_null_wait.en_doze_null_wait = OAL_FALSE;

                    /* acticve->doze��˯��null֡,����activtity��ʱ��,��ֹ����active״̬,��˯�߶�ʱ��,�޷�˯�� */
                    if (STA_PWR_SAVE_STATE_ACTIVE == GET_PM_STATE(pst_mac_sta_pm_handle))
                    {
                        dmac_psm_start_activity_timer(pst_dmac_vap,pst_mac_sta_pm_handle);
                    }
                }
            }
        }
    }
    return OAL_SUCC;
}

#if (_PRE_OS_VERSION_RAW == _PRE_OS_VERSION)  && defined (__CC_ARM)
#pragma arm section rwdata = "BTCM", code ="ATCM", zidata = "BTCM", rodata = "ATCM"
#endif

oal_void dmac_psm_tx_complete_sta(dmac_vap_stru *pst_dmac_vap, hal_tx_dscr_stru  *pst_dscr, oal_netbuf_stru *pst_netbuf)
{
    hal_to_dmac_device_stru             *pst_hal_device;
    mac_ieee80211_frame_stru            *pst_mac_header;
    mac_sta_pm_handler_stru             *pst_mac_sta_pm_handle;
    oal_uint8                            uc_dscr_status;
    oal_uint8                            uc_send_rate_rank;
    oal_uint8                            uc_subtype;
    oal_uint8                            uc_type;
    oal_uint16                           us_user_idx;

    /* p2p ɨ����dev �޵͹���״̬���ṹ�壬����Ҫ���е͹��Ĵ��� */
    if (IS_P2P_DEV(&(pst_dmac_vap->st_vap_base_info)))
    {
        return;
    }

    /* �ǽ���ģʽ�µĴ��� */
    if ((mac_mib_get_powermanagementmode(&(pst_dmac_vap->st_vap_base_info)) != WLAN_MIB_PWR_MGMT_MODE_PWRSAVE)
         || (0 == pst_dmac_vap->st_vap_base_info.us_sta_aid))
    {
        dmac_psm_process_no_powersave(pst_dmac_vap);

        return;
    }

    pst_hal_device        = pst_dmac_vap->pst_hal_device;
    if (OAL_PTR_NULL == pst_hal_device)
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR, "{dmac_psm_tx_complete_sta:hal device is null.}");
        return;
    }

    pst_mac_sta_pm_handle = &pst_dmac_vap->st_sta_pm_handler;
    if (OAL_FALSE == pst_mac_sta_pm_handle->en_is_fsm_attached)
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR, "{dmac_psm_tx_complete_sta:pm fsm not attached.}");
    }

    /* ����ģʽ�µĴ��� */
    if (WLAN_MIB_PWR_MGMT_MODE_PWRSAVE == (mac_mib_get_powermanagementmode(&(pst_dmac_vap->st_vap_base_info))))
    {
        pst_mac_header = (mac_ieee80211_frame_stru *)oal_netbuf_header(pst_netbuf);
        uc_subtype     =  mac_get_frame_type_and_subtype((oal_uint8 *)pst_mac_header);
        uc_type        =  mac_get_frame_type((oal_uint8 *)pst_mac_header);

        /* ��ȡ����״̬λ */
        hal_tx_get_dscr_status(pst_dmac_vap->pst_hal_device, pst_dscr, &uc_dscr_status);
        hal_tx_get_dscr_send_rate_rank(pst_dmac_vap->pst_hal_device, pst_dscr, &uc_send_rate_rank);

        if ((DMAC_TX_SUCC == uc_dscr_status) && (WLAN_FC0_TYPE_DATA == uc_type))
        {
            pst_mac_sta_pm_handle->ul_ps_keepalive_cnt = 0;
        }

        /* �������ʱ����doze״̬(˯��null֡�ȷ��ͳɹ���״̬���е�doze, pm=0�����ݰ��ķ�����ɲŵ�)���쳣����,���е�awake״̬ */
        if (STA_PWR_SAVE_STATE_DOZE == GET_PM_STATE(pst_mac_sta_pm_handle))
        {
            dmac_pm_sta_post_event(pst_dmac_vap, STA_PWR_EVENT_TX_COMPLETE, 0, OAL_PTR_NULL);
        }

        /* NULL֡�ķ������ */
        if ((WLAN_FC0_SUBTYPE_NODATA | WLAN_FC0_TYPE_DATA) == uc_subtype)
        {
            if (OAL_SUCC != (dmac_null_frame_complete_sta(pst_dmac_vap, uc_dscr_status, uc_send_rate_rank, pst_netbuf)))
            {
                return;
            }
        }

        /* For non-NULL frame check if a NULL ACTIVE frame has to be sent to */
        /* the AP and then process the frame based on protocol.              */
        /* ��ֹ�������ݰ�,�������˹���֡�ķ�������ж�,pro req��,���˵ȴ���־λ */
        else
        {
            /* ���۳ɹ�ʧ�ܶ�������ȴ���־λ,���ڷ��ͻ��ѵ�null֡��ǰ�е���active,�������˵ȴ���־λ�Ļ��ѵ����ݰ��ŵ�*/
            /* ��������ap�ĵ���֡��������״̬*/
            if (OAL_SUCC == mac_vap_find_user_by_macaddr(&(pst_dmac_vap->st_vap_base_info), pst_mac_header->auc_address1, &us_user_idx))
            {
                /* ֱ���л���־ֻ�ȴ�һ����������ж� */
                if (OAL_TRUE == pst_mac_sta_pm_handle->en_direct_change_to_active)
                {
                    pst_mac_sta_pm_handle->en_direct_change_to_active = OAL_FALSE;
                }

                /* awake״̬�·��ͻ��ѵ����ݰ��ɹ������л���active״̬,���ܴ˰��Ƿ��ͳɹ����л���active״̬ */
                if ((STA_PWR_SAVE_STATE_AWAKE == GET_PM_STATE(pst_mac_sta_pm_handle)) && (0 == pst_mac_header->st_frame_control.bit_power_mgmt))
                {
                    dmac_pm_change_to_active_state(pst_dmac_vap, STA_PWR_EVENT_TX_COMPLETE);
                }
            }

            dmac_psm_process_tx_complete_sta_prot(pst_dmac_vap, uc_dscr_status, pst_netbuf);
        }
    }
}

#if (_PRE_OS_VERSION_RAW == _PRE_OS_VERSION)  && defined (__CC_ARM)
#pragma arm section rodata, code, rwdata, zidata  // return to default placement
#endif


oal_void dmac_psm_process_tbtt_sta(dmac_vap_stru *pst_dmac_vap, mac_device_stru  *pst_mac_device)
{
    mac_sta_pm_handler_stru         *pst_sta_pm_handle;
    oal_uint32                       ul_max_allow_sleep_time;
    oal_uint32                       ul_max_pl_sleep_time;

    pst_sta_pm_handle = &pst_dmac_vap->st_sta_pm_handler;
    if (OAL_FALSE == pst_sta_pm_handle->en_is_fsm_attached)
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR, "{dmac_psm_process_tbtt_sta::pm fsm not attached.}");
        return;
    }

    if (MAC_SCAN_STATE_RUNNING != pst_mac_device->en_curr_scan_state)
    {
        /* ����tbtt����ͳ�� */
        pst_sta_pm_handle->aul_pmDebugCount[PM_MSG_TBTT_CNT]++;
        pst_sta_pm_handle->en_beacon_counting = OAL_TRUE;
        #ifdef _PRE_PM_TBTT_OFFSET_PROBE
        hal_tbtt_offset_probe_tbtt_cnt_incr(pst_dmac_vap->pst_hal_vap);
        #endif
    }

    pst_sta_pm_handle->ul_ps_keepalive_cnt++;

    if ((mac_mib_get_powermanagementmode(&(pst_dmac_vap->st_vap_base_info)) != WLAN_MIB_PWR_MGMT_MODE_PWRSAVE)
        ||(0 == pst_dmac_vap->st_vap_base_info.us_sta_aid))
    {
        dmac_psm_process_no_powersave(pst_dmac_vap);
        return;
    }


    ul_max_allow_sleep_time = dmac_psm_get_max_sleep_time(pst_dmac_vap);
    ul_max_pl_sleep_time    = g_ul_max_deep_sleep_time > g_ul_max_light_sleep_time ? g_ul_max_deep_sleep_time : g_ul_max_light_sleep_time;

    /* ƽ̨���˯��ʱ��*/
    if (ul_max_pl_sleep_time > ul_max_allow_sleep_time)
    {
        OAM_WARNING_LOG3(pst_dmac_vap->st_vap_base_info.uc_vap_id,OAM_SF_PWR,"{dmac_psm_process_tbtt_sta::deep sleep[%d]light sleep[%d] > allow[%d]}",g_ul_max_deep_sleep_time,g_ul_max_light_sleep_time,ul_max_allow_sleep_time);

        g_ul_max_deep_sleep_time   = 0;
        g_ul_max_light_sleep_time   = 0;

        /* Э��Ĺؼ����� */
        dmac_pm_key_info_dump(pst_dmac_vap);

        /* ƽ̨��˯�߼���*/
        pfn_wlan_dumpsleepcnt();
    }

    if (OAL_TRUE == pst_sta_pm_handle->st_mcast_timer.en_is_registerd)
    {
        OAM_WARNING_LOG3(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR, "{dmac_psm_process_tbtt_sta::timer is registerd,time now[%d],timer[%d],curr[%d]}",
                                (oal_uint32)OAL_TIME_GET_STAMP_MS(),pst_sta_pm_handle->st_mcast_timer.ul_time_stamp,pst_sta_pm_handle->st_mcast_timer.ul_curr_time_stamp);

        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_sta_pm_handle->st_mcast_timer));
    }
    /* wlan timer restart */
    frw_timer_restart();

    /* p2p noa������active״̬ǳ˯,tbtt�жϺ���Ҫȷ���յ���beacon����noaǳ˯ */
    pst_sta_pm_handle->en_beacon_frame_wait         = OAL_TRUE;

    /* pspoll����ģʽ��,����beacon�ȴ���־�����Ŀǰ״̬Ϊdoze���л���awake״̬ */
    if (STA_PWR_SAVE_STATE_DOZE == GET_PM_STATE(pst_sta_pm_handle))
    {
        pst_sta_pm_handle->en_more_data_expected    = OAL_FALSE;
        dmac_pm_sta_post_event(pst_dmac_vap, STA_PWR_EVENT_TBTT, 0, OAL_PTR_NULL);
    }

#ifdef _PRE_WLAN_FEATURE_NO_FRM_INT
    pst_sta_pm_handle->uc_tbtt_cnt_since_full_bcn++;
#endif

#ifdef _PRE_WLAN_DOWNLOAD_PM
    if(g_us_download_rate_limit_pps)
    {
        if ((OAL_TIME_GET_STAMP_MS() - g_ul_drop_time) >= g_uc_max_powersave_limit)
        {
            pst_sta_pm_handle->ul_rx_cnt = 0;
        }
        else
        {
            dmac_pm_sta_post_event(pst_dmac_vap, STA_PWR_EVENT_NOT_EXCEED_MAX_SLP_TIME, 0, OAL_PTR_NULL);
        }
    }
#endif
}


oal_uint32 dmac_send_pspoll_to_ap(dmac_vap_stru *pst_dmac_vap)
{
    oal_netbuf_stru                     *pst_net_buf;
    mac_tx_ctl_stru                     *pst_tx_ctrl;
    oal_uint32                           ul_ret;
    oal_uint16                           us_frame_ctl;
    dmac_user_stru                      *pst_dmac_user;
    mac_ieee80211_pspoll_frame_stru     *pst_pspoll_frame_hdr;

    /* ����net_buff */
    pst_net_buf = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, WLAN_SHORT_NETBUF_SIZE, OAL_NETBUF_PRIORITY_HIGH);
    if (OAL_PTR_NULL == pst_net_buf)
    {
        OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR, "{dmac_send_pspoll_to_ap::alloc netbuff failed.}");
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    oal_set_netbuf_prev(pst_net_buf, OAL_PTR_NULL);
    oal_set_netbuf_next(pst_net_buf, OAL_PTR_NULL);

    pst_pspoll_frame_hdr = (mac_ieee80211_pspoll_frame_stru *)OAL_NETBUF_HEADER(pst_net_buf);

    pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user(pst_dmac_vap->st_vap_base_info.us_assoc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dmac_user))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG,
            "{dmac_send_pspoll_to_ap::pst_dmac_user[%d] null pointer.}", pst_dmac_vap->st_vap_base_info.us_assoc_vap_id);
        oal_netbuf_free(pst_net_buf);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ����pspoll��frame control�� */
    us_frame_ctl  = WLAN_PROTOCOL_VERSION | WLAN_FC0_TYPE_CTL | WLAN_FC0_SUBTYPE_PS_POLL; //�Ͱ�λ
    us_frame_ctl |= (WLAN_FC1_DIR_NODS << 8 ) | (WLAN_FC1_PWR_MGT << 8); //�߰�λ
    mac_hdr_set_frame_control((oal_uint8 *)pst_pspoll_frame_hdr, us_frame_ctl);

    /* Set bits 14 and 15 to 1 when duration field carries Association ID */
    pst_pspoll_frame_hdr->bit_aid_value = WLAN_AID(pst_dmac_vap->st_vap_base_info.us_sta_aid);
    pst_pspoll_frame_hdr->bit_aid_flag1 = OAL_TRUE;
    pst_pspoll_frame_hdr->bit_aid_flag2 = OAL_TRUE;

    /* mode, Address1 = BSSID, Address2 = Source Address (SA) */
    oal_set_mac_addr(pst_pspoll_frame_hdr->auc_bssid, pst_dmac_user->st_user_base_info.auc_user_mac_addr);
    oal_set_mac_addr(pst_pspoll_frame_hdr->auc_trans_addr, mac_mib_get_StationID(&pst_dmac_vap->st_vap_base_info));

    /* ��дcb�ֶ� */
    pst_tx_ctrl = (mac_tx_ctl_stru *)OAL_NETBUF_CB(pst_net_buf);
    OAL_MEMZERO(pst_tx_ctrl, OAL_SIZEOF(mac_tx_ctl_stru));

    /* ��дtx���� */
    MAC_GET_CB_ACK_POLACY(pst_tx_ctrl)   =  WLAN_TX_NORMAL_ACK;
    MAC_GET_CB_EVENT_TYPE(pst_tx_ctrl)         = FRW_EVENT_TYPE_WLAN_DTX;
    MAC_GET_CB_RETRIED_NUM(pst_tx_ctrl)           = 0;
    MAC_GET_CB_WME_TID_TYPE(pst_tx_ctrl)   =  WLAN_TID_FOR_DATA;
    MAC_GET_CB_TX_VAP_INDEX(pst_tx_ctrl)          = pst_dmac_vap->st_vap_base_info.uc_vap_id;
    MAC_GET_CB_TX_USER_IDX(pst_tx_ctrl)           = pst_dmac_vap->st_vap_base_info.us_assoc_vap_id; //sta��ʽ��ap��һ��

    /* ��дtx rx�������� */
    MAC_GET_CB_IS_FROM_PS_QUEUE(pst_tx_ctrl)   = OAL_TRUE;
    MAC_SET_CB_FRAME_HEADER_ADDR(pst_tx_ctrl, (mac_ieee80211_frame_stru *)oal_netbuf_header(pst_net_buf));
    MAC_GET_CB_FRAME_HEADER_LENGTH(pst_tx_ctrl)    = OAL_SIZEOF(mac_ieee80211_frame_stru);
    MAC_GET_CB_MPDU_NUM(pst_tx_ctrl)               = 1;
    MAC_GET_CB_NETBUF_NUM(pst_tx_ctrl)             = 1;
    MAC_GET_CB_MPDU_LEN(pst_tx_ctrl)               = 0;
    MAC_GET_CB_WME_AC_TYPE(pst_tx_ctrl)  =  WLAN_WME_AC_MGMT;

    ul_ret = dmac_tx_mgmt(pst_dmac_vap, pst_net_buf, OAL_SIZEOF(mac_ieee80211_pspoll_frame_stru));
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR,
                 "{dmac_send_pspoll_to_ap failed[%d].}", ul_ret);
        dmac_tx_excp_free_netbuf(pst_net_buf);
        return ul_ret;
    }
    /* Do the protocol specific processing */
    dmac_send_ps_poll_to_ap_prot(pst_dmac_vap);

    return OAL_SUCC;
}

oal_uint32 dmac_send_null_frame_to_ap(dmac_vap_stru *pst_dmac_vap, oal_uint8  uc_psm, oal_bool_enum_uint8 en_qos)
{
    oal_uint32      ul_ret = OAL_SUCC;
#ifdef _PRE_WLAN_FEATURE_STA_UAPSD
    oal_uint32      uc_ac = WLAN_WME_AC_BE; //qos��acĬ��Ϊbe�������ٸ���ҵ����Ҫ��̬��ȡ
#endif
    oal_uint8       en_ps;
    dmac_user_stru  *pst_dmac_user;

#ifdef _PRE_WLAN_FEATURE_STA_UAPSD
	oal_uint8       uc_tid;
#endif
    mac_device_stru     *pst_device;

    if ((OAL_PTR_NULL == pst_dmac_vap) || (uc_psm >= STA_PWR_SAVE_STATE_BUTT))
    {
        OAM_WARNING_LOG2(0, OAM_SF_PWR, "{dmac_send_null_frame_to_ap::pst_dmac_vap [%d], en_psm [%d]}",
                        (oal_uint32)pst_dmac_vap, uc_psm);
        return OAL_FAIL;
    }

    pst_device      = mac_res_get_dev(pst_dmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_device))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{dmac_send_null_frame_to_ap::pst_device[%d] is NULL!}", pst_dmac_vap->st_vap_base_info.uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* DBAC running ������null֡����ʱ�͹��Ĺر�,˯�߶�ʱ������ͣ��,dbac���ٿ��͹��� */
    if(OAL_TRUE == mac_is_dbac_running(pst_device))
    {
        return OAL_SUCC;
    }

    /* ����UP״̬,������null֡ */
    if (MAC_VAP_STATE_UP != pst_dmac_vap->st_vap_base_info.en_vap_state)
    {
        return OAL_FAIL;
    }

    pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user(pst_dmac_vap->st_vap_base_info.us_assoc_vap_id);
    if (OAL_PTR_NULL == pst_dmac_user)
    {
        OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR,
            "{dmac_send_null_frame_to_ap::pst_dmac_user[%d] NULL.}", pst_dmac_vap->st_vap_base_info.us_assoc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ����pmλ�Ƿ���true */
    en_ps = (uc_psm > 0) ? 1 : 0;

    //OAM_WARNING_LOG2(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR,"{dmac_send_null_frame_to_ap::ps:[%d],return addr[0x%x]}", en_ps,__return_address());
    if(OAL_FALSE == en_qos)
    {
        ul_ret = dmac_psm_send_null_data(pst_dmac_vap, pst_dmac_user, en_ps);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR,
                 "{dmac_send_null_frame_to_ap::dmac_psm_send_null_data failed[%d].}", ul_ret);
            return ul_ret;
        }
    }

#ifdef _PRE_WLAN_FEATURE_STA_UAPSD
    else
    {
        uc_tid = dmac_get_highest_trigger_enabled_priority(pst_dmac_vap);
        uc_ac = WLAN_WME_TID_TO_AC(uc_tid);

        ul_ret = dmac_send_qosnull(pst_dmac_vap, pst_dmac_user, (oal_uint8)uc_ac, en_ps);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR,
                 "{dmac_send_null_frame_to_ap::dmac_send_qosnull failed[%d].}", ul_ret);
            return ul_ret;
        }

    }
#endif
    if (mac_mib_get_powermanagementmode(&(pst_dmac_vap->st_vap_base_info)) != WLAN_MIB_PWR_MGMT_MODE_ACTIVE)
    {
        dmac_send_null_frame_to_ap_opt(pst_dmac_vap, uc_psm, en_qos);
    }
    return ul_ret;
}



oal_uint32 dmac_psm_get_timer_restart_cnt(dmac_vap_stru   *pst_dmac_vap)
{
    mac_device_stru   *pst_mac_device = mac_res_get_dev(pst_dmac_vap->st_vap_base_info.uc_device_id);

    /*max sleep disable or scan running or in weak signal envirment */
    if(((MAX_FAST_PS != pst_dmac_vap->uc_cfg_pm_mode )||
        (MAC_SCAN_STATE_RUNNING == pst_mac_device->en_curr_scan_state) ||
       (WLAN_NORMAL_DISTANCE_RSSI_DOWN > oal_get_real_rssi(pst_dmac_vap->st_query_stats.s_signal))||
       (SYS_STATUS_WORK == PM_Driver_GetBfg_Status()))
       &&(g_device_wlan_pm_timeout==DMAC_PSM_TIMER_IDLE_TIMEOUT))
    {
        //OAM_WARNING_LOG3(0, OAM_SF_PWR, "{dmac_psm_get_timer_restart_cnt:sleep mode[%d],scan state[%d],rssi[%d]}",pst_dmac_vap->uc_cfg_pm_mode,pst_mac_device->en_curr_scan_state,oal_get_real_rssi(pst_dmac_vap->st_query_stats.s_signal));
        return DMAC_PSM_TIMER_NORMAL_CNT;
    }
    else
    {
        return g_pm_timer_restart_cnt;
    }

}



oal_uint32 dmac_psm_alarm_callback(void *p_arg)
{
    dmac_vap_stru                   *pst_dmac_vap;
    mac_sta_pm_handler_stru         *pst_sta_pm_handle;
    oal_uint32                       ul_ps_ret = OAL_SUCC;
    oal_uint8                        uc_doze_trans_flag;
    mac_device_stru                 *pst_mac_device;
    oal_uint32                       ul_max_timer_restart = g_pm_timer_restart_cnt;

    pst_dmac_vap                    = (dmac_vap_stru *)(p_arg);

    pst_sta_pm_handle = &pst_dmac_vap->st_sta_pm_handler;

    pst_mac_device  = (mac_device_stru *)mac_res_get_dev(pst_dmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
       OAM_ERROR_LOG0(0, OAM_SF_PWR, "{dmac_psm_alarm_callback::pst_mac_device is null.}");
       return OAL_SUCC;
    }

#ifdef _PRE_WLAN_FEATURE_P2P
    /* p2p���ܿ���,�������pause״̬,������ʱ�� */
    if ((OAL_TRUE == (oal_uint8)IS_P2P_PS_ENABLED(pst_dmac_vap)) && (OAL_TRUE == pst_mac_device->st_p2p_info.en_p2p_ps_pause))
    {
        dmac_psm_start_activity_timer(pst_dmac_vap,pst_sta_pm_handle);
        pst_sta_pm_handle->aul_pmDebugCount[PM_MSG_PSM_RESTART_P2P_PAUSE]++;

        return OAL_SUCC;
    }
#endif

    if ((mac_mib_get_powermanagementmode(&(pst_dmac_vap->st_vap_base_info)) != WLAN_MIB_PWR_MGMT_MODE_PWRSAVE) &&
        (OAL_FALSE == dmac_psm_sta_is_activity_cnt_zero(pst_sta_pm_handle)))
    {
        /* restart ��ʱ�� */
        dmac_psm_start_activity_timer(pst_dmac_vap,pst_sta_pm_handle);
        pst_sta_pm_handle->aul_pmDebugCount[PM_MSG_PSM_TIMEOUT_PM_OFF]++;

        dmac_psm_sta_reset_activity_cnt(pst_sta_pm_handle);

        return OAL_SUCC;
    }

    /* ��ʱ��doze������ */
    uc_doze_trans_flag = (pst_sta_pm_handle->en_beacon_frame_wait) | (pst_sta_pm_handle->st_null_wait.en_doze_null_wait << 1) | (pst_sta_pm_handle->en_more_data_expected << 2)
            | (pst_sta_pm_handle->st_null_wait.en_active_null_wait << 3) | (pst_sta_pm_handle->en_direct_change_to_active << 4);

    /* ���в�Ϊ��(tid + ��vapҲ����ǰ���Ӳ������),restart��ʱ�� */
    if ((OAL_FALSE == dmac_psm_is_tid_queues_empty(pst_dmac_vap)) || (OAL_FALSE == hal_is_hw_tx_queue_empty(pst_dmac_vap->pst_hal_device)))
    {
        dmac_psm_start_activity_timer(pst_dmac_vap,pst_sta_pm_handle);
        pst_sta_pm_handle->aul_pmDebugCount[PM_MSG_PSM_TIMEOUT_QUEUE_NO_EMPTY]++;
    }
#ifdef _PRE_WLAN_DOWNLOAD_PM
    else if(!g_us_download_rate_limit_pps && pst_sta_pm_handle->ul_psm_pkt_cnt)
#else
    else if(pst_sta_pm_handle->ul_psm_pkt_cnt)
#endif
    {
        dmac_psm_start_activity_timer(pst_dmac_vap,pst_sta_pm_handle);
        pst_sta_pm_handle->aul_pmDebugCount[PM_MSG_PSM_TIMEOUT_PKT_CNT]++;
        pst_sta_pm_handle->ul_psm_pkt_cnt = 0;
        pst_sta_pm_handle->uc_psm_timer_restart_cnt = 0; //�а��շ���start count��ͷ��ʼ��
    }
     /* ����Ϊ��,�ֵ͹���ģʽ�ͷǵ͹���KEEPALVIVEģʽ */
    else
    {
        if (WLAN_MIB_PWR_MGMT_MODE_PWRSAVE == (mac_mib_get_powermanagementmode(&(pst_dmac_vap->st_vap_base_info))))
        {
            /* uc_doze_trans_flag����������,����ȴ�uc_psm_timer_restart_cnt*g_device_wlan_pm_timeoutʱ��,ֻ����һ��g_device_wlan_pm_timeout */
            pst_sta_pm_handle->uc_psm_timer_restart_cnt++;

            ul_max_timer_restart = dmac_psm_get_timer_restart_cnt(pst_dmac_vap);

            /* ����ģʽ�µĳ�ʱ����ֻ���ڷ�doze������²����� */
            if (STA_PWR_SAVE_STATE_DOZE != GET_PM_STATE(pst_sta_pm_handle))
            {
                /* �˴�, �ӷǽ��ܵ�active̬�����л������ܵ�doze̬ */
                if((OAL_FALSE == uc_doze_trans_flag) && (pst_sta_pm_handle->uc_psm_timer_restart_cnt >= ul_max_timer_restart))
                {
                    pst_sta_pm_handle->uc_timer_fail_doze_trans_cnt = 0;

                    if (OAL_TRUE == dmac_is_sta_fast_ps_enabled(pst_sta_pm_handle))
                    {
                        ul_ps_ret = dmac_psm_process_fast_ps_state_change(pst_dmac_vap, STA_PWR_SAVE_STATE_DOZE);
                    }
                    else
                    {
                        dmac_pm_sta_post_event(pst_dmac_vap, STA_PWR_EVENT_TIMEOUT, 0, OAL_PTR_NULL);
                    }

                    pst_sta_pm_handle->uc_psm_timer_restart_cnt = 0; //����˯������������0,��ʹnull֡����ʧ�ܣ���һ��Ҳ������ͷ����
                }
                else
                {
                    pst_sta_pm_handle->uc_timer_fail_doze_trans_cnt++;

                    pst_sta_pm_handle->aul_pmDebugCount[PM_MSG_PSM_RESTART_A]++;

                    /* ����activity ��ʱ�� */
                    dmac_psm_start_activity_timer(pst_dmac_vap,pst_sta_pm_handle);
                }
            }
            else
            {
                OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR, "{dmac_psm_alarm_callback::event:[%d]change pm state doze.}", pst_sta_pm_handle->uc_doze_event);
            }
            /* uc_psm_timer_restart_cnt < g_pm_timer_restart_cnt ���쳣�����ӡ */
            if ((DMAC_TIMER_DOZE_TRANS_FAIL_NUM < pst_sta_pm_handle->uc_timer_fail_doze_trans_cnt) && (pst_sta_pm_handle->uc_psm_timer_restart_cnt >= ul_max_timer_restart))
            {
                OAM_WARNING_LOG3(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR, "{dmac_psm_alarm_callback::pm state:[%d],doze trans flag:[%d]timer restart cnt[%d].}", GET_PM_STATE(pst_sta_pm_handle),uc_doze_trans_flag,pst_sta_pm_handle->uc_psm_timer_restart_cnt);
                pst_sta_pm_handle->uc_timer_fail_doze_trans_cnt = 0;
            }
        }

        /* If no data is expected and the STA is not in power save send a NULL frame to AP indicating that STA is alive and active.  */
        /* Restart the activity timer with increased timeout.*/
        else
        {
            if (OAL_TRUE == pst_dmac_vap->st_vap_base_info.st_cap_flag.bit_keepalive)
            {
                ul_ps_ret = dmac_send_null_frame_to_ap(pst_dmac_vap, STA_PWR_SAVE_STATE_ACTIVE, OAL_FALSE);
                dmac_psm_start_activity_timer(pst_dmac_vap,pst_sta_pm_handle);
                pst_sta_pm_handle->aul_pmDebugCount[PM_MSG_PSM_RESTART_B]++;
            }
        }

        if(OAL_SUCC != ul_ps_ret)
        {
            /* fastģʽ�л�ʧ������activity ��ʱ�� */
            pst_sta_pm_handle->aul_pmDebugCount[PM_MSG_PSM_RESTART_C]++;
            dmac_psm_start_activity_timer(pst_dmac_vap,pst_sta_pm_handle);

            return ul_ps_ret;
        }
    }
    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_SINGLE_RF_RX_BCN

oal_void dmac_psm_update_bcn_rf_chain(dmac_vap_stru *pst_dmac_vap, oal_int8 c_rssi)
{
    hal_to_dmac_vap_stru    *pst_hal_vap = pst_dmac_vap->pst_hal_vap;
    hal_to_dmac_device_stru *pst_hal_device = pst_dmac_vap->pst_hal_device;
    oal_uint8                uc_up_vap_num;
    oal_uint8                uc_bcn_rf_chain;

    /* ��ͨ����bcn���عر�ʱ������bcn rx chain */
    if (!pst_hal_device->bit_srb_switch)
    {
        return;
    }

    uc_up_vap_num = hal_device_calc_up_vap_num(pst_hal_device);
    if (0 == uc_up_vap_num)
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR, "dmac_psm_update_bcn_rx_chain:no vap up");
        return;
    }

    if (1 == uc_up_vap_num)
    {
        /* rssi��������ʱʹ������ͨ������beacon������ʹ��˫ͨ������ */
        uc_bcn_rf_chain = c_rssi > DMAC_PSM_SINGLE_BCN_RX_CHAIN_TH ? pst_hal_device->st_cfg_cap_info.uc_single_tx_chain :
                                                                        pst_hal_device->st_cfg_cap_info.uc_rf_chain;
    }
    else
    {
        /* �ǵ�sta������ʹ��phy chain��Ϊbcn rx chain */
        uc_bcn_rf_chain = pst_hal_device->st_cfg_cap_info.uc_rf_chain;
    }

    if (uc_bcn_rf_chain != pst_hal_vap->st_pm_info.uc_bcn_rf_chain)
    {
        hal_pm_set_bcn_rf_chain(pst_hal_vap, uc_bcn_rf_chain);
    }
}
#endif

#endif /*_PRE_WLAN_FEATURE_STA_PM*/
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

