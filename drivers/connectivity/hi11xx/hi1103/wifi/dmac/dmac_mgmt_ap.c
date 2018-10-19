


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 ͷ�ļ�����
*****************************************************************************/
#include "mac_resource.h"
#include "dmac_main.h"
#include "dmac_tx_bss_comm.h"
#include "dmac_mgmt_bss_comm.h"
#include "dmac_mgmt_ap.h"
#include "dmac_chan_mgmt.h"
#include "dmac_mgmt_sta.h"
#include "oal_net.h"
#include "mac_ie.h"
#include "dmac_beacon.h"
#ifdef _PRE_WLAN_FEATURE_P2P
#include "dmac_p2p.h"
#endif
#ifdef _PRE_WLAN_FEATURE_BAND_STEERING
#include "dmac_bsd.h"
#endif
#include "dmac_power.h"


#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_MGMT_AP_C

/*****************************************************************************
  2 ȫ�ֱ�������
*****************************************************************************/


/*****************************************************************************
  3 ����ʵ��
*****************************************************************************/


oal_uint32  dmac_ap_up_rx_probe_req(dmac_vap_stru *pst_dmac_vap, oal_netbuf_stru *pst_netbuf,oal_uint8 *puc_addr,oal_int8 c_rssi)
{
    oal_netbuf_stru            *pst_mgmt_buf;
    oal_uint16                  us_mgmt_len;
    oal_uint8                  *puc_probe_req;
    mac_tx_ctl_stru            *pst_tx_ctl;
    dmac_rx_ctl_stru           *pst_rx_ctl;
    mac_ieee80211_frame_stru   *pst_frame_hdr;
    oal_uint32                  ul_ret;
    oal_uint16                  us_frame_len;
    oal_uint16                  us_netbuf_len;
    oal_bool_enum_uint8         en_is_p2p_req = OAL_FALSE;

    /* ��ȡ֡ͷ��Ϣ */
    pst_rx_ctl    = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
    pst_frame_hdr = (mac_ieee80211_frame_stru *)(mac_get_rx_cb_mac_hdr(&(pst_rx_ctl->st_rx_info)));
    puc_probe_req = MAC_GET_RX_PAYLOAD_ADDR(&(pst_rx_ctl->st_rx_info), pst_netbuf);
    us_frame_len  = (oal_uint16)oal_netbuf_get_len(pst_netbuf);

#ifdef _PRE_WLAN_FEATURE_DBAC
    dmac_alg_probe_req_rx_notify(pst_dmac_vap, pst_netbuf);
#endif

    if (OAL_FALSE == dmac_ap_check_probe_req(pst_dmac_vap, puc_probe_req, pst_frame_hdr))
    {
        return OAL_SUCC;
    }

#ifdef _PRE_WLAN_FEATURE_BAND_STEERING
    if(DMAC_BSD_RET_BLOCK == dmac_bsd_rx_probe_req_frame_handle(pst_dmac_vap,puc_addr,c_rssi))
    {
#ifdef _PRE_DEBUG_MODE
        if(dmac_bsd_debug_switch())
        {
            OAM_WARNING_LOG3(0, OAM_SF_BSD,
                         "{dmac_ap_up_rx_probe_req::the user[xx:xx:xx:%x:%x:%x]'s probe req handle process blocked by BSD!}",
                         puc_addr[WLAN_MAC_ADDR_LEN-3],puc_addr[WLAN_MAC_ADDR_LEN-2],puc_addr[WLAN_MAC_ADDR_LEN-1]);
        }
#endif
        return OAL_SUCC;
    }
#endif

#ifdef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT
    if (OAL_FALSE == dmac_ap_check_probe_rej_sta(pst_dmac_vap, pst_frame_hdr))
    {
        return OAL_SUCC;
    }
#endif

    /* �յ���probe req ֡����P2P_IE */
    en_is_p2p_req = (OAL_PTR_NULL != mac_find_vendor_ie(MAC_WLAN_OUI_WFA, MAC_WLAN_OUI_TYPE_WFA_P2P, puc_probe_req, us_frame_len)) ? OAL_TRUE : OAL_FALSE;
#if ( _PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)
    if((OAL_TRUE == en_is_p2p_req) && (pst_dmac_vap->st_vap_base_info.ast_app_ie[OAL_APP_PROBE_RSP_IE].ul_ie_len > OAL_MGMT_NETBUF_APP_PROBE_RSP_IE_LEN_LIMIT))
    {/*p2p IE
���ȹ���ʱ��������*/
        OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{dmac_ap_up_rx_probe_req::alloc probe rsp netbuf with WLAN_LARGE_NETBUF_SIZE,probe_rsp_p2p_ie=[%d].}",
        pst_dmac_vap->st_vap_base_info.ast_app_ie[OAL_APP_PROBE_RSP_IE].ul_ie_len);
        pst_mgmt_buf  = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, WLAN_LARGE_NETBUF_SIZE, OAL_NETBUF_PRIORITY_HIGH);
        us_netbuf_len = WLAN_LARGE_NETBUF_SIZE;
    }
    else
#endif
    {
        pst_mgmt_buf  = OAL_MEM_NETBUF_ALLOC(OAL_MGMT_NETBUF, WLAN_MGMT_NETBUF_SIZE, OAL_NETBUF_PRIORITY_HIGH);
        us_netbuf_len = WLAN_MGMT_NETBUF_SIZE;
    }
    if (OAL_PTR_NULL == pst_mgmt_buf)
    {
        OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{dmac_ap_up_rx_probe_req::alloc netbuff failed.}");
        OAL_MEM_INFO_PRINT(OAL_MEM_POOL_ID_NETBUF);
        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_set_netbuf_prev(pst_mgmt_buf, OAL_PTR_NULL);
    oal_set_netbuf_next(pst_mgmt_buf, OAL_PTR_NULL);

    OAL_MEM_NETBUF_TRACE(pst_mgmt_buf, OAL_TRUE);

    /* ��װprobe request֡ */
    us_mgmt_len = dmac_mgmt_encap_probe_response(pst_dmac_vap, pst_mgmt_buf, pst_frame_hdr->auc_address2, en_is_p2p_req);
    if(us_netbuf_len < (us_frame_len - MAC_80211_FRAME_LEN))
    {
        OAM_ERROR_LOG2(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{dmac_ap_up_rx_probe_req::us_payload_len=[%d] over net_buf_size=[%d].}",
        (us_frame_len - MAC_80211_FRAME_LEN), us_netbuf_len);
    }

#ifdef _PRE_WLAN_NARROW_BAND
    //mac_get_nb_ie(&pst_dmac_vap->st_vap_base_info, puc_probe_req, us_frame_len);
#endif

    /* ���÷��͹���֡�ӿ� */
    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_mgmt_buf);

    OAL_MEMZERO(pst_tx_ctl, sizeof(mac_tx_ctl_stru));
    MAC_GET_CB_TX_USER_IDX(pst_tx_ctl) = MAC_INVALID_USER_ID;       /* ��ʱ�Զ��û������ڣ���һ���Ƿ�ֵ��������ɻ�ֱ���ͷ� */
    MAC_GET_CB_WME_AC_TYPE(pst_tx_ctl) = WLAN_WME_AC_MGMT;

#ifdef _PRE_WLAN_FEATURE_USER_RESP_POWER
    /* ֪ͨ�㷨,�޸�probe resp����֡���͹��� */
    dmac_pow_change_mgmt_power_process(&pst_dmac_vap->st_vap_base_info, pst_rx_ctl->st_rx_statistic.c_rssi_dbm);
#endif

    ul_ret = dmac_tx_mgmt(pst_dmac_vap, pst_mgmt_buf, us_mgmt_len);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX, "{dmac_ap_up_rx_probe_req::dmac_tx_mgmt failed[%d].", ul_ret);
        oal_netbuf_free(pst_mgmt_buf);
        return ul_ret;
    }

    return OAL_SUCC;
}


oal_uint32 dmac_send_notify_chan_width(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_data)
{
    oal_uint16                    us_frame_len;
    oal_netbuf_stru              *pst_netbuf;
    mac_tx_ctl_stru              *pst_tx_ctl;
    oal_uint32                    ul_ret;
    dmac_vap_stru                *pst_dmac_vap;

    /*���û�ֱ�ӷ���*/
    if (0 == pst_mac_vap->us_user_nums)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_2040, "{dmac_send_notify_chan_width::no user.}");
        return OAL_SUCC;
    }

    pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_mac_vap->uc_vap_id);

    /* ����notify_chan_width ֡�ռ� */
    pst_netbuf = (oal_netbuf_stru *)OAL_MEM_NETBUF_ALLOC(OAL_MGMT_NETBUF, MAC_80211_FRAME_LEN + MAC_HT_NOTIFY_CHANNEL_WIDTH_LEN, OAL_NETBUF_PRIORITY_MID);
    if(OAL_PTR_NULL == pst_netbuf)
    {
       OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040, "{dmac_send_notify_chan_width::alloc netbuff failed(size %d) in normal_netbuf.}", MAC_80211_FRAME_LEN + MAC_HT_NOTIFY_CHANNEL_WIDTH_LEN);
       OAL_MEM_INFO_PRINT(OAL_MEM_POOL_ID_NETBUF);
       return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    /* ��װNotify Channel Width֡*/
    OAL_MEMZERO(oal_netbuf_cb(pst_netbuf), OAL_NETBUF_CB_SIZE());
    us_frame_len = dmac_encap_notify_chan_width(pst_mac_vap, (oal_uint8 *)OAL_NETBUF_HEADER(pst_netbuf), puc_data);
    if (0 == us_frame_len)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_2040, "{dmac_send_notify_chan_width::dmac_encap_notify_chan_width error.}");
        oal_netbuf_free(pst_netbuf);
        return OAL_SUCC;
    }

    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_netbuf); /* ��ȡcb�ṹ�� */
    MAC_GET_CB_MPDU_LEN(pst_tx_ctl)     = us_frame_len;               /* dmac������Ҫ��mpdu���� */
    ul_ret = mac_vap_set_cb_tx_user_idx(pst_mac_vap, pst_tx_ctl, puc_data);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_2040, "(dmac_send_notify_chan_width::fail to find user by xx:xx:xx:0x:0x:0x.}",
        puc_data[3],
        puc_data[4],
        puc_data[5]);
    }

    oal_netbuf_put(pst_netbuf, us_frame_len);

    /* ��дnetbuf��cb�ֶΣ������͹���֡�ͷ�����ɽӿ�ʹ�� */
    MAC_SET_CB_FRAME_HEADER_ADDR(pst_tx_ctl, (mac_ieee80211_frame_stru *)oal_netbuf_header(pst_netbuf));
    MAC_GET_CB_WME_AC_TYPE(pst_tx_ctl) = WLAN_WME_AC_MGMT;

    ul_ret = dmac_tx_mgmt(pst_dmac_vap, pst_netbuf, us_frame_len);
    if (OAL_SUCC != ul_ret)
    {
        oal_netbuf_free(pst_netbuf);
        return ul_ret;
    }

    return OAL_SUCC;
}


oal_void  dmac_chan_multi_switch_to_20MHz_ap(dmac_vap_stru *pst_dmac_vap)
{
    mac_vap_stru         *pst_mac_vap = OAL_PTR_NULL;
    dmac_set_chan_stru    st_set_chan = {{0}};
    oal_uint32            ul_ret;

    pst_mac_vap = &pst_dmac_vap->st_vap_base_info;

    OAM_INFO_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_2040,
            "{dmac_chan_multi_switch_to_20MHz_ap::bit_2040_channel_switch_prohibited=%d}",
            mac_mib_get_2040SwitchProhibited(pst_mac_vap));

    /* dmac vap�����л�֮ǰ�Ĵ������� */
    pst_dmac_vap->en_40M_bandwidth = pst_mac_vap->st_channel.en_bandwidth;

    /* ����������л�������ֱ�ӷ��� */
    if (OAL_TRUE == mac_mib_get_2040SwitchProhibited(pst_mac_vap))
    {
        return;
    }

    dmac_chan_initiate_switch_to_20MHz_ap(&pst_dmac_vap->st_vap_base_info);

    dmac_chan_multi_select_channel_mac(pst_mac_vap, pst_mac_vap->st_channel.uc_chan_number, WLAN_BAND_WIDTH_20M);
    ul_ret = dmac_send_notify_chan_width(pst_mac_vap, BROADCAST_MACADDR);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040, "{dmac_chan_multi_switch_to_20MHz_ap::dmac_send_notify_chan_width return %d.}", ul_ret);
        return ;
    }

    ul_ret = dmac_chan_sync_event(pst_mac_vap, &st_set_chan);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040, "{dmac_chan_multi_switch_to_20MHz_ap::dmac_chan_sync_event return %d.}", ul_ret);
        return ;
    }
}


oal_void  dmac_chan_reval_status(mac_device_stru *pst_mac_device, mac_vap_stru *pst_mac_vap)
{
    oal_uint8                            uc_new_channel   = 0;
    wlan_channel_bandwidth_enum_uint8    en_new_bandwidth = WLAN_BAND_WIDTH_BUTT;
    oal_uint32                           ul_ret;

    OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_2040, "{dmac_chan_reval_status}");

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_mac_device) || (OAL_PTR_NULL == pst_mac_vap)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_SCAN, "{dmac_chan_reval_status::pst_mac_device or pst_mac_vap null.}", pst_mac_device, pst_mac_vap);
        return;
    }

    /* ���AP�Ѿ�׼�������ŵ��л�����ֱ�ӷ��أ������κδ��� */
    if (WLAN_CH_SWITCH_STATUS_1 == pst_mac_vap->st_ch_switch_info.en_ch_switch_status)
    {
        return;
    }

    ul_ret = dmac_chan_select_channel_for_operation(pst_mac_vap, &uc_new_channel, &en_new_bandwidth);
    if (OAL_SUCC != ul_ret)
    {
        return;
    }

    ul_ret = mac_is_channel_num_valid(pst_mac_vap->st_channel.en_band, uc_new_channel);
    if ((OAL_SUCC != ul_ret) || (en_new_bandwidth >= WLAN_BAND_WIDTH_BUTT))
    {
        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_SCAN,
                         "{dmac_chan_reval_status::Could not start network using the selected channel[%d] or bandwidth[%d].}",
                         uc_new_channel, en_new_bandwidth);
        return;
    }

    #if 0
    /* ��20MHz�ŵ��ı�(����Ŀǰ���㷨�����ŵ���Ӧ�ò���ı䣬���ܻ�ı��ֻ�Ǵ���ģʽ) */
    if (uc_new_channel != pst_mac_vap->st_channel.uc_chan_number)
    {
        pst_mac_vap->st_ch_switch_info.uc_ch_switch_cnt = WLAN_CHAN_SWITCH_DEFAULT_CNT;
        dmac_chan_multi_switch_to_new_channel(pst_mac_vap, uc_new_channel, en_new_bandwidth);
    }
    /* ��20MHz�ŵ����䣬��20MHz or 40MHz�ŵ��ı� */
    else
    #endif
    if (en_new_bandwidth != pst_mac_vap->st_channel.en_bandwidth)
    {
        dmac_chan_multi_select_channel_mac(pst_mac_vap, pst_mac_vap->st_channel.uc_chan_number, en_new_bandwidth);
        ul_ret = dmac_send_notify_chan_width(pst_mac_vap, BROADCAST_MACADDR);
        if (OAL_SUCC != ul_ret)
        {
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{dmac_chan_reval_status::dmac_send_notify_chan_width return %d.}", ul_ret);
            return;
        }
    }
    else
    {
        OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{dmac_chan_reval_status::No Channel change after re-evaluation.}");
    }
}


oal_bool_enum_uint8  dmac_chan_get_40MHz_possibility(
                mac_vap_stru                 *pst_mac_vap,
                dmac_eval_scan_report_stru   *pst_chan_scan_report)
{
    oal_bool_enum_uint8   en_fortyMHz_poss = OAL_FALSE;

    OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_2040, "{dmac_chan_get_40MHz_possibility}");
#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
    if (OAL_TRUE ==  mac_mib_get_FortyMHzOperationImplemented(pst_mac_vap))
    {
        en_fortyMHz_poss = dmac_chan_get_2040_op_chan_list(pst_mac_vap, pst_chan_scan_report);
    }
#endif

    return en_fortyMHz_poss;
}



oal_uint32  dmac_chan_select_channel_for_operation(
                mac_vap_stru                        *pst_mac_vap,
                oal_uint8                           *puc_new_channel,
                wlan_channel_bandwidth_enum_uint8   *pen_new_bandwidth)
{
    mac_device_stru                     *pst_mac_device;
    dmac_eval_scan_report_stru          *pst_chan_scan_report;
    oal_uint8                            uc_least_busy_chan_idx = 0xFF;
    oal_uint16                           us_least_networks = 0xFFFF;
    oal_uint16                           us_cumulative_networks = 0;
    oal_bool_enum_uint8                  en_fortyMHz_poss, en_rslt = OAL_FALSE;
    mac_sec_ch_off_enum_uint8            en_user_chan_offset = MAC_SEC_CH_BUTT, en_chan_offset = MAC_SCN;
    oal_uint8                            uc_user_chan_idx = 0xFF, uc_chan_idx = 0xFF;
    oal_uint8                            uc_num_supp_chan = mac_get_num_supp_channel(pst_mac_vap->st_channel.en_band);
    oal_uint8                            uc_max_supp_channle = MAC_MAX_SUPP_CHANNEL;
    oal_uint32                           ul_ret = OAL_FAIL;

    OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_2040,"{dmac_chan_select_channel_for_operation}");

    pst_chan_scan_report = (dmac_eval_scan_report_stru *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, uc_max_supp_channle * OAL_SIZEOF(dmac_eval_scan_report_stru), OAL_TRUE);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_chan_scan_report))
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SCAN,
                       "{dmac_chan_select_channel_for_operation::pst_chan_scan_report memory alloc failed, size[%d].}", (uc_max_supp_channle * OAL_SIZEOF(*pst_chan_scan_report)));
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SCAN,
                       "{dmac_chan_select_channel_for_operation::pst_mac_device null,device_id=%d.}", pst_mac_vap->uc_device_id);
        OAL_MEM_FREE((oal_void *)pst_chan_scan_report, OAL_TRUE);

        return OAL_ERR_CODE_PTR_NULL;
    }

    dmac_chan_init_chan_scan_report(pst_mac_vap, pst_chan_scan_report, uc_num_supp_chan);

    /* ���Զ��ŵ�ѡ��û�п��������ȡ�û�ѡ������ŵ��ţ��Լ�����ģʽ */
    if (OAL_FALSE == mac_device_is_auto_chan_sel_enabled(pst_mac_device))
    {
#ifdef _PRE_WLAN_FEATURE_DBAC
        if(mac_is_dbac_enabled(pst_mac_device))
        {
            ul_ret = mac_get_channel_idx_from_num(pst_mac_vap->st_channel.en_band,
                            pst_mac_vap->st_channel.uc_chan_number, &uc_user_chan_idx) ;
        }
        else
#endif
        {
            ul_ret = mac_get_channel_idx_from_num(pst_mac_vap->st_channel.en_band,
                            pst_mac_device->uc_max_channel, &uc_user_chan_idx) ;
        }
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SCAN,
                             "{dmac_chan_select_channel_for_operation::mac_get_channel_idx_from_num failed[%d].}", ul_ret);
            OAL_MEM_FREE((oal_void *)pst_chan_scan_report, OAL_TRUE);

            return ul_ret;
        }

        en_user_chan_offset = mac_get_sco_from_bandwidth(pst_mac_vap->st_ch_switch_info.en_user_pref_bandwidth);

        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_SCAN,
                      "{dmac_chan_select_channel_for_operation::User Preferred Channel id=%d sco=%d.}",
                      uc_user_chan_idx, en_user_chan_offset);
    }

    /* �ж��ڵ�ǰ�������Ƿ��ܹ�����40MHz BSS */
    en_fortyMHz_poss = dmac_chan_get_40MHz_possibility(pst_mac_vap, pst_chan_scan_report);

    /* �û�ѡ�������ŵ� */
    if (uc_user_chan_idx != 0xFF)
    {
        /* ����ܹ�����40MHz BSS�������û�Ҳϣ������40MHz */
        if ((OAL_TRUE == en_fortyMHz_poss) && (MAC_SCN != en_user_chan_offset))
        {
            if (MAC_SCA == en_user_chan_offset)
            {
                en_rslt = dmac_chan_is_40MHz_sca_allowed(pst_mac_vap, pst_chan_scan_report, uc_user_chan_idx, en_user_chan_offset);
            }
            else if (MAC_SCB == en_user_chan_offset)
            {
                en_rslt = dmac_chan_is_40MHz_scb_allowed(pst_mac_vap, pst_chan_scan_report, uc_user_chan_idx, en_user_chan_offset);
            }
        }

        if (OAL_TRUE == en_rslt)
        {
            en_chan_offset = en_user_chan_offset;
        }

        uc_least_busy_chan_idx = uc_user_chan_idx;
    }
    /* �û�û��ѡ���ŵ����Զ�ѡ��һ�����æ���ŵ�(��) */
    else
    {
        for (uc_chan_idx = 0; uc_chan_idx < uc_num_supp_chan; uc_chan_idx++)
        {
            if (!(pst_chan_scan_report[uc_chan_idx].en_chan_op & DMAC_OP_ALLOWED))
            {
                continue;
            }

            /* �ж����ŵ��������Ƿ���Ч */
            ul_ret = mac_is_channel_idx_valid(pst_mac_vap->st_channel.en_band, uc_chan_idx);
            if (OAL_SUCC != ul_ret)
            {
                continue;
            }

            /* �ܹ�����40MHz BSS */
            if ((OAL_TRUE == en_fortyMHz_poss))
            {
                /* �ж����ŵ���(��)����ŵ��Ƿ����Ϊ���ŵ� */
                en_rslt = dmac_chan_is_40MHz_sca_allowed(pst_mac_vap, pst_chan_scan_report, uc_chan_idx, en_user_chan_offset);
                if (OAL_TRUE == en_rslt)
                {
                    /* ��������ŵ����æ����ѡ�������ŵ���Ϊ"��ǰ���æ�ŵ�" */
                    if (pst_chan_scan_report[uc_chan_idx].aus_num_networks[DMAC_NETWORK_SCA] < us_least_networks)
                    {
                        us_least_networks      = pst_chan_scan_report[uc_chan_idx].aus_num_networks[DMAC_NETWORK_SCA];
                        uc_least_busy_chan_idx = uc_chan_idx;
                        en_chan_offset         = MAC_SCA;
                    }
                }

                /* �ж����ŵ���(��)����ŵ��Ƿ����Ϊ���ŵ� */
                en_rslt = dmac_chan_is_40MHz_scb_allowed(pst_mac_vap, pst_chan_scan_report, uc_chan_idx, en_user_chan_offset);
                if (OAL_TRUE == en_rslt)
                {
                    /* ��������ŵ����æ����ѡ�������ŵ���Ϊ"��ǰ���æ�ŵ�" */
                    if (pst_chan_scan_report[uc_chan_idx].aus_num_networks[DMAC_NETWORK_SCB] < us_least_networks)
                    {
                        us_least_networks      = pst_chan_scan_report[uc_chan_idx].aus_num_networks[DMAC_NETWORK_SCB];
                        uc_least_busy_chan_idx = uc_chan_idx;
                        en_chan_offset         = MAC_SCB;
                    }
                }
            }
            /* ���ܹ�����40MHz BSS */
            else
            {
                /* ��ȡ��ǰ�ŵ��ڽ���BSS���� */
                us_cumulative_networks = dmac_chan_get_cumulative_networks(pst_mac_device,
                                pst_mac_vap->st_channel.en_band, uc_chan_idx);

                /* ѡ���ڽ�BSS���ٵ�һ���ŵ���Ϊ"��ǰ���æ�ŵ�" */
                if (us_cumulative_networks < us_least_networks)
                {
                    us_least_networks      = us_cumulative_networks;
                    uc_least_busy_chan_idx = uc_chan_idx;
                }
            }
        }
    }

    mac_get_channel_num_from_idx(pst_mac_vap->st_channel.en_band, uc_least_busy_chan_idx, puc_new_channel);

    *pen_new_bandwidth = mac_get_bandwidth_from_sco(en_chan_offset);

    OAM_INFO_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_SCAN,
                  "{dmac_chan_select_channel_for_operation::Selected Channel=%d, Selected Bandwidth=%d.}",
                  (*puc_new_channel), (*pen_new_bandwidth));

    OAL_MEM_FREE((oal_void *)pst_chan_scan_report, OAL_TRUE);

    return OAL_SUCC;
}


#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST

oal_void  dmac_ap_up_rx_2040_coext(dmac_vap_stru *pst_dmac_vap, oal_netbuf_stru *pst_netbuf)
{
    mac_device_stru    *pst_mac_device;
    mac_vap_stru       *pst_mac_vap = &(pst_dmac_vap->st_vap_base_info);
    oal_uint32          ul_index;
    oal_uint8           uc_coext_info;
    oal_uint8          *puc_data;
    oal_uint32          ul_ret  = OAL_SUCC;
    dmac_set_chan_stru  st_set_chan = {{0}};
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    mac_rx_ctl_stru    *pst_rx_ctl;
#endif

    /* 5GHzƵ�κ��� 20/40 BSS�������֡ */
    if ((WLAN_BAND_5G == pst_mac_vap->st_channel.en_band) ||
        (OAL_FALSE    == mac_mib_get_2040BSSCoexistenceManagementSupport(pst_mac_vap)))
    {
        OAM_INFO_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_2040,
                         "{dmac_ap_up_rx_2040_coext::Now in 5GHz.}");
        return;
    }

    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_2040, "{dmac_ap_up_rx_2040_coext::pst_mac_device null.}");
        return;
    }

    /* ��ȡ֡��ָ�� */
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    pst_rx_ctl    = (mac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
    puc_data   = MAC_GET_RX_PAYLOAD_ADDR(pst_rx_ctl, pst_netbuf);
#else
    puc_data   = OAL_NETBUF_PAYLOAD(pst_netbuf);
#endif

    ul_index = MAC_ACTION_OFFSET_ACTION + 1;

    /* 20/40 BSS Coexistence IE */
    if (MAC_EID_2040_COEXT == puc_data[ul_index])
    {
        uc_coext_info = puc_data[ul_index + MAC_IE_HDR_LEN];

        OAM_INFO_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_2040,
                      "{dmac_ap_up_rx_2040_coext::20/40 Coexistence Management frame received, Coext Info=0x%x.}", uc_coext_info);
        /* BIT1 - Forty MHz Intolerant */
        /* BIT2 - 20 MHz BSS Width Request */
        if (uc_coext_info & (BIT1 | BIT2))
        {
            dmac_chan_start_40M_recovery_timer(pst_dmac_vap);
            /* ���BIT1��BIT2����Ϊ1���ҵ�ǰ�����ŵ���ȴ���20MHz����AP��Ҫ�л���20MHz���� */
            if ((WLAN_BAND_WIDTH_40PLUS == pst_mac_vap->st_channel.en_bandwidth)
                ||(WLAN_BAND_WIDTH_40MINUS == pst_mac_vap->st_channel.en_bandwidth))
            {
                dmac_chan_multi_switch_to_20MHz_ap(pst_dmac_vap);
                return;
            }
            else
            {
                pst_mac_device->en_40MHz_intol_bit_recd = OAL_TRUE;
            }
        }

        ul_index += (MAC_IE_HDR_LEN + puc_data[ul_index + 1]);
    }

    /* 20/40 BSS Intolerant Channel Report IE */
    if (MAC_EID_2040_INTOLCHREPORT == puc_data[ul_index])
    {
        oal_uint8              uc_len        = puc_data[ul_index + 1];
        oal_uint8              uc_chan_idx   = 0, uc_loop;
        oal_bool_enum_uint8    en_reval_chan = OAL_FALSE;

        ul_index += (MAC_IE_HDR_LEN + 1);   /* skip Element ID��Length��Operating Class */
        OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040, "{dmac_ap_up_rx_2040_coext::Chan Report with len=%d.}\r\n", uc_len);

        for (uc_loop = 0; uc_loop < uc_len - 1; uc_loop++, ul_index++)
        {
            ul_ret = mac_get_channel_idx_from_num(pst_mac_vap->st_channel.en_band, puc_data[ul_index], &uc_chan_idx) ;
            if (OAL_SUCC != ul_ret)
            {
                OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_2040,
                                 "{dmac_ap_up_rx_2040_coext::mac_get_channel_idx_from_num failed[%d].}", ul_ret);
                continue;
            }

            ul_ret = mac_is_channel_idx_valid(pst_mac_vap->st_channel.en_band, uc_chan_idx);
            if (OAL_SUCC != ul_ret)
            {
                OAM_WARNING_LOG2(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_2040,
                                 "{dmac_ap_up_rx_2040_coext::channel(indx=%d) not valid, return[%d].}", uc_chan_idx, ul_ret);
                continue;
            }

            if (MAC_CH_TYPE_PRIMARY != pst_mac_device->st_ap_channel_list[uc_chan_idx].en_ch_type)
            {
                pst_mac_device->st_ap_channel_list[uc_chan_idx].en_ch_type = MAC_CH_TYPE_PRIMARY;
                en_reval_chan = OAL_TRUE;
            }
        }

        if (OAL_TRUE == en_reval_chan)
        {
            OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_2040,
                                 "{dmac_ap_up_rx_2040_coext::Re-evaluation needed as some channel status changed.}");

            /* ��������ŵ����ߴ����л����Ž����л� */
            if (0 == mac_mib_get_2040SwitchProhibited(pst_mac_vap))
            {
                dmac_chan_start_40M_recovery_timer(pst_dmac_vap);
                /* ���������Ƿ���Ҫ�����ŵ��л� */
                dmac_chan_reval_status(pst_mac_device, pst_mac_vap);
                ul_ret = dmac_chan_sync_event(pst_mac_vap, &st_set_chan);
                if (OAL_SUCC != ul_ret)
                {
                    OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040, "{dmac_ap_up_rx_2040_coext::return %d.}", ul_ret);
                    return ;
                }
            }
        }
    }
}
#endif

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

