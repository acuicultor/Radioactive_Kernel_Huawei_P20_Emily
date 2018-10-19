
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 ͷ�ļ�����
*****************************************************************************/
#include "oal_types.h"
#include "oal_ext_if.h"
#include "oal_net.h"
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#include "oal_schedule.h"
#endif
#include "oam_ext_if.h"
#include "frw_ext_if.h"
#include "hal_ext_if.h"
#include "mac_regdomain.h"
#include "mac_resource.h"
#include "mac_device.h"
#include "mac_ie.h"
#include "dmac_scan.h"
#include "dmac_main.h"
#include "dmac_fcs.h"
#include "dmac_tx_bss_comm.h"
#include "dmac_ext_if.h"
#include "dmac_device.h"
#include "dmac_mgmt_sta.h"
#include "dmac_alg.h"
#if defined(_PRE_WLAN_CHIP_TEST) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include "dmac_scan_test.h"
#endif

#ifdef _PRE_WLAN_FEATURE_STA_PM
#include "dmac_sta_pm.h"
#include "pm_extern.h"
#endif
#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
#include "dmac_dft.h"
#endif
#include "dmac_config.h"
#include "dmac_chan_mgmt.h"
#include "dmac_mgmt_classifier.h"
#ifdef _PRE_WLAN_FEATURE_11K
#include "dmac_11k.h"
#endif
#ifdef _PRE_WLAN_FEATURE_GREEN_AP
#include "dmac_green_ap.h"
#endif

#ifdef _PRE_WLAN_FEATURE_BAND_STEERING
#include "dmac_bsd.h"
#endif
#ifdef _PRE_WLAN_FEATURE_BTCOEX
#include "dmac_btcoex.h"
#endif

#include "dmac_power.h"
#include "dmac_beacon.h"
#include "dmac_mgmt_bss_comm.h"
#include "dmac_csa_sta.h"

#ifdef _PRE_WLAN_FEATURE_FTM
#include "dmac_ftm.h"
#endif
#ifdef _PRE_WLAN_FEATURE_M2S
#include "dmac_m2s.h"
#endif

#ifdef _PRE_WLAN_FEATURE_GNSS_SCAN
#include "ipc_manage.h"
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_SCAN_C


/*****************************************************************************
  2 ȫ�ֱ�������
*****************************************************************************/
#ifdef _PRE_WLAN_FEATURE_GNSS_SCAN
oal_uint8  g_uc_wifi_support_gscan = OAL_FALSE;   //WIFI�Ƿ�֧��gscan������ɾ��
#endif
/* ��̬�������� */

OAL_STATIC oal_uint32 dmac_scan_send_bcast_probe(mac_device_stru *pst_mac_device, oal_uint8 uc_band, oal_uint8  uc_index);
OAL_STATIC oal_uint32  dmac_scan_report_channel_statistics_result(hal_to_dmac_device_stru  *pst_hal_device, oal_uint8 uc_scan_idx);
OAL_STATIC oal_uint32  dmac_scan_switch_home_channel_work_timeout(void *p_arg);

OAL_STATIC oal_uint32  dmac_scan_start_pno_sched_scan_timer(void *p_arg);
#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
OAL_STATIC oal_uint32  dmac_scan_check_2g_scan_results(mac_device_stru *pst_mac_device, hal_to_dmac_device_stru *pst_hal_device, mac_vap_stru *pst_vap, wlan_channel_band_enum_uint8 en_next_band);
#endif
#ifdef _PRE_WLAN_FEATURE_GNSS_SCAN
OAL_STATIC oal_void dmac_scan_proc_scanned_bss(mac_device_stru *pst_mac_device, oal_netbuf_stru *pst_netbuf);
OAL_STATIC oal_void dmac_scan_dump_bss_list(oal_dlist_head_stru *pst_head);
OAL_STATIC oal_void dmac_scan_check_ap_bss_info(oal_dlist_head_stru *pst_head);
#endif
/*****************************************************************************
  3 ����ʵ��
*****************************************************************************/
#if 0
OAL_STATIC oal_void dmac_scan_print_time_stamp()
{
    oal_uint32                  ul_timestamp;

    ul_timestamp = (oal_uint32)OAL_TIME_GET_STAMP_MS();

    OAM_ERROR_LOG1(0, OAM_SF_SCAN, "{dmac_scan_print_time_stamp:: time_stamp:%d.}", ul_timestamp);

    return;
}
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST

oal_void dmac_detect_2040_te_a_b(dmac_vap_stru *pst_dmac_vap, oal_uint8 *puc_frame_body, oal_uint16 us_frame_len, oal_uint16 us_offset,oal_uint8 uc_curr_chan)
{
    oal_uint8            chan_index     = 0;
    oal_bool_enum_uint8  ht_cap         = OAL_FALSE;
    oal_uint8            uc_scan_chan_idx;
    oal_uint8            uc_real_chan   = uc_curr_chan;
    mac_device_stru     *pst_mac_device = OAL_PTR_NULL;
    oal_uint8           *puc_ie         = OAL_PTR_NULL;

    pst_mac_device = mac_res_get_dev(pst_dmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
                         "{dmac_detect_2040_te_a_b::mac_res_get_dev return null.}");
        return;
    }

    uc_scan_chan_idx = GET_HAL_DEV_CURRENT_SCAN_IDX(pst_dmac_vap->pst_hal_device);

    if (us_frame_len <= us_offset)
    {
        mac_get_channel_idx_from_num((pst_mac_device->st_scan_params.ast_channel_list[uc_scan_chan_idx]).en_band,
                                      uc_real_chan, &chan_index);
        /* Detect Trigger Event - A */
        pst_dmac_vap->st_vap_base_info.st_ch_switch_info.ul_chan_report_for_te_a |= (1U << chan_index);

        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{dmac_detect_2040_te_a_b::framebody_len[%d]}", us_frame_len);
        return;
    }

    us_frame_len   -= us_offset;
    puc_frame_body += us_offset;

    puc_ie = mac_find_ie(MAC_EID_HT_CAP, puc_frame_body, us_frame_len);
    if (OAL_PTR_NULL != puc_ie)
    {
        ht_cap = OAL_TRUE;

        /* Check for the Forty MHz Intolerant bit in HT-Capabilities */
        if((puc_ie[3] & BIT6) != 0)
        {
            //OAM_INFO_LOG0(0, OAM_SF_SCAN, "dmac_detect_2040_te_a_b::40 intolerant in ht cap");
            /* Register Trigger Event - B */
            pst_dmac_vap->st_vap_base_info.st_ch_switch_info.en_te_b = OAL_TRUE;
        }
    }

    puc_ie = mac_find_ie(MAC_EID_2040_COEXT, puc_frame_body, us_frame_len);
    if (OAL_PTR_NULL != puc_ie)
    {
        /* Check for the Forty MHz Intolerant bit in Coex-Mgmt IE */
        if((puc_ie[2] & BIT1) != 0)
        {
            //OAM_INFO_LOG0(0, OAM_SF_SCAN, "dmac_detect_2040_te_a_b::40 intolerant in co");
            /* Register Trigger Event - B */
            pst_dmac_vap->st_vap_base_info.st_ch_switch_info.en_te_b = OAL_TRUE;
        }

    }

    /* ֻ����HT����ΪFalseʱ����Ҫ��ȡ�ŵ���Ϣ����������²���Ҫ���� */
    if(OAL_FALSE == ht_cap)
    {
        puc_ie = mac_find_ie(MAC_EID_DSPARMS, puc_frame_body, us_frame_len);
        if (OAL_PTR_NULL != puc_ie)
        {
            uc_real_chan = puc_ie[2];
        }

        mac_get_channel_idx_from_num((pst_mac_device->st_scan_params.ast_channel_list[uc_scan_chan_idx]).en_band,
                            uc_real_chan, &chan_index);
        pst_dmac_vap->st_vap_base_info.st_ch_switch_info.ul_chan_report_for_te_a |= (1U << chan_index);
    }

    return;
}

#endif

oal_void  dmac_scan_proc_obss_scan_complete_event(dmac_vap_stru *pst_dmac_vap)
{
#if 0
    OAM_INFO_LOG2(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
                 "{dmac_scan_proc_obss_scan_complete_event::te_a:%d,te_b:%d}",
                 pst_dmac_vap->st_vap_base_info.st_ch_switch_info.ul_chan_report_for_te_a,
                 pst_dmac_vap->st_vap_base_info.st_ch_switch_info.en_te_b);
#endif

    if(!pst_dmac_vap->st_vap_base_info.st_ch_switch_info.ul_chan_report_for_te_a
       && (OAL_FALSE == pst_dmac_vap->st_vap_base_info.st_ch_switch_info.en_te_b))
    {
        return;
    }

    dmac_send_2040_coext_mgmt_frame_sta(&(pst_dmac_vap->st_vap_base_info));

    return;
}
#endif


OAL_STATIC oal_void  dmac_scan_set_vap_mac_addr_by_scan_state(mac_device_stru  *pst_mac_device,
                                                                           hal_to_dmac_device_stru *pst_hal_device,
                                                                           oal_bool_enum_uint8 en_is_scan_start)
{
    oal_uint8                      uc_orig_hal_dev_id;
    dmac_vap_stru                 *pst_dmac_vap;
    mac_vap_stru                  *pst_mac_vap;
    mac_scan_req_stru             *pst_scan_params;
    hal_scan_params_stru          *pst_hal_scan_params;

    /* ��ȡɨ����� */
    pst_scan_params = &(pst_mac_device->st_scan_params);

    /* �����mac addrɨ�裬ֱ�ӷ��أ���������֡���˼Ĵ��� */
    if (OAL_TRUE != pst_scan_params->en_is_random_mac_addr_scan)
    {
        //OAM_INFO_LOG0(0, OAM_SF_SCAN, "{dmac_scan_set_vap_mac_addr_by_scan_state:: don't need modified mac addr.}");
        return;
    }

    /* p2pɨ�費֧�����mac addr */
    if (OAL_TRUE == pst_scan_params->bit_is_p2p0_scan)
    {
        //OAM_INFO_LOG0(0, OAM_SF_SCAN, "{dmac_scan_set_vap_mac_addr_by_scan_state:: p2p scan, don't need modified mac addr.}");
        return;
    }

    /* ��ȡdmac vap */
    pst_dmac_vap = mac_res_get_dmac_vap(pst_scan_params->uc_vap_id);
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{dmac_scan_set_vap_mac_addr_by_scan_state:: pst_dmac_vap is null.}");
        return;
    }

    pst_mac_vap = &(pst_dmac_vap->st_vap_base_info);

    /* �жϵ�ǰ��P2P�������������MAC ADDR������ */
    if (!IS_LEGACY_VAP(pst_mac_vap))
    {
        return;
    }

    if (OAL_PTR_NULL == pst_hal_device)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{dmac_scan_set_vap_mac_addr_by_scan_state:: pst_hal_device is null.}");
        return;
    }

    pst_hal_scan_params = &(pst_hal_device->st_hal_scan_params);

    uc_orig_hal_dev_id = dmac_vap_get_hal_device_id(pst_dmac_vap);

    dmac_vap_set_hal_device_id(pst_dmac_vap, pst_hal_device->uc_device_id);

    /* ɨ�迪ʼʱ����������֡���˼Ĵ��� */
    if (OAL_TRUE == en_is_scan_start)
    {
        /* ����ԭ�ȵ�mac addr */
        oal_set_mac_addr(pst_hal_scan_params->auc_original_mac_addr, mac_mib_get_StationID(pst_mac_vap));

        /* ����mib��Ӳ����macaddrΪ���mac addr */
        oal_set_mac_addr(mac_mib_get_StationID(pst_mac_vap),
                         pst_scan_params->auc_sour_mac_addr);
        hal_vap_set_macaddr(pst_dmac_vap->pst_hal_vap, pst_scan_params->auc_sour_mac_addr);

#if (defined(_PRE_PRODUCT_ID_HI110X_DEV))
        OAM_WARNING_LOG_ALTER(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{dmac_scan_set_vap_mac_addr_by_scan_state::original mac addr[%02X:XX:XX:%02X:%02X:%02X], set random mac addr[%02X:XX:XX:%02X:%02X:%02X]}", 8,
                                 pst_hal_scan_params->auc_original_mac_addr[0], pst_hal_scan_params->auc_original_mac_addr[3],
                                 pst_hal_scan_params->auc_original_mac_addr[4], pst_hal_scan_params->auc_original_mac_addr[5],
                                 pst_scan_params->auc_sour_mac_addr[0], pst_scan_params->auc_sour_mac_addr[3],
                                 pst_scan_params->auc_sour_mac_addr[4], pst_scan_params->auc_sour_mac_addr[5]);
#endif
    }
    else
    {
        /* ɨ��������ָ�ԭ��mib��Ӳ���Ĵ�����mac addr */
        oal_set_mac_addr(mac_mib_get_StationID(pst_mac_vap),
                         pst_hal_scan_params->auc_original_mac_addr);
        hal_vap_set_macaddr(pst_dmac_vap->pst_hal_vap, pst_hal_scan_params->auc_original_mac_addr);

        OAM_WARNING_LOG4(0, OAM_SF_SCAN, "{dmac_scan_set_vap_mac_addr_by_scan_state:: resume original mac addr, mac_addr:%02X:XX:XX:%02X:%02X:%02X.}",
                         pst_hal_scan_params->auc_original_mac_addr[0], pst_hal_scan_params->auc_original_mac_addr[3], pst_hal_scan_params->auc_original_mac_addr[4], pst_hal_scan_params->auc_original_mac_addr[5]);

    }

    dmac_vap_set_hal_device_id(pst_dmac_vap, uc_orig_hal_dev_id);

    return;
}

#ifdef _PRE_WLAN_WEB_CMD_COMM

OAL_STATIC oal_uint32  dmac_scan_get_bss_max_rate(dmac_vap_stru *pst_dmac_vap, oal_netbuf_stru *pst_netbuf, oal_uint16 us_frame_len,
                                                                oal_uint32 *pul_max_rate_kbps, oal_uint8 *puc_max_nss, hal_channel_assemble_enum_uint8* pen_bw)
{
    hal_to_dmac_device_stru    *pst_hal_device;
    oal_uint8                  *puc_frame_body;
    oal_uint8                  *puc_ie = OAL_PTR_NULL;
    oal_uint8                  *puc_legacy_rate;
    oal_uint8                   uc_legacy_rate_num;
    oal_uint16                  us_offset;
    oal_uint8                   uc_legacy_max_rate = 0;
    wlan_nss_enum_uint8         en_nss;
    oal_uint8                  *puc_ht_mcs_bitmask;
    oal_uint8                   uc_ht_max_mcs;
    oal_uint8                   uc_mcs_bitmask;
    oal_uint16                  us_ht_cap_info;
    oal_bool_enum_uint8         en_ht_short_gi = 0;
    hal_statistic_stru          st_per_rate;
    oal_uint32                  ul_ht_max_rate_kbps;
    oal_uint16                  us_msg_idx = 0;
    oal_uint16                  us_vht_cap_filed_low;
    oal_uint16                  us_vht_cap_filed_high;
    oal_uint32                  ul_vht_cap_field;
    oal_bool_enum_uint8         en_vht_short_gi = 0;
    oal_uint16                  us_vht_mcs_map;
    oal_uint8                   uc_vht_max_mcs;
    oal_uint32                  ul_vht_max_rate_kbps;
    hal_channel_assemble_enum_uint8 en_ht_bw  = WLAN_BAND_ASSEMBLE_20M;
    hal_channel_assemble_enum_uint8 en_vht_bw = WLAN_BAND_ASSEMBLE_80M;

    pst_hal_device = DMAC_VAP_GET_HAL_DEVICE(&pst_dmac_vap->st_vap_base_info);
    if (OAL_PTR_NULL == pst_hal_device)
    {
        OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{dmac_scan_get_bss_max_rate:: current vap, DMAC_VAP_GET_HAL_DEVICE null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡ֡����ʼָ�� */
    puc_frame_body = (oal_uint8 *)OAL_NETBUF_DATA(pst_netbuf) + MAC_80211_FRAME_LEN;
    /* ����Beacon֡��fieldƫ���� */
    us_offset = MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN + MAC_CAP_INFO_LEN;

    *pul_max_rate_kbps  = 0;
    *puc_max_nss        = 1;

    /* ��ȡlegacy������� */
    puc_ie = mac_find_ie(MAC_EID_RATES, puc_frame_body + us_offset, us_frame_len - us_offset);
    if (OAL_PTR_NULL != puc_ie)
    {
        puc_legacy_rate     = puc_ie + MAC_IE_HDR_LEN;
        uc_legacy_rate_num  = puc_ie[1];
        uc_legacy_max_rate  = puc_legacy_rate[uc_legacy_rate_num - 1] & 0x7f;
    }

    /* ��ȡ��չlegacy������� */
    puc_ie = mac_find_ie(MAC_EID_XRATES, puc_frame_body + us_offset, us_frame_len - us_offset);
    if (OAL_PTR_NULL != puc_ie)
    {
        puc_legacy_rate     = puc_ie + MAC_IE_HDR_LEN;
        uc_legacy_rate_num  = puc_ie[1];
        uc_legacy_max_rate  = OAL_MAX(uc_legacy_max_rate, puc_legacy_rate[uc_legacy_rate_num - 1] & 0x7f);
    }

    /* ����BSS������� */
    *pul_max_rate_kbps  = ((oal_uint32)uc_legacy_max_rate) * 500;

    /* ����HT������ʣ�������BSS������� */
    puc_ie = mac_find_ie(MAC_EID_HT_CAP, puc_frame_body + us_offset, us_frame_len - us_offset);
    if (OAL_PTR_NULL != puc_ie)
    {
        /* ��ȡHT BW��short GI��Ϣ */
        us_msg_idx      = MAC_IE_HDR_LEN;
        us_ht_cap_info  = OAL_MAKE_WORD16(puc_ie[us_msg_idx], puc_ie[us_msg_idx + 1]);
        /* ��ȡHT NSS��MCS��Ϣ */
        us_msg_idx        += MAC_HT_CAPINFO_LEN + MAC_HT_AMPDU_PARAMS_LEN;
        puc_ht_mcs_bitmask = &puc_ie[us_msg_idx];
        for (en_nss = WLAN_SINGLE_NSS; en_nss <= WLAN_FOUR_NSS; en_nss++)
        {
            if (0 == puc_ht_mcs_bitmask[en_nss])
            {
                break;
            }
        }
        if (WLAN_SINGLE_NSS != en_nss)
        {
            en_nss--;

            uc_ht_max_mcs = 0;
            uc_mcs_bitmask = puc_ht_mcs_bitmask[en_nss] >> 1;
            while (uc_mcs_bitmask > 0)
            {
                uc_ht_max_mcs++;
                uc_mcs_bitmask >>= 1;
            }

            puc_ie = mac_find_ie(MAC_EID_HT_OPERATION, puc_frame_body + us_offset, us_frame_len - us_offset);
            if (OAL_PTR_NULL != puc_ie)
            {
                us_msg_idx      = MAC_IE_HDR_LEN;
                en_ht_bw = ((puc_ie[us_msg_idx + 1] & BIT2) ? WLAN_BAND_ASSEMBLE_40M : WLAN_BAND_ASSEMBLE_20M);
                en_ht_short_gi  = (WLAN_BAND_ASSEMBLE_20M == en_ht_bw) ?
                                  ((us_ht_cap_info & BIT5) >> 5) : ((us_ht_cap_info & BIT6) >> 6);
    //            OAM_WARNING_LOG4(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{dmac_scan_get_bss_max_rate::us_ht_cap_info: 0x%04x, shortgi: %d, nss: %d, puc_ht_mcs_bitmask: 0x%x}",
    //                us_ht_cap_info, en_ht_short_gi, en_nss, puc_ht_mcs_bitmask[en_nss]);
    //
    //            OAM_WARNING_LOG4(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{dmac_scan_get_bss_max_rate::channel: %d, ht_info: 0x%01x, en_ht_bw: %d, uc_ht_max_mcs: %d}",
    //                        puc_ie[us_msg_idx], puc_ie[us_msg_idx + 1], en_ht_bw, uc_ht_max_mcs);
            }

            /* ��ѯHT������� */
            st_per_rate.un_nss_rate.st_ht_rate.bit_protocol_mode    = WLAN_HT_PHY_PROTOCOL_MODE;
            st_per_rate.un_nss_rate.st_ht_rate.bit_ht_mcs           = uc_ht_max_mcs;
            st_per_rate.uc_bandwidth    = en_ht_bw;
            st_per_rate.uc_short_gi     = en_ht_short_gi;
            dmac_alg_get_rate_kbps(pst_hal_device, &st_per_rate, &ul_ht_max_rate_kbps);

            *puc_max_nss        = OAL_MAX(*puc_max_nss, en_nss + 1);
            *pul_max_rate_kbps  = OAL_MAX(*pul_max_rate_kbps, ul_ht_max_rate_kbps * (en_nss + 1));
        }
    }
    *pen_bw = en_ht_bw;

    /* ����VHT������ʣ�������BSS������� */
    puc_ie = mac_find_ie(MAC_EID_VHT_CAP, puc_frame_body + us_offset, us_frame_len - us_offset);
    if (OAL_PTR_NULL != puc_ie)
    {
        /* ����VHT capablities info field */
        us_msg_idx              = MAC_IE_HDR_LEN;
        us_vht_cap_filed_low    = OAL_MAKE_WORD16(puc_ie[us_msg_idx], puc_ie[us_msg_idx + 1]);
        us_vht_cap_filed_high   = OAL_MAKE_WORD16(puc_ie[us_msg_idx + 2], puc_ie[us_msg_idx + 3]);
        ul_vht_cap_field        = OAL_MAKE_WORD32(us_vht_cap_filed_low, us_vht_cap_filed_high);

        /* ��ȡVHT NSS��MCS��Ϣ */
        us_msg_idx    += MAC_VHT_CAP_INFO_FIELD_LEN;
        us_vht_mcs_map = OAL_MAKE_WORD16(puc_ie[us_msg_idx], puc_ie[us_msg_idx + 1]);
        uc_vht_max_mcs = 0;

        for (en_nss = WLAN_SINGLE_NSS; en_nss <= WLAN_FOUR_NSS; en_nss++)
        {
            if (WLAN_INVALD_VHT_MCS == WLAN_GET_VHT_MAX_SUPPORT_MCS(us_vht_mcs_map & 0x3))
            {
                break;
            }
            uc_vht_max_mcs = WLAN_GET_VHT_MAX_SUPPORT_MCS(us_vht_mcs_map & 0x3);
            us_vht_mcs_map >>= 2;
        }
        /* ��ȡVHT BW��short GI��Ϣ */
        puc_ie = mac_find_ie(MAC_EID_VHT_OPERN, puc_frame_body + us_offset, us_frame_len - us_offset);
        if (OAL_PTR_NULL != puc_ie)
        {
            us_msg_idx      = MAC_IE_HDR_LEN;
            en_vht_bw = (puc_ie[us_msg_idx] ? WLAN_BAND_ASSEMBLE_80M : en_ht_bw);
            if (0 != (ul_vht_cap_field & (BIT3 |BIT2)))
            {
                en_vht_bw       = WLAN_BAND_ASSEMBLE_160M;
                en_vht_short_gi =((ul_vht_cap_field & BIT6) >> 6);
            }
            else if(WLAN_BAND_ASSEMBLE_80M == en_vht_bw)
            {
                en_vht_short_gi = ((ul_vht_cap_field & BIT5) >> 5);
            }
            else
            {
                en_vht_short_gi = en_ht_short_gi;
            }
            //OAM_WARNING_LOG4(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{dmac_scan_get_bss_max_rate::us_vht_cap_info: 0x%x, vht shortgi: %d, vht nss: %d, puc_vht_mcs_bitmask: 0x%x}",
            //    ul_vht_cap_field, en_vht_short_gi, en_nss, us_vht_mcs_map);
            //OAM_WARNING_LOG3(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{dmac_scan_get_bss_max_rate::vht_info: 0x%01x, en_vht_bw: %d, uc_vht_max_mcs: %d}",
            //            *(oal_uint32*)puc_ie, en_vht_bw, uc_vht_max_mcs);
        }

        /* ��ѯVHT������� */
        st_per_rate.un_nss_rate.st_vht_nss_mcs.bit_protocol_mode    = WLAN_VHT_PHY_PROTOCOL_MODE;
        st_per_rate.un_nss_rate.st_vht_nss_mcs.bit_nss_mode         = WLAN_SINGLE_NSS;
        st_per_rate.un_nss_rate.st_vht_nss_mcs.bit_vht_mcs          = uc_vht_max_mcs;
        st_per_rate.uc_short_gi     = en_vht_short_gi;
        st_per_rate.uc_bandwidth    = en_vht_bw;
        dmac_alg_get_rate_kbps(pst_hal_device, &st_per_rate, &ul_vht_max_rate_kbps);

        *puc_max_nss        = OAL_MAX(*puc_max_nss, en_nss);
        *pul_max_rate_kbps  = OAL_MAX(*pul_max_rate_kbps, ul_vht_max_rate_kbps * en_nss);
        *pen_bw             = en_vht_bw;
    }

    return OAL_SUCC;
}
#endif


OAL_STATIC oal_uint32  dmac_scan_report_scanned_bss(dmac_vap_stru *pst_dmac_vap, oal_void *p_param)
{
    frw_event_mem_stru                    *pst_event_mem;
    frw_event_stru                        *pst_event;
    mac_device_stru                       *pst_mac_device;
    dmac_tx_event_stru                    *pst_dtx_event;
    oal_netbuf_stru                       *pst_netbuf;
    dmac_rx_ctl_stru                      *pst_rx_ctrl;
#ifdef _PRE_WLAN_FEATURE_M2S
    mac_ieee80211_frame_stru              *pst_frame_hdr;
    oal_uint8                             *puc_frame_body;
#endif
    oal_uint8                             *puc_frame_body_tail;            /* ָ��֡���β�� */
    mac_scan_req_stru                     *pst_scan_params;
    mac_scanned_result_extend_info_stru   *pst_scanned_result_extend_info;
    oal_uint16                             us_frame_len;
    oal_uint16                             us_remain_netbuf_len;
#ifdef _PRE_WLAN_WEB_CMD_COMM
    oal_uint32                             ul_ret;
#endif

    pst_mac_device = mac_res_get_dev(pst_dmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
                         "{dmac_scan_report_scanned_bss::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_netbuf = (oal_netbuf_stru *)p_param;

    /* ��ȡ֡��Ϣ */
    us_frame_len = (oal_uint16)oal_netbuf_get_len(pst_netbuf);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    us_remain_netbuf_len = oal_netbuf_get_payload_len(pst_netbuf) - (us_frame_len - MAC_80211_FRAME_LEN);
#else
    us_remain_netbuf_len = (oal_uint16)oal_netbuf_tailroom(pst_netbuf);
#endif

    if(us_remain_netbuf_len < OAL_SIZEOF(mac_scanned_result_extend_info_stru))
    {
        OAM_ERROR_LOG2(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
                     "{dmac_scan_report_scanned_bss::scan netbuf tailroom not enough,requet[%u],actual[%u] }",
                     us_remain_netbuf_len,
                     OAL_SIZEOF(mac_scanned_result_extend_info_stru));
        return OAL_FAIL;
    }

    /* ��ȡ��buffer�Ŀ�����Ϣ */
    pst_rx_ctrl = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);

    /* ��ȡɨ����� */
    pst_scan_params = &(pst_mac_device->st_scan_params);

    /* ÿ��ɨ��������ϱ�ɨ���������¼���HMAC, �����¼��ڴ� */
    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(dmac_tx_event_stru));
    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
                       "{dmac_scan_report_scanned_bss::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��д�¼� */
    pst_event = frw_get_event_stru(pst_event_mem);

    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_WLAN_CRX,
                       DMAC_WLAN_CRX_EVENT_SUB_TYPE_EVERY_SCAN_RESULT,
                       OAL_SIZEOF(dmac_tx_event_stru),
                       FRW_EVENT_PIPELINE_STAGE_1,
                       pst_dmac_vap->st_vap_base_info.uc_chip_id,
                       pst_dmac_vap->st_vap_base_info.uc_device_id,
                       pst_dmac_vap->st_vap_base_info.uc_vap_id);

    /***********************************************************************************************/
    /*            netbuf data����ϱ���ɨ�������ֶεķֲ�                                        */
    /* ------------------------------------------------------------------------------------------  */
    /* beacon/probe rsp body  |     ֡����渽���ֶ�(mac_scanned_result_extend_info_stru)          */
    /* -----------------------------------------------------------------------------------------   */
    /* �յ���beacon/rsp��body | rssi(4�ֽ�) | channel num(1�ֽ�)| band(1�ֽ�)|bss_tye(1�ֽ�)|���  */
    /* ------------------------------------------------------------------------------------------  */
    /*                                                                                             */
    /***********************************************************************************************/

    /* ����չ��Ϣ������֡��ĺ��棬�ϱ���host�� */
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    puc_frame_body_tail = (oal_uint8 *)MAC_GET_RX_PAYLOAD_ADDR(&(pst_rx_ctrl->st_rx_info), pst_netbuf) + us_frame_len - MAC_80211_FRAME_LEN;
#else
    puc_frame_body_tail = (oal_uint8 *)mac_get_rx_cb_mac_hdr(&(pst_rx_ctrl->st_rx_info)) + us_frame_len;
#endif

    /* ָ��֡���β������netbuf������չ��Я��������Ҫ�ϱ�����Ϣ */
    pst_scanned_result_extend_info = (mac_scanned_result_extend_info_stru *)puc_frame_body_tail;

    /* ����ϱ�ɨ��������չ�ֶ���Ϣ�����и�ֵ */
    OAL_MEMZERO(pst_scanned_result_extend_info, OAL_SIZEOF(mac_scanned_result_extend_info_stru));
    pst_scanned_result_extend_info->l_rssi = (oal_int32)pst_rx_ctrl->st_rx_statistic.c_rssi_dbm;
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)
    pst_scanned_result_extend_info->c_ant0_rssi = pst_rx_ctrl->st_rx_statistic.c_ant0_rssi;
    pst_scanned_result_extend_info->c_ant1_rssi = pst_rx_ctrl->st_rx_statistic.c_ant1_rssi;
#endif
    pst_scanned_result_extend_info->en_bss_type = pst_scan_params->en_bss_type;
#ifdef _PRE_WLAN_FEATURE_11K_EXTERN
    /*��ȡTSF TIMERʱ���*/
    hal_vap_tsf_get_32bit(pst_dmac_vap->pst_hal_vap, (oal_uint32 *)&(pst_scanned_result_extend_info->ul_parent_tsf));
    pst_scanned_result_extend_info->c_snr_ant0 = (oal_int32)pst_rx_ctrl->st_rx_statistic.c_snr_ant0;
    pst_scanned_result_extend_info->c_snr_ant1 = (oal_int32)pst_rx_ctrl->st_rx_statistic.c_snr_ant1;
#endif

#ifdef _PRE_WLAN_FEATURE_M2S
    /* ��ȡ֡��Ϣ */
    pst_frame_hdr  = (mac_ieee80211_frame_stru *)mac_get_rx_cb_mac_hdr(&(pst_rx_ctrl->st_rx_info));
    puc_frame_body = MAC_GET_RX_PAYLOAD_ADDR(&(pst_rx_ctrl->st_rx_info), pst_netbuf);

    pst_scanned_result_extend_info->en_support_max_nss = dmac_m2s_get_bss_max_nss(&pst_dmac_vap->st_vap_base_info, pst_netbuf, us_frame_len, OAL_FALSE);
    pst_scanned_result_extend_info->uc_num_sounding_dim = dmac_m2s_scan_get_num_sounding_dim(pst_netbuf, us_frame_len);

    /* ֻ��probe rsp֡�����Ʋ�֧��OPMODE���Զ˲Ų�֧��OPMODE��beacon��assoc rsp֡�����Ƶ�OPMODE��Ϣ������,��˵����ǽ���probe rspʱ���Ƿ���probe req֡�����Ҫ��ext cap�ֶ� */
    if(WLAN_PROBE_RSP == pst_frame_hdr->st_frame_control.bit_sub_type)
    {
        pst_scanned_result_extend_info->en_support_opmode = dmac_m2s_get_bss_support_opmode(&pst_dmac_vap->st_vap_base_info, puc_frame_body, us_frame_len);
    }
    else
    {
        pst_scanned_result_extend_info->en_support_opmode = OAL_FALSE;
    }
#endif

#ifdef _PRE_WLAN_WEB_CMD_COMM
    ul_ret = dmac_scan_get_bss_max_rate(pst_dmac_vap, pst_netbuf, us_frame_len,
                &pst_scanned_result_extend_info->ul_max_rate_kbps,
                &pst_scanned_result_extend_info->uc_max_nss,
                &pst_scanned_result_extend_info->en_bw);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
               "{dmac_scan_report_scanned_bss:: dmac_scan_get_bss_max_rate fail, ul_ret = %u}", ul_ret);
        return ul_ret;
    }
#endif

    /* ����֡�ĳ���Ϊ������չ�ֶεĳ��� */
    us_frame_len += OAL_SIZEOF(mac_scanned_result_extend_info_stru);
    oal_netbuf_put(pst_netbuf, OAL_SIZEOF(mac_scanned_result_extend_info_stru));

    /* ҵ���¼���Ϣ */
    pst_dtx_event               = (dmac_tx_event_stru *)pst_event->auc_event_data;
    pst_dtx_event->pst_netbuf   = pst_netbuf;
    pst_dtx_event->us_frame_len = us_frame_len;

#if 0
    /* ����ά����Ϣ����ǰ�ŵ��š��ź�ǿ�ȡ��ϱ���netbuf���� */
    OAM_ERROR_LOG4(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
                   "{dmac_scan_report_scanned_bss::rssi[%d], cb_rssi[%d], channel_num[%d], buf_len[%d].}",
                   pst_scanned_result_extend_info->l_rssi,
                   pst_rx_ctrl->st_rx_statistic.c_rssi_dbm,
                   pst_rx_ctrl->st_rx_info.uc_channel_number,
                   us_frame_len);
#endif

    /* �ַ��¼� */
    frw_event_dispatch_event(pst_event_mem);
    FRW_EVENT_FREE(pst_event_mem);

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  dmac_scan_check_bss_in_pno_scan(oal_uint8         *puc_frame_body,
                                                                  oal_int32          l_frame_body_len,
                                                                  mac_pno_scan_stru *pst_pno_scan_info,
                                                                  oal_int32          l_rssi)
{
    oal_uint8       *puc_ssid;
    oal_int32        l_loop;
    oal_int8         ac_ssid[WLAN_SSID_MAX_LEN];
    oal_uint8        uc_ssid_len = 0;

    /* �����֡���ź�С��pnoɨ����ϱ�����ֵ���򷵻�ʧ�ܣ���֡���ϱ� */
    if (l_rssi < pst_pno_scan_info->l_rssi_thold)
    {
        return OAL_FAIL;
    }

    OAL_MEMZERO(ac_ssid, OAL_SIZEOF(ac_ssid));

    /* ��ȡ����֡�е�ssid IE��Ϣ */
    puc_ssid = mac_get_ssid(puc_frame_body, l_frame_body_len, &uc_ssid_len);
    if ((OAL_PTR_NULL != puc_ssid) && (0 != uc_ssid_len))
    {
        oal_memcopy(ac_ssid, puc_ssid, uc_ssid_len);
        ac_ssid[uc_ssid_len] = '\0';
    }

    /* ��pno�����в��ұ�ssid�Ƿ���ڣ�������ڣ�������ϱ� */
    for (l_loop = 0; l_loop < pst_pno_scan_info->l_ssid_count; l_loop++)
    {
        /* ���ssid��ͬ�����سɹ� */
        if (0 == oal_memcmp(ac_ssid, pst_pno_scan_info->ast_match_ssid_set[l_loop].auc_ssid, (uc_ssid_len + 1)))
        {
            OAM_WARNING_LOG0(0, OAM_SF_SCAN, "{dmac_scan_check_bss_in_pno_scan::ssid match success.}");
            return OAL_SUCC;
        }
    }

    return OAL_FAIL;
}


OAL_STATIC oal_uint32  dmac_scan_check_bss_type(oal_uint8 *puc_frame_body, mac_scan_req_stru *pst_scan_params)
{
    mac_cap_info_stru         *pst_cap_info;

    /*************************************************************************/
    /*                       Beacon Frame - Frame Body                       */
    /* ----------------------------------------------------------------------*/
    /* |Timestamp|BcnInt|CapInfo|SSID|SupRates|DSParamSet|TIM  |CountryElem |*/
    /* ----------------------------------------------------------------------*/
    /* |8        |2     |2      |2-34|3-10    |3         |6-256|8-256       |*/
    /* ----------------------------------------------------------------------*/
    pst_cap_info = (mac_cap_info_stru *)(puc_frame_body + MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN);

    if ((WLAN_MIB_DESIRED_BSSTYPE_INFRA == pst_scan_params->en_bss_type) &&
        (1 != pst_cap_info->bit_ess))
    {
        //OAM_INFO_LOG0(0, OAM_SF_SCAN, "{dmac_scan_check_bss_type::expect infra bss, but it's not infra bss.}\r\n");
        return OAL_FAIL;
    }

    if ((WLAN_MIB_DESIRED_BSSTYPE_INDEPENDENT == pst_scan_params->en_bss_type) &&
        (1 != pst_cap_info->bit_ibss))
    {
        //OAM_INFO_LOG0(0, OAM_SF_SCAN, "{dmac_scan_check_bss_type::expect ibss, but it's not ibss.}\r\n");
        return OAL_FAIL;
    }

    return OAL_SUCC;
}


oal_void  dmac_scan_check_assoc_ap_channel(dmac_vap_stru *pst_dmac_vap, oal_netbuf_stru *pst_netbuf,scan_check_assoc_channel_enum_uint8 en_check_mode)
{
    dmac_rx_ctl_stru                        *pst_rx_ctrl;
    mac_ieee80211_frame_stru                *pst_frame_hdr;
    oal_uint8                               *puc_frame_body;
    oal_uint16                               us_frame_len;
    oal_uint16                               us_offset =  MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN + MAC_CAP_INFO_LEN;
    mac_cfg_ssid_param_stru                  st_mib_ssid = {0};
    oal_uint8                                uc_mib_ssid_len = 0;
    oal_uint8                                uc_frame_channel;
    oal_uint8                                uc_ssid_len = 0;
    oal_uint8                               *puc_ssid;
    oal_uint32                               ul_ret;
    oal_uint8                                uc_idx;
    oal_ieee80211_channel_sw_ie              st_csa_info;
    mac_vap_stru                            *pst_mac_vap;

    pst_rx_ctrl = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);

    /* ��ȡ֡��Ϣ */
    pst_frame_hdr  = (mac_ieee80211_frame_stru *)mac_get_rx_cb_mac_hdr(&(pst_rx_ctrl->st_rx_info));
    puc_frame_body = MAC_GET_RX_PAYLOAD_ADDR(&(pst_rx_ctrl->st_rx_info), pst_netbuf);
    us_frame_len   = (oal_uint16)oal_netbuf_get_len(pst_netbuf);
    pst_mac_vap    = &(pst_dmac_vap->st_vap_base_info);

    /*�Ƿ��������Ĺ���֡*/
    if(OAL_MEMCMP(pst_frame_hdr->auc_address3, pst_dmac_vap->st_vap_base_info.auc_bssid, WLAN_MAC_ADDR_LEN))
    {
        return;
    }

    /*ssid��Ϣ��һ��*/
    puc_ssid = mac_get_ssid(puc_frame_body, (oal_int32)(us_frame_len - MAC_80211_FRAME_LEN), &uc_ssid_len);
    mac_mib_get_ssid(&pst_dmac_vap->st_vap_base_info, &uc_mib_ssid_len, (oal_uint8 *)&st_mib_ssid);
    if((OAL_PTR_NULL == puc_ssid) || (0 == uc_ssid_len) || (st_mib_ssid.uc_ssid_len != uc_ssid_len) ||
        OAL_MEMCMP(st_mib_ssid.ac_ssid, puc_ssid, uc_ssid_len))
    {
        return;
    }

    /*����֡�л�ȡ�ŵ���Ϣ*/
    uc_frame_channel = mac_ie_get_chan_num(puc_frame_body, (us_frame_len - MAC_80211_FRAME_LEN),
                                       us_offset, pst_rx_ctrl->st_rx_info.uc_channel_number);
    if(0 == uc_frame_channel)
    {
        return;
    }

    ul_ret = mac_get_channel_idx_from_num(pst_dmac_vap->st_vap_base_info.st_channel.en_band, uc_frame_channel, &uc_idx);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{dmac_scan_check_assoc_ap_channel::Get channel idx failed, To DISASSOC! vap_channel[%d], frame_channel=[%d].}",
        pst_mac_vap->st_channel.uc_chan_number, uc_frame_channel);
        dmac_vap_linkloss_clean(pst_dmac_vap);
        dmac_sta_csa_fsm_post_event(pst_mac_vap, WLAN_STA_CSA_EVENT_TO_INIT, 0, OAL_PTR_NULL);
        dmac_send_disasoc_misc_event(pst_mac_vap, pst_mac_vap->us_assoc_vap_id, DMAC_DISASOC_MISC_GET_CHANNEL_IDX_FAIL);
        return;
    }

    switch (en_check_mode)
    {
        /*LinkLoss ���*/
        case SCAN_CHECK_ASSOC_CHANNEL_LINKLOSS:
            if(pst_dmac_vap->st_vap_base_info.st_channel.uc_chan_number != uc_frame_channel)
            {/*�ŵ������仯*/
                /*CSA �ŵ��л�������ֱ���˳�*/
                if(OAL_TRUE == dmac_sta_csa_is_in_waiting(pst_mac_vap))
                {
                    dmac_vap_linkloss_clean(pst_dmac_vap);
                    OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{dmac_scan_check_assoc_ap_channel::vap_channel=[%d],fram_channel=[%d];sta in csa process,return.}",
                    pst_mac_vap->st_channel.uc_chan_number,uc_frame_channel);
                    return;
                }

                /*����CSA����  */
                st_csa_info.new_ch_num                          = uc_frame_channel;
                st_csa_info.mode                                = WLAN_CSA_MODE_TX_DISABLE;
                st_csa_info.count                               = 0;
                /*������������20M, Beacon֡������������*/
                pst_mac_vap->st_ch_switch_info.en_new_bandwidth = WLAN_BAND_WIDTH_20M;
                OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{dmac_scan_check_assoc_ap_channel::Trigger csa, change channle from [%d] to [%d], bw=[%d]}",
                    pst_mac_vap->st_channel.uc_chan_number, uc_frame_channel, pst_mac_vap->st_ch_switch_info.en_new_bandwidth);
                dmac_sta_csa_fsm_post_event(pst_mac_vap, WLAN_STA_CSA_EVENT_GET_IE, sizeof(st_csa_info), (oal_uint8*)&st_csa_info);
                /* ���������ŵ���vap�µĴ����е�normal״̬ */
                if ((IS_LEGACY_STA(pst_mac_vap)) && (OAL_TRUE == dmac_sta_csa_is_in_waiting(pst_mac_vap))
                      && (DMAC_STA_BW_SWITCH_FSM_NORMAL != MAC_VAP_GET_CURREN_BW_STATE(pst_mac_vap)))
                {
                    dmac_sta_bw_switch_fsm_post_event((dmac_vap_stru *)pst_mac_vap, DMAC_STA_BW_SWITCH_EVENT_CHAN_SYNC, 0, OAL_PTR_NULL);
                    /* �����л�״̬�� */
                    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{dmac_ie_proc_ch_switch_ie::VAP CURREN BW STATE[%d].}",
                                     MAC_VAP_GET_CURREN_BW_STATE(pst_mac_vap));
                }
            }
            break;
        case SCAN_CHECK_ASSOC_CHANNEL_CSA:
            OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CSA, "{dmac_scan_check_assoc_ap_channel::csa scan recv_probe_rsp_channel = [%d].}", uc_frame_channel);
            dmac_sta_csa_fsm_post_event(&(pst_dmac_vap->st_vap_base_info), WLAN_STA_CSA_EVENT_RCV_PROBE_RSP, sizeof(uc_frame_channel), &uc_frame_channel);
            break;
        default:
            break;
    }

    return;
}



hal_to_dmac_device_stru *dmac_scan_find_hal_device(mac_device_stru  *pst_mac_device, dmac_vap_stru *pst_dmac_vap, oal_uint8 uc_idx)
{
    hal_to_dmac_device_stru     *pst_hal_device = OAL_PTR_NULL;
    dmac_device_stru            *pst_dmac_device;

    /* ���ȷ���dmac_vap���ҽӵ�hal device */
    if (0 == uc_idx)
    {
        return pst_dmac_vap->pst_hal_device;
    }

    pst_dmac_device = dmac_res_get_mac_dev(pst_mac_device->uc_device_id);
    if (OAL_PTR_NULL == pst_dmac_device)
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "dmac_scan_find_hal_device::pst_dmac_device is null");
        return pst_hal_device;

    }
    /* �Ƿ�֧�ֲ��� */
    if (OAL_TRUE == pst_dmac_device->en_is_fast_scan)
    {
        pst_hal_device = dmac_device_get_another_h2d_dev(pst_dmac_device, pst_dmac_vap->pst_hal_device);

        /* ֧�ֲ����ض�������·hal device */
        if (OAL_PTR_NULL  == pst_hal_device)
        {
            OAM_ERROR_LOG2(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_DBAC, "dmac_scan_find_hal_device::pst_hal_device NULL,chip id[%d],ori hal device id[%d]",
                            pst_dmac_vap->st_vap_base_info.uc_chip_id, pst_dmac_vap->pst_hal_device->uc_device_id);
        }

    }

    return pst_hal_device;
}

oal_void dmac_scan_check_rev_frame(mac_device_stru *pst_mac_device, dmac_vap_stru *pst_dmac_vap, oal_uint8 uc_frame_channel_num)
{
    hal_to_dmac_device_stru         *pst_hal_device;
    oal_uint8                        uc_idx;
    oal_uint8                        uc_device_max;

    /* HAL�ӿڻ�ȡ֧��device���� */
    hal_chip_get_device_num(pst_mac_device->uc_chip_id, &uc_device_max);

    for (uc_idx = 0; uc_idx < uc_device_max; uc_idx++)
    {
        /* ���ȼ��dmac_vap ���ҽӵ�hal device */
        pst_hal_device = dmac_scan_find_hal_device(pst_mac_device, pst_dmac_vap, uc_idx);
        if (OAL_PTR_NULL == pst_hal_device)
        {
            continue;
        }

        if ((pst_hal_device->uc_current_chan_number == uc_frame_channel_num)
            && (OAL_FALSE == pst_hal_device->st_hal_scan_params.en_working_in_home_chan))
        {
            pst_hal_device->st_hal_scan_params.en_scan_curr_chan_find_bss_flag = OAL_TRUE;
            break;
        }
    }
}

oal_uint32  dmac_scan_mgmt_filter(dmac_vap_stru *pst_dmac_vap, oal_void *p_param, oal_bool_enum_uint8 *pen_report_bss, oal_uint8 *pen_go_on)
{
    /* !!! ע��:dmac_rx_filter_mgmt �����pen_report_bss �����Ƿ���Ҫ�ͷ�netbuf,����pen_go_on��־�Ƿ���Ҫ�����ϱ� */
    /* ���pen_report_bss����ΪOAL_TRUE����ɨ��ӿ��Ѿ��ϱ�,dmac_rx_filter_mgmt�򲻻��ͷ�netbuf,�������pen_go_on��־�����ϱ����ͷ�netbuf */
    /* 1151�ϣ�����dmac_scan_report_scanned_bss֮�󣬲�����������skb����Ϊ�Ѿ���hmac�ͷ��� */

    oal_netbuf_stru            *pst_netbuf;
    dmac_rx_ctl_stru           *pst_rx_ctrl;
    mac_ieee80211_frame_stru   *pst_frame_hdr;
    oal_uint8                  *puc_frame_body;
    mac_device_stru            *pst_mac_device;
    mac_scan_req_stru          *pst_scan_params;
    oal_uint32                  ul_ret;
    oal_uint16                  us_frame_len;
    oal_uint8                   uc_frame_channel_num;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
    oal_uint16                  us_offset =  MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN + MAC_CAP_INFO_LEN;
#endif
#endif
    *pen_report_bss = OAL_FALSE;

    pst_mac_device  = mac_res_get_dev(pst_dmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{dmac_scan_mgmt_filter::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_scan_params = &(pst_mac_device->st_scan_params);

    pst_netbuf = (oal_netbuf_stru *)p_param;

    /* ��ȡ��buffer�Ŀ�����Ϣ */
    pst_rx_ctrl = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);

    /* ��ȡ֡��Ϣ */
    pst_frame_hdr  = (mac_ieee80211_frame_stru *)mac_get_rx_cb_mac_hdr(&(pst_rx_ctrl->st_rx_info));
    puc_frame_body = MAC_GET_RX_PAYLOAD_ADDR(&(pst_rx_ctrl->st_rx_info), pst_netbuf);

    us_frame_len   = (oal_uint16)oal_netbuf_get_len(pst_netbuf);

    if ((WLAN_BEACON == pst_frame_hdr->st_frame_control.bit_sub_type) ||
        (WLAN_PROBE_RSP == pst_frame_hdr->st_frame_control.bit_sub_type))
    {
        /* ���ɨ�赽bss������ */
        if (OAL_SUCC != dmac_scan_check_bss_type(puc_frame_body, pst_scan_params))
        {
            //OAM_INFO_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{dmac_scan_mgmt_filter::scanned bss isn't the desired one.}\r\n");
            return OAL_SUCC;
        }

        /* �����obssɨ�裬���ϱ�ɨ������ֻ�ڴ��ŵ���⵽��beacon֡����probe rsp֡�����¼���host����20/40�����߼����� */
        if (WLAN_SCAN_MODE_BACKGROUND_OBSS == pst_scan_params->en_scan_mode)
        {
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
            dmac_detect_2040_te_a_b(pst_dmac_vap, puc_frame_body, us_frame_len, us_offset, pst_rx_ctrl->st_rx_info.uc_channel_number);
#endif
#endif
            /* OBSSɨ�費��Ҫ�����ϱ��������ݻ�ɨ������host */
            *pen_go_on = OAL_FALSE;
            return OAL_SUCC;
        }
        else
        {
            /* ����ɨ��״̬���ҽ��յ���bss �ŵ���Ϣ�뵱ǰɨ���ŵ���ͬ���ű�ʶ��ǰ�ŵ�ɨ�赽BSS */
            uc_frame_channel_num = mac_ie_get_chan_num(puc_frame_body, (us_frame_len - MAC_80211_FRAME_LEN),
                                                                MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN + MAC_CAP_INFO_LEN,
                                                                pst_rx_ctrl->st_rx_info.uc_channel_number);

            dmac_scan_check_rev_frame(pst_mac_device, pst_dmac_vap, uc_frame_channel_num);
            if ((WLAN_VAP_MODE_BSS_STA == pst_dmac_vap->st_vap_base_info.en_vap_mode)
                 && (MAC_VAP_STATE_PAUSE == pst_dmac_vap->st_vap_base_info.en_vap_state))
            {
                /* STAɨ����linkloss��������1/4ʱ������AP�ŵ��Ƿ��л���ʶ�� */
                if (GET_CURRENT_LINKLOSS_CNT(pst_dmac_vap) >= GET_CURRENT_LINKLOSS_THRESHOLD(pst_dmac_vap) >> 2)
                {
                    dmac_scan_check_assoc_ap_channel(pst_dmac_vap, pst_netbuf, SCAN_CHECK_ASSOC_CHANNEL_LINKLOSS);
                }

                if (WLAN_SCAN_MODE_BACKGROUND_CSA == pst_scan_params->en_scan_mode)
                {
                    dmac_scan_check_assoc_ap_channel(pst_dmac_vap, pst_netbuf, SCAN_CHECK_ASSOC_CHANNEL_CSA);
                    /* linkloss CSAɨ�費��Ҫ�ϱ����Ļ�ɨ������host */
                    *pen_go_on = OAL_FALSE;
                    return OAL_SUCC;
                }
            }

        #ifdef _PRE_WLAN_FEATURE_GNSS_SCAN
            dmac_scan_proc_scanned_bss(pst_mac_device, pst_netbuf);
        #endif

            /* �����pno����ɨ�裬����Ҫ����rssi��ssid�Ĺ��� */
            if (WLAN_SCAN_MODE_BACKGROUND_PNO == pst_scan_params->en_scan_mode)
            {
                /* ��Ȿbss�Ƿ�����ϱ�����pnoɨ��ĳ����� */
                ul_ret = dmac_scan_check_bss_in_pno_scan(puc_frame_body,
                                                         (oal_int32)(us_frame_len - MAC_80211_FRAME_LEN),
                                                         &(pst_mac_device->pst_pno_sched_scan_mgmt->st_pno_sched_scan_params),
                                                         (oal_int32)pst_rx_ctrl->st_rx_statistic.c_rssi_dbm);
                if (OAL_SUCC != ul_ret)
                {
                    //OAM_INFO_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{dmac_scan_mgmt_filter::this bss info can't report host.}");

                    /* PNOɨ��,û��ɨ�赽��Ҫ������AP,����Ҫ�ϱ����Ļ�ɨ������host */
                    *pen_go_on = OAL_FALSE;

                    return OAL_SUCC;
                }

                /* �����ɨ�赽�˵�һ��ƥ���ssid����ɨ�赽��ƥ���ssid���λΪ�� */
                if (OAL_TRUE != pst_mac_device->pst_pno_sched_scan_mgmt->en_is_found_match_ssid)
                {
                    pst_mac_device->pst_pno_sched_scan_mgmt->en_is_found_match_ssid = OAL_TRUE;

                    /* ֹͣpno����ɨ�趨ʱ�� */
                    dmac_scan_stop_pno_sched_scan_timer(pst_mac_device->pst_pno_sched_scan_mgmt);
                }
            }
#ifdef _PRE_WLAN_FEATURE_11K
            else if (WLAN_SCAN_MODE_RRM_BEACON_REQ == pst_scan_params->en_scan_mode)
            {
                /* WLAN_SCAN_MODE_RRM_BEACON_REQɨ��,����Ҫ�ϱ����Ļ�ɨ������host */
                *pen_go_on = OAL_FALSE;

                dmac_rrm_get_bcn_info_from_rx(pst_dmac_vap, pst_netbuf);
                return OAL_SUCC;
            }
#endif

            /* ����ģʽɨ�裬�ϱ�ɨ�赽��ɨ���� */
            ul_ret = dmac_scan_report_scanned_bss(pst_dmac_vap, p_param);
            if (OAL_SUCC != ul_ret)
            {
                *pen_report_bss = OAL_FALSE;
                OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{dmac_scan_mgmt_filter::report scan result failed.}");
                return OAL_SUCC;
            }
            else
            {
                *pen_report_bss = OAL_TRUE;
            }
        }
    }
    else if(WLAN_ACTION == pst_frame_hdr->st_frame_control.bit_sub_type)
    {
        switch (puc_frame_body[MAC_ACTION_OFFSET_CATEGORY])
        {
            case MAC_ACTION_CATEGORY_PUBLIC:
            {
                switch (puc_frame_body[MAC_ACTION_OFFSET_ACTION])
                {
                    case MAC_PUB_FTM:
#ifdef _PRE_WLAN_FEATURE_FTM
                        *pen_go_on = OAL_FALSE;
                        if(WLAN_SCAN_MODE_FTM_REQ == pst_scan_params->en_scan_mode)
                        {
                            dmac_sta_rx_ftm(pst_dmac_vap, pst_netbuf);
                        }
#endif
                        break;

                    default:
                        break;
                }
            }
            break;

            default:
            break;
        }
    }
    return OAL_SUCC;
}


OAL_STATIC oal_uint16  dmac_scan_encap_probe_req_frame(dmac_vap_stru *pst_dmac_vap, oal_netbuf_stru *pst_mgmt_buf, oal_uint8 *puc_bssid, oal_int8 *pc_ssid)
{
    oal_uint8        uc_ie_len;
    oal_uint8       *puc_mac_header          = oal_netbuf_header(pst_mgmt_buf);
    oal_uint8       *puc_payload_addr        = mac_netbuf_get_payload(pst_mgmt_buf);
    oal_uint8       *puc_payload_addr_origin = puc_payload_addr;
    mac_device_stru *pst_mac_device          = OAL_PTR_NULL;
    oal_uint16       us_app_ie_len;
    oal_uint8        uc_dsss_channel_num;

    pst_mac_device = mac_res_get_dev(pst_dmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{dmac_scan_encap_probe_req_frame::pst_mac_device[%d] null!}", pst_dmac_vap->st_vap_base_info.uc_device_id);
        return 0;
    }

    /*************************************************************************/
    /*                        Management Frame Format                        */
    /* --------------------------------------------------------------------  */
    /* |Frame Control|Duration|DA|SA|BSSID|Sequence Control|Frame Body|FCS|  */
    /* --------------------------------------------------------------------  */
    /* | 2           |2       |6 |6 |6    |2               |0 - 2312  |4  |  */
    /* --------------------------------------------------------------------  */
    /*                                                                       */
    /*************************************************************************/

    /*************************************************************************/
    /*                Set the fields in the frame header                     */
    /*************************************************************************/
    /* ֡�����ֶ�ȫΪ0������type��subtype */
    mac_hdr_set_frame_control(puc_mac_header, WLAN_PROTOCOL_VERSION| WLAN_FC0_TYPE_MGT | WLAN_FC0_SUBTYPE_PROBE_REQ);

    /* ���÷�Ƭ���Ϊ0 */
    mac_hdr_set_fragment_number(puc_mac_header, 0);

    /* ���õ�ַ1���㲥��ַ */
    oal_set_mac_addr(puc_mac_header + WLAN_HDR_ADDR1_OFFSET, BROADCAST_MACADDR);

    /* ���õ�ַ2��MAC��ַ(p2pɨ��Ϊp2p�ĵ�ַ������Ϊ������ַ��������mac addrɨ�迪������Ϊ�����ַ) */
    oal_set_mac_addr(puc_mac_header + WLAN_HDR_ADDR2_OFFSET, pst_mac_device->st_scan_params.auc_sour_mac_addr);

    /* ��ַ3���㲥��ַ */
    oal_set_mac_addr(puc_mac_header + WLAN_HDR_ADDR3_OFFSET, puc_bssid);

    /*************************************************************************/
    /*                       Probe Request Frame - Frame Body                */
    /* --------------------------------------------------------------------- */
    /* |SSID |Supported Rates |Extended supp rates| HT cap|Extended cap      */
    /* --------------------------------------------------------------------- */
    /* |2-34 |   3-10         | 2-257             |  28   | 3-8              */
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /*************************************************************************/

    /* ����SSID */
    /***************************************************************************
                    ----------------------------
                    |Element ID | Length | SSID|
                    ----------------------------
           Octets:  |1          | 1      | 0~32|
                    ----------------------------
    ***************************************************************************/
    if ('\0' == pc_ssid[0])    /* ͨ��SSID */
    {
        puc_payload_addr[0] = MAC_EID_SSID;
        puc_payload_addr[1] = 0;
        puc_payload_addr   += MAC_IE_HDR_LEN;    /* ƫ��bufferָ����һ��ie */
    }
    else
    {
        puc_payload_addr[0] = MAC_EID_SSID;
        puc_payload_addr[1] = (oal_uint8)OAL_STRLEN(pc_ssid);
        oal_memcopy(&(puc_payload_addr[2]), pc_ssid, puc_payload_addr[1]);
        puc_payload_addr += MAC_IE_HDR_LEN + puc_payload_addr[1];  /* ƫ��bufferָ����һ��ie */
    }

    /* ����֧�ֵ����ʼ� */
    mac_set_supported_rates_ie(&(pst_dmac_vap->st_vap_base_info), puc_payload_addr, &uc_ie_len);
    puc_payload_addr += uc_ie_len;

    /* ��ȡdsss ie�ڵ�channel num */
    uc_dsss_channel_num = dmac_get_dsss_ie_channel_num(&(pst_dmac_vap->st_vap_base_info));/* ����dsss������ */

    /* ����dsss������ */
    mac_set_dsss_params(&(pst_dmac_vap->st_vap_base_info), puc_payload_addr, &uc_ie_len, uc_dsss_channel_num);
    puc_payload_addr += uc_ie_len;

    /* ����extended supported rates��Ϣ */
    mac_set_exsup_rates_ie(&(pst_dmac_vap->st_vap_base_info), puc_payload_addr, &uc_ie_len);
    puc_payload_addr += uc_ie_len;

    /* PNOɨ��,probe request����ֻ�����ŵ������ʼ���ϢԪ��,���ٷ��ͱ��ĳ��� */
    if((MAC_SCAN_STATE_RUNNING == pst_mac_device->en_curr_scan_state)
       && (WLAN_SCAN_MODE_BACKGROUND_PNO == pst_mac_device->st_scan_params.en_scan_mode))
    {
        return (oal_uint16)((puc_payload_addr - puc_payload_addr_origin) + MAC_80211_FRAME_LEN);
    }

    /* ���HT Capabilities��Ϣ */
    mac_set_ht_capabilities_ie(&(pst_dmac_vap->st_vap_base_info), puc_payload_addr, &uc_ie_len);
    puc_payload_addr += uc_ie_len;

    /* ���Extended Capabilities��Ϣ */
    mac_set_ext_capabilities_ie(&(pst_dmac_vap->st_vap_base_info), puc_payload_addr, &uc_ie_len);
    puc_payload_addr += uc_ie_len;

    /* ���vht capabilities��Ϣ */
    if((WLAN_BAND_2G != pst_dmac_vap->st_vap_base_info.st_channel.en_band))
    {
        mac_set_vht_capabilities_ie(&(pst_dmac_vap->st_vap_base_info), puc_payload_addr, &uc_ie_len);
        puc_payload_addr += uc_ie_len;
    }

    /* ���WPS��Ϣ */
    mac_add_app_ie((oal_void *)&(pst_dmac_vap->st_vap_base_info), puc_payload_addr, &us_app_ie_len, OAL_APP_PROBE_REQ_IE);
    puc_payload_addr += us_app_ie_len;
#ifdef _PRE_WLAN_FEATURE_HILINK
    /* ���okc ie��Ϣ */
    mac_add_app_ie((oal_void *)&(pst_dmac_vap->st_vap_base_info), puc_payload_addr, &us_app_ie_len, OAL_APP_OKC_PROBE_IE);
    puc_payload_addr += us_app_ie_len;
#endif

#ifdef _PRE_WLAN_FEATURE_11K
    if (OAL_TRUE == pst_dmac_vap->bit_11k_enable)
    {
        mac_set_wfa_tpc_report_ie(&(pst_dmac_vap->st_vap_base_info), puc_payload_addr, &uc_ie_len);
        puc_payload_addr += uc_ie_len;
    }
#endif //_PRE_WLAN_FEATURE_11K

    /* multi-sta����������4��ַie */
#ifdef _PRE_WLAN_FEATURE_VIRTUAL_MULTI_STA
    mac_set_vender_4addr_ie((oal_void *)(&pst_dmac_vap->st_vap_base_info), puc_payload_addr, &uc_ie_len);
    puc_payload_addr += uc_ie_len;
#endif

    return (oal_uint16)((puc_payload_addr - puc_payload_addr_origin) + MAC_80211_FRAME_LEN);
}


oal_uint32  dmac_scan_send_probe_req_frame(dmac_vap_stru *pst_dmac_vap,
                                            oal_uint8 *puc_bssid,
                                            oal_int8 *pc_ssid)
{
    oal_netbuf_stru        *pst_mgmt_buf;
    mac_tx_ctl_stru        *pst_tx_ctl;
    oal_uint32              ul_ret;
    oal_uint16              us_mgmt_len;
    oal_uint8              *puc_mac_header = OAL_PTR_NULL;
    oal_uint8              *puc_saddr;

    /* �������֡�ڴ� */
    pst_mgmt_buf = OAL_MEM_NETBUF_ALLOC(OAL_MGMT_NETBUF, WLAN_SMGMT_NETBUF_SIZE, OAL_NETBUF_PRIORITY_MID);
    if (OAL_PTR_NULL == pst_mgmt_buf)
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{dmac_scan_send_probe_req_frame::alloc netbuf failed.}");
        OAL_MEM_INFO_PRINT(OAL_MEM_POOL_ID_NETBUF);
        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_set_netbuf_prev(pst_mgmt_buf, OAL_PTR_NULL);
    oal_set_netbuf_next(pst_mgmt_buf, OAL_PTR_NULL);

    OAL_MEM_NETBUF_TRACE(pst_mgmt_buf, OAL_TRUE);

    /* ��װprobe request֡ */
    us_mgmt_len = dmac_scan_encap_probe_req_frame(pst_dmac_vap, pst_mgmt_buf, puc_bssid, pc_ssid);
    if(WLAN_SMGMT_NETBUF_SIZE < (us_mgmt_len - MAC_80211_FRAME_LEN))
    {
        OAM_ERROR_LOG2(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{dmac_scan_send_probe_req_frame::us_payload_len=[%d] over net_buf_size=[%d].}",
        us_mgmt_len - MAC_80211_FRAME_LEN, WLAN_SMGMT_NETBUF_SIZE);
    }

    /* ��дnetbuf��cb�ֶΣ������͹���֡�ͷ�����ɽӿ�ʹ�� */
    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_mgmt_buf);

    OAL_MEMZERO(pst_tx_ctl, sizeof(mac_tx_ctl_stru));
    MAC_GET_CB_WME_AC_TYPE(pst_tx_ctl) = WLAN_WME_AC_MGMT;

    if (!ETHER_IS_MULTICAST(puc_bssid))
    {
        /* ���͵���̽��֡ */
        puc_mac_header = oal_netbuf_header(pst_mgmt_buf);
        puc_saddr = mac_vap_get_mac_addr(&(pst_dmac_vap->st_vap_base_info));
        oal_set_mac_addr(puc_mac_header + WLAN_HDR_ADDR1_OFFSET, puc_bssid);
        oal_set_mac_addr(puc_mac_header + WLAN_HDR_ADDR2_OFFSET, puc_saddr);
        MAC_GET_CB_TX_USER_IDX(pst_tx_ctl) = pst_dmac_vap->st_vap_base_info.us_assoc_vap_id;
    }
    else
    {
        /* �����㲥̽��֡ */
        MAC_GET_CB_IS_MCAST(pst_tx_ctl) = OAL_TRUE;
        MAC_GET_CB_TX_USER_IDX(pst_tx_ctl) = pst_dmac_vap->st_vap_base_info.us_multi_user_idx; /* probe request֡�ǹ㲥֡ */
    }

    /* ���÷��͹���֡�ӿ� */
    ul_ret = dmac_tx_mgmt(pst_dmac_vap, pst_mgmt_buf, us_mgmt_len);
    if (OAL_SUCC != ul_ret)
    {

        oal_netbuf_free(pst_mgmt_buf);
        return ul_ret;
    }

    return OAL_SUCC;
}


oal_uint32  dmac_scan_proc_scan_complete_event(dmac_vap_stru *pst_dmac_vap,
                                               mac_scan_status_enum_uint8 en_scan_rsp_status)
{
    frw_event_mem_stru         *pst_event_mem;
    frw_event_stru             *pst_event;
    mac_device_stru            *pst_mac_device;
    oal_uint8                   uc_vap_id;
    mac_scan_rsp_stru          *pst_scan_rsp_info;
#ifdef _PRE_WLAN_FEATURE_DBAC
    mac_vap_state_enum_uint8    en_state = pst_dmac_vap->st_vap_base_info.en_vap_state;
    oal_bool_enum_uint8         en_dbac_running;
#endif
    uc_vap_id = pst_dmac_vap->st_vap_base_info.uc_vap_id;

    /* ��ȡdevice�ṹ�� */
    pst_mac_device = mac_res_get_dev(pst_dmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(uc_vap_id, OAM_SF_SCAN, "{dmac_scan_proc_scan_complete_event::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

#ifdef _PRE_WLAN_FEATURE_DBAC
    en_dbac_running = mac_is_dbac_running(pst_mac_device);
#endif
    /* ��ɨ�������¼���DMAC, �����¼��ڴ� */
    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(mac_scan_rsp_stru));
    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG0(uc_vap_id, OAM_SF_SCAN, "{dmac_scan_proc_scan_complete_event::alloc memory failed.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��д�¼� */
    pst_event = frw_get_event_stru(pst_event_mem);

    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_WLAN_CRX,
                       DMAC_WLAN_CRX_EVENT_SUB_TYPE_SCAN_COMP,
                       OAL_SIZEOF(mac_scan_rsp_stru),
                       FRW_EVENT_PIPELINE_STAGE_1,
                       pst_dmac_vap->st_vap_base_info.uc_chip_id,
                       pst_dmac_vap->st_vap_base_info.uc_device_id,
                       pst_dmac_vap->st_vap_base_info.uc_vap_id);

    pst_scan_rsp_info = (mac_scan_rsp_stru *)(pst_event->auc_event_data);

    /* ����ɨ�����ʱ״̬���Ǳ��ܾ�������ִ�гɹ� */
    if(OAL_TRUE == pst_mac_device->st_scan_params.en_abort_scan_flag)
    {
        pst_scan_rsp_info->en_scan_rsp_status = MAC_SCAN_ABORT;
    }
    else
    {
        pst_scan_rsp_info->en_scan_rsp_status = en_scan_rsp_status;
    }

    pst_scan_rsp_info->ull_cookie         = pst_mac_device->st_scan_params.ull_cookie;

    OAM_WARNING_LOG4(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{dmac_scan_proc_scan_complete_event::status:%d, vap channel:%d, cookie[%x], scan_mode[%d]}",
                        en_scan_rsp_status,
                        pst_dmac_vap->st_vap_base_info.st_channel.uc_chan_number,
                        pst_scan_rsp_info->ull_cookie,
                        pst_mac_device->st_scan_params.en_scan_mode);

    /* �ַ��¼� */
    frw_event_dispatch_event(pst_event_mem);
    FRW_EVENT_FREE(pst_event_mem);

    // HMAC�ὫVAP��״̬����Ϊɨ��֮ǰ��״̬�����ܵ���VAP��UP,�˴��ָ�VAP��״̬
    // HMACִ�������п����޸���DBAC����״̬����˲������ڴ˴���ȡvap״̬
#ifdef _PRE_WLAN_FEATURE_DBAC
    if (en_dbac_running
    && (pst_dmac_vap->st_vap_base_info.en_vap_state == MAC_VAP_STATE_UP)
    && (pst_dmac_vap->st_vap_base_info.en_vap_state != en_state))
    {
        OAM_WARNING_LOG2(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
                "{dmac_scan_proc_scan_complete_event::restore state from %d to %d when scan while dbac running}",
                        pst_dmac_vap->st_vap_base_info.en_vap_state, en_state);
        mac_vap_state_change(&pst_dmac_vap->st_vap_base_info, en_state);
    }
#endif

    return OAL_SUCC;
}

#if 0

OAL_STATIC oal_uint32  dmac_scan_is_too_busy(mac_device_stru *pst_mac_device, mac_scan_req_stru *pst_scan_req_params)
{
    oal_uint32       ul_ret;
    oal_uint32       ul_timestamp;
    oal_uint32       ul_deltatime;

    ul_ret = mac_device_is_p2p_connected(pst_mac_device);
    if (OAL_SUCC == ul_ret)
    {
        ul_timestamp = (oal_uint32)OAL_TIME_GET_STAMP_MS();
        ul_deltatime = (ul_timestamp > pst_mac_device->ul_scan_timestamp)? \
                       (ul_timestamp - pst_mac_device->ul_scan_timestamp):(0xffffffff - pst_mac_device->ul_scan_timestamp + ul_timestamp);

        if (MAC_SCAN_FUNC_P2P_LISTEN != pst_scan_req_params->uc_scan_func)
        {
            if (ul_deltatime < DMAC_SCAN_DBAC_SCAN_DELTA_TIME)
            {
                OAM_WARNING_LOG2(0, OAM_SF_DBAC, "has connected p2p. scan deltatime:%d<%d, refused", ul_deltatime, DMAC_SCAN_DBAC_SCAN_DELTA_TIME);
                return OAL_TRUE;
            }
        }
        else
        {
            if (ul_deltatime > 500 || pst_scan_req_params->us_scan_time >= DMAC_SCAN_GO_MAX_SCAN_TIME)
            {
                OAM_WARNING_LOG2(0, OAM_SF_DBAC, "has connected p2p. p2p listen deltatime:%d, scan_time:%d, refused", ul_deltatime, pst_scan_req_params->us_scan_time);
                return OAL_TRUE;
            }
        }
    }

    return OAL_FALSE;
}
#endif


oal_uint32  dmac_scan_update_channel_list(mac_device_stru    *pst_mac_device,
                                        dmac_vap_stru      *pst_dmac_vap)
{
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    if (mac_is_proxysta_enabled(pst_mac_device))
    {
        /* proxysta ֻɨ��һ���ŵ� */
        if (mac_vap_is_vsta(&pst_dmac_vap->st_vap_base_info))
        {
            oal_uint8 uc_idx;
            oal_int32 l_found_idx = -1;

            mac_vap_stru *pst_msta = mac_find_main_proxysta(pst_mac_device);

            if (!pst_msta)
            {
                return OAL_FAIL;
            }

            for (uc_idx = 0; uc_idx < pst_mac_device->st_scan_params.uc_channel_nums; uc_idx++)
            {
                if (pst_mac_device->st_scan_params.ast_channel_list[uc_idx].uc_chan_number == pst_msta->st_channel.uc_chan_number)
                {
                    l_found_idx = (oal_int32)uc_idx;
                    break;
                }
            }

            if (l_found_idx >= 0)
            {
                pst_mac_device->st_scan_params.uc_channel_nums = 1;
                pst_mac_device->st_scan_params.ast_channel_list[0] = pst_mac_device->st_scan_params.ast_channel_list[l_found_idx];
            }
        }
    }
#endif
    return OAL_SUCC;
}

OAL_STATIC oal_void dmac_scan_set_fast_scan_flag(hal_scan_info_stru *pst_scan_info, dmac_device_stru *pst_dmac_device)
{
    /* ͬʱɨ2g 5g,Ӳ��֧��,���֧�� */
    if ((pst_scan_info->uc_num_channels_2G != 0) && (pst_scan_info->uc_num_channels_5G != 0)
          && (OAL_TRUE == dmac_device_is_support_double_hal_device(pst_dmac_device))
          && (OAL_TRUE == pst_dmac_device->en_fast_scan_enable))
    {
        pst_dmac_device->en_is_fast_scan = OAL_TRUE;
    }
    else
    {
        pst_dmac_device->en_is_fast_scan = OAL_FALSE;
    }
}


OAL_STATIC oal_void dmac_scan_assign_chan_for_hal_device(dmac_device_stru  *pst_dmac_device, hal_scan_info_stru *pst_scan_info, dmac_vap_stru *pst_dmac_vap)
{
    oal_uint8                               uc_idx;
    oal_uint8                               uc_dev_num_per_chip;
    hal_to_dmac_device_stru                *pst_hal_device;
    //hal_to_dmac_device_stru                *pst_work_hal_device = OAL_PTR_NULL;
    //hal_to_dmac_device_stru                *pst_other_hal_device = OAL_PTR_NULL;

    if (OAL_FALSE == pst_dmac_device->en_is_fast_scan)
    {
        /* hal device ׼��ɨ��channel���� */
        hal_device_handle_event(pst_dmac_vap->pst_hal_device, HAL_DEVICE_EVENT_SCAN_BEGIN, OAL_SIZEOF(hal_scan_info_stru), (oal_uint8 *)pst_scan_info);
        return;
    }

    hal_chip_get_device_num(pst_dmac_device->pst_device_base_info->uc_chip_id, &uc_dev_num_per_chip);
    for (uc_idx = 0; uc_idx < uc_dev_num_per_chip; uc_idx++)
    {
        pst_hal_device = dmac_scan_find_hal_device(pst_dmac_device->pst_device_base_info, pst_dmac_vap, uc_idx);
        if (OAL_PTR_NULL == pst_hal_device)
        {
            continue;
        }

        if (OAL_FALSE == pst_dmac_device->pst_device_base_info->en_dbdc_running)
        {
            /* ��·ɨ2g,��·ɨ5g */
            pst_scan_info->en_scan_band = (OAL_TRUE == pst_hal_device->en_is_master_hal_device) ? WLAN_BAND_2G : WLAN_BAND_5G;

            /* ����Ǩ�Ƶ�vap�����Ƿ�Ҫ��11b�����Ǩ�Ƶ���hal device */
            if (WLAN_BAND_2G == pst_scan_info->en_scan_band)
            {
                hal_set_11b_reuse_sel(pst_hal_device);
            }
        }
        /* DBDCģʽ�¿����Լ�ɨ�Լ�������Ƶ��,���Զ���������hal device��ɨ�� */
        else
        {
            pst_scan_info->en_scan_band = pst_hal_device->st_wifi_channel_status.en_band;
        }

        /* hal device ׼��ɨ��channel���� */
        hal_device_handle_event(pst_hal_device, HAL_DEVICE_EVENT_SCAN_BEGIN, OAL_SIZEOF(hal_scan_info_stru), (oal_uint8 *)pst_scan_info);
    }
}

OAL_STATIC oal_void dmac_fscan_reorder_channel_list(mac_device_stru  *pst_mac_device, hal_scan_info_stru *pst_scan_info)
{
    oal_uint8                               uc_idx;
    oal_int8                                c_chan_idx;
    oal_uint8                               uc_chan_cnt = 0;
    oal_int8                                c_bad_chan_cnt =0;
    mac_channel_stru                        ast_bad_channel_list[6];

    OAL_MEMZERO(ast_bad_channel_list, OAL_SIZEOF(ast_bad_channel_list));

    uc_chan_cnt = pst_scan_info->uc_num_channels_2G;//�ӵ�һ��5g�ŵ���ʼ

    /*����ɨ���ǽ�5G 100-120�ŵ��ŵ������˳��Ϊ120 116 112 108 104 100*/
    for (uc_idx = uc_chan_cnt; uc_idx < pst_mac_device->st_scan_params.uc_channel_nums; uc_idx++)
    {
        if (WLAN_BAND_2G == pst_mac_device->st_scan_params.ast_channel_list[uc_idx].en_band)
        {
            OAM_ERROR_LOG4(0, OAM_SF_DBDC, "dmac_fscan_reorder_channel_list:wrong!!!:band[%d],chan[%d]bw[%d]idx[%d]",
                        pst_mac_device->st_scan_params.ast_channel_list[uc_idx].en_band,pst_mac_device->st_scan_params.ast_channel_list[uc_idx].uc_chan_number, pst_mac_device->st_scan_params.ast_channel_list[uc_idx].en_bandwidth,
                        pst_mac_device->st_scan_params.ast_channel_list[uc_idx].uc_chan_idx);
        }
        if (((pst_mac_device->st_scan_params.ast_channel_list[uc_idx].uc_chan_number < 100)
               || (pst_mac_device->st_scan_params.ast_channel_list[uc_idx].uc_chan_number > 120)))
        {
            /* 100-120��λ���ɺ�����ŵ����� */
            if (uc_chan_cnt != uc_idx)
            {
                pst_mac_device->st_scan_params.ast_channel_list[uc_chan_cnt] = pst_mac_device->st_scan_params.ast_channel_list[uc_idx];
            }
            uc_chan_cnt++;
        }
        /* ����100-120���ŵ���Ϣ */
        else
        {
            ast_bad_channel_list[(oal_uint8)c_bad_chan_cnt++] = pst_mac_device->st_scan_params.ast_channel_list[uc_idx];
        }
    }

    /*5G good �ŵ�����С��2G�ܵ��ŵ�����,��ʱ�ض���ײ��2GӰ��5G�ŵ��ĳ��� */
    if ((pst_scan_info->uc_num_channels_5G - c_bad_chan_cnt) < pst_scan_info->uc_num_channels_2G)
    {
        OAM_WARNING_LOG0(0, OAM_SF_DBDC, "dmac_scan_reorder_channel_list::2g affect 5g");
    }

    /*�ٽ�100-120���ŵ��Ż�ԭɨ���ŵ��б�,�ŵ���ԽС���Ƶ�2g�ŵ�Խ��,������� */
    if (c_bad_chan_cnt != 0)
    {
        for (c_chan_idx = (c_bad_chan_cnt - 1); c_chan_idx >= 0; c_chan_idx--)
        {
            pst_mac_device->st_scan_params.ast_channel_list[uc_chan_cnt++] = ast_bad_channel_list[(oal_uint8)c_chan_idx];
        }
    }
}


// TODO: ���ǽ���Щ����vap����д�ļĴ�������������·�Ǿ�ͬʱд��·��
OAL_STATIC oal_void dmac_prepare_for_fast_scan(dmac_device_stru *pst_dmac_device, dmac_vap_stru *pst_dmac_vap, hal_scan_info_stru *pst_scan_info)
{
    hal_to_dmac_vap_stru                   *pst_ori_hal_vap;
    hal_to_dmac_vap_stru                   *pst_shift_hal_vap;
    hal_to_dmac_device_stru                *pst_ori_hal_device;
    hal_to_dmac_device_stru                *pst_shift_hal_device;

    pst_ori_hal_device   = DMAC_VAP_GET_HAL_DEVICE(pst_dmac_vap);
    pst_shift_hal_device = dmac_device_get_another_h2d_dev(pst_dmac_device, pst_ori_hal_device);
    if (OAL_PTR_NULL  == pst_shift_hal_device)
    {
        OAM_ERROR_LOG2(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_DBDC, "dmac_prepare_for_fast_scan::pst_shift_hal_device NULL, chip id[%d],ori hal device id[%d]",
                        pst_dmac_vap->st_vap_base_info.uc_chip_id, pst_ori_hal_device->uc_device_id);
        return;
    }

    pst_ori_hal_vap  = DMAC_VAP_GET_HAL_VAP(pst_dmac_vap);
    hal_get_hal_vap(pst_shift_hal_device, pst_ori_hal_vap->uc_vap_id, &pst_shift_hal_vap);

#ifdef _PRE_WLAN_FEATURE_P2P
    /* P2P ����MAC ��ַ */
    if ((WLAN_P2P_DEV_MODE == pst_dmac_vap->st_vap_base_info.en_p2p_mode) && (WLAN_VAP_MODE_BSS_STA == pst_dmac_vap->st_vap_base_info.en_vap_mode))
    {
        hal_vap_set_macaddr(pst_shift_hal_vap, pst_dmac_vap->st_vap_base_info.pst_mib_info->st_wlan_mib_sta_config.auc_p2p0_dot11StationID);
    }
    else
    {
        /* ��������vap ��mac ��ַ */
        hal_vap_set_macaddr(pst_shift_hal_vap, mac_mib_get_StationID(&(pst_dmac_vap->st_vap_base_info)));
    }
#else
    /* ����MAC��ַ */
    hal_vap_set_macaddr(pst_shift_hal_vap, mac_mib_get_StationID(&(pst_dmac_vap->st_vap_base_info)));
#endif

    /* ʹ��PA_CONTROL��vap_controlλ */
    hal_vap_set_opmode(pst_shift_hal_vap, pst_dmac_vap->st_vap_base_info.en_vap_mode);

    /* ������ɨ���ŵ��б� */
    dmac_fscan_reorder_channel_list(pst_dmac_device->pst_device_base_info, pst_scan_info);
}


OAL_STATIC oal_void dmac_prepare_for_scan(mac_device_stru  *pst_mac_device, dmac_vap_stru *pst_dmac_vap)
{
    oal_uint8                               uc_idx;
    hal_scan_info_stru                      st_scan_info = {0};
    dmac_device_stru                       *pst_dmac_device;

#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
    if (pst_mac_device->st_scan_params.en_scan_mode < WLAN_SCAN_MODE_BACKGROUND_OBSS)
    {
        pst_mac_device->uc_scan_count++;
    }
#endif

    /* ��¼ɨ�迪ʼʱ��,ɨ�����������ɨ��ʱ�� */
    pst_mac_device->ul_scan_timestamp  = (oal_uint32)OAL_TIME_GET_STAMP_MS();

    for (uc_idx = 0; uc_idx < pst_mac_device->st_scan_params.uc_channel_nums; uc_idx++)
    {
        if (WLAN_BAND_2G == pst_mac_device->st_scan_params.ast_channel_list[uc_idx].en_band)
        {
             st_scan_info.uc_num_channels_2G++;
        }
        else if (WLAN_BAND_5G == pst_mac_device->st_scan_params.ast_channel_list[uc_idx].en_band)
        {
             st_scan_info.uc_num_channels_5G++;
        }
        else
        {

        }
    }

    pst_dmac_device = dmac_res_get_mac_dev(pst_mac_device->uc_device_id);
    if (OAL_PTR_NULL == pst_dmac_device)
    {
        OAM_ERROR_LOG0(pst_mac_device->uc_device_id, OAM_SF_SCAN, "{dmac_prepare_for_scan::pst_dmac_device null.}");
        return;
    }

#ifdef _PRE_WLAN_FEATURE_GNSS_SCAN
    dmac_scan_init_bss_info_list(pst_mac_device);
#endif

    /* Ӳ��֧�ֲ�����Ҫɨ2G��5G��ô���ǲ���ɨ�� */
    dmac_scan_set_fast_scan_flag(&st_scan_info, pst_dmac_device);
    if (OAL_TRUE == pst_dmac_device->en_is_fast_scan)
    {
        dmac_prepare_for_fast_scan(pst_dmac_device, pst_dmac_vap, &st_scan_info);
    }

    st_scan_info.en_is_fast_scan               = pst_dmac_device->en_is_fast_scan;
    st_scan_info.us_scan_time                  = pst_mac_device->st_scan_params.us_scan_time;
    st_scan_info.en_scan_mode                  = pst_mac_device->st_scan_params.en_scan_mode;
    st_scan_info.uc_scan_channel_interval      = pst_mac_device->st_scan_params.uc_scan_channel_interval;
    st_scan_info.uc_max_scan_count_per_channel = pst_mac_device->st_scan_params.uc_max_scan_count_per_channel;
    st_scan_info.us_work_time_on_home_channel  = pst_mac_device->st_scan_params.us_work_time_on_home_channel;

    dmac_scan_assign_chan_for_hal_device(pst_dmac_device, &st_scan_info, pst_dmac_vap);
}

oal_uint32  dmac_scan_handle_scan_req_entry(mac_device_stru    *pst_mac_device,
                                            dmac_vap_stru      *pst_dmac_vap,
                                            mac_scan_req_stru  *pst_scan_req_params)
{
    if (WLAN_SCAN_MODE_BACKGROUND_CCA == pst_scan_req_params->en_scan_mode)
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "dmac_scan_handle_scan_req_entry::cca can not be here!!!");
        return OAL_FAIL;
    }

    /* �������ɨ��״̬����ֱ�ӷ��� */
    /* ������ڳ�������״̬����ֱ�ӷ��� */
    if((MAC_SCAN_STATE_RUNNING == pst_mac_device->en_curr_scan_state)
       || ((OAL_SWITCH_ON == pst_dmac_vap->st_vap_base_info.bit_al_tx_flag) && (WLAN_VAP_MODE_BSS_STA == pst_dmac_vap->st_vap_base_info.en_vap_mode)))
    {
    #if (defined(_PRE_PRODUCT_ID_HI110X_DEV))
        OAM_WARNING_LOG_ALTER(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
                         "{dmac_scan_handle_scan_req_entry:: device scan is running or always tx is running, cann't start scan. scan_vap_id[%d], scan_func[0x%02x], curr_scan_mode[%d], req_scan_mode[%d], scan_cookie[%x], al_tx_flag[%d].}",
                         6,
                         pst_mac_device->st_scan_params.uc_vap_id,
                         pst_mac_device->st_scan_params.uc_scan_func,
                         pst_mac_device->st_scan_params.en_scan_mode,
                         pst_scan_req_params->en_scan_mode,
                         pst_scan_req_params->ull_cookie,
                         pst_dmac_vap->st_vap_base_info.bit_al_tx_flag);
    #else
        OAM_WARNING_LOG4(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
                         "{dmac_scan_handle_scan_req_entry:: device scan is running or always tx is running, cann't start scan. scan_vap_id[%d], scan_func[0x%02x], scan_cookie[%x], al_tx_flag[%d].}",
                         pst_mac_device->st_scan_params.uc_vap_id,
                         pst_mac_device->st_scan_params.uc_scan_func,
                         pst_scan_req_params->ull_cookie,
                         pst_dmac_vap->st_vap_base_info.bit_al_tx_flag);
    #endif

        /* ������ϲ��·���ɨ������ֱ����ɨ������¼�; OBSSɨ���򷵻ؽ������ȴ���һ�ζ�ʱ����ʱ�ٷ���ɨ�� */
        if (pst_scan_req_params->en_scan_mode < WLAN_SCAN_MODE_BACKGROUND_OBSS)
        {
            /* ����ɨ���·���cookie ֵ */
            pst_mac_device->st_scan_params.ull_cookie = pst_scan_req_params->ull_cookie;

            /* ��ɨ������¼���ɨ�����󱻾ܾ� */
            return dmac_scan_proc_scan_complete_event(pst_dmac_vap, MAC_SCAN_REFUSED);
        }
#ifdef _PRE_WLAN_FEATURE_11K
        else if (WLAN_SCAN_MODE_RRM_BEACON_REQ == pst_scan_req_params->en_scan_mode)
        {
            OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
                         "{dmac_scan_handle_scan_req_entry::RRM BEACON REQ SCAN FAIL");
            return OAL_FAIL;
        }
#endif
#ifdef _PRE_WLAN_FEATURE_FTM
        else if (WLAN_SCAN_MODE_FTM_REQ == pst_scan_req_params->en_scan_mode)
        {
            OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
                         "{dmac_scan_handle_scan_req_entry::FTM SCAN FAIL");
            return OAL_FAIL;
        }
#endif
#ifdef _PRE_WLAN_FEATURE_GNSS_SCAN
        else if (WLAN_SCAN_MODE_GNSS_SCAN == pst_scan_req_params->en_scan_mode)
        {
            OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
                         "{dmac_scan_handle_scan_req_entry::GNSS SCAN FAIL");
            return OAL_FAIL;
        }
#endif
        else
        {
            return OAL_SUCC;
        }
    }

    /* ����device��ǰɨ��״̬Ϊ����״̬ */
    pst_mac_device->en_curr_scan_state = MAC_SCAN_STATE_RUNNING;

#ifdef _PRE_WLAN_FEATURE_BTCOEX
    //ɨ�裬bt ps�͵͹���ͬ�ȵ��ȣ�ֱ����ͣbt ps��ʱ����
    //dmac_btcoex_bt_acl_status_check(pst_dmac_vap);

    /* ������ʼ��֪ͨBT */
    hal_set_btcoex_soc_gpreg1(OAL_FALSE, BIT0, 0);  //�����һ���ϵ�ɨ���״̬
    hal_set_btcoex_soc_gpreg1(OAL_TRUE, BIT1, 1);   // �������̣�����ɨ��״̬

    if (WLAN_P2P_DEV_MODE == pst_dmac_vap->st_vap_base_info.en_p2p_mode)
    {
        hal_set_btcoex_soc_gpreg0(OAL_TRUE, BIT14, 14);   // p2p ɨ�����̿�ʼ
    }

    hal_coex_sw_irq_set(HAL_COEX_SW_IRQ_BT);
#endif

#ifdef _PRE_WLAN_FEATURE_STA_PM
    /* ƽ̨�ļ��� */
    if (WLAN_SCAN_MODE_BACKGROUND_AP >= pst_scan_req_params->en_scan_mode)
    {
        pfn_wlan_dumpsleepcnt();
    }
#endif

    /* ������ɨ��Ȩ�޺󣬽�ɨ�����������mac deivce�ṹ���£���ʱ������Ҳ��Ϊ�˷�ֹɨ�������������� */
    oal_memcopy(&(pst_mac_device->st_scan_params), pst_scan_req_params, OAL_SIZEOF(mac_scan_req_stru));

#ifdef _PRE_WLAN_FEATURE_P2P
    /* P2P0 ɨ��ʱ��¼P2P listen channel */
    if (OAL_TRUE == pst_scan_req_params->bit_is_p2p0_scan)
    {
        pst_dmac_vap->st_vap_base_info.uc_p2p_listen_channel = pst_scan_req_params->uc_p2p0_listen_channel;
    }
#endif
    /* ��ʼ��ɨ���ŵ����� */
    dmac_prepare_for_scan(pst_mac_device, pst_dmac_vap);

    dmac_scan_update_channel_list(pst_mac_device, pst_dmac_vap);

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    if (mac_is_proxysta_enabled(pst_mac_device)
      && pst_mac_device->st_scan_params.uc_channel_nums == 1
      && pst_mac_device->st_scan_params.ast_channel_list[0].uc_chan_number == pst_dmac_vap->pst_hal_device->uc_current_chan_number)
    {
        OAM_INFO_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,"{dmac_scan_handle_scan_req_entry:proxysta scan working channel %d, skip switching}",
                pst_mac_device->st_scan_params.ast_channel_list[0].uc_chan_number);
    }
    else
#endif
    {
        dmac_scan_switch_channel_off(pst_mac_device);//lzhqi ��������ʵ����;��Щƫ��
    }

#ifdef _PRE_WLAN_FEATURE_ANTI_INTERF
    dmac_alg_anti_intf_switch(pst_dmac_vap->pst_hal_device, OAL_FALSE);
#endif

#ifdef  _PRE_WLAN_FEATURE_GREEN_AP
    dmac_green_ap_switch_auto(pst_mac_device->uc_device_id, DMAC_GREEN_AP_SCAN_START);
#endif

    return OAL_SUCC;
}


OAL_STATIC oal_void dmac_scan_prepare_pno_scan_params(mac_scan_req_stru  *pst_scan_params,
                                                                  dmac_vap_stru    *pst_dmac_vap)
{
    oal_uint8           uc_chan_idx;
    oal_uint8           uc_2g_chan_num = 0;
    oal_uint8           uc_5g_chan_num = 0;
    oal_uint8           uc_chan_number;
    mac_device_stru    *pst_mac_device;

    /* ɨ������������� */
    OAL_MEMZERO(pst_scan_params, OAL_SIZEOF(mac_scan_req_stru));

    /* ���÷���ɨ���vap id */
    pst_scan_params->uc_vap_id = pst_dmac_vap->st_vap_base_info.uc_vap_id;

    /* ��ȡmac device */
    pst_mac_device = mac_res_get_dev(pst_dmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id,OAM_SF_SCAN, "{dmac_scan_prepare_pno_scan_params::pst_mac_device null.}");
        return;
    }

    /* ���ó�ʼɨ������Ĳ��� */
    pst_scan_params->en_bss_type    = WLAN_MIB_DESIRED_BSSTYPE_INFRA;
    pst_scan_params->en_scan_type   = WLAN_SCAN_TYPE_ACTIVE;
    pst_scan_params->en_scan_mode   = WLAN_SCAN_MODE_BACKGROUND_PNO;
    pst_scan_params->us_scan_time   = WLAN_DEFAULT_ACTIVE_SCAN_TIME * 2; /* ����PNOָ��SSIDɨ��,�ӳ�ÿ�ŵ�ɨ��ʱ��Ϊ40ms */
    pst_scan_params->uc_probe_delay = 0;
    pst_scan_params->uc_scan_func   = MAC_SCAN_FUNC_BSS;   /* Ĭ��ɨ��bss */
    pst_scan_params->uc_max_scan_count_per_channel           = 1;
    pst_scan_params->uc_max_send_probe_req_count_per_channel = WLAN_DEFAULT_SEND_PROBE_REQ_COUNT_PER_CHANNEL;

    /* ����ɨ���õ�pro req��src mac��ַ*/
    oal_set_mac_addr(pst_scan_params->auc_sour_mac_addr, pst_mac_device->pst_pno_sched_scan_mgmt->st_pno_sched_scan_params.auc_sour_mac_addr);
    pst_scan_params->en_is_random_mac_addr_scan = pst_mac_device->pst_pno_sched_scan_mgmt->st_pno_sched_scan_params.en_is_random_mac_addr_scan;

    /* ����ɨ�������ssid��Ϣ */
    pst_scan_params->ast_mac_ssid_set[0].auc_ssid[0] = '\0';   /* ͨ��ssid */
    pst_scan_params->uc_ssid_num = 1;

    /* ����ɨ������ָֻ��1��bssid��Ϊ�㲥��ַ */
    oal_set_mac_addr(pst_scan_params->auc_bssid[0], BROADCAST_MACADDR);
    pst_scan_params->uc_bssid_num = 1;

    /* 2G��ʼɨ���ŵ�, ȫ�ŵ�ɨ�� */
    for (uc_chan_idx = 0; uc_chan_idx < MAC_CHANNEL_FREQ_2_BUTT; uc_chan_idx++)
    {
        /* �ж��ŵ��ǲ����ڹ������� */
        if (OAL_SUCC == mac_is_channel_idx_valid(WLAN_BAND_2G, uc_chan_idx))
        {
            mac_get_channel_num_from_idx(WLAN_BAND_2G, uc_chan_idx, &uc_chan_number);

            pst_scan_params->ast_channel_list[uc_2g_chan_num].uc_chan_number = uc_chan_number;
            pst_scan_params->ast_channel_list[uc_2g_chan_num].en_band        = WLAN_BAND_2G;
            pst_scan_params->ast_channel_list[uc_2g_chan_num].uc_chan_idx         = uc_chan_idx;
            pst_scan_params->uc_channel_nums++;
            uc_2g_chan_num++;
        }
    }
    //OAM_INFO_LOG1(0, OAM_SF_SCAN, "{dmac_scan_prepare_pno_scan_params::after regdomain filter, the 2g total channel num is %d", uc_2g_chan_num);

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
    if (!hal_get_5g_enable())
    {
        return;
    }
#endif

    /* 5G��ʼɨ���ŵ�, ȫ�ŵ�ɨ�� */
    for (uc_chan_idx = 0; uc_chan_idx < MAC_CHANNEL_FREQ_5_BUTT; uc_chan_idx++)
    {
        /* �ж��ŵ��ǲ����ڹ������� */
        if (OAL_SUCC == mac_is_channel_idx_valid(WLAN_BAND_5G, uc_chan_idx))
        {
            mac_get_channel_num_from_idx(WLAN_BAND_5G, uc_chan_idx, &uc_chan_number);

            pst_scan_params->ast_channel_list[uc_2g_chan_num + uc_5g_chan_num].uc_chan_number = uc_chan_number;
            pst_scan_params->ast_channel_list[uc_2g_chan_num + uc_5g_chan_num].en_band        = WLAN_BAND_5G;
            pst_scan_params->ast_channel_list[uc_2g_chan_num + uc_5g_chan_num].uc_chan_idx         = uc_chan_idx;
            pst_scan_params->uc_channel_nums++;
            uc_5g_chan_num++;
        }
    }
    //OAM_INFO_LOG1(0, OAM_SF_SCAN, "{dmac_scan_prepare_pno_scan_params::after regdomain filter, the 5g total channel num is %d", uc_5g_chan_num);

    return;
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

/*lint -e528*/
OAL_STATIC oal_void  dmac_scan_pno_scan_timeout_fn(void *p_ptr, void *p_arg)
{
    dmac_vap_stru                       *pst_dmac_vap;
    mac_device_stru                     *pst_mac_device;
    mac_scan_req_stru                    st_scan_req_params;


    pst_dmac_vap = (dmac_vap_stru *)p_arg;
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dmac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{dmac_scan_pno_scan_timeout_fn::pst_dmac_vap null.}");
        return;
    }

    /* ��ȡmac device */
    pst_mac_device = mac_res_get_dev(pst_dmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
                       "{dmac_scan_pno_scan_timeout_fn::pst_mac_device null.}");
        return;
    }

#if 0
    /* ����pno����ɨ��Ĵ��� */
    pst_mac_device->pst_pno_sched_scan_mgmt->uc_curr_pno_sched_scan_times++;

    /* pno����ɨ�赽������ظ�������ֹͣɨ�����͹��� */
    if (pst_mac_device->pst_pno_sched_scan_mgmt->uc_curr_pno_sched_scan_times >= pst_mac_device->pst_pno_sched_scan_mgmt->st_pno_sched_scan_params.uc_pno_scan_repeat)
    {
        /* ֹͣPNOɨ�裬������͹��� */
        OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
                       "{dmac_scan_pno_scan_timeout_fn:: reached max pno scan repeat times, stop pno sched scan.}");

#ifdef _PRE_WLAN_FEATURE_STA_PM
        dmac_pm_sta_post_event(pst_dmac_vap, STA_PWR_EVENT_PNO_SCHED_SCAN_COMP, 0, OAL_PTR_NULL);
#endif
        /* �ͷ�PNO����ṹ���ڴ� */
        OAL_MEM_FREE(pst_mac_device->pst_pno_sched_scan_mgmt, OAL_TRUE);
        pst_mac_device->pst_pno_sched_scan_mgmt = OAL_PTR_NULL;
        return OAL_SUCC;
    }
#endif

    /* ��ʼ������Ϊ: δɨ�赽ƥ���bss */
    pst_mac_device->pst_pno_sched_scan_mgmt->en_is_found_match_ssid = OAL_FALSE;

    /* ׼��PNOɨ�������׼������ɨ�� */
    dmac_scan_prepare_pno_scan_params(&st_scan_req_params, pst_dmac_vap);

    /* ���·���ɨ�� */
    dmac_scan_handle_scan_req_entry(pst_mac_device, pst_dmac_vap, &st_scan_req_params);

    /* ��������PNO����ɨ�趨ʱ�� */
    dmac_scan_start_pno_sched_scan_timer((void *)pst_dmac_vap);

    return;
}
#endif



OAL_STATIC oal_uint32  dmac_scan_start_pno_sched_scan_timer(void *p_arg)
{
#if ((_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) && (_PRE_TEST_MODE != _PRE_TEST_MODE_UT))
    dmac_vap_stru                           *pst_dmac_vap = (dmac_vap_stru *)p_arg;
    mac_device_stru                         *pst_mac_device;
    mac_pno_sched_scan_mgmt_stru            *pst_pno_mgmt;
    oal_int32                                l_ret = OAL_SUCC;

    /* ��ȡmac device */
    pst_mac_device = mac_res_get_dev(pst_dmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
                       "{dmac_scan_start_pno_sched_scan_timer:: pst_mac_device is null.}");
        return OAL_FAIL;
    }

    /* ��ȡpno����ṹ�� */
    pst_pno_mgmt = pst_mac_device->pst_pno_sched_scan_mgmt;

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /* �ж�pno����ɨ���rtcʱ�Ӷ�ʱ���Ƿ��Ѿ����������δ�����������´���������ֱ���������� */
    if (OAL_PTR_NULL == pst_pno_mgmt->p_pno_sched_scan_timer)
    {
        pst_pno_mgmt->p_pno_sched_scan_timer = (oal_void *)oal_rtctimer_create(dmac_scan_pno_scan_timeout_fn, p_arg);
        if (OAL_PTR_NULL == pst_pno_mgmt->p_pno_sched_scan_timer)
        {
            OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
                           "{dmac_scan_start_pno_sched_scan_timer:: create pno timer faild.}");
            return OAL_FAIL;
        }
    }

#endif

    /* �����Ϸ��Լ�飬ʱ�������̣�pno����ִֻ��һ�� */
    if (0 == pst_pno_mgmt->st_pno_sched_scan_params.ul_pno_scan_interval / 100)
    {
        OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
                         "{dmac_scan_start_pno_sched_scan_timer:: pno scan interval[%d] time too short.}",
                         pst_pno_mgmt->st_pno_sched_scan_params.ul_pno_scan_interval);
        return OAL_FAIL;
    }

    /* ������ʱ�����ϲ��·���ɨ�����Ǻ��뼶�ģ�����ʱ����100���뼶�ģ������Ҫ����100 */
    l_ret = oal_rtctimer_start((STIMER_STRU *)pst_pno_mgmt->p_pno_sched_scan_timer, pst_pno_mgmt->st_pno_sched_scan_params.ul_pno_scan_interval / 100);
    if (OAL_SUCC != l_ret)
    {
        OAM_ERROR_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
                         "{dmac_scan_start_pno_sched_scan_timer:: start pno timer faild[%d].}",l_ret);
        return OAL_FAIL;
    }

    OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
                     "{dmac_scan_start_pno_sched_scan_timer:: start pno timer succ, timeout[%d].}",
                     pst_pno_mgmt->st_pno_sched_scan_params.ul_pno_scan_interval / 100);

    /* ��ǰPNOɨ�趨ʱ���ϲ��ʼ����60s(PNO_SCHED_SCAN_INTERVAL),��ʱ�����ں�ÿ�����ӳ�60��,�������300s */
    pst_pno_mgmt->st_pno_sched_scan_params.ul_pno_scan_interval += (60 * 1000);
    if(pst_pno_mgmt->st_pno_sched_scan_params.ul_pno_scan_interval > (300 * 1000))
    {
        pst_pno_mgmt->st_pno_sched_scan_params.ul_pno_scan_interval = (300 * 1000);
    }

#else
    /* 1151��֧�֣��Ҳ����ߵ��˴���do nothing����Ҫԭ��ƽ̨��δ��װ��ʱ����ؽӿ� */
#endif

    return OAL_SUCC;
}


oal_uint32  dmac_scan_stop_pno_sched_scan_timer(void *p_arg)
{
#if ((_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) && (_PRE_TEST_MODE != _PRE_TEST_MODE_UT))
    mac_pno_sched_scan_mgmt_stru    *pst_pno_mgmt;

    pst_pno_mgmt = (mac_pno_sched_scan_mgmt_stru *)p_arg;

    /* �����ʱ�������ڣ�ֱ�ӷ��� */
    if (OAL_PTR_NULL == pst_pno_mgmt->p_pno_sched_scan_timer)
    {
        OAM_WARNING_LOG0(0, OAM_SF_SCAN,
                         "{dmac_scan_stop_pno_sched_scan_timer:: pno sched timer not create yet.}");
        return OAL_SUCC;
    }
    /* ɾ����ʱ�� */
    oal_rtctimer_delete((STIMER_STRU *)pst_pno_mgmt->p_pno_sched_scan_timer);
    pst_pno_mgmt->p_pno_sched_scan_timer = OAL_PTR_NULL;
#else
    /* 1151��֧�֣��Ҳ����ߵ��˴���do nothing����Ҫԭ��ƽ̨��δ��װ��ʱ����ؽӿ� */
#endif

    return OAL_SUCC;
}


oal_uint32  dmac_scan_proc_sched_scan_req_event(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru             *pst_event;
    frw_event_hdr_stru         *pst_event_hdr;
    dmac_vap_stru              *pst_dmac_vap;
    mac_device_stru            *pst_mac_device;
    mac_scan_req_stru           st_scan_req_params;

    /* �����Ϸ��Լ�� */
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{dmac_scan_proc_sched_scan_req_event::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡ�¼���Ϣ */
    pst_event        = frw_get_event_stru(pst_event_mem);
    pst_event_hdr    = &(pst_event->st_event_hdr);

    /* ��ȡdmac vap */
    pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_event_hdr->uc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dmac_vap))
    {
        OAM_ERROR_LOG0(pst_event_hdr->uc_vap_id, OAM_SF_SCAN, "{dmac_scan_proc_sched_scan_req_event::pst_dmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡmac device */
    pst_mac_device = mac_res_get_dev(pst_event_hdr->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(pst_event_hdr->uc_vap_id, OAM_SF_SCAN, "{dmac_scan_proc_sched_scan_req_event::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ׼��pnoɨ��������� */
    dmac_scan_prepare_pno_scan_params(&st_scan_req_params, pst_dmac_vap);

    /* ����PNO����ɨ��Ĵ���Ϊ0 */
    pst_mac_device->pst_pno_sched_scan_mgmt->uc_curr_pno_sched_scan_times = 0;
    pst_mac_device->pst_pno_sched_scan_mgmt->en_is_found_match_ssid = OAL_FALSE;

    /* ����ɨ����ڣ�ִ��ɨ�� */
    dmac_scan_handle_scan_req_entry(pst_mac_device, pst_dmac_vap, &st_scan_req_params);

    /* ����pno����ɨ���rtcʱ�Ӷ�ʱ�����ɻ�����˯��device */
    dmac_scan_start_pno_sched_scan_timer((void *)pst_dmac_vap);

    return OAL_SUCC;
}


oal_uint32  dmac_scan_proc_scan_req_event(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru             *pst_event;
    frw_event_hdr_stru         *pst_event_hdr;
    dmac_vap_stru              *pst_dmac_vap;
    mac_device_stru            *pst_mac_device;
    mac_scan_req_stru          *pst_h2d_scan_req_params;

    /* �����Ϸ��Լ�� */
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{dmac_scan_proc_scan_req_event::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡ�¼���Ϣ */
    pst_event        = frw_get_event_stru(pst_event_mem);
    pst_event_hdr    = &(pst_event->st_event_hdr);

    pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_event_hdr->uc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dmac_vap))
    {
        OAM_ERROR_LOG0(pst_event_hdr->uc_vap_id, OAM_SF_SCAN, "{dmac_scan_proc_scan_req_event::pst_dmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡmac device */
    pst_mac_device = mac_res_get_dev(pst_event_hdr->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(pst_event_hdr->uc_vap_id, OAM_SF_SCAN, "{dmac_scan_proc_scan_req_event::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡ��ɨ��������� */
    pst_h2d_scan_req_params = (mac_scan_req_stru *)frw_get_event_payload(pst_event_mem);

    /* �������macɨ�迪�أ�����dmac��������ɨ�� */
    pst_mac_device->en_is_random_mac_addr_scan = pst_h2d_scan_req_params->en_is_random_mac_addr_scan;

    /* host�෢���ɨ������Ĵ��� */
    return dmac_scan_handle_scan_req_entry(pst_mac_device, pst_dmac_vap, pst_h2d_scan_req_params);
}

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST


OAL_STATIC oal_uint32 dmac_scan_prepare_obss_scan_params(mac_scan_req_stru  *pst_scan_params,
                                                         dmac_vap_stru      *pst_dmac_vap)
{
    mac_device_stru *pst_mac_device;
    oal_uint8        uc_2g_chan_num      = 0;
    oal_uint8        uc_channel_idx      = 0;
    oal_uint8        uc_low_channel_idx  = 0;
    oal_uint8        uc_high_channel_idx = 0;
    oal_uint8        uc_channel_num      = 0;
    oal_uint8        uc_curr_channel_num = pst_dmac_vap->st_vap_base_info.st_channel.uc_chan_number;
    oal_uint8        uc_curr_band        = pst_dmac_vap->st_vap_base_info.st_channel.en_band;

    /* ��ȡmac device */
    pst_mac_device = mac_res_get_dev(pst_dmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
                       "{dmac_scan_prepare_obss_scan_params::mac_res_get_dev failed.}");
        return OAL_FAIL;
    }

    OAL_MEMZERO(pst_scan_params, OAL_SIZEOF(mac_scan_req_stru));

    /* 1.���÷���ɨ���vap id */
    pst_scan_params->uc_vap_id = pst_dmac_vap->st_vap_base_info.uc_vap_id;

    /* 2.���ó�ʼɨ������Ĳ��� */
    pst_scan_params->en_bss_type         = WLAN_MIB_DESIRED_BSSTYPE_INFRA;
    pst_scan_params->en_scan_type        = WLAN_SCAN_TYPE_ACTIVE;
    pst_scan_params->uc_probe_delay      = 0;
    pst_scan_params->uc_scan_func        = MAC_SCAN_FUNC_BSS;               /* Ĭ��ɨ��bss */
    pst_scan_params->uc_max_send_probe_req_count_per_channel = WLAN_DEFAULT_SEND_PROBE_REQ_COUNT_PER_CHANNEL;
    pst_scan_params->uc_max_scan_count_per_channel           = 1;

    /* ����ɨ�����ͣ�ȷ��ÿ�ŵ�ɨ��ʱ�� */
    if (WLAN_SCAN_TYPE_ACTIVE == pst_scan_params->en_scan_type)
    {
        pst_scan_params->us_scan_time = WLAN_DEFAULT_ACTIVE_SCAN_TIME;
    }
    else
    {
        pst_scan_params->us_scan_time = WLAN_DEFAULT_PASSIVE_SCAN_TIME;
    }

    /* OBSSɨ��ͨ��ssid */
    pst_scan_params->ast_mac_ssid_set[0].auc_ssid[0] = '\0';
    pst_scan_params->uc_ssid_num = 1;
    /* OBSSɨ������Source MAC ADDRESS */
    if((pst_mac_device->en_is_random_mac_addr_scan)
       && ((pst_mac_device->auc_mac_oui[0] != 0) || (pst_mac_device->auc_mac_oui[1] != 0) || (pst_mac_device->auc_mac_oui[2] != 0)))
    {
        pst_scan_params->auc_sour_mac_addr[0] = (pst_mac_device->auc_mac_oui[0] & 0xfe);  /*��֤�ǵ���mac*/
        pst_scan_params->auc_sour_mac_addr[1] = pst_mac_device->auc_mac_oui[1];
        pst_scan_params->auc_sour_mac_addr[2] = pst_mac_device->auc_mac_oui[2];
        pst_scan_params->auc_sour_mac_addr[3] = oal_gen_random((oal_uint32)OAL_TIME_GET_STAMP_MS(), 1);
        pst_scan_params->auc_sour_mac_addr[4] = oal_gen_random((oal_uint32)OAL_TIME_GET_STAMP_MS(), 1);
        pst_scan_params->auc_sour_mac_addr[5] = oal_gen_random((oal_uint32)OAL_TIME_GET_STAMP_MS(), 1);
        pst_scan_params->en_is_random_mac_addr_scan = OAL_TRUE;
    }
    else
    {
        oal_set_mac_addr(pst_scan_params->auc_sour_mac_addr, mac_mib_get_StationID(&pst_dmac_vap->st_vap_base_info));
    }

    /* OBSSɨ��ɨ��ָֻ��1��bssid��Ϊ�㲥��ַ */
    oal_set_mac_addr(pst_scan_params->auc_bssid[0], BROADCAST_MACADDR);
    pst_scan_params->uc_bssid_num = 1;

    /* ����ɨ��ģʽΪOBSSɨ�� */
    pst_scan_params->en_scan_mode = WLAN_SCAN_MODE_BACKGROUND_OBSS;

    /* ׼��OBSSɨ����ŵ� */
    if (WLAN_BAND_2G == uc_curr_band)
    {

        /* �ӵ�ǰ�ŵ�����ƫ��5���ŵ�������OBSSɨ���ŵ�
           1) ��ǰ�ŵ�idxС�ڵ���5�����0��ʼ����idx+5,
           2) ����5С��8��Ӧ�ô�idx-5��idx+5,
           3) ����8�����Ǵ�idx-5��13 */
        if (uc_curr_channel_num <= 5)
        {
            uc_low_channel_idx = 0;
            uc_high_channel_idx = uc_curr_channel_num + 5;
        }
        else if (5 < uc_curr_channel_num && uc_curr_channel_num <= 8)
        {
            uc_low_channel_idx  = uc_curr_channel_num - 5;
            uc_high_channel_idx = uc_curr_channel_num + 5;
        }
        else if (8 < uc_curr_channel_num && uc_curr_channel_num <= 13)
        {
            uc_low_channel_idx = uc_curr_channel_num - 5;
            uc_high_channel_idx = 13;
        }
        else
        {
            uc_low_channel_idx  = 0;
            uc_high_channel_idx = 0;
            OAM_ERROR_LOG1(0, OAM_SF_SCAN, "{dmac_scan_update_obss_scan_params::2040M,Current channel index is %d.}",
                           uc_curr_channel_num);
        }

        /* ׼��2.4G��OBSSɨ���ŵ� */
        for(uc_channel_idx = uc_low_channel_idx; uc_channel_idx <= uc_high_channel_idx; uc_channel_idx++)
        {
            /* �ж��ŵ��ǲ����ڹ������� */
            if (OAL_SUCC == mac_is_channel_idx_valid(WLAN_BAND_2G, uc_channel_idx))
            {
                mac_get_channel_num_from_idx(WLAN_BAND_2G, uc_channel_idx, &uc_channel_num);

                pst_scan_params->ast_channel_list[uc_2g_chan_num].uc_chan_number = uc_channel_num;
                pst_scan_params->ast_channel_list[uc_2g_chan_num].en_band        = WLAN_BAND_2G;
                pst_scan_params->ast_channel_list[uc_2g_chan_num].uc_chan_idx         = uc_channel_idx;
                pst_scan_params->uc_channel_nums++;
                uc_2g_chan_num++;
            }
        }

        /* ���±���ɨ����ŵ����� */
        pst_scan_params->uc_channel_nums = uc_2g_chan_num;
    }
#if 0
    else if (WLAN_BAND_5G == uc_curr_band)
    {
        /* ��ʱ������5G�µ�obssɨ�� */
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{dmac_scan_update_obss_scan_params::5G don't do obss scan.}");
    }
    else
    {
        OAM_ERROR_LOG1(0, OAM_SF_SCAN, "{dmac_scan_update_obss_scan_params::error band[%d].}", uc_curr_band);
    }
#endif
    /* �����ǰɨ���ŵ�������Ϊ0�����ش��󣬲�ִ��ɨ������ */
    if (0 == pst_scan_params->uc_channel_nums)
    {
        OAM_ERROR_LOG1(0, OAM_SF_SCAN, "{dmac_scan_update_obss_scan_params::scan total channel num is 0, band[%d]!}", uc_curr_band);
        return OAL_FAIL;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  dmac_scan_obss_timeout_fn(void *p_arg)
{
    dmac_vap_stru          *pst_dmac_vap;
    mac_device_stru        *pst_mac_device;
    mac_scan_req_stru       st_scan_req_params;
    oal_uint32              ul_ret;
    oal_uint8               uc_scan_now = OAL_FALSE;

    //OAM_INFO_LOG0(0, OAM_SF_SCAN, "{dmac_scan_obss_timeout_fn::obss timer time out.}");

    pst_dmac_vap = (dmac_vap_stru *)p_arg;
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dmac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{dmac_scan_obss_timeout_fn::pst_dmac_vap null.}");
        return OAL_FAIL;
    }

    /* ��ȡmac device */
    pst_mac_device = mac_res_get_dev(pst_dmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
                       "{dmac_scan_obss_timeout_fn::pst_mac_device null.}");
        return OAL_FAIL;
    }

    // �����¼�����
    if (pst_mac_device->st_obss_scan_timer.p_timeout_arg != (oal_void *)pst_dmac_vap)
    {
        return OAL_FAIL;
    }

    /* �����obss scan timer�����ڼ䶯̬�޸���sta��������sta��֧��obssɨ�裬
     * ��ر�obss scan timer
     * �˴���Ψһ��ֹobssɨ��ĵط�!!!
     */
    if (OAL_FALSE == dmac_mgmt_need_obss_scan(&pst_dmac_vap->st_vap_base_info))
    {
        pst_dmac_vap->ul_obss_scan_timer_remain  = 0;
        pst_dmac_vap->uc_obss_scan_timer_started = OAL_FALSE;
        //OAM_INFO_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
        //            "{dmac_scan_obss_timeout_fn::stop obss scan timer}");
        return OAL_SUCC;
    }

    if (0 == pst_dmac_vap->ul_obss_scan_timer_remain)
    {
        uc_scan_now = OAL_TRUE;
    }

    dmac_scan_start_obss_timer(&pst_dmac_vap->st_vap_base_info);

    /* ����ɨ����ڣ�ִ��obssɨ�� */
    if (OAL_TRUE == uc_scan_now)
    {
        /* ׼��OBSSɨ�������׼������ɨ�� */
        ul_ret = dmac_scan_prepare_obss_scan_params(&st_scan_req_params, pst_dmac_vap);
        if (OAL_SUCC != ul_ret)
        {
            //OAM_WARNING_LOG1(0, OAM_SF_SCAN, "{dmac_scan_obss_timeout_fn::update scan params error[%d].}", ul_ret);
            return ul_ret;
        }

        OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
                         "{dmac_scan_obss_timeout_fn:: start scan}");
        return dmac_scan_handle_scan_req_entry(pst_mac_device, pst_dmac_vap, &st_scan_req_params);
    }

    return OAL_SUCC;
}


oal_void dmac_scan_start_obss_timer(mac_vap_stru *pst_mac_vap)
{
    dmac_vap_stru                 *pst_dmac_vap;
    mac_device_stru               *pst_mac_device;
    oal_uint32                    ul_new_timer;

    /* ���ݷ���ɨ���vap id��ȡdmac vap */
    pst_dmac_vap = mac_res_get_dmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_WARNING_LOG0(0, OAM_SF_SCAN, "{dmac_scan_start_obss_timer:: pst_dmac_vap is NULL.}");
        return;
    }

    /* ��ȡmac device */
    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_SCAN,
                       "{dmac_scan_start_obss_timer::pst_mac_device null.}");
        return;
    }

    /* ����ɨ�趨ʱ�� */
    if (0 == pst_dmac_vap->ul_obss_scan_timer_remain)
    {
        pst_dmac_vap->ul_obss_scan_timer_remain = 1000*mac_mib_get_BSSWidthTriggerScanInterval(&pst_dmac_vap->st_vap_base_info);
    }

    ul_new_timer = pst_dmac_vap->ul_obss_scan_timer_remain > DMAC_SCAN_MAX_TIMER?
                        DMAC_SCAN_MAX_TIMER:pst_dmac_vap->ul_obss_scan_timer_remain;
    pst_dmac_vap->ul_obss_scan_timer_remain -= ul_new_timer;
    OAM_INFO_LOG2(0, OAM_SF_SCAN, "{dmac_scan_start_obss_timer::remain=%d new_timer=%d}",
                pst_dmac_vap->ul_obss_scan_timer_remain, ul_new_timer);

    FRW_TIMER_CREATE_TIMER(&(pst_mac_device->st_obss_scan_timer),
                           dmac_scan_obss_timeout_fn,
                           ul_new_timer,
                           (void *)pst_dmac_vap,
                           OAL_FALSE,
                           OAM_MODULE_ID_DMAC,
                           pst_mac_device->ul_core_id);
    pst_dmac_vap->uc_obss_scan_timer_started = OAL_TRUE;

    return;
}

oal_void dmac_scan_destroy_obss_timer(dmac_vap_stru *pst_dmac_vap)
{
    mac_vap_stru        *pst_mac_vap;
    mac_device_stru     *pst_mac_device;

    pst_mac_vap = &(pst_dmac_vap->st_vap_base_info);

    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (!pst_mac_device)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_scan_destroy_obss_timer:null dev ptr, id=%d}", pst_mac_vap->uc_device_id);

        return;
    }

    if ((pst_mac_device->st_obss_scan_timer.en_is_registerd)
        && pst_mac_device->st_obss_scan_timer.p_timeout_arg == (void *)pst_mac_vap) // same as dmac_vap
    {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&pst_mac_device->st_obss_scan_timer);
        pst_mac_device->st_obss_scan_timer.p_timeout_arg = OAL_PTR_NULL;
        pst_dmac_vap->uc_obss_scan_timer_started = OAL_FALSE;
    }
}

oal_uint32 dmac_trigger_csa_scan(mac_vap_stru      *pst_mac_vap)
{
    oal_uint8               uc_chan_num      = 0;
    oal_uint8               uc_channel_idx      = 0;
    dmac_vap_stru          *pst_dmac_vap;
    mac_device_stru        *pst_mac_device;
    mac_scan_req_stru       st_scan_params;

    if((pst_mac_vap->st_ch_switch_info.st_old_channel.uc_chan_number == pst_mac_vap->st_channel.uc_chan_number))
    {
        dmac_sta_csa_fsm_post_event(pst_mac_vap, WLAN_STA_CSA_EVENT_TO_INIT, 0, OAL_PTR_NULL);
        return OAL_SUCC;
    }

    OAM_WARNING_LOG0(0, OAM_SF_SCAN, "{dmac_trigger_csa_scan::update csa scan params.}");

    OAL_MEMZERO(&st_scan_params, OAL_SIZEOF(mac_scan_req_stru));

    /* 1.���÷���ɨ���vap id */
    st_scan_params.uc_vap_id = pst_mac_vap->uc_vap_id;

    /* 2.���ó�ʼɨ������Ĳ��� */
    st_scan_params.en_bss_type         = WLAN_MIB_DESIRED_BSSTYPE_INFRA;
    st_scan_params.en_scan_type        = WLAN_SCAN_TYPE_ACTIVE;
    st_scan_params.uc_probe_delay      = 0;
    st_scan_params.uc_scan_func        = MAC_SCAN_FUNC_BSS;               /* Ĭ��ɨ��bss */
    st_scan_params.uc_max_send_probe_req_count_per_channel = WLAN_DEFAULT_SEND_PROBE_REQ_COUNT_PER_CHANNEL;
    st_scan_params.uc_max_scan_count_per_channel           = 2;

    /* ����ɨ�����ͣ�ȷ��ÿ�ŵ�ɨ��ʱ�� */
    if (WLAN_SCAN_TYPE_ACTIVE == st_scan_params.en_scan_type)
    {
        st_scan_params.us_scan_time = WLAN_DEFAULT_ACTIVE_SCAN_TIME;
    }

    /* CSAɨ��ͨ��ssid */
    st_scan_params.ast_mac_ssid_set[0].auc_ssid[0] = '\0';
    st_scan_params.uc_ssid_num = 1;
    /* CSAɨ������Source MAC ADDRESS */
    oal_set_mac_addr(st_scan_params.auc_sour_mac_addr, mac_mib_get_StationID(pst_mac_vap));

    /* CSAɨ��ɨ��ָֻ��1��bssid��Ϊ�㲥��ַ */
    oal_set_mac_addr(st_scan_params.auc_bssid[0], BROADCAST_MACADDR);
    st_scan_params.uc_bssid_num = 1;

    /* ����ɨ��ģʽΪCSAɨ�� */
    st_scan_params.en_scan_mode = WLAN_SCAN_MODE_BACKGROUND_CSA;

    /* ׼��ɨ����ŵ� */
    /* �ж��ŵ��ǲ����ڹ������� */

    if (OAL_SUCC == mac_is_channel_num_valid(pst_mac_vap->st_ch_switch_info.st_old_channel.en_band, pst_mac_vap->st_ch_switch_info.st_old_channel.uc_chan_number))
    {
        mac_get_channel_idx_from_num(pst_mac_vap->st_ch_switch_info.st_old_channel.en_band, pst_mac_vap->st_ch_switch_info.st_old_channel.uc_chan_number, &uc_channel_idx);
        st_scan_params.ast_channel_list[uc_chan_num].uc_chan_number = pst_mac_vap->st_ch_switch_info.st_old_channel.uc_chan_number;
        st_scan_params.ast_channel_list[uc_chan_num].en_band        = pst_mac_vap->st_ch_switch_info.st_old_channel.en_band;
        st_scan_params.ast_channel_list[uc_chan_num].uc_chan_idx         = uc_channel_idx;
        st_scan_params.uc_channel_nums++;
        uc_chan_num++;
    }

    if (OAL_SUCC == mac_is_channel_num_valid(pst_mac_vap->st_channel.en_band, pst_mac_vap->st_channel.uc_chan_number))
    {
        mac_get_channel_idx_from_num(pst_mac_vap->st_channel.en_band, pst_mac_vap->st_channel.uc_chan_number, &uc_channel_idx);
        st_scan_params.ast_channel_list[uc_chan_num].uc_chan_number = pst_mac_vap->st_channel.uc_chan_number;
        st_scan_params.ast_channel_list[uc_chan_num].en_band        = pst_mac_vap->st_channel.en_band;
        st_scan_params.ast_channel_list[uc_chan_num].uc_chan_idx         = uc_channel_idx;
        st_scan_params.uc_channel_nums++;
        uc_chan_num++;
    }


    /* ���±���ɨ����ŵ����� */
    st_scan_params.uc_channel_nums = uc_chan_num;

    /* �����ǰɨ���ŵ�������Ϊ0�����ش��󣬲�ִ��ɨ������ */
    if (0 == st_scan_params.uc_channel_nums)
    {
       OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{dmac_trigger_csa_scan::scan total channel num is 0.}");
       return OAL_FAIL;
    }

    pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dmac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{dmac_trigger_csa_scan::pst_dmac_vap null.}");
        return OAL_FAIL;
    }

    /* ��ȡmac device */
    pst_mac_device = mac_res_get_dev(pst_dmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
                       "{dmac_trigger_csa_scan::pst_mac_device null.}");
        return OAL_FAIL;
    }

    dmac_scan_handle_scan_req_entry(pst_mac_device, pst_dmac_vap, &st_scan_params);

    return OAL_SUCC;
}

#endif


OAL_STATIC oal_void  dmac_scan_switch_channel_notify_alg(hal_to_dmac_device_stru  *pst_scan_hal_device,
                                                         dmac_vap_stru     *pst_dmac_vap,
                                                         mac_channel_stru  *pst_channel)
{
    mac_channel_stru                st_channel_tmp;
    hal_to_dmac_device_stru        *pst_original_hal_device;

    /* �����Ϸ��Լ�� */
    if ((OAL_PTR_NULL == pst_dmac_vap) || (OAL_PTR_NULL == pst_channel) || (OAL_PTR_NULL == pst_scan_hal_device))
    {
        OAM_ERROR_LOG3(0, OAM_SF_SCAN, "{dmac_scan_switch_channel_notify_alg::pst_dmac_vap[%p], pst_channel[%p].pst_scan_hal_device[%p]}",
                       pst_dmac_vap, pst_channel, pst_scan_hal_device);
        return;
    }

    /* ��¼��ǰvap�µ��ŵ���Ϣ */
    st_channel_tmp = pst_dmac_vap->st_vap_base_info.st_channel;

    /* ��ͬ��hal deviceɨ��,֪ͨ�㷨��ͬ��ɨ��hal device */
    pst_original_hal_device   = pst_dmac_vap->pst_hal_device;
    pst_dmac_vap->pst_hal_device = pst_scan_hal_device;

    /* ��¼Ҫ�л����ŵ���Ƶ�Σ��л��ŵ� */
    pst_dmac_vap->st_vap_base_info.st_channel.en_band        = pst_channel->en_band;
    pst_dmac_vap->st_vap_base_info.st_channel.uc_chan_number = pst_channel->uc_chan_number;
    pst_dmac_vap->st_vap_base_info.st_channel.uc_chan_idx         = pst_channel->uc_chan_idx;
    pst_dmac_vap->st_vap_base_info.st_channel.en_bandwidth = WLAN_BAND_WIDTH_20M;

    /* ˢ�·��͹��� */
    dmac_pow_set_vap_tx_power(&pst_dmac_vap->st_vap_base_info, HAL_POW_SET_TYPE_INIT);

    /* ֪ͨ�㷨 */
    dmac_alg_cfg_channel_notify(&pst_dmac_vap->st_vap_base_info, CH_BW_CHG_TYPE_SCAN);
    dmac_alg_cfg_bandwidth_notify(&pst_dmac_vap->st_vap_base_info, CH_BW_CHG_TYPE_SCAN);

    /* ֪ͨ�㷨�󣬻ָ�vapԭ���ŵ���Ϣ */
    pst_dmac_vap->st_vap_base_info.st_channel = st_channel_tmp;
    pst_dmac_vap->pst_hal_device = pst_original_hal_device;

    return;
}


oal_uint32  dmac_switch_channel_off(
                mac_device_stru     *pst_mac_device,
                mac_vap_stru        *pst_mac_vap,
                mac_channel_stru    *pst_dst_chl,
                oal_uint16           us_protect_time)
{
    mac_fcs_mgr_stru               *pst_fcs_mgr;
    mac_fcs_cfg_stru               *pst_fcs_cfg;
    hal_to_dmac_device_stru        *pst_hal_device;

    pst_hal_device = DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap);
    if (OAL_PTR_NULL == pst_hal_device)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{dmac_switch_channel_off::vap id [%d],DMAC_VAP_GET_HAL_DEVICE null}",pst_mac_vap->uc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��¼������ŵ�����ɨ������л� */
    pst_hal_device->st_hal_scan_params.st_home_channel = pst_mac_vap->st_channel;

    pst_fcs_mgr = dmac_fcs_get_mgr_stru(pst_mac_device);
    pst_fcs_cfg = MAC_DEV_GET_FCS_CFG(pst_mac_device);

    OAL_MEMZERO(pst_fcs_cfg, OAL_SIZEOF(mac_fcs_cfg_stru));

    pst_fcs_cfg->st_src_chl = pst_mac_vap->st_channel;
    pst_fcs_cfg->st_dst_chl = *pst_dst_chl;

    pst_fcs_cfg->pst_hal_device = pst_hal_device;

    pst_fcs_cfg->pst_src_fake_queue = DMAC_VAP_GET_FAKEQ(pst_mac_vap);

    dmac_fcs_prepare_one_packet_cfg(pst_mac_vap, &(pst_fcs_cfg->st_one_packet_cfg), us_protect_time);

    /* ����FCS���ŵ��ӿ� ���浱ǰӲ�����е�֡���Լ�����ٶ���,����DBAC����Ҫ�л����� */
    dmac_fcs_start(pst_fcs_mgr, pst_fcs_cfg, 0);
    mac_fcs_release(pst_fcs_mgr);

    OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{dmac_switch_channel_off::switch src channel[%d] to dst channel[%d].}",
                 pst_mac_vap->st_channel.uc_chan_number, pst_fcs_cfg->st_dst_chl.uc_chan_number);

    return OAL_SUCC;
}
#ifdef _PRE_WLAN_FEATURE_DBAC

oal_void dmac_dbac_switch_channel_off(mac_device_stru  *pst_mac_device,
                                                mac_vap_stru   *pst_mac_vap1,
                                                mac_vap_stru   *pst_mac_vap2,
                                                mac_channel_stru  *pst_dst,
                                                oal_uint16  us_protect_time)
{
    mac_fcs_mgr_stru               *pst_fcs_mgr;
    mac_fcs_cfg_stru               *pst_fcs_cfg;
    hal_to_dmac_device_stru        *pst_hal_device;
    hal_to_dmac_device_stru        *pst_hal_device2;
    mac_vap_stru                   *pst_current_chan_vap;

    pst_hal_device = DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap1);
    if (OAL_PTR_NULL == pst_hal_device)
    {
        OAM_ERROR_LOG1(pst_mac_vap1->uc_vap_id, OAM_SF_DBAC, "{dmac_dbac_switch_channel_off::vap id [%d],DMAC_VAP_GET_HAL_DEVICE null}",pst_mac_vap1->uc_vap_id);
        return;
    }

    pst_hal_device2 = DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap2);
    if (OAL_PTR_NULL == pst_hal_device2)
    {
        OAM_ERROR_LOG1(pst_mac_vap2->uc_vap_id, OAM_SF_DBAC, "{dmac_dbac_switch_channel_off::vap id [%d],DMAC_VAP_GET_HAL_DEVICE null}",pst_mac_vap2->uc_vap_id);
        return;
    }

    if (pst_hal_device != pst_hal_device2)
    {
        OAM_ERROR_LOG2(0, OAM_SF_DBAC, "{dmac_dbac_switch_channel_off::diff pst_hal_device id [%d] [%d].}", pst_hal_device->uc_device_id, pst_hal_device2->uc_device_id);
        return;
    }

    if (pst_hal_device->uc_current_chan_number == pst_mac_vap1->st_channel.uc_chan_number)
    {
        pst_current_chan_vap = pst_mac_vap1;
    }
    else
    {
        pst_current_chan_vap = pst_mac_vap2;
    }

    /* ��ͣDBAC���ŵ� */
    dmac_alg_dbac_pause(pst_hal_device);

    dmac_vap_pause_tx(pst_mac_vap1);
    dmac_vap_pause_tx(pst_mac_vap2);

    pst_fcs_mgr = dmac_fcs_get_mgr_stru(pst_mac_device);
    pst_fcs_cfg = MAC_DEV_GET_FCS_CFG(pst_mac_device);
    OAL_MEMZERO(pst_fcs_cfg, OAL_SIZEOF(mac_fcs_cfg_stru));

    pst_fcs_cfg->st_dst_chl     = *pst_dst;
    pst_fcs_cfg->st_src_chl = pst_current_chan_vap->st_channel;
    pst_fcs_cfg->pst_hal_device = pst_hal_device;

    pst_fcs_cfg->pst_src_fake_queue = DMAC_VAP_GET_FAKEQ(pst_current_chan_vap);

    dmac_fcs_prepare_one_packet_cfg(pst_current_chan_vap, &(pst_fcs_cfg->st_one_packet_cfg), us_protect_time);

    OAM_WARNING_LOG2(pst_current_chan_vap->uc_vap_id, OAM_SF_DBAC, "dmac_dbac_switch_channel_off::switch chan off when dbac running. curr chan num:%d, fake_q_vap_id:%d",
                    pst_current_chan_vap->st_channel.uc_chan_number, pst_current_chan_vap->uc_vap_id);


    if (pst_hal_device->uc_current_chan_number != pst_current_chan_vap->st_channel.uc_chan_number)
    {
        OAM_WARNING_LOG2(0, OAM_SF_DBAC, "dmac_dbac_switch_channel_off::switch chan off when dbac running. hal chan num:%d, curr vap chan num:%d. not same,do not send protect frame",
                        pst_hal_device->uc_current_chan_number,
                        pst_current_chan_vap->st_channel.uc_chan_number);

        pst_fcs_cfg->st_one_packet_cfg.en_protect_type = HAL_FCS_PROTECT_TYPE_NONE;
    }

    dmac_fcs_start(pst_fcs_mgr, pst_fcs_cfg, 0);
    mac_fcs_release(pst_fcs_mgr);
}

OAL_STATIC oal_void dmac_scan_dbac_switch_channel_off(mac_device_stru  *pst_mac_device,
                                                      mac_vap_stru *pst_mac_vap,
                                                      mac_vap_stru *pst_mac_vap2)
{
    oal_uint8                       uc_scan_chan_idx;
    oal_uint16                      us_protect_time;
    hal_to_dmac_device_stru        *pst_hal_device;

    pst_hal_device = DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap);
    uc_scan_chan_idx = GET_HAL_DEV_CURRENT_SCAN_IDX(pst_hal_device);
    us_protect_time = pst_mac_device->st_scan_params.us_scan_time;

    dmac_dbac_switch_channel_off(pst_mac_device, pst_mac_vap,pst_mac_vap2,
                                 &(pst_mac_device->st_scan_params.ast_channel_list[uc_scan_chan_idx]),
                                 us_protect_time);
}

#endif

OAL_STATIC oal_uint32 dmac_the_same_channel_switch_channel_off(
                mac_device_stru     *pst_mac_device,
                mac_vap_stru        *pst_mac_vap1,
                mac_vap_stru        *pst_mac_vap2,
                mac_channel_stru    *pst_dst_chl,
                oal_uint16           us_protect_time)
{
    mac_vap_stru                   *pst_vap_sta;
    mac_fcs_mgr_stru               *pst_fcs_mgr;
    mac_fcs_cfg_stru               *pst_fcs_cfg;
    hal_to_dmac_device_stru        *pst_hal_device;
    hal_to_dmac_device_stru        *pst_hal_device2;

    pst_hal_device = DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap1);
    if (OAL_PTR_NULL == pst_hal_device)
    {
        OAM_ERROR_LOG1(pst_mac_vap1->uc_vap_id, OAM_SF_SCAN, "{dmac_the_same_channel_switch_channel_off::vap id [%d],DMAC_VAP_GET_HAL_DEVICE null}",
                            pst_mac_vap1->uc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hal_device2 = DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap2);
    if (OAL_PTR_NULL == pst_hal_device2)
    {
        OAM_ERROR_LOG1(pst_mac_vap1->uc_vap_id, OAM_SF_SCAN, "{dmac_the_same_channel_switch_channel_off::vap id [%d],DMAC_VAP_GET_HAL_DEVICE null}",
                            pst_mac_vap2->uc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (pst_hal_device != pst_hal_device2)
    {
        OAM_ERROR_LOG2(0, OAM_SF_SCAN, "{dmac_the_same_channel_switch_channel_off::diff pst_hal_device id [%d] [%d].}",
                            pst_hal_device->uc_device_id, pst_hal_device2->uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (pst_mac_vap1->st_channel.uc_chan_number != pst_mac_vap2->st_channel.uc_chan_number)
    {
        OAM_ERROR_LOG2(0, OAM_SF_SCAN, "dmac_the_same_channel_switch_channel_off::vap1 channel num[%d] != vap2 channel num[%d].",pst_mac_vap1->st_channel.uc_chan_number,
                    pst_mac_vap2->st_channel.uc_chan_number);
        return OAL_FAIL;
    }

    /* ��¼����ʱ��������ŵ�����ͬ�ŵ�����ɨ������л� */
    if (pst_mac_vap1->st_channel.en_bandwidth >= pst_mac_vap2->st_channel.en_bandwidth)
    {
        pst_hal_device->st_hal_scan_params.st_home_channel = pst_mac_vap1->st_channel;
    }
    else
    {
        pst_hal_device->st_hal_scan_params.st_home_channel = pst_mac_vap2->st_channel;
    }

    /* ��ͣ����VAP�ķ��� */
    dmac_vap_pause_tx(pst_mac_vap1);
    dmac_vap_pause_tx(pst_mac_vap2);

    pst_fcs_mgr = dmac_fcs_get_mgr_stru(pst_mac_device);
    pst_fcs_cfg = MAC_DEV_GET_FCS_CFG(pst_mac_device);
    OAL_MEMZERO(pst_fcs_cfg, OAL_SIZEOF(mac_fcs_cfg_stru));

    pst_fcs_cfg->st_dst_chl = *pst_dst_chl;
    pst_fcs_cfg->pst_hal_device = pst_hal_device;

    pst_fcs_cfg->pst_src_fake_queue = DMAC_VAP_GET_FAKEQ(pst_mac_vap1);

    OAM_WARNING_LOG4(0, OAM_SF_SCAN, "{dmac_the_same_channel_switch_channel::hal device[%d], curr hal chan[%d], dst channel[%d], fakeq vap id[%d]}",
                        pst_hal_device->uc_device_id,pst_hal_device->uc_current_chan_number,
                        pst_fcs_cfg->st_dst_chl.uc_chan_number, pst_mac_vap1->uc_vap_id);

    /* ͬƵ˫STAģʽ����Ҫ������one packet */
    if (WLAN_VAP_MODE_BSS_STA == pst_mac_vap1->en_vap_mode && WLAN_VAP_MODE_BSS_STA == pst_mac_vap2->en_vap_mode)
    {
        /* ׼��VAP1��fcs���� */
        pst_fcs_cfg->st_src_chl = pst_mac_vap1->st_channel;
        dmac_fcs_prepare_one_packet_cfg(pst_mac_vap1, &(pst_fcs_cfg->st_one_packet_cfg), us_protect_time);

        /* ׼��VAP2��fcs���� */
        pst_fcs_cfg->st_src_chl2 = pst_mac_vap2->st_channel;
        dmac_fcs_prepare_one_packet_cfg(pst_mac_vap2, &(pst_fcs_cfg->st_one_packet_cfg2), us_protect_time);
        pst_fcs_cfg->st_one_packet_cfg2.us_timeout = MAC_FCS_DEFAULT_PROTECT_TIME_OUT2;     /* ��С�ڶ���one packet�ı���ʱ�����Ӷ�������ʱ�� */
        pst_fcs_cfg->pst_hal_device = pst_hal_device;

        dmac_fcs_start_enhanced(pst_fcs_mgr, pst_fcs_cfg);
        mac_fcs_release(pst_fcs_mgr);
    }
    /* ͬƵSTA+GOģʽ��ֻ��ҪSTA��һ��one packet */
    else
    {
        if (WLAN_VAP_MODE_BSS_STA == pst_mac_vap1->en_vap_mode)
        {
            pst_vap_sta = pst_mac_vap1;
        }
        else
        {
            pst_vap_sta = pst_mac_vap2;
        }

        pst_fcs_cfg->st_src_chl = pst_vap_sta->st_channel;
        dmac_fcs_prepare_one_packet_cfg(pst_vap_sta, &(pst_fcs_cfg->st_one_packet_cfg), us_protect_time);

        /* ����FCS���ŵ��ӿ� ���浱ǰӲ�����е�֡��ɨ����ٶ��� */
        dmac_fcs_start(pst_fcs_mgr, pst_fcs_cfg, 0);
        mac_fcs_release(pst_fcs_mgr);
    }
    return OAL_SUCC;
}
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)

OAL_STATIC hal_to_dmac_device_stru *dmac_find_the_pause_scan_hal_dev(hal_to_dmac_device_stru *pst_hal_device1, hal_to_dmac_device_stru *pst_hal_device2)
{
    if (oal_bit_get_bit_one_byte(pst_hal_device1->st_hal_scan_params.uc_scan_pause_bitmap, HAL_SCAN_PASUE_TYPE_CHAN_CONFLICT))
    {
        return  pst_hal_device1;
    }
    else if (oal_bit_get_bit_one_byte(pst_hal_device2->st_hal_scan_params.uc_scan_pause_bitmap, HAL_SCAN_PASUE_TYPE_CHAN_CONFLICT))
    {
        return  pst_hal_device2;
    }

    return OAL_PTR_NULL;
}


oal_bool_enum_uint8 dmac_check_can_start_scan(mac_device_stru *pst_mac_device, hal_to_dmac_device_stru *pst_hal_device)
{
    oal_uint8                uc_scan_channel_idx;
    mac_channel_stru        *pst_scanning_channel;
    mac_channel_stru        *pst_another_scan_channel;
    hal_to_dmac_device_stru *pst_another_hal_device;
    hal_to_dmac_device_stru *pst_pause_hal_device;
    dmac_device_stru        *pst_dmac_device;
    oal_bool_enum_uint8      en_can_scan = OAL_TRUE;

    pst_dmac_device = dmac_res_get_mac_dev(pst_mac_device->uc_device_id);
    if (OAL_PTR_NULL == pst_dmac_device)
    {
        OAM_ERROR_LOG1(0, OAM_SF_SCAN, "{dmac_check_can_start_scan::dmac_res_get_mac_dev[%d] is null.}", pst_mac_device->uc_device_id);
        return en_can_scan;
    }

    /* �ǲ���ɨ�費��Ҫ������ɨ�� */
    if (OAL_FALSE == pst_dmac_device->en_is_fast_scan)
    {
        return en_can_scan;
    }

    uc_scan_channel_idx  = GET_HAL_DEV_CURRENT_SCAN_IDX(pst_hal_device);
    pst_scanning_channel = &(pst_mac_device->st_scan_params.ast_channel_list[uc_scan_channel_idx]);

    pst_another_hal_device = dmac_device_get_another_h2d_dev(pst_dmac_device, pst_hal_device);
    if (OAL_PTR_NULL == pst_another_hal_device)
    {
        OAM_ERROR_LOG1(0, OAM_SF_SCAN, "{dmac_check_can_start_scan::ori hal dev[%d]pst_another_hal_device is null.}", pst_hal_device->uc_device_id);
        return en_can_scan;
    }
    uc_scan_channel_idx = GET_HAL_DEV_CURRENT_SCAN_IDX(pst_another_hal_device);

    /* ��һ·�Ѿ���ɲ���Ҫ��ȥcheckֱ��ɨ�� */
    if (uc_scan_channel_idx >= pst_another_hal_device->st_hal_scan_params.uc_channel_nums)
    {
        return en_can_scan;
    }

    pst_another_scan_channel = &(pst_mac_device->st_scan_params.ast_channel_list[uc_scan_channel_idx]);

    /* ����ͬʱɨ��ʱ,����Ƿ���һ·��pause,�о�Ҫ�ָ� */
    if (OAL_TRUE == dmac_dbdc_channel_check(pst_scanning_channel, pst_another_scan_channel))
    {
        pst_pause_hal_device = dmac_find_the_pause_scan_hal_dev(pst_hal_device, pst_another_hal_device);
        if (pst_pause_hal_device)
        {
            hal_device_handle_event(pst_pause_hal_device, HAL_DEVICE_EVENT_SCAN_RESUME_FROM_CHAN_CONFLICT, 0, OAL_PTR_NULL);
        }
        return en_can_scan;
    }

    /* ����ɨ���5G�����Լ���ɨ,�ȴ�2.4G��ͻ�ŵ�ɨ���������� */
    if (WLAN_BAND_5G == pst_scanning_channel->en_band)
    {
        pst_pause_hal_device = pst_hal_device;
        en_can_scan = OAL_FALSE;
    }
    else
    {
        pst_pause_hal_device = pst_another_hal_device;
        en_can_scan = OAL_TRUE;
    }

    /* ��ͣ��Ҫ��ͣɨ���dev */
    if (!oal_bit_get_bit_one_byte(pst_pause_hal_device->st_hal_scan_params.uc_scan_pause_bitmap, HAL_SCAN_PASUE_TYPE_CHAN_CONFLICT))
    {
        hal_device_handle_event(pst_pause_hal_device, HAL_DEVICE_EVENT_SCAN_PAUSE_FROM_CHAN_CONFLICT, 0, OAL_PTR_NULL);
    }

    return en_can_scan;
}
#endif

oal_void dmac_scan_do_switch_channel_off(mac_device_stru *pst_mac_device, hal_to_dmac_device_stru *pst_hal_device, dmac_vap_stru *pst_dmac_vap)
{
    mac_channel_stru            *pst_next_scan_channel;
    oal_uint8                    uc_scan_channel_idx;
    oal_uint8                    uc_up_vap_num;
    oal_uint8                    uc_vap_idx;
    oal_uint8                    auc_mac_vap_id[WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE];
    mac_vap_stru                *pst_mac_vap[WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE] = {OAL_PTR_NULL};

    /* ��·ɨ����� */
    if (MAC_SCAN_STATE_IDLE == pst_hal_device->st_hal_scan_params.en_curr_scan_state)
    {
        OAM_ERROR_LOG2(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "dmac_scan_do_switch_channel_off::but this hal device[%d] is scan channel[%d]complete",
                            pst_hal_device->uc_device_id, pst_hal_device->st_hal_scan_params.uc_channel_nums);
        return;
    }

    uc_scan_channel_idx = GET_HAL_DEV_CURRENT_SCAN_IDX(pst_hal_device);
    if ((uc_scan_channel_idx > pst_mac_device->st_scan_params.uc_channel_nums) ||
            (pst_hal_device->st_hal_scan_params.uc_scan_chan_idx > pst_hal_device->st_hal_scan_params.uc_channel_nums))
    {
        OAM_ERROR_LOG4(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "dmac_scan_do_switch_channel_off::hal device id[%d]scan[%d]channel,total[%d]channel,now scan channel idx[%d],",
                    pst_hal_device->uc_device_id, uc_scan_channel_idx, pst_hal_device->st_hal_scan_params.uc_channel_nums,
                    pst_hal_device->st_hal_scan_params.uc_channel_nums);
    }

    pst_next_scan_channel = &(pst_mac_device->st_scan_params.ast_channel_list[uc_scan_channel_idx]);
    //if (pst_hal_device->st_hal_scan_params.uc_channel_nums <= DMAC_SCAN_CHANENL_NUMS_TO_PRINT_SWITCH_INFO)
    {
        OAM_WARNING_LOG2(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "dmac_scan_do_switch_channel_off::hal device[%d]switch channel to %d",
                                                    pst_hal_device->uc_device_id, pst_next_scan_channel->uc_chan_number);
    }

    dmac_scan_switch_channel_notify_alg(pst_hal_device, pst_dmac_vap, pst_next_scan_channel);

#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
    dmac_scan_check_2g_scan_results(pst_mac_device, pst_hal_device, &(pst_dmac_vap->st_vap_base_info), pst_next_scan_channel->en_band);
    pst_hal_device->st_hal_scan_params.uc_last_channel_band = pst_next_scan_channel->en_band;
#endif

    /* �ǹ���״̬����ֱ������,����Ҫ���� */
    if (GET_HAL_DEVICE_STATE(pst_hal_device) != HAL_DEVICE_WORK_STATE)
    {
        hal_device_handle_event(pst_hal_device, HAL_DEVICE_EVENT_SCAN_SWITCH_CHANNEL_OFF, 0, OAL_PTR_NULL);
        dmac_mgmt_switch_channel(pst_hal_device, pst_next_scan_channel, OAL_TRUE);//�ǹ���ģʽ������Ӳ�����У��л��ŵ���Ҫ��fifo������TRUE
        return;
    }

    uc_up_vap_num = hal_device_find_all_up_vap(pst_hal_device, auc_mac_vap_id);
    for (uc_vap_idx = 0; uc_vap_idx < uc_up_vap_num; uc_vap_idx++)
    {
        pst_mac_vap[uc_vap_idx]  = (mac_vap_stru *)mac_res_get_mac_vap(auc_mac_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_mac_vap[uc_vap_idx])
        {
            OAM_ERROR_LOG1(0, OAM_SF_SCAN, "dmac_scan_do_switch_channel_off::pst_mac_vap[%d] IS NULL.", auc_mac_vap_id[uc_vap_idx]);
            return;
        }
    }

    hal_device_handle_event(pst_hal_device, HAL_DEVICE_EVENT_SCAN_SWITCH_CHANNEL_OFF, 0, OAL_PTR_NULL);

    if (2 == uc_up_vap_num)
    {
#ifdef _PRE_WLAN_FEATURE_DBAC
        if (mac_is_dbac_running(pst_mac_device))
        {
            dmac_scan_dbac_switch_channel_off(pst_mac_device, pst_mac_vap[0], pst_mac_vap[1]);
        }
        else
#endif
        {
            /* ���Ǻ�����Ĺ�һ */
            dmac_the_same_channel_switch_channel_off(pst_mac_device, pst_mac_vap[0], pst_mac_vap[1], pst_next_scan_channel, pst_hal_device->st_hal_scan_params.us_scan_time);
        }
    }
    else
    {
        /* work״̬��Ҳ�п�����0��up vap,p2p go ɨ�����״̬ */
        if (uc_up_vap_num != 0)
        {
            for (uc_vap_idx = 0; uc_vap_idx < uc_up_vap_num; uc_vap_idx++)
            {
                dmac_vap_pause_tx(pst_mac_vap[uc_vap_idx]);
                dmac_switch_channel_off(pst_mac_device, pst_mac_vap[uc_vap_idx], pst_next_scan_channel, pst_hal_device->st_hal_scan_params.us_scan_time);
            }
        }
        else
        {
            OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "dmac_scan_do_switch_channel_off::vap is joining can not scan!!!");
            dmac_mgmt_switch_channel(pst_hal_device, pst_next_scan_channel, OAL_TRUE);//�ǹ���ģʽ������Ӳ�����У��л��ŵ���Ҫ��fifo������TRUE
        }
    }
}

oal_void dmac_scan_one_channel_start(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_is_scan_start)
{
    dmac_vap_stru      *pst_dmac_vap;
    mac_device_stru    *pst_mac_device;

    pst_mac_device = mac_res_get_dev(pst_hal_device->uc_mac_device_id);
    pst_dmac_vap   = mac_res_get_dmac_vap(pst_mac_device->st_scan_params.uc_vap_id);
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(pst_hal_device->uc_device_id, OAM_SF_SCAN, "{dmac_scan_one_channel_start::pst_dmac_vap null.}");
        return;
    }

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)
    /* ɨ���������,����������������ɨ */
    if (OAL_FALSE == dmac_check_can_start_scan(pst_mac_device, pst_hal_device))
    {
        return;
    }
#endif

    /* ���ŵ�:����ֱ�������Լ��������� */
    dmac_scan_do_switch_channel_off(pst_mac_device, pst_hal_device, pst_dmac_vap);

    /* ɨ�迪ʼ�Լ���home channel����ʱ��Ҫ�������mac��ַ */
    if (OAL_TRUE == en_is_scan_start)
    {
        dmac_scan_set_vap_mac_addr_by_scan_state(pst_mac_device, pst_hal_device, OAL_TRUE);
    }

#ifdef _PRE_WLAN_FEATURE_BTCOEX
    /* ����ps״̬�£�ɨ��ȵ�bt��ps��������ִ�У���scan begin״̬��btcoex */
    if(HAL_BTCOEX_SW_POWSAVE_WORK == GET_HAL_BTCOEX_SW_PREEMPT_TYPE(pst_hal_device))
    {
        GET_HAL_BTCOEX_SW_PREEMPT_TYPE(pst_hal_device) = HAL_BTCOEX_SW_POWSAVE_SCAN_BEGIN;
    }
    else /* ��begin֮ǰ��switch_channel_off�Ὣhal device�л���scan״̬ */
#endif
    {
        dmac_scan_begin(pst_mac_device, pst_hal_device);
    }
}

oal_void  dmac_scan_switch_channel_off(mac_device_stru *pst_mac_device)
{
    wlan_scan_mode_enum_uint8       en_scan_mode;
    dmac_vap_stru                  *pst_dmac_vap;   /* ����ɨ���VAP */
    oal_uint8                       uc_device_max;
    oal_uint8                       uc_idx;
    hal_to_dmac_device_stru        *pst_hal_device;

    en_scan_mode = pst_mac_device->st_scan_params.en_scan_mode;

    if (en_scan_mode >= WLAN_SCAN_MODE_BUTT)
    {
        OAM_ERROR_LOG1(0, OAM_SF_SCAN, "{dmac_scan_switch_channel_off::scan mode[%d] is invalid.}", en_scan_mode);
        return;
    }

    pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_mac_device->st_scan_params.uc_vap_id);
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{dmac_scan_switch_channel_off::pst_dmac_vap null.}");
        return;
    }

    /* ǰ��ɨ�����PNO�����ɨ��(ע: PNOֻ���豸δ������״̬�²ŷ���ɨ��) ֱ�����ŵ� */
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    if (!pst_mac_device->st_scan_params.en_need_switch_back_home_channel && ((WLAN_SCAN_MODE_FOREGROUND == en_scan_mode) || (WLAN_SCAN_MODE_BACKGROUND_PNO == en_scan_mode)))
#else
    if ((WLAN_SCAN_MODE_FOREGROUND == en_scan_mode) || (WLAN_SCAN_MODE_BACKGROUND_PNO == en_scan_mode))
#endif
    {
    #ifdef _PRE_WLAN_FEATURE_PROXYSTA
        if (mac_is_proxysta_enabled(pst_mac_device) && mac_vap_is_vsta(&pst_dmac_vap->st_vap_base_info))
        {
            /* proxysta�����ŵ� */
            OAM_INFO_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{dmac_scan_switch_channel_off::proxysta DO NOT switch channnel.}");
            return;
        }
    #endif
    }

    /* HAL�ӿڻ�ȡ֧��device���� */
    hal_chip_get_device_num(pst_mac_device->uc_chip_id, &uc_device_max);

    for (uc_idx = 0; uc_idx < uc_device_max; uc_idx++)
    {
        pst_hal_device = dmac_scan_find_hal_device(pst_mac_device, pst_dmac_vap, uc_idx);
        if (OAL_PTR_NULL == pst_hal_device)
        {
            continue;
        }

        /* �ǹ���״̬�µ�ɨ�裬��֤1M dbb scaling��11Mһ�� */
        if (GET_HAL_DEVICE_STATE(pst_hal_device) != HAL_DEVICE_WORK_STATE)
        {
        #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
            dmac_config_update_dsss_scaling_reg(pst_hal_device, HAL_ALG_USER_DISTANCE_FAR);
        #endif  /* _PRE_PLAT_FEATURE_CUSTOMIZE */
        }

        dmac_scan_one_channel_start(pst_hal_device, OAL_TRUE);
    }
}

oal_bool_enum_uint8  dmac_scan_switch_channel_back(mac_device_stru *pst_mac_device, hal_to_dmac_device_stru *pst_hal_device)
{
    hal_scan_params_stru           *pst_hal_scan_status;

    pst_hal_scan_status = &(pst_hal_device->st_hal_scan_params);
#ifdef _PRE_WLAN_FEATURE_DBAC
    if (mac_is_dbac_running(pst_mac_device) && dmac_alg_dbac_is_pause(pst_hal_device))
    {
        if (OAL_FALSE == pst_hal_scan_status->en_working_in_home_chan)
        {
            /* clear fifo when dbac resume*/
            hal_disable_machw_phy_and_pa(pst_hal_device);
            hal_clear_tx_hw_queue(pst_hal_device);
            hal_recover_machw_phy_and_pa(pst_hal_device);
            dmac_clear_tx_queue(pst_hal_device);
        }
        /* dbac����ֻ��ָ�dbac����dbac�����е������ŵ� */
        dmac_alg_dbac_resume(pst_hal_device, OAL_TRUE);
        return OAL_FALSE;
    }
#endif

    if (OAL_FALSE == pst_hal_scan_status->en_working_in_home_chan)
    {
        /* �лع���VAP���ڵ��ŵ� */
        OAM_WARNING_LOG3(0, OAM_SF_SCAN, "{dmac_scan_switch_channel_back::switch home channel[%d], band[%d], bw[%d]}",
                      pst_hal_scan_status->st_home_channel.uc_chan_number,
                      pst_hal_scan_status->st_home_channel.en_band,
                      pst_hal_scan_status->st_home_channel.en_bandwidth);

        /* �л��ŵ���Ҫ��fifo������TRUE */
        dmac_mgmt_switch_channel(pst_hal_device, &(pst_hal_scan_status->st_home_channel), OAL_TRUE);

        /* �ָ�home�ŵ��ϱ���ͣ�ķ���,������ٶ��а��İ��� */
        dmac_vap_resume_tx_by_chl(pst_mac_device, pst_hal_device, &(pst_hal_scan_status->st_home_channel));

        return OAL_TRUE;
    }
    return OAL_FALSE;
}

OAL_STATIC oal_bool_enum_uint8  dmac_scan_need_switch_home_channel(hal_to_dmac_device_stru  *pst_hal_device)
{
    oal_uint8                    uc_reamin_chans;
    hal_scan_params_stru        *pst_scan_params;

    pst_scan_params = &(pst_hal_device->st_hal_scan_params);

    /* ����ɨ����Ҫ�лع����ŵ� */
    /* en_need_switch_back_home_channel ���������ж� */
    if (OAL_TRUE == pst_scan_params->en_need_switch_back_home_channel)
    {
        if (0 == pst_scan_params->uc_scan_channel_interval)
        {
            OAM_WARNING_LOG0(0, OAM_SF_SCAN, "{dmac_scan_need_switch_home_channel::scan_channel_interval is 0, set default value 6!}");
            pst_scan_params->uc_scan_channel_interval = MAC_SCAN_CHANNEL_INTERVAL_DEFAULT;
        }

        if (pst_scan_params->uc_channel_nums < (pst_scan_params->uc_scan_chan_idx + 1))
        {
            OAM_ERROR_LOG2(0, OAM_SF_SCAN, "{dmac_scan_need_switch_home_channel::scan channel nums[%d] < scan idx[%d]!!!}",
                            pst_scan_params->uc_channel_nums, pst_scan_params->uc_scan_chan_idx);
            return OAL_FALSE;
        }

        uc_reamin_chans = pst_scan_params->uc_channel_nums - (pst_scan_params->uc_scan_chan_idx + 1);

        /* ʣ����ŵ��� >= uc_scan_channel_interval / 2 */
        if (uc_reamin_chans >= (pst_scan_params->uc_scan_channel_interval >> 1))
        {
            return (0 == pst_scan_params->uc_scan_chan_idx % pst_scan_params->uc_scan_channel_interval);
        }
    }

    return OAL_FALSE;
}
#if defined(_PRE_PRODUCT_ID_HI110X_DEV)

OAL_STATIC oal_uint32  dmac_scan_check_2g_scan_results(mac_device_stru *pst_mac_device, hal_to_dmac_device_stru *pst_hal_device, mac_vap_stru *pst_vap, wlan_channel_band_enum_uint8 en_next_band)
{
    hal_scan_params_stru          *pst_hal_scan_params = &(pst_hal_device->st_hal_scan_params);

    if (((WLAN_SCAN_MODE_FOREGROUND == pst_mac_device->st_scan_params.en_scan_mode)
         || (WLAN_SCAN_MODE_BACKGROUND_STA == pst_mac_device->st_scan_params.en_scan_mode))
         && (OAL_TRUE != pst_mac_device->st_scan_params.bit_is_p2p0_scan))
    {
        if (WLAN_BAND_2G == pst_hal_scan_params->uc_last_channel_band)
        {
            if (pst_hal_scan_params->en_scan_curr_chan_find_bss_flag == OAL_TRUE)
            {
                pst_hal_scan_params->uc_scan_ap_num_in_2p4++;
                pst_hal_scan_params->en_scan_curr_chan_find_bss_flag = OAL_FALSE;
            }

            if ((WLAN_BAND_5G == en_next_band)
               ||(MAC_SCAN_STATE_IDLE == pst_hal_scan_params->en_curr_scan_state))
            {
                if ((pst_hal_scan_params->uc_scan_ap_num_in_2p4 <= 2) || ((pst_mac_device->uc_scan_count % 30) == 0))
                {
                    OAM_WARNING_LOG2(0, OAM_SF_SCAN, "{dmac_scan_check_2g_scan_results::2.4G scan ap channel num = %d, scan_count = %d.}",
                                  pst_hal_scan_params->uc_scan_ap_num_in_2p4,
                                  pst_mac_device->uc_scan_count);
#ifdef _PRE_WLAN_DFT_STAT
                    dmac_dft_report_all_ota_state(pst_vap);
#endif
                }
            }
        }
    }

    return OAL_SUCC;
}
#endif /* _PRE_PRODUCT_ID_HI110X_DEV */


OAL_STATIC oal_void dmac_scan_update_dfs_channel_scan_param(mac_device_stru     *pst_mac_device,
                                                            hal_to_dmac_device_stru *pst_hal_device,
                                                            mac_channel_stru    *pst_mac_channel,
                                                            oal_uint16          *pus_scan_time,
                                                            oal_bool_enum_uint8 *pen_send_probe_req)
{
    mac_vap_stru           *pst_mac_vap;
    hal_scan_params_stru   *pst_scan_params = &(pst_hal_device->st_hal_scan_params);

    /* ���״��ŵ�����Ҫ����probe req��ɨ��ʱ���ɨ������л�ȡ */
    if (OAL_FALSE == mac_is_dfs_channel(pst_mac_channel->en_band, pst_mac_channel->uc_chan_number))
    {
        *pen_send_probe_req = OAL_TRUE;
        *pus_scan_time      = pst_scan_params->us_scan_time;
        return;
    }

    /* �����ǰΪ����״̬���ҹ���AP �ŵ��ͱ���ɨ���ŵ���ͬ������Ϊ���ŵ����״ֱ�ӷ���probe req ����ɨ�� */
    pst_mac_vap = mac_res_get_mac_vap(pst_mac_device->st_scan_params.uc_vap_id);
    if (OAL_PTR_NULL == pst_mac_vap)
    {
        OAM_WARNING_LOG1(pst_mac_device->st_scan_params.uc_vap_id, OAM_SF_SCAN,
                        "{dmac_scan_update_dfs_channel_scan_param::get vap [%d] fail.}",
                        pst_mac_device->st_scan_params.uc_vap_id);
        *pen_send_probe_req = OAL_FALSE;
        *pus_scan_time      = WLAN_DEFAULT_PASSIVE_SCAN_TIME;
        return;
    }

    if (IS_AP(pst_mac_vap))
    {
        *pen_send_probe_req = OAL_FALSE;
        *pus_scan_time      = pst_scan_params->us_scan_time;
        return;
    }

    if ((MAC_VAP_STATE_UP == pst_mac_vap->en_vap_state || MAC_VAP_STATE_PAUSE == pst_mac_vap->en_vap_state)
        && (pst_mac_channel->uc_chan_number == pst_mac_vap->st_channel.uc_chan_number))
    {
        *pen_send_probe_req = OAL_TRUE;
        *pus_scan_time      = pst_scan_params->us_scan_time;
        return;
    }

    /* �״��ŵ���һ��ɨ�裬������probe req �����ڸ��ŵ���ͣ60ms
     * �״��ŵ��ڶ���ɨ�裬����ڵ�һ��ɨ��ʱ��û�з�����AP��
     *                     ���˳����ŵ�ɨ��
     *                     ����ڵ�һ��ɨ��ʱ������AP��
     *                     ����probe req �����ڸ��ŵ���ͣ20ms
     */
    if (pst_scan_params->uc_curr_channel_scan_count == 0)
    {
        /* �״��ŵ���һ��ɨ�裬������probe req ���ڸ��ŵ�����60ms */
        *pen_send_probe_req = OAL_FALSE;
        *pus_scan_time      = WLAN_DEFAULT_PASSIVE_SCAN_TIME;
    }
    else
    {
        if (pst_scan_params->en_scan_curr_chan_find_bss_flag == OAL_TRUE)
        {
            /* �״��ŵ��ڶ���ɨ�裬�ҵ�һ��ɨ��ʱ�з���AP,
             * ������Ҫ����probe req��ɨ��ʱ���ɨ�������ȡ
             */
            *pen_send_probe_req = OAL_TRUE;
            *pus_scan_time      = pst_scan_params->us_scan_time;
        }
        else
        {
            /* �״��ŵ��ڶ���ɨ�裬�ҵ�һ��ɨ��ʱû�з���AP,
             * ���ó�ʱ��ʱ��Ϊ0
             */
            *pen_send_probe_req = OAL_FALSE;
            *pus_scan_time      = 0;
        }
    }
    return;
}

oal_void dmac_scan_handle_switch_channel_back(mac_device_stru *pst_mac_device, hal_to_dmac_device_stru *pst_hal_device, hal_scan_params_stru *pst_hal_scan_params)
{
    oal_uint32                  ul_ret;
    oal_bool_enum_uint8         en_switched;
    oal_uint8                   uc_mac_vap_id;
    dmac_vap_stru              *pst_dmac_home_vap;

    en_switched = dmac_scan_switch_channel_back(pst_mac_device, pst_hal_device);
    if (OAL_TRUE == en_switched)
    {
        /* �����л�home�ŵ��ĳ�����Ҫ��ȡhome�ŵ�vap֪ͨ�㷨 */
        ul_ret = hal_device_find_one_up_vap(pst_hal_device, &uc_mac_vap_id);
        if (OAL_SUCC == ul_ret)
        {
            pst_dmac_home_vap  = (dmac_vap_stru *)mac_res_get_dmac_vap(uc_mac_vap_id);
            if (pst_dmac_home_vap != OAL_PTR_NULL)
            {
                if (pst_hal_scan_params->st_home_channel.uc_chan_number == pst_dmac_home_vap->st_vap_base_info.st_channel.uc_chan_number)
                {
                    dmac_scan_switch_channel_notify_alg(pst_hal_device, pst_dmac_home_vap, &(pst_hal_scan_params->st_home_channel));
                }
            }
            else
            {
                OAM_ERROR_LOG1(0, OAM_SF_SCAN, "{dmac_scan_handle_switch_channel_back::vap id[%d],pst_dmac_vap is null.}", uc_mac_vap_id);
            }
        }
    }
}

oal_void dmac_single_hal_device_scan_complete(mac_device_stru *pst_mac_device, hal_to_dmac_device_stru  *pst_hal_device,
                                                         dmac_vap_stru  *pst_dmac_vap, oal_uint8 uc_scan_complete_event)
{
    oal_uint8                   uc_do_p2p_listen;
    oal_uint32                  ul_run_time;
    oal_uint32                  ul_current_timestamp;
    hal_scan_params_stru       *pst_hal_scan_params = &(pst_hal_device->st_hal_scan_params);

#ifdef _PRE_WLAN_FEATURE_GNSS_SCAN
    dmac_device_stru           *pst_dmac_device = dmac_res_get_mac_dev(pst_mac_device->uc_device_id);;
    if (OAL_PTR_NULL == pst_dmac_device)
    {
        OAM_ERROR_LOG0(pst_mac_device->uc_device_id, OAM_SF_SCAN, "{dmac_single_hal_device_scan_complete::pst_dmac_device null.}");
        return;
    }
#endif

    if (MAC_SCAN_STATE_IDLE == pst_hal_scan_params->en_curr_scan_state)
    {
        OAM_ERROR_LOG1(pst_hal_device->uc_device_id, OAM_SF_SCAN, "{dmac_single_hal_device_scan_complete::hal dev is already scan complete[%d].}",
                        pst_hal_scan_params->en_curr_scan_state);
        return;
    }

    /* listenʱ�޸�vap�ŵ�Ϊlisten�ŵ���listen��������Ҫ�ָ� p2p listen�������ǲ��� */
    uc_do_p2p_listen = pst_mac_device->st_scan_params.uc_scan_func & MAC_SCAN_FUNC_P2P_LISTEN;
    if (uc_do_p2p_listen)
    {
        pst_dmac_vap->st_vap_base_info.st_channel = pst_mac_device->st_p2p_vap_channel;
    }

    ul_current_timestamp = (oal_uint32)OAL_TIME_GET_STAMP_MS();

    ul_run_time = OAL_TIME_GET_RUNTIME(pst_hal_scan_params->ul_scan_timestamp, ul_current_timestamp);

    OAM_WARNING_LOG4(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,"{dmac_single_hal_device_scan_complete::hal device[%d]scan total time[%d]ms,scan chan[%d]need_back_home[%d]}",
                        pst_hal_device->uc_device_id, ul_run_time, pst_hal_scan_params->uc_channel_nums, pst_hal_scan_params->en_need_switch_back_home_channel);

    dmac_scan_set_vap_mac_addr_by_scan_state(pst_mac_device, pst_hal_device, OAL_FALSE);

#ifdef _PRE_WLAN_FEATURE_GNSS_SCAN
    //dmac_scan_dump_bss_list(&(pst_dmac_device->st_scan_for_gnss_info.st_dmac_scan_info_list));
    dmac_scan_check_ap_bss_info(&(pst_dmac_device->st_scan_for_gnss_info.st_dmac_scan_info_list));
#endif

    /* ��·ɨ��������Ȼ�home channel���� */
    if (OAL_TRUE == pst_hal_scan_params->en_need_switch_back_home_channel)
    {
#ifdef _PRE_WLAN_FEATURE_BTCOEX
        /* abort�Ļ���ǿ�ƽ���ɨ�裬����������������, abort֮��ָ���idle����work״̬�ˣ�
            ps���ư������������ߣ����Դ���save����normal״̬��btcoexֻ����ά�⣬ȷ����Ƶ���̶� */
        if(HAL_DEVICE_EVENT_SCAN_ABORT == uc_scan_complete_event)
        {
            GET_HAL_BTCOEX_SW_PREEMPT_TYPE(pst_hal_device) = HAL_BTCOEX_SW_POWSAVE_SCAN_ABORT;
            OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_COEX,
                                "{dmac_single_hal_device_scan_complete:: do scan abort right now!!!}");
        }
#endif
        dmac_scan_handle_switch_channel_back(pst_mac_device, pst_hal_device, pst_hal_scan_params);

        hal_device_handle_event(pst_hal_device, uc_scan_complete_event, 0, OAL_PTR_NULL);
    }
    else
    {
        hal_disable_machw_phy_and_pa(pst_hal_device);
        hal_clear_tx_hw_queue(pst_hal_device);
        hal_recover_machw_phy_and_pa(pst_hal_device);
        dmac_clear_tx_queue(pst_hal_device);

        hal_device_handle_event(pst_hal_device, uc_scan_complete_event, 0, OAL_PTR_NULL);

#ifdef _PRE_WLAN_FEATURE_BTCOEX
        if(HAL_DEVICE_EVENT_SCAN_ABORT == uc_scan_complete_event)
        {
            OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_COEX,
                             "{dmac_single_hal_device_scan_complete:: scan abort not need back home channel bt_sw_preempt_type[%d]!}",
                             GET_HAL_BTCOEX_SW_PREEMPT_TYPE(pst_hal_device));

            GET_HAL_BTCOEX_SW_PREEMPT_TYPE(pst_hal_device) = HAL_BTCOEX_SW_POWSAVE_SCAN_ABORT;
        }
#endif
    }

#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
    dmac_scan_check_2g_scan_results(pst_mac_device, pst_hal_device, &(pst_dmac_vap->st_vap_base_info), pst_hal_scan_params->uc_last_channel_band);
#endif
}

#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
extern    oal_uint32        g_ul_frw_timer_start_stamp;     //ά���źţ�������¼��һ�����ж϶�ʱ��������ʱ��
extern    oal_uint32        g_ul_dispatch_event_time;       //ά���źţ�������¼�жϺ������¼���ʱ��
extern    oal_uint32        g_ul_proc_start_time;           //ά���źţ�������¼�¼��������ʼʱ��
extern    oal_uint32        g_ul_interrupt_start_time;
extern    oal_dlist_head_stru         g_st_timer_list;
OAL_STATIC oal_void dmac_scan_check_scan_timer(hal_to_dmac_device_stru  *pst_hal_device)
{
    oal_uint32             ul_timer_runtime;
    oal_uint32             ul_current_time;
    oal_dlist_head_stru   *pst_timeout_entry;
    frw_timeout_stru      *pst_timeout_element;
    frw_timeout_stru      *pst_scan_timer = &(pst_hal_device->st_hal_scan_params.st_scan_timer);

    /*timer create time = ul_time_stamp - ul_timeout */
    ul_current_time  = (oal_uint32)OAL_TIME_GET_STAMP_MS();
    ul_timer_runtime = OAL_TIME_GET_RUNTIME((pst_scan_timer->ul_time_stamp - pst_scan_timer->ul_timeout), ul_current_time);
    if ((ul_timer_runtime > (pst_scan_timer->ul_timeout + DMAC_SCAN_TIMER_DEVIATION_TIME))&&(g_ul_interrupt_start_time > (g_ul_frw_timer_start_stamp + 2)))
    {
        OAM_WARNING_LOG3(0, OAM_SF_SCAN, "{dmac_scan_check_scan_timer::current time[%u]timer too long[%d]to run expert[%d]}", ul_current_time, ul_timer_runtime, pst_scan_timer->ul_timeout);
        OAM_WARNING_LOG4(0, OAM_SF_ANY, "Device:frw_timer_start_stamp [%u],inter_start_time[%u],dispatch_event_time[%u],proc_start_time[%u]",g_ul_frw_timer_start_stamp,g_ul_interrupt_start_time,g_ul_dispatch_event_time,g_ul_proc_start_time);

        //�ų���Ϊtimer�¼��׳����ȴ������ʱ�䵼�µ�timer��ʱ
        OAM_WARNING_LOG3(0, OAM_SF_ANY, "current_timer enabled[%d], registerd[%d], period[%d]",
                     pst_scan_timer->en_is_enabled, pst_scan_timer->en_is_registerd, pst_scan_timer->en_is_periodic);
        OAM_WARNING_LOG3(0, OAM_SF_ANY, "current_timer stamp[%u], timeout[%u], curr timer stamp[%u]",
                     pst_scan_timer->ul_time_stamp, pst_scan_timer->ul_timeout, pst_scan_timer->ul_curr_time_stamp);
        pst_timeout_entry = g_st_timer_list.pst_next;
        if (OAL_PTR_NULL != pst_timeout_entry)
        {
            pst_timeout_element = OAL_DLIST_GET_ENTRY(pst_timeout_entry, frw_timeout_stru, st_entry);

            OAM_WARNING_LOG3(0, OAM_SF_ANY, "first_timer enabled[%d], registerd[%d], period[%d]",
                         pst_timeout_element->en_is_enabled, pst_timeout_element->en_is_registerd, pst_timeout_element->en_is_periodic);
            OAM_WARNING_LOG3(0, OAM_SF_ANY, "first_timer stamp[%u], timeout[%u], curr timer stamp[%u]",
                         pst_timeout_element->ul_time_stamp, pst_timeout_element->ul_timeout, pst_timeout_element->ul_curr_time_stamp);
        }
    }
}
#endif

OAL_STATIC oal_uint32  dmac_scan_curr_channel_scan_time_out(void *p_arg)
{
    mac_device_stru                 *pst_mac_device;
    hal_scan_params_stru            *pst_hal_scan_params;
    dmac_vap_stru                   *pst_dmac_vap;
    oal_uint8                        uc_do_meas;
    hal_to_dmac_device_stru         *pst_hal_device = (hal_to_dmac_device_stru *)p_arg;

    /* ��ȡɨ����� */
    pst_hal_scan_params = &(pst_hal_device->st_hal_scan_params);

    pst_mac_device  = mac_res_get_dev(pst_hal_device->uc_mac_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG1(0, OAM_SF_SCAN, "{dmac_scan_curr_channel_scan_time_out::id[%d]pst_mac_device == NULL}", pst_hal_device->uc_mac_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡ����ɨ���dmac vap�ṹ��Ϣ */
    pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_mac_device->st_scan_params.uc_vap_id);
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_WARNING_LOG0(pst_mac_device->st_scan_params.uc_vap_id, OAM_SF_SCAN, "{dmac_scan_curr_channel_scan_time_out::mac_res_get_dmac_vap fail}");
        return OAL_ERR_CODE_PTR_NULL;
    }

#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
    dmac_scan_check_scan_timer(pst_hal_device);
#endif

    /* ���±����ŵ���ɨ����� */
    pst_hal_scan_params->uc_curr_channel_scan_count++;

    /* ����ɨ��������ŵ�ɨ�����������ж��Ƿ��л��ŵ��� */
    if (pst_hal_scan_params->uc_curr_channel_scan_count >= pst_hal_scan_params->uc_max_scan_count_per_channel)
    {
        /* ���ŵ�ɨ�����������·���ɨ�������Ҫ�ϱ��ŵ�ͳ����Ϣ�����ϱ���Ĭ�Ϲر� */
        uc_do_meas = pst_mac_device->st_scan_params.uc_scan_func & (MAC_SCAN_FUNC_MEAS | MAC_SCAN_FUNC_STATS);
        if (uc_do_meas)
        {
            /* �ϱ��ŵ�������� */
            dmac_scan_report_channel_statistics_result(pst_hal_device, (GET_HAL_DEV_CURRENT_SCAN_IDX(pst_hal_device)));

            /* ����ŵ����������׼����һ���ŵ�����ֵ��ͳ�� */
            OAL_MEMZERO(&(pst_hal_device->st_chan_result), OAL_SIZEOF(wlan_scan_chan_stats_stru));
        }

        pst_hal_scan_params->uc_scan_chan_idx += 1;              /* �л��ŵ� */
    }
    else
    {
        /* ���ŵ�ɨ�����δ��ɣ������л��ŵ���ֱ�ӷ���ɨ�� */
#ifdef _PRE_WLAN_FEATURE_BTCOEX
        /* ����ps״̬�£�ɨ��ȵ�bt��ps��������ִ�У���scan begin״̬��btcoex */
        if(HAL_BTCOEX_SW_POWSAVE_WORK == GET_HAL_BTCOEX_SW_PREEMPT_TYPE(pst_hal_device))
        {
            GET_HAL_BTCOEX_SW_PREEMPT_TYPE(pst_hal_device) = HAL_BTCOEX_SW_POWSAVE_SCAN_BEGIN;
        }
        else
#endif
        {
            dmac_scan_begin(pst_mac_device, pst_hal_device);
        }

        return OAL_SUCC;
    }

    /* �˴�ɨ��������ɣ���һЩ��β���� */
    if (pst_hal_scan_params->uc_scan_chan_idx >= pst_hal_scan_params->uc_channel_nums)
    {
        dmac_scan_prepare_end(pst_mac_device, pst_hal_device);
        return OAL_SUCC;
    }

    if (OAL_TRUE == dmac_scan_need_switch_home_channel(pst_hal_device))
    {
#ifdef _PRE_WLAN_FEATURE_BTCOEX
        /* ��Ҫ��home channelʱ���������ps״̬����Ҫ�ӳ٣��ȵ�ps������ɺ��home channel */
        if (HAL_BTCOEX_SW_POWSAVE_WORK == GET_HAL_BTCOEX_SW_PREEMPT_TYPE(pst_hal_device))
        {
            GET_HAL_BTCOEX_SW_PREEMPT_TYPE(pst_hal_device) = HAL_BTCOEX_SW_POWSAVE_SCAN_WAIT;
        }
        else
#endif
        {
            dmac_scan_switch_home_channel_work(pst_mac_device, pst_hal_device);
        }
    }
    else
    {
        dmac_scan_one_channel_start(pst_hal_device, OAL_FALSE);
    }

    return OAL_SUCC;
}


OAL_STATIC oal_void dmac_pno_scan_send_probe_with_ssid(mac_device_stru *pst_mac_device, hal_to_dmac_device_stru *pst_hal_device, oal_uint8 uc_band)
{
    mac_pno_sched_scan_mgmt_stru *pst_pno_sched_scan_mgmt;
    dmac_vap_stru                *pst_dmac_vap;
    oal_uint8                     uc_band_tmp;
    oal_uint8                     uc_loop;
    oal_uint32                    ul_ret;

    if(OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{dmac_pno_scan_send_probe_with_ssid::pst_mac_device is null.}");
        return;
    }

    pst_pno_sched_scan_mgmt = pst_mac_device->pst_pno_sched_scan_mgmt;

    pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_mac_device->st_scan_params.uc_vap_id);
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{dmac_pno_scan_send_probe_with_ssid::pst_dmac_vap null.}");
        return;
    }

    
    if(OAL_PTR_NULL == pst_dmac_vap->st_vap_base_info.pst_mib_info)
    {
        OAM_ERROR_LOG4(0, OAM_SF_SCAN, "{dmac_pno_scan_send_probe_with_ssid:: vap mib info is null,uc_vap_id[%d], p_fn_cb[%p], uc_scan_func[%d], uc_curr_channel_scan_count[%d].}",
                   pst_mac_device->st_scan_params.uc_vap_id,
                   pst_mac_device->st_scan_params.p_fn_cb,
                   pst_mac_device->st_scan_params.uc_scan_func,
                   pst_hal_device->st_hal_scan_params.uc_curr_channel_scan_count);
        return;
    }


    /* ��̽������ʱ����Ҫ��ʱ����vap��band��Ϣ����ֹ5G��11b */
    uc_band_tmp = pst_dmac_vap->st_vap_base_info.st_channel.en_band;

    pst_dmac_vap->st_vap_base_info.st_channel.en_band = uc_band;

    for(uc_loop = 0; uc_loop < pst_pno_sched_scan_mgmt->st_pno_sched_scan_params.l_ssid_count; uc_loop++)
    {
        if(OAL_TRUE == pst_pno_sched_scan_mgmt->st_pno_sched_scan_params.ast_match_ssid_set[uc_loop].en_scan_ssid)
        {
            /* ���������SSID,��ָ��SSIDɨ�� */
            ul_ret = dmac_scan_send_probe_req_frame(pst_dmac_vap, BROADCAST_MACADDR, (oal_int8 *)pst_pno_sched_scan_mgmt->st_pno_sched_scan_params.ast_match_ssid_set[uc_loop].auc_ssid);
            if (OAL_SUCC != ul_ret)
            {
                OAM_WARNING_LOG1(0, OAM_SF_SCAN, "{dmac_pno_scan_send_probe_with_ssid::dmac_scan_send_probe_req_frame failed[%u].}", ul_ret);
            }
        }
    }

    pst_dmac_vap->st_vap_base_info.st_channel.en_band = uc_band_tmp;
    return;
}


oal_void dmac_scan_begin(mac_device_stru *pst_mac_device, hal_to_dmac_device_stru *pst_hal_device)
{
    hal_to_dmac_device_stru         *pst_original_hal_device;
    mac_scan_req_stru               *pst_scan_params;
    dmac_vap_stru                   *pst_dmac_vap;
    oal_uint32                       ul_ret;
    oal_uint8                        uc_band;
    oal_uint8                        uc_do_bss_scan;
    oal_uint8                        uc_do_meas;
    oal_uint8                        uc_loop;
    oal_uint8                        uc_scan_chan_idx;
    oal_uint8                        uc_do_p2p_listen = 0;
    oal_bool_enum_uint8              en_send_probe_req;
    oal_uint16                       us_scan_time;
#ifdef _PRE_WLAN_FEATURE_11K_EXTERN
    oal_uint32                       aul_act_meas_start_time[2];
#endif
#ifdef _PRE_WLAN_FEATURE_FTM
    dmac_ftm_initiator_stru         *past_ftm_init;
#endif

    /* ��·hal device������,�˴β�ɨ�� */
    if (OAL_PTR_NULL == pst_hal_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{dmac_scan_begin::pst_hal_device NULL.}");
        return;
    }

    /* ��·����ɨ����ɲ��ٷ�pro req */
    if (MAC_SCAN_STATE_IDLE == pst_hal_device->st_hal_scan_params.en_curr_scan_state)
    {
        OAM_ERROR_LOG1(pst_hal_device->uc_device_id, OAM_SF_SCAN, "{dmac_scan_begin::now chan idx[%d] scan is completed.}", pst_hal_device->st_hal_scan_params.uc_scan_chan_idx);
        return;
    }

    /* ��·��pause����ɨ�� */
    if (pst_hal_device->st_hal_scan_params.uc_scan_pause_bitmap)
    {
        OAM_WARNING_LOG2(pst_hal_device->uc_device_id, OAM_SF_SCAN, "{dmac_scan_begin::scan is paused[%x%x].scan chan idx[%d]}",
                                pst_hal_device->st_hal_scan_params.uc_scan_pause_bitmap, pst_hal_device->st_hal_scan_params.uc_scan_chan_idx);
        return;
    }

    /* ��ȡɨ����� */
    pst_scan_params = &(pst_mac_device->st_scan_params);

    uc_scan_chan_idx = GET_HAL_DEV_CURRENT_SCAN_IDX(pst_hal_device);
    uc_band          = pst_scan_params->ast_channel_list[uc_scan_chan_idx].en_band;
    uc_do_bss_scan   = pst_scan_params->uc_scan_func & MAC_SCAN_FUNC_BSS;
    uc_do_p2p_listen = pst_scan_params->uc_scan_func & MAC_SCAN_FUNC_P2P_LISTEN;
    pst_dmac_vap     = mac_res_get_dmac_vap(pst_scan_params->uc_vap_id);
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(pst_scan_params->uc_vap_id, OAM_SF_SCAN, "{dmac_scan_begin::pst_dmac_vap null.}");
        return;
    }

#ifdef _PRE_WLAN_FEATURE_FTM
    past_ftm_init = pst_dmac_vap->pst_ftm->ast_ftm_init;
#endif

    /* ��������л�ָ���Ƿ������� */
    pst_original_hal_device   = pst_dmac_vap->pst_hal_device;
    pst_dmac_vap->pst_hal_device = pst_hal_device;

    /* �����ǰɨ��ģʽ��Ҫͳ���ŵ���Ϣ����ʹ�ܶ�Ӧ�Ĵ��� */
    uc_do_meas = pst_scan_params->uc_scan_func & (MAC_SCAN_FUNC_MEAS | MAC_SCAN_FUNC_STATS);
    if (uc_do_meas)
    {
        /* ʹ���ŵ������ж� */
        hal_set_ch_statics_period(pst_hal_device, DMAC_SCAN_CHANNEL_STATICS_PERIOD_US);
        hal_set_ch_measurement_period(pst_hal_device, DMAC_SCAN_CHANNEL_MEAS_PERIOD_MS);
        hal_enable_ch_statics(pst_hal_device, 1);
    }

#ifdef _PRE_WLAN_FEATURE_11K
    if ((OAL_TRUE == pst_dmac_vap->bit_11k_enable) &&
        (WLAN_SCAN_MODE_RRM_BEACON_REQ == pst_scan_params->en_scan_mode))
    {
        hal_vap_tsf_get_64bit(pst_dmac_vap->pst_hal_vap, &(pst_dmac_vap->pst_rrm_info->aul_act_meas_start_time[1]),
                                  &(pst_dmac_vap->pst_rrm_info->aul_act_meas_start_time[0]));
        OAM_WARNING_LOG1(0, OAM_SF_SCAN, "{dmac_scan_begin::update start tsf ok, vap id[%d].}", pst_scan_params->uc_vap_id);
    }
#endif

#ifdef _PRE_WLAN_FEATURE_11K_EXTERN
    if(mac_mib_get_dot11RadioMeasurementActivated(&(pst_dmac_vap->st_vap_base_info)))
    {
        hal_vap_tsf_get_64bit(pst_dmac_vap->pst_hal_vap, &aul_act_meas_start_time[1], &aul_act_meas_start_time[0]);
        //OAM_WARNING_LOG2(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{dmac_scan_begin::aul_act_meas_start_time[%u][%u].}", aul_act_meas_start_time[1], aul_act_meas_start_time[0]);
        dmac_send_sys_event(&(pst_dmac_vap->st_vap_base_info), WLAN_CFGID_GET_MEAS_START_TSF, OAL_SIZEOF(oal_uint32)*2, (oal_uint8*)aul_act_meas_start_time);
    }
#endif

#ifdef _PRE_WLAN_FEATURE_FTM
    if(OAL_TRUE == past_ftm_init[0].en_iftmr)
    {
        OAM_WARNING_LOG0(pst_scan_params->uc_vap_id, OAM_SF_SCAN, "{dmac_scan_begin::dmac_send_iftmr}");
        dmac_sta_send_ftm_req(pst_dmac_vap);
        past_ftm_init[0].en_iftmr = OAL_FALSE;
    }
    if(OAL_TRUE == past_ftm_init[0].en_ftm_trigger)
    {
        OAM_WARNING_LOG0(pst_scan_params->uc_vap_id, OAM_SF_SCAN, "{dmac_scan_begin::dmac_ftm_send_trigger}");
        dmac_ftm_send_trigger(pst_dmac_vap);
        //dmac_sta_send_ftm_req(pst_dmac_vap);
        past_ftm_init[0].en_ftm_trigger = OAL_FALSE;
    }
#endif

    /* dfs�ŵ��жϣ�������״��ŵ���ִ�б���ɨ�� */
    dmac_scan_update_dfs_channel_scan_param(pst_mac_device,
                                            pst_hal_device,
                                            &(pst_scan_params->ast_channel_list[uc_scan_chan_idx]),
                                            &us_scan_time,
                                            &en_send_probe_req);

    /* ACTIVE��ʽ�·��͹㲥RPOBE REQ֡ */
    if (uc_do_bss_scan && (WLAN_SCAN_TYPE_ACTIVE == pst_scan_params->en_scan_type)
        && (OAL_TRUE == en_send_probe_req))
    {
        /* PNOָ��SSIDɨ��,���ָ��16��SSID */
        if(WLAN_SCAN_MODE_BACKGROUND_PNO == pst_mac_device->st_scan_params.en_scan_mode)
        {
            dmac_pno_scan_send_probe_with_ssid(pst_mac_device, pst_hal_device, uc_band);
        }

        /* ÿ���ŵ����͵�probe req֡�ĸ��� */
        for (uc_loop = 0; uc_loop < pst_scan_params->uc_max_send_probe_req_count_per_channel; uc_loop++)
        {
            ul_ret = dmac_scan_send_bcast_probe(pst_mac_device, uc_band, uc_loop);
            if (OAL_SUCC != ul_ret)
            {
                OAM_WARNING_LOG1(0, OAM_SF_SCAN, "{dmac_scan_begin::dmac_scan_send_bcast_probe failed[%d].}", ul_ret);
            }
        }
    }

    /* ���ڷ����������probe request������������ʱ��,��ָֹ��SSIDɨ�豨�Ĺ���,��ʱ��ʱ���ڶ��ڷ���,���ڽ���ɨ����ʱ����� */
    /* ����ɨ�趨ʱ�� */
    FRW_TIMER_CREATE_TIMER(&(pst_hal_device->st_hal_scan_params.st_scan_timer),
                           dmac_scan_curr_channel_scan_time_out,
                           us_scan_time,
                           pst_hal_device,
                           OAL_FALSE,
                           OAM_MODULE_ID_DMAC,
                           pst_mac_device->ul_core_id);

    pst_dmac_vap->pst_hal_device = pst_original_hal_device;

    /* p2p���������߼� */
    /* p2p listenʱ��Ҫ����VAP���ŵ�����probe rsp֡(DSSS ie, ht ie)��Ҫ��listen������ָ� */
    if (uc_do_p2p_listen)
    {
        pst_mac_device->st_p2p_vap_channel = pst_dmac_vap->st_vap_base_info.st_channel;

        pst_dmac_vap->st_vap_base_info.st_channel = pst_scan_params->ast_channel_list[0];
    }

    return;
}


oal_void dmac_scan_end(mac_device_stru *pst_mac_device)
{
    dmac_vap_stru              *pst_dmac_vap;
    mac_vap_stru               *pst_mac_vap;
    wlan_scan_mode_enum_uint8   en_scan_mode = WLAN_SCAN_MODE_BUTT;
#ifdef _PRE_WLAN_FEATURE_GNSS_SCAN
    dmac_device_stru           *pst_dmac_device;
#endif
    /* ��ȡdmac vap */
    pst_dmac_vap = mac_res_get_dmac_vap(pst_mac_device->st_scan_params.uc_vap_id);
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{dmac_scan_end::pst_dmac_vap is null.}");

        /* �ָ�deviceɨ��״̬Ϊ����״̬ */
        pst_mac_device->en_curr_scan_state = MAC_SCAN_STATE_IDLE;
        return;
    }

    /* ��ȡɨ��ģʽ */
    en_scan_mode = pst_mac_device->st_scan_params.en_scan_mode;
    pst_mac_vap  = &pst_dmac_vap->st_vap_base_info;

    if(WALN_LINKLOSS_SCAN_SWITCH_CHAN_EN == pst_mac_vap->st_ch_switch_info.en_linkloss_scan_switch_chan)
    {
        pst_mac_vap->st_ch_switch_info.en_linkloss_scan_switch_chan = WALN_LINKLOSS_SCAN_SWITCH_CHAN_DISABLE;
        OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{dmac_scan_end::csa_fsm back to init}");
        dmac_sta_csa_fsm_post_event(&(pst_dmac_vap->st_vap_base_info), WLAN_STA_CSA_EVENT_TO_INIT, 0, OAL_PTR_NULL);

        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{dmac_scan_end::linkloss scan change channel from [%d] to [%d].}",
            pst_mac_vap->st_channel.uc_chan_number, pst_mac_vap->st_ch_switch_info.uc_linkloss_change_chanel);
        pst_mac_vap->st_ch_switch_info.uc_new_channel   = pst_mac_vap->st_ch_switch_info.uc_linkloss_change_chanel;
        pst_mac_vap->st_ch_switch_info.en_new_bandwidth = pst_mac_vap->st_channel.en_bandwidth;
        dmac_chan_sta_switch_channel(pst_mac_vap);
    }

    /* ����ɨ��ģʽ���ж�Ӧɨ�����Ĵ��� */
    switch (en_scan_mode)
    {
        case WLAN_SCAN_MODE_FOREGROUND:
        case WLAN_SCAN_MODE_BACKGROUND_STA:
        case WLAN_SCAN_MODE_BACKGROUND_AP:
#ifdef _PRE_WLAN_FEATURE_HILINK
        case WLAN_SCAN_MODE_BACKGROUND_HILINK:
#endif
        {
#ifdef _PRE_WLAN_FEATURE_STA_PM
            /* ƽ̨�ļ���*/
            pfn_wlan_dumpsleepcnt();
#endif
            /* �ϱ�ɨ������¼���ɨ��״̬Ϊ�ɹ� */
            (oal_void)dmac_scan_proc_scan_complete_event(pst_dmac_vap, MAC_SCAN_SUCCESS);
            break;
        }
        case WLAN_SCAN_MODE_BACKGROUND_OBSS:
        {
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
            dmac_scan_proc_obss_scan_complete_event(pst_dmac_vap);
#endif
#endif
            break;
        }
        case WLAN_SCAN_MODE_BACKGROUND_PNO:
        {
            /* �Ƿ�ɨ�赽��ƥ���ssid������ǣ��ϱ�ɨ����; �������˯�� */
            if (OAL_TRUE == pst_mac_device->pst_pno_sched_scan_mgmt->en_is_found_match_ssid)
            {
                /* �ϱ�ɨ������¼���ɨ��״̬Ϊ�ɹ� */
                (oal_void)dmac_scan_proc_scan_complete_event(pst_dmac_vap, MAC_SCAN_PNO);

                /* �ͷ�PNO����ṹ���ڴ� */
                OAL_MEM_FREE(pst_mac_device->pst_pno_sched_scan_mgmt, OAL_TRUE);
                pst_mac_device->pst_pno_sched_scan_mgmt = OAL_PTR_NULL;
            }

            break;
        }
        case WLAN_SCAN_MODE_BACKGROUND_CSA:
        {
            OAM_WARNING_LOG0(0, OAM_SF_SCAN, "{dmac_scan_end::scan_mode BACKGROUND_CSA}");
            dmac_sta_csa_fsm_post_event(&(pst_dmac_vap->st_vap_base_info), WLAN_STA_CSA_EVENT_SCAN_END, 0, OAL_PTR_NULL);
            break;
        }

#ifdef _PRE_WLAN_FEATURE_11K
        case WLAN_SCAN_MODE_RRM_BEACON_REQ:
        {
            /* ɨ�����������Ӧ֡����䲢���� */
            /* �������֡�ڴ沢���ͷ����Ϣ */
            dmac_rrm_encap_and_send_bcn_rpt(pst_dmac_vap);
            break;
        }
#endif

#ifdef _PRE_WLAN_FEATURE_FTM
        case WLAN_SCAN_MODE_FTM_REQ:
        {
            break;
        }
#endif

#ifdef _PRE_WLAN_FEATURE_GNSS_SCAN
        case WLAN_SCAN_MODE_GNSS_SCAN:
        {
            pst_dmac_device = dmac_res_get_mac_dev(pst_mac_device->uc_device_id);
            pst_dmac_device->st_scan_for_gnss_info.ul_scan_end_timstamps = (oal_uint32)OAL_TIME_GET_STAMP_MS();
            OAM_WARNING_LOG0(0, OAM_SF_SCAN, "{dmac_scan_end::scan_mode gscan}");
            break;
        }
#endif

        default:
        {
            OAM_ERROR_LOG1(0, OAM_SF_SCAN, "{dmac_scan_end::scan_mode[%d] error.}", en_scan_mode);
            break;
        }
    }

#ifdef _PRE_WLAN_FEATURE_BTCOEX
    hal_set_btcoex_soc_gpreg1(OAL_FALSE, BIT1, 1);   // �������̽���

    if (WLAN_P2P_DEV_MODE == pst_dmac_vap->st_vap_base_info.en_p2p_mode)
    {
        hal_set_btcoex_soc_gpreg0(OAL_FALSE, BIT14, 14);   // p2p ɨ�����̽���
    }

    hal_coex_sw_irq_set(HAL_COEX_SW_IRQ_BT);
#endif

#ifdef _PRE_WLAN_FEATURE_ANTI_INTERF
    dmac_alg_anti_intf_switch(pst_dmac_vap->pst_hal_device, OAL_TRUE);
#endif

    /* �ָ�deviceɨ��״̬Ϊ����״̬ */
    pst_mac_device->en_curr_scan_state = MAC_SCAN_STATE_IDLE;
    pst_mac_device->st_scan_params.en_scan_mode = WLAN_SCAN_MODE_BUTT;

    return;
}

oal_void dmac_prepare_fast_scan_end_in_dbdc(mac_device_stru *pst_mac_device, hal_to_dmac_device_stru *pst_hal_device,
                                                    hal_to_dmac_device_stru   *pst_other_hal_device, dmac_vap_stru  *pst_dmac_vap)
{
    dmac_single_hal_device_scan_complete(pst_mac_device, pst_hal_device, pst_dmac_vap, HAL_DEVICE_EVENT_SCAN_END);
    if (MAC_SCAN_STATE_IDLE == pst_other_hal_device->st_hal_scan_params.en_curr_scan_state)
    {
        dmac_scan_end(pst_mac_device);
    }
}

oal_void dmac_prepare_fast_scan_end_in_normal(mac_device_stru *pst_mac_device, hal_to_dmac_device_stru *pst_hal_device,
                                                    hal_to_dmac_device_stru   *pst_other_hal_device, dmac_vap_stru  *pst_dmac_vap)
{
    hal_to_dmac_device_stru    *pst_master_hal_device = OAL_PTR_NULL;
    hal_to_dmac_device_stru    *pst_slave_hal_device  = OAL_PTR_NULL;

    if (pst_other_hal_device->st_hal_scan_params.uc_scan_chan_idx >= pst_other_hal_device->st_hal_scan_params.uc_channel_nums)
    {
        if (HAL_DEVICE_ID_MASTER == pst_other_hal_device->uc_device_id)
        {
            pst_master_hal_device = pst_other_hal_device;
            pst_slave_hal_device  = pst_hal_device;
        }
        else
        {
            pst_master_hal_device = pst_hal_device;
            pst_slave_hal_device  = pst_other_hal_device;
        }

        dmac_single_hal_device_scan_complete(pst_mac_device, pst_slave_hal_device, pst_dmac_vap, HAL_DEVICE_EVENT_SCAN_END);//�ȸ�·ɨ������е�idle
        dmac_single_hal_device_scan_complete(pst_mac_device, pst_master_hal_device, pst_dmac_vap, HAL_DEVICE_EVENT_SCAN_END);//��·ɨ�����
        dmac_scan_end(pst_mac_device);
    }
}

OAL_STATIC oal_void dmac_scan_prepare_fscan_end(dmac_device_stru  *pst_dmac_device, hal_to_dmac_device_stru *pst_hal_device, dmac_vap_stru *pst_dmac_vap)
{
    hal_to_dmac_device_stru                *pst_other_hal_device  = OAL_PTR_NULL;
    mac_device_stru                        *pst_mac_device = pst_dmac_device->pst_device_base_info;

    pst_other_hal_device = dmac_device_get_another_h2d_dev(pst_dmac_device, pst_hal_device);
    if (OAL_PTR_NULL == pst_other_hal_device)
    {
        OAM_ERROR_LOG1(pst_mac_device->st_scan_params.uc_vap_id, OAM_SF_SCAN, "{dmac_scan_prepare_fscan_end::dmac_device_get_another_h2d_dev ori hal[%d]}", pst_hal_device->uc_device_id);
        return;
    }

    /* ��·����������һ·�Ƿ�scan pause,trueҪ�ָ�����һ·��ɨ�� */
    if (oal_bit_get_bit_one_byte(pst_other_hal_device->st_hal_scan_params.uc_scan_pause_bitmap, HAL_SCAN_PASUE_TYPE_CHAN_CONFLICT))
    {
        hal_device_handle_event(pst_other_hal_device, HAL_DEVICE_EVENT_SCAN_RESUME_FROM_CHAN_CONFLICT, 0, OAL_PTR_NULL);
    }

    if (OAL_TRUE == pst_mac_device->en_dbdc_running)
    {
        dmac_prepare_fast_scan_end_in_dbdc(pst_mac_device, pst_hal_device, pst_other_hal_device, pst_dmac_vap);
    }
    else
    {
        dmac_prepare_fast_scan_end_in_normal(pst_mac_device, pst_hal_device, pst_other_hal_device, pst_dmac_vap);
    }
}

oal_void  dmac_scan_prepare_end(mac_device_stru *pst_mac_device, hal_to_dmac_device_stru *pst_hal_device)
{
    dmac_device_stru                       *pst_dmac_device;
    dmac_vap_stru                          *pst_dmac_vap;

    pst_dmac_device = dmac_res_get_mac_dev(pst_mac_device->uc_device_id);
    if (OAL_PTR_NULL == pst_dmac_device)
    {
        OAM_ERROR_LOG0(pst_mac_device->uc_device_id, OAM_SF_SCAN, "{dmac_scan_prepare_end::pst_dmac_device null.}");
        return;
    }

    /* ��ȡ����ɨ���dmac vap�ṹ��Ϣ */
    pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_mac_device->st_scan_params.uc_vap_id);
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_WARNING_LOG0(pst_mac_device->st_scan_params.uc_vap_id, OAM_SF_SCAN, "{dmac_scan_prepare_end::mac_res_get_dmac_vap fail}");
        return;
    }

    /* ����ɨ����Ҫ�ȸ�·��ȥ����·����,�Ա���·��mimo��������� */
    if (OAL_TRUE == pst_dmac_device->en_is_fast_scan)
    {
        dmac_scan_prepare_fscan_end(pst_dmac_device, pst_hal_device, pst_dmac_vap);
    }
    else
    {
#ifdef _PRE_WLAN_FEATURE_BTCOEX
        /* end�¼�Ҫ����btcoex��ps״̬��psʹ��ʱ����Ҫ�ӳ����ָ�end������״̬��Ӱ�� */
        if (HAL_BTCOEX_SW_POWSAVE_WORK == GET_HAL_BTCOEX_SW_PREEMPT_TYPE(pst_hal_device))
        {
            GET_HAL_BTCOEX_SW_PREEMPT_TYPE(pst_hal_device) = HAL_BTCOEX_SW_POWSAVE_SCAN_END;
            OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_COEX, "{dmac_scan_prepare_end:: normal scan end delay by btcoex!}");
        }
        else
#endif
        {
            dmac_single_hal_device_scan_complete(pst_mac_device, pst_hal_device, pst_dmac_vap, HAL_DEVICE_EVENT_SCAN_END);
            dmac_scan_end(pst_mac_device);
        }
    }
}


oal_void  dmac_scan_abort(mac_device_stru *pst_mac_device)
{
    oal_uint8                               uc_device_id;
    oal_uint8                               uc_device_max;
    dmac_vap_stru                          *pst_dmac_vap;
    hal_to_dmac_device_stru                *pst_hal_device;
    dmac_device_stru                       *pst_dmac_device;

    /* ������ɨ��״̬�����߲�����CCAɨ��״̬��ֱ�ӷ��ء�����CCAɨ�����ͨɨ���а����ȥ,�˴�CCA�߼�����ɾ����TBD */
    if (MAC_SCAN_STATE_RUNNING != pst_mac_device->en_curr_scan_state)
    {
        return;
    }

    pst_dmac_vap = mac_res_get_dmac_vap(pst_mac_device->st_scan_params.uc_vap_id);
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_device->st_scan_params.uc_vap_id, OAM_SF_SCAN, "{dmac_scan_abort::pst_dmac_vap null.}");
        return;
    }

    pst_dmac_device = dmac_res_get_mac_dev(pst_mac_device->uc_device_id);
    if (OAL_PTR_NULL == pst_dmac_device)
    {
        OAM_ERROR_LOG0(pst_mac_device->uc_device_id, OAM_SF_SCAN, "{dmac_scan_abort::pst_dmac_device null.}");
        return;
    }
    /* HAL�ӿڻ�ȡ֧��device���� */
    hal_chip_get_device_num(pst_mac_device->uc_chip_id, &uc_device_max);

    /* �����ǲ���ɨ�������ʽ��һ�� */
    if (OAL_FALSE == pst_dmac_device->en_is_fast_scan)
    {
        for (uc_device_id = 0; uc_device_id < uc_device_max; uc_device_id++)
        {
            pst_hal_device = pst_dmac_device->past_hal_device[uc_device_id];
            if (OAL_PTR_NULL == pst_hal_device)
            {
                continue;
            }

            /* ɾ��ɨ�趨ʱ�� */
            if (OAL_TRUE == pst_hal_device->st_hal_scan_params.st_scan_timer.en_is_registerd)
            {
                FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_hal_device->st_hal_scan_params.st_scan_timer));
            }

            if ((MAC_SCAN_STATE_RUNNING == pst_hal_device->st_hal_scan_params.en_curr_scan_state))
            {
                dmac_single_hal_device_scan_complete(pst_mac_device, pst_hal_device, pst_dmac_vap, HAL_DEVICE_EVENT_SCAN_ABORT);
            }
        }
    }
    else
    {
        pst_hal_device = DMAC_DEV_GET_SLA_HAL_DEV(pst_dmac_device);
        if (pst_hal_device != OAL_PTR_NULL)
        {
            dmac_single_hal_device_scan_complete(pst_mac_device, pst_hal_device, pst_dmac_vap, HAL_DEVICE_EVENT_SCAN_ABORT);//�����ȸ�·ɨ������е�idle
        }
        dmac_single_hal_device_scan_complete(pst_mac_device, DMAC_DEV_GET_MST_HAL_DEV(pst_dmac_device), pst_dmac_vap, HAL_DEVICE_EVENT_SCAN_ABORT);//��·ɨ�����
    }

    pst_mac_device->st_scan_params.en_abort_scan_flag = OAL_TRUE;
    dmac_scan_end(pst_mac_device);
    pst_mac_device->st_scan_params.en_abort_scan_flag = OAL_FALSE;

    OAM_WARNING_LOG0(0, OAM_SF_SCAN, "dmac_scan_abort: scan has been aborted");
}


OAL_STATIC oal_uint32 dmac_scan_get_ssid_ie_info(mac_device_stru *pst_mac_device, oal_int8 *pc_ssid, oal_uint8  uc_index)
{
    dmac_vap_stru     *pst_dmac_vap;

    pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_mac_device->st_scan_params.uc_vap_id);
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_device->st_scan_params.uc_vap_id, OAM_SF_SCAN, "{dmac_scan_get_ssid_ie_info::pst_dmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (IS_LEGACY_VAP(&(pst_dmac_vap->st_vap_base_info)))
    {
        /* ���������ţ�׼��probe req֡��ssid��Ϣ */
        oal_memcopy(pc_ssid, pst_mac_device->st_scan_params.ast_mac_ssid_set[uc_index].auc_ssid, WLAN_SSID_MAX_LEN);
    }
    else
    {
        /* P2P �豸ɨ�裬��Ҫ��ȡָ��ssid ��Ϣ����P2P �豸��ɨ��ʱֻɨ��һ��ָ��ssid */
        oal_memcopy(pc_ssid, pst_mac_device->st_scan_params.ast_mac_ssid_set[0].auc_ssid, WLAN_SSID_MAX_LEN);
    }

    return OAL_SUCC;
}



OAL_STATIC oal_uint32 dmac_scan_send_bcast_probe(mac_device_stru *pst_mac_device, oal_uint8 uc_band, oal_uint8  uc_index)
{
    oal_int8           ac_ssid[WLAN_SSID_MAX_LEN] = {'\0'};
    dmac_vap_stru     *pst_dmac_vap;
    oal_uint32         ul_ret;
    oal_uint8          uc_band_tmp;

    if (0 == pst_mac_device->uc_vap_num)
    {
        OAM_WARNING_LOG0(0, OAM_SF_SCAN, "{dmac_scan_send_bcast_probe::uc_vap_num=0.}");
        return OAL_FAIL;
    }

    pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_mac_device->st_scan_params.uc_vap_id);
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{dmac_scan_send_bcast_probe::pst_dmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    
    if(OAL_PTR_NULL == pst_dmac_vap->st_vap_base_info.pst_mib_info)
    {
        OAM_ERROR_LOG4(0, OAM_SF_SCAN, "{dmac_scan_send_bcast_probe:: vap mib info is null,uc_vap_id[%d], p_fn_cb[%p], uc_scan_func[%d], uc_curr_channel_scan_count[%d].}",
                   pst_mac_device->st_scan_params.uc_vap_id,
                   pst_mac_device->st_scan_params.p_fn_cb,
                   pst_mac_device->st_scan_params.uc_scan_func,
                   pst_dmac_vap->pst_hal_device->st_hal_scan_params.uc_curr_channel_scan_count);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��̽������ʱ����Ҫ��ʱ����vap��band��Ϣ����ֹ5G��11b */
    uc_band_tmp = pst_dmac_vap->st_vap_base_info.st_channel.en_band;

    pst_dmac_vap->st_vap_base_info.st_channel.en_band = uc_band;

    /* ��ȡ����ɨ������֡����ҪЯ����ssid ie��Ϣ */
    ul_ret = dmac_scan_get_ssid_ie_info(pst_mac_device, ac_ssid, uc_index);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(0, OAM_SF_SCAN, "{dmac_scan_send_bcast_probe::get ssid failed, error[%d].}", ul_ret);
        return ul_ret;
    }

    /* ����probe req֡ */
    ul_ret = dmac_scan_send_probe_req_frame(pst_dmac_vap, BROADCAST_MACADDR, ac_ssid);

    pst_dmac_vap->st_vap_base_info.st_channel.en_band = uc_band_tmp;

    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{dmac_scan_send_bcast_probe::dmac_mgmt_send_probe_request failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_void  dmac_scan_switch_home_channel_work(mac_device_stru *pst_mac_device, hal_to_dmac_device_stru *pst_hal_device)
{
    hal_scan_params_stru       *pst_scan_params;

    pst_scan_params = &(pst_hal_device->st_hal_scan_params);

    /* �лع����ŵ�����ʱ�������Ƿ�Ϊ���mac addrɨ�裬�ָ�vapԭ�ȵ�mac addr */
    dmac_scan_set_vap_mac_addr_by_scan_state(pst_mac_device, pst_hal_device, OAL_FALSE);

    /* ����ɨ�� �лع����ŵ� */
    dmac_scan_switch_channel_back(pst_mac_device, pst_hal_device);

    hal_device_handle_event(pst_hal_device, HAL_DEVICE_EVENT_SCAN_SWITCH_CHANNEL_BACK, 0, OAL_PTR_NULL);

    /* ������ */
    if (0 == pst_scan_params->us_work_time_on_home_channel)
    {
        OAM_WARNING_LOG1(0, OAM_SF_SCAN, "{dmac_scan_switch_home_channel_work:work_time_on_home_channel is 0, set it to default [%d]ms!}",MAC_WORK_TIME_ON_HOME_CHANNEL_DEFAULT);
        pst_scan_params->us_work_time_on_home_channel = MAC_WORK_TIME_ON_HOME_CHANNEL_DEFAULT;
    }

    /* ���������ʱ�����ڹ����ŵ�����һ��ʱ����л�ɨ���ŵ�����ɨ�� */
    FRW_TIMER_CREATE_TIMER(&(pst_scan_params->st_scan_timer),
                           dmac_scan_switch_home_channel_work_timeout,
                           pst_scan_params->us_work_time_on_home_channel,
                           pst_hal_device,
                           OAL_FALSE,
                           OAM_MODULE_ID_DMAC,
                           pst_mac_device->ul_core_id);

    return;
}


OAL_STATIC oal_uint32  dmac_scan_switch_home_channel_work_timeout(void *p_arg)
{
    mac_device_stru         *pst_mac_device;
    dmac_vap_stru           *pst_dmac_vap;
    hal_to_dmac_device_stru *pst_hal_device = (hal_to_dmac_device_stru *)p_arg;

    pst_mac_device = mac_res_get_dev(pst_hal_device->uc_mac_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{dmac_scan_switch_home_channel_work_timeout::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* �ж��Ƿ���Ҫ��������ɨ�裬�����ʱɨ��״̬Ϊ������״̬��˵��ɨ���Ѿ�ֹͣ�������ټ���ɨ�� */
    if (MAC_SCAN_STATE_RUNNING != pst_mac_device->en_curr_scan_state)
    {
        OAM_WARNING_LOG0(pst_mac_device->st_scan_params.uc_vap_id, OAM_SF_SCAN,
                         "{dmac_scan_switch_home_channel_work_timeout::scan has been aborted, no need to continue.}");
        return OAL_SUCC;
    }

    pst_dmac_vap = mac_res_get_dmac_vap(pst_mac_device->st_scan_params.uc_vap_id);
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_device->st_scan_params.uc_vap_id, OAM_SF_SCAN, "{dmac_scan_switch_home_channel_work_timeout::pst_dmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
    dmac_scan_check_scan_timer(pst_hal_device);
#endif

    dmac_scan_one_channel_start(pst_hal_device, OAL_TRUE);

    /* ����ŵ�������� */
    OAL_MEMZERO(&(pst_hal_device->st_chan_result), OAL_SIZEOF(wlan_scan_chan_stats_stru));

    return OAL_SUCC;
}
#if 0

oal_void dmac_scan_radar_detected(mac_device_stru *pst_mac_device, hal_radar_det_event_stru *pst_radar_det_info)
{
    pst_mac_device->st_chan_result.uc_radar_detected = 1;
    pst_mac_device->st_chan_result.uc_radar_bw       = 0;
}
#endif


OAL_STATIC oal_uint32  dmac_scan_report_channel_statistics_result(hal_to_dmac_device_stru  *pst_hal_device, oal_uint8 uc_scan_idx)
{
    frw_event_mem_stru         *pst_event_mem;
    frw_event_stru             *pst_event;
    dmac_crx_chan_result_stru  *pst_chan_result_param;

    /* ���ŵ�ɨ������HMAC, �����¼��ڴ� */
    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(dmac_crx_chan_result_stru));
    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{dmac_scan_report_channel_statistics_result::alloc mem fail.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��д�¼� */
    pst_event = frw_get_event_stru(pst_event_mem);

    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_WLAN_CRX,
                       DMAC_WLAN_CRX_EVENT_SUB_TYPE_CHAN_RESULT,
                       OAL_SIZEOF(dmac_crx_chan_result_stru),
                       FRW_EVENT_PIPELINE_STAGE_1,
                       pst_hal_device->uc_chip_id,
                       pst_hal_device->uc_mac_device_id,
                       0);

    pst_chan_result_param = (dmac_crx_chan_result_stru *)(pst_event->auc_event_data);

    pst_chan_result_param->uc_scan_idx    = uc_scan_idx;
    pst_chan_result_param->st_chan_result = pst_hal_device->st_chan_result;

    frw_event_dispatch_event(pst_event_mem);
    FRW_EVENT_FREE(pst_event_mem);

    return OAL_SUCC;
}


oal_uint32 dmac_scan_init(mac_device_stru *pst_device)
{
    /* ��ʼ��deviceɨ��״̬Ϊ���� */
    pst_device->en_curr_scan_state = MAC_SCAN_STATE_IDLE;

#ifdef _PRE_WLAN_FEATURE_GNSS_SCAN
    dmac_scan_init_bss_info_list(pst_device);
#endif

    return OAL_SUCC;
}



oal_uint32 dmac_scan_exit(mac_device_stru *pst_device)
{
    return OAL_SUCC;
}

// TODO:����������ݲ�׼:1��5g�״��ŵ�ʱ�䲻��2����home channel�ĸ������Ǵ���ο�ʼ����
oal_uint32 dmac_scan_get_remaining_time(hal_to_dmac_device_stru *pst_hal_device, oal_uint16 *pus_scan_remain_time)
{
    oal_uint8                uc_remain_scan_count;   //ĳ�ŵ�ʣ��ɨ�����
    oal_uint8                uc_remain_channel_nums; //�Ѿ�ɨ����ŵ�����
    oal_uint16               us_need_scan_channel_time;
    oal_uint16               us_need_work_on_home_channel_time;
    hal_scan_params_stru    *pst_hal_scan_params;
    mac_device_stru         *pst_mac_device;

    pst_mac_device =  mac_res_get_dev(pst_hal_device->uc_device_id);
    if (pst_mac_device->en_curr_scan_state != MAC_SCAN_STATE_RUNNING)
    {
        return OAL_FAIL;
    }

    /* ��ȡɨ����� */
    pst_hal_scan_params = &(pst_hal_device->st_hal_scan_params);

    /* ʣ��ɨ���ŵ���,�ܹ���-��ɨ��� */
    uc_remain_channel_nums = pst_hal_scan_params->uc_channel_nums - (pst_hal_scan_params->uc_scan_chan_idx - pst_hal_scan_params->uc_start_chan_idx);

    /* ĳ�ŵ�ʣ��ɨ����� */
    uc_remain_scan_count = pst_hal_scan_params->uc_max_scan_count_per_channel - pst_hal_scan_params->uc_curr_channel_scan_count;

    /*ʣ�൥ɨ���ŵ���Ҫ��ʱ�� = һ��ɨ��ʱ��*(��ǰ�ŵ�ʣ�����+ʣ���ŵ�ɨ�����)*/
    us_need_scan_channel_time = (oal_uint16)(pst_hal_scan_params->us_scan_time * (uc_remain_scan_count + uc_remain_channel_nums * pst_hal_scan_params->uc_max_scan_count_per_channel));

    /* ��Ҫ�ڹ����ŵ�������ʣ��ʱ�� = (ʣ���ŵ���/����) * ����ʱ�� */
    us_need_work_on_home_channel_time = (oal_uint16)((uc_remain_channel_nums / pst_hal_scan_params->uc_scan_channel_interval) * pst_hal_scan_params->us_work_time_on_home_channel);

    *pus_scan_remain_time = us_need_scan_channel_time + us_need_work_on_home_channel_time;

    OAM_WARNING_LOG1(0, OAM_SF_SCAN, "{dmac_scan_get_remaining_time::scan remain time[%d]ms}", *pus_scan_remain_time);

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_GNSS_SCAN

oal_void dmac_scan_update_gscan_vap_id(mac_vap_stru *pst_mac_vap, oal_uint8 en_is_add_vap)
{
    dmac_device_stru   *pst_dmac_device = dmac_res_get_mac_dev(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_dmac_device)
    {
        OAM_ERROR_LOG1(0, OAM_SF_SCAN, "{dmac_scan_update_gscan_vap_id::dmac_res_get_mac_dev[%d]fail}", pst_mac_vap->uc_device_id);
        return;
    }

    if (IS_LEGACY_STA(pst_mac_vap))
    {
        pst_dmac_device->uc_gscan_mac_vap_id = (OAL_TRUE == en_is_add_vap) ? pst_mac_vap->uc_vap_id : 0Xff;
    }
    else
    {
        OAM_ERROR_LOG3(0, OAM_SF_SCAN, "{dmac_scan_update_gscan_vap_id::vap[%d]mode[%d],not legacy sta[%d]}",
                        pst_mac_vap->uc_vap_id, pst_mac_vap->en_vap_mode, IS_LEGACY_VAP(pst_mac_vap));
    }
}


oal_uint32 dmac_trigger_gscan(oal_ipc_message_header_stru *pst_ipc_message)
{
    oal_uint8               uc_chan_idx;
    oal_uint8               uc_chan_num    = 0;
    oal_uint8               uc_channel_idx = 0;
    oal_uint32              ul_ret;
    mac_scan_req_stru       st_scan_req_params;
    dmac_gscan_params_stru *pst_ipc_scan_params;
    dmac_device_stru       *pst_dmac_device;
    mac_vap_stru           *pst_mac_vap;

    OAL_MEMZERO(&st_scan_req_params, OAL_SIZEOF(mac_scan_req_stru));

    /* ��ȡdmac device */
    pst_dmac_device = dmac_res_get_mac_dev(0);

    if (OAL_TRUE == dmac_device_check_is_vap_in_assoc(pst_dmac_device->pst_device_base_info))
    {
        OAM_WARNING_LOG0(pst_dmac_device->uc_gscan_mac_vap_id, OAM_SF_SCAN, "{dmac_trigger_gscan::vap is in assoc,not start gscan}");
        return OAL_FAIL;
    }

    /* ���÷���ɨ���vap id,Ŀǰд���õ�һ��ҵ��vap,����ע��Ҫʹ��staut,��Ҫʹ��aput */
    st_scan_req_params.uc_vap_id = pst_dmac_device->uc_gscan_mac_vap_id;
    pst_mac_vap = (mac_vap_stru*)mac_res_get_mac_vap(pst_dmac_device->uc_gscan_mac_vap_id);
    if (OAL_PTR_NULL == pst_mac_vap)
    {
        OAM_WARNING_LOG1(pst_dmac_device->uc_gscan_mac_vap_id, OAM_SF_SCAN, "{dmac_trigger_gscan::pst_mac_vap[%d] null.}", pst_dmac_device->uc_gscan_mac_vap_id);
        return OAL_FAIL;
    }

    /* ���ó�ʼɨ������Ĳ��� */
    st_scan_req_params.en_bss_type     = WLAN_MIB_DESIRED_BSSTYPE_INFRA;
    st_scan_req_params.en_scan_type    = WLAN_SCAN_TYPE_ACTIVE;
    st_scan_req_params.en_scan_mode    = WLAN_SCAN_MODE_GNSS_SCAN;
    st_scan_req_params.us_scan_time    = WLAN_DEFAULT_ACTIVE_SCAN_TIME;
    st_scan_req_params.uc_probe_delay  = 0;
    st_scan_req_params.uc_scan_func    = MAC_SCAN_FUNC_BSS;   /* Ĭ��ɨ��bss */
    st_scan_req_params.uc_max_scan_count_per_channel           = 1;  //gscanһ���ŵ�һ��
    st_scan_req_params.uc_max_send_probe_req_count_per_channel = WLAN_DEFAULT_SEND_PROBE_REQ_COUNT_PER_CHANNEL;

    /* ����ɨ���õ�pro req��src mac��ַ*/
    oal_set_mac_addr(st_scan_req_params.auc_sour_mac_addr, mac_mib_get_StationID(pst_mac_vap));
    st_scan_req_params.en_is_random_mac_addr_scan = OAL_FALSE;

    /* ����ɨ�������ssid��Ϣ */
    st_scan_req_params.ast_mac_ssid_set[0].auc_ssid[0] = '\0';   /* ͨ��ssid */
    st_scan_req_params.uc_ssid_num = 1;

    /* ����ɨ������ָֻ��1��bssid��Ϊ�㲥��ַ */
    oal_set_mac_addr(st_scan_req_params.auc_bssid[0], BROADCAST_MACADDR);
    st_scan_req_params.uc_bssid_num = 1;

    pst_ipc_scan_params = (dmac_gscan_params_stru *)((pst_ipc_message->ul_data_addr & 0x000fffff) + GNSS_TO_WIFI_MEM_OFFSET);
#if 0
    for (uc_chan_idx = 0; uc_chan_idx < pst_ipc_scan_params->uc_ch_valid_num; uc_chan_idx++)
    {
        OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{dmac_trigger_gscan::in[%d],ch num[%d]}",
                            pst_ipc_scan_params->ast_wlan_channel[uc_chan_idx].en_band, pst_ipc_scan_params->ast_wlan_channel[uc_chan_idx].uc_chan_number);
    }
#endif
    /* ĿǰgnssֻҪ��2.4gɨ�� */
    for (uc_chan_idx = 0; uc_chan_idx < pst_ipc_scan_params->uc_ch_valid_num; uc_chan_idx++)
    {
        ul_ret = mac_get_channel_idx_from_num(pst_ipc_scan_params->ast_wlan_channel[uc_chan_idx].en_band, pst_ipc_scan_params->ast_wlan_channel[uc_chan_idx].uc_chan_number, &uc_channel_idx);
        if (ul_ret != OAL_SUCC)
        {
            OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{dmac_trigger_gscan::in[%d]band,wrong ch num[%d]}",
                                    pst_ipc_scan_params->ast_wlan_channel[uc_chan_idx].en_band, pst_ipc_scan_params->ast_wlan_channel[uc_chan_idx].uc_chan_number);
            return OAL_FAIL;
        }
        st_scan_req_params.ast_channel_list[uc_chan_num].uc_chan_number = pst_ipc_scan_params->ast_wlan_channel[uc_chan_idx].uc_chan_number;
        st_scan_req_params.ast_channel_list[uc_chan_num].en_band        = pst_ipc_scan_params->ast_wlan_channel[uc_chan_idx].en_band;
        st_scan_req_params.ast_channel_list[uc_chan_num].uc_chan_idx    = uc_channel_idx;
        uc_chan_num++;
    }

    st_scan_req_params.uc_channel_nums = uc_chan_num;

    /* ����ɨ����ڣ�ִ��ɨ�� */
    return dmac_scan_handle_scan_req_entry(pst_dmac_device->pst_device_base_info, MAC_GET_DMAC_VAP(pst_mac_vap), &st_scan_req_params);
}

oal_void dmac_scan_prepare_refuse_reponse_to_gnss(oal_ipc_message_header_stru *pst_req_ipc_message, oal_ipc_message_header_stru *pst_response_message)
{
    mac_vap_stru                *pst_mac_vap;
    oal_uint16                   us_scan_remain_time = 0;
    dmac_device_stru            *pst_dmac_device = dmac_res_get_mac_dev(0);

    pst_response_message->en_cmd = WLAN_REFUSE_REQUEST;

    pst_mac_vap = (mac_vap_stru*)mac_res_get_mac_vap(pst_dmac_device->uc_gscan_mac_vap_id);
    if (OAL_PTR_NULL == pst_mac_vap)
    {
        return;
    }

    /* ���������ɨ��ܾ�gnss�����󣬸�֪���´�����ȡ�����ʱ�� */
    if (MAC_SCAN_STATE_RUNNING == pst_dmac_device->pst_device_base_info->en_curr_scan_state)
    {
        pst_response_message->en_cmd = (BFGX_REQUEST_SCAN == pst_req_ipc_message->en_cmd) ? WLAN_REFUSE_SCAN : WLAN_REFUSE_GET_DATA;
        dmac_scan_get_remaining_time(DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap), &us_scan_remain_time);
        pst_response_message->us_arg16 = us_scan_remain_time;
    }
}

oal_void dmac_handle_gscan_req_event(oal_ipc_message_header_stru *pst_req_ipc_message, oal_ipc_message_header_stru *pst_response_message)
{
    // TODO:��Ҫ����wifi�Լ����жϲ�����gnssɨ��1��������2��������
    if (OAL_SUCC == dmac_trigger_gscan(pst_req_ipc_message))
    {
        pst_response_message->en_cmd = WLAN_ACCEPT_SCAN;
    }
    else
    {
        dmac_scan_prepare_refuse_reponse_to_gnss(pst_req_ipc_message, pst_response_message);
    }
}

oal_void dmac_handle_gnss_get_result_event(oal_ipc_message_header_stru *pst_req_ipc_message, oal_ipc_message_header_stru *pst_response_message)
{
    oal_uint8                      uc_chan_num = 0;
    oal_uint32                     ul_current_time;
    mac_device_stru               *pst_mac_device;
    dmac_device_stru              *pst_dmac_device;
    dmac_scanned_bss_info_stru    *pst_scanned_bss_info;
    dmac_scan_for_gnss_stru       *pst_scan_for_gnss_info;
    oal_dlist_head_stru           *pst_scanned_bss_entry = OAL_PTR_NULL;
    oal_dlist_head_stru           *pst_temp_entry        = OAL_PTR_NULL;;
    dmac_gscan_report_info_stru   *pst_gscan_report_info;
    mac_vap_stru                  *pst_mac_vap;

    pst_response_message->en_cmd = WLAN_REFUSE_GET_DATA;

    /* ��ȡmac device */
    pst_mac_device = mac_res_get_dev(0);
    pst_dmac_device = dmac_res_get_mac_dev(pst_mac_device->uc_device_id);
    if (OAL_PTR_NULL == pst_dmac_device)
    {
        OAM_ERROR_LOG0(pst_mac_device->uc_device_id, OAM_SF_SCAN, "{dmac_handle_gnss_get_result_event::pst_dmac_device null.}");
        return;
    }

    pst_mac_vap = (mac_vap_stru*)mac_res_get_mac_vap(pst_dmac_device->uc_gscan_mac_vap_id);
    if ((MAC_SCAN_STATE_RUNNING == pst_mac_device->en_curr_scan_state) || (OAL_PTR_NULL == pst_mac_vap))
    {
        dmac_scan_prepare_refuse_reponse_to_gnss(pst_req_ipc_message, pst_response_message);
        OAM_WARNING_LOG3(0, OAM_SF_SCAN, "{dmac_handle_gnss_get_result_event::scan mode[%d]scan state[%d],mac vap addr[0x%x]refuse gnss get data",
                                pst_mac_device->st_scan_params.en_scan_mode, pst_mac_device->en_curr_scan_state, pst_mac_vap);
        return;
    }

    pst_response_message->en_cmd = WLAN_SEND_SCAN_DATA;
    pst_gscan_report_info = (dmac_gscan_report_info_stru *)((pst_req_ipc_message->ul_data_addr & 0x000fffff) + GNSS_TO_WIFI_MEM_OFFSET);
    pst_scan_for_gnss_info = &(pst_dmac_device->st_scan_for_gnss_info);

    dmac_scan_dump_bss_list(&(pst_scan_for_gnss_info->st_dmac_scan_info_list));

    ul_current_time = (oal_uint32)OAL_TIME_GET_STAMP_MS();
    pst_gscan_report_info->ul_interval_from_last_scan = OAL_TIME_GET_RUNTIME(ul_current_time, pst_scan_for_gnss_info->ul_scan_end_timstamps);
    OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_scanned_bss_entry, pst_temp_entry, &(pst_scan_for_gnss_info->st_dmac_scan_info_list))
    {
        /* ������ͷ�����һ����Ҫ����������� */
        pst_scanned_bss_info = OAL_DLIST_GET_ENTRY(pst_scanned_bss_entry, dmac_scanned_bss_info_stru, st_entry);
        oal_memcopy(&pst_gscan_report_info->ast_wlan_ap_measurement_info[uc_chan_num], &pst_scanned_bss_info->st_wlan_ap_measurement_info, OAL_SIZEOF(wlan_ap_measurement_info_stru));
        uc_chan_num++;
    }

    pst_gscan_report_info->uc_ap_valid_number = uc_chan_num;
}

oal_uint32 dmac_ipc_irq_event(frw_event_mem_stru *pst_event_mem)
{
    oal_ipc_message_header_stru *pst_ipc_message;
    oal_ipc_message_header_stru  st_response_ipc = {0};

    st_response_ipc.en_cmd = WLAN_REFUSE_REQUEST;
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem)|| (OAL_FALSE == g_uc_wifi_support_gscan))
    {
        OAM_ERROR_LOG2(0, OAM_SF_SCAN, "{dmac_ipc_irq_event::pst_event_mem[0x%x].,gscan_support[%d]}", pst_event_mem, g_uc_wifi_support_gscan);
        IPC_W2B_msg_tx((oal_uint8 *)&st_response_ipc, OAL_SIZEOF(oal_ipc_message_header_stru));
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_ipc_message = (oal_ipc_message_header_stru *)frw_get_event_payload(pst_event_mem);
    if (BFGX_REQUEST_SCAN == pst_ipc_message->en_cmd)
    {
        dmac_handle_gscan_req_event(pst_ipc_message, &st_response_ipc);
    }
    else if (BFGX_GET_SCAN_DATA == pst_ipc_message->en_cmd)
    {
        dmac_handle_gnss_get_result_event(pst_ipc_message, &st_response_ipc);
    }
    else
    {
        OAM_ERROR_LOG1(0, OAM_SF_SCAN, "{dmac_ipc_irq_event::wrong cmd[0x%x]}", pst_ipc_message->en_cmd);
    }

    IPC_W2B_msg_tx((oal_uint8 *)&st_response_ipc, OAL_SIZEOF(oal_ipc_message_header_stru));

    return OAL_SUCC;
}

oal_void  dmac_scan_init_bss_info_list(mac_device_stru *pst_mac_dev)
{
    oal_uint8                            uc_scan_bss_info_ele_idx;
    dmac_device_stru                    *pst_dmac_device;
    dmac_scan_for_gnss_stru             *pst_scan_for_gnss_info;

    pst_dmac_device = dmac_res_get_mac_dev(pst_mac_dev->uc_device_id);
    if (OAL_PTR_NULL == pst_dmac_device)
    {
        OAM_ERROR_LOG1(0, OAM_SF_SCAN, "{dmac_scan_init_bss_info_list::dmac_res_get_mac_dev[%d] null}", pst_mac_dev->uc_device_id);
        return;
    }

    pst_scan_for_gnss_info = &(pst_dmac_device->st_scan_for_gnss_info);
    OAL_MEMZERO(pst_scan_for_gnss_info, OAL_SIZEOF(pst_dmac_device->st_scan_for_gnss_info));

    oal_dlist_init_head(&(pst_scan_for_gnss_info->st_dmac_scan_info_list));
    oal_dlist_init_head(&(pst_scan_for_gnss_info->st_scan_info_res_list));
    for (uc_scan_bss_info_ele_idx = 0; uc_scan_bss_info_ele_idx < DMAC_SCAN_MAX_AP_NUM_TO_GNSS; uc_scan_bss_info_ele_idx++)
    {
        oal_dlist_add_tail(&(pst_scan_for_gnss_info->ast_scan_bss_info_member[uc_scan_bss_info_ele_idx].st_entry), &(pst_scan_for_gnss_info->st_scan_info_res_list));
    }
}
#if 0

oal_void  dmac_scan_free_bss_info_list(oal_dlist_head_stru  *pst_scan_info_list)
{
    dmac_scanned_bss_info_stru           *pst_scanned_bss_info;
    oal_dlist_head_stru                  *pst_dlist_entry;
    oal_dlist_head_stru                  *pst_temp               = OAL_PTR_NULL;

    /* �ͷŵ�������δ������ж���Ϣ��Ա */
    oal_irq_disable();
    OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_dlist_entry, pst_temp, pst_scan_info_list)
    {
        pst_scanned_bss_info = OAL_DLIST_GET_ENTRY(pst_dlist_entry, dmac_scanned_bss_info_stru, st_entry);

        /* ��������ɾ�� */
        oal_dlist_delete_entry(&pst_scanned_bss_info->st_entry);

        /* �ͷű��εĽڵ���Ϣ */
        OAL_MEMZERO(pst_scanned_bss_info, OAL_SIZEOF(dmac_scanned_bss_info_stru));
    }
    oal_irq_enable();
}


OAL_STATIC oal_void dmac_scan_free_bss_info(oal_dlist_head_stru *pst_head, oal_dlist_head_stru *pst_free_bss)
{
    oal_irq_disable();
    oal_dlist_add_tail(pst_free_bss, pst_head);
    oal_irq_enable();
}
#endif

OAL_STATIC dmac_scanned_bss_info_stru *dmac_scan_alloc_bss_info(oal_dlist_head_stru *pst_head)
{
    oal_dlist_head_stru           *pst_scanned_bss_entry = OAL_PTR_NULL;
    dmac_scanned_bss_info_stru    *pst_scanned_bss_info  = OAL_PTR_NULL;

    if (!oal_dlist_is_empty(pst_head))
    {
        pst_scanned_bss_entry = oal_dlist_delete_head(pst_head);
        pst_scanned_bss_info = OAL_DLIST_GET_ENTRY(pst_scanned_bss_entry, dmac_scanned_bss_info_stru, st_entry);
        /* ��������������� */
        OAL_MEMZERO(&(pst_scanned_bss_info->st_wlan_ap_measurement_info), OAL_SIZEOF(pst_scanned_bss_info->st_wlan_ap_measurement_info));
    }
    return pst_scanned_bss_info;
}

OAL_STATIC oal_void dmac_scan_info_add_in_order(oal_dlist_head_stru *pst_new, oal_dlist_head_stru *pst_head)
{
    dmac_scanned_bss_info_stru    *pst_scanned_bss_info;
    dmac_scanned_bss_info_stru    *pst_scanned_bss_info_new;
    oal_dlist_head_stru           *pst_scanned_bss_entry = OAL_PTR_NULL;
    oal_dlist_head_stru           *pst_temp_entry        = OAL_PTR_NULL;;

    pst_scanned_bss_info_new = OAL_DLIST_GET_ENTRY(pst_new, dmac_scanned_bss_info_stru, st_entry);
    OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_scanned_bss_entry, pst_temp_entry, pst_head)
    {
        /* ������ͷ�����һ����Ҫ����������� */
        pst_scanned_bss_info = OAL_DLIST_GET_ENTRY(pst_scanned_bss_entry, dmac_scanned_bss_info_stru, st_entry);
        /* ���������rssi��ǰ�� */
        if (pst_scanned_bss_info_new->st_wlan_ap_measurement_info.c_rssi >= pst_scanned_bss_info->st_wlan_ap_measurement_info.c_rssi)
        {
            break;
        }
    }

    /* ��ӵ�һ���ڵ�ͼ�������β�� */
    if ((pst_scanned_bss_entry != OAL_PTR_NULL) && (pst_scanned_bss_entry->pst_prev != OAL_PTR_NULL))
    {
        oal_dlist_add(pst_new, pst_scanned_bss_entry->pst_prev, pst_scanned_bss_entry);
    }
    else
    {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{dmac_scan_info_add_in_order::scan info list is broken !}");
    }
}

OAL_STATIC oal_void dmac_scan_dump_bss_list(oal_dlist_head_stru *pst_head)
{
    dmac_scanned_bss_info_stru    *pst_scanned_bss_info;
    wlan_ap_measurement_info_stru *pst_wlan_ap_measurement_info;
    oal_dlist_head_stru           *pst_scanned_bss_entry = OAL_PTR_NULL;
    oal_dlist_head_stru           *pst_temp_entry        = OAL_PTR_NULL;;

    OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_scanned_bss_entry, pst_temp_entry, pst_head)
    {
        /* ������ͷ�����һ����Ҫ����������� */
        pst_scanned_bss_info = OAL_DLIST_GET_ENTRY(pst_scanned_bss_entry, dmac_scanned_bss_info_stru, st_entry);
        pst_wlan_ap_measurement_info = &(pst_scanned_bss_info->st_wlan_ap_measurement_info);
        OAM_WARNING_LOG_ALTER(0, OAM_SF_SCAN, "{dmac_scan_dump_bss_info_list::mac addr[%x][%x][%x][%x][%x][%x]rssi[%d]channel[%d]", 8,
                      pst_wlan_ap_measurement_info->auc_bssid[0], pst_wlan_ap_measurement_info->auc_bssid[1], pst_wlan_ap_measurement_info->auc_bssid[2],
                      pst_wlan_ap_measurement_info->auc_bssid[3], pst_wlan_ap_measurement_info->auc_bssid[4], pst_wlan_ap_measurement_info->auc_bssid[5],
                      pst_wlan_ap_measurement_info->c_rssi,
                      pst_wlan_ap_measurement_info->uc_channel_num);
    }
}

OAL_STATIC oal_void dmac_scan_check_ap_bss_info(oal_dlist_head_stru *pst_head)
{
    dmac_scanned_bss_info_stru    *pst_scanned_bss_info;
    wlan_ap_measurement_info_stru *pst_wlan_ap_measurement_info;
    oal_dlist_head_stru           *pst_scanned_bss_entry = OAL_PTR_NULL;
    oal_dlist_head_stru           *pst_temp_entry        = OAL_PTR_NULL;
    oal_int8                       c_last_rssi = 0x7F;

    OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_scanned_bss_entry, pst_temp_entry, pst_head)
    {
        /* ������ͷ�����һ����Ҫ����������� */
        pst_scanned_bss_info = OAL_DLIST_GET_ENTRY(pst_scanned_bss_entry, dmac_scanned_bss_info_stru, st_entry);
        pst_wlan_ap_measurement_info = &(pst_scanned_bss_info->st_wlan_ap_measurement_info);
        if (c_last_rssi < pst_wlan_ap_measurement_info->c_rssi)
        {
            OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{dmac_scan_check_ap_info::not max in head!!!}");
            dmac_scan_dump_bss_list(pst_head);
            break;
        }
        c_last_rssi = pst_wlan_ap_measurement_info->c_rssi;
    }
}


OAL_STATIC dmac_scanned_bss_info_stru *dmac_scan_find_scanned_bss_by_bssid(oal_dlist_head_stru *pst_head, oal_uint8  *puc_bssid)
{
    dmac_scanned_bss_info_stru    *pst_scanned_bss_info  = OAL_PTR_NULL;
    oal_dlist_head_stru           *pst_scanned_bss_entry = OAL_PTR_NULL;
    oal_dlist_head_stru           *pst_temp_entry        = OAL_PTR_NULL;

    OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_scanned_bss_entry, pst_temp_entry, pst_head)
    {
        pst_scanned_bss_info = OAL_DLIST_GET_ENTRY(pst_scanned_bss_entry, dmac_scanned_bss_info_stru, st_entry);

        /* ��ͬ��bssid��ַ */
        if (0 == oal_compare_mac_addr(pst_scanned_bss_info->st_wlan_ap_measurement_info.auc_bssid, puc_bssid))
        {
            return pst_scanned_bss_info;
        }
    }

    return OAL_PTR_NULL;
}


OAL_STATIC oal_void dmac_scan_proc_scanned_bss(mac_device_stru *pst_mac_device, oal_netbuf_stru *pst_netbuf)
{
    oal_uint8                                uc_frame_channel;
    dmac_rx_ctl_stru                        *pst_rx_ctrl;
    mac_ieee80211_frame_stru                *pst_frame_hdr;
    oal_uint8                               *puc_frame_body;
    dmac_scanned_bss_info_stru              *pst_scanned_bss_info;
    dmac_device_stru                        *pst_dmac_device;
    oal_dlist_head_stru                     *pst_scanned_bss_entry = OAL_PTR_NULL;
    oal_dlist_head_stru                     *pst_scan_info_list = OAL_PTR_NULL;
    oal_dlist_head_stru                     *pst_scan_info_res_list = OAL_PTR_NULL;
    oal_uint16                               us_frame_len;
    oal_uint16                               us_offset =  MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN + MAC_CAP_INFO_LEN;

    pst_dmac_device = dmac_res_get_mac_dev(pst_mac_device->uc_device_id);
    if (OAL_PTR_NULL == pst_dmac_device)
    {
        OAM_ERROR_LOG0(pst_mac_device->uc_device_id, OAM_SF_SCAN, "{dmac_scan_proc_scanned_bss::pst_dmac_device null.}");
        return;
    }

    pst_rx_ctrl    = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
    pst_frame_hdr  = (mac_ieee80211_frame_stru *)mac_get_rx_cb_mac_hdr(&(pst_rx_ctrl->st_rx_info));
    puc_frame_body = MAC_GET_RX_PAYLOAD_ADDR(pst_rx_ctrl, pst_netbuf);
    us_frame_len   = MAC_GET_RX_CB_PAYLOAD_LEN(&(pst_rx_ctrl->st_rx_info));  /*֡�峤��*/

    pst_scan_info_list     = &(pst_dmac_device->st_scan_for_gnss_info.st_dmac_scan_info_list);
    pst_scan_info_res_list = &(pst_dmac_device->st_scan_for_gnss_info.st_scan_info_res_list);
    pst_scanned_bss_info = dmac_scan_find_scanned_bss_by_bssid(pst_scan_info_list, pst_frame_hdr->auc_address3);
    /* ��ͬbssid���Ѿ���ɨ����������,��������ժ��,update��������add */
    if (pst_scanned_bss_info != OAL_PTR_NULL)
    {
        /* ɨ����ĲŸ��� */
        if (pst_scanned_bss_info->st_wlan_ap_measurement_info.c_rssi >= pst_rx_ctrl->st_rx_statistic.c_rssi_dbm)
        {
            return;
        }
        oal_dlist_delete_entry(&pst_scanned_bss_info->st_entry);
    }
    else
    {
        if (OAL_TRUE == oal_dlist_is_empty(pst_scan_info_res_list))
        {
            /* ɨ����Ĳ�ɾȥ��С�� */
            pst_scanned_bss_info = OAL_DLIST_GET_ENTRY(pst_scan_info_list->pst_prev, dmac_scanned_bss_info_stru, st_entry);
            if (pst_scanned_bss_info->st_wlan_ap_measurement_info.c_rssi >= pst_rx_ctrl->st_rx_statistic.c_rssi_dbm)
            {
                return;
            }

            /* �޿�����Դ,��info listɾ��rssi��С�Ľڵ�,�ٷŻ�free list */
            pst_scanned_bss_entry = oal_dlist_delete_tail(pst_scan_info_list);
            oal_dlist_add_tail(pst_scanned_bss_entry, pst_scan_info_res_list);
        }
        pst_scanned_bss_info = dmac_scan_alloc_bss_info(pst_scan_info_res_list);
        if (OAL_PTR_NULL == pst_scanned_bss_info)
        {
            OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{dmac_scan_proc_scanned_bss::info res not empty but alloc fail}");
            dmac_scan_dump_bss_list(pst_scan_info_res_list);
            dmac_scan_dump_bss_list(pst_scan_info_list);
            return;
        }
    }
    /* ��ȡ����֡�е��ŵ� */
    uc_frame_channel = mac_ie_get_chan_num(puc_frame_body, us_frame_len,
                       us_offset, pst_rx_ctrl->st_rx_info.uc_channel_number);

    oal_set_mac_addr(pst_scanned_bss_info->st_wlan_ap_measurement_info.auc_bssid, pst_frame_hdr->auc_address3);
    pst_scanned_bss_info->st_wlan_ap_measurement_info.c_rssi = pst_rx_ctrl->st_rx_statistic.c_rssi_dbm;
    pst_scanned_bss_info->st_wlan_ap_measurement_info.uc_channel_num = uc_frame_channel;
    dmac_scan_info_add_in_order(&(pst_scanned_bss_info->st_entry), pst_scan_info_list);
}

#endif


oal_void  dmac_scan_get_ch_statics_measurement_result_ram(mac_device_stru *pst_mac_device, hal_to_dmac_device_stru *pst_hal_device)
{
    hal_ch_statics_irq_event_stru    st_stats_result;
    hal_ch_mac_statics_stru          st_mac_stats;
    oal_uint8                        uc_chan_idx;
    oal_uint32                       ul_trx_time_us = 0;
    hal_ch_mac_statics_stru         *pst_mac_stats;
    hal_to_dmac_device_rom_stru     *pst_hal_dev_rom;

    /* ��ȡ��� */
    OAL_MEMZERO(&st_stats_result, OAL_SIZEOF(st_stats_result));
    OAL_MEMZERO(&st_mac_stats, OAL_SIZEOF(st_mac_stats));

    /* MACͳ����Ϣ */
    hal_get_txrx_frame_time(pst_hal_device, &st_mac_stats);
    hal_get_ch_statics_result(pst_hal_device, &st_stats_result);
    /* PHY������Ϣ */
    hal_get_ch_measurement_result_ram(pst_hal_device, &st_stats_result);

#if defined(_PRE_WLAN_CHIP_TEST) && defined(_PRE_SUPPORT_ACS) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    dmac_acs_channel_meas_comp_handler(pst_hal_device, &st_stats_result);
#endif

#ifdef _PRE_WLAN_FEATURE_STA_PM
    /* ��ǰ��ͳ�����������PA�أ���ǰ��ͳ�����ڲ��������� */
    if (OAL_TRUE == pst_hal_device->en_intf_det_invalid)
    {
        if (OAL_TRUE == pst_hal_device->en_is_mac_pa_enabled)
        {
            pst_hal_device->en_intf_det_invalid = OAL_FALSE;
        }
        return;
    }
#endif

    uc_chan_idx  = GET_HAL_DEV_CURRENT_SCAN_IDX(pst_hal_device);

    /* PHY�ŵ�����ͳ����Ϣ */
    pst_hal_device->st_chan_result.uc_stats_valid = 1;
    pst_hal_device->st_chan_result.uc_channel_number = pst_mac_device->st_scan_params.ast_channel_list[uc_chan_idx].uc_chan_number;

    if (st_stats_result.c_pri20_idle_power < 0)
    {
        pst_hal_device->st_chan_result.s_free_power_stats_20M  += (oal_int8)st_stats_result.c_pri20_idle_power; /* ��20M�ŵ����й��� */
        pst_hal_device->st_chan_result.s_free_power_stats_40M  += (oal_int8)st_stats_result.c_pri40_idle_power; /* ��40M�ŵ����й��� */
        pst_hal_device->st_chan_result.s_free_power_stats_80M  += (oal_int8)st_stats_result.c_pri80_idle_power; /* ȫ��80M�ŵ����й��� */
        pst_hal_device->st_chan_result.uc_free_power_cnt += 1;
    }

    /* MAC�ŵ�����ͳ��ʱ�� */
    ul_trx_time_us = st_stats_result.ul_ch_rx_time_us + st_stats_result.ul_ch_tx_time_us;
    st_stats_result.ul_pri20_free_time_us = DMAC_SCAN_GET_VALID_FREE_TIME(ul_trx_time_us, st_stats_result.ul_ch_stats_time_us, st_stats_result.ul_pri20_free_time_us);
    st_stats_result.ul_pri40_free_time_us = DMAC_SCAN_GET_VALID_FREE_TIME(ul_trx_time_us, st_stats_result.ul_ch_stats_time_us, st_stats_result.ul_pri40_free_time_us);
    st_stats_result.ul_pri80_free_time_us = DMAC_SCAN_GET_VALID_FREE_TIME(ul_trx_time_us, st_stats_result.ul_ch_stats_time_us, st_stats_result.ul_pri80_free_time_us);

    pst_hal_device->st_chan_result.ul_total_free_time_20M_us += st_stats_result.ul_pri20_free_time_us;
    pst_hal_device->st_chan_result.ul_total_free_time_40M_us += st_stats_result.ul_pri40_free_time_us;
    pst_hal_device->st_chan_result.ul_total_free_time_80M_us += st_stats_result.ul_pri80_free_time_us;
    pst_hal_device->st_chan_result.ul_total_recv_time_us     += st_stats_result.ul_ch_rx_time_us;
    pst_hal_device->st_chan_result.ul_total_send_time_us     += st_stats_result.ul_ch_tx_time_us;
    pst_hal_device->st_chan_result.ul_total_stats_time_us    += st_stats_result.ul_ch_stats_time_us;

    /* ͳ��MAC FCS��ȷ֡ʱ�� */
    pst_hal_dev_rom = (hal_to_dmac_device_rom_stru*)pst_hal_device->_rom;
    pst_mac_stats = &pst_hal_dev_rom->st_mac_ch_stats;

    ul_trx_time_us = st_mac_stats.ul_rx_direct_time + st_mac_stats.ul_tx_time;
    st_mac_stats.ul_rx_nondir_time = DMAC_SCAN_GET_VALID_FREE_TIME(ul_trx_time_us, st_stats_result.ul_ch_stats_time_us, st_mac_stats.ul_rx_nondir_time);
    pst_mac_stats->ul_rx_direct_time    += st_mac_stats.ul_rx_direct_time;
    pst_mac_stats->ul_rx_nondir_time    += st_mac_stats.ul_rx_nondir_time;
    pst_mac_stats->ul_tx_time           += st_mac_stats.ul_tx_time;

}


oal_uint32 dmac_scan_channel_statistics_complete(frw_event_mem_stru *pst_event_mem)
{
    mac_device_stru                 *pst_mac_device;
    frw_event_stru                  *pst_event;
    oal_uint16                       us_total_scan_time_per_chan;
    oal_uint8                        uc_do_meas;        /* ����ɨ���Ƿ�Ҫ��ȡ�ŵ������Ľ�� */
    oal_uint8                        uc_chan_stats_cnt;
    hal_to_dmac_device_stru         *pst_hal_device;


    /* Ѱ�Ҷ�Ӧ��DEVICE�ṹ�Լ���Ӧ��ACS�ṹ */
    pst_event = frw_get_event_stru(pst_event_mem);

    hal_get_hal_to_dmac_device(pst_event->st_event_hdr.uc_chip_id, pst_event->st_event_hdr.uc_device_id, &pst_hal_device);
    if (OAL_PTR_NULL == pst_hal_device)
    {
        OAM_ERROR_LOG2(0, OAM_SF_SCAN, "{dmac_scan_channel_statistics_complete::pst_hal_device null.chip id[%d],device id[%d]}",
                            pst_event->st_event_hdr.uc_chip_id, pst_event->st_event_hdr.uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_device = mac_res_get_dev(pst_hal_device->uc_mac_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_WARNING_LOG0(0, OAM_SF_SCAN, "{dmac_scan_channel_statistics_complete::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡӲ��ͳ�� ������� */
    uc_do_meas = pst_mac_device->st_scan_params.uc_scan_func & (MAC_SCAN_FUNC_MEAS | MAC_SCAN_FUNC_STATS);
    if (uc_do_meas)
    {
        dmac_scan_get_ch_statics_measurement_result_ram(pst_mac_device, pst_hal_device);
    }

    /* ÿ��Ӳ������ʱ����10ms�����ݱ���ɨ��ʱ����ȷ��Ҫ�������ٴ�Ӳ������ */
    pst_hal_device->st_chan_result.uc_stats_cnt++;
    uc_chan_stats_cnt = pst_hal_device->st_chan_result.uc_stats_cnt;

    us_total_scan_time_per_chan = (oal_uint16)(pst_mac_device->st_scan_params.us_scan_time * pst_mac_device->st_scan_params.uc_max_scan_count_per_channel);
    if (uc_chan_stats_cnt * DMAC_SCAN_CHANNEL_MEAS_PERIOD_MS < us_total_scan_time_per_chan)
    {
        /* ʹ���ŵ�����ǰ���MACͳ����Ϣ,�ŵ�����ͳ��MAC/PHY�Զ���� */
        hal_set_mac_clken(pst_hal_device, OAL_TRUE);
        hal_set_counter1_clear(pst_hal_device);
        hal_set_mac_clken(pst_hal_device, OAL_FALSE);

        /* �ٴ�����һ�β��� */
        hal_set_ch_statics_period(pst_hal_device, DMAC_SCAN_CHANNEL_STATICS_PERIOD_US);
        hal_set_ch_measurement_period(pst_hal_device, DMAC_SCAN_CHANNEL_MEAS_PERIOD_MS);
        hal_enable_ch_statics(pst_hal_device, 1);
    }
    else
    {
        /* CCA���ʹ���ŵ������ж����жϽ���������ɨ����� */
        if (WLAN_SCAN_MODE_BACKGROUND_CCA == pst_mac_device->st_scan_params.en_scan_mode)
        {
            if (OAL_PTR_NULL != pst_mac_device->st_scan_params.p_fn_cb)
            {
                pst_mac_device->st_scan_params.p_fn_cb(pst_mac_device);
                dmac_scan_calcu_channel_ratio(pst_hal_device);
                //�ŵ�����������Ҫ���ģ���ʱ�ڴ˴�����bsd���ԵĽӿ��������ŵ������Ľ��
        #ifdef _PRE_WLAN_FEATURE_BAND_STEERING
                dmac_bsd_device_load_scan_cb(pst_mac_device);
        #endif
            }
        }
    }

    return OAL_SUCC;
}

/*lint -e19 */

oal_module_symbol(dmac_scan_switch_channel_off);
oal_module_symbol(dmac_scan_begin);
oal_module_symbol(dmac_scan_abort);
oal_module_symbol(dmac_scan_switch_channel_back);
#ifdef _PRE_WLAN_FEATURE_CCA_OPT
oal_module_symbol(dmac_scan_handle_scan_req_entry);
#endif

/*lint +e19 */
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif
