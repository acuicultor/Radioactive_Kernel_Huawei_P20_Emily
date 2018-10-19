


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 ͷ�ļ�����
*****************************************************************************/
#include "oal_net.h"
#include "oam_ext_if.h"
#include "oam_main.h"

#include "wlan_spec.h"
#include "wlan_mib.h"

#include "mac_frame.h"
#include "mac_data.h"
#include "hal_ext_if.h"
#include "dmac_device.h"
#include "dmac_resource.h"
#include "dmac_mgmt_classifier.h"
#include "dmac_blockack.h"
#include "dmac_rx_data.h"
#include "dmac_ext_if.h"
#include "dmac_beacon.h"

#ifdef _PRE_WLAN_CHIP_TEST
#include "dmac_test_main.h"
#include "dmac_test_sch.h"
#endif
#include "dmac_reset.h"

#ifdef _PRE_WLAN_FEATURE_BTCOEX
#include "dmac_btcoex.h"
#endif

#include "oal_profiling.h"

#include "dmac_dfx.h"
#include "dmac_auto_adjust_freq.h"

#ifdef _PRE_WLAN_MAC_BUGFIX_SW_CTRL_RSP
#include "dmac_vap.h"
#endif

#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
#include "hal_mac.h"
#endif

#ifdef _PRE_WLAN_FEATURE_USER_EXTEND
#include "dmac_user_extend.h"
#endif

#ifdef _PRE_WLAN_FEATURE_PACKET_CAPTURE
#include "dmac_pkt_capture.h"
#endif

#ifdef _PRE_WLAN_11K_STAT
#include "dmac_stat.h"
#endif

#ifdef _PRE_WLAN_FEATURE_M2S
#include "dmac_m2s.h"
#endif
#ifdef _PRE_WLAN_FEATURE_FTM
#include "dmac_ftm.h"
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_RX_DATA_C

/*****************************************************************************
  2 ȫ�ֱ�������
*****************************************************************************/
#define MAX_ERR_INFO_LEN               50         /* ָʾ��ӡ�Ĵ�����Ϣ��󳤶� */
oal_uint8          guc_csi_rx_interrupt = 0;


OAL_STATIC oal_uint32 ul_deauth_flow_control_ms = 0;
#define DEAUTH_INTERVAL_MIN 300 /*���Ĺ��˹����У�����ȥ��֤����С��� 300ms*/

/*****************************************************************************
  3 ����ʵ��
*****************************************************************************/


OAL_STATIC oal_uint8 dmac_ant_ch_sel_by_snr(hal_rx_ant_snr_stru *pst_rx_ant_snr)
{
    oal_uint8                  uc_chan = 0xFF;

    /* ANT0 �ϱ���SNR����,�Ҵ���ANT1, ����ͨ��0 */
    if (pst_rx_ant_snr->c_ant0_snr > pst_rx_ant_snr->c_ant1_snr)
    {
        uc_chan = WLAN_PHY_CHAIN_ZERO;
    }
    else
    {
        uc_chan = WLAN_PHY_CHAIN_ONE;
    }

    return uc_chan;

}


OAL_STATIC oal_uint8 dmac_ant_ch_sel_by_rssi(hal_rx_ant_rssi_stru *pst_rx_ant_rssi)
{
    oal_uint8                  uc_chan = 0xFF;
    oal_int8                   c_ant0_rssi;
    oal_int8                   c_ant1_rssi;

    /* ĳ·RSSIδ���� */
    if (OAL_RSSI_INIT_MARKER == pst_rx_ant_rssi->s_ant0_rssi_smth ||
        OAL_RSSI_INIT_MARKER == pst_rx_ant_rssi->s_ant1_rssi_smth)
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "dmac_ant_ch_sel_by_rssi::ant0[%d],ant1[%d]",
                   (oal_int32)pst_rx_ant_rssi->s_ant0_rssi_smth, (oal_int32)pst_rx_ant_rssi->s_ant1_rssi_smth);
        return uc_chan;
    }

    c_ant0_rssi = oal_get_real_rssi(pst_rx_ant_rssi->s_ant0_rssi_smth);
    c_ant1_rssi = oal_get_real_rssi(pst_rx_ant_rssi->s_ant1_rssi_smth);

    /* ANT0 �ϱ���rssi����,�Ҵ���ANT1, ����ͨ��0 */
    if (c_ant0_rssi > c_ant1_rssi)
    {
        uc_chan = WLAN_PHY_CHAIN_ZERO;
    }
    else
    {
        uc_chan = WLAN_PHY_CHAIN_ONE;
    }

    return uc_chan;
}


OAL_STATIC oal_uint8 dmac_ant_channel_select(hal_to_dmac_device_stru *hal_dev)
{
    hal_rx_ant_rssi_stru      *pst_rx_ant_rssi;
    hal_rx_ant_snr_stru       *pst_rx_ant_snr;
    oal_uint8                  uc_rssi_chan = 0xFF;
    oal_uint8                  uc_snr_chan = 0xFF;

    pst_rx_ant_rssi = &hal_dev->st_rssi.st_rx_rssi;
    pst_rx_ant_snr = &hal_dev->st_rssi.st_rx_snr;

    if (OAL_TRUE == pst_rx_ant_rssi->en_ant_rssi_sw)
    {
        uc_rssi_chan = dmac_ant_ch_sel_by_rssi(pst_rx_ant_rssi);
    }

    if (OAL_TRUE == pst_rx_ant_snr->en_ant_snr_sw)
    {
        uc_snr_chan = dmac_ant_ch_sel_by_snr(pst_rx_ant_snr);
    }

    return (0xFF != uc_rssi_chan) ? uc_rssi_chan : uc_snr_chan;
}




OAL_STATIC hal_m2s_state_uint8 dmac_ant_rssi_proc(hal_to_dmac_device_stru *hal_dev, dmac_rx_ctl_stru *pst_rx_cb)
{
    hal_rx_ant_rssi_stru      *pst_rx_ant_rssi;
    oal_uint8                  uc_rssi_abs = 0;
    oal_int8                   c_ant0_rssi;
    oal_int8                   c_ant1_rssi;

    pst_rx_ant_rssi = &hal_dev->st_rssi.st_rx_rssi;

    /* ֻ�е������ź� */
    if (OAL_RSSI_INIT_VALUE == pst_rx_ant_rssi->c_ant0_rssi ||
        OAL_RSSI_INIT_VALUE == pst_rx_ant_rssi->c_ant1_rssi)
    {
        return HAL_M2S_STATE_BUTT;
    }

#ifdef _PRE_WLAN_1103_MPW2_BUGFIX

    /* BUG: phy�ϱ���rssiֵ�����߲�ƥ��, SNRƥ��; ͨ��SNR���ж�RSSI�����ߵ�ƥ���ϵ */
    if ((OAL_SNR_INIT_VALUE == pst_rx_cb->st_rx_statistic.c_snr_ant0) ||
        (OAL_SNR_INIT_VALUE == pst_rx_cb->st_rx_statistic.c_snr_ant1))
    {
        /* BUTT״̬���л� */
        return HAL_M2S_STATE_BUTT;
    }

    if (OAL_TRUE == pst_rx_ant_rssi->en_log_print)
    {
        OAM_ERROR_LOG3(0, OAM_SF_DBDC, "dmac_ant_rssi_proc::rx rssi[%d],snr0[%d],snr1[%d]",
           pst_rx_cb->st_rx_statistic.c_rssi_dbm, pst_rx_cb->st_rx_statistic.c_snr_ant0, pst_rx_cb->st_rx_statistic.c_snr_ant1);
        OAM_ERROR_LOG2(0, OAM_SF_DBDC, "dmac_ant_rssi_proc:: switch ant0[%d] ant1[%d]",
                 pst_rx_ant_rssi->c_ant0_rssi,pst_rx_ant_rssi->c_ant1_rssi);
    }

    c_ant0_rssi = pst_rx_ant_rssi->c_ant0_rssi;
    c_ant1_rssi = pst_rx_ant_rssi->c_ant1_rssi;

    if (pst_rx_cb->st_rx_statistic.c_snr_ant0 > pst_rx_cb->st_rx_statistic.c_snr_ant1)
    {
        pst_rx_ant_rssi->c_ant0_rssi = OAL_MAX(c_ant0_rssi, c_ant1_rssi);
        pst_rx_ant_rssi->c_ant1_rssi = OAL_MIN(c_ant0_rssi, c_ant1_rssi);
    }
    else
    {
        pst_rx_ant_rssi->c_ant0_rssi = OAL_MIN(c_ant0_rssi, c_ant1_rssi);
        pst_rx_ant_rssi->c_ant1_rssi = OAL_MAX(c_ant0_rssi, c_ant1_rssi);
    }

#endif


    oal_rssi_smooth(&pst_rx_ant_rssi->s_ant0_rssi_smth, pst_rx_ant_rssi->c_ant0_rssi);
    oal_rssi_smooth(&pst_rx_ant_rssi->s_ant1_rssi_smth, pst_rx_ant_rssi->c_ant1_rssi);

    c_ant0_rssi = oal_get_real_rssi(pst_rx_ant_rssi->s_ant0_rssi_smth);
    c_ant1_rssi = oal_get_real_rssi(pst_rx_ant_rssi->s_ant1_rssi_smth);

    uc_rssi_abs = (oal_uint8)OAL_ABSOLUTE_SUB(c_ant0_rssi, c_ant1_rssi);

    if (OAL_TRUE == pst_rx_ant_rssi->en_log_print)
    {
        OAM_ERROR_LOG3(0, OAM_SF_CFG, "dmac_ant_rssi_proc::c_rssi_abs[%d],ant0[%d],ant1[%d]",
                           uc_rssi_abs,(oal_int32)c_ant0_rssi,(oal_int32)c_ant1_rssi);
    }

    /* MIMO״̬�ж��Ƿ���Ҫ�л���SISO */
    if (HAL_M2S_STATE_MIMO == GET_HAL_M2S_CUR_STATE(hal_dev))
    {
        /* ˫����RSSI��С����ֵ�򲻴��� */
        /* ����˫���߾��쳣,Ҳ�ڴ˴�����,������ */
        if (uc_rssi_abs < pst_rx_ant_rssi->uc_rssi_high)
        {
            pst_rx_ant_rssi->uc_rssi_high_cnt = 0;
            return HAL_M2S_STATE_MIMO;
        }

        /* �������rssi������ֵ���л�SISO */
        pst_rx_ant_rssi->uc_rssi_high_cnt++;
        if (pst_rx_ant_rssi->uc_rssi_high_cnt > pst_rx_ant_rssi->uc_rssi_high_cnt_th)
        {
            pst_rx_ant_rssi->uc_rssi_high_cnt = 0;
            return HAL_M2S_STATE_SISO;
        }

        /* ����MIMO */
        return HAL_M2S_STATE_MIMO;
    }
    /* MISO״̬,�����ֵ�ﵽ�ָ�MIMO�������л���MIMO */
    else
    {
        /* RSSI��ֵδ�ָ�����������SISO */
        if (uc_rssi_abs > pst_rx_ant_rssi->uc_rssi_low)
        {
            pst_rx_ant_rssi->uc_rssi_low_cnt = 0;
            return HAL_M2S_STATE_SISO;
        }

        /* �������rssi��ֵС����ֵ��ָ�MIMO */
        pst_rx_ant_rssi->uc_rssi_low_cnt++;
        if (pst_rx_ant_rssi->uc_rssi_low_cnt > pst_rx_ant_rssi->uc_rssi_low_cnt_th)
        {
            pst_rx_ant_rssi->uc_rssi_low_cnt = 0;
            return HAL_M2S_STATE_MIMO;
        }

        /* ����MISO */
        return HAL_M2S_STATE_MISO;
    }

}


OAL_STATIC hal_m2s_state_uint8 dmac_ant_snr_proc(hal_to_dmac_device_stru *hal_dev, dmac_rx_ctl_stru *pst_rx_cb)
{
    hal_rx_ant_snr_stru       *pst_rx_ant_snr;
    oal_uint8                  uc_snr_abs = 0;

    if ((OAL_SNR_INIT_VALUE == pst_rx_cb->st_rx_statistic.c_snr_ant0) ||
        (OAL_SNR_INIT_VALUE == pst_rx_cb->st_rx_statistic.c_snr_ant1))
    {
        /* BUTT״̬���л� */
        return HAL_M2S_STATE_BUTT;
    }

    pst_rx_ant_snr = &hal_dev->st_rssi.st_rx_snr;

    pst_rx_ant_snr->c_ant0_snr = pst_rx_cb->st_rx_statistic.c_snr_ant0;
    pst_rx_ant_snr->c_ant1_snr = pst_rx_cb->st_rx_statistic.c_snr_ant1;

    uc_snr_abs = (oal_uint8)OAL_ABSOLUTE_SUB(pst_rx_ant_snr->c_ant0_snr, pst_rx_ant_snr->c_ant1_snr);

    if (OAL_TRUE == pst_rx_ant_snr->en_log_print)
    {
        OAM_ERROR_LOG3(0, OAM_SF_CFG, "dmac_ant_snr_proc::uc_snr_abs[%d],ant0[%d],ant1[%d]",
                    uc_snr_abs,(oal_int32)pst_rx_ant_snr->c_ant0_snr,(oal_int32)pst_rx_ant_snr->c_ant1_snr);
    }

    /* MIMO״̬�ж��Ƿ���Ҫ�л���SISO */
    if (HAL_M2S_STATE_MIMO == GET_HAL_M2S_CUR_STATE(hal_dev))
    {
        if (uc_snr_abs < pst_rx_ant_snr->uc_snr_high)
        {
            pst_rx_ant_snr->uc_snr_high_cnt = 0;
            return HAL_M2S_STATE_MIMO;
        }

        /* �������rssi������ֵ���л�SISO */
        pst_rx_ant_snr->uc_snr_high_cnt++;
        if (pst_rx_ant_snr->uc_snr_high_cnt > pst_rx_ant_snr->uc_snr_high_cnt_th)
        {
            pst_rx_ant_snr->uc_snr_high_cnt = 0;
            return HAL_M2S_STATE_SISO;
        }

        return HAL_M2S_STATE_MIMO;
    }
    /* MISO״̬,�����ֵ�ﵽ�ָ�MIMO�������л���MIMO */
    else
    {
        /* SNR��ֵδ�ָ�����������SISO */
        if (uc_snr_abs > pst_rx_ant_snr->uc_snr_low)
        {
            pst_rx_ant_snr->uc_snr_low_cnt = 0;
            return HAL_M2S_STATE_SISO;
        }

        /* �������SNR��ֵС����ֵ��ָ�MIMO */
        pst_rx_ant_snr->uc_snr_low_cnt++;
        if (pst_rx_ant_snr->uc_snr_low_cnt > pst_rx_ant_snr->uc_snr_low_cnt_th)
        {
            pst_rx_ant_snr->uc_snr_low_cnt = 0;
            return HAL_M2S_STATE_MIMO;
        }

        return HAL_M2S_STATE_MISO;
    }

}


hal_m2s_state_uint8 dmac_ant_state_proc(hal_to_dmac_device_stru *hal_dev)
{
    dmac_rx_ctl_stru               *pst_rx_cb;
    hal_m2s_state_uint8             uc_rssi_det_state = HAL_M2S_STATE_BUTT;
    hal_m2s_state_uint8             uc_snr_det_state = HAL_M2S_STATE_BUTT;
    hal_m2s_state_uint8             uc_next_m2s_state = HAL_M2S_STATE_BUTT;
    hal_m2s_state_uint8             uc_cur_m2s_state = HAL_M2S_STATE_BUTT;

    pst_rx_cb = (dmac_rx_ctl_stru*)hal_dev->st_rssi.pst_cb;

    /* RSSI detct enable */
    if (OAL_TRUE == hal_dev->st_rssi.st_rx_rssi.en_ant_rssi_sw)
    {
        uc_rssi_det_state = dmac_ant_rssi_proc(hal_dev, pst_rx_cb);
    }

    /* SNR detct enable */
    if (OAL_TRUE == hal_dev->st_rssi.st_rx_snr.en_ant_snr_sw)
    {
        uc_snr_det_state = dmac_ant_snr_proc(hal_dev, pst_rx_cb);
    }

    /* ��û��ʹ�����˳� */
    if ((HAL_M2S_STATE_BUTT == uc_rssi_det_state) && (HAL_M2S_STATE_BUTT == uc_snr_det_state))
    {
        return HAL_M2S_STATE_BUTT;
    }

    uc_cur_m2s_state = GET_HAL_M2S_CUR_STATE(hal_dev);

    /* ��һ��̽�����뵱ǰ״̬һ��,���л� */
    if ((uc_rssi_det_state == uc_cur_m2s_state) || (uc_snr_det_state == uc_cur_m2s_state))
    {
        return HAL_M2S_STATE_BUTT;
    }

    uc_next_m2s_state = (HAL_M2S_STATE_BUTT != uc_rssi_det_state) ? uc_rssi_det_state : uc_snr_det_state;


    /* �Ƿ���RSSI/SNR�����л���SISO */
    if (HAL_M2S_STATE_MIMO == uc_cur_m2s_state && HAL_M2S_STATE_SISO == uc_next_m2s_state)
    {
        GET_DEV_RSSI_TRIGGER(hal_dev) = OAL_TRUE;
    }

    return uc_next_m2s_state;
}


oal_bool_enum_uint8 dmac_ant_rssi_need_hw_set(hal_to_dmac_device_stru *hal_dev)
{
    oal_uint8                  uc_up_vap_num = 0;
    oal_uint8                  uc_vap_index  = 0;
    oal_uint8                  auc_mac_vap_id[WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE] = {0};
    mac_vap_stru              *pst_mac_vap;
    oal_bool_enum_uint8        en_hw_switch = OAL_FALSE;

    /*  ���չ��ʴ������л�,��Ҫ�ж�:
        1. 2.4G�л�Ϊ��ͨ���շ�
        2. 5G ͨ��˫ͨ������*/
    if (OAL_TRUE == GET_DEV_RSSI_TRIGGER(hal_dev))
    {
        /* ���ʿ����л�ΪSISOʱ��ѯhal dev���Ƿ���2G vap, ������Ҫ�л�Ϊ��ͨ�� */
        uc_up_vap_num = hal_device_find_all_up_vap(hal_dev, auc_mac_vap_id);
        for (uc_vap_index = 0; uc_vap_index < uc_up_vap_num; uc_vap_index++)
        {
            pst_mac_vap  = (mac_vap_stru *)mac_res_get_mac_vap(auc_mac_vap_id[uc_vap_index]);
            if (OAL_PTR_NULL == pst_mac_vap)
            {
                OAM_ERROR_LOG0(auc_mac_vap_id[uc_vap_index], OAM_SF_ANY, "dmac_ant_rssi_need_hw_set::pst_mac_vap IS NULL.");
                continue;
            }

            if (WLAN_BAND_2G == pst_mac_vap->st_channel.en_band)
            {
                en_hw_switch = OAL_TRUE;
                break;
            }
        }
    }

    OAM_WARNING_LOG1(0, OAM_SF_ANY, "{dmac_ant_rssi_need_hw_set:open only one chan[%d][1:yes,0:no].}",en_hw_switch);

    return en_hw_switch;
}


oal_void dmac_rssi_event(hal_to_dmac_device_stru *hal_dev, hal_m2s_state_uint8 uc_new_state)
{
    oal_bool_enum_uint8          en_switch = OAL_TRUE;
    oal_uint8                    uc_chan;
#ifdef _PRE_WLAN_FEATURE_M2S
    hal_m2s_event_tpye_uint16    en_event_type = HAL_M2S_EVENT_ANT_RSSI_MISO_TO_C0_SISO;
#endif

    OAM_WARNING_LOG2(0, OAM_SF_ANY, "{dmac_rssi_event:cur state[%d]next state[%d].}",
                                       GET_HAL_M2S_CUR_STATE(hal_dev), uc_new_state);

    /* �л���SISO��Ҫ�ж��Ƿ�ر�ͨ�� */
    if (HAL_M2S_STATE_SISO == uc_new_state)
    {
        en_switch = dmac_ant_rssi_need_hw_set(hal_dev);
    }

    /* ���չ��ʾ�������ָ���MIMO״̬ */
    if (HAL_M2S_STATE_MIMO == uc_new_state)
    {
        /* �ָ�MIMO�϶��Ǵ�MISO״̬�ָ� */
        if  (HAL_M2S_STATE_MISO != GET_HAL_M2S_CUR_STATE(hal_dev))
        {
            OAM_ERROR_LOG2(0, OAM_SF_ANY, "{dmac_rssi_event:cur state[%d], next state[%d].}", GET_HAL_M2S_CUR_STATE(hal_dev), uc_new_state);
            return;
        }

        /* MIMO״̬�ָ� */
        GET_DEV_RSSI_TRIGGER(hal_dev) = OAL_FALSE;
        GET_DEV_RSSI_MISO_HOLD(hal_dev) = OAL_FALSE;
        /* ����,��ʱ��̽�� */
        GET_DEV_RSSI_MIMO_HOLD(hal_dev) = OAL_TRUE;

#ifdef _PRE_WLAN_FEATURE_M2S
        dmac_m2s_handle_event(hal_dev, HAL_M2S_EVENT_ANT_RSSI_MISO_TO_MIMO, 0, OAL_PTR_NULL);
#endif
    }
    else
    {
        GET_DEV_RSSI_MIMO_HOLD(hal_dev) = OAL_FALSE;

        /* ������л���siso��ѡ����Ҫ�򿪵�ͨ�� */
        uc_chan = dmac_ant_channel_select(hal_dev);

#ifdef _PRE_WLAN_FEATURE_M2S
        /* ����PHY0 ͨ��1 RF1 */
        if (WLAN_PHY_CHAIN_ONE == uc_chan)
        {
            en_event_type = HAL_M2S_EVENT_ANT_RSSI_MISO_TO_C1_SISO;
        }
        else
        {
            en_event_type = HAL_M2S_EVENT_ANT_RSSI_MISO_TO_C0_SISO;
        }
#endif

        /* MISO������SISO */
        if (HAL_M2S_STATE_MISO == GET_HAL_M2S_CUR_STATE(hal_dev))
        {
            /* MISO�л���SISO��Ҫ�ж��Ƿ���2G�豸 */
            if (OAL_TRUE == en_switch)
            {
                /* ��������MISO״̬ */
                GET_DEV_RSSI_MISO_HOLD(hal_dev) = OAL_FALSE;

#ifdef _PRE_WLAN_FEATURE_M2S
                dmac_m2s_handle_event(hal_dev, en_event_type, 0, OAL_PTR_NULL);
#endif
            }
            else
            {
                /* ������MISO״̬ */
                GET_DEV_RSSI_MISO_HOLD(hal_dev) = OAL_TRUE;
            }
        }
        /* MIMO��SISO��Ҫ�л�����״̬ */
        else if (HAL_M2S_STATE_MIMO == GET_HAL_M2S_CUR_STATE(hal_dev))
        {
            /* ��Ҫ�л���ͨ�� */
            if (OAL_TRUE == en_switch)
            {
                /* ��������MISO״̬ */
                GET_DEV_RSSI_MISO_HOLD(hal_dev) = OAL_FALSE;

                OAM_WARNING_LOG1(0, OAM_SF_ANY, "{dmac_rssi_event:only open phy chan[%d].}",uc_chan);

#ifdef _PRE_WLAN_FEATURE_M2S
                dmac_m2s_handle_event(hal_dev, HAL_M2S_EVENT_ANT_RSSI_MIMO_TO_MISO, 0, OAL_PTR_NULL);

                dmac_m2s_handle_event(hal_dev, en_event_type, 0, OAL_PTR_NULL);
#endif
            }
            else
            {
                /* ������MISO״̬ */
                GET_DEV_RSSI_MISO_HOLD(hal_dev) = OAL_TRUE;

#ifdef _PRE_WLAN_FEATURE_M2S
                dmac_m2s_handle_event(hal_dev, HAL_M2S_EVENT_ANT_RSSI_MIMO_TO_MISO, 0, OAL_PTR_NULL);
#endif
            }
        }
        else
        {
            OAM_ERROR_LOG2(0, OAM_SF_ANY, "{dmac_rssi_event:cur state[%d], next state[%d].}", GET_HAL_M2S_CUR_STATE(hal_dev), uc_new_state);
        }
    }
#if 0
    if (HAL_M2S_STATE_SISO == uc_new_state)
    {
        hal_dev->st_rssi.st_rx_rssi.s_ant0_rssi_smth = OAL_RSSI_INIT_MARKER;
        hal_dev->st_rssi.st_rx_rssi.s_ant1_rssi_smth = OAL_RSSI_INIT_MARKER;
    }
#endif
}


oal_void dmac_ant_rssi_notify(hal_to_dmac_device_stru *hal_dev)
{
    hal_m2s_state_uint8       uc_next_m2s_state = HAL_M2S_STATE_BUTT;

    uc_next_m2s_state = dmac_ant_state_proc(hal_dev);
    if (HAL_M2S_STATE_BUTT == uc_next_m2s_state)
    {
        return;
    }

    dmac_rssi_event(hal_dev, uc_next_m2s_state);
}


oal_void dmac_ant_rx_frame(hal_to_dmac_device_stru *pst_hal_dev, dmac_rx_ctl_stru *pst_cb_ctrl)
{
    /* û��ʹ�����˳� */
    if ((OAL_TRUE != pst_hal_dev->st_rssi.st_rx_rssi.en_ant_rssi_sw) &&
        (OAL_TRUE != pst_hal_dev->st_rssi.st_rx_snr.en_ant_snr_sw))
    {
        return;
    }

    /* �����һ֡������ */
    if (OAL_TRUE != pst_cb_ctrl->st_rx_status.bit_last_mpdu_flag)
    {
        return;
    }

    /* SISO״̬������RSSI/SNR */
    if (HAL_M2S_STATE_SISO == GET_HAL_M2S_CUR_STATE(pst_hal_dev))
    {
        return;
    }

    /* ��MISO̬����,������̽�� */
    if (OAL_TRUE == GET_DEV_RSSI_MISO_HOLD(pst_hal_dev) || OAL_TRUE == GET_DEV_RSSI_MIMO_HOLD(pst_hal_dev))
    {
        return;
    }

    dmac_ant_rssi_notify(pst_hal_dev);
}


OAL_STATIC oal_void dmac_ant_tbtt_notify(hal_to_dmac_device_stru *hal_dev)
{
    /* MIMO״̬ */
    if (HAL_M2S_STATE_MIMO == GET_HAL_M2S_CUR_STATE(hal_dev))
    {
        GET_DEV_RSSI_MIMO_TBTT_CNT(hal_dev)++;

        if (GET_DEV_RSSI_MIMO_TBTT_CNT(hal_dev) > GET_DEV_RSSI_MIMO_TBTT_OPEN_TH(hal_dev)+ GET_DEV_RSSI_MIMO_TBTT_CLOSE_TH(hal_dev))
        {
            GET_DEV_RSSI_MIMO_TBTT_CNT(hal_dev) = 0;

            /* ÿ��һ������MIMO״ֹ̬ͣ̽�� */
             GET_DEV_RSSI_MIMO_HOLD(hal_dev) = OAL_TRUE;
        }

        if (GET_DEV_RSSI_MIMO_TBTT_CNT(hal_dev) > GET_DEV_RSSI_MIMO_TBTT_OPEN_TH(hal_dev))
        {
            /* ÿ��һ������MIMO״̬����̽�� */
            GET_DEV_RSSI_MIMO_HOLD(hal_dev) = OAL_FALSE;
        }
    }
    else
    {
        /* MIMOδ�����л���SISO�򲻴��� */
        if (OAL_TRUE != GET_DEV_RSSI_TRIGGER(hal_dev))
        {
            return;
        }

        GET_DEV_RSSI_TBTT_CNT(hal_dev)++;
        if (GET_DEV_RSSI_TBTT_CNT(hal_dev) > GET_DEV_RSSI_TBTT_TH(hal_dev))
        {
            GET_DEV_RSSI_TBTT_CNT(hal_dev) = 0;

            /* MISO״̬�յ�MISO�¼�, ����̽��ģʽ  */
            if (HAL_M2S_STATE_MISO == GET_HAL_M2S_CUR_STATE(hal_dev))
            {
                GET_DEV_RSSI_MISO_HOLD(hal_dev) = OAL_FALSE;
            }
            else
            {
#ifdef _PRE_WLAN_FEATURE_M2S
                /* ÿ��һ�������л���MISO״̬̽�� */
                dmac_m2s_handle_event(hal_dev, HAL_M2S_EVENT_ANT_RSSI_SISO_TO_MISO, 0, OAL_PTR_NULL);
#endif
            }
        }
    }
}


oal_void dmac_ant_tbtt_process(hal_to_dmac_device_stru *pst_hal_dev)
{
    /* û��ʹ�����˳� */
    if ((OAL_TRUE != pst_hal_dev->st_rssi.st_rx_rssi.en_ant_rssi_sw) &&
        (OAL_TRUE != pst_hal_dev->st_rssi.st_rx_snr.en_ant_snr_sw))
    {
        return;
    }

    /* ��·������ */
    if (OAL_FALSE == pst_hal_dev->en_is_master_hal_device)
    {
        return;
    }

    dmac_ant_tbtt_notify(pst_hal_dev);
}


#ifdef _PRE_WLAN_DFT_STAT
OAL_STATIC oal_uint32  dmac_rx_vap_stat(dmac_vap_stru *pst_dmac_vap,
                                            oal_netbuf_head_stru *pst_netbuf_hdr,
                                            oal_netbuf_stru     *pst_buf,
                                            dmac_rx_ctl_stru *pst_rx_cb)
{
    oal_uint32          *pul_mac_hdr;
    oal_uint8            uc_buf_nums;
    oal_netbuf_stru     *pst_buf_next;
    oal_uint8            uc_msdu_nums_in_mpdu = 0;
    oal_uint16           us_bytes;
    mac_rx_ctl_stru     *pst_rx_info = &(pst_rx_cb->st_rx_info);
    mac_ieee80211_frame_stru  *pst_frame_hdr;

    pul_mac_hdr = mac_get_rx_cb_mac_hdr(pst_rx_info);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pul_mac_hdr))
    {
        OAM_ERROR_LOG0(0, OAM_SF_RX, "{dmac_rx_vap_stat::mac_hdr is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_frame_hdr  = (mac_ieee80211_frame_stru *)(mac_get_rx_cb_mac_hdr(pst_rx_info));

    if (!pst_rx_info->bit_amsdu_enable)
    {
        /* ��mpduֻ��һ��msdu���ֽ���Ӧ��ȥ��snap���� */
        if (WLAN_DATA_BASICTYPE == pst_frame_hdr->st_frame_control.bit_type)
        {
            DMAC_VAP_STATS_PKT_INCR(pst_dmac_vap->st_query_stats.ul_drv_rx_pkts,  1);
#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
            DMAC_VAP_STATS_PKT_INCR(pst_dmac_vap->st_query_stats.ul_drv_rx_bytes,
                                    (pst_rx_cb->st_rx_info.us_frame_len - pst_rx_cb->st_rx_info.uc_mac_header_len) - SNAP_LLC_FRAME_LEN);
#else
            DMAC_VAP_STATS_PKT_INCR(pst_dmac_vap->st_query_stats.ul_drv_rx_bytes,
                                    (pst_rx_cb->st_rx_info.us_frame_len - pst_rx_cb->st_rx_info.uc_mac_header_len) - SNAP_LLC_FRAME_LEN);

#endif
        }
    }
    /* �����mpdu������amsdu����Ҫ��ȡsub_msdu������ÿһ��sub_msdu���ֽ��� */
    else
    {

        for (uc_buf_nums = 0; uc_buf_nums < pst_rx_cb->st_rx_info.bit_buff_nums; uc_buf_nums++)
        {
            pst_buf_next = oal_get_netbuf_next(pst_buf);

            /* ����mpdu��msdu����Ŀ */
            uc_msdu_nums_in_mpdu += pst_rx_cb->st_rx_info.uc_msdu_in_buffer;

            pst_buf = pst_buf_next;
            if ((oal_netbuf_head_stru *)pst_buf == pst_netbuf_hdr)
            {
                break;
            }
            pst_rx_cb = (dmac_rx_ctl_stru *)OAL_NETBUF_CB(pst_buf);
        }
        if (WLAN_DATA_BASICTYPE == pst_frame_hdr->st_frame_control.bit_type)
        {
            DMAC_VAP_STATS_PKT_INCR(pst_dmac_vap->st_query_stats.ul_drv_rx_pkts, uc_msdu_nums_in_mpdu);
#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
            /* �����mpdu���ֽ���Ŀ������sub_msdt�ֽ���֮�ͣ�������snap��������padding��ע�����һ��sub_msduû��padding */
            us_bytes = (pst_rx_cb->st_rx_info.us_frame_len - pst_rx_cb->st_rx_info.uc_mac_header_len)
                     - SNAP_LLC_FRAME_LEN * uc_msdu_nums_in_mpdu;
#else
            /* �����mpdu���ֽ���Ŀ������sub_msdt�ֽ���֮�ͣ�������snap��������padding��ע�����һ��sub_msduû��padding */
            us_bytes = pst_rx_cb->st_rx_info.us_frame_len - pst_rx_cb->st_rx_info.uc_mac_header_len
                     - SNAP_LLC_FRAME_LEN * uc_msdu_nums_in_mpdu;

#endif
            DMAC_VAP_STATS_PKT_INCR(pst_dmac_vap->st_query_stats.ul_drv_rx_bytes, us_bytes);
        }

    }

    return OAL_SUCC;
}
#endif

#if (_PRE_OS_VERSION_RAW == _PRE_OS_VERSION)  && defined (__CC_ARM)
#pragma arm section rwdata = "BTCM", code ="ATCM", zidata = "BTCM", rodata = "ATCM"
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

OAL_STATIC oal_uint32  dmac_rx_update_dscr_queue(
                hal_to_dmac_device_stru            *pst_device,
                hal_hw_rx_dscr_info_stru           *pst_rx_dscr_info,
                oal_netbuf_head_stru               *pst_netbuf_header,
                oal_uint16                         *pus_pre_dscr_num)
{
    dmac_rx_ctl_stru               *pst_cb_ctrl    = OAL_PTR_NULL;                    /* ��дnetbuf��cb�ֶ�ʹ�� */
    oal_netbuf_stru                *pst_netbuf     = OAL_PTR_NULL;      /* ָ��ÿһ����������Ӧ��netbuf */
    oal_dlist_head_stru            *pst_dscr_entry = OAL_PTR_NULL;
    oal_dlist_head_stru            *pst_temp       = OAL_PTR_NULL;
    hal_rx_dscr_stru               *pst_curr_dscr  = OAL_PTR_NULL;                  /* ���ڴ���������� */
    oal_bool_enum_uint8             en_last_int_flag = OAL_FALSE;   /* ����ϱ����жϵı�ʶ OAL_TRUE ����ʼ�������ж�*/
    oal_uint16                      us_dscr_num;
    hal_rx_dscr_queue_id_enum_uint8 en_queue_num;
    oal_uint8                       *puc_mac_hdr_addr;
    oal_uint8                       *puc_tail;
    oal_uint16                      us_frame_len;
    oal_uint8                       uc_sub_type;
    oam_sdt_print_beacon_rxdscr_type_enum_uint8           en_beacon_switch;
    oal_uint32                      ul_rx_dscr_len;
#ifdef _PRE_WLAN_FEATURE_PACKET_CAPTURE
    dmac_device_stru               *pst_dmac_device;
    oal_bool_enum_uint8             en_is_ampdu = OAL_FALSE;
    oal_int8                        c_rssi_ampdu = 0;
    oal_uint32                      ul_ret;
#endif

    us_dscr_num = pst_rx_dscr_info->us_dscr_num;
    en_queue_num = pst_rx_dscr_info->uc_queue_id;

#ifdef _PRE_DEBUG_MODE
    /* ȡ�����е�һ�������� */
    pst_dscr_entry = pst_device->ast_rx_dscr_queue[pst_rx_dscr_info->uc_queue_id].st_header.pst_next;
    pst_curr_dscr = OAL_DLIST_GET_ENTRY(pst_dscr_entry, hal_rx_dscr_stru, st_entry);

    if (pst_curr_dscr != (hal_rx_dscr_stru*)(pst_rx_dscr_info->pul_base_dscr))
    {
        pst_device->ul_rx_irq_loss_cnt++;
        OAM_WARNING_LOG1(0, OAM_SF_RX, "{dmac_rx_update_dscr_queue::Losed interrupt[%d].", pst_device->ul_rx_irq_loss_cnt);
    }
#endif

    OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_dscr_entry, pst_temp, &(pst_device->ast_rx_dscr_queue[en_queue_num].st_header))
    {
        /* ������ͷ�����һ����Ҫ����������� */
        pst_curr_dscr = OAL_DLIST_GET_ENTRY(pst_dscr_entry, hal_rx_dscr_stru, st_entry);

        /* �ж��Ƿ��Ѿ���ʼ�����жϴ��� */
        if (pst_curr_dscr == (hal_rx_dscr_stru*)(pst_rx_dscr_info->pul_base_dscr))
        {
            en_last_int_flag = OAL_TRUE;
        }

        //OAM_ERROR_LOG4(0, OAM_SF_RX, "{dmac_rx_update_dscr_queue::RX_NEW SOFTWARE!dscr addr=0x%x,real=0x%x,base dscr addr=0x%08x,real=0x%x.}",
        //pst_curr_dscr, HAL_RX_DSCR_GET_REAL_ADDR((oal_uint32*)pst_curr_dscr),pst_rx_dscr_info->pul_base_dscr,HAL_RX_DSCR_GET_REAL_ADDR(pst_rx_dscr_info->pul_base_dscr));

        /* ��ȡ��������Ӧ��netbuf */
        hal_rx_get_netbuffer_addr_dscr(pst_device, (oal_uint32*)pst_curr_dscr, &pst_netbuf);

        pst_cb_ctrl = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);

        /* ��ȡ���������ֶ���Ϣ��������ӵ�netbuf��cb�ֶ��� */
        dmac_rx_get_dscr_info(pst_device, (oal_uint32*)pst_curr_dscr, pst_cb_ctrl);

#if defined _PRE_WLAN_PRODUCT_1151V200
        /*����Ǳ����ж�,AMPDU��֡���������ж�����������*/
        if (OAL_TRUE == en_last_int_flag)
        {
             pst_cb_ctrl->st_rx_statistic.us_mpdu_num = pst_rx_dscr_info->us_dscr_num;
        }
#endif
        /* ��cb�е��ŵ��Ž��и�ֵ */
        pst_cb_ctrl->st_rx_info.uc_channel_number = pst_rx_dscr_info->uc_channel_num;

        dmac_ant_rx_frame(pst_device, pst_cb_ctrl);

        /* �Ƿ�RX NEW */
        if (HAL_RX_NEW == pst_cb_ctrl->st_rx_status.bit_dscr_status)
        {

            if (OAL_TRUE == en_last_int_flag)
            {
                /* Ӳ��RX_NEW */
                OAM_ERROR_LOG4(0, OAM_SF_RX, "{dmac_rx_update_dscr_queue::RX_NEW HARDWARE!dscr addr=0x%08x,base dscr addr=0x%08x,cnt=%d,q=%d.}",
                               pst_curr_dscr, HAL_RX_DSCR_GET_SW_ADDR(pst_rx_dscr_info->pul_base_dscr), us_dscr_num, en_queue_num);
            }
            else
            {
                /* ���RX_NEW */
                OAM_ERROR_LOG4(0, OAM_SF_RX, "{dmac_rx_update_dscr_queue::RX_NEW SOFTWARE!dscr addr=0x%08x,base dscr addr=0x%08x,cnt=%d,q=%d.}",
                                pst_curr_dscr, HAL_RX_DSCR_GET_SW_ADDR(pst_rx_dscr_info->pul_base_dscr), us_dscr_num, en_queue_num);
            }
        }

#ifdef _PRE_WLAN_CACHE_COHERENT_SUPPORT
        /* ��Ч��cache�����¶�ȡnetbuf������ */
        oal_dma_map_single(NULL, pst_netbuf->data, pst_cb_ctrl->st_rx_info.us_frame_len, OAL_FROM_DEVICE);
#endif

        puc_mac_hdr_addr = oal_netbuf_header(pst_netbuf);
        us_frame_len = pst_cb_ctrl->st_rx_info.us_frame_len;
        oal_netbuf_set_len(pst_netbuf, us_frame_len);

        if (us_frame_len < WLAN_LARGE_NETBUF_SIZE)
        {
            puc_tail = puc_mac_hdr_addr + us_frame_len;
        }
        else
        {
            puc_tail = puc_mac_hdr_addr + WLAN_LARGE_NETBUF_SIZE;
        }

        oal_set_netbuf_tail(pst_netbuf, puc_tail);

        /* ���뵽netbuf�������� */
        oal_netbuf_add_to_list_tail(pst_netbuf, pst_netbuf_header);

        /* phy_debug info��ӡ */
        hal_rx_print_phy_debug_info(pst_device,(oal_uint32 *)pst_curr_dscr, &pst_cb_ctrl->st_rx_statistic);

#ifdef _PRE_WLAN_FEATURE_ALWAYS_TX
        /* ���հ�ͳ�� */
        hal_rx_record_frame_status_info(pst_device, (oal_uint32 *)pst_curr_dscr, en_queue_num);
#endif

#ifdef _PRE_WLAN_FEATURE_MU_TRAFFIC_CTL
        /* �㷨ͳ��rx�������� */
        if (HAL_RX_DSCR_NORMAL_PRI_QUEUE == en_queue_num)
        {
            pst_device->st_rx_dscr_ctl.ul_rx_norm_comp_mpdu_num++;
        }
        if (HAL_RX_DSCR_SMALL_QUEUE == en_queue_num)
        {
           pst_device->st_rx_dscr_ctl.ul_rx_small_comp_mpdu_num++;
        }
#endif
        /* �Ƿ��ϱ�����������;�����beacon֡�Ľ���������������Ҫ���ݿ�������ϱ� */
        if (OAL_SWITCH_ON == oam_ota_get_rx_dscr_switch())
        {
            uc_sub_type = mac_get_frame_type_and_subtype((oal_uint8 *)mac_get_rx_cb_mac_hdr(&(pst_cb_ctrl->st_rx_info)));
            en_beacon_switch = oam_ota_get_beacon_switch();
            hal_rx_get_size_dscr(pst_device, &ul_rx_dscr_len);
            if (((WLAN_FC0_SUBTYPE_BEACON|WLAN_FC0_TYPE_MGT) != uc_sub_type)
                || (OAM_SDT_PRINT_BEACON_RXDSCR_TYPE_RXDSCR == en_beacon_switch)
                || (OAM_SDT_PRINT_BEACON_RXDSCR_TYPE_BOTH == en_beacon_switch))
            {
                oam_report_dscr(BROADCAST_MACADDR, (oal_uint8 *)pst_curr_dscr, (oal_uint16)ul_rx_dscr_len, OAM_OTA_RX_DSCR_TYPE);
            }
        }

        /* ���չ���ץ��ģʽ�� */
#ifdef _PRE_WLAN_FEATURE_PACKET_CAPTURE
        pst_dmac_device = dmac_res_get_mac_dev(pst_device->uc_mac_device_id);
        /* ���ץ���ʹ�ץ�������߸ú��� */
        if ((OAL_PTR_NULL != pst_dmac_device) && (DMAC_PKT_CAP_NORMAL != pst_dmac_device->st_pkt_capture_stat.uc_capture_switch))
        {
            /* ����Ǿۺ�֡, ���þۺϱ�ʶλ */
            if (1 == pst_cb_ctrl->st_rx_status.bit_AMPDU)
            {
                en_is_ampdu = OAL_TRUE;
                if (0 != pst_cb_ctrl->st_rx_statistic.c_rssi_dbm)
                {
                    c_rssi_ampdu = pst_cb_ctrl->st_rx_statistic.c_rssi_dbm;
                }
            }
            /* ���չ���ץ�������� */
            ul_ret = dmac_pkt_cap_rx(pst_cb_ctrl, pst_netbuf, &pst_dmac_device->st_pkt_capture_stat, pst_device, c_rssi_ampdu);
        }
#endif

#ifdef _PRE_DEBUG_MODE
        /* ��¼�°벿����������켣 */
        if (OAL_FALSE == pst_device->ul_track_stop_flag)
        {
            pst_device->ul_dpart_save_idx++;
        }

        if (pst_device->ul_dpart_save_idx >= HAL_DOWM_PART_RX_TRACK_MEM)
        {
            pst_device->ul_dpart_save_idx = 0;
        }

        OAL_MEMZERO(&pst_device->ast_dpart_track[pst_device->ul_dpart_save_idx], OAL_SIZEOF(hal_rx_dpart_track_stru));
        pst_device->ast_dpart_track[pst_device->ul_dpart_save_idx].ul_phy_addr = OAL_VIRT_TO_PHY_ADDR(pst_curr_dscr);
        pst_device->ast_dpart_track[pst_device->ul_dpart_save_idx].uc_status   = pst_cb_ctrl->st_rx_status.bit_dscr_status;
        pst_device->ast_dpart_track[pst_device->ul_dpart_save_idx].uc_q        = en_queue_num;
        pst_device->ast_dpart_track[pst_device->ul_dpart_save_idx].ul_timestamp= 0;
        pst_device->ast_dpart_track[pst_device->ul_dpart_save_idx].ul_irq_cnt  = pst_device->ul_irq_cnt;
        oal_get_cpu_stat(&pst_device->ast_dpart_track[pst_device->ul_dpart_save_idx].st_cpu_state);
#endif

        OAL_MEM_TRACE(pst_curr_dscr, OAL_FALSE);
        /* �黹������ */
        hal_rx_free_dscr_list(pst_device, en_queue_num, (oal_uint32*)pst_curr_dscr);

        if (OAL_TRUE == en_last_int_flag)
        {
            us_dscr_num = OAL_SUB(us_dscr_num, 1);
        }
        else
        {
            (*pus_pre_dscr_num)++;    /* �Ǳ����жϵ��������ĸ�������1 */
        }

#ifdef _PRE_WLAN_FEATURE_PACKET_CAPTURE
        if ((OAL_PTR_NULL != pst_dmac_device) && (0 == us_dscr_num) && (OAL_TRUE == en_is_ampdu) && (DMAC_PKT_CAP_MIX == pst_dmac_device->st_pkt_capture_stat.uc_capture_switch))
        {
            ul_ret = dmac_pkt_cap_ba(&(pst_dmac_device->st_pkt_capture_stat), pst_cb_ctrl, pst_device);
            if (OAL_SUCC != ul_ret)
            {
                OAM_WARNING_LOG1(0, OAM_SF_PKT_CAP, "{dmac_rx_update_dscr_queue::dmac_pkt_cap_ba err return 0x%x.}", ul_ret);
            }
        }
#endif

        /* �жϵ�ǰ�����ж��Ƿ������ */
        if (0 == us_dscr_num)
        {
            break;
        }
    }

    return OAL_SUCC;
}
#else
OAL_STATIC oal_uint32  dmac_rx_update_dscr_queue(
                hal_to_dmac_device_stru            *pst_device,
                hal_hw_rx_dscr_info_stru           *pst_rx_dscr_info,
                oal_netbuf_head_stru               *pst_netbuf_header,
                oal_uint16                         *pus_pre_dscr_num)
{
    hal_rx_dscr_queue_header_stru  *pst_rx_dscr_queue;
    dmac_rx_ctl_stru               *pst_cb_ctrl;                    /* ��дnetbuf��cb�ֶ�ʹ�� */
    oal_netbuf_stru                *pst_netbuf = OAL_PTR_NULL;      /* ָ��ÿһ����������Ӧ��netbuf */
    oal_uint32                     *pul_curr_dscr;                  /* ���ڴ���������� */
    oal_uint32                     *pul_next_dscr;                  /* ��һ����Ҫ����������� */
    hal_rx_dscr_stru               *pst_hal_to_dmac_dscr;           /* HAL��DMAC�ṩ�Ķ����������ṹ */
    oal_bool_enum_uint8             en_last_int_flag = OAL_FALSE;   /* ����ϱ����жϵı�ʶ OAL_TRUE ����ʼ�������ж�*/
    oal_uint16                      us_dscr_num = pst_rx_dscr_info->us_dscr_num;
    hal_rx_dscr_queue_id_enum_uint8 en_queue_num = pst_rx_dscr_info->uc_queue_id;
    mac_vap_stru                   *pst_mac_vap;
#if (defined(_PRE_PRODUCT_ID_HI110X_DEV))
    oal_uint8                       *puc_mac_hdr_addr;
    oal_uint8                       *puc_tail;
    oal_uint16                      us_frame_len;
#endif
    oal_uint8                       uc_sub_type;
    oam_sdt_print_beacon_rxdscr_type_enum_uint8           en_beacon_switch;
    oal_uint32                      ul_rx_dscr_len;

#ifdef _PRE_WLAN_FEATURE_PACKET_CAPTURE
    dmac_device_stru               *pst_dmac_device;
    oal_bool_enum_uint8             en_is_ampdu = OAL_FALSE;
    oal_int8                        c_rssi_ampdu = 0;
    oal_uint32                      ul_ret;
#endif
#if defined _PRE_WLAN_PRODUCT_1151V200 && defined _PRE_WLAN_RX_DSCR_TRAILER
    mac_ieee80211_frame_stru           *pst_frame_hdr;
    oal_uint8                       uc_frame_type;
#endif
    oal_bool_enum_uint8             en_is_first_dscr = OAL_TRUE;

    pst_rx_dscr_queue = &(pst_device->ast_rx_dscr_queue[pst_rx_dscr_info->uc_queue_id]);

    pul_curr_dscr = pst_rx_dscr_queue->pul_element_head;

#ifdef _PRE_DEBUG_MODE
    if (pul_curr_dscr != pst_rx_dscr_info->pul_base_dscr)
    {
        pst_device->ul_rx_irq_loss_cnt++;
        OAM_WARNING_LOG4(0, OAM_SF_RX, "{dmac_rx_update_dscr_queue::Losed interrupt[%d], us_dscr_num[%d], phy addr=0x%08x, base phy addr=0x%08x,.",
                                pst_device->ul_rx_irq_loss_cnt, us_dscr_num,
                                OAL_DSCR_VIRT_TO_PHY(HAL_RX_DSCR_GET_REAL_ADDR(pul_curr_dscr)),
                                OAL_DSCR_VIRT_TO_PHY(HAL_RX_DSCR_GET_REAL_ADDR(pst_rx_dscr_info->pul_base_dscr)));
    }
#endif

    while(OAL_PTR_NULL != pul_curr_dscr)
    {
        /* �ж��Ƿ��Ѿ���ʼ�����жϴ��� */
        if (pul_curr_dscr == pst_rx_dscr_info->pul_base_dscr)
        {
            en_last_int_flag = OAL_TRUE;
        }

        /* ��ȡ��������Ӧ��netbuf */
        hal_rx_get_netbuffer_addr_dscr(pst_device, pul_curr_dscr, &pst_netbuf);

        pst_cb_ctrl = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);

        /* ��ȡ���������ֶ���Ϣ��������ӵ�netbuf��cb�ֶ��� */
        dmac_rx_get_dscr_info(pst_device, pul_curr_dscr, pst_cb_ctrl);

#if defined _PRE_WLAN_PRODUCT_1151V200 && defined _PRE_WLAN_RX_DSCR_TRAILER
        pst_frame_hdr = (mac_ieee80211_frame_stru *)mac_get_rx_cb_mac_hdr((mac_rx_ctl_stru*)(&pst_cb_ctrl->st_rx_info));
        if (OAL_PTR_NULL != pst_frame_hdr)
        {
            uc_frame_type    = mac_frame_get_type_value((oal_uint8 *)pst_frame_hdr);
            if ((pst_cb_ctrl->st_rx_info.bit_vap_id < WLAN_HAL_OHTER_BSS_ID) && (HAL_VAP_ID_IS_VALID(pst_cb_ctrl->st_rx_info.bit_vap_id))
                && en_is_first_dscr && WLAN_DATA_BASICTYPE == uc_frame_type)
            {
                dmac_stat_update_ant_rssi(pst_device, pst_cb_ctrl->st_rx_info.bit_vap_id, pst_cb_ctrl->st_rx_statistic);
            }
        }
#endif
#ifdef _PRE_WLAN_FEATURE_ALWAYS_TX
        /* ���հ�ͳ�� */
        hal_rx_record_frame_status_info(pst_device, pul_curr_dscr, en_queue_num);
#endif
#if defined _PRE_WLAN_PRODUCT_1151V200
        /*����Ǳ����ж�,AMPDU��֡���������ж�����������*/
        if (OAL_TRUE == en_last_int_flag)
        {
            pst_cb_ctrl->st_rx_statistic.us_mpdu_num = pst_rx_dscr_info->us_dscr_num;
        }
#endif
        /* ��cb�е��ŵ��Ž��и�ֵ */
        pst_cb_ctrl->st_rx_info.uc_channel_number = pst_rx_dscr_info->uc_channel_num;

        /* �����beacon֡�Ľ���������������Ҫ���ݿ�������ϱ� */
        if (OAL_SWITCH_ON == oam_ota_get_rx_dscr_switch())
        {
            uc_sub_type = mac_get_frame_type_and_subtype((oal_uint8 *)mac_get_rx_cb_mac_hdr(&(pst_cb_ctrl->st_rx_info)));
            en_beacon_switch = oam_ota_get_beacon_switch();
            hal_rx_get_size_dscr(pst_device, &ul_rx_dscr_len);
            if (((WLAN_FC0_SUBTYPE_BEACON|WLAN_FC0_TYPE_MGT) != uc_sub_type)
                || (OAM_SDT_PRINT_BEACON_RXDSCR_TYPE_RXDSCR == en_beacon_switch)
                || (OAM_SDT_PRINT_BEACON_RXDSCR_TYPE_BOTH == en_beacon_switch))
            {
                oam_report_dscr(BROADCAST_MACADDR, (oal_uint8 *)pul_curr_dscr, (oal_uint16)ul_rx_dscr_len, OAM_OTA_RX_DSCR_TYPE);
            }
        }
        else
        {
            if (OAL_SWITCH_ON == oam_report_data_get_global_switch(OAM_OTA_FRAME_DIRECTION_TYPE_RX))
            {
                if (OAL_SUCC == dmac_rx_get_vap(pst_device,pst_cb_ctrl->st_rx_info.bit_vap_id,HAL_RX_DSCR_NORMAL_PRI_QUEUE,&pst_mac_vap))
                {
                    mac_report_80211_frame((oal_uint8 *)pst_mac_vap,(oal_uint8 *)pst_cb_ctrl,pst_netbuf,(oal_uint8 *)pul_curr_dscr,OAM_OTA_RX_DSCR_TYPE);
                }

            }
        }

        if (HAL_RX_NEW == pst_cb_ctrl->st_rx_status.bit_dscr_status)
        {
            if (OAL_TRUE == en_last_int_flag)
            {
                /* Ӳ��RX_NEW */
                OAM_ERROR_LOG4(0, OAM_SF_RX, "{dmac_rx_update_dscr_queue::RX_NEW HARDWARE!phy addr=0x%08x,base phy addr=0x%08x,cnt=%d,q=%d.}",
                               OAL_DSCR_VIRT_TO_PHY(HAL_RX_DSCR_GET_REAL_ADDR(pul_curr_dscr)),
                               OAL_DSCR_VIRT_TO_PHY(HAL_RX_DSCR_GET_REAL_ADDR(pst_rx_dscr_info->pul_base_dscr)), us_dscr_num, en_queue_num);
            }
            else
            {
                /* ���RX_NEW */
                OAM_ERROR_LOG4(0, OAM_SF_RX, "{dmac_rx_update_dscr_queue::RX_NEW SOFTWARE!phy addr=0x%08x,base phy addr=0x%08x,cnt=%d,q=%d.}",
                               OAL_DSCR_VIRT_TO_PHY(HAL_RX_DSCR_GET_REAL_ADDR(pul_curr_dscr)),
                               OAL_DSCR_VIRT_TO_PHY(HAL_RX_DSCR_GET_REAL_ADDR(pst_rx_dscr_info->pul_base_dscr)), us_dscr_num, en_queue_num);
#if(defined(_PRE_PRODUCT_ID_HI110X_DEV))
                //return OAL_FAIL;
#else
                return OAL_FAIL;
#endif
            }
        }

#if(defined(_PRE_PRODUCT_ID_HI110X_DEV))
        puc_mac_hdr_addr = oal_netbuf_header(pst_netbuf);
        oal_netbuf_set_len(pst_netbuf, pst_cb_ctrl->st_rx_info.us_frame_len);

        us_frame_len = pst_cb_ctrl->st_rx_info.us_frame_len;
        if (us_frame_len < WLAN_LARGE_NETBUF_SIZE)
        {
            puc_tail = puc_mac_hdr_addr + us_frame_len;
        }
        else
        {
            puc_tail = puc_mac_hdr_addr + WLAN_LARGE_NETBUF_SIZE;
        }
        oal_set_netbuf_tail(pst_netbuf, puc_tail);
#else
        oal_netbuf_trim(pst_netbuf, OAL_NETBUF_LEN(pst_netbuf));
    #ifdef _PRE_WLAN_HW_TEST
        if (HAL_ALWAYS_RX_RESERVED == pst_device->bit_al_rx_flag)          /* ���� */
        {
            if(pst_cb_ctrl->st_rx_info.us_frame_len < HAL_AL_RX_FRAME_LEN)
            {
                oal_netbuf_put(pst_netbuf, pst_cb_ctrl->st_rx_info.us_frame_len);
            }
            else
            {
                oal_netbuf_put(pst_netbuf, HAL_AL_RX_FRAME_LEN);
            }
        }
        else
    #endif
        {
            if(pst_cb_ctrl->st_rx_info.us_frame_len < WLAN_LARGE_NETBUF_SIZE)
            {
                oal_netbuf_put(pst_netbuf, pst_cb_ctrl->st_rx_info.us_frame_len);
            }
            else
            {
                oal_netbuf_put(pst_netbuf, WLAN_LARGE_NETBUF_SIZE);
            }
        }
        /*OAL_NETBUF_LEN(pst_netbuf)  = pst_cb_ctrl->st_rx_info.us_frame_len;
        OAL_NETBUF_TAIL(pst_netbuf) = (pst_cb_ctrl->st_rx_info.us_frame_len < WLAN_LARGE_NETBUF_SIZE)
                                       ? (oal_netbuf_data(pst_netbuf) + pst_cb_ctrl->st_rx_info.us_frame_len)
                                       : (oal_netbuf_data(pst_netbuf) + WLAN_LARGE_NETBUF_SIZE);*/
#endif
        /* ���չ���ץ��ģʽ�� */
#ifdef _PRE_WLAN_FEATURE_PACKET_CAPTURE
        pst_dmac_device = dmac_res_get_mac_dev(pst_device->uc_mac_device_id);
        /* ���ץ���ʹ�ץ�������߸ú��� */
        if ((OAL_PTR_NULL != pst_dmac_device) && (DMAC_PKT_CAP_NORMAL != pst_dmac_device->st_pkt_capture_stat.uc_capture_switch))
        {
            /* ����Ǿۺ�֡, ���þۺϱ�ʶλ */
            if (1 == pst_cb_ctrl->st_rx_status.bit_AMPDU)
            {
                en_is_ampdu     = OAL_TRUE;
                if (0 != pst_cb_ctrl->st_rx_statistic.c_rssi_dbm)
                {
                    c_rssi_ampdu = pst_cb_ctrl->st_rx_statistic.c_rssi_dbm;
                }
            }
            /* ���չ���ץ�������� */
            ul_ret = dmac_pkt_cap_rx(pst_cb_ctrl, pst_netbuf, &pst_dmac_device->st_pkt_capture_stat, pst_device, c_rssi_ampdu);
        }
#endif

#ifdef _PRE_WLAN_CACHE_COHERENT_SUPPORT
        /* ��Ч��cache�����¶�ȡnetbuf������ */
        oal_dma_map_single(NULL, pst_netbuf->data, WLAN_MEM_NETBUF_SIZE2, OAL_FROM_DEVICE);
#endif
        /* ���뵽netbuf�������� */
        oal_netbuf_add_to_list_tail(pst_netbuf, pst_netbuf_header);

        /* �����һ����Ҫ����������� */
        pst_hal_to_dmac_dscr = (hal_rx_dscr_stru *)pul_curr_dscr;
        if(en_is_first_dscr)
        {
            en_is_first_dscr = OAL_FALSE;
        }

        if(NULL != pst_hal_to_dmac_dscr->pul_next_rx_dscr)
        {
            pul_next_dscr = HAL_RX_DSCR_GET_SW_ADDR((oal_uint32*)OAL_DSCR_PHY_TO_VIRT((oal_uint)(pst_hal_to_dmac_dscr->pul_next_rx_dscr)));
        }
        else
        {
            pul_next_dscr = HAL_RX_DSCR_GET_SW_ADDR(pst_hal_to_dmac_dscr->pul_next_rx_dscr);
        }

        if (OAL_TRUE == en_last_int_flag)
        {
            us_dscr_num = OAL_SUB(us_dscr_num, 1);
        }
        else
        {
            (*pus_pre_dscr_num)++;    /* �Ǳ����жϵ��������ĸ�������1 */
        }

        /* 51 ��ʱȡ���ù켣׷��DEBUG */
#if (_PRE_PRODUCT_ID_HI1151 != _PRE_PRODUCT_ID)
    #ifdef _PRE_DEBUG_MODE
        /* ��¼�°벿����������켣 */
        if (OAL_FALSE == pst_device->ul_track_stop_flag)
        {
            pst_device->ul_dpart_save_idx++;
        }

        if (pst_device->ul_dpart_save_idx >= HAL_DOWM_PART_RX_TRACK_MEM)
        {
            pst_device->ul_dpart_save_idx = 0;
        }

        OAL_MEMZERO(&pst_device->ast_dpart_track[pst_device->ul_dpart_save_idx], OAL_SIZEOF(hal_rx_dpart_track_stru));
        pst_device->ast_dpart_track[pst_device->ul_dpart_save_idx].ul_phy_addr = OAL_VIRT_TO_PHY_ADDR(HAL_RX_DSCR_GET_REAL_ADDR(pul_curr_dscr));
        pst_device->ast_dpart_track[pst_device->ul_dpart_save_idx].uc_status   = pst_cb_ctrl->st_rx_status.bit_dscr_status;
        pst_device->ast_dpart_track[pst_device->ul_dpart_save_idx].uc_q        = en_queue_num;
        pst_device->ast_dpart_track[pst_device->ul_dpart_save_idx].ul_timestamp= pul_curr_dscr[HAL_DEBUG_RX_DSCR_LINE];
        pst_device->ast_dpart_track[pst_device->ul_dpart_save_idx].ul_irq_cnt  = pst_device->ul_irq_cnt;
        oal_get_cpu_stat(&pst_device->ast_dpart_track[pst_device->ul_dpart_save_idx].st_cpu_state);
    #endif
#endif

#ifdef _PRE_WLAN_FEATURE_PACKET_CAPTURE
        /* ����ץ���н��վۺ�֡AMPDU��BA, ��ץ���²����߸ú��� */
        if ((OAL_PTR_NULL != pst_dmac_device) && (0 == us_dscr_num) && (OAL_TRUE == en_is_ampdu) && (DMAC_PKT_CAP_MIX == pst_dmac_device->st_pkt_capture_stat.uc_capture_switch))
        {
            ul_ret = dmac_pkt_cap_ba(&(pst_dmac_device->st_pkt_capture_stat), pst_cb_ctrl, pst_device);
            if (OAL_SUCC != ul_ret)
            {
                OAM_WARNING_LOG1(0, OAM_SF_PKT_CAP, "{dmac_rx_update_dscr_queue::dmac_pkt_cap_ba err return 0x%x.}", ul_ret);
            }
        }
#endif

        OAL_MEM_TRACE(pul_curr_dscr, OAL_FALSE);
        /* �黹������ */
        hal_rx_free_dscr_list(pst_device, en_queue_num, pul_curr_dscr);

        /* �ж��Ƿ������ */
        if (0 == us_dscr_num)
        {
            break;
        }

        pul_curr_dscr = pul_next_dscr;
    }

    return OAL_SUCC;
}
#endif

#if (_PRE_OS_VERSION_RAW == _PRE_OS_VERSION)  && defined (__CC_ARM)
#pragma arm section rodata, code, rwdata, zidata  // return to default placement
#endif

#ifdef _PRE_WLAN_FEATURE_PROXYSTA

oal_uint32  dmac_rx_mpdu_filter_duplicate_frame_proxysta(
                mac_vap_stru               *pst_mac_vap,
                mac_ieee80211_frame_stru   *pst_frame_hdr,
                dmac_rx_ctl_stru           *pst_cb_ctrl)
{
    oal_uint8               uc_tid;
    oal_uint16              us_seq_num;
    oal_bool_enum_uint8     en_is_4addr;
    oal_uint8               uc_is_tods;
    oal_uint8               uc_is_from_ds;
    dmac_vap_stru          *pst_dmac_vap;

    pst_dmac_vap = mac_res_get_dmac_vap(pst_mac_vap->uc_vap_id);
    if (!pst_dmac_vap)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_RX,
                         "{dmac_rx_mpdu_filter_duplicate_frame_proxysta::null dmac vap, id=%d}",
                         pst_mac_vap->uc_vap_id);
        return OAL_FAIL;
    }
    us_seq_num = mac_get_seq_num((oal_uint8 *)pst_frame_hdr);

    /* �����ĵ�ַ�����ȡ���ĵ�tid */
    uc_is_tods    = mac_hdr_get_to_ds((oal_uint8 *)pst_frame_hdr);
    uc_is_from_ds = mac_hdr_get_from_ds((oal_uint8 *)pst_frame_hdr);
    en_is_4addr   = uc_is_tods && uc_is_from_ds;

    if (0 == pst_frame_hdr->st_frame_control.bit_retry)
    {
        if ((WLAN_FC0_SUBTYPE_QOS | WLAN_FC0_TYPE_DATA) != ((oal_uint8 *)pst_frame_hdr)[0])
        {
            pst_dmac_vap->st_psta.us_non_qos_seq_num   = us_seq_num;

        }
        else
        {
            uc_tid = mac_get_tid_value((oal_uint8 *)pst_frame_hdr, en_is_4addr);

            pst_dmac_vap->st_psta.aus_last_qos_seq_num[uc_tid] = us_seq_num;
        }

        return OAL_SUCC;
    }

    /* �жϸ�֡�ǲ���qos֡ */
    if ((WLAN_FC0_SUBTYPE_QOS | WLAN_FC0_TYPE_DATA) != ((oal_uint8 *)pst_frame_hdr)[0])
    {
        if (us_seq_num == pst_dmac_vap->st_psta.us_non_qos_seq_num)
        {
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_RX,
                             "{dmac_rx_mpdu_filter_duplicate_frame_proxysta::non qos duplicate frame[%d]}",
                             us_seq_num);

            return OAL_FAIL;
        }

        pst_dmac_vap->st_psta.us_non_qos_seq_num = us_seq_num;

        return OAL_SUCC;
    }

    uc_tid = mac_get_tid_value((oal_uint8 *)pst_frame_hdr, en_is_4addr);

    /* qos���� */
    if (us_seq_num == pst_dmac_vap->st_psta.aus_last_qos_seq_num[uc_tid])
    {
        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_RX,
                         "{dmac_rx_mpdu_filter_duplicate_frame_proxysta::tid is[%d],qos duplicate frame[%d]. }",
                         uc_tid,
                         us_seq_num);

        return OAL_FAIL;
    }
    else
    {
        pst_dmac_vap->st_psta.aus_last_qos_seq_num[uc_tid] = us_seq_num;
    }

    return OAL_SUCC;
}
#endif


oal_void  dmac_rx_data_user_is_null(mac_vap_stru *pst_mac_vap, mac_ieee80211_frame_stru *pst_frame_hdr)
{
    dmac_vap_stru           *pst_dmac_vap;
    oal_uint32               ul_cur_tick_ms;
    dmac_diasoc_deauth_event st_deauth_evt;
    oal_uint32               ul_delta_time;

    pst_dmac_vap = mac_res_get_dmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id,OAM_SF_PWR,"{dmac_rx_data_user_is_null::mac_res_get_dmac_vap fail}");
        return;
    }

    OAM_STAT_VAP_INCR(pst_mac_vap->uc_vap_id, rx_ta_check_dropped, 1);
    DMAC_VAP_DFT_STATS_PKT_INCR(pst_dmac_vap->st_query_stats.ul_rx_ta_check_dropped, 1);

    ul_cur_tick_ms = (oal_uint32)OAL_TIME_GET_STAMP_MS();
    ul_delta_time = (oal_uint32)OAL_TIME_GET_RUNTIME(ul_deauth_flow_control_ms, ul_cur_tick_ms);
    if (ul_delta_time < DEAUTH_INTERVAL_MIN)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id,OAM_SF_PWR,"{dmac_rx_data_user_is_null::two deauth interval(%d) is too short}",ul_delta_time);
        return;
    }

    ul_deauth_flow_control_ms = ul_cur_tick_ms;

    oal_memcopy(st_deauth_evt.auc_des_addr, pst_frame_hdr->auc_address2, WLAN_MAC_ADDR_LEN);
    st_deauth_evt.uc_reason = MAC_NOT_AUTHED;
    st_deauth_evt.uc_event  = DMAC_WLAN_CRX_EVENT_SUB_TYPE_DEAUTH;

    OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_RX, "dmac_rx_data_user_is_null: ta: %02X:XX:XX:%02X:%02X:%02X. post sending deauth event to hmac",
                        pst_frame_hdr->auc_address2[0],
                        pst_frame_hdr->auc_address2[3],
                        pst_frame_hdr->auc_address2[4],
                        pst_frame_hdr->auc_address2[5]);

    dmac_rx_send_event(pst_dmac_vap, DMAC_WLAN_CRX_EVENT_SUB_TYPE_DEAUTH, (oal_uint8 *)&st_deauth_evt, OAL_SIZEOF(st_deauth_evt));
}


#ifndef _PRE_WLAN_PROFLING_MIPS
#if (_PRE_OS_VERSION_RAW == _PRE_OS_VERSION)  && defined (__CC_ARM)
#pragma arm section rwdata = "BTCM", code ="ATCM", zidata = "BTCM", rodata = "ATCM"
#endif
#endif

oal_uint32  dmac_rx_filter_frame_sta(mac_vap_stru *pst_vap, dmac_rx_ctl_stru *pst_cb_ctrl, dmac_user_stru *pst_dmac_user)
{
    mac_ieee80211_frame_stru   *pst_frame_hdr;
    oal_uint32                  ul_ret           = OAL_SUCC;
    oal_uint8                  *puc_addr         = OAL_PTR_NULL;
    dmac_vap_stru              *pst_dmac_vap     = OAL_PTR_NULL;

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    mac_device_stru            *pst_dev = mac_res_get_dev(pst_vap->uc_device_id);

    if (!pst_dev)
    {
        return OAL_FAIL;
    }
#endif
    /* ��ȡ�û���֡��Ϣ */
    pst_frame_hdr = (mac_ieee80211_frame_stru *)mac_get_rx_cb_mac_hdr(&(pst_cb_ctrl->st_rx_info));

    pst_dmac_vap  = MAC_GET_DMAC_VAP(pst_vap);

    if (WLAN_DATA_BASICTYPE == pst_frame_hdr->st_frame_control.bit_type)
    {
        if (OAL_PTR_NULL == pst_dmac_user)
        {
            /* ����OAL_FAIL ��ʾ����֡��Ҫ���� */
            dmac_rx_data_user_is_null(pst_vap, pst_frame_hdr);

            return OAL_FAIL;
        }
        else
        {
            mac_get_transmit_addr(pst_frame_hdr, &puc_addr);
            /* TA���ǷǱ�BSS������֡�����ϱ���ֱ�ӷ���ʧ�ܣ������ͷ��ڴ棬�������ֱ�ӷ��� */
            if (OAL_MEMCMP(pst_dmac_user->st_user_base_info.auc_user_mac_addr, puc_addr, WLAN_MAC_ADDR_LEN))
            {
                return OAL_FAIL;
            }
        }
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
        if (mac_is_proxysta_enabled(pst_dev))
        {
            /* ��AMPDU�ظ�֡���� */
            if (OAL_FALSE == pst_cb_ctrl->st_rx_status.bit_AMPDU)
            {
                 if (mac_vap_is_vsta(pst_vap)) // FIXME:
                 {
                     ul_ret = dmac_rx_mpdu_filter_duplicate_frame_proxysta(pst_vap, pst_frame_hdr, pst_cb_ctrl);
                     if(OAL_SUCC != ul_ret)
                     {
                         return ul_ret;
                     }
                 }
            }
        }
#endif

    #ifdef _PRE_WLAN_FEATURE_AMPDU
        ul_ret = dmac_ba_filter_serv(pst_vap, pst_dmac_user, pst_cb_ctrl, pst_frame_hdr);
    #endif

        /* �����û���RSSIͳ����Ϣ */
        oal_rssi_smooth(&(pst_dmac_user->s_rx_rssi), pst_cb_ctrl->st_rx_statistic.c_rssi_dbm);

        /* ����֡����SNR */
        dmac_vap_update_snr_info(pst_dmac_vap, pst_cb_ctrl, pst_frame_hdr);

    }

    /* ����rx��11nЭ����ص�mib */
    //dmac_rx_update_mib_11n(pst_vap, pst_cb_ctrl);

    return ul_ret;
}
#ifndef _PRE_WLAN_PROFLING_MIPS
#if (_PRE_OS_VERSION_RAW == _PRE_OS_VERSION)  && defined (__CC_ARM)
#pragma arm section rodata, code, rwdata, zidata  // return to default placement
#endif
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)

oal_uint32  dmac_rx_mpdu_filter_duplicate_frame(
                dmac_user_stru             *pst_dmac_user,
                mac_ieee80211_frame_stru   *pst_frame_hdr,
                dmac_rx_ctl_stru           *pst_cb_ctrl)
{
    oal_uint8               uc_tid;
    oal_uint16              us_seq_num;
    oal_uint16              us_seq_frag_num;
    oal_bool_enum_uint8     en_is_4addr;
    oal_uint8               uc_is_tods;
    oal_uint8               uc_is_from_ds;

    us_seq_num = mac_get_seq_num((oal_uint8 *)pst_frame_hdr);

    us_seq_frag_num = ((oal_uint16)(pst_frame_hdr->bit_seq_num) << 4) | pst_frame_hdr->bit_frag_num;

    /* �����ĵ�ַ�����ȡ���ĵ�tid */
    uc_is_tods    = mac_hdr_get_to_ds((oal_uint8 *)pst_frame_hdr);
    uc_is_from_ds = mac_hdr_get_from_ds((oal_uint8 *)pst_frame_hdr);
    en_is_4addr   = uc_is_tods && uc_is_from_ds;

    if (0 == pst_frame_hdr->st_frame_control.bit_retry)
    {
        if ((WLAN_FC0_SUBTYPE_QOS | WLAN_FC0_TYPE_DATA) != ((oal_uint8 *)pst_frame_hdr)[0])
        {
            pst_dmac_user->us_non_qos_seq_frag_num = us_seq_frag_num;
        }
        else
        {
            uc_tid = mac_get_tid_value((oal_uint8 *)pst_frame_hdr, en_is_4addr);

            pst_dmac_user->ast_tx_tid_queue[uc_tid].us_last_seq_frag_num = us_seq_frag_num;
        }

        return OAL_SUCC;
    }

    /* �жϸ�֡�ǲ���qos֡ */
    if ((WLAN_FC0_SUBTYPE_QOS | WLAN_FC0_TYPE_DATA) != ((oal_uint8 *)pst_frame_hdr)[0])
    {
        if (us_seq_frag_num == pst_dmac_user->us_non_qos_seq_frag_num)
        {
            OAM_WARNING_LOG2(pst_dmac_user->st_user_base_info.uc_vap_id, OAM_SF_RX,
                             "{dmac_rx_ucast_filter_duplicate_frame::non qos duplicate frame[%d], seq_num with frag_num [%d]}",
                             us_seq_num, us_seq_frag_num);
#ifdef _PRE_WLAN_11K_STAT
            dmac_stat_rx_duplicate_num(pst_dmac_user);
#endif

            return OAL_FAIL;
        }

        pst_dmac_user->us_non_qos_seq_frag_num = us_seq_frag_num;

        return OAL_SUCC;
    }

    uc_tid = mac_get_tid_value((oal_uint8 *)pst_frame_hdr, en_is_4addr);

    /* qos���� */
    if (us_seq_frag_num == pst_dmac_user->ast_tx_tid_queue[uc_tid].us_last_seq_frag_num)
    {
        OAM_WARNING_LOG3(pst_dmac_user->st_user_base_info.uc_vap_id, OAM_SF_RX,
                         "{dmac_rx_ucast_filter_duplicate_frame::tid is[%d],qos duplicate frame[%d], seq_num with frag num [%d].}",
                         uc_tid, us_seq_num, us_seq_frag_num);
#ifdef _PRE_WLAN_11K_STAT
        dmac_stat_rx_duplicate_num(pst_dmac_user);
#endif

        return OAL_FAIL;
    }
    else
    {
        pst_dmac_user->ast_tx_tid_queue[uc_tid].us_last_seq_frag_num = us_seq_frag_num;
    }

    return OAL_SUCC;
}
#endif


oal_uint32  dmac_rssi_process(
                mac_vap_stru               *pst_mac_vap,
                dmac_user_stru             *pst_dmac_user,
                dmac_rx_ctl_stru           *pst_cb_ctrl)
{
    dmac_device_stru                 *pst_dmac_device;
    oal_int8                          c_cb_rssi_dbm;    //CB�ֶ��������ʵrssi
    oal_int8                          c_user_real_rssi_dbm;     //user�ṹ�������ƽ�������rssi��������õ��û���ʵ��rssi

    if (OAL_PTR_NULL == pst_mac_vap)
    {
       OAM_WARNING_LOG0(0, OAM_SF_ANY, "{dmac_low_rssi_process::pst_mac_vap null.}");
       return OAL_ERR_CODE_PTR_NULL;
    }

    //dmac_dev�����rssi�����Ƿ��
    pst_dmac_device = dmac_res_get_mac_dev(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_dmac_device)
    {
       OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{dmac_low_rssi_process::pst_dmac_device null.}");
       return OAL_ERR_CODE_PTR_NULL;
    }

    /* VAPģʽ�ж�, rssi_limitֻ������AP */
    if ((WLAN_VAP_MODE_BSS_AP != pst_mac_vap->en_vap_mode))
    {
         OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_TX, "{dmac_rssi_process:: rssi_limit only used in AP mode; en_vap_mode=%d.}", pst_mac_vap->en_vap_mode);
         return OAL_FAIL;
    }

    OAM_INFO_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{dmac_low_rssi_process::en_rssi_limit_enable_flag[%d], uc_device_id[%d],device_s_rssi[%d] .}",
        pst_dmac_device->st_rssi_limit.en_rssi_limit_enable_flag,
        pst_mac_vap->uc_device_id, (oal_int32)pst_dmac_device->st_rssi_limit.c_rssi, (oal_int32)pst_cb_ctrl->st_rx_statistic.c_rssi_dbm);

    if (OAL_FALSE == pst_dmac_device->st_rssi_limit.en_rssi_limit_enable_flag)
    {
        return OAL_SUCC; //rssi_limit����δ�򿪣�ֱ�ӷ���succ
    }

    //pst_dmac_userΪNULL,˵���û������ڣ������û��Ĺ���֡���¹����õ����ޣ���Ҫ����ֵ�Ļ����������delta
    if (OAL_PTR_NULL == pst_dmac_user)
    {
        //��֡��rssiС������ֵ����ֱ��fail
        c_cb_rssi_dbm = pst_cb_ctrl->st_rx_statistic.c_rssi_dbm;

        OAM_INFO_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{dmac_low_rssi_process::[mgmt]c_cb_rssi_dbm[%d], limit_rssi[%d], c_rssi_delta[%d].}",
                                            (oal_int32)c_cb_rssi_dbm, (oal_int32)pst_dmac_device->st_rssi_limit.c_rssi, (oal_int32)pst_dmac_device->st_rssi_limit.c_rssi_delta);

        if(c_cb_rssi_dbm < (pst_dmac_device->st_rssi_limit.c_rssi + pst_dmac_device->st_rssi_limit.c_rssi_delta))
        {
            OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{dmac_low_rssi_process::[mgmt] cb_rssi_dbm[%d], limit_rssi[%d], c_rssi_delta[%d]}",
                                            (oal_int32)c_cb_rssi_dbm, (oal_int32)pst_dmac_device->st_rssi_limit.c_rssi, (oal_int32)pst_dmac_device->st_rssi_limit.c_rssi_delta);
            return OAL_FAIL;
        }
    }
    else
    {
        /* �û�����, �����ѹ����õ�����֡ */
        //��֡��rssiС������ֵ�����ߵ��û���������fail
        //��ȡuser��ʵ��rssi dbm
        c_user_real_rssi_dbm = oal_get_real_rssi(pst_dmac_user->s_rx_rssi);

        OAM_INFO_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{dmac_low_rssi_process::c_user_real_rssi_dbm[%d], limit_rssi[%d], user_rssi[%d].}",
                                                    (oal_int32)c_user_real_rssi_dbm, (oal_int32)pst_dmac_device->st_rssi_limit.c_rssi, (oal_int32)pst_dmac_user->s_rx_rssi);

        if (c_user_real_rssi_dbm < pst_dmac_device->st_rssi_limit.c_rssi)
        {
            OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{dmac_low_rssi_process::c_user_real_rssi_dbm[%d], limit_rssi[%d], user_rssi[%d].}",
                (oal_int32)c_user_real_rssi_dbm, (oal_int32)pst_dmac_device->st_rssi_limit.c_rssi,(oal_int32) pst_dmac_user->s_rx_rssi);
            /* �ߵ��û� */
            dmac_send_disasoc_misc_event(pst_mac_vap, pst_dmac_user->st_user_base_info.us_assoc_id, DMAC_DISASOC_MISC_LOW_RSSI);
            return OAL_FAIL;
        }
    }

    return OAL_SUCC;
}



oal_uint32  dmac_rx_filter_frame_ap(
                mac_vap_stru           *pst_vap,
                dmac_rx_ctl_stru       *pst_cb_ctrl)
{
    mac_ieee80211_frame_stru   *pst_frame_hdr;
    dmac_user_stru             *pst_dmac_user;
    oal_uint32                  ul_ret = OAL_SUCC;
    dmac_diasoc_deauth_event    st_disasoc_evt;
    oal_uint32                  ul_len = 0;
    dmac_vap_stru              *pst_dmac_vap;
    oal_uint32                  ul_cur_tick_ms = 0;
    oal_uint32                  ul_delta_time;

    /* ��ȡ�û���֡��Ϣ */
    pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user(MAC_GET_RX_CB_TA_USER_IDX(&(pst_cb_ctrl->st_rx_info)));
    pst_frame_hdr = (mac_ieee80211_frame_stru *)(mac_get_rx_cb_mac_hdr(&(pst_cb_ctrl->st_rx_info)));
    pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(pst_vap->uc_vap_id, OAM_SF_RX, "{dmac_rx_filter_frame_ap::pst_dmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (WLAN_DATA_BASICTYPE == pst_frame_hdr->st_frame_control.bit_type)
    {
        if (OAL_PTR_NULL == pst_dmac_user)  /* �û������� */
        {
            /* ����OAL_FAIL ��ʾ����֡��Ҫ���� */
            dmac_rx_data_user_is_null(pst_vap, pst_frame_hdr);

            return OAL_FAIL;
        }

        if (MAC_USER_STATE_ASSOC != pst_dmac_user->st_user_base_info.en_user_asoc_state)/* �û�δ���� */
        {
            /* ����OAL_FAIL ��ʾ����֡��Ҫ���� */
            OAM_WARNING_LOG2(pst_vap->uc_vap_id, OAM_SF_RX, "{dmac_rx_filter_frame_ap::status is not assoc,user index:%d,en_user_asoc_state:%d}",
                             pst_dmac_user->st_user_base_info.us_assoc_id,
                             pst_dmac_user->st_user_base_info.en_user_asoc_state);

            OAM_WARNING_LOG4(pst_vap->uc_vap_id, OAM_SF_RX, "{dmac_rx_filter_frame_ap::user mac:%02X:XX:XX:%02X:%02X:%02X}",
                             pst_dmac_user->st_user_base_info.auc_user_mac_addr[0],
                             pst_dmac_user->st_user_base_info.auc_user_mac_addr[3],
                             pst_dmac_user->st_user_base_info.auc_user_mac_addr[4],
                             pst_dmac_user->st_user_base_info.auc_user_mac_addr[5]);

            OAM_STAT_VAP_INCR(pst_vap->uc_vap_id, rx_ta_check_dropped, 1);
            DMAC_VAP_DFT_STATS_PKT_INCR(pst_dmac_vap->st_query_stats.ul_rx_ta_check_dropped, 1);

            ul_cur_tick_ms = (oal_uint32)OAL_TIME_GET_STAMP_MS();
            ul_delta_time = (oal_uint32)OAL_TIME_GET_RUNTIME(ul_deauth_flow_control_ms, ul_cur_tick_ms);
            if (ul_delta_time < DEAUTH_INTERVAL_MIN)
            {
                OAM_WARNING_LOG1(pst_vap->uc_vap_id,OAM_SF_PWR,"{dmac_rx_filter_frame_ap::two deauth interval(%d) is too short}",ul_delta_time);
                return OAL_FAIL;
            }

            ul_deauth_flow_control_ms = ul_cur_tick_ms;
            oal_memcopy(st_disasoc_evt.auc_des_addr, pst_frame_hdr->auc_address2, WLAN_MAC_ADDR_LEN);
            st_disasoc_evt.uc_reason = MAC_NOT_ASSOCED;
            st_disasoc_evt.uc_event  = DMAC_WLAN_CRX_EVENT_SUB_TYPE_DISASSOC;
            ul_len = OAL_SIZEOF(st_disasoc_evt);

            dmac_rx_send_event(pst_dmac_vap, DMAC_WLAN_CRX_EVENT_SUB_TYPE_DISASSOC, (oal_uint8 *)&st_disasoc_evt, ul_len);
            return OAL_FAIL;
        }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
        /* ��AMPDU�ظ�֡���� */
        if (OAL_FALSE == pst_cb_ctrl->st_rx_status.bit_AMPDU)
        {
            ul_ret = dmac_rx_mpdu_filter_duplicate_frame(pst_dmac_user, pst_frame_hdr, pst_cb_ctrl);
            if(OAL_SUCC != ul_ret)
            {
                return ul_ret;
            }
        }
#endif

    #ifdef _PRE_WLAN_FEATURE_AMPDU
        ul_ret = dmac_ba_filter_serv(pst_vap, pst_dmac_user, pst_cb_ctrl, pst_frame_hdr);
    #endif

        /* �����û���RSSIͳ����Ϣ */
        oal_rssi_smooth(&(pst_dmac_user->s_rx_rssi), pst_cb_ctrl->st_rx_statistic.c_rssi_dbm);

    }
#ifdef _PRE_WLAN_FEATURE_USER_RESP_POWER
    else if (WLAN_MANAGEMENT == pst_frame_hdr->st_frame_control.bit_type)
    {
        if (OAL_PTR_NULL != pst_dmac_user)  /* �û����� */
        {
            /* ���¹����û���RSSIͳ����Ϣ����Ҫ��auth req֮���û�����auth resp��assoc resp֡����֡���ʿ��� */
            oal_rssi_smooth(&(pst_dmac_user->s_rx_rssi), pst_cb_ctrl->st_rx_statistic.c_rssi_dbm);
        }
    }
#endif
    else
    {

    }

    /* low rssi�ж� */
    if ((WLAN_MANAGEMENT == pst_frame_hdr->st_frame_control.bit_type) || (WLAN_DATA_BASICTYPE == pst_frame_hdr->st_frame_control.bit_type))
    {
        if (OAL_SUCC != dmac_rssi_process(pst_vap, pst_dmac_user, pst_cb_ctrl))  //���ز�Ϊsucc��˵��Ҫ�߳��û�������������
        {
            return OAL_FAIL;
        }
    }

    /* ����rx��11nЭ����ص�mib */
    //dmac_rx_update_mib_11n(pst_vap, pst_cb_ctrl);

    return ul_ret;
}



#if (_PRE_OS_VERSION_RAW == _PRE_OS_VERSION)  && defined (__CC_ARM)
#pragma arm section rwdata = "BTCM", code ="ATCM", zidata = "BTCM", rodata = "ATCM"
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

oal_void  dmac_rx_update_packets_process_info(hal_to_dmac_device_stru *pst_device,
                                                      hal_hw_rx_dscr_info_stru *pst_rx_isr_info,
                                                      oal_uint8 *puc_rx_isr_info_cnt)
{

    if ( 0 == pst_rx_isr_info->us_dscr_num)
    {
        OAM_WARNING_LOG1(0, OAM_SF_RX, "{hi1103_update_rx_packets_process:: q_id[%d] 0 == pst_rx_isr_info->us_dscr_num}" ,pst_rx_isr_info->uc_queue_id);
    }
    else
    {
        GET_DEV_RX_DSCR_EVENT_CNT(pst_device, pst_rx_isr_info->uc_queue_id)++;
        GET_DEV_RX_DSCR_EVENT_PKTS(pst_device, pst_rx_isr_info->uc_queue_id) += pst_rx_isr_info->us_dscr_num;
       *(puc_rx_isr_info_cnt + pst_rx_isr_info->uc_queue_id) = ((*(puc_rx_isr_info_cnt + pst_rx_isr_info->uc_queue_id)) + 1);
    }

    return;
}


OAL_STATIC OAL_INLINE oal_void dmac_rx_update_max_dscr_usage(hal_to_dmac_device_stru *pst_device)
{
    hal_rx_dscr_queue_id_enum_uint8 uc_queue_id = HAL_RX_DSCR_NORMAL_PRI_QUEUE;
    do
    {
        if (pst_device->ast_rx_dscr_queue[uc_queue_id].us_element_cnt > GET_DEV_RX_DSCR_MAX_USED_CNT(pst_device, uc_queue_id))
        {
            GET_DEV_RX_DSCR_MAX_USED_CNT(pst_device, uc_queue_id) = pst_device->ast_rx_dscr_queue[uc_queue_id].us_element_cnt;
            //OAM_WARNING_LOG3(0, OAM_SF_RX, "{dmac_rx_update_max_dscr_usage:: q[%d], max_elemt[%d], dev[%d]}" ,uc_queue_id, pst_device->ast_rx_dscr_queue[uc_queue_id].us_element_cnt,pst_device->uc_device_id);
        }

        uc_queue_id++;
    } while (uc_queue_id < HAL_RX_DSCR_QUEUE_ID_BUTT);
}
#endif
OAL_STATIC oal_uint32  dmac_rx_get_dscr_list(
                hal_to_dmac_device_stru        *pst_device,
                hal_hw_rx_dscr_info_stru       *pst_rx_isr_info,
                frw_event_stru                 *pst_event,
                oal_netbuf_head_stru           *pst_netbuf_header)
{
#if(_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    mac_vap_stru           *pst_vap;
#endif
    oal_uint8               uc_vap_id;
    oal_uint8               uc_rx_dscr_status;      /* ���յ���������״̬ */
    oal_netbuf_stru        *pst_netbuf;
    oal_uint16              us_pre_dscr_num = 0;    /* �����쳣�ж�δ������������ĸ��� */
    oal_uint32              ul_ret;

    hal_wlan_rx_event_stru *pst_wlan_rx_event = (hal_wlan_rx_event_stru *)(pst_event->auc_event_data);
    uc_vap_id = pst_event->st_event_hdr.uc_vap_id;

    /* ���ж��ϱ��������������ӽ��ն�����ժ�� */
    ul_ret = dmac_rx_update_dscr_queue(pst_device, pst_rx_isr_info, pst_netbuf_header, &us_pre_dscr_num);
    if (OAL_SUCC != ul_ret)
    {
#if(defined(_PRE_PRODUCT_ID_HI110X_DEV))
        //pst_wlan_rx_event->us_dscr_num += us_pre_dscr_num;
#else
        pst_wlan_rx_event->us_dscr_num += us_pre_dscr_num;
#endif
        OAM_WARNING_LOG3(uc_vap_id, OAM_SF_RX, "{dmac_rx_get_dscr_list::dmac_rx_update_dscr_queue failed[%d], num[%d], us_pre_dscr_num[%d].", ul_ret, pst_wlan_rx_event->us_dscr_num, us_pre_dscr_num);
        return OAL_FAIL;
    }

    if(oal_netbuf_list_empty(pst_netbuf_header))
    {
        if(pst_rx_isr_info->us_dscr_num)
        {
            OAL_IO_PRINT("[ERROR]did not found netbuf but us_dscr_num is %d\r\n", pst_rx_isr_info->us_dscr_num);
        }
        return OAL_SUCC;
    }

    /* �����쳣�������ĸ������ø�������dmac_rx_update_dscr_queue�����н���ͳ�� */
    pst_wlan_rx_event->us_dscr_num += us_pre_dscr_num;

    /* ��ȡ���յ��ĵ�һ����������״̬ */
    pst_netbuf = oal_netbuf_peek(pst_netbuf_header);
    if (OAL_PTR_NULL == pst_netbuf)
    {
        OAM_ERROR_LOG0(0, OAM_SF_TX, "{dmac_rx_get_dscr_list::pst_netbuf null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    uc_rx_dscr_status = ((dmac_rx_ctl_stru *)(oal_netbuf_cb(pst_netbuf)))->st_rx_status.bit_dscr_status;

    /* �������쳣:���������������쳣 */
    if (OAL_UNLIKELY(HAL_RX_NEW == uc_rx_dscr_status))
    {
        OAM_WARNING_LOG0(uc_vap_id, OAM_SF_RX, "{dmac_rx_get_dscr_list::the rx_state of the base dscr is HAL_RX_NEW.}");

        dmac_rx_free_netbuf_list(pst_netbuf_header, &pst_netbuf, pst_wlan_rx_event->us_dscr_num);
#if(_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
        pst_vap = (mac_vap_stru *)mac_res_get_dmac_vap(uc_vap_id);
        if (OAL_PTR_NULL != pst_vap)
        {
           /* ����ͳ�� */
            pst_vap->st_vap_stats.ul_rx_first_dscr_excp_dropped += pst_wlan_rx_event->us_dscr_num;
        }
#endif
        return OAL_FAIL;
    }

    return OAL_SUCC;
}


oal_void  dmac_rx_process_data_mgmt_event(frw_event_mem_stru *pst_event_mem,
                                       oal_netbuf_head_stru  *pst_netbuf_header,
                                       mac_vap_stru          *pst_vap,
                                       oal_netbuf_stru       *pst_netbuf)
{
    frw_event_stru                     *pst_event;
    frw_event_hdr_stru                 *pst_event_hdr;
    hal_wlan_rx_event_stru             *pst_wlan_rx_event;
    hal_to_dmac_device_stru            *pst_device;
    dmac_wlan_drx_event_stru           *pst_wlan_rx;
    oal_uint32                          ul_rslt;
    oal_netbuf_stru                    *pst_first_buf = OAL_PTR_NULL;
    dmac_vap_stru                      *pst_dmac_vap;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_vap))
    {
        OAM_WARNING_LOG0(0, OAM_SF_RX, "{dmac_rx_process_data_mgmt_event::pst_vap null.}");

        pst_first_buf = oal_netbuf_peek(pst_netbuf_header);
        if (OAL_PTR_NULL == pst_first_buf)
        {
            OAM_ERROR_LOG0(0, OAM_SF_RX, "{dmac_rx_process_data_mgmt_event::pst_first_buf null.}");
            return;
        }

        /* �쳣 �ͷ���Դ */
        dmac_rx_free_netbuf_list(pst_netbuf_header, &pst_first_buf, (oal_uint16)oal_netbuf_get_buf_num(pst_netbuf_header));

        return;
    }

    pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(pst_vap->uc_vap_id, OAM_SF_RX, "{dmac_rx_process_data_mgmt_event::pst_dmac_vap null.}");
        return;
    }
    /* ��ȡ�¼�ͷ���¼��ṹ��ָ�� */
    pst_event               = frw_get_event_stru(pst_event_mem);
    pst_event_hdr           = &(pst_event->st_event_hdr);
    pst_wlan_rx_event       = (hal_wlan_rx_event_stru *)(pst_event->auc_event_data);
    pst_device              = pst_wlan_rx_event->pst_hal_device;

    /* DMAC�׵�HMAC��VAP ID�ı������VAP��ID */
    pst_event_hdr->uc_vap_id = pst_vap->uc_vap_id;

    /* ����֡���� */
    if (FRW_EVENT_TYPE_WLAN_DRX == pst_event_hdr->en_type)
    {
        /* ˫оƬ�£�0��1��������vap id�����������Ҫ����ҵ��vap ��ʵid���������vap mac numֵ�����ж� */
        if (oal_board_get_service_vap_start_id() > pst_vap->uc_vap_id || WLAN_VAP_SUPPORT_MAX_NUM_LIMIT <= pst_vap->uc_vap_id)
        {
            OAM_ERROR_LOG1(0, OAM_SF_RX, "{dmac_rx_process_data_mgmt_event::pst_event_mem vap[%d].}", pst_vap->uc_vap_id);
        }

        /* �����һ��NETBUF�ÿ� */
        oal_set_netbuf_next((oal_netbuf_tail(pst_netbuf_header)), OAL_PTR_NULL);

        /* �̳��¼��������޸��¼�ͷ */
        FRW_EVENT_HDR_MODIFY_PIPELINE_AND_SUBTYPE(pst_event_hdr, DMAC_WLAN_DRX_EVENT_SUB_TYPE_RX_DATA);

        dmac_vap_linkloss_clean(pst_dmac_vap);
        dmac_vap_linkloss_threshold_incr(pst_dmac_vap);

        pst_wlan_rx = (dmac_wlan_drx_event_stru *)(pst_event->auc_event_data);
        pst_wlan_rx->pst_netbuf    = oal_netbuf_peek(pst_netbuf_header);
        pst_wlan_rx->us_netbuf_num = (oal_uint16)oal_netbuf_get_buf_num(pst_netbuf_header);

        //OAM_INFO_LOG0(pst_vap->uc_vap_id, OAM_SF_RX, "{dmac_rx_process_data_event::dispatch to hmac.}\r\n");
        /* ����ppsͳ�� */
        dmac_auto_freq_pps_count(pst_wlan_rx->us_netbuf_num);
        OAL_MIPS_RX_STATISTIC(DMAC_PROFILING_FUNC_RX_DMAC_HANDLE_PREPARE_EVENT);
        OAM_PROFILING_RX_STATISTIC(OAM_PROFILING_FUNC_RX_DMAC_HANDLE_PREPARE_EVENT);

        /* �ַ��¼� */
        ul_rslt = frw_event_dispatch_event(pst_event_mem);
        if(ul_rslt != OAL_SUCC)
        {
             OAM_ERROR_LOG1(pst_vap->uc_vap_id, OAM_SF_RX, "{dmac_rx_process_data_mgmt_event::frw_event_dispatch_event fail[%d].}", ul_rslt);
             dmac_rx_free_netbuf_list(pst_netbuf_header, &pst_first_buf, (oal_uint16)oal_netbuf_get_buf_num(pst_netbuf_header));
        }


        OAL_MIPS_RX_STATISTIC(DMAC_PROFILING_FUNC_RX_DMAC_END);
#ifdef _PRE_WLAN_PROFLING_MIPS
        oal_profiling_stop_rx_save();
#endif
    }
    else    /* ����֡�Ĵ��� */
    {
        /* �˴�while����:�������֡���ж� */
        while (OAL_FALSE == oal_netbuf_list_empty(pst_netbuf_header))
        {
            DMAC_VAP_DFT_STATS_PKT_INCR(pst_dmac_vap->st_query_stats.ul_rx_mgmt_mpdu_num_cnt, 1);
            pst_netbuf = oal_netbuf_delist(pst_netbuf_header);
            if (OAL_PTR_NULL == pst_netbuf)
            {
                OAM_ERROR_LOG0(0, OAM_SF_RX, "{dmac_rx_process_data_mgmt_event::pst_netbuf null.}");
                return;
            }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
            OAL_MEM_NETBUF_TRACE(pst_netbuf, OAL_TRUE);
#endif
            /* ���ù���֡�ӿ� */
            ul_rslt = dmac_rx_process_mgmt(pst_event_mem, pst_event_hdr, pst_device, pst_netbuf);
            if (ul_rslt != OAL_SUCC)
            {
//                OAM_WARNING_LOG1(pst_vap->uc_vap_id, OAM_SF_RX, "{dmac_rx_process_data_event::dmac_rx_process_mgmt failed[%d].", ul_rslt);
                OAL_IO_PRINT("\r\n dmac_rx_process_data_event::dmac_rx_process_mgmt failed[%u].\r\n", ul_rslt);
                oal_netbuf_free(pst_netbuf);
                DMAC_VAP_DFT_STATS_PKT_INCR(pst_dmac_vap->st_query_stats.ul_rx_mgmt_abnormal_dropped, 1);
            }
        }
    }

}
#ifndef _PRE_WLAN_PROFLING_MIPS
#if (_PRE_OS_VERSION_RAW == _PRE_OS_VERSION)  && defined (__CC_ARM)
#pragma arm section rodata, code, rwdata, zidata  // return to default placement
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_HILINK


oal_uint32  dmac_fbt_scan_rx_process_frame(hal_to_dmac_device_stru *pst_hal_device, dmac_rx_ctl_stru *pst_cb_ctrl)
{
    mac_ieee80211_frame_stru           *pst_frame_hdr;
    oal_uint8                          *puc_transmit_addr;
    oal_uint8                          *puc_bssid_addr;
    oal_uint8                           uc_user_index;
    mac_fbt_scan_mgmt_stru             *pst_fbt_scan_mgmt;
    mac_fbt_scan_result_stru           *pst_fbt_scan_result;
    mac_device_stru                    *pst_mac_device;
    oal_uint8                           auc_bssid[WLAN_MAC_ADDR_LEN];  /* sta������ap mac��ַ */

    /* ��ȡdevcie����ʵ�� */
    pst_mac_device = mac_res_get_dev(pst_hal_device->uc_mac_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RX, "{dmac_fbt_scan_rx_process_frame::pst_device null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    if (OAL_PTR_NULL == pst_cb_ctrl)
    {
        OAM_ERROR_LOG0(pst_mac_device->uc_device_id, OAM_SF_RX, "{dmac_fbt_scan_rx_process_frame::pst_device null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }


    /* ���fbt scan����û�п�����ֱ�ӷ��� */
    if (OAL_FALSE == pst_mac_device->st_fbt_scan_mgmt.uc_fbt_scan_enable)
    {
        return OAL_FAIL;
    }

    /* ��ȡ802.11ͷָ�롢���Ͷ˵�ַ */
    pst_frame_hdr     = (mac_ieee80211_frame_stru *)(mac_get_rx_cb_mac_hdr(&(pst_cb_ctrl->st_rx_info)));
    puc_transmit_addr = pst_frame_hdr->auc_address2;

    if (ETHER_IS_MULTICAST(puc_transmit_addr))
    {
        return OAL_FAIL;
    }

    /* ��ȡmac vapʵ���е�fbt����ʵ�� */
    pst_fbt_scan_mgmt = &(pst_mac_device->st_fbt_scan_mgmt);

    /* �����sta����FBT scan�û������б��ڣ�Ҳֱ�ӷ��أ���������*/
    for (uc_user_index = 0; uc_user_index < HMAC_FBT_MAX_USER_NUM; uc_user_index++)
    {
        if (OAL_FALSE == (pst_fbt_scan_mgmt->ast_fbt_scan_user_list[uc_user_index].uc_is_used))
        {
            continue;
        }

        if (OAL_SUCC == oal_compare_mac_addr(pst_fbt_scan_mgmt->ast_fbt_scan_user_list[uc_user_index].auc_user_mac_addr, puc_transmit_addr))
        {
            break;
        }
    }

    if (HMAC_FBT_MAX_USER_NUM == uc_user_index)
    {
        return OAL_SUCC;
    }

    OAM_INFO_LOG4(pst_mac_device->uc_device_id, OAM_SF_HILINK, "{dmac_fbt_scan_rx_process_frame::uc_user_index = %d,puc_transmit_addr MAC[%x:%x:%x].}",
                                  uc_user_index,
                                  *(puc_transmit_addr+3),
                                  *(puc_transmit_addr+4),
                                  *(puc_transmit_addr+5));


    /* �ҵ���¼��sta FBT scan�����¼ʵ�� */
    pst_fbt_scan_result = &(pst_fbt_scan_mgmt->ast_fbt_scan_user_list[uc_user_index]);

    /* ��ȡ��sta��bssid */
    mac_get_bssid((oal_uint8 *)pst_frame_hdr, auc_bssid);
    puc_bssid_addr = auc_bssid;

    /*��¼��sta��rssi������Ľ���ʱ����Ȳ���������ǵ�һ���ҵ���������
      ��uc_scan_state״̬�޸�Ϊfound���������ŵ���bssid��rssi������ʱ����������Ӽ�����
      ����������Ѿ�����found״̬�������rssi��ʱ�����������������ŵ���bssid�����б䶯��Ҳ���²���ʾ��*/
    if (OAL_FALSE == pst_fbt_scan_result->uc_is_found)
    {
        pst_fbt_scan_result->uc_is_found = OAL_TRUE;
    }
    else
    {
        if (pst_fbt_scan_result->uc_user_channel != pst_cb_ctrl->st_rx_info.uc_channel_number)
        {
            OAM_WARNING_LOG2(pst_mac_device->uc_device_id, OAM_SF_HILINK, "{dmac_fbt_scan_rx_process_frame::user is in found-state, channel change frome [%d] to [%d].}",
                                  pst_fbt_scan_result->uc_user_channel,
                                  pst_cb_ctrl->st_rx_info.uc_channel_number);
        }

        if (0 != oal_memcmp(pst_fbt_scan_result->auc_user_bssid, puc_bssid_addr, WLAN_MAC_ADDR_LEN))
        {
            OAM_WARNING_LOG4(pst_mac_device->uc_device_id, OAM_SF_HILINK, "{dmac_fbt_scan_rx_process_frame::user is in found-state, bssid change frome [%x:%x] to [%x:%x].}",
                                  pst_fbt_scan_result->auc_user_bssid[4],
                                  pst_fbt_scan_result->auc_user_bssid[5],
                                  auc_bssid[4],
                                  auc_bssid[5]);
        }
    }

    /* �����û����ŵ��ź�bssid */
    pst_fbt_scan_result->uc_user_channel = pst_cb_ctrl->st_rx_info.uc_channel_number;
    oal_memcopy(pst_fbt_scan_result->auc_user_bssid, puc_bssid_addr, WLAN_MAC_ADDR_LEN);

    /* �����û���RSSIͳ����Ϣ */
    oal_rssi_smooth(&(pst_fbt_scan_result->s_rssi), pst_cb_ctrl->st_rx_statistic.c_rssi_dbm);
    pst_fbt_scan_result->ul_rssi_timestamp = (oal_uint32)OAL_TIME_GET_STAMP_MS();

    pst_fbt_scan_result->ul_total_pkt_cnt++;

    OAM_INFO_LOG4(pst_mac_device->uc_device_id, OAM_SF_HILINK, "{dmac_fbt_scan_rx_process_frame::chan = %d,frame bssid MAC[%x:%x:%x].}",
                                                    pst_cb_ctrl->st_rx_info.uc_channel_number,
                                                    *(puc_bssid_addr+3),
                                                    *(puc_bssid_addr+4),
                                                    *(puc_bssid_addr+5));

    OAM_INFO_LOG4(pst_mac_device->uc_device_id, OAM_SF_HILINK, "{dmac_fbt_scan_rx_process_frame::new rssi = %d pkt_cnt=%d bssid[%x:%x].}",
                                                    (oal_uint8)pst_cb_ctrl->st_rx_statistic.c_rssi_dbm,
                                                    pst_fbt_scan_result->ul_total_pkt_cnt,
                                                    pst_fbt_scan_result->auc_user_bssid[4],
                                                    pst_fbt_scan_result->auc_user_bssid[5]);
    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_MAC_BUGFIX_SW_CTRL_RSP

oal_void dmac_rx_set_rsp_rate_by_mgmt(mac_rx_ctl_stru    *pst_cb_ctrl)
{
    mac_vap_stru                    *pst_mac_vap = OAL_PTR_NULL;
    dmac_device_stru                *pst_dmac_dev = OAL_PTR_NULL;

    if (OAL_PTR_NULL == pst_cb_ctrl)
    {
        OAM_WARNING_LOG0(0, OAM_SF_RX, "{pst_cb_ctrl is NULL, fail to set rsp frm datarate}");
        return;
    }

    pst_mac_vap = mac_res_get_mac_vap(pst_cb_ctrl->uc_mac_vap_id);
    if (OAL_PTR_NULL == pst_mac_vap)
    {
        OAM_WARNING_LOG1(0, OAM_SF_RX, "{pst_mac_vap is NULL, fail to set rsp frm datarate, vap id[%d]}", pst_cb_ctrl->st_rx_info.uc_mac_vap_id);
        return;
    }

    pst_dmac_dev = dmac_res_get_mac_dev(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_dmac_dev)
    {
        OAM_WARNING_LOG1(0, OAM_SF_RX, "{pst_dmac_dev is NULL, fail to set rsp frm datarate, dev id[%d]}", pst_mac_vap->uc_device_id);
        return;
    }

    hal_cfg_rsp_dyn_bw(OAL_TRUE, pst_dmac_dev->en_usr_bw_mode);
    if (WLAN_PHY_RATE_6M != pst_dmac_dev->uc_rsp_frm_rate_val)
    {
        pst_dmac_dev->uc_rsp_frm_rate_val = WLAN_PHY_RATE_6M;
        hal_set_rsp_rate((oal_uint32)pst_dmac_dev->uc_rsp_frm_rate_val);
    }
}


oal_void dmac_rx_set_rsp_rate(oal_uint8 uc_device_id, dmac_rx_ctl_stru *pst_cb_ctrl, frw_event_type_enum_uint8 en_type)
{
    dmac_device_stru                   *pst_dmac_dev = OAL_PTR_NULL;

    if (OAL_PTR_NULL == pst_cb_ctrl)
    {
        return;
    }

    pst_dmac_dev = dmac_res_get_mac_dev(uc_device_id);
    if (OAL_PTR_NULL == pst_dmac_dev)
    {
        OAM_WARNING_LOG1(0, OAM_SF_TX, "{dmac_rx_set_rsp_rate::pst_dmac_dev null.dev id [%d]}", uc_device_id);
        return;
    }

    /* �����˶�̬����ʱ�����ݽ����������ʵ�����Ӧ֡���� */
    if (OAL_TRUE == pst_dmac_dev->en_state_in_sw_ctrl_mode)
    {
        if (FRW_EVENT_TYPE_WLAN_DRX == en_type)
        {
            dmac_vap_update_rsp_frm_rate(pst_dmac_dev,
                                        pst_cb_ctrl->st_rx_statistic.un_nss_rate.st_vht_nss_mcs.bit_protocol_mode,
                                        pst_cb_ctrl->st_rx_status.bit_freq_bandwidth_mode,
                                        pst_cb_ctrl->st_rx_statistic.un_nss_rate.st_vht_nss_mcs.bit_vht_mcs);
        }
        else if (FRW_EVENT_TYPE_WLAN_CRX == en_type)
        {
            dmac_rx_set_rsp_rate_by_mgmt(pst_cb_ctrl);
        }
    }
}

#endif


oal_uint32 dmac_rx_data_dscr_bw_process(frw_event_hdr_stru *pst_event_hdr, mac_vap_stru *pst_vap, dmac_rx_ctl_stru *pst_rx_ctl)
{
    dmac_user_stru             *pst_dmac_user   = OAL_PTR_NULL;
    mac_user_stru              *pst_mac_user    = OAL_PTR_NULL;

    if ((OAL_PTR_NULL == pst_event_hdr) || (OAL_PTR_NULL == pst_vap) || (OAL_PTR_NULL == pst_rx_ctl))
    {
        OAM_ERROR_LOG0(0, OAM_SF_RX, "{dmac_rx_data_dscr_bw_process::param null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ����У��״̬��BW_FSM�ϱ���������жϴ�����Ϣͳ�� */
    if (!(IS_LEGACY_STA(pst_vap) && MAC_VAP_BW_FSM_VERIFY(pst_vap)))
    {
        return OAL_SUCC;
    }

    /* ����֡����Ҫ */
    if (FRW_EVENT_TYPE_WLAN_DRX != pst_event_hdr->en_type)
    {
        return OAL_SUCC;
    }

    /* ��ȡ�û���֡��Ϣ */
    pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user(MAC_GET_RX_CB_TA_USER_IDX(&pst_rx_ctl->st_rx_info));
    if (OAL_PTR_NULL == pst_dmac_user)
    {
        OAM_WARNING_LOG1(pst_event_hdr->uc_vap_id, OAM_SF_RX,
                         "{dmac_rx_data_dscr_bw_process::pst_dmac_user[%d] null.}", MAC_GET_RX_CB_TA_USER_IDX(&pst_rx_ctl->st_rx_info));
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* �㲥�û�����Ҫ�ϱ� */
    pst_mac_user = (mac_user_stru *)(&(pst_dmac_user->st_user_base_info));
    if (OAL_TRUE == pst_mac_user->en_is_multi_user)
    {
        return OAL_SUCC;
    }

    /* ����У��״̬��BW_FSM�ϱ���������жϴ�����Ϣͳ�� */
    dmac_sta_bw_switch_fsm_post_event((dmac_vap_stru *)pst_vap, DMAC_STA_BW_SWITCH_EVENT_RX_UCAST_DATA_COMPLETE,
                                       OAL_SIZEOF(dmac_rx_ctl_stru), (oal_uint8 *)pst_rx_ctl);
    return OAL_SUCC;
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

OAL_STATIC oal_void dmac_rx_update_user_eapol_key_open(mac_vap_stru *pst_mac_vap, oal_netbuf_stru *pst_netbuf)
{
    dmac_rx_ctl_stru       *pst_rx_cb;
    dmac_user_stru         *pst_dmac_user;
    mac_eapol_header_stru  *pst_eapol_header;

    pst_rx_cb = (dmac_rx_ctl_stru *)OAL_NETBUF_CB(pst_netbuf);

    if (IS_STA(pst_mac_vap)
        && pst_rx_cb->st_rx_status.bit_dscr_status == HAL_RX_SUCCESS)
    {
        pst_eapol_header = (mac_eapol_header_stru *)(oal_netbuf_payload(pst_netbuf) + OAL_SIZEOF(mac_llc_snap_stru));
        if (mac_is_eapol_key_ptk(pst_eapol_header) == OAL_TRUE)
        {
            pst_dmac_user = mac_res_get_dmac_user(pst_mac_vap->us_assoc_vap_id);
            if (pst_dmac_user == OAL_PTR_NULL)
            {
                OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_RX,
                            "{dmac_rx_update_user_eapol_key_open::dmac_user is null. user_id %d}",
                            pst_mac_vap->us_assoc_vap_id);
                return;
            }
            pst_dmac_user->bit_is_rx_eapol_key_open
                          = (pst_rx_cb->st_rx_status.bit_cipher_protocol_type == HAL_NO_ENCRYP);

            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_RX, "dmac_rx_update_user_eapol_key_open::bit_is_rx_eapol_key_open=%d", pst_dmac_user->bit_is_rx_eapol_key_open);
        }
    }

    return;
}
#endif

#ifndef _PRE_WLAN_PROFLING_MIPS
#if (_PRE_OS_VERSION_RAW == _PRE_OS_VERSION)  && defined (__CC_ARM)
#pragma arm section rwdata = "BTCM", code ="ATCM", zidata = "BTCM", rodata = "ATCM"
#endif
#endif
#ifdef _PRE_WLAN_FEATURE_CSI


oal_void dmac_get_csi_info_len(hal_to_dmac_device_stru *pst_hal_device,wlan_channel_bandwidth_enum_uint8 en_bandwidth, oal_uint8 uc_frame_type, oal_uint32 *pul_csi_info_len)
{

    if(WLAN_LEGACY_OFDM_PHY_PROTOCOL_MODE == uc_frame_type)
    {
        *pul_csi_info_len = OAL_SIZEOF(dmac_csi_info_stru) * DMAC_CSI_20M_NONHT_NUM;
    }
    else if(WLAN_HT_PHY_PROTOCOL_MODE <= uc_frame_type)
    {
        if(en_bandwidth == WLAN_BAND_WIDTH_20M)
        {
            *pul_csi_info_len = OAL_SIZEOF(dmac_csi_info_stru) * DMAC_CSI_20M_SUBCARRIER_NUM;
        }
        else if((en_bandwidth == WLAN_BAND_WIDTH_40PLUS) || (en_bandwidth == WLAN_BAND_WIDTH_40MINUS))
        {
            *pul_csi_info_len = OAL_SIZEOF(dmac_csi_info_stru) * DMAC_CSI_40M_SUBCARRIER_NUM;
        }
        else
        {
            *pul_csi_info_len = OAL_SIZEOF(dmac_csi_info_stru) * DMAC_CSI_80M_SUBCARRIER_NUM;
        }
    }
    else
    {
        *pul_csi_info_len = 0;
    }

    OAM_WARNING_LOG2(0, OAM_SF_ANY, "{dmac_get_csi_info_len::bw = %d, csi_info_len = %d.}",en_bandwidth,*pul_csi_info_len);

}


oal_void dmac_reset_csi_sample(hal_to_dmac_device_stru *pst_hal_device)
{
    hal_set_csi_en(pst_hal_device, OAL_FALSE);
    hal_disable_csi_sample(pst_hal_device);
    hal_free_csi_sample_mem(pst_hal_device);
}

#if 1
oal_uint32  dmac_csi_data_handler(hal_to_dmac_device_stru *pst_hal_device)
{
    oal_uint16                            us_finish_state;
    dmac_vap_stru                        *pst_dmac_vap = OAL_PTR_NULL;

#if 1
    oal_uint8                             uc_up_vap_num;
    //oal_uint8                             uc_vap_index;
    oal_uint8                             auc_mac_vap_id[WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE] = {0};
    //mac_vap_stru                       *pst_mac_vap = OAL_PTR_NULL;
    //oal_uint8                            *puc_addr = OAL_PTR_NULL;
    //mac_user_stru                        *pst_mac_user;
    //oal_dlist_head_stru                  *pst_entry;
    //oal_dlist_head_stru                  *pst_next_entry;
#endif
    oal_uint32                            ul_csi_info_len;
    oal_uint32                            ul_start_len;
    oal_uint32                            ul_second_len;
    oal_uint8                             auc_ta_addr[WLAN_MAC_ADDR_LEN];
    oal_uint32                           *pul_csi_end_addr;
    oal_uint32                           *pul_pktmem_start_addr;
    oal_uint32                           *pul_pktmem_end_addr;
    oal_uint32                           *pul_csi_start_addr;
    oal_uint32                           *pul_csi_second_start_addr;

    wlan_channel_bandwidth_enum_uint8     en_bandwidth;
    oal_uint8                             uc_frame_type;
    oal_uint8                             uc_mac_frame_type;

    oal_uint8                             uc_he_flag;

    if (OAL_PTR_NULL == pst_hal_device)
    {
       OAM_ERROR_LOG0(0, OAM_SF_ANY, "{dmac_csi_data_handler::pst_hal_device null.}");
       return OAL_ERR_CODE_PTR_NULL;
    }
    guc_csi_rx_interrupt = 0;

    /* �������ָʾ */
    hal_get_sample_state(pst_hal_device, &us_finish_state);
    /*packetram ����Ϊ���߷���*/
    hal_set_pktmem_bus_access(pst_hal_device);

    /* ��ȡCSI�ϱ����--MAC */
    hal_get_mac_csi_info(pst_hal_device);
    /* ��ȡCSI TA */
    hal_get_mac_csi_ta(pst_hal_device,auc_ta_addr);

    /* ��ȡCSI�ϱ����--PHY */
    hal_get_phy_csi_info(pst_hal_device, &en_bandwidth, &uc_frame_type);

    hal_get_csi_frame_type(pst_hal_device, &uc_he_flag, &uc_mac_frame_type);
    if((0 == uc_he_flag) && (uc_frame_type != uc_mac_frame_type))
    {

        OAM_WARNING_LOG3(0, OAM_SF_ANY, "{dmac_csi_data_handler::NO!!uc_he_flag = %d,uc_frame_type = %d, uc_mac_frame_type = %d.}",
                         uc_he_flag, uc_frame_type, uc_mac_frame_type);
    }

    /* ��ȡCSI�ɼ����� */
    dmac_get_csi_info_len(pst_hal_device,en_bandwidth, uc_mac_frame_type, &ul_csi_info_len);
    if(0 == ul_csi_info_len)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{dmac_csi_data_handler::NO!!ul_csi_info_len = %d .}",ul_csi_info_len);
        dmac_reset_csi_sample(pst_hal_device);
        return OAL_SUCC;
    }

    if(1 == us_finish_state)
    {
        /* ��ȡCSI��Ϣ�ϱ���ʼ�ͽ�����ַ,��Ҫ��֤8�ֽڶ�����? */
        hal_get_csi_end_addr(pst_hal_device, &pul_csi_end_addr);
        if(OAL_PTR_NULL == pul_csi_end_addr)
        {
            OAM_ERROR_LOG0(0, OAM_SF_ANY, "{dmac_csi_data_handler::end_addr get failed! .}");
            dmac_reset_csi_sample(pst_hal_device);
            return OAL_ERR_CODE_PTR_NULL;
        }

        hal_get_pktmem_start_addr(pst_hal_device, &pul_pktmem_start_addr);
        if(OAL_PTR_NULL == pul_pktmem_start_addr)
        {
            OAM_ERROR_LOG0(0, OAM_SF_ANY, "{dmac_csi_data_handler::pktmem_start_addr get failed! .}");
            dmac_reset_csi_sample(pst_hal_device);
            return OAL_ERR_CODE_PTR_NULL;
        }
        hal_get_pktmem_end_addr(pst_hal_device, &pul_pktmem_end_addr);
        if(OAL_PTR_NULL == pul_pktmem_end_addr)
        {
            OAM_ERROR_LOG0(0, OAM_SF_ANY, "{dmac_csi_data_handler::pktmem_end_addr get failed! .}");
            dmac_reset_csi_sample(pst_hal_device);
            return OAL_ERR_CODE_PTR_NULL;
        }
        uc_up_vap_num = hal_device_find_all_up_vap(pst_hal_device, auc_mac_vap_id);
        if(uc_up_vap_num == 1)
        {
            pst_dmac_vap = mac_res_get_dmac_vap(auc_mac_vap_id[0]);
            if (OAL_PTR_NULL == pst_dmac_vap)
            {
                OAM_ERROR_LOG1(0, OAM_SF_ANY, "{dmac_csi_data_handler::pst_dmac_vap null, vap id is %d.}",
                               auc_mac_vap_id[0]);
                dmac_reset_csi_sample(pst_hal_device);
                return OAL_PTR_NULL;
            }

        }

        ul_start_len = (oal_uint32)((oal_uint32)((pul_csi_end_addr - pul_pktmem_start_addr) + 2) * OAL_SIZEOF(oal_uint32));
        //OAM_WARNING_LOG1(0, OAM_SF_ANY, "{dmac_csi_data_handler::ul_start_len = %d.}", ul_start_len);
        //ul_csi_info_len = (ul_csi_info_len > OAL_SIZEOF(ast_csi_20M))? OAL_SIZEOF(ast_csi_20M): ul_csi_info_len;
        if(ul_start_len >= ul_csi_info_len)
        {
            pul_csi_start_addr = (pul_csi_end_addr - (ul_csi_info_len >> 2)) + 2;

            OAM_WARNING_LOG3(0, OAM_SF_ANY, "{dmac_csi_data_handler::csi_start_addr = 0x%p,csi_end_addr = 0x%p, len = %d.}",
               pul_csi_start_addr, pul_csi_end_addr, ul_csi_info_len);

            guc_csi_loop_flag = 0;
            /* ��ȡCSI������Ϣ---��ȡ�ڴ��ַ,���ϱ���hmac */
            dmac_location_csi_process(pst_dmac_vap, auc_ta_addr, ul_csi_info_len, &pul_csi_start_addr);
        }
        else
        {
             ul_second_len =  ul_csi_info_len - ul_start_len;
             pul_csi_second_start_addr = pul_pktmem_end_addr + 2 - (ul_second_len >> 2);
             OAM_WARNING_LOG3(0, OAM_SF_ANY, "{dmac_csi_data_handler::ul_start_len = %d,ul_second_len = %d, csi_second_addr = 0x%p.}",
                    ul_start_len, ul_second_len, pul_csi_second_start_addr);
             guc_csi_loop_flag = 1;
             /* ��ȡCSI������Ϣ---��ȡ�ڴ��ַ,���ϱ���hmac */
             dmac_location_csi_process(pst_dmac_vap, auc_ta_addr, ul_second_len, &pul_csi_second_start_addr);
             guc_csi_loop_flag = 2;
             dmac_location_csi_process(pst_dmac_vap, auc_ta_addr, ul_start_len, &pul_pktmem_start_addr);

        }
    }
#if 0
    for (uc_vap_index = 0; uc_vap_index < uc_up_vap_num; uc_vap_index++)
    {
        pst_dmac_vap = mac_res_get_dmac_vap(auc_mac_vap_id[uc_vap_index]);
        if (OAL_PTR_NULL == pst_dmac_vap)
        {
            OAM_ERROR_LOG1(0, OAM_SF_SCAN, "{dmac_csi_data_handler::pst_dmac_vap null, vap id is %d.}",
                           auc_mac_vap_id[uc_vap_index]);
            continue;
        }
        OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_entry, pst_next_entry, &(pst_dmac_vap->st_vap_base_info.st_mac_user_list_head))
        {
            pst_mac_user = OAL_DLIST_GET_ENTRY(pst_entry, mac_user_stru, st_user_dlist);

             if (OAL_PTR_NULL == pst_mac_user)
            {
                OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{dmac_csi_data_handler::pst_mac_user null.}");
                continue;
            }
            if (0 == oal_memcmp(pst_mac_user->auc_user_mac_addr, puc_addr, WLAN_MAC_ADDR_LEN))
            {
                /* ��ȡCSI������Ϣ---��ȡ�ڴ��ַ,���ϱ���hmac */
                dmac_location_csi_process(pst_dmac_vap, puc_addr, ul_csi_info_len, pul_csi_start_addr);

                break;
            }

         }

    }
#endif
    pst_hal_device->uc_csi_status = OAL_FALSE;
    dmac_reset_csi_sample(pst_hal_device);

    return OAL_SUCC;

}
#endif
#endif

oal_uint32  dmac_rx_process_data_event(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru                     *pst_event;
    frw_event_hdr_stru                 *pst_event_hdr;
    hal_wlan_rx_event_stru             *pst_wlan_rx_event;
    hal_to_dmac_device_stru            *pst_device;
    oal_uint16                          us_dscr_num;                    /* �洢�ж����������������� */
    oal_netbuf_head_stru                st_netbuf_header;               /* �洢����HMACģ��sk_buf�����headerָ�� */
    oal_netbuf_stru                    *pst_curr_netbuf;                /* �洢���ڴ����SK_BUF */
    dmac_rx_ctl_stru                   *pst_cb_ctrl;                    /* ָ��SK_BUF��cb�ֶε�ָ�� */
    mac_vap_stru                       *pst_vap          = OAL_PTR_NULL;
    oal_uint8                           uc_vap_id        = 0xFF;        /* ���汾�δ����VAP ID */
    dmac_rx_frame_ctrl_enum_uint8       en_frame_ctrl    = DMAC_RX_FRAME_CTRL_BUTT;
    oal_uint32                          ul_netbuf_index;
    oal_netbuf_stru                    *pst_netbuf = OAL_PTR_NULL;
    hal_rx_dscr_queue_id_enum_uint8     en_dscr_queue_id;
    oal_uint32                          ul_rslt;
    oal_uint8                           uc_frame_type;
    oal_uint8                           uc_frame_subtype;
    oal_dlist_head_stru                *pst_rx_isr_list;
    hal_hw_rx_dscr_info_stru           *pst_rx_isr_info;
    mac_ieee80211_frame_stru           *pst_frame_hdr;
    dmac_vap_stru                      *pst_dmac_vap;
#if defined(_PRE_DEBUG_MODE) || (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    oal_uint8                           uc_data_type = MAC_DATA_BUTT;
#endif
    mac_rx_ctl_stru                     *pst_rx_info;
#ifdef _PRE_WLAN_FEATURE_DBAC
    mac_device_stru                    *pst_mac_device;
#endif
    oal_uint                            ul_flag = 0;
#ifdef _PRE_DEBUG_MODE
    pkt_trace_type_enum_uint8           en_trace_pkt_type;
    oal_uint8                          *puc_payload;
#endif
#ifdef _PRE_WLAN_FEATURE_PACKET_CAPTURE
    dmac_device_stru                   *pst_dmac_device;
#endif
    oal_bool_enum_uint8                 en_pkt_cap_sw    = OAL_FALSE;

    oal_netbuf_head_stru                st_part_netbuf[WLAN_RX_INTERRUPT_MAX_NUM_PER_DEVICE];/*��Ҫ��༸��netbuf�����ϱ�*/
    mac_vap_stru                       *pst_part_vap     = OAL_PTR_NULL;
    oal_bool_enum_uint8                 en_is_first_dscr = OAL_TRUE;
    oal_uint8                           uc_irq_index     = 0;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    oal_uint8                           uc_isr_info_cnt[HAL_RX_DSCR_QUEUE_ID_BUTT] = {0};
    oal_uint8                           uc_queue_idx = 0;
#endif
#if (_PRE_PRODUCT_ID_HI1151 != _PRE_PRODUCT_ID)
#ifdef _PRE_DEBUG_MODE
    oal_uint32                          ul_profingling_time1;
    oal_uint32                          ul_profingling_time2;

    ul_profingling_time1 = oal_5115timer_get_10ns();
#endif
#endif

    OAM_PROFILING_RX_STATISTIC(OAM_PROFILING_FUNC_RX_DMAC_START);

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(0, OAM_SF_RX, "{dmac_rx_process_data_event::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡ�¼�ͷ���¼��ṹ��ָ�� */
    pst_event               = frw_get_event_stru(pst_event_mem);
    pst_event_hdr           = &(pst_event->st_event_hdr);
    pst_wlan_rx_event       = (hal_wlan_rx_event_stru *)(pst_event->auc_event_data);
    pst_device              = pst_wlan_rx_event->pst_hal_device;

    if (OAL_PTR_NULL == pst_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RX, "{dmac_rx_process_data_event::pst_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡ���������¼�RX QUEUE ID */
    en_dscr_queue_id = (FRW_EVENT_TYPE_WLAN_DRX == pst_event_hdr->en_type) ? HAL_RX_DSCR_NORMAL_PRI_QUEUE : HAL_RX_DSCR_HIGH_PRI_QUEUE;
#if 1
#ifdef _PRE_WLAN_FEATURE_CSI
    /* CSI�ж�����ĵڶ�����������ж϶�ȡ���� */
    if(OAL_TRUE == pst_device->uc_csi_status)
    {
        guc_csi_rx_interrupt++;
        if(2 <= guc_csi_rx_interrupt)
        {
            dmac_csi_data_handler(pst_device);
        }
    }
#endif
#endif

    /* ��ʼ��netbuf�����ͷ */
    oal_netbuf_list_head_init(&st_netbuf_header);
    /* ��ʼ����һ������hmac netbuf�����ͷ */
    oal_netbuf_list_head_init(&(st_part_netbuf[0]));

    pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_event_hdr->uc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dmac_vap))
    {
        OAM_ERROR_LOG0(pst_event_hdr->uc_vap_id, OAM_SF_RX, "{dmac_rx_process_data_event::pst_dmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

#ifdef _PRE_WLAN_FEATURE_STA_PM
    /* idle״̬�²���һ�����pa,����pa�Ż���destroy rx irq list�Լ�rx dscr*/
    if ((HAL_DEVICE_IDLE_STATE == GET_HAL_DEVICE_STATE(pst_device)) && (OAL_FALSE == pst_device->en_is_mac_pa_enabled))
    {
        OAM_ERROR_LOG2(pst_event_hdr->uc_vap_id, OAM_SF_RX, "{dmac_rx_process_data_event:: device[%d]idle state rx dscr type[%d]wrong!!!.}",
                    pst_device->uc_device_id, pst_event_hdr->en_type);
    }
#endif

    /* ����֡ʹ���������������ƹ�Ҳ��� */
    if (FRW_EVENT_TYPE_WLAN_DRX == pst_event_hdr->en_type)
    {
        OAL_MIPS_RX_STATISTIC(DMAC_PROFILING_FUNC_RX_DMAC_START);

        pst_wlan_rx_event->us_dscr_num = 0;

        /*�л�Ping-pong����ʱ��Ҫ��סMac rx�ж�*/
        /*AMPϵͳ��סCPU�ж�����MIPS���٣���SMP������סMAC RX�ж�!*/
        oal_irq_save(&ul_flag, OAL_5115IRQ_DMSC);
        hal_mask_interrupt(pst_device, 0);

        /* ��ȡ��Ҫ��������� */
        pst_rx_isr_list = &(pst_device->ast_rx_isr_info_list[pst_device->uc_current_rx_list_index]);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        /* �������洢�����ж���Ϣ������ */
        pst_device->uc_current_rx_list_index = (pst_device->uc_current_rx_list_index + 1) & HAL_HW_MAX_RX_DSCR_LIST_IDX;
#else
        // ��ping/pongʱ����Ҫ�ڹ��ж�ʱdelte head
        pst_rx_isr_info = !oal_dlist_is_empty(pst_rx_isr_list) ? (hal_hw_rx_dscr_info_stru *)oal_dlist_delete_head(pst_rx_isr_list) : OAL_PTR_NULL;
#endif

        hal_unmask_interrupt(pst_device, 0);
        oal_irq_restore(&ul_flag, OAL_5115IRQ_DMSC);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        dmac_rx_update_max_dscr_usage(pst_device);
        while (!oal_dlist_is_empty(pst_rx_isr_list))
#else
        if (pst_rx_isr_info)
#endif
        {
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
            pst_rx_isr_info = (hal_hw_rx_dscr_info_stru *)oal_dlist_delete_head(pst_rx_isr_list);
#endif

            /* ���±����ж���Ҫ���������������Ϣ */
            pst_wlan_rx_event->us_dscr_num  += pst_rx_isr_info->us_dscr_num;
            OAL_MIPS_RX_STATISTIC(DMAC_PROFILING_FUNC_RX_DMAC_GET_INTR_INFO_FROM_LIST);

            /* ����Ҫ�������������������������ժȡ���������ҽ��������黹������������ */
            ul_rslt = dmac_rx_get_dscr_list(pst_device, pst_rx_isr_info, pst_event, &st_netbuf_header);

            OAL_MIPS_RX_STATISTIC(DMAC_PROFILING_FUNC_RX_DMAC_GET_DSCR_AND_RET_BACK);
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
            /* ��ƹ�Ҷ����е�ĳһ�������У�һ���¼������˶��ٸ�С���ʹ�� */
            dmac_rx_update_packets_process_info(pst_device, pst_rx_isr_info, &uc_isr_info_cnt[0]);
#endif

            /* �ͷű����������Ľڵ���Ϣ */
            hal_free_rx_isr_info(pst_device, pst_rx_isr_info);

            if (oal_netbuf_list_empty(&st_netbuf_header) && (OAL_SUCC != ul_rslt))
            {
                /* �ͷ������������ڵ���Ϣ */
                hal_free_rx_isr_list(pst_device, pst_rx_isr_list);
                OAM_STAT_VAP_INCR(pst_event_hdr->uc_vap_id, rx_ppdu_dropped, 1);
                DMAC_VAP_DFT_STATS_PKT_INCR(pst_dmac_vap->st_query_stats.ul_rx_ppdu_dropped, 1);

                return ul_rslt;
            }
        }
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        for (uc_queue_idx = HAL_RX_DSCR_NORMAL_PRI_QUEUE; uc_queue_idx < HAL_RX_DSCR_QUEUE_ID_BUTT; uc_queue_idx++)
        {
            /* ÿ�δ�rx�������ͷ���������Դ�󣬼�鲢����rx��������Դ��Ӳ������ */
            if (uc_isr_info_cnt[uc_queue_idx]!= 0)
            {
                hal_rx_add_dscr(pst_device, uc_queue_idx, 1);
            }
        }
#endif
        OAL_MIPS_RX_STATISTIC(DMAC_PROFILING_FUNC_RX_DMAC_INTR_LIST_OVER);
        OAM_PROFILING_RX_STATISTIC(OAM_PROFILING_FUNC_RX_DMAC_GET_CB_LIST);

    }
    else
    {
        hal_hw_rx_dscr_info_stru    st_rx_isr_info;
        st_rx_isr_info.uc_queue_id    = HAL_RX_DSCR_HIGH_PRI_QUEUE;
        st_rx_isr_info.uc_channel_num = pst_wlan_rx_event->uc_channel_num;
        st_rx_isr_info.us_dscr_num    = pst_wlan_rx_event->us_dscr_num;
        st_rx_isr_info.pul_base_dscr  = pst_wlan_rx_event->pul_base_dscr;

        /* ����Ҫ�������������������������ժȡ���������ҽ��������黹������������ */
        ul_rslt = dmac_rx_get_dscr_list(pst_device, &st_rx_isr_info, pst_event, &st_netbuf_header);
        if (oal_netbuf_list_empty(&st_netbuf_header) && (OAL_SUCC != ul_rslt))
        {
            OAM_ERROR_LOG1(pst_event_hdr->uc_vap_id, OAM_SF_RX, "{dmac_rx_process_data_event::dmac_rx_get_dscr_list failed[%d].}", ul_rslt);

            OAM_STAT_VAP_INCR(pst_event_hdr->uc_vap_id, rx_ppdu_dropped, 1);
            DMAC_VAP_DFT_STATS_PKT_INCR(pst_dmac_vap->st_query_stats.ul_rx_ppdu_dropped, 1);
            return ul_rslt;
        }
    }

#ifdef _PRE_WLAN_FEATURE_ALWAYS_TX
    if((HAL_ALWAYS_RX_DISABLE != pst_device->bit_al_rx_flag))
    {
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
        pst_netbuf = oal_netbuf_peek(&st_netbuf_header);
        if (OAL_PTR_NULL != pst_netbuf)
        {
            dmac_rx_free_netbuf_list(&st_netbuf_header, &pst_netbuf, pst_wlan_rx_event->us_dscr_num);
        }
#endif
     //   OAM_WARNING_LOG0(0, OAM_SF_RX, "{dmac_rx_process_data_event:: always rx data process already done in isr.}");
        return OAL_SUCC;
    }
#endif

    //OAL_MIPS_RX_STATISTIC(DMAC_PROFILING_FUNC_RX_DMAC_GET_CB_LIST);

    /* ���½��յ��������ĸ�������dmac_rx_get_dscr_list�����У��жԶ��жϵĴ��� */
    us_dscr_num = pst_wlan_rx_event->us_dscr_num;

    /* ѭ������ÿһ��MPDU */
    while (0 != us_dscr_num)
    {
        OAL_MIPS_RX_STATISTIC(DMAC_PROFILING_FUNC_RX_DMAC_HANDLE_PER_MPDU_START);
        OAM_PROFILING_RX_STATISTIC(OAM_PROFILING_FUNC_RX_DMAC_HANDLE_PER_MPDU_START);
        pst_curr_netbuf = oal_netbuf_peek(&st_netbuf_header);
        if (OAL_PTR_NULL == pst_curr_netbuf)
        {
            OAM_ERROR_LOG0(pst_event_hdr->uc_vap_id, OAM_SF_RX, "{dmac_rx_process_data_event::pst_curr_netbuf null.}");
            OAM_STAT_VAP_INCR(pst_event_hdr->uc_vap_id, rx_abnormal_cnt, 1);
            DMAC_VAP_DFT_STATS_PKT_INCR(pst_dmac_vap->st_query_stats.ul_rx_abnormal_dropped, 1);
            break;
        }

        OAL_MEM_NETBUF_TRACE(pst_curr_netbuf, OAL_TRUE);

        /* ��ȡÿһ��MPDU�Ŀ�����Ϣ */
        pst_cb_ctrl = (dmac_rx_ctl_stru*)oal_netbuf_cb(pst_curr_netbuf);

        pst_rx_info = &(pst_cb_ctrl->st_rx_info);

#ifdef _PRE_WLAN_MAC_BUGFIX_SW_CTRL_RSP
        dmac_rx_set_rsp_rate(pst_device->uc_mac_device_id, pst_cb_ctrl, pst_event_hdr->en_type);
#endif /* _PRE_WLAN_SW_CTRL_RSP */

        /* Ӳ���ϱ���NUM RX buffers for current MPDU����0���쳣���� */
        if (0 == pst_rx_info->bit_buff_nums)
        {
            /* �ͷź������е�netbuf */
            dmac_rx_free_netbuf_list(&st_netbuf_header, &pst_curr_netbuf, us_dscr_num);
            OAM_STAT_VAP_INCR(pst_event_hdr->uc_vap_id, rx_abnormal_cnt, 1);
            DMAC_VAP_DFT_STATS_PKT_INCR(pst_dmac_vap->st_query_stats.ul_rx_abnormal_dropped, 1);
            OAM_ERROR_LOG0(pst_event_hdr->uc_vap_id, OAM_SF_RX, "{dmac_rx_process_data_event:: 0 == pst_cb_ctrl->st_rx_info.bit_buff_nums");
            break;
        }

        /* Ӳ���ϱ���RX Frame Length���ȵ��쳣���� */
        if (OAL_FALSE == pst_rx_info->bit_amsdu_enable)
        {
        #ifdef _PRE_WLAN_HW_TEST
            /* ����������������Ϊ��� */
            if (HAL_ALWAYS_RX_RESERVED == pst_device->bit_al_rx_flag)
            {
                if (pst_rx_info->us_frame_len > HAL_AL_RX_FRAME_LEN)
                {
                    /* �ͷŵ�ǰMPDU���ڴ� */
                    us_dscr_num -= pst_cb_ctrl->st_rx_info.bit_buff_nums;
                    OAM_ERROR_LOG2(pst_event_hdr->uc_vap_id, OAM_SF_RX, "{dmac_rx_process_data_event::non_amsdu mpdu \
                                   too long! mpdu_len=[%d], mpdu_len_limit=[%d].}", pst_rx_info->us_frame_len,
                                   HAL_AL_RX_FRAME_LEN);

                    dmac_rx_free_netbuf_list(&st_netbuf_header, &pst_curr_netbuf, pst_rx_info->bit_buff_nums);
                    DMAC_VAP_DFT_STATS_PKT_INCR(pst_dmac_vap->st_query_stats.ul_rx_abnormal_dropped, 1);

                    continue;
                }
            }
            else
        #endif
            {
                if (pst_rx_info->us_frame_len > WLAN_MAX_NETBUF_SIZE)
                {
                    /* �ͷŵ�ǰMPDU���ڴ� */
                    us_dscr_num -= pst_rx_info->bit_buff_nums;
                    OAM_ERROR_LOG2(pst_event_hdr->uc_vap_id, OAM_SF_RX, "{dmac_rx_process_data_event::non_amsdu mpdu \
                                   too long! mpdu_len=[%d], mpdu_len_limit=[%d].}", pst_rx_info->us_frame_len,
                                   WLAN_MAX_NETBUF_SIZE);

                    dmac_rx_free_netbuf_list(&st_netbuf_header, &pst_curr_netbuf, pst_rx_info->bit_buff_nums);
                    DMAC_VAP_DFT_STATS_PKT_INCR(pst_dmac_vap->st_query_stats.ul_rx_abnormal_dropped, 1);
                    continue;
                }
            }
        }

    #ifdef _PRE_WLAN_HW_TEST
        if (HAL_ALWAYS_RX_DISABLE == pst_device->bit_al_rx_flag)
    #endif
        {
            /* FCS�����֡ Ӳ������Ľ��������������壬��һʱ��drop */
            if (HAL_RX_FCS_ERROR == pst_cb_ctrl->st_rx_status.bit_dscr_status)
            {
#ifdef _PRE_WLAN_11K_STAT
                dmac_stat_rx_tid_fcs_error(pst_device, pst_rx_info->bit_vap_id, pst_cb_ctrl);
#endif
                /* �ͷŵ�ǰMPDU���ڴ� */
                us_dscr_num -= pst_rx_info->bit_buff_nums;
            #ifdef _PRE_WLAN_FEATURE_PACKET_CAPTURE
                pst_dmac_device = dmac_res_get_mac_dev(pst_device->uc_mac_device_id);
                if (DMAC_PKT_CAP_NORMAL != pst_dmac_device->st_pkt_capture_stat.uc_capture_switch)
                {
                    en_pkt_cap_sw = OAL_TRUE;
                }
            #endif
                /* ץ���򿪵�������ϱ�FCS ERROR��������, �ر������ӡ */
                if (OAL_FALSE == en_pkt_cap_sw)
                {
                    OAM_ERROR_LOG0(pst_event_hdr->uc_vap_id, OAM_SF_RX, "{dmac_rx_process_data_event:: HAL_RX_FCS_ERROR == pst_cb_ctrl->st_rx_status.bit_dscr_status");
                }
                dmac_rx_free_netbuf_list(&st_netbuf_header, &pst_curr_netbuf, pst_rx_info->bit_buff_nums);
                continue;
            }
        }

        pst_frame_hdr = (mac_ieee80211_frame_stru *)mac_get_rx_cb_mac_hdr(pst_rx_info);
        uc_frame_type = mac_get_frame_type((oal_uint8 *)pst_frame_hdr);
        if((WLAN_FC0_TYPE_DATA != uc_frame_type) && (WLAN_FC0_TYPE_MGT != uc_frame_type) && (WLAN_FC0_TYPE_CTL != uc_frame_type))
        {
            OAM_WARNING_LOG1(pst_event_hdr->uc_vap_id, OAM_SF_RX, "{dmac_rx_process_data_event::uc_frame_type [%d].}", uc_frame_type);
            return OAL_FAIL;
        }

#ifdef _PRE_WLAN_DFT_DUMP_FRAME
        ul_rslt = dmac_rx_compare_frametype_and_rxq(uc_frame_type, en_dscr_queue_id, pst_cb_ctrl, pst_curr_netbuf);
#else
        ul_rslt = dmac_rx_compare_frametype_and_rxq(uc_frame_type, en_dscr_queue_id);
#endif
        if (OAL_SUCC != ul_rslt)
        {
            /* �ͷŵ�ǰMPDU���ڴ� */
            us_dscr_num -= pst_rx_info->bit_buff_nums;
            dmac_rx_free_netbuf_list(&st_netbuf_header, &pst_curr_netbuf, pst_rx_info->bit_buff_nums);
            OAM_WARNING_LOG1(pst_event_hdr->uc_vap_id, OAM_SF_RX, "{dmac_rx_process_data_event:: dmac_rx_compare_frametype_and_rxq fail[%u]", ul_rslt);
            DMAC_VAP_DFT_STATS_PKT_INCR(pst_dmac_vap->st_query_stats.ul_rx_abnormal_dropped, 1);
            continue;
        }
        OAL_MIPS_RX_STATISTIC(DMAC_PROFILING_FUNC_RX_DMAC_HANDLE_PER_MPDU_FILTER_FRAME_RXQ);
        OAM_PROFILING_RX_STATISTIC(OAM_PROFILING_FUNC_RX_DMAC_HANDLE_PER_MPDU_FILTER_FRAME_RXQ);

        /* �����յ���MPDU�쳣 */
        ul_rslt = dmac_rx_filter_mpdu(pst_cb_ctrl, us_dscr_num);
        if (OAL_SUCC != ul_rslt)
        {
            OAM_WARNING_LOG1(pst_event_hdr->uc_vap_id, OAM_SF_RX, "{dmac_rx_process_data_event::dmac_rx_filter_mpdu failed[%d].", ul_rslt);
            DMAC_VAP_DFT_STATS_PKT_INCR(pst_dmac_vap->st_query_stats.ul_rx_abnormal_dropped, us_dscr_num);

            /* �ͷź������е�netbuf */
            dmac_rx_free_netbuf_list(&st_netbuf_header, &pst_curr_netbuf, us_dscr_num);
            OAM_STAT_VAP_INCR(pst_event_hdr->uc_vap_id, rx_abnormal_cnt, 1);
            break;
        }
        OAL_MIPS_RX_STATISTIC(DMAC_PROFILING_FUNC_RX_DMAC_HANDLE_PER_MPDU_FILTER_CB_CHECK);
        OAM_PROFILING_RX_STATISTIC(OAM_PROFILING_FUNC_RX_DMAC_HANDLE_PER_MPDU_FILTER_CB_CHECK);

    #ifdef _PRE_WLAN_CHIP_TEST_ALG
        dmac_alg_rx_event_notify(uc_vap_id, pst_curr_netbuf, pst_cb_ctrl);
    #endif

        /* �˴���vap idΪhal���vap id */
        uc_vap_id = pst_rx_info->bit_vap_id;

#ifdef _PRE_WLAN_FEATURE_HILINK
        if ((WLAN_HAL_OTHER_BSS_UCAST_ID == uc_vap_id) || (WLAN_HAL_OHTER_BSS_ID == uc_vap_id))
        {
            dmac_fbt_scan_rx_process_frame(pst_device, pst_cb_ctrl);
        }
#endif

        
        ul_rslt = dmac_rx_get_vap(pst_device, uc_vap_id, en_dscr_queue_id, &pst_vap);
        if (OAL_SUCC != ul_rslt)
        {
            if ((OAL_FAIL == ul_rslt) && (pst_vap->en_vap_state == MAC_VAP_STATE_STA_WAIT_ASOC)
                && (mac_get_data_type(pst_curr_netbuf) == MAC_DATA_EAPOL))
            {
                OAM_WARNING_LOG0(pst_vap->uc_vap_id, OAM_SF_RX,
                    "{dmac_rx_process_data_event::report EAPOL to host when vap_state is WAIT_ASOC}");
            }
            else
            {
                //OAM_INFO_LOG1(pst_event_hdr->uc_vap_id, OAM_SF_RX, "{dmac_rx_process_data_event::dmac_rx_get_vap ul_rslt=%d.}", ul_rslt);

                /* �ͷŵ�ǰMPDUռ�õ�netbuf */
                us_dscr_num -= pst_rx_info->bit_buff_nums;
                dmac_rx_free_netbuf_list(&st_netbuf_header, &pst_curr_netbuf, pst_rx_info->bit_buff_nums);
                OAM_STAT_VAP_INCR(0, rx_abnormal_dropped, 1);
                DMAC_VAP_DFT_STATS_PKT_INCR(pst_dmac_vap->st_query_stats.ul_rx_abnormal_dropped, 1);
                continue;
            }
        }

        /* ��¼��cb�� */
        pst_rx_info->uc_mac_vap_id = pst_vap->uc_vap_id;

        if (en_is_first_dscr)
        {
            pst_part_vap = pst_vap;
            en_is_first_dscr = OAL_FALSE;
        }

        uc_frame_type    = mac_frame_get_type_value((oal_uint8 *)pst_frame_hdr);
        uc_frame_subtype = mac_frame_get_subtype_value((oal_uint8 *)pst_frame_hdr);

        if (WLAN_DATA_BASICTYPE == uc_frame_type && WLAN_NULL_FRAME != uc_frame_subtype)
        {
#if defined(_PRE_DEBUG_MODE) || (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
            /* ά�⣬���һ���ؼ�֡��ӡ. DBAC�����±���յ�VIP֡ */
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
            uc_data_type = mac_get_data_type_from_80211(pst_curr_netbuf, pst_rx_info->uc_mac_header_len);
            if(MAC_DATA_EAPOL >= uc_data_type) /* 51 arp̫֡����������ˢ����ӡ��wapi֡��ʱ������ */
            {
                OAM_WARNING_LOG4(pst_vap->uc_vap_id, OAM_SF_ANY, "{dmac_rx_process_data_event::rx datatype=%u, len=%u}[0:dhcp 1:eapol 2:arp_rsp 3:arp_req] from XX:XX:XX:XX:%x:%x",uc_data_type, pst_cb_ctrl->st_rx_info.us_frame_len, pst_frame_hdr->auc_address2[4], pst_frame_hdr->auc_address2[5]);
                if(MAC_DATA_EAPOL == uc_data_type)
                {
                    /*lint -e666*/
                    OAM_WARNING_LOG1(pst_vap->uc_vap_id, OAM_SF_CONN, "{dmac_rx_process_data_event:: EAPOL rx, key info is %x }", OAL_NET2HOST_SHORT(mac_rx_get_eapol_keyinfo_51(pst_curr_netbuf)));
                    /*lint +e666*/
                }
            }
#else
            uc_data_type = mac_get_data_type(pst_curr_netbuf);
            if((MAC_DATA_ARP_RSP >= uc_data_type) ||
            (OAL_TRUE == dmac_check_arp_req_owner(pst_curr_netbuf, pst_vap->uc_vap_id, uc_data_type)))  /* arp_req���Ϊת��֡������������־ˢ�� */
            {
                OAM_WARNING_LOG4(pst_vap->uc_vap_id, OAM_SF_ANY, "{dmac_rx_process_data_event::rx datatype=%u, len=%u}[0:dhcp 1:eapol 2:arp_rsp 3:arp_req] from XX:XX:XX:XX:%x:%x",uc_data_type, pst_cb_ctrl->st_rx_info.us_frame_len, pst_frame_hdr->auc_address2[4], pst_frame_hdr->auc_address2[5]);
                if((MAC_DATA_EAPOL == uc_data_type) && (OAL_EAPOL_TYPE_KEY == mac_get_eapol_type(pst_curr_netbuf)))
                {
                    /*lint -e666*/
                    OAM_WARNING_LOG1(pst_vap->uc_vap_id, OAM_SF_CONN, "{dmac_rx_process_data_event::rx eapol, info is %x }", OAL_NET2HOST_SHORT(mac_get_eapol_keyinfo(pst_curr_netbuf)));
                    /*lint +e666*/

                    dmac_rx_update_user_eapol_key_open(pst_vap, pst_curr_netbuf);
                }
#ifdef _PRE_WLAN_DOWNLOAD_PM
                pst_cb_ctrl->st_rx_info.bit_is_key_frame = 1;
#endif
            }
#endif


#endif
#ifdef _PRE_WLAN_FEATURE_DBAC
            pst_mac_device = mac_res_get_dev(pst_vap->uc_device_id);
            if( pst_mac_device != OAL_PTR_NULL)
            {
#if defined(_PRE_WLAN_FEATURE_DBAC) && defined(_PRE_PRODUCT_ID_HI110X_DEV)
                /* VIP֡�Ż���Ҫ�����STAģʽ��1151 DBAC����Ҫ */
                if (mac_is_dbac_running(pst_mac_device) &&
                    (pst_device->uc_current_chan_number == pst_vap->st_channel.uc_chan_number) &&
                    (MAC_DATA_DHCP == uc_data_type || MAC_DATA_EAPOL == uc_data_type))
                {
                    pst_mac_device->en_dbac_has_vip_frame = OAL_TRUE;
                }
#endif
            } // ( pst_mac_device != OAL_PTR_NULL)
#endif // _PRE_WLAN_FEATURE_DBAC
#ifdef _PRE_WLAN_FEATURE_BTCOEX
            dmac_btcoex_release_rx_prot(pst_vap, uc_data_type);
#endif
        }

        OAL_MIPS_RX_STATISTIC(DMAC_PROFILING_FUNC_RX_DMAC_HANDLE_PER_MPDU_GET_VAP_ID);
        OAM_PROFILING_RX_STATISTIC(OAM_PROFILING_FUNC_RX_DMAC_HANDLE_PER_MPDU_GET_VAP_ID);

#ifdef _PRE_WLAN_DFT_DUMP_FRAME
        /* �������Beacon֡�е�Beacon Interval��Ϊ0ʱ��һ��ERROR�� ���ⵥ�رպ��ɾ����ά�� */
#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
        {
            oal_uint8   uc_sub_type;
            oal_uint8  *puc_frame_body;
            oal_uint16 *pus_bi;

            uc_sub_type = mac_get_frame_type_and_subtype((oal_uint8 *)mac_get_rx_cb_mac_hdr(pst_rx_info));
            puc_frame_body = oal_netbuf_payload(pst_curr_netbuf);
            pus_bi = (oal_uint16 *)(puc_frame_body + 8); /* ƫ��8�ֽ�tsf����beacon interval */

            if (((WLAN_FC0_SUBTYPE_BEACON|WLAN_FC0_TYPE_MGT) == uc_sub_type || (WLAN_FC0_SUBTYPE_PROBE_RSP|WLAN_FC0_TYPE_MGT) == uc_sub_type)
                 && (0 == *pus_bi))
            {
                OAM_WARNING_LOG1(0, OAM_SF_ANY, "dmac_rx_process_data_event::rx beacon/probe rsp Beacon Interval:%d", *pus_bi);
            }
        }
#else /* APģʽ�´˴����յ��ܶ಻�����Լ��Ĺ㲥֡��02����֡ά�ⲻ���ڴ˴� */
        /* �ϱ�֡��֡��Ӧ��CB�ֶε�SDT */
        if ((FRW_EVENT_TYPE_WLAN_DRX != pst_event_hdr->en_type)||
            (OAL_SWITCH_ON == oam_report_data_get_global_switch(OAM_OTA_FRAME_DIRECTION_TYPE_RX)))
        {
            if (pst_cb_ctrl->st_rx_status.bit_dscr_status == HAL_RX_SUCCESS)
            {
                mac_rx_report_80211_frame((oal_uint8 *)pst_vap, (oal_uint8 *)(pst_rx_info), pst_curr_netbuf, OAM_OTA_TYPE_RX_DMAC_CB);
            }
        }
#endif /* #if defined(_PRE_PRODUCT_ID_HI110X_DEV) */
#endif /* #ifdef _PRE_WLAN_DFT_DUMP_FRAME */

        en_frame_ctrl = dmac_rx_process_frame(pst_vap, pst_cb_ctrl, pst_curr_netbuf, &st_netbuf_header);
        if (DMAC_RX_FRAME_CTRL_DROP == en_frame_ctrl)
        {
            /* �ͷŵ�ǰMPDU���ڴ� */
            us_dscr_num -= pst_rx_info->bit_buff_nums;
            dmac_rx_free_netbuf_list(&st_netbuf_header, &pst_curr_netbuf, pst_rx_info->bit_buff_nums);
            DMAC_VAP_STATS_PKT_INCR(pst_dmac_vap->st_query_stats.ul_rx_dropped_misc, 1);
            continue;
        }
        OAL_MIPS_RX_STATISTIC(DMAC_PROFILING_FUNC_RX_DMAC_HANDLE_PER_MPDU_FILTER_OVER);
        OAM_PROFILING_RX_STATISTIC(OAM_PROFILING_FUNC_RX_DMAC_HANDLE_PER_MPDU_FILTER_OVER);

#ifdef _PRE_WLAN_PERFORM_STAT
        /* ���½�������ͳ���� */
        dmac_stat_rx_thrpt(pst_event_hdr, pst_vap, pst_cb_ctrl);
#endif

#ifdef _PRE_WLAN_11K_STAT
        dmac_user_stat_rx_info(pst_event_hdr, pst_vap, pst_cb_ctrl, 0);
#endif

        dmac_rx_data_dscr_bw_process(pst_event_hdr, pst_vap, pst_cb_ctrl);

        if (WLAN_DATA_BASICTYPE == uc_frame_type)
        {
#ifdef _PRE_WLAN_FEATURE_BTCOEX
            /* ps֡����ȥ�󣬻��յ��Զ˵���������������Ҫ�����ٷ�һ֡���Ż���ʱ����Ҫ */
            //dmac_btcoex_rx_data_bt_acl_check(pst_vap);
            dmac_btcoex_rx_rate_process_check(pst_vap, uc_frame_subtype, uc_data_type, pst_cb_ctrl->st_rx_status.bit_AMPDU);
#endif

#ifdef _PRE_WLAN_FEATURE_M2S
            dmac_m2s_rx_rate_nss_process(pst_vap, pst_cb_ctrl, pst_frame_hdr);
#endif
        }

#ifdef _PRE_WLAN_CHIP_TEST
        DMAC_CHIP_TEST_CALL(dmac_test_sch_stat_rx_mpdu_num(pst_event_hdr, pst_vap, pst_cb_ctrl));
        DMAC_CHIP_TEST_CALL(dmac_test_sch_stat_rx_sta_num(pst_event_hdr, pst_vap, pst_cb_ctrl));
#endif
#ifdef _PRE_DEBUG_MODE
        //���ӹؼ�֡��ӡ�������ж��Ƿ�����Ҫ��ӡ��֡���ͣ�Ȼ���ӡ��֡����
        en_trace_pkt_type = wifi_pkt_should_trace( pst_curr_netbuf, MAC_GET_RX_CB_MAC_HEADER_LEN(&(pst_cb_ctrl->st_rx_info)));
        if( PKT_TRACE_BUTT != en_trace_pkt_type)
        {
            OAM_WARNING_LOG1(pst_vap->uc_vap_id, OAM_SF_RX, "{dmac_rx_process_data_event::type%d from wifi[0:dhcp 1:arp_req 2:arp_rsp 3:eapol 4:icmp 5:assoc_req 6:assoc_rsp 9:dis_assoc 10:auth 11:deauth]}\r\n", en_trace_pkt_type);
            if( PKT_TRACE_DATA_ICMP < en_trace_pkt_type)
            {
                puc_payload = MAC_GET_RX_PAYLOAD_ADDR(&(pst_cb_ctrl->st_rx_info), pst_curr_netbuf);
                oam_report_80211_frame(BROADCAST_MACADDR,
                                       (oal_uint8 *)(pst_frame_hdr),
                                       MAC_GET_RX_CB_MAC_HEADER_LEN(&(pst_cb_ctrl->st_rx_info)),
                                       puc_payload,
                                       pst_cb_ctrl->st_rx_info.us_frame_len,
                                       OAM_OTA_FRAME_DIRECTION_TYPE_RX);
            }
        }
#endif

        /* �����ǰ�����vap��֮ǰnetbuf��vap��һ������ǰ��İ���ɵ�netbuf�����ϱ�,ͬʱ��pst_part_vap����Ϊ��ǰvap */
        if (pst_part_vap != pst_vap)
        {
            if (OAL_FALSE == oal_netbuf_list_empty(&st_part_netbuf[uc_irq_index]))
            {
                dmac_rx_process_data_mgmt_event(pst_event_mem, &st_part_netbuf[uc_irq_index], pst_part_vap, pst_netbuf);
            }

            OAM_PROFILING_RX_STATISTIC(OAM_PROFILING_FUNC_RX_DMAC_END);
            pst_part_vap = pst_vap;
            uc_irq_index++;

            /* rx fifo���һ�δ����жϸ���8 */
            /* ������жϸ�����������������޷����ϱ� */
            /* �ͷź������е�netbuf */
            if (uc_irq_index >= WLAN_RX_INTERRUPT_MAX_NUM_PER_DEVICE)
            {
                dmac_rx_free_netbuf_list(&st_netbuf_header, &pst_curr_netbuf, us_dscr_num);
                OAM_WARNING_LOG1(pst_event_hdr->uc_vap_id, OAM_SF_RX, "{dmac_rx_process_data_event::rx interruptions[%d] exceed.}", uc_irq_index);
                OAM_STAT_VAP_INCR(pst_event_hdr->uc_vap_id, rx_abnormal_cnt, 1);
                DMAC_VAP_DFT_STATS_PKT_INCR(pst_dmac_vap->st_query_stats.ul_rx_abnormal_dropped, 1);
                break;
            }

            /* ��ʼ����ǰ����hmac netbuf�����ͷ */
            oal_netbuf_list_head_init(&(st_part_netbuf[uc_irq_index]));
        }

        /* ����֡��st_netbuf_header����ͷժ�������뵱ǰvap id��netbuf���� */
        for (ul_netbuf_index = pst_rx_info->bit_buff_nums; ul_netbuf_index > 0; ul_netbuf_index--)
        {
            pst_netbuf = oal_netbuf_delist(&st_netbuf_header);
            OAL_MEM_NETBUF_TRACE(pst_netbuf, OAL_FALSE);
            oal_netbuf_add_to_list_tail(pst_netbuf, &st_part_netbuf[uc_irq_index]);
        }

        /* ����δ������������ĸ��� */
        us_dscr_num     -= pst_rx_info->bit_buff_nums;

#ifdef _PRE_WLAN_11K_STAT
        if(0 == us_dscr_num)
        {
            dmac_user_stat_rx_info(pst_event_hdr, pst_vap, pst_cb_ctrl, 1);
        }
#endif

        /* ͳ�ƽ��յ�MPDU���� */
        //dmac_rx_update_aggr_mib(pst_vap, 1);

        OAL_MIPS_RX_STATISTIC(DMAC_PROFILING_FUNC_RX_DMAC_HANDLE_PER_MPDU_MAKE_NETBUF_LIST);
        OAM_PROFILING_RX_STATISTIC(OAM_PROFILING_FUNC_RX_DMAC_HANDLE_PER_MPDU_MAKE_NETBUF_LIST);
    }


    //OAM_PROFILING_RX_STATISTIC(OAM_PROFILING_FUNC_RX_DMAC_HANDLE_MPDU);
    //OAL_MIPS_RX_STATISTIC(DMAC_PROFILING_FUNC_RX_DMAC_HANDLE_MPDU);

    /* �����һ��vap��netbuf�����ϱ� */
    if (uc_irq_index < WLAN_RX_INTERRUPT_MAX_NUM_PER_DEVICE)
    {
        if (OAL_FALSE == oal_netbuf_list_empty(&st_part_netbuf[uc_irq_index]))
        {
            dmac_rx_process_data_mgmt_event(pst_event_mem, &st_part_netbuf[uc_irq_index], pst_vap, pst_netbuf);
        }

        OAM_PROFILING_RX_STATISTIC(OAM_PROFILING_FUNC_RX_DMAC_END);
    }

    /* 51 ��ʱȡ���ù켣׷��DEBUG */
#if (_PRE_PRODUCT_ID_HI1151 != _PRE_PRODUCT_ID)
#ifdef _PRE_DEBUG_MODE
    ul_profingling_time2 = oal_5115timer_get_10ns();

    pst_device->ast_dpart_track[pst_device->ul_dpart_save_idx].ul_arrive_time = ul_profingling_time1;
    pst_device->ast_dpart_track[pst_device->ul_dpart_save_idx].ul_handle_time = ul_profingling_time1 - ul_profingling_time2;
#endif
#endif
    return OAL_SUCC;
}

#ifndef _PRE_WLAN_PROFLING_MIPS
#if (_PRE_OS_VERSION_RAW == _PRE_OS_VERSION)  && defined (__CC_ARM)
#pragma arm section rodata, code, rwdata, zidata  // return to default placement
#endif
#endif

#if 0

OAL_STATIC oal_void dmac_error_irq_resume_dscr_queue(
                hal_error_irq_event_stru           *pst_error_irq_event,
                hal_rx_dscr_queue_id_enum_uint8     en_queue_num)
{
    hal_to_dmac_device_stru            *pst_hal_device;
    oal_uint16                          us_netbuf_free_cnt;     /* netbuf�ڴ�صĿ���netbuf����Ŀ */
    oal_uint16                          us_dscr_free_cnt;       /* �����������ڴ���п��е��������ĸ��� */
    oal_uint16                          us_actual_free_cnt;     /* ����netbuf��dscr�Ľ�С��ֵ */
    oal_uint16                          us_curr_dscr_num;       /* ��ǰ���������ĸ��� */
    oal_uint16                          us_target_num;          /* ʵ�ʿ��Բ�����������ĸ��� */

    pst_hal_device = pst_error_irq_event->pst_hal_device;

    /* ��ͣӲ�� */
    hal_set_machw_tx_suspend(pst_hal_device);

    /* disable PHY and PA */
    hal_disable_machw_phy_and_pa(pst_hal_device);

    oal_mem_get_free_count(OAL_MEM_POOL_ID_NETBUF, 1, &us_netbuf_free_cnt);     /* 1����ڶ����ӳ� */
#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
    oal_mem_get_free_count(OAL_MEM_POOL_ID_TX_DSCR_1, 0, &us_dscr_free_cnt);  /* 0�����һ���ӳ� */
#else
    oal_mem_get_free_count(OAL_MEM_POOL_ID_SHARED_DSCR, 0, &us_dscr_free_cnt);  /* 0�����һ���ӳ� */
#endif
    /* ��ȡ�ڴ����Ŀ��и��� */
    us_actual_free_cnt = (us_netbuf_free_cnt > us_dscr_free_cnt) ? us_dscr_free_cnt : us_netbuf_free_cnt;
    if (0 == us_actual_free_cnt)    /* �޿�ʹ�ÿռ� */
    {
        return;
    }

    /* ��ȡ�ܲ�����������ĸ��� */
    us_curr_dscr_num = pst_hal_device->ast_rx_dscr_queue[en_queue_num].us_element_cnt;

    if ((us_actual_free_cnt + us_curr_dscr_num) < HAL_NORMAL_RX_MAX_BUFFS)
    {
        us_target_num = us_actual_free_cnt;
    }
    else
    {
        us_target_num = HAL_NORMAL_RX_MAX_BUFFS - us_curr_dscr_num;
    }

    /* ��������������������� */
    while (0 != us_target_num)
    {
        hal_rx_add_dscr(pst_hal_device, en_queue_num);
        us_target_num--;
    }

    /* enable PHY and PA */
    hal_enable_machw_phy_and_pa(pst_hal_device);

    /* �ָ�Ӳ�� */
    hal_set_machw_tx_resume(pst_hal_device);
}
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

OAL_STATIC oal_void  dmac_rx_resume_dscr_queue(mac_device_stru *pst_mac_device, hal_to_dmac_device_stru  *pst_hal_device,
                                                           hal_rx_dscr_queue_id_enum_uint8 en_queue_num)
{
    frw_event_type_enum_uint8       en_event_type;
    oal_uint32                      ul_event_succ;
    oal_dlist_head_stru            *pst_dscr_entry;
    hal_rx_dscr_stru               *pst_firt_dscr;
    oal_uint                        ul_irq_flag = 0;

    if (OAL_PTR_NULL ==  pst_hal_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RX, "{dmac_rx_resume_dscr_queue::pst_hal_device null.}");
        return;
    }

    /* 1.1 ��ȡ��Ҫ�ָ��Ķ��к� */
    if (en_queue_num >= HAL_RX_QUEUE_NUM)
    {
        OAM_ERROR_LOG1(0, OAM_SF_RX, "{dmac_rx_resume_dscr_queue::en_queue_num[%d] error.}", en_queue_num);
        return;
    }

    /* 2.1 diable MAC/PHY */
    /*hal_disable_machw_phy_and_pa(pst_hal_device);*/

    /* 3.1 ��ս��ղ����¼� */
    en_event_type = (HAL_RX_DSCR_HIGH_PRI_QUEUE == en_queue_num) ? FRW_EVENT_TYPE_WLAN_CRX : FRW_EVENT_TYPE_WLAN_DRX;
    /* TBD Ŀǰ1102�Ĺ���֡�п��ܱ�mac������ͨ���ȼ��������ϱ������������Ҫ�����������¼�����֮ǰ������Ĺ��˲�����
           �ⲿ���߼���ʱû��ʵ�֣�Ϊ�˷�ֹ4.1����ʱ��headָ�뻹���¼������У����������Ҫ����ͨ���ȼ��¼����м�
           �����ȼ��¼����ж�flush�� */
    ul_event_succ = frw_event_flush_event_queue(en_event_type);

#ifdef _PRE_WLAN_FEATURE_STA_PM
    /* 110x����Ѿ�����idle״̬����Ҫ��ȥ����dscr */
    if ((HAL_DEVICE_IDLE_STATE == GET_HAL_DEVICE_STATE(pst_hal_device)) && (OAL_FALSE == pst_hal_device->en_is_mac_pa_enabled))
    {
        OAM_WARNING_LOG2(0, OAM_SF_RX, "{dmac_rx_resume_dscr_queue::device[%d] in idle state not resume dscr queue[%d]}",
                        pst_hal_device->uc_device_id, en_queue_num);
        return;
    }
#endif

    oal_irq_save(&ul_irq_flag, OAL_5115IRQ_DMSC);

    /* 4.1 ���������������¹Ҹ�mac���������������Ϊ�գ���Ҫ�������� */
    hal_rx_add_dscr(pst_hal_device, en_queue_num, 1);

    /*�ϰ벿���������������üĴ�����ͳһ��Q-EMPTYʱ�������ã���֤Ψһ,���ù����п��Բ���PA*/
    if(0 != pst_hal_device->ast_rx_dscr_queue[en_queue_num].us_element_cnt)
    {
        pst_dscr_entry = pst_hal_device->ast_rx_dscr_queue[en_queue_num].st_header.pst_next;
        pst_firt_dscr = OAL_DLIST_GET_ENTRY(pst_dscr_entry, hal_rx_dscr_stru, st_entry);

        if(OAL_TRUE != hal_set_machw_rx_buff_addr_sync(pst_hal_device, (oal_uint32*)pst_firt_dscr, en_queue_num))
        {
            OAM_WARNING_LOG1(0, OAM_SF_RX,"[ERROR]Q-EMPTY[%d] but machw reg still not zero!\r\n",en_queue_num);
        }
    }
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_RX, "{Q-EMPTY[%d] and did not catch any buffs!}",en_queue_num);
    }

    oal_irq_restore(&ul_irq_flag, OAL_5115IRQ_DMSC);

    OAM_INFO_LOG3(0, OAM_SF_RX, "{dmac_rx_resume_dscr_queue[%d]::OK [%u] flushed & [%u] left.}",
                   en_queue_num, ul_event_succ, pst_hal_device->ast_rx_dscr_queue[en_queue_num].us_element_cnt);

    /* 5.1 enable MAC/PHY */
    /*hal_enable_machw_phy_and_pa(pst_hal_device);*/

}
#else

OAL_STATIC oal_void  dmac_rx_resume_dscr_queue(mac_device_stru *pst_mac_device, hal_to_dmac_device_stru  *pst_hal_device,
                                                           hal_rx_dscr_queue_id_enum_uint8 en_queue_num)
{
    hal_rx_dscr_queue_header_stru  *pst_rx_dscr_queue;
    dmac_rx_ctl_stru               *pst_cb_ctrl;                    /* ��дnetbuf��cb�ֶ�ʹ�� */
    oal_netbuf_stru                *pst_netbuf = OAL_PTR_NULL;      /* ָ��ÿһ����������Ӧ��netbuf */
    oal_uint32                     *pul_curr_dscr;                  /* ���ڴ���������� */
    oal_uint32                     *pul_next_dscr;
    hal_rx_dscr_stru               *pst_hal_to_dmac_dscr;
    oal_uint32                      ul_dscr_idx;
    frw_event_type_enum_uint8       en_event_type;
#if (defined(_PRE_PRODUCT_ID_HI110X_DEV) || defined(_PRE_PRODUCT_ID_HI110X_HOST))
    oal_uint16                      aus_dscr_num[HAL_RX_DSCR_QUEUE_ID_BUTT] = {HAL_NORMAL_RX_MAX_BUFFS, HAL_HIGH_RX_MAX_BUFFS, HAL_SMALL_RX_MAX_BUFFS};
#else
    oal_uint16                      aus_dscr_num[HAL_RX_DSCR_QUEUE_ID_BUTT] = {HAL_NORMAL_RX_MAX_BUFFS, HAL_HIGH_RX_MAX_BUFFS};
#endif
    oal_uint                        ul_irq_flag;
#if 0
    if (0 == pst_mac_dev->uc_resume_qempty_flag)
    {
        /* qempty flag����Ϊ0ʱ����ʹ��qempty�ָ� */
        return;
    }
#endif

    /* ��ȡ��Ҫ�ָ��Ķ��к� */
    if (en_queue_num >= HAL_RX_QUEUE_NUM)
    {
        OAM_ERROR_LOG1(0, OAM_SF_RX, "{dmac_rx_resume_dscr_queue::en_queue_num[%d] error.}", en_queue_num);
        return;
    }

    /* ����Ӳ������ */
    hal_set_machw_tx_suspend(pst_hal_device);

    /* diable MAC/PHY */
    hal_disable_machw_phy_and_pa(pst_hal_device);

#ifdef _PRE_WLAN_MAC_BUGFIX_RESET
    /* ��λǰ�ӳ�50us */
    oal_udelay(50);
#endif

    OAM_WARNING_LOG2(0, OAM_SF_RX, "{dmac_rx_resume_dscr_queue:: dscr_q=%d, element=%d}", en_queue_num, pst_hal_device->ast_rx_dscr_queue[en_queue_num].us_element_cnt);

    /* ��������¼� */
    en_event_type = (HAL_RX_DSCR_NORMAL_PRI_QUEUE == en_queue_num) ? FRW_EVENT_TYPE_WLAN_DRX : FRW_EVENT_TYPE_WLAN_CRX;
    frw_event_flush_event_queue(en_event_type);

    OAM_WARNING_LOG2(0, OAM_SF_RX, "{dmac_rx_resume_dscr_queue:: dscr_q=%d, element=%d}", en_queue_num, pst_hal_device->ast_rx_dscr_queue[en_queue_num].us_element_cnt);

    /* ���ն��лָ� */
    pst_rx_dscr_queue = &(pst_hal_device->ast_rx_dscr_queue[en_queue_num]);
    if (OAL_PTR_NULL ==  pst_rx_dscr_queue)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RX, "{dmac_rx_resume_dscr_queue::pst_rx_dscr_queue null.}");
        return;
    }
    pul_curr_dscr = pst_rx_dscr_queue->pul_element_head;

    oal_irq_save(&ul_irq_flag, OAL_5115IRQ_DMSC);
    for (ul_dscr_idx = 0; ul_dscr_idx < aus_dscr_num[en_queue_num]; ul_dscr_idx++)
    {
        if (OAL_PTR_NULL == pul_curr_dscr)
        {
            OAM_ERROR_LOG3(0, OAM_SF_RX, "{dmac_rx_resume_dscr_queue::pul_curr_dscr null, ul_dscr_idx=%d en_queue_num=%d, element=%d}",
                           ul_dscr_idx, en_queue_num, pst_hal_device->ast_rx_dscr_queue[en_queue_num].us_element_cnt);

			/* speת�����ܵͣ�����rx dscr supplementʱ�����Գ���alloc skbʧ�ܣ���������̺�ֱ��return��û��enable phy_and_pa,�����쳣 */
            //oal_irq_enable();
            //return;
            break;
        }

        /* �����һ����Ҫ����������� */
        pst_hal_to_dmac_dscr = (hal_rx_dscr_stru *)pul_curr_dscr;
        if (NULL != pst_hal_to_dmac_dscr->pul_next_rx_dscr)
        {
            pul_next_dscr = HAL_RX_DSCR_GET_SW_ADDR((oal_uint32 *)OAL_DSCR_PHY_TO_VIRT((oal_uint)(pst_hal_to_dmac_dscr->pul_next_rx_dscr)));
        }
        else
        {
            pul_next_dscr = HAL_RX_DSCR_GET_SW_ADDR(pst_hal_to_dmac_dscr->pul_next_rx_dscr);
        }

        /* ��ȡ��������Ӧ��netbuf */
        hal_rx_get_netbuffer_addr_dscr(pst_hal_device, pul_curr_dscr, &pst_netbuf);

        pst_cb_ctrl = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);

        /* ��ȡ���������ֶ���Ϣ��������ӵ�netbuf��cb�ֶ��� */
        dmac_rx_get_dscr_info(pst_hal_device, pul_curr_dscr, pst_cb_ctrl);

        if (HAL_RX_NEW == pst_cb_ctrl->st_rx_status.bit_dscr_status)
        {
            pul_curr_dscr = pul_next_dscr;

            continue;
        }

        hal_rx_sync_invalid_dscr(pst_hal_device, pul_curr_dscr, en_queue_num);

        pul_curr_dscr = pul_next_dscr;
    }
    /* �������������¹Ҹ�MAC */
    hal_set_machw_rx_buff_addr(pst_hal_device, (oal_uint32)(pst_rx_dscr_queue->pul_element_head), en_queue_num);
    hal_reset_phy_machw(pst_hal_device, HAL_RESET_HW_TYPE_MAC, 0, 0, 0);

    oal_irq_restore(&ul_irq_flag, OAL_5115IRQ_DMSC);

    /* enable MAC/PHY */
    hal_recover_machw_phy_and_pa(pst_hal_device);
    /* �ָ�Ӳ������ */
    hal_set_machw_tx_resume(pst_hal_device);

    pst_hal_device->ul_track_stop_flag = OAL_FALSE;

    OAM_WARNING_LOG2(0, OAM_SF_RX, "{dmac_rx_resume_dscr_queue:: dscr_q=%d, element=%d}", en_queue_num, pst_hal_device->ast_rx_dscr_queue[en_queue_num].us_element_cnt);

    return;
}
#endif

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
OAL_STATIC oal_void dmac_tx_dataflow_break_error(mac_device_stru *pst_mac_device, hal_to_dmac_device_stru *pst_hal_device)
{
    mac_tx_dataflow_brk_bypass_struc *pst_brk_bypass = &(pst_mac_device->st_dataflow_brk_bypass);
    oal_uint32 ul_tx_comp_cnt = 0;

    /* ÿ��128��DataFlow Break�Ĵ����жϣ��ȶԵ�ǰ�ķ�������жϸ�����֮ǰ��¼�ķ�������жϸ��� */
    if(0 != ((++pst_brk_bypass->ul_tx_dataflow_brk_cnt) & ((1 << 7) - 1)))
    {
        return;
    }

    hal_get_irq_stat(pst_hal_device, (oal_uint8 *)&ul_tx_comp_cnt, sizeof(ul_tx_comp_cnt), HAL_IRQ_TX_COMP_CNT);
    if(pst_brk_bypass->ul_last_tx_complete_isr_cnt == ul_tx_comp_cnt)
    {
        dmac_reset_para_stru            st_reset_param;

        OAM_INFO_LOG1(0, OAM_SF_TX, "{dmac_tx_dataflow_break_error::No TX Complete irq between 128 dataflow break, CurTxCompCnt: %d", ul_tx_comp_cnt);

        /* �������ۺϸ��� */
        if(OAL_TRUE == pst_brk_bypass->en_brk_limit_aggr_enable)
        {
            OAM_WARNING_LOG1(0, OAM_SF_TX, "{dmac_tx_dataflow_break_error::Bypass Fail, CurTxCompCnt: %d", ul_tx_comp_cnt);
        }
        else
        {
            pst_brk_bypass->en_brk_limit_aggr_enable = OAL_TRUE;
            OAM_WARNING_LOG1(0, OAM_SF_TX, "{dmac_tx_dataflow_break_error::Enable DataFlowBreak limit, CurTxCompCnt: %d", ul_tx_comp_cnt);
        }

        /* ��ʼ��Mac�߼� */
        OAL_MEMZERO(&st_reset_param, sizeof(st_reset_param));
        st_reset_param.uc_reset_type = HAL_RESET_HW_TYPE_MAC;
        dmac_reset_hw(pst_mac_device, pst_hal_device, (oal_uint8 *)&st_reset_param);
    }

    /* ������ؼ����� */
    pst_brk_bypass->ul_last_tx_complete_isr_cnt = ul_tx_comp_cnt;
}


OAL_STATIC oal_void dmac_intr_fifo_overrun_error(mac_device_stru *pst_mac_device, hal_to_dmac_device_stru *pst_hal_dev)
{
    dmac_tx_reinit_tx_queue(pst_mac_device, pst_hal_dev);

    pst_hal_dev->ul_track_stop_flag = OAL_FALSE;
}

#endif

#if 0

oal_void dmac_check_txq(mac_device_stru *pst_mac_dev, oal_uint32 ul_param)
{
    oal_uint8                uc_q_idx           = 0;
    hal_tx_dscr_stru        *pst_tx_dscr_first  = OAL_PTR_NULL;
    hal_to_dmac_device_stru *pst_hal_device     = OAL_PTR_NULL;
    oal_uint32               ul_txq_ptr_status  = 0;
    oal_uint32               ul_txq_reg_dscr    = 0;
    oal_uint32               ul_reg_addr        = 0;
    oal_uint32               ul_ret             = OAL_SUCC;
    oal_uint32               ul_time_start;
    oal_uint32               ul_time_end;
    oal_uint32               ul_time_delay;

    pst_hal_device = pst_mac_dev->pst_device_stru;

    for (uc_q_idx = 0; uc_q_idx <= HAL_TX_QUEUE_VI; uc_q_idx++)
    {
        /* ����÷��Ͷ���û������,�����һ������ */
        if (pst_hal_device->ast_tx_dscr_queue[uc_q_idx].uc_ppdu_cnt == 0)
        {
            continue;
        }

        /* ��ʱ2ms,�ȴ�MACˢ�¼Ĵ���֮���ٶ�ȡ */
        ul_time_start = oal_5115timer_get_10ns();
        do
        {
            ul_time_end = oal_5115timer_get_10ns();
            if (ul_time_end < ul_time_start)
            {
                ul_time_delay = ul_time_start - ul_time_end;
            }
            else
            {
                ul_time_delay = ul_time_start + (0xFFFFFFFF - ul_time_end);
            }
        } while (ul_time_delay < MAX_DELAY_TIME);

        ul_reg_addr = (oal_uint32)(WITP_PA_AC_BK_FIRST_FRM_PTR_STATUS_REG + (uc_q_idx * 8));
        hal_reg_info(pst_hal_device, ul_reg_addr, &ul_txq_ptr_status);
        ul_txq_reg_dscr = (oal_uint32)OAL_PHY_TO_VIRT_ADDR(ul_txq_ptr_status);

        /* ȡ���ö����еĵ�һ�������� */
        pst_tx_dscr_first  = (hal_tx_dscr_stru *)OAL_DLIST_GET_ENTRY(pst_hal_device->ast_tx_dscr_queue[uc_q_idx].st_header.pst_next, hal_tx_dscr_stru, st_entry);
        /* ������ж��ˣ������ppdu */
        if (OAL_UNLIKELY((oal_uint32)pst_tx_dscr_first != ul_txq_reg_dscr))
        {
            OAM_WARNING_LOG2(0, OAM_SF_TX, "{dmac_check_txq::MAC loss tx complete irq, q_num[%d], dscr_addr[0x%x]}", pst_tx_dscr_first->uc_q_num, pst_tx_dscr_first);
            ul_ret = dmac_tx_complete_buff(pst_hal_device, pst_tx_dscr_first);
            if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
            {
                OAM_WARNING_LOG1(0, OAM_SF_TX, "{dmac_check_txq::dmac_tx_complete failed[%d].", ul_ret);
            }
        }
    }
}


OAL_STATIC oal_uint32  dmac_handle_reset_mac_error_immediately(
                mac_device_stru                 *pst_mac_device,
                hal_mac_error_type_enum_uint8    en_error_id)
{
    hal_to_dmac_device_stru   *pst_hal_device;
    dmac_reset_para_stru       st_reset_param;

    if (OAL_UNLIKELY(en_error_id >= HAL_MAC_ERROR_TYPE_BUTT))
    {
        return OAL_ERR_CODE_ARRAY_OVERFLOW;
    }

    pst_hal_device = pst_mac_device->pst_device_stru;

    /* ��λ: ��������쳣ͳ�ƣ����¿�ʼ���� */
    OAL_MEMZERO(pst_hal_device->st_dfr_err_opern, OAL_SIZEOF(pst_hal_device->st_dfr_err_opern));

    /*��λMAC + PHY���߼�*/
    st_reset_param.uc_reset_type    = HAL_RESET_HW_TYPE_ALL;
    st_reset_param.uc_reset_mac_mod = HAL_RESET_MAC_ALL;
    st_reset_param.uc_reset_mac_reg = OAL_FALSE;
    st_reset_param.uc_reset_phy_reg = OAL_FALSE;

    dmac_reset_hw(pst_mac_device, (oal_uint8 *)&st_reset_param);

    return OAL_SUCC;
}
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
#ifndef _PRE_WLAN_PRODUCT_1151V200

OAL_STATIC OAL_INLINE oal_void dmac_set_agc_lock_channel(
            mac_vap_stru        *pst_vap,
            dmac_rx_ctl_stru    *pst_cb_ctrl)
{
    mac_device_stru           *pst_mac_device = OAL_PTR_NULL;
    dmac_vap_stru             *pst_dmac_vap = OAL_PTR_NULL;
    hal_to_dmac_device_stru   *pst_hal_device = OAL_PTR_NULL;

    pst_mac_device = mac_res_get_dev(pst_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        return;
    }

    if (pst_mac_device->en_is_wavetest)
    {
        /* Wavetest���ܲ��Գ�����,�趨AGC�̶���ͨ��0  */
        if (pst_mac_device->uc_lock_channel == 0x00)
        {
            return;
        }
        else
        {
            pst_dmac_vap = mac_res_get_dmac_vap(pst_vap->uc_vap_id);
            if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dmac_vap))
            {
                return;
            }

            pst_hal_device = pst_dmac_vap->pst_hal_device;
            hal_set_agc_track_ant_sel(pst_hal_device, (oal_uint32)0x00);

            pst_mac_device->uc_lock_channel = 0x00;
        }
    }
    else
    {
        /* ��Wavetest���ܲ��Գ�����,�趨AGC��ͨ��Ϊ����Ӧ */
        if (pst_mac_device->uc_lock_channel == 0x02)
        {
            return;
        }
        else
        {
            pst_dmac_vap = mac_res_get_dmac_vap(pst_vap->uc_vap_id);
            if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dmac_vap))
            {
                return;
            }

            pst_hal_device = pst_dmac_vap->pst_hal_device;
            hal_set_agc_track_ant_sel(pst_hal_device, (oal_uint32)0x02);

            pst_mac_device->uc_lock_channel = 0x02;
        }
    }
}
#endif

#ifdef _PRE_FEATURE_WAVEAPP_CLASSIFY
/*ʶ��������������ͬMCS���������֡��Ŀ*/
#define MAX_FRAME_NUM_FOR_WAVEAPP_CHECK     2000


OAL_STATIC OAL_INLINE oal_void dmac_rx_check_is_waveapp_test(
            mac_vap_stru        *pst_vap,
            dmac_rx_ctl_stru    *pst_cb_ctrl)
{
    dmac_vap_stru                      *pst_dmac_vap;
    dmac_user_stru                     *pst_dmac_user;
    mac_ieee80211_frame_stru           *pst_frame_hdr;

    hal_to_dmac_device_stru   *pst_hal_device;
    oal_uint8   uc_rate;
    oal_uint8   uc_protocol_mode;
    oal_bool_enum_uint8     en_flag;

    pst_dmac_vap  = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_vap->uc_vap_id);

    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(pst_vap->uc_vap_id, OAM_SF_RX, "{dmac_rx_check_is_waveapp_test::pst_dmac_vap null.}");
        return;
    }

    pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user(MAC_GET_RX_CB_TA_USER_IDX(&(pst_cb_ctrl->st_rx_info)));

    if (OAL_PTR_NULL == pst_dmac_user)
    {
        OAM_ERROR_LOG0(pst_vap->uc_vap_id, OAM_SF_RX, "{dmac_rx_check_is_waveapp_test::pst_dmac_user null.}");
        return;
    }

    pst_hal_device = pst_dmac_vap->pst_hal_device;
    if (OAL_PTR_NULL == pst_hal_device)
    {
        OAM_ERROR_LOG0(pst_vap->uc_vap_id, OAM_SF_RX, "{dmac_rx_check_is_waveapp_test::pst_hal_device null.}");
        return;
    }
    en_flag = pst_hal_device->en_test_is_on_waveapp_flag;
    /* ��ʶ��Ϊ�������߷�������ͳ�ƴ����ﵽ���ޣ��򲻼���ʶ�� */
    if ((en_flag != OAL_BUTT)||(pst_hal_device->ul_rx_mcs_cnt >= MAX_FRAME_NUM_FOR_WAVEAPP_CHECK))
    {
        return;
    }
    /* ��Qos����֡,��ʶ��*/
    pst_frame_hdr = (mac_ieee80211_frame_stru *)(mac_get_rx_cb_mac_hdr(&(pst_cb_ctrl->st_rx_info)));
    if (OAL_PTR_NULL == pst_frame_hdr)
    {
        OAM_ERROR_LOG0(pst_vap->uc_vap_id, OAM_SF_RX, "{dmac_rx_check_is_waveapp_test::pst_frame_hdr null.}");
        return;
    }

    if (WLAN_QOS_DATA != pst_frame_hdr->st_frame_control.bit_sub_type)
    {
        return;
    }
    if(WLAN_VHT_MODE == pst_vap->en_protocol)
    {
        uc_protocol_mode = pst_cb_ctrl->st_rx_statistic.un_nss_rate.st_vht_nss_mcs.bit_protocol_mode;
        if ((0x03 != uc_protocol_mode))
        {
            return;
        }
        /* ��ȡMCS���� */
        uc_rate = pst_cb_ctrl->st_rx_statistic.un_nss_rate.st_vht_nss_mcs.bit_vht_mcs;
    }
    else if(WLAN_HT_MODE == pst_vap->en_protocol)
    {
        uc_protocol_mode = pst_cb_ctrl->st_rx_statistic.un_nss_rate.st_ht_rate.bit_protocol_mode;
        if ((0x02 != uc_protocol_mode))
        {
            return;
        }
        /* ��ȡMCS���� */
        uc_rate = pst_cb_ctrl->st_rx_statistic.un_nss_rate.st_ht_rate.bit_ht_mcs;
    }
    else
    {
        //����Э��ģʽ�����������ݲ���Ҫ�Զ�����ʶ����������Ż�
        return;
    }

    /* ��ʼʶ�� */
    if (0xffff == pst_hal_device->us_rx_assoc_id)
    {
        pst_hal_device->us_rx_assoc_id = pst_dmac_user->st_user_base_info.us_assoc_id;
    }
    else if (pst_hal_device->us_rx_assoc_id == pst_dmac_user->st_user_base_info.us_assoc_id)
    {
        if ( uc_rate != pst_hal_device->uc_rx_expect_mcs)
        {
            en_flag = OAL_FALSE;
            pst_hal_device->ul_rx_mcs_cnt = 0;
        }
        else
        {
            pst_hal_device->ul_rx_mcs_cnt++;
            /* �������յ�Ԥ�ڵ����MCS����֡, ͳ�ƴ����ﵽ���ޣ����ж�Ϊ�������� */
            if (MAX_FRAME_NUM_FOR_WAVEAPP_CHECK == pst_hal_device->ul_rx_mcs_cnt)
            {
                en_flag = OAL_TRUE;
            }
        }
        /* ��Ϊ���������������˳��ж�,�ָ�����ӦAGCͨ������response data rate */
        if (en_flag != pst_hal_device->en_test_is_on_waveapp_flag)
        {
            hal_set_is_waveapp_test(pst_hal_device, en_flag);
        }

    }
}
#endif
#endif


oal_uint32  dmac_mac_error_process_event(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru                     *pst_event;
    hal_error_irq_event_stru           *pst_error_irq_event;
    hal_to_dmac_device_stru            *pst_hal_device;

    mac_device_stru                    *pst_device;
    oal_uint32                          ul_error1_irq_state = 0;
    oal_uint32                          ul_error2_irq_state = 0;
    hal_mac_error_type_enum_uint8       en_error_id = 0;
    oal_uint8                           uc_show_error_mask[] = {
                                    #if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
                                        HAL_MAC_ERROR_RXBUFF_LEN_TOO_SMALL,
                                    #endif
                                    #if defined(_PRE_PRODUCT_ID_HI110X_DEV)
                                        HAL_MAC_ERROR_UNEXPECTED_RX_Q_EMPTY,
                                        HAL_MAC_ERROR_UNEXPECTED_HIRX_Q_EMPTY,
                                        HAL_MAC_ERROR_UNEXPECTED_RX_DESC_ADDR,
                                    #endif
                                    #if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)
                                        HAL_MAC_ERROR_OBSS_NAV_PORT,
                                    #endif
                                        HAL_MAC_ERROR_BA_ENTRY_NOT_FOUND
                                        };
    oal_uint32                          ul_error_mask = 0;
    oal_uint8                           uc_err_mask_idx;
    oal_uint8                           uc_status_idx    = 0;
    oal_uint32                          ul_err_code      = 0;

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    oal_bool_enum_uint8                 en_report_flag = OAL_FALSE;
#if (_PRE_CONFIG_TARGET_PRODUCT == _PRE_TARGET_PRODUCT_TYPE_E5)
    dmac_reset_para_stru            st_reset_param;
#endif
#endif

#ifdef _PRE_WLAN_DFT_REG
    oal_uint32                          ul_ret;
#endif

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(0, OAM_SF_IRQ, "{dmac_mac_error_process_event::null param.}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡ�¼�ͷ���¼��ṹ��ָ�� */
    pst_event               = frw_get_event_stru(pst_event_mem);
    pst_error_irq_event     = (hal_error_irq_event_stru *)(pst_event->auc_event_data);
    pst_hal_device          = pst_error_irq_event->pst_hal_device;
    ul_error1_irq_state = pst_error_irq_event->st_error_state.ul_error1_val;
    ul_error2_irq_state = pst_error_irq_event->st_error_state.ul_error2_val;

#ifdef _PRE_WLAN_DFT_REG
    hal_debug_refresh_reg_ext(pst_hal_device, OAM_REG_EVT_RX, &ul_ret);
    if (OAL_SUCC == ul_ret)
    {
        hal_debug_frw_evt(pst_hal_device);
    }
#endif

    pst_device = mac_res_get_dev(pst_event->st_event_hdr.uc_device_id);
    if (OAL_PTR_NULL == pst_device)
    {
        OAM_ERROR_LOG1(0, OAM_SF_IRQ, "{dmac_mac_error_process_event::mac_res_get_dev return null. device_id=[%d]}\r\n",
                       pst_event->st_event_hdr.uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

#ifdef _PRE_WLAN_FEATURE_DFR
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    /* ����pcie err���� */
    pst_hal_device->ul_pcie_err_cnt += oal_pci_check_clear_error_nonfatal(pst_hal_device->uc_chip_id);
#endif
#endif

    /* ���ϱ�HAL_MAC_ERROR_RXBUFF_LEN_TOO_SMALL �� HAL_MAC_ERROR_MATRIX_CALC_TIMEOUT �� HAL_MAC_ERROR_BA_ENTRY_NOT_FOUND �Ĵ����ж� */
    if((ul_error1_irq_state &
        (~(
      #if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
        1 << HAL_MAC_ERROR_RXBUFF_LEN_TOO_SMALL | 1 << HAL_MAC_ERROR_MATRIX_CALC_TIMEOUT |
      #endif
        1 << HAL_MAC_ERROR_BA_ENTRY_NOT_FOUND))) ||
        (ul_error2_irq_state
      #if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)
        &(~(
        1 << (HAL_MAC_ERROR_OBSS_NAV_PORT - 32)))
      #endif
         )
      )
    {
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
        OAM_WARNING_LOG4(0, OAM_SF_IRQ, "{dmac_mac_error_process_event::chip id=[%d], error1 state=[0x%08x], erro2 state=[0x%08x], beacon miss counter=%d\r\n",
             pst_event->st_event_hdr.uc_chip_id, ul_error1_irq_state, ul_error2_irq_state, pst_hal_device->ul_beacon_miss_show_counter);

#else
        OAM_WARNING_LOG3(0, OAM_SF_IRQ, "{dmac_mac_error_process_event::chip id=[%d], error1 state=[0x%08x], erro2 state=[0x%08x]\r\n",
           pst_event->st_event_hdr.uc_chip_id, ul_error1_irq_state, ul_error2_irq_state);
#endif
    }

    for (uc_status_idx = 0; uc_status_idx < 2; uc_status_idx++)
    {
        if (uc_status_idx == 0)
        {
            ul_err_code = ul_error1_irq_state;
            en_error_id = 0;
        }
        else
        {
            ul_err_code = ul_error2_irq_state;
            en_error_id = 32; /* ƫ��32��ȡ64λ����һ��32λul����ʼλ�� */
        }

        while (ul_err_code)
        {
            if (HAL_MAC_ERROR_TYPE_BUTT <= en_error_id)
            {
                OAM_WARNING_LOG2(0, OAM_SF_IRQ, "{dmac_mac_error_process_event::error type[%d], ul_err_code = [%d].}\r\n",
                                en_error_id, ul_err_code);
                ul_err_code = 0;

                break;
            }

            if (0 == (ul_err_code & BIT0))
            {
                en_error_id++;
                ul_err_code = ul_err_code >> 1;
                continue;
            }

            /* 0.Ԥ���� */
            g_st_err_proc_func[en_error_id].p_func(pst_hal_device, (g_st_err_proc_func[en_error_id].ul_param));

            /* 1.����QEMPTY�Ĵ��� */
            if(en_error_id == HAL_MAC_ERROR_UNEXPECTED_RX_Q_EMPTY)
            {
                dmac_rx_resume_dscr_queue(pst_device, pst_hal_device, HAL_RX_DSCR_NORMAL_PRI_QUEUE);
            }
            else if(en_error_id == HAL_MAC_ERROR_UNEXPECTED_HIRX_Q_EMPTY)
            {
                dmac_rx_resume_dscr_queue(pst_device, pst_hal_device, HAL_RX_DSCR_HIGH_PRI_QUEUE);
            }
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV)||(_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)
            else if(en_error_id == HAL_MAC_ERROR_RX_SMALL_Q_EMPTY)
            {
                dmac_rx_resume_dscr_queue(pst_device, pst_hal_device, HAL_RX_DSCR_SMALL_QUEUE);
            }
#endif

            /* 2.����beacon miss�ȴ��� */
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
            if((en_error_id == HAL_MAC_ERROR_BEACON_MISS) ||(en_error_id == HAL_MAC_ERROR_RX_FSM_ST_TIMEOUT)
                ||(en_error_id == HAL_MAC_ERROR_TX_FSM_ST_TIMEOUT)||(en_error_id == HAL_MAC_ERROR_TX_DATAFLOW_BREAK))
            {
                if (en_error_id == HAL_MAC_ERROR_BEACON_MISS)
                {
                    if ((pst_hal_device->ul_beacon_miss_show_counter <= 50) || (pst_hal_device->ul_beacon_miss_show_counter % 100 == 0))
                    {
                        hal_mac_error_msg_report(pst_hal_device, en_error_id);
                        /* BEACON MISSά�� */
                        hal_get_beacon_miss_status(pst_hal_device);
                        en_report_flag = OAL_TRUE;
                    }
                }
                else if (en_error_id == HAL_MAC_ERROR_RX_FSM_ST_TIMEOUT)
                {
                    if ((pst_hal_device->ul_rx_fsm_st_timeout_show_counter <= 50) || (pst_hal_device->ul_rx_fsm_st_timeout_show_counter % 100 == 0))
                    {
                        hal_mac_error_msg_report(pst_hal_device, en_error_id);
                        en_report_flag = OAL_TRUE;
                    }
                }
                else if (en_error_id == HAL_MAC_ERROR_TX_FSM_ST_TIMEOUT)
                {
                    if ((pst_hal_device->ul_tx_fsm_st_timeout_show_counter <= 50) || (pst_hal_device->ul_tx_fsm_st_timeout_show_counter % 100 == 0))
                    {
                        hal_mac_error_msg_report(pst_hal_device, en_error_id);
                        en_report_flag = OAL_TRUE;
                    }
                }
                else
                {
                    hal_mac_error_msg_report(pst_hal_device, en_error_id);
                    /* DATAFLOW_BREAKά�� */
                    dmac_tx_dataflow_break_error(pst_device, pst_hal_device);
                    en_report_flag = OAL_TRUE;
                }
            }
            else if (en_error_id == HAL_MAC_ERROR_TX_INTR_FIFO_OVERRUN)
            {
                hal_mac_error_msg_report(pst_hal_device, en_error_id);
                dmac_intr_fifo_overrun_error(pst_device, pst_hal_device);

                en_report_flag = OAL_TRUE;
            }
            else if ((en_error_id == HAL_MAC_ERROR_RX_INTR_FIFO_OVERRUN)
                    || (en_error_id == HAL_MAC_ERROR_HIRX_INTR_FIFO_OVERRUN))
            {
                hal_mac_error_msg_report(pst_hal_device, en_error_id);
#ifndef _PRE_WLAN_PRODUCT_1151V200
                dmac_intr_fifo_overrun_error(pst_device, pst_hal_device);
#endif
                en_report_flag = OAL_TRUE;
            }
#ifdef _PRE_WLAN_PRODUCT_1151V200
            else if (en_error_id == HAL_MAC_ERROR_NAV_THRESHOLD_ERR)
            {
                if(pst_hal_device->ul_nav_threshold_err_show_counter % 100 == 0)
                {
                    hal_mac_error_msg_report(pst_hal_device, en_error_id);
                    en_report_flag = OAL_TRUE;
                }
            }
#endif /* _PRE_WLAN_PRODUCT_1151V200 */
#if (_PRE_CONFIG_TARGET_PRODUCT == _PRE_TARGET_PRODUCT_TYPE_E5)
            else if (en_error_id == HAL_MAC_ERROR_BUS_WADDR_ERR)
            {
                OAL_MEMZERO(&st_reset_param, sizeof(st_reset_param));
                st_reset_param.uc_reset_type = HAL_RESET_HW_TYPE_MAC;
                dmac_reset_hw(pst_device, pst_hal_device, (oal_uint8 *)&st_reset_param);

                hal_mac_error_msg_report(pst_hal_device, en_error_id);
            }
#endif
            else
            {
                hal_mac_error_msg_report(pst_hal_device, en_error_id);
                en_report_flag = OAL_TRUE;
            }
#else
            hal_mac_error_msg_report(pst_hal_device, en_error_id);
    #ifdef _PRE_WLAN_FEATURE_BTCOEX
            if (HAL_MAC_ERROR_BEACON_MISS == en_error_id)
            {
                dmac_btcoex_beacon_miss_handler(pst_hal_device);
            }
    #endif
#endif
            /* 3.�ô�����ֹ���ʱ���� */
            if (dmac_mac_error_overload(pst_device, en_error_id))
            {
                 /*�������̫���ʱ�򣬲��������ϱ�����˲���Ҫunmask��bit,��tbtt�ж���ͳһ��0xffffffff unmaskȫ������*/
                 if ((en_error_id == HAL_MAC_ERROR_BA_ENTRY_NOT_FOUND)
                 #if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
                   ||(en_error_id == HAL_MAC_ERROR_RXBUFF_LEN_TOO_SMALL)
                 #endif
                   )
                 {
                     OAM_WARNING_LOG1(0, OAM_SF_IRQ, "{dmac_mac_error_process_event::error type[%d] overload.}\r\n", en_error_id);
                     ul_error1_irq_state &= ~((oal_uint32)1 << en_error_id);
                 }
                 else
                 {
                     OAM_ERROR_LOG1(0, OAM_SF_IRQ, "{dmac_mac_error_process_event::error type[%d] overload.}\r\n", en_error_id);
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151) && defined(_PRE_WLAN_PRODUCT_1151V200)
                     if (en_error_id <= HAL_MAC_ERROR_NAV_THRESHOLD_ERR)
                     {
                         ul_error1_irq_state &= ~((oal_uint32)1 << en_error_id);
                     }
#else
                     if (en_error_id <= HAL_MAC_ERROR_RESERVED_31)
                     {
                         ul_error1_irq_state &= ~((oal_uint32)1 << en_error_id);
                     }
#endif
                     else
                     {
                         ul_error2_irq_state &= ~((oal_uint32)1 << (en_error_id - 32)); /* ��ȥ32����ʾ64λ������ȡ��32bitλ�Ĵ���ֵ */
                     }
                 }
            }

            /* 4. ���mac �����Ƿ���Ҫ�����λ���� */
            dmac_dfr_process_mac_error(pst_device, pst_hal_device, en_error_id, ul_error1_irq_state, ul_error2_irq_state);

            en_error_id++;
            ul_err_code = ul_err_code >> 1;
        }
    }

    /*���㲻��Ҫ��ӡpcie�������͵����λͼ*/
    for (uc_err_mask_idx = 0; uc_err_mask_idx < OAL_SIZEOF(uc_show_error_mask) / OAL_SIZEOF(oal_uint8); uc_err_mask_idx++)
    {
        ul_error_mask |= 1U << uc_show_error_mask[uc_err_mask_idx];
    }

    /* ���ڷ�MASK��Errorʱ�Ŵ�ӡpcie״̬ */
    if ((ul_error1_irq_state & ~(ul_error_mask)) || (ul_error2_irq_state))
    {
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
        //��ֹ��ӡ̫�ֻࣻ�е���hal_mac_error_msg_report(beacon missÿ100�λ����һ��)��ʱ��Ŵ�ӡfsm info
        if (OAL_TRUE == en_report_flag)
#endif
        {
            hal_show_fsm_info(pst_hal_device);
        }
    }

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    /* ά��: wow״̬��Ӧ�ڴ����ж� */
    hal_show_wow_state_info(pst_hal_device);
#endif

    /* ��mac�ж�״̬ */
    hal_device_process_mac_error_status(pst_hal_device, ul_error1_irq_state, ul_error2_irq_state);

    /* �ڳ���mac error ����ʱ��ǿ�����µ��ȣ���������ж϶�ʧ�޷����ȵ���� */
    dmac_tx_complete_schedule(pst_hal_device, WLAN_WME_AC_BE);

    return OAL_SUCC;
}

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)

oal_uint32  dmac_soc_error_process_event(frw_event_mem_stru *pst_event_mem)
{
    dmac_reset_para_stru                st_reset;
    frw_event_stru                     *pst_event;
    hal_error_irq_event_stru           *pst_error_irq_event;
    mac_device_stru                    *pst_mac_device;
    hal_to_dmac_device_stru            *pst_hal_device;
    hal_soc_error_type_enum_uint8       en_error_id = 0;
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{dmac_soc_error_process_event::pst_event_mem null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡ�¼�ͷ���¼��ṹ��ָ�� */
    pst_event               = frw_get_event_stru(pst_event_mem);
    pst_error_irq_event     = (hal_error_irq_event_stru *)(pst_event->auc_event_data);

    pst_mac_device = mac_res_get_dev(pst_event->st_event_hdr.uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{dmac_soc_error_process_event::pst_mac_device null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hal_device = pst_error_irq_event->pst_hal_device;
    if (OAL_PTR_NULL == pst_hal_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{dmac_soc_error_process_event::pst_hal_device null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    /*��ȡfsm״̬����Ϣ*/
    hal_show_fsm_info(pst_hal_device);

    for (en_error_id = 0; en_error_id < HAL_SOC_ERROR_TYPE_BUTT; en_error_id++)
    {
        if (pst_error_irq_event->st_error_state.ul_error1_val & (1 << en_error_id))
        {
            hal_soc_error_msg_report(0, en_error_id);
        }
    }

    /*��λMAC+PHY���߼�*/
    st_reset.uc_reset_type = HAL_RESET_HW_TYPE_ALL;
    st_reset.uc_reset_mac_mod = HAL_RESET_MAC_ALL;
    st_reset.uc_reset_mac_reg = OAL_FALSE;
    st_reset.uc_reset_phy_reg = OAL_FALSE;

    dmac_reset_hw(pst_mac_device, pst_hal_device, (oal_uint8*)&st_reset);
    hal_en_soc_intr(pst_hal_device);

    /* �ڳ���soc_error ����ʱ��ǿ�����µ��ȣ���������ж϶�ʧ�޷����ȵ���� */
    dmac_tx_complete_schedule(pst_hal_device, WLAN_WME_AC_BE);

    return OAL_SUCC;
}
#endif


/*lint -e801*/
#if (_PRE_OS_VERSION_RAW == _PRE_OS_VERSION)  && defined (__CC_ARM)
#pragma arm section rwdata = "BTCM", code ="ATCM", zidata = "BTCM", rodata = "ATCM"
#endif

dmac_rx_frame_ctrl_enum_uint8  dmac_rx_process_frame(
                mac_vap_stru                   *pst_vap,
                dmac_rx_ctl_stru               *pst_cb_ctrl,
                oal_netbuf_stru                *pst_netbuf,
                oal_netbuf_head_stru           *pst_netbuf_header)
                //dmac_rx_frame_ctrl_enum_uint8  *pen_frame_ctrl)
{
    mac_ieee80211_frame_stru           *pst_frame_hdr;
    oal_uint8                          *puc_transmit_addr;
    oal_uint8                          *puc_dest_addr;
    dmac_user_stru                     *pst_ta_dmac_user = OAL_PTR_NULL;
    oal_uint16                          us_user_idx      = 0xffff;
    oal_nl80211_key_type                en_key_type;
    oal_uint32                          ul_ret = OAL_FAIL;
    oal_uint16                          us_dscr_status;
#ifdef _PRE_WLAN_FEATURE_RX_AGGR_EXTEND
    mac_chip_stru                      *pst_mac_chip;
    mac_device_stru                    *pst_mac_device;
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE) && defined(_PRE_FEATURE_WAVEAPP_CLASSIFY)
    hal_to_dmac_device_stru   *pst_hal_device;
#endif


#if defined (_PRE_WLAN_DFT_STAT) || defined (_PRE_WLAN_HW_TEST) \
    ||(defined(_PRE_FEATURE_WAVEAPP_CLASSIFY) && (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE))
    dmac_vap_stru               *pst_dmac_vap;
    pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "mac_res_get_dmac_vap::pst_dmac_vap null. vap_id[%u]",pst_vap->uc_vap_id);
        goto rx_pkt_drop;
    }
#endif

#ifdef _PRE_WLAN_FEATURE_RX_AGGR_EXTEND
    pst_mac_chip = mac_res_get_mac_chip(pst_vap->uc_chip_id);
    if(OAL_PTR_NULL == pst_mac_chip)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "mac_res_get_mac_chip::pst_mac_chip null. vap_id[%u]",pst_vap->uc_vap_id);
        goto rx_pkt_drop;
    }
    pst_mac_device = mac_res_get_dev(pst_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "mac_res_get_dev::pst_mac_device null. vap_id[%u]",pst_vap->uc_vap_id);
        goto rx_pkt_drop;
    }
#endif

    /* ͳ��VAP������յ���MPDU���� */
    OAM_STAT_VAP_INCR(pst_vap->uc_vap_id, rx_mpdu_total_num, 1);
    DMAC_VAP_DFT_STATS_PKT_INCR(pst_dmac_vap->st_query_stats.ul_rx_mpdu_total_num, 1);

    /* ��ȡ802.11ͷָ�롢���Ͷ˵�ַ */
    pst_frame_hdr     = (mac_ieee80211_frame_stru *)(mac_get_rx_cb_mac_hdr(&(pst_cb_ctrl->st_rx_info)));
    puc_transmit_addr = pst_frame_hdr->auc_address2;
    puc_dest_addr     = pst_frame_hdr->auc_address1;

    dmac_rx_record_tid(pst_vap, pst_frame_hdr);

    /* ���Ͷ˵�ַΪ�㲥֡��ֱ�Ӷ��� */
    if (ETHER_IS_MULTICAST(puc_transmit_addr))
    {
        //OAM_INFO_LOG0(pst_vap->uc_vap_id, OAM_SF_RX, "{dmac_rx_process_frame::the transmit addr is multicast.}");

// *pen_frame_ctrl = DMAC_RX_FRAME_CTRL_DROP;
        goto rx_pkt_drop;
    }

    us_dscr_status = pst_cb_ctrl->st_rx_status.bit_dscr_status;

    /* VAP ID Ϊ0�������֡����������BSS�Ĺ㲥����֡ */
#ifdef _PRE_WLAN_HW_TEST
    if ((HAL_ALWAYS_RX_DISABLE == pst_dmac_vap->pst_hal_device->bit_al_rx_flag) && (0 == pst_vap->uc_vap_id))
#else
    if (0 == pst_vap->uc_vap_id)
#endif
    {
        //OAM_INFO_LOG0(pst_vap->uc_vap_id, OAM_SF_RX, "this frame is bcast frame from other bss. exit func dmac_rx_process_frame");

        if ((HAL_RX_SUCCESS != us_dscr_status)
             || (WLAN_DATA_BASICTYPE == pst_frame_hdr->st_frame_control.bit_type))
        {
            //OAM_INFO_LOG2(pst_vap->uc_vap_id, OAM_SF_RX, "{dmac_rx_process_frame::dscr_status[%d]type[%d]", us_dscr_status, pst_frame_hdr->st_frame_control.bit_type);
            goto rx_pkt_drop;
        }

        /* ����֡(����beacon probe req��)��Ҫ��ǰ��cb��user idΪ�Ƿ�ֵ����Ȼ��0�Ļ���������Чֵ */
        MAC_GET_RX_CB_TA_USER_IDX(&(pst_cb_ctrl->st_rx_info)) = us_user_idx;

        return DMAC_RX_FRAME_CTRL_BUTT;
    }

    /* �Ե���֡���й��� ���Ƿ����ҵ�֡��ֱ�ӹ��˵� */
    if (OAL_FALSE == ETHER_IS_MULTICAST(puc_dest_addr))
    {
#ifdef _PRE_WLAN_FEATURE_P2P
        ul_ret = oal_compare_mac_addr(puc_dest_addr, mac_mib_get_StationID(pst_vap))
                    && oal_compare_mac_addr(puc_dest_addr, mac_mib_get_p2p0_dot11StationID(pst_vap));
#else
        ul_ret = oal_compare_mac_addr(puc_dest_addr, mac_mib_get_StationID(pst_vap));
#endif  /* _PRE_WLAN_FEATURE_P2P */
        if (0 != ul_ret)
        {
            goto rx_pkt_drop;
        }
    }
    OAL_MIPS_RX_STATISTIC(DMAC_PROFILING_FUNC_RX_DMAC_HANDLE_PER_MPDU_FILTER_ADDR_VAP);

    if (WLAN_VAP_MODE_BSS_STA == dmac_vap_get_bss_type(pst_vap))
    {
        us_user_idx = pst_vap->us_assoc_vap_id;
    }
    else if(WLAN_VAP_MODE_BSS_AP == dmac_vap_get_bss_type(pst_vap))
    {
        /* ��ȡ���Ͷ˵��û�ָ�룬���浽cb�ֶ��У�uapd���ܹ���ʱ���õ� */
        ul_ret = mac_vap_find_user_by_macaddr(pst_vap, puc_transmit_addr, &us_user_idx);
        if (OAL_ERR_CODE_PTR_NULL == ul_ret)
        {
            //OAM_INFO_LOG0(pst_vap->uc_vap_id, OAM_SF_RX, "{dmac_rx_process_frame::mac_vap_find_user_by_macaddr return null.}");
            OAM_STAT_VAP_INCR(pst_vap->uc_vap_id, rx_ta_check_dropped, 1);
            DMAC_VAP_DFT_STATS_PKT_INCR(pst_dmac_vap->st_query_stats.ul_rx_ta_check_dropped, 1);

// *pen_frame_ctrl = DMAC_RX_FRAME_CTRL_DROP;
            goto rx_pkt_drop;
        }
    }
    pst_ta_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user(us_user_idx);

    if (OAL_PTR_NULL != pst_ta_dmac_user)
    {
        /* �����û�ʱ��� */
        pst_ta_dmac_user->ul_last_active_timestamp = (oal_uint32)OAL_TIME_GET_STAMP_MS();

    #ifdef _PRE_DEBUG_MODE_USER_TRACK
        /* ������Դ��û���֡Э��ģʽ�Ƿ�ı䣬����ı������ϱ� */
        dmac_user_check_txrx_protocol_change(pst_ta_dmac_user,
                                        HAL_GET_DATA_PROTOCOL_MODE(*((oal_uint8 *)&(pst_cb_ctrl->st_rx_statistic.un_nss_rate))),
                                        OAM_USER_INFO_CHANGE_TYPE_RX_PROTOCOL);
    #endif

        /*��Ϣ�ϱ�ͳ�ƣ��ṩ��linux�ں�PKT����*/
        if (WLAN_DATA_BASICTYPE == pst_frame_hdr->st_frame_control.bit_type)
        {
            DMAC_USER_STATS_PKT_INCR(pst_ta_dmac_user->st_query_stats.ul_drv_rx_pkts, 1);
            DMAC_USER_STATS_PKT_INCR(pst_ta_dmac_user->st_query_stats.ul_drv_rx_bytes,
                                    (pst_cb_ctrl->st_rx_info.us_frame_len - MAC_GET_RX_CB_MAC_HEADER_LEN(&(pst_cb_ctrl->st_rx_info))) - SNAP_LLC_FRAME_LEN);
        }

#ifdef _PRE_WLAN_FEATURE_USER_EXTEND
        /* update chip active user dlist. */
        dmac_user_update_active_user_dlist(pst_ta_dmac_user);
#endif

    }

    /*OAM_INFO_LOG1(pst_vap->uc_vap_id, OAM_SF_RX, "{dmac_rx_process_frame::bit_dscr_status=%d. }", pst_cb_ctrl->st_rx_status.bit_dscr_status);*/

    if (HAL_RX_SUCCESS != us_dscr_status)
    {
        /* ����������statusΪkey search failʱ���쳣���� */
        if (HAL_RX_KEY_SEARCH_FAILURE == us_dscr_status)
        {
            /* ����waveapp����ʱ���û�״̬��Ҫ��BA״̬����һ�� */
        #ifdef _PRE_WLAN_FEATURE_RX_AGGR_EXTEND
            if(!pst_mac_chip->en_waveapp_32plus_user_enable)
            {
                dmac_user_key_search_fail_handler(pst_vap,pst_ta_dmac_user,pst_frame_hdr);
            }
        #else
            dmac_user_key_search_fail_handler(pst_vap,pst_ta_dmac_user,pst_frame_hdr);
        #endif
// *pen_frame_ctrl = DMAC_RX_FRAME_CTRL_DROP;
#ifdef _PRE_WLAN_FEATURE_SOFT_CRYPTO
            if(OAL_PTR_NULL != pst_ta_dmac_user)
            {
            #ifdef _PRE_WLAN_FEATURE_RX_AGGR_EXTEND
                if(pst_mac_chip->en_waveapp_32plus_user_enable)
                {
                    /* �������û�����ʱֻ�зǾۺ�֡����Ҫ����� �ۺ�֡����ʧ��ʱBAҲ���ڱ������������ش� */
                    if(OAL_FALSE == pst_cb_ctrl->st_rx_status.bit_AMPDU)
                    {
                        /* ������ܣ���������ʧ�ܣ��ٶ��� */
                        ul_ret          = dmac_rx_decrypt_data_frame(pst_netbuf, us_user_idx);
                        us_dscr_status  = pst_cb_ctrl->st_rx_status.bit_dscr_status;
                    }
                    if(pst_mac_chip->pst_rx_aggr_extend->uc_rx_ba_seq_index < pst_mac_device->uc_rx_ba_session_num)
                    {
                        us_dscr_status = HAL_RX_SUCCESS;
                    }
                }
                else
                {
                    /* ������ܣ���������ʧ�ܣ��ٶ��� */
                    ul_ret          = dmac_rx_decrypt_data_frame(pst_netbuf, us_user_idx);
                    us_dscr_status  = pst_cb_ctrl->st_rx_status.bit_dscr_status;
                }
            #else
                /* ������ܣ���������ʧ�ܣ��ٶ��� */
                ul_ret          = dmac_rx_decrypt_data_frame(pst_netbuf, us_user_idx);
                us_dscr_status  = pst_cb_ctrl->st_rx_status.bit_dscr_status;
            #endif
            }

            if ((us_dscr_status != HAL_RX_SUCCESS) || (ul_ret != OAL_SUCC))
#endif
            {
                OAM_STAT_VAP_INCR(pst_vap->uc_vap_id, rx_key_search_fail_dropped, 1);
                DMAC_VAP_DFT_STATS_PKT_INCR(pst_dmac_vap->st_query_stats.ul_rx_key_search_fail_dropped, 1);

                goto rx_pkt_drop;
            }
        }

        /* ����������statusΪ tkip mic faile ʱ���쳣���� */
        if (HAL_RX_TKIP_MIC_FAILURE == us_dscr_status)
        {
            OAM_WARNING_LOG2(pst_vap->uc_vap_id, OAM_SF_RX, "{dmac_rx_process_frame:: tkip mic err from src addr=%02x:%02x.}",
                    puc_transmit_addr[4], puc_transmit_addr[5]);

        #ifdef _PRE_WLAN_PRODUCT_1151V200
            if ((OAL_PTR_NULL != pst_ta_dmac_user)
                && (OAL_FALSE == pst_ta_dmac_user->st_user_base_info.en_is_reauth_user))
        #endif
            {
                 /*�ж���Կ����,peerkey �ݲ�����*/
                en_key_type = ETHER_IS_MULTICAST(puc_dest_addr) ? NL80211_KEYTYPE_GROUP : NL80211_KEYTYPE_PAIRWISE;
                dmac_11i_tkip_mic_failure_handler(pst_vap, puc_transmit_addr, en_key_type);

                OAM_STAT_VAP_INCR(pst_vap->uc_vap_id, rx_tkip_mic_fail_dropped, 1);
                DMAC_VAP_DFT_STATS_PKT_INCR(pst_dmac_vap->st_query_stats.ul_rx_tkip_mic_fail_dropped, 1);
                goto rx_pkt_drop;
            }
        }

        if (OAL_TRUE != dmac_rx_check_mgmt_replay_failure(pst_cb_ctrl))
        {
            /* if ((HAL_RX_TKIP_REPLAY_FAILURE != pst_cb_ctrl->st_rx_status.bit_dscr_status) && (HAL_RX_CCMP_REPLAY_FAILURE != pst_cb_ctrl->st_rx_status.bit_dscr_status)) */
            OAM_WARNING_LOG1(pst_vap->uc_vap_id, OAM_SF_ANY, "{dmac_rx_process_frame::bit_dscr_status=%d.}", us_dscr_status);
// *pen_frame_ctrl = DMAC_RX_FRAME_CTRL_DROP;

            OAM_STAT_VAP_INCR(pst_vap->uc_vap_id, rx_replay_fail_dropped, 1);
            DMAC_VAP_DFT_STATS_PKT_INCR(pst_dmac_vap->st_query_stats.ul_rx_replay_fail_dropped, 1);
            goto rx_pkt_drop;
        }

#if (defined(_PRE_PRODUCT_ID_HI110X_DEV))
        if (HAL_RX_CCMP_REPLAY_FAILURE == us_dscr_status)
        {
            if (OAL_PTR_NULL != pst_ta_dmac_user)
            {
                DMAC_USER_STATS_PKT_INCR(pst_ta_dmac_user->st_query_stats.ul_rx_replay,1);
            }
            if (OAL_TRUE == ETHER_IS_BROADCAST(puc_dest_addr)
                && mac_get_data_type(pst_netbuf) == MAC_DATA_ARP_REQ)/*lint !e731 */
            {
                if (OAL_PTR_NULL != pst_ta_dmac_user)
                {
                    DMAC_USER_STATS_PKT_INCR(pst_ta_dmac_user->st_query_stats.ul_rx_replay_droped,1);
                }
                goto rx_pkt_drop;
            }
        }
#endif
    }

    /* ��ȡ���û�index�������cb�ֶ��У���������ʹ��(dmac_rx_filter_frame) */
    MAC_GET_RX_CB_TA_USER_IDX(&(pst_cb_ctrl->st_rx_info)) = us_user_idx;

    if (OAL_PTR_NULL != pst_ta_dmac_user)
    {
        OAM_STAT_USER_INCR(us_user_idx, rx_mpdu_num, 1);

        /*��Ϣ�ϱ�ͳ�ƣ��ṩ��linux�ں�PKT����*/
        if (WLAN_DATA_BASICTYPE == pst_frame_hdr->st_frame_control.bit_type)
        {
#if (!defined(_PRE_PRODUCT_ID_HI110X_DEV))
            OAM_STAT_USER_INCR(us_user_idx, rx_mpdu_bytes,
                                pst_cb_ctrl->st_rx_info.us_frame_len - pst_cb_ctrl->st_rx_info.uc_mac_header_len - SNAP_LLC_FRAME_LEN);
#endif
        }
    }

    OAL_MIPS_RX_STATISTIC(DMAC_PROFILING_FUNC_RX_DMAC_HANDLE_PER_MPDU_FILTER_DSCR_SEC);

#ifdef _PRE_WLAN_MAC_BUGFIX_SA_FILTER
    if (OAL_FALSE == mac_is_4addr((oal_uint8 *)pst_frame_hdr))
    {
        if (((WLAN_VAP_MODE_BSS_STA == dmac_vap_get_bss_type(pst_vap))
            && (0 == oal_compare_mac_addr(pst_frame_hdr->auc_address3, mac_mib_get_StationID(pst_vap))))
            || ((WLAN_VAP_MODE_BSS_AP == dmac_vap_get_bss_type(pst_vap))
                 && (HAL_ALWAYS_RX_DISABLE == pst_dmac_vap->pst_hal_device->bit_al_rx_flag)
                 && (0 == oal_compare_mac_addr(pst_frame_hdr->auc_address2, mac_mib_get_StationID(pst_vap)))))
        {
            goto rx_pkt_drop;
        }
    }
#endif

    /* ����һ:��Է��Ͷ˵�ַ����(��������״̬��AMPDU���ԡ����ܴ���) */
    ul_ret = dmac_rx_filter_frame(pst_vap, pst_cb_ctrl, pst_ta_dmac_user);
    if (OAL_SUCC != ul_ret)        /* �����쳣 */
    {
        //OAM_INFO_LOG1(pst_vap->uc_vap_id, OAM_SF_RX, "{dmac_rx_process_frame::dmac_rx_filter_frame failed[%d].}", ul_ret);
        goto rx_pkt_drop;
    }
    OAL_MIPS_RX_STATISTIC(DMAC_PROFILING_FUNC_RX_DMAC_HANDLE_PER_MPDU_FILTER_CIPHER_AMPDU);

    if (WLAN_DATA_BASICTYPE == pst_frame_hdr->st_frame_control.bit_type)
    {
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE) && defined(_PRE_FEATURE_WAVEAPP_CLASSIFY)
        pst_hal_device = pst_dmac_vap->pst_hal_device;
        if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hal_device))
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "dmac_rx_process_frame::pst_hal_device null. vap_id[%u]",pst_vap->uc_vap_id);
            goto rx_pkt_drop;
        }

        //���������������㷨���̲���Ҫ
        if(pst_hal_device->en_test_is_on_waveapp_flag != OAL_TRUE)
        {
#endif


        /* AGC��ͨ���趨  */
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
#ifndef _PRE_WLAN_PRODUCT_1151V200
        dmac_set_agc_lock_channel(pst_vap, pst_cb_ctrl);
#endif
#endif
        ul_ret = dmac_rx_process_data_frame(pst_vap, pst_cb_ctrl, pst_netbuf);
        OAL_MEM_NETBUF_TRACE(pst_netbuf, OAL_TRUE);

        if (OAL_SUCC != ul_ret)
        {
// *pen_frame_ctrl = DMAC_RX_FRAME_CTRL_DROP;
            //OAM_INFO_LOG1(pst_vap->uc_vap_id, OAM_SF_RX, "{dmac_rx_process_frame::dmac_rx_process_data_frame failed[%d].}", ul_ret);
            goto rx_pkt_drop;
        }

        /*�͹��ı��ļ������ӣ��ڴ˴����ã��㲥����ARP����null֡���ѱ�drop������ͳ��*/
#ifdef _PRE_WLAN_FEATURE_STA_PM
        if (WLAN_VAP_MODE_BSS_STA == dmac_vap_get_bss_type(pst_vap))
        {
            dmac_psm_rx_inc_pkt_cnt(pst_vap,pst_netbuf);
        }
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE) && defined(_PRE_FEATURE_WAVEAPP_CLASSIFY)
    }
    /*ʶ���Ƿ�Ϊ��������*/
    /* ���ι�տڱ�ʶ������� Ӱ��2G 5G TCP���� ��ʱȥ��ʶ���� */
    //dmac_rx_check_is_waveapp_test(pst_vap,pst_cb_ctrl);
#endif
    }

#ifdef _PRE_WLAN_DFT_STAT
    dmac_rx_vap_stat(pst_dmac_vap, pst_netbuf_header, pst_netbuf, pst_cb_ctrl);
#endif
    OAL_MIPS_RX_STATISTIC(DMAC_PROFILING_FUNC_RX_DMAC_HANDLE_PER_MPDU_FILTER_ALG_PSM_NULL);

    return DMAC_RX_FRAME_CTRL_BUTT;

rx_pkt_drop:
    /* ��֡���� */
    if (OAL_PTR_NULL != pst_ta_dmac_user)
    {
        /*��Ϣ�ϱ�ͳ�ƣ��ṩ��linux�ں�PKT����*/
        DMAC_USER_STATS_PKT_INCR(pst_ta_dmac_user->st_query_stats.ul_rx_dropped_misc,1);
    }

    return DMAC_RX_FRAME_CTRL_DROP;
}
/*lint +e801*/
#if (_PRE_OS_VERSION_RAW == _PRE_OS_VERSION)  && defined (__CC_ARM)
#pragma arm section rodata, code, rwdata, zidata  // return to default placement
#endif

#if 0

oal_uint32 dmac_start_stat_rssi(dmac_user_stru *pst_dmac_user)
{
    pst_dmac_user->st_user_rate_info.en_dmac_rssi_stat_flag = OAL_TRUE;

    return OAL_SUCC;
}

oal_uint32 dmac_get_stat_rssi(dmac_user_stru *pst_dmac_user, oal_int8 *pc_tx_rssi, oal_int8 *pc_rx_rssi)
{
    dmac_user_rate_info_stru   *pst_user_info;

    pst_user_info = &pst_dmac_user->st_user_rate_info;

    /* ��ȡƽ��ACK RSSI */
    if (0 != pst_user_info->st_dmac_rssi_stat_info.us_tx_rssi_stat_count)
    {
        *pc_tx_rssi = (oal_int8)(pst_user_info->st_dmac_rssi_stat_info.l_tx_rssi / pst_user_info->st_dmac_rssi_stat_info.us_tx_rssi_stat_count);
    }

    /* ��ȡƽ������ RSSI */
    if (0 != pst_user_info->st_dmac_rssi_stat_info.us_rx_rssi_stat_count)
    {
        *pc_rx_rssi = (oal_int8)(pst_user_info->st_dmac_rssi_stat_info.l_rx_rssi / pst_user_info->st_dmac_rssi_stat_info.us_rx_rssi_stat_count);
    }

    /* ���ͳ����Ϣ */
    OAL_MEMZERO(&pst_user_info->st_dmac_rssi_stat_info, OAL_SIZEOF(dmac_rssi_stat_info_stru));

    return OAL_SUCC;
}


oal_uint32 dmac_stop_stat_rssi(dmac_user_stru *pst_dmac_user)
{
    pst_dmac_user->st_user_rate_info.en_dmac_rssi_stat_flag = OAL_FALSE;

    return OAL_SUCC;
}



oal_uint32 dmac_start_stat_rate(dmac_user_stru *pst_dmac_user)
{
    pst_dmac_user->st_user_rate_info.en_dmac_rate_stat_flag = OAL_TRUE;

    return OAL_SUCC;
}
#endif


oal_uint32 dmac_get_stat_rate(dmac_user_stru *pst_dmac_user, oal_uint32 *pul_tx_rate, oal_uint32 *pul_rx_rate)
{
    dmac_user_rate_info_stru   *pst_user_info;

    pst_user_info = &pst_dmac_user->st_user_rate_info;

    /* ��ȡƽ���������� */
    if (0 != pst_user_info->st_dmac_rate_stat_info.us_tx_rate_stat_count)
    {
        *pul_tx_rate = (oal_uint32)(pst_user_info->st_dmac_rate_stat_info.ul_tx_rate / pst_user_info->st_dmac_rate_stat_info.us_tx_rate_stat_count);
    }

    /* ��ȡƽ���������� */
    if (0 != pst_user_info->st_dmac_rate_stat_info.us_rx_rate_stat_count)
    {
        *pul_rx_rate = (oal_uint32)(pst_user_info->st_dmac_rate_stat_info.ul_rx_rate / pst_user_info->st_dmac_rate_stat_info.us_rx_rate_stat_count);
    }

    /* ���ͳ����Ϣ */
    OAL_MEMZERO(&pst_user_info->st_dmac_rate_stat_info, OAL_SIZEOF(dmac_rate_stat_info_stru));

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_IP_FILTER

OAL_STATIC oal_bool_enum_uint8 dmac_check_ip_btable(mac_ip_filter_item_stru *pst_filter_item)
{
    oal_uint8 uc_items_indx;

    /* �����������ڣ����ؼ��ʧ�ܵ�״̬ */
    if (OAL_PTR_NULL == g_pst_mac_board->st_rx_ip_filter.pst_filter_btable)
    {
        return OAL_FALSE;
    }

    /* ��ʼ��� */
    for (uc_items_indx = 0; uc_items_indx < g_pst_mac_board->st_rx_ip_filter.uc_btable_items_num; uc_items_indx++)
    {
        if ((pst_filter_item->uc_protocol == g_pst_mac_board->st_rx_ip_filter.pst_filter_btable[uc_items_indx].uc_protocol)&&
            (pst_filter_item->us_port == g_pst_mac_board->st_rx_ip_filter.pst_filter_btable[uc_items_indx].us_port))
        {
            return OAL_TRUE;
        }
    }

    return OAL_FALSE;
}


dmac_rx_frame_ctrl_enum_uint8 dmac_rx_ip_filter_process(dmac_vap_stru *pst_dmac_vap, oal_netbuf_stru *pst_netbuf, dmac_rx_ctl_stru *pst_cb_ctrl)
{
    mac_llc_snap_stru   *pst_snap;
    oal_bool_enum_uint8  en_is_need_filter;
    mac_ip_header_stru  *pst_ip;
    mac_ip_filter_item_stru st_filter_btable;

    /* AMSDU�������� */
    if(OAL_TRUE == pst_cb_ctrl->st_rx_info.bit_amsdu_enable)
    {
        return DMAC_RX_FRAME_CTRL_GOON;
    }

    /* ����δ��&���ڹ��� & ���Ϊ�� ״̬������Ҫ��֡���н��� */
    if ((OAL_TRUE != pst_dmac_vap->st_vap_base_info.st_cap_flag.bit_ip_filter) ||
        (MAC_RX_IP_FILTER_WORKING != g_pst_mac_board->st_rx_ip_filter.en_state) ||
        (0 == g_pst_mac_board->st_rx_ip_filter.uc_btable_items_num))
    {
        return DMAC_RX_FRAME_CTRL_GOON;
    }

    /* ������IP���� */
    pst_snap = (mac_llc_snap_stru *)oal_netbuf_payload(pst_netbuf);
    if(OAL_PTR_NULL == pst_snap)
    {
        return DMAC_RX_FRAME_CTRL_GOON;
    }

    /* ��ȡЭ��&Ŀ�Ķ˿ں� */
    switch (pst_snap->us_ether_type)
    {
        case OAL_HOST2NET_SHORT(ETHER_TYPE_IP):
            pst_ip = (mac_ip_header_stru *)(((oal_uint8 *)pst_snap) + (oal_uint16)OAL_SIZEOF(mac_llc_snap_stru));
            st_filter_btable.uc_protocol = pst_ip->uc_protocol;

            if (MAC_UDP_PROTOCAL == pst_ip->uc_protocol)
            {
                udp_hdr_stru *pst_udp_hdr;
                pst_udp_hdr = (udp_hdr_stru *)(pst_ip + 1);
                st_filter_btable.us_port = pst_udp_hdr->us_des_port;
            }
            else if (MAC_TCP_PROTOCAL == pst_ip->uc_protocol)
            {
                mac_tcp_header_stru *pst_tcp_hdr;
                pst_tcp_hdr = (mac_tcp_header_stru *)(pst_ip + 1);
                st_filter_btable.us_port = pst_tcp_hdr->us_dport;
            }
            else
            {
                return DMAC_RX_FRAME_CTRL_GOON;
            }

            break;
        /* IPV6���ݲ����� */

        default:
            return DMAC_RX_FRAME_CTRL_GOON;
    }

    /* ���������� */
    en_is_need_filter = (oal_bool_enum_uint8 )dmac_check_ip_btable(&st_filter_btable);
    if (OAL_TRUE == en_is_need_filter)
    {
        return DMAC_RX_FRAME_CTRL_DROP;
    }

    return DMAC_RX_FRAME_CTRL_GOON;
}

#endif //_PRE_WLAN_FEATURE_IP_FILTER
#if 0

oal_uint32 dmac_stop_stat_rate(dmac_user_stru *pst_dmac_user)
{
    dmac_user_rate_info_stru   *pst_user_info;
    pst_user_info = &pst_dmac_user->st_user_rate_info;

    if (OAL_TRUE == pst_user_info->en_dmac_rate_stat_flag)
    {
        pst_user_info->en_dmac_rate_stat_flag = OAL_FALSE;
        /* ���ͳ����Ϣ */
        OAL_MEMZERO(&pst_user_info->st_dmac_rate_stat_info, OAL_SIZEOF(dmac_rate_stat_info_stru));
    }
    return OAL_SUCC;
}
#endif

/*lint -e19*/
oal_module_symbol(dmac_rx_process_data_event);
/*lint +e19*/

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

