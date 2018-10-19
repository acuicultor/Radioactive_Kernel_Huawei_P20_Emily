


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 ͷ�ļ�����
*****************************************************************************/
#include "hal_ext_if.h"
#include "mac_ie.h"
#include "dmac_main.h"
#include "dmac_alg.h"
#include "dmac_mgmt_bss_comm.h"
#include "dmac_tx_bss_comm.h"
#include "dmac_chan_mgmt.h"
#include "dmac_scan.h"
#include "oam_ext_if.h"
#include "dmac_beacon.h"
#include "dmac_mgmt_sta.h"
#include "dmac_mgmt_ap.h"
#include "dmac_config.h"
#ifdef _PRE_WLAN_FEATURE_STA_PM
#include "dmac_psm_sta.h"
#endif
#ifdef _PRE_WLAN_FEATURE_BAND_STEERING
#include "dmac_bsd.h"
#endif
#ifdef _PRE_WLAN_FEATURE_M2S
#include "dmac_m2s.h"
#endif

#ifdef _PRE_WLAN_FEATURE_DFS_OPTIMIZE
#include "dmac_radar.h"
#endif

#include "dmac_power.h"
#include "dmac_csa_sta.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_CHAN_MGMT_C

/*****************************************************************************
  2 ȫ�ֱ�������
*****************************************************************************/


/*****************************************************************************
  3 ����ʵ��
*****************************************************************************/

oal_void  dmac_chan_update_user_bandwidth(mac_vap_stru *pst_mac_vap)
{
    wlan_bw_cap_enum_uint8    en_bwcap_ap;
    wlan_bw_cap_enum_uint8    en_bandwidth_cap;
    mac_user_stru            *pst_mac_user;
    wlan_bw_cap_enum_uint8    en_bwcap_min;
    oal_dlist_head_stru      *pst_entry;

    mac_vap_get_bandwidth_cap(pst_mac_vap, &en_bwcap_ap);

    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_mac_vap->st_mac_user_list_head))
    {
        pst_mac_user = OAL_DLIST_GET_ENTRY(pst_entry, mac_user_stru, st_user_dlist);
        /*lint -save -e774 */
        if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_user))
        {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_2040, "{dmac_chan_update_user_bandwidth::pst_user null pointer.}");
            continue;
        }
        /*lint -restore */

        if ( (pst_mac_user->uc_chip_id == pst_mac_vap->uc_chip_id) &&
             (pst_mac_user->uc_device_id == pst_mac_vap->uc_device_id) &&
             (pst_mac_user->uc_vap_id == pst_mac_vap->uc_vap_id) )
         {
            mac_user_get_sta_cap_bandwidth(pst_mac_user, &en_bandwidth_cap);
            en_bwcap_min = OAL_MIN(en_bwcap_ap, en_bandwidth_cap);
            mac_user_set_bandwidth_info(pst_mac_user, en_bwcap_min, en_bwcap_min);

            /* user��������㷨�ı����֪ͨ��,��ͬ����host�� */
            dmac_alg_cfg_user_bandwidth_notify(pst_mac_vap, pst_mac_user);
            dmac_config_d2h_user_info_syn(pst_mac_vap, pst_mac_user);

            OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_2040,
                "dmac_chan_update_user_bandwidth::update us_assoc_id=[%d] bw_cap=[%d] avail_bw=[%d] cur_bw=[%d].",
                pst_mac_user->us_assoc_id, en_bandwidth_cap, pst_mac_user->en_avail_bandwidth,
                pst_mac_user->en_cur_bandwidth);
         }
    }
}


oal_uint32  dmac_chan_initiate_switch_to_new_channel(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru                  *pst_event;
    frw_event_hdr_stru              *pst_event_hdr;
    dmac_vap_stru                   *pst_dmac_vap;
    mac_vap_stru                    *pst_mac_vap;
    dmac_set_ch_switch_info_stru    *pst_ch_switch_info;
    mac_device_stru                 *pst_mac_device;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{dmac_chan_initiate_switch_to_new_channel::pst_event_mem null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡ�¼����¼�ͷ�Լ��¼�payload�ṹ�� */
    pst_event          = frw_get_event_stru(pst_event_mem);
    pst_event_hdr      = &(pst_event->st_event_hdr);
    pst_ch_switch_info = (dmac_set_ch_switch_info_stru *)pst_event->auc_event_data;

    pst_dmac_vap  = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_event_hdr->uc_vap_id);

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dmac_vap))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{dmac_chan_initiate_switch_to_new_channel::pst_dmac_vap[%d] is NULL!}", pst_event_hdr->uc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_vap   = &(pst_dmac_vap->st_vap_base_info);
    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG1(pst_event_hdr->uc_vap_id, OAM_SF_ANY, "{dmac_chan_initiate_switch_to_new_channel::pst_mac_device[%d] is NULL!}", pst_mac_vap->uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAM_WARNING_LOG4(pst_event_hdr->uc_vap_id, OAM_SF_ANY, "{dmac_chan_initiate_switch_to_new_channel::csa_channel=[%d],csa_cnt=[%d],csa_mode=[%d],bandwidth=[%d]}",
        pst_ch_switch_info->uc_announced_channel,pst_ch_switch_info->uc_ch_switch_cnt,pst_ch_switch_info->en_csa_mode,
        pst_ch_switch_info->en_announced_bandwidth);
    /* ͬ��channel switch info */
    pst_mac_vap->st_ch_switch_info.en_ch_switch_status    = pst_ch_switch_info->en_ch_switch_status;
    pst_mac_vap->st_ch_switch_info.uc_announced_channel   = pst_ch_switch_info->uc_announced_channel;
    pst_mac_vap->st_ch_switch_info.en_announced_bandwidth = pst_ch_switch_info->en_announced_bandwidth;
    pst_mac_vap->st_ch_switch_info.uc_ch_switch_cnt       = pst_ch_switch_info->uc_ch_switch_cnt;
    pst_mac_vap->st_ch_switch_info.en_csa_present_in_bcn  = pst_ch_switch_info->en_csa_present_in_bcn;
    pst_mac_vap->st_ch_switch_info.en_csa_mode            = pst_ch_switch_info->en_csa_mode;

    dmac_encap_beacon(pst_dmac_vap, pst_dmac_vap->pauc_beacon_buffer[pst_dmac_vap->uc_beacon_idx], &(pst_dmac_vap->us_beacon_len));

    pst_mac_device->uc_csa_vap_cnt = pst_ch_switch_info->uc_csa_vap_cnt;

    /* 11n Э��ģʽ��֧��CSA action ֡  ���� Channel Switch Announcement ֡ */
    if (OAL_TRUE == mac_mib_get_HighThroughputOptionImplemented(&pst_dmac_vap->st_vap_base_info))
    {
        return dmac_mgmt_send_csa_action(pst_dmac_vap, pst_dmac_vap->st_vap_base_info.st_ch_switch_info.uc_announced_channel,
                                     pst_dmac_vap->st_vap_base_info.st_ch_switch_info.uc_ch_switch_cnt,
                                     pst_dmac_vap->st_vap_base_info.st_ch_switch_info.en_announced_bandwidth);
    }

    return OAL_SUCC;

}


oal_uint32  dmac_mgmt_switch_channel(hal_to_dmac_device_stru *pst_hal_device, mac_channel_stru *pst_channel, oal_bool_enum en_clear_fifo)
{
    oal_uint                 ul_irq_flag;

    /* ��·Ϊ�ղ�ִ�� */
    if (OAL_PTR_NULL == pst_hal_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{dmac_mgmt_switch_channel::pst_hal_device NULL}");
        return OAL_FAIL;
    }

    OAM_PROFILING_CHSWITCH_STATISTIC(OAM_PROFILING_CHSWITCH_START);

#if 0 //03 wifi��bt����rf���ŵ������²���Ҫ���˲����� ֻ��02FPGA�²���Ҫ
#ifdef _PRE_WLAN_FEATURE_BTCOEX
#if (_PRE_WLAN_CHIP_ASIC != _PRE_WLAN_CHIP_VERSION)
    /* �ж� BT �Ƿ���*/
    if(1 == pst_hal_device->st_btcoex_btble_status.un_bt_status.st_bt_status.bit_bt_on)
    {
        oal_uint32 ul_mode_sel = 0;
        oal_uint32 ul_delay_cnt = 0;
        hal_set_btcoex_occupied_period(50000);    // 50ms
        /* �ж��Ƿ���mode sel */
        hal_get_btcoex_pa_status(&ul_mode_sel);
        while (BIT23 == (ul_mode_sel & BIT23))
        {
            oal_udelay(10);
            if(ul_delay_cnt++ > 1000)
            {
                OAM_ERROR_LOG1(0, OAM_SF_COEX, "{dmac_mgmt_switch_channel:ul_mode_sel = 0x%x!}",ul_mode_sel);
                break;
            }
            hal_get_btcoex_pa_status(&ul_mode_sel);
        }
        oal_udelay(50);     // 50us
    }
#endif
#endif
#endif

    /* �ر�pa */
    hal_disable_machw_phy_and_pa(pst_hal_device);

    /* ���жϣ�����Ӳ��������Ҫ���ж� */
    oal_irq_save(&ul_irq_flag, OAL_5115IRQ_DMSC);

    /* ����Ƶ�� */
    hal_set_freq_band(pst_hal_device, pst_channel->en_band);

#if (_PRE_WLAN_CHIP_ASIC == _PRE_WLAN_CHIP_VERSION)
        /*dummy*/
#else
    if (pst_channel->en_bandwidth >= WLAN_BAND_WIDTH_80PLUSPLUS)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RX, "{dmac_mgmt_switch_channel:: fpga is not support 80M.}\r\n");
        pst_channel->en_bandwidth = WLAN_BAND_WIDTH_20M;
    }
#endif

    /* ���ô��� */
    hal_set_bandwidth_mode(pst_hal_device, pst_channel->en_bandwidth);

    /* �����ŵ��� */
    hal_set_primary_channel(pst_hal_device, pst_channel->uc_chan_number, pst_channel->en_band, pst_channel->uc_chan_idx, pst_channel->en_bandwidth);

    /* ���ж� */
    oal_irq_restore(&ul_irq_flag, OAL_5115IRQ_DMSC);

    /* Set TRUE to clear FIFO when scan switch channel; set FALSE when just bandwidth changed  */
    if (OAL_TRUE == en_clear_fifo)
    {
        hal_clear_tx_hw_queue(pst_hal_device);

        /* ��fifo֮��ҲҪɾ��tx����������֡ */
        dmac_clear_tx_queue(pst_hal_device);
    }

    /* ʹ��pa */
    hal_recover_machw_phy_and_pa(pst_hal_device);

#ifdef _PRE_WLAN_DFT_EVENT
    oam_event_chan_report(pst_channel->uc_chan_number);
#endif

    OAM_PROFILING_CHSWITCH_STATISTIC(OAM_PROFILING_CHSWITCH_END);

#if 0 //03 wifi��bt����rf���ŵ������²���Ҫ���˲����� ֻ��02FPGA�²���Ҫ
#ifdef _PRE_WLAN_FEATURE_BTCOEX
#if (_PRE_WLAN_CHIP_ASIC != _PRE_WLAN_CHIP_VERSION)
    hal_set_btcoex_occupied_period(0);    // 0us
#endif
#endif
#endif

    return OAL_SUCC;
}
/*lint -e19*/
oal_module_symbol(dmac_mgmt_switch_channel);
/*lint +e19*/


oal_void  dmac_chan_select_channel_mac(mac_vap_stru           *pst_mac_vap,
                                       oal_uint8                            uc_channel,
                                       wlan_channel_bandwidth_enum_uint8    en_bandwidth)
{
    hal_to_dmac_device_stru        *pst_hal_device;
    mac_device_stru                *pst_mac_device;
    oal_uint8                       uc_idx;
    oal_uint32                      ul_ret;
    mac_channel_stru                st_channel;
    oal_bool_enum                   en_is_support_bw;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{dmac_chan_select_channel_mac::pst_mac_vap null.}");

        return;
    }

    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{dmac_chan_select_channel_mac::pst_mac_device null.}");
        return;
    }

    pst_hal_device = DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap);
    if (OAL_PTR_NULL == pst_hal_device)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{dmac_chan_select_channel_mac::DMAC_VAP_GET_HAL_DEVICE null}");
        return;
    }

    /* ����VAP�µ���20MHz�ŵ��š�����ģʽ���ŵ����� */
    ul_ret = mac_get_channel_idx_from_num(pst_mac_vap->st_channel.en_band, uc_channel, &uc_idx);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{dmac_chan_select_channel_mac::mac_get_channel_idx_from_num failed[%d].}", ul_ret);

        return;
    }

    en_is_support_bw = mac_regdomain_channel_is_support_bw(en_bandwidth, uc_channel);
    if(OAL_FALSE == en_is_support_bw)
    {
        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CSA,"{dmac_chan_select_channel_mac::channel[%d] is not support bw[%d],set BW 20MHz}",
        uc_channel, en_bandwidth);
        en_bandwidth = WLAN_BAND_WIDTH_20M;
    }

    pst_mac_vap->st_channel.uc_chan_number = uc_channel;
    pst_mac_vap->st_channel.en_bandwidth   = en_bandwidth;
    pst_mac_vap->st_channel.uc_chan_idx    = uc_idx;
    st_channel                             = pst_mac_vap->st_channel;

#ifdef _PRE_WLAN_FEATURE_DBAC
    if (mac_is_dbac_running(pst_mac_device))
    {
        dmac_alg_update_dbac_fcs_config(pst_mac_vap);
        return;
    }
#endif

    if (IS_AP(pst_mac_vap))
    {
        dmac_chan_update_user_bandwidth(pst_mac_vap);
    }

#ifdef _PRE_WLAN_FEATURE_DFS
    /* ʹ��ȥʹ���״��� */
    if(IS_AP(pst_mac_vap) &&(OAL_TRUE == mac_vap_get_dfs_enable(pst_mac_vap)))
    {
        dmac_dfs_radar_detect_check(pst_hal_device, pst_mac_device, pst_mac_vap);
    }
#endif

    /* ˢ�·��͹��� */
    dmac_pow_set_vap_tx_power(pst_mac_vap, HAL_POW_SET_TYPE_INIT);

    /* ֪ͨ�㷨�ŵ��ı� */
    dmac_alg_cfg_channel_notify(pst_mac_vap, CH_BW_CHG_TYPE_MOVE_WORK);

    /* ֪ͨ�㷨����ı� */
    dmac_alg_cfg_bandwidth_notify(pst_mac_vap, CH_BW_CHG_TYPE_MOVE_WORK);

    /* ѡ����Ҫ���õ��ŵ���Ϣ */
    dmac_chan_select_real_channel(pst_mac_device, &st_channel, pst_mac_vap->st_channel.uc_chan_number);

    /* �л��ŵ�����Ҫ��fifo������FALSE */
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    dmac_mgmt_switch_channel(pst_hal_device, &st_channel, OAL_TRUE);
#else

    dmac_mgmt_switch_channel(pst_hal_device, &st_channel, OAL_FALSE);
#endif

}


oal_uint32  dmac_chan_sync(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru           *pst_event;
    frw_event_hdr_stru       *pst_event_hdr;
    dmac_set_chan_stru       *pst_set_chan;
    mac_vap_stru             *pst_mac_vap;
    oal_bool_enum_uint8       en_teb;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{dmac_chan_sync::pst_event_mem null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡ�¼����¼�ͷ�Լ��¼�payload�ṹ�� */
    pst_event     = frw_get_event_stru(pst_event_mem);
    pst_event_hdr = &(pst_event->st_event_hdr);
    pst_set_chan  = (dmac_set_chan_stru *)pst_event->auc_event_data;

    pst_mac_vap   = (mac_vap_stru *)mac_res_get_mac_vap(pst_event_hdr->uc_vap_id);
    if (OAL_PTR_NULL == pst_mac_vap)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{dmac_chan_sync::mac_res_get_mac_vap fail.vap_id:%u}",pst_event_hdr->uc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }
    en_teb = pst_mac_vap->st_ch_switch_info.en_te_b;
    oal_memcopy(&pst_mac_vap->st_channel, &pst_set_chan->st_channel, OAL_SIZEOF(mac_channel_stru));
    oal_memcopy(&pst_mac_vap->st_ch_switch_info, &pst_set_chan->st_ch_switch_info,
                    OAL_SIZEOF(mac_ch_switch_info_stru));
    pst_mac_vap->st_ch_switch_info.en_te_b = en_teb;

    mac_mib_set_FortyMHzIntolerant(pst_mac_vap, pst_set_chan->en_dot11FortyMHzIntolerant);
    //OAM_INFO_LOG0(0, OAM_SF_2040, "\r\n\r\ndmac_chan_sync\r\n");
    dmac_dump_chan(pst_mac_vap, (oal_uint8*)pst_set_chan);

    if (OAL_TRUE == pst_set_chan->en_switch_immediately)
    {
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        dmac_chan_multi_select_channel_mac(pst_mac_vap, pst_set_chan->st_channel.uc_chan_number, pst_set_chan->st_channel.en_bandwidth);
#else
        dmac_chan_select_channel_mac(pst_mac_vap, pst_set_chan->st_channel.uc_chan_number, pst_set_chan->st_channel.en_bandwidth);
#endif
    }
    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_DFS
#ifdef _PRE_WLAN_FEATURE_OFFCHAN_CAC

oal_uint32  dmac_dfs_switch_to_offchan_event_process(frw_event_mem_stru* pst_event_mem)
{
    frw_event_stru           *pst_event;
    frw_event_hdr_stru       *pst_event_hdr;
    mac_vap_stru             *pst_mac_vap;
    mac_device_stru          *pst_mac_device;
    mac_channel_stru          st_off_chan;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(0, OAM_SF_DFS, "{dmac_chan_select_chan_event_process::pst_event_mem null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡ�¼����¼�ͷ�Լ��¼�payload�ṹ�� */
    pst_event     = frw_get_event_stru(pst_event_mem);
    pst_event_hdr = &(pst_event->st_event_hdr);

    pst_mac_vap   = (mac_vap_stru *)mac_res_get_mac_vap(pst_event_hdr->uc_vap_id);
    pst_mac_device= mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_DFS, "{dmac_dfs_switch_to_offchan_event_process:: vap state[%d].}", pst_mac_vap->en_vap_state);
        return OAL_ERR_CODE_PTR_NULL;
    }
    st_off_chan.uc_chan_number = pst_mac_device->st_dfs.st_dfs_info.uc_offchan_num;
    st_off_chan.en_band        = pst_mac_vap->st_channel.en_band;
    st_off_chan.en_bandwidth   = pst_mac_vap->st_channel.en_bandwidth;
    st_off_chan.uc_chan_idx         = 0;
    mac_get_channel_idx_from_num(st_off_chan.en_band, st_off_chan.uc_chan_number, &st_off_chan.uc_chan_idx);

    //hal_set_machw_tx_suspend(pst_mac_device->pst_device_stru);

    //hal_vap_beacon_suspend(pst_dmac_vap->pst_hal_vap);
    //hal_set_machw_tx_suspend(pst_mac_device->pst_device_stru);

    dmac_vap_pause_tx_by_chl(pst_mac_device, &(pst_mac_vap->st_channel));

    /* �л���offchan���� */
    dmac_switch_channel_off(pst_mac_device, pst_mac_vap, &st_off_chan, pst_mac_device->st_dfs.st_dfs_info.uc_cts_duration);
    //dmac_mgmt_switch_channel(pst_mac_device, &st_off_chan);

    pst_mac_device->st_dfs.st_dfs_info.uc_dmac_channel_flag = 1;

    return OAL_SUCC;
}


oal_uint32  dmac_dfs_switch_back_event_process(frw_event_mem_stru* pst_event_mem)
{
    frw_event_stru           *pst_event;
    frw_event_hdr_stru       *pst_event_hdr;
    mac_vap_stru             *pst_mac_vap;
    mac_device_stru          *pst_mac_device;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(0, OAM_SF_DFS, "{dmac_chan_select_chan_event_process::pst_event_mem null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡ�¼����¼�ͷ�Լ��¼�payload�ṹ�� */
    pst_event     = frw_get_event_stru(pst_event_mem);
    pst_event_hdr = &(pst_event->st_event_hdr);

    pst_mac_vap   = (mac_vap_stru *)mac_res_get_mac_vap(pst_event_hdr->uc_vap_id);
    pst_mac_device= mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_DFS, "{dmac_dfs_switch_back_event_process:: vap state[%d].}", pst_mac_vap->en_vap_state);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hal_device = DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap);
    if (OAL_PTR_NULL == pst_hal_device)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_DFS, "{dmac_dfs_switch_back_event_process:: pst_hal_device is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* �л���home channel���� */
    dmac_scan_switch_channel_back(pst_mac_device, pst_hal_device);

    pst_mac_device->st_dfs.st_dfs_info.uc_dmac_channel_flag = 0;

    if (WLAN_CH_SWITCH_STATUS_1 != pst_mac_vap->st_ch_switch_info.en_ch_switch_status)
    {
        //hal_set_machw_tx_resume(pst_mac_device->pst_device_stru);
        //hal_vap_beacon_resume(pst_dmac_vap->pst_hal_vap);
    }

    return OAL_SUCC;
}
#endif
#endif


#ifdef _PRE_WLAN_FEATURE_DFS
oal_void  dmac_chan_attempt_new_chan(dmac_vap_stru                       *pst_dmac_vap,
                                     oal_uint8                            uc_channel,
                                     wlan_channel_bandwidth_enum_uint8    en_bandwidth)
{
    mac_device_stru   *pst_mac_device;

    pst_mac_device = mac_res_get_dev(pst_dmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_DFS, "{dmac_chan_attempt_new_chan::pst_mac_device null.}");

        return;
    }

    /* ѡ��20/40/80MHz�ŵ� */
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    dmac_chan_multi_select_channel_mac(&(pst_dmac_vap->st_vap_base_info), uc_channel, en_bandwidth);
#else
    dmac_chan_select_channel_mac(&(pst_dmac_vap->st_vap_base_info), uc_channel, en_bandwidth);
#endif

    /* �����ŵ��л�״̬Ϊ WLAN_CH_SWITCH_DONE(���) */
    pst_dmac_vap->st_vap_base_info.st_ch_switch_info.en_ch_switch_status = WLAN_CH_SWITCH_DONE;

    mac_vap_set_bssid(&(pst_dmac_vap->st_vap_base_info), mac_mib_get_StationID(&(pst_dmac_vap->st_vap_base_info)));


    /* �ϱ��ŵ��л�����¼� hmac�ж��Ƿ���ҪCAC��� */
    dmac_switch_complete_notify(&(pst_dmac_vap->st_vap_base_info), OAL_TRUE);

#ifdef _PRE_WLAN_FEATURE_BAND_STEERING
    dmac_bsd_channel_switch_handle(pst_dmac_vap);
#endif

    if (mac_dfs_get_debug_level(pst_mac_device) & 0x1)
    {
        //OAM_INFO_LOG1(0, OAM_SF_DFS, "{dmac_chan_attempt_new_chan::chan switch time(ms): %d.}", ul_delta_time_for_chan_switch);
    }

}

#else

oal_void  dmac_chan_attempt_new_chan(dmac_vap_stru                       *pst_dmac_vap,
                                     oal_uint8                            uc_channel,
                                     wlan_channel_bandwidth_enum_uint8    en_bandwidth)
{
    mac_device_stru   *pst_mac_device;

    /* ѡ��20/40/80MHz�ŵ� */
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    dmac_chan_multi_select_channel_mac(&(pst_dmac_vap->st_vap_base_info), uc_channel, en_bandwidth);
#else
    dmac_chan_select_channel_mac(&(pst_dmac_vap->st_vap_base_info), uc_channel, en_bandwidth);
#endif

    /* DFS �Ƿ���Ҫ����ɨ���ŵ� */

    /* �����ŵ��л�״̬Ϊ WLAN_CH_SWITCH_DONE(���) */
    pst_dmac_vap->st_vap_base_info.st_ch_switch_info.en_ch_switch_status = WLAN_CH_SWITCH_DONE;

    pst_mac_device = mac_res_get_dev(pst_dmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_DFS, "{dmac_chan_attempt_new_chan::pst_mac_device null.}");

        return;
    }

    /* CSA�������� */
    pst_mac_device->uc_csa_cnt = 0;

    mac_vap_set_bssid(&(pst_dmac_vap->st_vap_base_info), mac_mib_get_StationID(&(pst_dmac_vap->st_vap_base_info)));

    /* �����ŵ��ϻָ�Beacon֡�ķ��� */
    hal_vap_beacon_resume(pst_dmac_vap->pst_hal_vap);

    /* �����ŵ��ϻָ�Ӳ���ķ��� */
    hal_set_machw_tx_resume(pst_dmac_vap->pst_hal_device);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    dmac_switch_complete_notify(&(pst_dmac_vap->st_vap_base_info), OAL_FALSE);
#endif

#ifdef _PRE_WLAN_FEATURE_BAND_STEERING
    dmac_bsd_channel_switch_handle(pst_dmac_vap);
#endif
}

#endif   /* end of _PRE_WLAN_FEATURE_DFS */
#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST

OAL_STATIC oal_bool_enum_uint8  dmac_chan_40MHz_possibly(
                mac_device_stru              *pst_mac_device,
                mac_vap_stru                 *pst_mac_vap,
                oal_uint8                     uc_pri_chan_idx,
                mac_sec_ch_off_enum_uint8     en_sec_chan_offset,
                dmac_eval_scan_report_stru   *pst_chan_scan_report)
{
    mac_ap_ch_info_stru           *pst_ap_channel_list;
    wlan_channel_band_enum_uint8   en_band = pst_mac_vap->st_channel.en_band;
    oal_uint8                      uc_num_supp_chan = mac_get_num_supp_channel(en_band);
    oal_uint8                      uc_affected_ch_idx_offset = mac_get_affected_ch_idx_offset(en_band);
    oal_uint8                      uc_sec_ch_idx_offset = mac_get_sec_ch_idx_offset(en_band);
    oal_uint8                      uc_ch_idx;
    oal_uint8                      uc_sec_chan_idx;
    oal_uint8                      uc_affected_chan_lo = 0, uc_affected_chan_hi = 0;
    dmac_network_type_enum_uint8   en_network_type;
    dmac_chan_op_enum_uint8        en_allowed_bit;
    oal_bool_enum_uint8            en_flag_2040_op_permitted = OAL_TRUE;
    oal_uint32                     ul_ret;

    OAM_INFO_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_2040,
        "{dmac_chan_40MHz_possibly::uc_pri_chan_idx=%d,en_sec_chan_offset=%d}",
        uc_pri_chan_idx, en_sec_chan_offset);

    ul_ret = mac_is_channel_idx_valid(en_band, uc_pri_chan_idx);
    if (OAL_SUCC != ul_ret)
    {
        return OAL_FALSE;
    }

    if (MAC_SCA == en_sec_chan_offset)
    {
        /* ������ŵ����� */
        if (uc_num_supp_chan > uc_pri_chan_idx + uc_sec_ch_idx_offset)
        {
            uc_sec_chan_idx = uc_pri_chan_idx + uc_sec_ch_idx_offset;
        }
        else
        {
            return OAL_FALSE;
        }

        en_network_type = DMAC_NETWORK_SCA;
        en_allowed_bit  = DMAC_SCA_ALLOWED;

        /* ������Ӱ����ŵ��������� */
        uc_affected_chan_lo = (uc_pri_chan_idx >= uc_affected_ch_idx_offset) ?
                        (uc_pri_chan_idx - uc_affected_ch_idx_offset) : 0;

        /* ������Ӱ����ŵ��������� */
        uc_affected_chan_hi = (uc_num_supp_chan > uc_sec_chan_idx + uc_affected_ch_idx_offset) ?
                        (uc_sec_chan_idx + uc_affected_ch_idx_offset) : (uc_num_supp_chan - 1);
    }
    else if (MAC_SCB == en_sec_chan_offset)
    {
        /* ������ŵ����� */
        if (uc_pri_chan_idx >= uc_sec_ch_idx_offset)
        {
            uc_sec_chan_idx = uc_pri_chan_idx - uc_sec_ch_idx_offset;
        }
        else
        {
            return OAL_FALSE;
        }

        en_network_type = DMAC_NETWORK_SCB;
        en_allowed_bit  = DMAC_SCB_ALLOWED;

        /* ������Ӱ����ŵ��������� */
        uc_affected_chan_lo = (uc_sec_chan_idx >= uc_affected_ch_idx_offset) ?
                        (uc_sec_chan_idx - uc_affected_ch_idx_offset) : 0;

        /* ������Ӱ����ŵ��������� */
        uc_affected_chan_hi = (uc_num_supp_chan > uc_pri_chan_idx + uc_affected_ch_idx_offset) ?
                        (uc_pri_chan_idx + uc_affected_ch_idx_offset) : (uc_num_supp_chan - 1);
    }
    else
    {
        return OAL_FALSE;
    }

    ul_ret = mac_is_channel_idx_valid(en_band, uc_sec_chan_idx);
    if (OAL_SUCC != ul_ret)
    {
        return OAL_FALSE;
    }

    /*lint -save -e506 */
    if(uc_affected_chan_hi >= MAC_MAX_SUPP_CHANNEL)
    {
        return OAL_FALSE;
    }
    /*lint -restore */

    /* ���ڸ�����"���ŵ� + ���ŵ�ƫ����"�������ķ�Χ��(����Ƶ�� +/- 5���ŵ�)���ж��ܷ���40MHz BSS */
    for (uc_ch_idx = uc_affected_chan_lo; uc_ch_idx <= uc_affected_chan_hi; uc_ch_idx++)
    {
        ul_ret = mac_is_channel_idx_valid(en_band, uc_ch_idx);
        if (OAL_SUCC != ul_ret)
        {
            continue;
        }

        pst_ap_channel_list = &(pst_mac_device->st_ap_channel_list[uc_ch_idx]);

        /* ��������ŵ��ϴ���BSS */
        if (MAC_CH_TYPE_NONE != pst_ap_channel_list->en_ch_type)
        {
            /* �ۼ������ŵ���ɨ�赽��BSS���� */
            pst_chan_scan_report[uc_pri_chan_idx].aus_num_networks[en_network_type] += pst_ap_channel_list->us_num_networks;

            /* ��BSS�����ŵ����������е�20/40MHz BSS�����ŵ��غ� */
            if (uc_ch_idx == uc_pri_chan_idx)
            {
                if (MAC_CH_TYPE_PRIMARY == pst_ap_channel_list->en_ch_type)
                {
                    continue;
                }
            }

            /* ��BSS�Ĵ��ŵ����������е�20/40MHz BSS�Ĵ��ŵ��غ� */
            if (uc_ch_idx == uc_sec_chan_idx)
            {
                if (MAC_CH_TYPE_SECONDARY == pst_ap_channel_list->en_ch_type)
                {
                    continue;
                }
            }

            en_flag_2040_op_permitted = OAL_FALSE;
        }
    }

    /* ���20/40MHz����û��ʹ�ܣ�����Ϊ���Խ���40MHz BSS�������û�������"40MHz������"λ */
    if ((OAL_FALSE == mac_mib_get_2040BSSCoexistenceManagementSupport(pst_mac_vap)) &&
        (OAL_FALSE == mac_mib_get_FortyMHzIntolerant(pst_mac_vap)))
    {
        en_flag_2040_op_permitted = OAL_TRUE;
    }
    /* 5GHz����²�����20/40M�������ú�40M���������ã�һ������40MHz BSS */
    else if (WLAN_BAND_5G == en_band)
    {
        en_flag_2040_op_permitted = OAL_TRUE;
    }

    if (OAL_TRUE == en_flag_2040_op_permitted)
    {
        pst_chan_scan_report[uc_pri_chan_idx].en_chan_op |= en_allowed_bit;
        return OAL_TRUE;
    }

    return OAL_FALSE;
}


oal_bool_enum_uint8  dmac_chan_get_2040_op_chan_list(mac_vap_stru *pst_mac_vap, dmac_eval_scan_report_stru *pst_chan_scan_report)
{
    mac_device_stru       *pst_mac_device;
    oal_uint8              uc_chan_idx;
    oal_uint8              uc_num_supp_chan = mac_get_num_supp_channel(pst_mac_vap->st_channel.en_band);
    oal_bool_enum_uint8    en_fortyMHz_poss = OAL_FALSE;
    oal_bool_enum_uint8    en_flag;

    OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_2040, "{dmac_chan_get_2040_op_chan_list}");

    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_SCAN,
                       "{dmac_chan_get_2040_op_chan_list::pst_mac_device null.}");
        return OAL_FALSE;
    }

    for (uc_chan_idx = 0; uc_chan_idx < uc_num_supp_chan; uc_chan_idx++)
    {
        /* �ж��ܷ���SCA���͵�40MHz BSS */
        en_flag = dmac_chan_40MHz_possibly(pst_mac_device, pst_mac_vap, uc_chan_idx, MAC_SCA, pst_chan_scan_report);
        if (OAL_TRUE == en_flag)
        {
            en_fortyMHz_poss = OAL_TRUE;
        }

        /* �ж��ܷ���SCB���͵�40MHz BSS */
        en_flag = dmac_chan_40MHz_possibly(pst_mac_device, pst_mac_vap, uc_chan_idx, MAC_SCB, pst_chan_scan_report);
        if (OAL_TRUE == en_flag)
        {
            en_fortyMHz_poss = OAL_TRUE;
        }
    }

    /* 2.4GHz�£����"40MHz������"λ�Ƿ����ã���������40MHz BSS */
    if (WLAN_BAND_2G == pst_mac_vap->st_channel.en_band)
    {
        if ((OAL_TRUE == pst_mac_device->en_40MHz_intol_bit_recd) ||
            (OAL_TRUE == mac_mib_get_FortyMHzIntolerant(pst_mac_vap)))
        {
            en_fortyMHz_poss = OAL_FALSE;
        }
    }

    return en_fortyMHz_poss;
}




oal_void dmac_chan_update_40M_intol_user(mac_vap_stru *pst_mac_vap)
{
    oal_dlist_head_stru     *pst_entry;
    mac_user_stru           *pst_mac_user;

    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_mac_vap->st_mac_user_list_head))
    {
        pst_mac_user = OAL_DLIST_GET_ENTRY(pst_entry, mac_user_stru, st_user_dlist);
        /*lint -save -e774 */
        if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_user))
        {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_2040, "{dmac_chan_update_40M_intol_user::pst_user null pointer.}");
            continue;
        }
        else
        {
            if(pst_mac_user->st_ht_hdl.bit_forty_mhz_intolerant)
            {
                pst_mac_vap->en_40M_intol_user = OAL_TRUE;
                return;
            }
        }
        /*lint -restore */
    }

    pst_mac_vap->en_40M_intol_user = OAL_FALSE;
}


oal_uint32 dmac_chan_prepare_for_40M_recovery(dmac_vap_stru *pst_dmac_vap,
                                            wlan_channel_bandwidth_enum_uint8 en_bandwidth)
{
    mac_device_stru *pst_mac_device;

    pst_mac_device = mac_res_get_dev(pst_dmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_2040,
                       "{dmac_chan_prepare_for_40M_recovery::pst_device null,uc_device_id=%d.}",
                       pst_dmac_vap->st_vap_base_info.uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_mac_device->en_40MHz_intol_bit_recd = OAL_FALSE;

    //st_ap_channel_list�ŵ�

    /* ����VAP����ģʽΪ40MHz */
    pst_dmac_vap->st_vap_base_info.st_channel.en_bandwidth = en_bandwidth;

    /* ���ô����л�״̬��������������һ��DTIMʱ���л���20MHz���� */
    pst_dmac_vap->st_vap_base_info.st_ch_switch_info.en_bw_switch_status = WLAN_BW_SWITCH_20_TO_40;

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  dmac_chan_40M_recovery_timeout_fn(void *p_arg)
{
    dmac_vap_stru          *pst_dmac_vap;
    mac_vap_stru           *pst_mac_vap;
    dmac_set_chan_stru      st_set_chan = {{0}};
    oal_uint32              ul_ret;

    OAM_INFO_LOG0(0, OAM_SF_2040, "{dmac_chan_40M_recovery_timeout_fn::40M recovery timer time out.}");

    pst_dmac_vap = (dmac_vap_stru *)p_arg;
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dmac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_2040, "{dmac_chan_40M_recovery_timeout_fn::pst_dmac_vap null.}");
        return OAL_FAIL;
    }
    pst_mac_vap = &pst_dmac_vap->st_vap_base_info;

    if(OAL_TRUE == mac_mib_get_2040SwitchProhibited(pst_mac_vap))
    {
        //dmac_chan_stop_40M_recovery_timer(pst_dmac_vap);
        return OAL_SUCC;
    }

     /* ���ap��ʼ����Ϊ20M, ��ֹͣ��ʱ��*/
    if ((pst_dmac_vap->en_40M_bandwidth == WLAN_BAND_WIDTH_20M)
        || (pst_dmac_vap->en_40M_bandwidth == WLAN_BAND_WIDTH_BUTT))
    {
        OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_2040,
                "{dmac_chan_40M_recovery_timeout_fn::no need 40M recovery because init 20M.}");
        dmac_chan_stop_40M_recovery_timer(pst_dmac_vap);
        return OAL_SUCC;
    }

    if (OAL_TRUE == pst_mac_vap->en_40M_intol_user)
    {
        OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_2040,
                "{dmac_chan_40M_recovery_timeout_fn::no need 40M recovery because 40M intol sta assoc}");
        return OAL_SUCC;
    }

    if ((WLAN_BAND_WIDTH_40PLUS == pst_mac_vap->st_channel.en_bandwidth)
        ||(WLAN_BAND_WIDTH_40MINUS == pst_mac_vap->st_channel.en_bandwidth))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_2040,
                "{dmac_chan_40M_recovery_timeout_fn::no need 40M recovery because already 40M}");
        dmac_chan_stop_40M_recovery_timer(pst_dmac_vap);
        return OAL_SUCC;
    }

    dmac_chan_prepare_for_40M_recovery(pst_dmac_vap, pst_dmac_vap->en_40M_bandwidth);
    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040,
                "{dmac_chan_40M_recovery_timeout_fn::back to bw=%d",pst_dmac_vap->en_40M_bandwidth);


    dmac_chan_multi_select_channel_mac(pst_mac_vap, pst_mac_vap->st_channel.uc_chan_number, pst_dmac_vap->en_40M_bandwidth);
    ul_ret = dmac_send_notify_chan_width(pst_mac_vap, BROADCAST_MACADDR);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040, "{dmac_chan_40M_recovery_timeout_fn::dmac_send_notify_chan_width return %d.}", ul_ret);
        return ul_ret;
    }

    ul_ret = dmac_chan_sync_event(pst_mac_vap, &st_set_chan);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040, "{dmac_chan_40M_recovery_timeout_fn::dmac_chan_sync_event return %d.}", ul_ret);
        return ul_ret;
    }
    return OAL_SUCC;
}

oal_void dmac_chan_start_40M_recovery_timer(dmac_vap_stru *pst_dmac_vap)
{
    oal_uint32                     ul_timeout;

    ul_timeout = mac_mib_get_BSSWidthTriggerScanInterval(&(pst_dmac_vap->st_vap_base_info)) * 2 * 1000;

    OAM_INFO_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_2040, "{dmac_chan_start_40M_recovery_timer::ul_timeout=%d}", ul_timeout);
    if((OAL_TRUE != pst_dmac_vap->st_40M_recovery_timer.en_is_registerd))
    {
        FRW_TIMER_CREATE_TIMER(&(pst_dmac_vap->st_40M_recovery_timer),
                               dmac_chan_40M_recovery_timeout_fn,
                               ul_timeout,
                               (void *)pst_dmac_vap,
                               OAL_TRUE,
                               OAM_MODULE_ID_DMAC,
                               pst_dmac_vap->st_vap_base_info.ul_core_id);
    }
    else
    {
        FRW_TIMER_RESTART_TIMER(&(pst_dmac_vap->st_40M_recovery_timer), ul_timeout, OAL_TRUE);
    }
}
#if !defined(_PRE_WLAN_FEATURE_DFS)

oal_void  dmac_chan_tx_complete_2040_coexist(mac_device_stru *pst_mac_device, hal_to_dmac_device_stru *pst_hal_device, oal_netbuf_stru *pst_netbuf)
{
    oal_uint8       *puc_payload;

    /* ��Channel Switch Announcement֡���ͺ���Ҫ��ֹӲ������ */
    if (mac_ieeee80211_is_action(oal_netbuf_header(pst_netbuf)))
    {
        puc_payload = (oal_uint8 *)mac_netbuf_get_payload(pst_netbuf);

        if ((MAC_ACTION_CATEGORY_SPECMGMT == puc_payload[0]) && (MAC_SPEC_CH_SWITCH_ANNOUNCE == puc_payload[1]))
        {
            pst_mac_device->uc_csa_cnt++;

            /* ��device������AP��CSA֡��������ɺ󣬹���Ӳ������ */
            if (pst_mac_device->uc_csa_cnt == (pst_mac_device->uc_vap_num - pst_mac_device->uc_sta_num))
            {
                //OAM_INFO_LOG0(0, OAM_SF_2040, "{dmac_chan_tx_complete_2040_coexist::machw tx suspend.}\r\n");

                /* ����Ӳ������ */
                hal_set_machw_tx_suspend(pst_hal_device);
            }
        }
    }
}
#endif
#endif

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

