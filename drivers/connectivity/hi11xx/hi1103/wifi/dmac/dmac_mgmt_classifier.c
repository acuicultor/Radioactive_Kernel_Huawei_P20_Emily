


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 ͷ�ļ�����
*****************************************************************************/
#include "mac_frame.h"
#include "mac_resource.h"
#include "mac_ie.h"
#include "dmac_ext_if.h"
#include "dmac_main.h"
#include "dmac_vap.h"
#include "dmac_rx_data.h"
#include "dmac_mgmt_bss_comm.h"
#include "dmac_mgmt_classifier.h"
#include "dmac_blockack.h"
#include "dmac_psm_ap.h"
#include "dmac_mgmt_ap.h"
#include "dmac_tx_bss_comm.h"
#include "dmac_scan.h"
#include "dmac_alg_if.h"
#include "mac_vap.h"
#include "dmac_11w.h"
#include "dmac_dft.h"
#include "dmac_mgmt_sta.h"
#include "dmac_p2p.h"
#include "oal_net.h"
#include "dmac_beacon.h"
#include "dmac_dft.h"
#include "dmac_chan_mgmt.h"
#ifdef _PRE_WLAN_FEATURE_STA_PM
#include "dmac_psm_sta.h"
#endif
#ifdef _PRE_WLAN_FEATURE_P2P
#include "dmac_p2p.h"
#endif

#ifdef _PRE_WLAN_FEATURE_11K
#include "dmac_11k.h"
#endif
#ifdef _PRE_WLAN_FEATURE_11V
#include "dmac_11v.h"
#endif

#ifdef _PRE_WLAN_FEATURE_BTCOEX
#include "dmac_device.h"
#include "dmac_resource.h"
#include "dmac_btcoex.h"
#endif


#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
#include "pm_extern.h"
#endif
#ifdef _PRE_WLAN_FEATURE_FTM
#include "dmac_ftm.h"
#endif

#ifdef _PRE_WLAN_FEATURE_OPMODE_NOTIFY
#include "dmac_opmode.h"
#endif
#ifdef _PRE_WLAN_FEATURE_SMPS
#include "dmac_smps.h"
#endif
#ifdef _PRE_WLAN_FEATURE_M2S
#include "dmac_m2s.h"
#endif

#ifdef _PRE_WLAN_FEATURE_WMMAC
#include "dmac_wmmac.h"
#endif
#ifdef _PRE_WLAN_FEATURE_BAND_STEERING
#include "dmac_bsd.h"
#endif

#include "dmac_power.h"
#include "dmac_csa_sta.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_MGMT_CLASSIFIER_C

/*****************************************************************************
  2 ȫ�ֱ�������
*****************************************************************************/


/*****************************************************************************
  3 ����ʵ��
*****************************************************************************/

oal_uint32  dmac_rx_process_control(dmac_vap_stru *pst_dmac_vap, oal_netbuf_stru *pst_netbuf, oal_uint8 *pen_go_on)
{
    dmac_rx_ctl_stru                 *pst_rx_ctl;
    mac_ieee80211_frame_stru         *pst_frame_hdr;
    mac_ieee80211_pspoll_frame_stru  *pst_pspoll_frame_hdr;
    oal_uint8                        *puc_payload;
    dmac_tid_stru                    *pst_tid;
    dmac_user_stru                   *pst_ta_user;
    oal_uint8                        *puc_sa_addr;
    oal_uint16                        us_user_idx = 0xFFFF;
    oal_uint8                         uc_tidno;
    oal_uint32                        ul_rslt;

    *pen_go_on = OAL_FALSE;//Ĭ�ϲ��ϱ�control֡

    /* ��ȡ֡ͷ��Ϣ */
    pst_rx_ctl    = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
    pst_frame_hdr = (mac_ieee80211_frame_stru  *)mac_get_rx_cb_mac_hdr(&(pst_rx_ctl->st_rx_info));
    /* ����ACK֡ */
    if (WLAN_ACK == pst_frame_hdr->st_frame_control.bit_sub_type)
    {
        //OAM_INFO_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX, "{dmac_rx_process_control::ack frame.}\r\n");
        OAM_STAT_VAP_INCR(pst_dmac_vap->st_vap_base_info.uc_vap_id, rx_ack_dropped, 1);
        return OAL_SUCC;
    }

    /* ��ȡԴ��ַ */
    puc_sa_addr = pst_frame_hdr->auc_address2;

    /*  ��ȡ�û�ָ�� */
    ul_rslt = mac_vap_find_user_by_macaddr(&(pst_dmac_vap->st_vap_base_info), puc_sa_addr, &us_user_idx);
    /*
        �����û�ʧ��: �����쳣�����أ�����Χ�ͷſռ�
        û���ҵ���Ӧ���û�: �������ִ��
    */
    if (OAL_SUCC != ul_rslt)
    {
        OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX,
                         "{dmac_rx_process_control::mac_vap_find_user_by_macaddr failed[%d]}", ul_rslt);

        return ul_rslt;
    }

    /* ת��Ϊdmac user */
    pst_ta_user = (dmac_user_stru *)mac_res_get_dmac_user(us_user_idx);
    if (OAL_PTR_NULL == pst_ta_user)
    {
        OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX, "{dmac_rx_process_control::pst_ta_user[%d] null.}",
            us_user_idx);

        mac_rx_report_80211_frame((oal_uint8 *)&(pst_dmac_vap->st_vap_base_info),
                                  (oal_uint8 *)&(pst_rx_ctl->st_rx_info),
                                  pst_netbuf,
                                  OAM_OTA_TYPE_RX_DMAC_CB);

        return OAL_ERR_CODE_PTR_NULL;
    }

    /* �����ҵ��û�: ����ʱ��� */
    pst_ta_user->ul_last_active_timestamp = (oal_uint32)OAL_TIME_GET_STAMP_MS();

    if (WLAN_BLOCKACK_REQ == (pst_frame_hdr->st_frame_control.bit_sub_type))    /* BAR 1000 */
    {
        /* ��ȡ֡ͷ��payloadָ��*/
        puc_payload = MAC_GET_RX_PAYLOAD_ADDR(&(pst_rx_ctl->st_rx_info), pst_netbuf);

        /*************************************************************************/
        /*                     BlockAck Request Frame Format                     */
        /* --------------------------------------------------------------------  */
        /* |Frame Control|Duration|DA|SA|BAR Control|BlockAck Starting    |FCS|  */
        /* |             |        |  |  |           |Sequence number      |   |  */
        /* --------------------------------------------------------------------  */
        /* | 2           |2       |6 |6 |2          |2                    |4  |  */
        /* --------------------------------------------------------------------  */
        /*                                                                       */
        /*************************************************************************/

        uc_tidno        = (puc_payload[1] & 0xF0) >> 4;
        //us_start_seqnum = mac_get_bar_start_seq_num(puc_payload);

        /*drop bar frame when tid num nonsupport*/
        if(uc_tidno >= WLAN_TID_MAX_NUM)
        {
            OAM_STAT_VAP_INCR(pst_dmac_vap->st_vap_base_info.uc_vap_id, rx_bar_process_dropped, 1);
            OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX, "{dmac_rx_process_control:: invaild tidno:%d.}", (oal_int32)uc_tidno);
            return OAL_SUCC;
        }

        pst_tid = &(pst_ta_user->ast_tx_tid_queue[uc_tidno]);
        if (DMAC_BA_COMPLETE != pst_tid->st_ba_rx_hdl.en_ba_conn_status)
        {
            OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX, "{dmac_rx_process_control::pst_ba_rx_hdl null.}");
            OAM_STAT_VAP_INCR(pst_dmac_vap->st_vap_base_info.uc_vap_id, rx_bar_process_dropped, 1);
            dmac_mgmt_delba(pst_dmac_vap, pst_ta_user, uc_tidno, MAC_RECIPIENT_DELBA, MAC_QSTA_SETUP_NOT_DONE);
            return OAL_ERR_CODE_PTR_NULL;
        }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        /*HOST������ʱ,Bar֡�ϴ���Hmac����*/
        if(OAL_FALSE == PM_WLAN_IsHostSleep())
#endif
        {
            *pen_go_on = OAL_TRUE;
        }
        return OAL_SUCC;

    }

    if (WLAN_PS_POLL == (pst_frame_hdr->st_frame_control.bit_sub_type))         /* PS_POLL 1010 */
    {
        /* �����ps-poll����Ӧ�ð�֡ͷת��Ϊps-poll��֡ͷ��ʽ */
        pst_pspoll_frame_hdr = (mac_ieee80211_pspoll_frame_stru  *)mac_get_rx_cb_mac_hdr(&(pst_rx_ctl->st_rx_info));

        /* AP��Ҫ�ж�PS_POLL�����AID��AP������û�AID�Ƿ�һ��,��һ�µĻ�ֱ�Ӻ��� */
        if (pst_pspoll_frame_hdr->bit_aid_value != pst_ta_user->st_user_base_info.us_assoc_id)
        {
            OAM_ERROR_LOG2(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX,
                           "{dmac_rx_process_control::AID in pspoll and AID in user_stru mismatch, bit_aid_value=%d us_assoc_id=%d.}",
                           pst_pspoll_frame_hdr->bit_aid_value, pst_ta_user->st_user_base_info.us_assoc_id);

            OAM_STAT_VAP_INCR(pst_dmac_vap->st_vap_base_info.uc_vap_id, rx_pspoll_process_dropped, 1);
            return OAL_FAIL;
        }

        /* ֱ�ӵ���PS_POLL, ����ʹ��֡���ͼ��� */
        ul_rslt = dmac_psm_resv_ps_poll(pst_dmac_vap, pst_ta_user);
        if (OAL_SUCC != ul_rslt)
        {
            OAM_STAT_VAP_INCR(pst_dmac_vap->st_vap_base_info.uc_vap_id, rx_pspoll_process_dropped, 1);
            return ul_rslt;
        }
    }
    return OAL_SUCC;
}


OAL_STATIC oal_void  dmac_rx_notify_channel_width(mac_vap_stru *pst_mac_vap,oal_uint8 *puc_data,dmac_user_stru *pst_dmac_user)
{
    wlan_bw_cap_enum_uint8      en_bwcap_vap;

    en_bwcap_vap = puc_data[MAC_ACTION_OFFSET_ACTION + 1] & BIT0;

    /* ����ģʽδ�ı�or HT40��ֹ or��Ҫ����notify channel width ��action�ϱ� */
    if ((pst_dmac_user->st_user_base_info.en_avail_bandwidth == en_bwcap_vap)
         ||(OAL_FALSE == mac_mib_get_FortyMHzOperationImplemented(pst_mac_vap)))
    {
        return;
    }

    /*csa������̲��������仯*/
    if ((OAL_FALSE == dmac_sta_csa_is_in_waiting(pst_mac_vap)) && (MAC_VAP_BW_FSM_BEACON_AVAIL(pst_mac_vap)))
    {
        pst_mac_vap->st_ch_switch_info.bit_wait_bw_change = OAL_TRUE;

        /* ���µ�"STA Channel Width" field */
        mac_ie_proc_chwidth_field(pst_mac_vap, &(pst_dmac_user->st_user_base_info),en_bwcap_vap);

        /* �����㷨�ı����֪ͨ�� */
        dmac_alg_cfg_user_bandwidth_notify(pst_mac_vap, &(pst_dmac_user->st_user_base_info));
    }
}


OAL_STATIC oal_void  dmac_sta_up_rx_ch_switch(mac_vap_stru *pst_mac_vap, oal_uint8  *puc_frame_body,oal_uint16  us_framebody_len)
{
    oal_uint8          *puc_ie                = OAL_PTR_NULL;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_frame_body)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{dmac_sta_up_rx_ch_switch::param null,pst_mac_vap = [%p],puc_frame_body = [%p]}",
                       pst_mac_vap,puc_frame_body);
        return;
    }

    if (us_framebody_len <= (MAC_ACTION_OFFSET_ACTION + 1))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{dmac_sta_up_rx_ch_switch::framebody_len[%d]}", us_framebody_len);
        return;
    }

    us_framebody_len -= (MAC_ACTION_OFFSET_ACTION + 1);
    puc_frame_body   += (MAC_ACTION_OFFSET_ACTION + 1);


    puc_ie = mac_find_ie(MAC_EID_CHANSWITCHANN, puc_frame_body, us_framebody_len);
    if (OAL_PTR_NULL != puc_ie)
    {
        dmac_ie_proc_ch_switch_ie(pst_mac_vap, puc_ie, MAC_EID_CHANSWITCHANN);
    }

    dmac_ie_proc_csa_bw_ie(pst_mac_vap, puc_frame_body, us_framebody_len);

    return;
}

OAL_STATIC oal_void  dmac_sta_up_rx_ext_ch_switch(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_frame_body,oal_uint16 us_framebody_len)
{
    oal_uint16          us_index;
    oal_uint8          *puc_wide_bw_ch_switch_ie = OAL_PTR_NULL;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_frame_body)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_RX, "{dmac_sta_up_rx_ext_ch_switch::param null,pst_mac_vap = [%p],pst_netbuf = [%p]}",
                       pst_mac_vap, puc_frame_body);
        return;
    }

    if (OAL_FALSE == mac_mib_get_SpectrumManagementImplemented(pst_mac_vap))
    {
        OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_RX, "{dmac_sta_up_rx_ext_ch_switch::Ignoring Spectrum Management frames.}");
        return;
    }

    dmac_ie_proc_ch_switch_ie(pst_mac_vap, puc_frame_body, MAC_EID_EXTCHANSWITCHANN);

    /* �����֧��VHT������Ժ�����Wide Bandwidth Channel Switch IE */
    if (OAL_FALSE == mac_mib_get_VHTOptionImplemented(pst_mac_vap))
    {
        return;
    }

    us_index = 6;

    puc_wide_bw_ch_switch_ie = mac_find_ie(MAC_EID_WIDE_BW_CH_SWITCH, &puc_frame_body[us_index], (oal_int32)(us_framebody_len - us_index));
    if (OAL_PTR_NULL != puc_wide_bw_ch_switch_ie)
    {
        dmac_ie_proc_wide_bandwidth_ie(pst_mac_vap, puc_wide_bw_ch_switch_ie);
    }
}

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST

oal_void  dmac_send_2040_coext_mgmt_frame_sta(mac_vap_stru *pst_mac_vap)
{
    dmac_vap_stru     *pst_dmac_vap;
    oal_netbuf_stru   *pst_netbuf = OAL_PTR_NULL;
    mac_tx_ctl_stru   *pst_tx_ctl;
    oal_uint16         us_frame_len = 0;
    oal_uint8          uc_coext_info = 0;
    oal_uint32         ul_channel_report = pst_mac_vap->st_ch_switch_info.ul_chan_report_for_te_a;
    oal_uint32         ul_ret;

    /* ����dot11FortyMHzIntolerant��д��field */
    if ( (OAL_TRUE == mac_mib_get_FortyMHzIntolerant(pst_mac_vap)) ||
         (OAL_TRUE == pst_mac_vap->st_ch_switch_info.en_te_b))
    {
        uc_coext_info |= BIT1;
    }

    /* ����⵽Trigger Event Bʱ�����ô�fieldΪ1 */
    if (0 != ul_channel_report)
    {
        uc_coext_info |= BIT2;
    }

    /* ����ϴ�ɨ���� */
    pst_mac_vap->st_ch_switch_info.ul_chan_report_for_te_a = 0;
    pst_mac_vap->st_ch_switch_info.en_te_b = OAL_FALSE;

    /* �������֡�ڴ� */
    pst_netbuf = OAL_MEM_NETBUF_ALLOC(OAL_MGMT_NETBUF, WLAN_SMGMT_NETBUF_SIZE, OAL_NETBUF_PRIORITY_MID);
    if (OAL_PTR_NULL == pst_netbuf)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_2040, "{dmac_send_2040_coext_mgmt_frame_sta::pst_netbuf null.}");
        return;
    }

    OAL_MEM_NETBUF_TRACE(pst_netbuf, OAL_TRUE);
    oal_set_netbuf_prev(pst_netbuf, OAL_PTR_NULL);
    oal_set_netbuf_next(pst_netbuf, OAL_PTR_NULL);

    /* ��װ20/40 �������֡ */
    us_frame_len = mac_encap_2040_coext_mgmt((oal_void *)pst_mac_vap, pst_netbuf, uc_coext_info, ul_channel_report);

    if (us_frame_len > WLAN_SMGMT_NETBUF_SIZE)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_2040, "{dmac_send_2040_coext_mgmt_frame_sta:: probably memory used cross-border.}");
        oal_netbuf_free(pst_netbuf);
        return;
    }

    oal_netbuf_put(pst_netbuf, us_frame_len);

    /* ��дnetbuf��cb�ֶΣ������͹���֡�ͷ�����ɽӿ�ʹ�� */
    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_netbuf);

    OAL_MEMZERO(pst_tx_ctl, OAL_NETBUF_CB_SIZE());
    MAC_GET_CB_MPDU_LEN(pst_tx_ctl) = us_frame_len - MAC_80211_FRAME_LEN;
    MAC_GET_CB_TX_USER_IDX(pst_tx_ctl) = pst_mac_vap->us_assoc_vap_id;//���͸�����ap�ĵ���,�úϷ�user idx
    MAC_GET_CB_WME_AC_TYPE(pst_tx_ctl) = WLAN_WME_AC_MGMT;

    OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_2040,
        "{dmac_send_2040_coext_mgmt_frame_sta::uc_coext_info=0x%x, ul_channel_report=0x%x}",
        uc_coext_info, ul_channel_report);

    pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_mac_vap->uc_vap_id);

    ul_ret = dmac_tx_mgmt(pst_dmac_vap, pst_netbuf, us_frame_len);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040, "{dmac_send_2040_coext_mgmt_frame_sta::ul_ret=%d}", ul_ret);
        oal_netbuf_free(pst_netbuf);
    }
}


OAL_STATIC oal_void  dmac_sta_up_rx_2040_coext(mac_vap_stru *pst_mac_vap, oal_uint8  *puc_frame_body)
{
    /* ���STA��֧��"20/40�������"���ԣ���ֱ�Ӻ���AP��������"20/40�������֡"  ��HTվ�㣬�������֡ */
    if (OAL_FALSE == mac_mib_get_2040BSSCoexistenceManagementSupport(pst_mac_vap) ||
            (OAL_FALSE == mac_mib_get_HighThroughputOptionImplemented(pst_mac_vap)))
    {
        OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_2040, "{dmac_sta_up_rx_2040_coext::Ignoring the 20/40 Coexistence Management frame.}");
        return;
    }

    /* "Information Request" field */
    if (puc_frame_body[MAC_ACTION_OFFSET_ACTION + 1 + 1 + 1] & BIT0)
    {
        /* ��STA�յ�һ��Information RequestΪ1��֡����Ҫ��һ��20/40�������֡ */
        dmac_send_2040_coext_mgmt_frame_sta(pst_mac_vap);
    }
}


oal_void  dmac_ap_rx_notify_channel_width(mac_vap_stru *pst_mac_vap, dmac_user_stru *pst_dmac_user, oal_uint8 *puc_data)
{
    if ((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == pst_dmac_user) || (OAL_PTR_NULL == puc_data))
    {
        OAM_ERROR_LOG3(0, OAM_SF_CFG, "{dmac_ap_rx_notify_channel_width::null param, 0x%x 0x%x 0x%x.}", pst_mac_vap, pst_dmac_user, puc_data);
        return;
    }

    if (OAL_FALSE == mac_mib_get_HighThroughputOptionImplemented(pst_mac_vap))
    {
        return;
    }

    /* ���µ�"STA Channel Width" field */
    mac_ie_proc_chwidth_field(pst_mac_vap, &(pst_dmac_user->st_user_base_info),(puc_data[MAC_ACTION_OFFSET_ACTION + 1] & BIT0));
}

#endif


oal_uint8 dmac_ap_up_rx_action(dmac_vap_stru *pst_dmac_vap,oal_netbuf_stru *pst_netbuf,oal_uint16 us_user_idx)
{
    mac_vap_stru                   *pst_mac_vap;
    dmac_user_stru                 *pst_dmac_user;
    oal_uint8                      *puc_data;
    oal_uint8                       uc_go_on = OAL_FALSE;
    dmac_rx_ctl_stru               *pst_rx_ctl;

    if ((OAL_PTR_NULL == pst_dmac_vap) || (OAL_PTR_NULL == pst_netbuf))
    {
        OAM_ERROR_LOG2(0, OAM_SF_RX, "{dmac_ap_up_rx_action::pst_dmac_vap = [%p], pst_netbuf = [%p]!}", pst_dmac_vap, pst_netbuf);
        return OAL_FALSE;
    }
    pst_dmac_user = mac_res_get_dmac_user(us_user_idx);
    if (OAL_PTR_NULL == pst_dmac_user)
    {
        OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX,"{dmac_ap_up_rx_action::pst_dmac_user[%d] null.}", us_user_idx);
        return OAL_FALSE;
    }

    pst_mac_vap    = &pst_dmac_vap->st_vap_base_info;    /* ��ȡ֡��ָ�� */
    pst_rx_ctl     = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
#ifdef  _PRE_PRODUCT_ID_HI110X_DEV
    if(pst_rx_ctl)
    {
        //avoid complier warning!!!
    }
#endif
    /* ��ȡ֡��ָ�� */
//    puc_data = OAL_NETBUF_PAYLOAD(pst_netbuf);
	/* ������ȡ֡��ָ��ĺ������󣬻�ȡ��ָ��Ϊ֡��MACͷ��ַ ���Ϊ���淽�� */
    puc_data = MAC_GET_RX_PAYLOAD_ADDR(&(pst_rx_ctl->st_rx_info), pst_netbuf);
//    us_frame_len = MAC_GET_RX_CB_PAYLOAD_LEN(pst_rx_info);  /*֡�峤��*/

    switch (puc_data[MAC_ACTION_OFFSET_CATEGORY])
    {
        case MAC_ACTION_CATEGORY_BA:
        {
            switch(puc_data[MAC_ACTION_OFFSET_ACTION])
            {
                case MAC_BA_ACTION_ADDBA_REQ:
                    uc_go_on = OAL_TRUE;
                    break;

                case MAC_BA_ACTION_ADDBA_RSP:
                    uc_go_on = OAL_TRUE;
                    break;

                case MAC_BA_ACTION_DELBA:
                    uc_go_on = OAL_TRUE;
                    break;

                default:
                    break;
            }
        }
        break;

        case MAC_ACTION_CATEGORY_PUBLIC:
        {
            /* Action */
            switch (puc_data[MAC_ACTION_OFFSET_ACTION])
            {
#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
                case MAC_PUB_COEXT_MGMT:
                    /*20/40�������֡��dmac���������ϱ�host*/
                    uc_go_on = OAL_FALSE;
                    dmac_ap_up_rx_2040_coext(pst_dmac_vap, pst_netbuf);
                    break;
#endif  /* _PRE_WLAN_FEATURE_20_40_80_COEXIST */

                case MAC_PUB_VENDOR_SPECIFIC:
#ifdef _PRE_WLAN_FEATURE_P2P
                    /*����OUI-OUI typeֵΪ 50 6F 9A - 09 (WFA P2P v1.0)  */
                    /* ����hmac_rx_mgmt_send_to_host�ӿ��ϱ� */
                    uc_go_on = OAL_TRUE;
#endif  /* _PRE_WLAN_FEATURE_P2P */
                    break;

#ifdef _PRE_WLAN_FEATURE_FTM
                case MAC_PUB_FTM_REQ:
                    uc_go_on = OAL_FALSE;
                    dmac_ftm_rsp_rx_ftm_req(pst_dmac_vap, pst_netbuf);
                    break;
#endif

                default:
                    break;
            }
        }
        break;

        case MAC_ACTION_CATEGORY_HT:
        {
            /* Action */
            switch (puc_data[MAC_ACTION_OFFSET_ACTION])
            {
#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
                case MAC_HT_ACTION_NOTIFY_CHANNEL_WIDTH:
                    uc_go_on = OAL_FALSE;
                    dmac_rx_notify_channel_width(pst_mac_vap,puc_data,pst_dmac_user);
                    dmac_ap_rx_notify_channel_width(&(pst_dmac_vap->st_vap_base_info), pst_dmac_user, puc_data);
                    break;
#endif
#ifdef _PRE_WLAN_FEATURE_SMPS
                case MAC_HT_ACTION_SMPS:
                    /* m2s���action֡�������ƣ���Ҫʱ��ͬ����host�� */
                    uc_go_on = OAL_FALSE;
                    dmac_mgmt_rx_smps_frame(pst_mac_vap, pst_dmac_user, puc_data);
                    break;
#endif
                case MAC_HT_ACTION_BUTT:
                default:
                    break;
            }
        }
        break;

#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)
        case MAC_ACTION_CATEGORY_SA_QUERY:
        {
            /* Action */
            switch (puc_data[MAC_ACTION_OFFSET_ACTION])
            {
                case MAC_SA_QUERY_ACTION_REQUEST:
                    uc_go_on = OAL_TRUE;
                    break;
                case MAC_SA_QUERY_ACTION_RESPONSE:
                    uc_go_on = OAL_TRUE;
                    break;
                default:
                    break;
            }
        }
        break;
#endif
#ifdef _PRE_WLAN_FEATURE_11V
        case MAC_ACTION_CATEGORY_WNM:
        {
            switch(puc_data[MAC_ACTION_OFFSET_ACTION])
            {
                /* BSS transition query֡������� �޴������ �ݲ����� */
                case MAC_WNM_ACTION_BSS_TRANSITION_MGMT_QUERY:
                    /* TO DO :dmac_rx_bsst_query_action */
                    break;
                /* BSS transition response֡������� */
                case MAC_WNM_ACTION_BSS_TRANSITION_MGMT_RESPONSE:
                    dmac_rx_bsst_rsp_action(pst_dmac_vap, pst_dmac_user, pst_netbuf);
                    uc_go_on = OAL_FALSE;
                    break;
                default:
                    break;
            }
        }
        break;
 #endif

        case MAC_ACTION_CATEGORY_VHT:
        {
            switch(puc_data[MAC_ACTION_OFFSET_ACTION])
            {
            #ifdef _PRE_WLAN_FEATURE_OPMODE_NOTIFY
                case MAC_VHT_ACTION_OPREATING_MODE_NOTIFICATION:
                    uc_go_on = OAL_FALSE;
                    dmac_mgmt_rx_opmode_notify_frame(pst_dmac_vap, pst_dmac_user, pst_netbuf);
                    break;
            #endif
                case MAC_VHT_ACTION_BUTT:
                default:
                    break;
            }
        }
        break;

        case MAC_ACTION_CATEGORY_VENDOR:
        {
#ifdef _PRE_WLAN_FEATURE_P2P
            /*����OUI-OUI typeֵΪ 50 6F 9A - 09 (WFA P2P v1.0)  */
            /* ����hmac_rx_mgmt_send_to_host�ӿ��ϱ� */
            uc_go_on = OAL_TRUE;
#endif
        }
        break;

#ifdef _PRE_WLAN_FEATURE_WMMAC
        case MAC_ACTION_CATEGORY_WMMAC_QOS:
        {
            if (OAL_TRUE == g_en_wmmac_switch)
            {
                switch(puc_data[MAC_ACTION_OFFSET_ACTION])
                {
                    case MAC_WMMAC_ACTION_ADDTS_REQ:
                        uc_go_on = OAL_TRUE;
                        break;

                    default:
                        break;
                }
            }
        }
        break;
#endif //_PRE_WLAN_FEATURE_WMMAC

#ifdef _PRE_WLAN_FEATURE_11K_EXTERN
        case MAC_ACTION_CATEGORY_RADIO_MEASURMENT:
        {
            switch (puc_data[MAC_ACTION_OFFSET_ACTION])
            {
                case MAC_RM_ACTION_RADIO_MEASUREMENT_REQUEST:
                    uc_go_on = OAL_TRUE;
                    break;

                case MAC_RM_ACTION_NEIGHBOR_REPORT_REQUEST:
                    uc_go_on = OAL_TRUE;
                    break;

                case MAC_RM_ACTION_RADIO_MEASUREMENT_REPORT:
                    uc_go_on = OAL_TRUE;
                    break;

                case MAC_RM_ACTION_NEIGHBOR_REPORT_RESPONSE:
                    uc_go_on = OAL_TRUE;
                    break;

                default:
                    break;
            }
        }
        break;
#endif //_PRE_WLAN_FEATURE_11K

        default:
        break;
    }

    return uc_go_on;
}


oal_uint8 dmac_ap_up_rx_action_nonuser(dmac_vap_stru *pst_dmac_vap,oal_netbuf_stru *pst_netbuf)
{
    //mac_vap_stru                   *pst_mac_vap;
    oal_uint8                      *puc_data;
    oal_uint8                       uc_go_on = OAL_FALSE;
    dmac_rx_ctl_stru               *pst_rx_ctl;

    if ((OAL_PTR_NULL == pst_dmac_vap) || (OAL_PTR_NULL == pst_netbuf))
    {
        OAM_ERROR_LOG2(0, OAM_SF_RX, "{dmac_ap_up_rx_action_nonuser::pst_dmac_vap = [%p], pst_netbuf = [%p]!}", pst_dmac_vap, pst_netbuf);
        return OAL_FALSE;
    }

    //pst_mac_vap    = &pst_dmac_vap->st_vap_base_info;    /* ��ȡ֡��ָ�� */
    pst_rx_ctl     = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
#ifdef  _PRE_PRODUCT_ID_HI110X_DEV
    if(pst_rx_ctl)
    {
        //avoid complier warning!!!
    }
#endif
    /* ��ȡ֡��ָ�� */
//    puc_data = OAL_NETBUF_PAYLOAD(pst_netbuf);
	/* ������ȡ֡��ָ��ĺ������󣬻�ȡ��ָ��Ϊ֡��MACͷ��ַ ���Ϊ���淽�� */
    puc_data = MAC_GET_RX_PAYLOAD_ADDR(&(pst_rx_ctl->st_rx_info), pst_netbuf);
//    us_frame_len = MAC_GET_RX_CB_PAYLOAD_LEN(pst_rx_info);  /*֡�峤��*/

    switch (puc_data[MAC_ACTION_OFFSET_CATEGORY])
    {
        case MAC_ACTION_CATEGORY_PUBLIC:
        {
            /* Action */
            switch (puc_data[MAC_ACTION_OFFSET_ACTION])
            {
#ifdef _PRE_WLAN_FEATURE_FTM
                case MAC_PUB_FTM_REQ:
                    uc_go_on = OAL_FALSE;
                    dmac_ftm_rsp_rx_ftm_req(pst_dmac_vap, pst_netbuf);
                    break;
#endif

                default:
                    break;
            }
        }
        break;

        default:
        break;
    }

    return uc_go_on;
}


oal_uint8 dmac_sta_up_rx_action(dmac_vap_stru *pst_dmac_vap,oal_netbuf_stru *pst_netbuf,oal_uint16 us_user_idx)
{
    mac_vap_stru                   *pst_mac_vap;
    dmac_user_stru                 *pst_dmac_user;
    dmac_rx_ctl_stru               *pst_rx_ctl;
    mac_rx_ctl_stru                *pst_rx_info;
    oal_uint8                      *puc_data;
    oal_uint16                      us_frame_len;
    oal_uint8                       uc_go_on = OAL_TRUE;   //action֡Ĭ���ϱ�host

    if ((OAL_PTR_NULL == pst_dmac_vap) || (OAL_PTR_NULL == pst_netbuf))
    {
        OAM_ERROR_LOG2(0, OAM_SF_RX, "{dmac_sta_up_rx_action::pst_dmac_vap = [%p], pst_netbuf = [%p]!}",
                        pst_dmac_vap, pst_netbuf);
        return OAL_FALSE;
    }

    pst_dmac_user = mac_res_get_dmac_user(us_user_idx);
    if (OAL_PTR_NULL == pst_dmac_user)
    {
        OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX,
                       "{dmac_sta_up_rx_action::pst_dmac_user[%d] null.}", us_user_idx);
        return OAL_FALSE;
    }

    pst_mac_vap    = &pst_dmac_vap->st_vap_base_info;

    pst_rx_ctl     = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
    pst_rx_info    = (mac_rx_ctl_stru *)(&(pst_rx_ctl->st_rx_info));

    /* ��ȡ֡��ָ�� */
    puc_data = MAC_GET_RX_PAYLOAD_ADDR(pst_rx_info,pst_netbuf);
    us_frame_len = MAC_GET_RX_CB_PAYLOAD_LEN(pst_rx_info);  /*֡�峤��*/

    /* Category values 128 - 255 Error*/
    if (puc_data[MAC_ACTION_OFFSET_CATEGORY] > MAC_ACTION_CATEGORY_VENDOR)
    {
        return OAL_FALSE;
    }

    switch (puc_data[MAC_ACTION_OFFSET_CATEGORY])
    {
        case MAC_ACTION_CATEGORY_HT:
        {
            /* Action */
            switch (puc_data[MAC_ACTION_OFFSET_ACTION])
            {
#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
                case MAC_HT_ACTION_NOTIFY_CHANNEL_WIDTH:
                    uc_go_on = OAL_FALSE;
                    dmac_rx_notify_channel_width(pst_mac_vap,puc_data,pst_dmac_user);
                    break;
#endif
                case MAC_HT_ACTION_BUTT:
                default:
                    break;
            }
        }
        break;

        case MAC_ACTION_CATEGORY_PUBLIC:
        {
            switch (puc_data[MAC_ACTION_OFFSET_ACTION])
            {
#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
                case MAC_PUB_COEXT_MGMT:
                    uc_go_on = OAL_FALSE;
                    dmac_sta_up_rx_2040_coext(&(pst_dmac_vap->st_vap_base_info), puc_data);
                  break;
#endif  /* _PRE_WLAN_FEATURE_20_40_80_COEXIST */
                case MAC_PUB_EX_CH_SWITCH_ANNOUNCE:
                    uc_go_on = OAL_FALSE;
                    dmac_sta_up_rx_ext_ch_switch(pst_mac_vap,puc_data,us_frame_len);
                    break;

#ifdef _PRE_WLAN_FEATURE_FTM
                case MAC_PUB_FTM:
                    uc_go_on = OAL_FALSE;
                    dmac_sta_rx_ftm(pst_dmac_vap, pst_netbuf);
                    break;
#endif
                default:
                    break;
            }
        }
        break;

        case MAC_ACTION_CATEGORY_SPECMGMT:
        {
            switch (puc_data[MAC_ACTION_OFFSET_ACTION])
            {
                case MAC_SPEC_CH_SWITCH_ANNOUNCE:
                    uc_go_on = OAL_FALSE;
                    dmac_sta_up_rx_ch_switch(pst_mac_vap,puc_data,us_frame_len);
                    break;

                default:
                    break;
            }
        }
        break;

        case MAC_ACTION_CATEGORY_VHT:
        {
            switch(puc_data[MAC_ACTION_OFFSET_ACTION])
            {
            #ifdef _PRE_WLAN_FEATURE_OPMODE_NOTIFY
                case MAC_VHT_ACTION_OPREATING_MODE_NOTIFICATION:
                    uc_go_on = OAL_FALSE;
                    //staģʽ��ʱ��֧��֡����
                    //dmac_mgmt_rx_opmode_notify_frame(pst_dmac_vap, pst_dmac_user, pst_netbuf);
                    break;
            #endif
                case MAC_VHT_ACTION_BUTT:
                default:
                    break;
            }
        }
        break;

        case MAC_ACTION_CATEGORY_RADIO_MEASURMENT:
        {
            switch (puc_data[MAC_ACTION_OFFSET_ACTION])
            {
                case MAC_RM_ACTION_RADIO_MEASUREMENT_REQUEST:
#if defined(_PRE_WLAN_FEATURE_11K) || defined(_PRE_WLAN_FEATURE_FTM)
                    dmac_rrm_proc_rm_request(pst_dmac_vap, pst_netbuf);
                    uc_go_on = OAL_FALSE;
#endif
                    break;

                case MAC_RM_ACTION_LINK_MEASUREMENT_REQUEST:
#ifdef _PRE_WLAN_FEATURE_11K
                    dmac_rrm_get_link_req_info(pst_dmac_vap, puc_data);
                    dmac_rrm_send_link_meas_rpt_action(pst_dmac_vap, pst_netbuf);
                    uc_go_on = OAL_FALSE;
#endif //_PRE_WLAN_FEATURE_11K
                    break;

                case MAC_RM_ACTION_NEIGHBOR_REPORT_RESPONSE:
                    break;

                default:
                    break;
            }
        }
        break;


#ifdef _PRE_WLAN_FEATURE_11V
    case MAC_ACTION_CATEGORY_WNM:
    {
        switch (puc_data[MAC_ACTION_OFFSET_ACTION])
            {
                /* bss transition query ֡������� */
                case MAC_WNM_ACTION_BSS_TRANSITION_MGMT_REQUEST:
                {
                    dmac_rx_bsst_req_action(pst_dmac_vap, pst_dmac_user, pst_netbuf);
                    uc_go_on = OAL_FALSE;
                }
                    break;
                default:
                    break;
            }

    }
    break;
#endif // _PRE_WLAN_FEATURE_11V

#ifdef _PRE_WLAN_FEATURE_WMMAC
        case MAC_ACTION_CATEGORY_WMMAC_QOS:
        {
            if(OAL_TRUE == g_en_wmmac_switch)
            {
                switch (puc_data[MAC_ACTION_OFFSET_ACTION])
                {
                    case MAC_WMMAC_ACTION_ADDTS_RSP:
                        dmac_mgmt_rx_addts_rsp(pst_dmac_vap, pst_dmac_user, puc_data);
                        break;

                    case MAC_WMMAC_ACTION_DELTS:
                        dmac_mgmt_rx_delts(pst_dmac_vap, pst_dmac_user, puc_data);
                        break;

                    default:
                        break;
                }
            }
        }
        break;
#endif //_PRE_WLAN_FEATURE_WMMAC

        default:
        break;
    }

    return uc_go_on;
}

#ifdef _PRE_WLAN_FEATURE_M2S

OAL_STATIC oal_void dmac_rx_beacon_ant_change(dmac_vap_stru *pst_dmac_vap,
                                                            oal_netbuf_stru *pst_netbuf)
{
    hal_to_dmac_device_stru *pst_hal_device;
    dmac_rx_ctl_stru        *pst_rx_ctl;
    oal_uint8                  uc_rssi_abs = 0;
    oal_int8                   c_ant0_rssi;
    oal_int8                   c_ant1_rssi;
    hal_rx_ant_rssi_mgmt_stru  *pst_hal_rx_ant_rssi_mgmt;

    pst_hal_device = pst_dmac_vap->pst_hal_device;

    if(OAL_PTR_NULL == pst_hal_device)
    {
        OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX, "{dmac_rx_beacon_ant_change::pst_hal_device null.}");
        return;
    }

    pst_hal_rx_ant_rssi_mgmt = GET_HAL_DEVICE_RX_ANT_RSSI_MGMT(pst_hal_device);

    /* ��ΪMIMO�������� */
    if (HAL_M2S_STATE_MIMO != GET_HAL_M2S_CUR_STATE(pst_hal_device))
    {
        return;
    }

    if(OAL_TRUE != pst_hal_rx_ant_rssi_mgmt->en_ant_rssi_sw)
    {
        return;
    }

    pst_rx_ctl = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
    oal_rssi_smooth(&pst_hal_rx_ant_rssi_mgmt->s_ant0_rssi_smth, pst_rx_ctl->st_rx_statistic.c_ant0_rssi);
    oal_rssi_smooth(&pst_hal_rx_ant_rssi_mgmt->s_ant1_rssi_smth, pst_rx_ctl->st_rx_statistic.c_ant1_rssi);

    c_ant0_rssi = oal_get_real_rssi(pst_hal_rx_ant_rssi_mgmt->s_ant0_rssi_smth);
    c_ant1_rssi = oal_get_real_rssi(pst_hal_rx_ant_rssi_mgmt->s_ant1_rssi_smth);

    uc_rssi_abs = (oal_uint8)OAL_ABSOLUTE_SUB(c_ant0_rssi, c_ant1_rssi);

    if (OAL_TRUE == pst_hal_rx_ant_rssi_mgmt->en_log_print)
    {
        OAM_WARNING_LOG4(0, OAM_SF_CFG, "dmac_rx_beacon_ant_change::c_rssi_abs[%d],ant0[%d],ant1[%d],rssi_th[%d]",
                           uc_rssi_abs,(oal_int32)c_ant0_rssi,(oal_int32)c_ant1_rssi,pst_hal_rx_ant_rssi_mgmt->uc_rssi_th);
    }

    if(uc_rssi_abs > pst_hal_rx_ant_rssi_mgmt->uc_rssi_th)
    {
        if(c_ant0_rssi > c_ant1_rssi)
        {
            /* �е�c0 */
            dmac_m2s_mgmt_switch(pst_hal_device,pst_dmac_vap,WLAN_TX_CHAIN_ZERO);
            if (OAL_TRUE == pst_hal_rx_ant_rssi_mgmt->en_log_print)
            {
                OAM_WARNING_LOG0(0, OAM_SF_CFG,"dmac_rx_beacon_ant_change::use c0");
            }
        }
        else
        {
            /* �е�c1 */
            dmac_m2s_mgmt_switch(pst_hal_device,pst_dmac_vap,WLAN_TX_CHAIN_ONE);
            if (OAL_TRUE == pst_hal_rx_ant_rssi_mgmt->en_log_print)
            {
                OAM_WARNING_LOG0(0, OAM_SF_CFG,"dmac_rx_beacon_ant_change::use c1");
            }
        }
    }
}
#endif


OAL_STATIC oal_void dmac_rx_sta_up_beacon_mgmt(dmac_vap_stru *pst_dmac_vap,
                                               mac_ieee80211_frame_stru   *pst_frame_hdr,
                                               oal_uint8 auc_bssid[],
                                               oal_netbuf_stru *pst_netbuf,
                                               oal_uint8 *pen_go_on)
{

    /* ��ȡBeacon֡�е�mac��ַ����AP��mac��ַ */
    #if (!defined(HI1102_EDA)) && (!defined(HI110x_EDA))
    if(0 == oal_memcmp(auc_bssid, pst_dmac_vap->st_vap_base_info.auc_bssid, WLAN_MAC_ADDR_LEN))
    {
        #ifdef _PRE_WLAN_FEATURE_STA_PM
        dmac_psm_process_tim_elm(pst_dmac_vap, pst_netbuf);
        #endif

        #ifdef _PRE_WLAN_FEATURE_P2P
        if (IS_P2P_CL((&pst_dmac_vap->st_vap_base_info)))
        {
            /* ��ȡGO Beacon֡�е�NoA��Ѷ */
            dmac_process_p2p_noa(pst_dmac_vap, pst_netbuf);
        }

        if ((IS_P2P_CL(&pst_dmac_vap->st_vap_base_info))&&
        (IS_P2P_OPPPS_ENABLED(pst_dmac_vap)))
        {
            dmac_p2p_oppps_ctwindow_start_event(pst_dmac_vap);
        }
        #endif  /* #ifdef _PRE_WLAN_FEATURE_P2P */

        #ifdef _PRE_WLAN_FEATURE_TSF_SYNC
        dmac_sync_tsf_by_bcn(pst_dmac_vap, pst_netbuf);
        #endif

        #ifdef _PRE_WLAN_FEATURE_11K
        if (OAL_TRUE == pst_dmac_vap->bit_11k_enable)
        {
            dmac_rrm_proc_pwr_constraint(pst_dmac_vap, pst_netbuf);
            dmac_rrm_parse_quiet(pst_dmac_vap, pst_netbuf);
        }
        #endif //_PRE_WLAN_FEATURE_11K

        #ifdef _PRE_WLAN_FEATURE_M2S
        dmac_rx_beacon_ant_change(pst_dmac_vap,pst_netbuf);
        #endif
    }
    #else
    #ifdef _PRE_WLAN_FEATURE_STA_PM
    dmac_psm_process_tim_elm(pst_dmac_vap, pst_netbuf);
    #endif
    #endif /*(!defined(HI1102_EDA))*/
    #ifndef HI110x_EDA
    dmac_sta_up_rx_beacon(pst_dmac_vap, pst_netbuf, pen_go_on);
    #endif
}

#ifdef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT

OAL_STATIC oal_void dmac_rx_ap_save_sensing_bssid_info(dmac_vap_stru *pst_dmac_vap,
                                                                mac_ieee80211_frame_stru *pst_frame_hdr,
                                                                dmac_rx_ctl_stru *pst_rx_ctl)
{
    dmac_sensing_bssid_list_stru           *pst_sensing_bssid_list;
    dmac_sensing_bssid_list_member_stru    *pst_sensing_bssid_member;
    oal_dlist_head_stru                   *pst_entry;
    oal_dlist_head_stru                   *pst_entry_temp;

    pst_sensing_bssid_list = &(pst_dmac_vap->st_sensing_bssid_list);
    oal_spin_lock(&(pst_sensing_bssid_list->st_lock));
    OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_entry, pst_entry_temp, &(pst_sensing_bssid_list->st_list_head))
    {
        pst_sensing_bssid_member = OAL_DLIST_GET_ENTRY(pst_entry, dmac_sensing_bssid_list_member_stru, st_dlist);
        if (0 == oal_memcmp(pst_sensing_bssid_member->auc_mac_addr, pst_frame_hdr->auc_address2, WLAN_MAC_ADDR_LEN))
        {
            oal_int8 c_rssi_temp = pst_rx_ctl->st_rx_statistic.c_rssi_dbm + HMAC_FBT_RSSI_ADJUST_VALUE;
            if (c_rssi_temp < 0)
            {
                c_rssi_temp = 0;
            }
            if (c_rssi_temp > HMAC_FBT_RSSI_MAX_VALUE)
            {
                c_rssi_temp = HMAC_FBT_RSSI_MAX_VALUE;
            }
            pst_sensing_bssid_member->c_rssi = c_rssi_temp;
            pst_sensing_bssid_member->ul_timestamp = (oal_uint32)OAL_TIME_GET_STAMP_MS();
            break;
        }
    }
    /* ����֮ǰ���ͷ��� */
    oal_spin_unlock(&(pst_sensing_bssid_list->st_lock));
}

#endif


OAL_STATIC oal_void dmac_rx_ap_mgmt(dmac_vap_stru *pst_dmac_vap,
                                            mac_ieee80211_frame_stru   *pst_frame_hdr,
                                            oal_uint8 auc_bssid[],
                                            oal_netbuf_stru *pst_netbuf,
                                            oal_uint8 *pen_go_on,
                                            dmac_rx_ctl_stru *pst_rx_ctl)
{
        oal_uint32                  ul_ret;
        oal_uint16                  us_user_idx = 0;
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
        mac_rx_ctl_stru            *pst_rx_info;
        oal_uint8                  *puc_wps_ie      = OAL_PTR_NULL;
        oal_uint8                  *puc_payload     = OAL_PTR_NULL;
        oal_uint16                  us_msg_len;         /* ��Ϣ�ܳ���,������FCS */
#ifdef _PRE_WLAN_FEATURE_HILINK
        oal_uint8                  *puc_hilink_ie   = OAL_PTR_NULL;
        oal_uint8                  *puc_sa;
#endif
#endif

    if (MAC_VAP_STATE_UP == pst_dmac_vap->st_vap_base_info.en_vap_state && WLAN_VAP_MODE_BSS_AP == pst_dmac_vap->st_vap_base_info.en_vap_mode)
    {
#ifdef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT
        if (WLAN_BEACON == pst_frame_hdr->st_frame_control.bit_sub_type)
        {
            dmac_rx_ap_save_sensing_bssid_info(pst_dmac_vap, pst_frame_hdr, pst_rx_ctl);
        }
#endif
        if (WLAN_PROBE_REQ == pst_frame_hdr->st_frame_control.bit_sub_type)
        {
            *pen_go_on      = OAL_FALSE;
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
            pst_rx_info     = (mac_rx_ctl_stru *)(&(pst_rx_ctl->st_rx_info));
            puc_payload     = MAC_GET_RX_PAYLOAD_ADDR(pst_rx_info, pst_netbuf);
            us_msg_len      = pst_rx_info->us_frame_len;                          /* ��Ϣ�ܳ���,������FCS */
            //puc_wps_ie      = mac_get_wps_ie(puc_payload, us_msg_len, 0);
            puc_wps_ie      = mac_find_vendor_ie(MAC_WLAN_OUI_MICROSOFT, MAC_WLAN_OUI_TYPE_MICROSOFT_WPS, puc_payload, us_msg_len);
            if (OAL_PTR_NULL != puc_wps_ie)
            {
                *pen_go_on = OAL_TRUE;
            }
#ifdef _PRE_WLAN_FEATURE_HILINK
            puc_hilink_ie      = mac_find_vendor_ie(MAC_WLAN_OUI_HUAWEI, MAC_WLAN_OUI_TYPE_HUAWEI_HILINK, puc_payload, us_msg_len);
            if (OAL_PTR_NULL != puc_hilink_ie)
            {
                mac_rx_get_sa(pst_frame_hdr, &puc_sa);
                OAM_INFO_LOG4(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX, "{dmac rx hilink probe req:seq num=%d,mac[%x:%x:%x]}",
                                                                                    pst_frame_hdr->bit_seq_num,puc_sa[3],puc_sa[4],puc_sa[5]);

                *pen_go_on = OAL_TRUE;
            }
#endif
#endif

            //OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX, "{dmac_ap_up_rx_probe}");
            dmac_ap_up_rx_probe_req(pst_dmac_vap, pst_netbuf,pst_frame_hdr->auc_address2,pst_rx_ctl->st_rx_statistic.c_rssi_dbm);
        }
        else if (WLAN_AUTH == pst_frame_hdr->st_frame_control.bit_sub_type)
        {
            *pen_go_on = OAL_TRUE;
#ifdef _PRE_WLAN_FEATURE_BAND_STEERING
            if(DMAC_BSD_RET_BLOCK == dmac_bsd_rx_auth_req_frame_handle(pst_dmac_vap,pst_frame_hdr->auc_address2,pst_netbuf))
            {
                *pen_go_on = OAL_FALSE;
            }
#endif
        }
        else if(WLAN_ACTION == pst_frame_hdr->st_frame_control.bit_sub_type)
        {
#ifdef _PRE_WLAN_FEATURE_P2P
            if (IS_P2P_GO((&pst_dmac_vap->st_vap_base_info)))
            {
                *pen_go_on = dmac_p2p_listen_rx_mgmt(pst_dmac_vap, pst_netbuf);
            }
#endif
            ul_ret = mac_vap_find_user_by_macaddr(&(pst_dmac_vap->st_vap_base_info),  pst_frame_hdr->auc_address2, &us_user_idx);
            if(OAL_SUCC != ul_ret)
            {
                *pen_go_on = dmac_ap_up_rx_action_nonuser(pst_dmac_vap,pst_netbuf);
            }
            else/* ap��upʱ���յ�action֡�Ĵ���Ŀǰ����smps action��opmode notify action��������Ҫ�������Ų���˽ӿ� */
            {
                *pen_go_on = dmac_ap_up_rx_action(pst_dmac_vap,pst_netbuf,us_user_idx);
            }

        }
        else if (WLAN_DISASOC == pst_frame_hdr->st_frame_control.bit_sub_type||(WLAN_DEAUTH == pst_frame_hdr->st_frame_control.bit_sub_type))
        {
            if(0 != oal_memcmp(auc_bssid, pst_dmac_vap->st_vap_base_info.auc_bssid, WLAN_MAC_ADDR_LEN))
            {
                *pen_go_on = OAL_FALSE;
                OAM_INFO_LOG4(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX,
                                "{dmac_rx_filter_mgmt::drop  disasoc frame! bssid not match! local bssid[%x:X:X:%x:%x:%x]}",
                                  pst_dmac_vap->st_vap_base_info.auc_bssid[0],
                                  pst_dmac_vap->st_vap_base_info.auc_bssid[3],
                                  pst_dmac_vap->st_vap_base_info.auc_bssid[4],
                                  pst_dmac_vap->st_vap_base_info.auc_bssid[5]);
            }
        }

    }
    else if(MAC_VAP_STATE_PAUSE == pst_dmac_vap->st_vap_base_info.en_vap_state && WLAN_VAP_MODE_BSS_AP == pst_dmac_vap->st_vap_base_info.en_vap_mode)
    {
        /* green ap֧�� */
        if (WLAN_PROBE_REQ == pst_frame_hdr->st_frame_control.bit_sub_type)
        {
            *pen_go_on = OAL_FALSE;
        }
    }
}

OAL_STATIC oal_void dmac_rx_sta_mgmt(dmac_vap_stru *pst_dmac_vap,
                                            mac_ieee80211_frame_stru   *pst_frame_hdr,
                                            oal_uint8 auc_bssid[],
                                            oal_netbuf_stru *pst_netbuf,
                                            oal_uint8 *pen_go_on,
                                            dmac_rx_ctl_stru *pst_rx_ctl)
{
    oal_uint32                  ul_ret;
    oal_uint16                  us_user_idx;
#ifdef _PRE_WLAN_FEATURE_P2P
    oal_uint8                   *puc_p2p0_mac_addr;
#endif

    if (MAC_VAP_STATE_UP == pst_dmac_vap->st_vap_base_info.en_vap_state)
    {
        if(WLAN_ACTION == pst_frame_hdr->st_frame_control.bit_sub_type)
        {
        #ifdef _PRE_WLAN_FEATURE_P2P
            /* P2P0�豸�����յ�actionȫ���ϱ� */
            puc_p2p0_mac_addr = mac_mib_get_p2p0_dot11StationID(&pst_dmac_vap->st_vap_base_info);
            if (0 == oal_compare_mac_addr(pst_frame_hdr->auc_address1, puc_p2p0_mac_addr))
            {
                *pen_go_on = OAL_TRUE;
                return;
            }
        #endif
            ul_ret = mac_vap_find_user_by_macaddr(&(pst_dmac_vap->st_vap_base_info), pst_frame_hdr->auc_address2, &us_user_idx);
            if (OAL_SUCC != ul_ret)
            {
                *pen_go_on = OAL_FALSE;
            }
            else
            {
                *pen_go_on = dmac_sta_up_rx_action(pst_dmac_vap, pst_netbuf, us_user_idx);
            }
        }
    }

    if (0 == oal_memcmp(auc_bssid, pst_dmac_vap->st_vap_base_info.auc_bssid, WLAN_MAC_ADDR_LEN))
    {
        if (WLAN_PROBE_RSP == pst_frame_hdr->st_frame_control.bit_sub_type)
        {
            
            /* ��ȡprobe rsp �е��ŵ� */
            dmac_vap_linkloss_channel_clean(pst_dmac_vap, pst_netbuf);
            *pen_go_on = OAL_FALSE;
            
        }
        else
        {
            /* UP״̬�£��÷�rsp֡����Ĺ���֡����RSSI; ��UP״̬�£���(re)assoc rsp����RSSI */
            if ((MAC_VAP_STATE_UP == pst_dmac_vap->st_vap_base_info.en_vap_state)
              || ((MAC_VAP_STATE_UP != pst_dmac_vap->st_vap_base_info.en_vap_state)
              && ((WLAN_ASSOC_RSP == pst_frame_hdr->st_frame_control.bit_sub_type)
                || (WLAN_REASSOC_RSP == pst_frame_hdr->st_frame_control.bit_sub_type))))
            {
                /* RSSIƽ��, ֵ���浽dmac_vap�� */
                oal_rssi_smooth(&(pst_dmac_vap->st_query_stats.s_signal), pst_rx_ctl->st_rx_statistic.c_rssi_dbm);

                /* ����֡����SNR��Ϣ */
                dmac_vap_update_snr_info(pst_dmac_vap, pst_rx_ctl, pst_frame_hdr);
            }
        }
        if (WLAN_AUTH == pst_frame_hdr->st_frame_control.bit_sub_type)
        {
            pst_dmac_vap->en_auth_received = OAL_TRUE;
        }
        if (WLAN_REASSOC_RSP == pst_frame_hdr->st_frame_control.bit_sub_type)
        {
            pst_dmac_vap->en_assoc_rsp_received = OAL_TRUE;
        }
    }
}


oal_uint32  dmac_rx_filter_mgmt(dmac_vap_stru *pst_dmac_vap, oal_netbuf_stru *pst_netbuf, frw_event_mem_stru *pst_event_mem, oal_uint8 *pen_go_on)
{
    mac_device_stru            *pst_mac_device;
    dmac_rx_ctl_stru           *pst_rx_ctl;
    mac_ieee80211_frame_stru   *pst_frame_hdr;
    oal_uint8                  *puc_frame_body;

    oal_uint8                   auc_bssid[WLAN_MAC_ADDR_LEN];  /* sta������ap mac��ַ */
    oal_uint8                   auc_bad_bssid[WLAN_MAC_ADDR_LEN] = {0x0,0x0,0x0,0x0,0x0,0x0};  /* �Ƿ�bssid��ȫ0 */
    oal_uint32                  ul_ret;
    oal_bool_enum_uint8         en_report_bss = OAL_FALSE;      /* �Ƿ��ϱ���bss */

    OAL_MEMZERO(auc_bssid, OAL_SIZEOF(auc_bssid));
    /* ��ȡmac device */
    pst_mac_device = mac_res_get_dev(pst_dmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX, "{dmac_rx_filter_mgmt::pst_mac_device is NULL.}");
        return OAL_FAIL;
    }

    /* ��ȡ֡ͷ��Ϣ */
    pst_rx_ctl    = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
    pst_frame_hdr = (mac_ieee80211_frame_stru *)mac_get_rx_cb_mac_hdr(&(pst_rx_ctl->st_rx_info));
    puc_frame_body = MAC_GET_RX_PAYLOAD_ADDR(&(pst_rx_ctl->st_rx_info), pst_netbuf);

    *pen_go_on = OAL_TRUE;  /* �������غ��Ƿ񷢵�HMAC����ʼΪtrue */

    mac_get_bssid((oal_uint8 *)pst_frame_hdr, auc_bssid);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    if (WLAN_BEACON == pst_frame_hdr->st_frame_control.bit_sub_type)
    {
        if(OAL_TRUE == PM_WLAN_IsHostSleep())
        {
            *pen_go_on = OAL_FALSE;
        }
        pst_rx_ctl->st_rx_info.bit_is_beacon = OAL_TRUE;
    }
#endif

    /* STAUT��beacon�����Ⱦ��촦������ʱ������ǰ�ر�ǰ�˽��� */
    if((MAC_VAP_STATE_UP == pst_dmac_vap->st_vap_base_info.en_vap_state)
        && (WLAN_VAP_MODE_BSS_STA == pst_dmac_vap->st_vap_base_info.en_vap_mode)
        && (WLAN_BEACON == pst_frame_hdr->st_frame_control.bit_sub_type))
    {
        dmac_rx_sta_up_beacon_mgmt(pst_dmac_vap, pst_frame_hdr, auc_bssid, pst_netbuf, pen_go_on);
    }

#ifdef _PRE_WLAN_MAC_BUGFIX_TSF_SYNC
    /* ���TSF�������� */
    dmac_rx_mgmt_update_tsf(pst_dmac_vap, pst_frame_hdr, pst_mac_device, pst_netbuf);
#endif

    /* Action Frame Filter */
    if (WLAN_ACTION == pst_frame_hdr->st_frame_control.bit_sub_type)
    {
        /* Category values 128 - 255 Error*/
        if (puc_frame_body[MAC_ACTION_OFFSET_CATEGORY] > MAC_ACTION_CATEGORY_VENDOR)
        {
            *pen_go_on = OAL_FALSE;
            oal_netbuf_free(pst_netbuf);
            return OAL_SUCC;
        }
    }

    /* xiexiaoming:except action no ack for txbf */
    if ((0 == oal_memcmp(auc_bssid, auc_bad_bssid, OAL_SIZEOF(auc_bssid)))
        && (WLAN_ACTION_NO_ACK != pst_frame_hdr->st_frame_control.bit_sub_type))
    {
        OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX, "{dmac_rx_filter_mgmt::drop bssid zero packet}");
        *pen_go_on = OAL_FALSE;
        oal_netbuf_free(pst_netbuf);
        return OAL_SUCC;
    }


#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)

    ul_ret = dmac_11w_rx_filter(pst_dmac_vap, pst_netbuf);
    if (OAL_SUCC != ul_ret)
    {
        /* �鲥����ʧ�ܣ����ϱ�����֡ */
        OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX,
                       "{dmac_rx_mgmt_classify::dmac_11w_rx_filter failed[%d].}", ul_ret);
        *pen_go_on = OAL_FALSE;

        oal_netbuf_free(pst_netbuf);
        return OAL_SUCC;
    }
#endif /* #if(_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT) */

#ifdef _PRE_WLAN_FEATURE_BTCOEX
    dmac_btcoex_rx_mgmt_occupied_check(pst_frame_hdr, pst_dmac_vap);
#endif

    /* �����ǰdevice����ɨ��״̬�������յ��Ĺ���֡ */
    if (MAC_SCAN_STATE_RUNNING == pst_mac_device->en_curr_scan_state)
    {
        /* ��������ת�Ƶ�ɨ��ģ�鴦��ʱ��ִ�����´��� */
#ifdef _PRE_WLAN_FEATURE_P2P
        if (MAC_SCAN_FUNC_P2P_LISTEN == pst_mac_device->st_scan_params.uc_scan_func)
        {
            /* P2P �豸���յ�����֡ */
            if (!IS_LEGACY_VAP((&pst_dmac_vap->st_vap_base_info)))
            {
                *pen_go_on = dmac_p2p_listen_rx_mgmt(pst_dmac_vap, pst_netbuf);
            }
        }
        else
#endif
        {
            /* ���ɨ�趯������bss��Ϣ����ô����ɨ�����֡���ˣ����ж�Ӧ�Ĵ�����������do nothing  */
            if (pst_mac_device->st_scan_params.uc_scan_func & MAC_SCAN_FUNC_BSS)
            {
// *pen_go_on = OAL_FALSE;

                /* ɨ��״̬��֡���˴��� */
                dmac_scan_mgmt_filter(pst_dmac_vap, pst_netbuf, &en_report_bss, pen_go_on);
            }
        }
    }

    if (WLAN_VAP_MODE_BSS_AP == pst_dmac_vap->st_vap_base_info.en_vap_mode)
    {
        dmac_rx_ap_mgmt(pst_dmac_vap, pst_frame_hdr, auc_bssid, pst_netbuf, pen_go_on, pst_rx_ctl);
        if (WLAN_BEACON == pst_frame_hdr->st_frame_control.bit_sub_type
            ||WLAN_PROBE_RSP == pst_frame_hdr->st_frame_control.bit_sub_type)
        {
            if(MAC_VAP_STATE_UP == pst_dmac_vap->st_vap_base_info.en_vap_state)
            {
                *pen_go_on = OAL_FALSE;
                dmac_ap_up_rx_obss_beacon(pst_dmac_vap, pst_netbuf);
            }

            else if(MAC_VAP_STATE_AP_WAIT_START == pst_dmac_vap->st_vap_base_info.en_vap_state)
            {
                *pen_go_on = OAL_FALSE;
                dmac_ap_wait_start_rx_obss_beacon(pst_mac_device, &(pst_dmac_vap->st_vap_base_info), pst_netbuf);
            }
        }
    }

    if (WLAN_VAP_MODE_BSS_STA == pst_dmac_vap->st_vap_base_info.en_vap_mode)
    {
        dmac_rx_sta_mgmt(pst_dmac_vap, pst_frame_hdr, auc_bssid, pst_netbuf, pen_go_on, pst_rx_ctl);
    }

    if(OAL_TRUE == en_report_bss)
    {
        /*�����ɨ�����Ѿ��ϱ����˴���Ӧ�����ϱ���netbuf,���Ҳ����ͷ�netbuf*/
        *pen_go_on = OAL_FALSE;
        return OAL_SUCC;
    }

    /*�ϱ�sdt���*/
#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
#ifdef _PRE_WLAN_DFT_DUMP_FRAME
    mac_rx_report_80211_frame((oal_uint8 *)&(pst_dmac_vap->st_vap_base_info), (oal_uint8 *)&(pst_rx_ctl->st_rx_info), pst_netbuf, OAM_OTA_TYPE_RX_DMAC_CB);
#endif /* #ifdef _PRE_WLAN_DFT_DUMP_FRAME */
#endif /* #if defined(_PRE_PRODUCT_ID_HI110X_DEV) */

    if (OAL_FALSE == *pen_go_on)
    {
        /* ��netbuf�黹�ڴ�� */
        oal_netbuf_free(pst_netbuf);
    }

    return OAL_SUCC;
}


oal_uint32  dmac_rx_mgmt_classify(
                frw_event_mem_stru             *pst_event_mem,
                frw_event_hdr_stru             *pst_event_hdr,
                mac_vap_stru                   *pst_vap,
                oal_netbuf_stru                *pst_netbuf)
{
    frw_event_stru                    *pst_event;
    dmac_wlan_crx_event_stru          *pst_crx_event;
    dmac_vap_stru                     *pst_dmac_vap;
    dmac_rx_ctl_stru                  *pst_rx_ctl;
    mac_ieee80211_frame_stru          *pst_frame_hdr;
    oal_bool_enum_uint8                en_go_on          = OAL_TRUE;           /* �Ƿ������hmac���� */
    mac_user_stru                     *pst_user;


    /* ��ȡ�¼�ͷ���¼��ṹ��ָ�� */
    pst_event = frw_get_event_stru(pst_event_mem);

    /* �����¼�ͷ�е�VAP ID */
    pst_event_hdr->uc_vap_id = pst_vap->uc_vap_id;

    /* ��ȡ֡ͷ��Ϣ */
    pst_rx_ctl    = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
    pst_frame_hdr = (mac_ieee80211_frame_stru *)mac_get_rx_cb_mac_hdr(&(pst_rx_ctl->st_rx_info));

    /* ת��ΪDMAC VAP */
    pst_dmac_vap = mac_res_get_dmac_vap(pst_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(pst_vap->uc_vap_id, OAM_SF_RX, "{dmac_rx_mgmt_classify::pst_dmac_vap null.}");

        OAM_STAT_VAP_INCR(0, rx_mgmt_abnormal_dropped, 1);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_user = (mac_user_stru *)mac_res_get_dmac_user(MAC_GET_RX_CB_TA_USER_IDX(&(pst_rx_ctl->st_rx_info)));

    /* ����֡����Ŀǰû���ϱ���HMAC��ֱ��return */
    if (WLAN_CONTROL == pst_frame_hdr->st_frame_control.bit_type)
    {
        if (OAL_PTR_NULL != pst_user)
        {
            dmac_alg_rx_cntl_notify(&(pst_dmac_vap->st_vap_base_info), pst_user, pst_netbuf);
        }

        dmac_rx_process_control(pst_dmac_vap, pst_netbuf, &en_go_on);
        if (OAL_FALSE == en_go_on)
        {
            oal_netbuf_free(pst_netbuf);
            return OAL_SUCC;
        }
    }

    /* DMAC����֡���� */
    if (WLAN_MANAGEMENT == pst_frame_hdr->st_frame_control.bit_type)
    {
#ifdef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT
        if ((1 << pst_frame_hdr->st_frame_control.bit_sub_type) & pst_vap->us_mgmt_frame_filters)
        {
            oal_uint32 ul_ret;
            pst_rx_ctl->st_rx_info.bit_mgmt_to_hostapd = 1;
            FRW_EVENT_HDR_MODIFY_PIPELINE_AND_SUBTYPE(pst_event_hdr, DMAC_WLAN_CRX_EVENT_SUB_TYPE_RX);

            pst_crx_event = (dmac_wlan_crx_event_stru *)(pst_event->auc_event_data);
            pst_crx_event->pst_netbuf = pst_netbuf;
            ul_ret = frw_event_dispatch_event(pst_event_mem);
            if (ul_ret != OAL_SUCC)
            {
                OAM_ERROR_LOG1(pst_event_hdr->uc_vap_id, OAM_SF_RX,
                               "{dmac_rx_mgmt_classify::frw_event_dispatch_event fail, ret = %d.}", ul_ret);
            }
        }
#endif
        /*dmac_rx_filter_mgmt����ܻ����netbuf���ݣ���������֪ͨ�㷨*/
        if (OAL_PTR_NULL != pst_user)
        {
            dmac_alg_rx_mgmt_notify(&(pst_dmac_vap->st_vap_base_info), pst_user, pst_netbuf);
        }
        dmac_rx_filter_mgmt(pst_dmac_vap, pst_netbuf, pst_event_mem, &en_go_on);
    }
#ifdef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT
    pst_rx_ctl->st_rx_info.bit_mgmt_to_hostapd = 0;
#endif
    if (OAL_TRUE == en_go_on)
    {
        /* ��������֡ͳһ�ϱ���HMAC���� */
        /* �̳��¼��������޸��¼�ͷ����ʱδ����STA��APģʽ */
        FRW_EVENT_HDR_MODIFY_PIPELINE_AND_SUBTYPE(pst_event_hdr, DMAC_WLAN_CRX_EVENT_SUB_TYPE_RX);

        pst_crx_event = (dmac_wlan_crx_event_stru *)(pst_event->auc_event_data);
        pst_crx_event->pst_netbuf = pst_netbuf;

        /* �ַ� */
        return frw_event_dispatch_event(pst_event_mem);
    }

    return OAL_SUCC;
}




oal_uint32  dmac_rx_multi_mgmt_frame(
                frw_event_mem_stru             *pst_event_mem,
                frw_event_hdr_stru             *pst_event_hdr,
                oal_netbuf_stru                *pst_netbuf)
{
    mac_device_stru    *pst_device;
    mac_vap_stru       *pst_mac_vap;
    oal_netbuf_stru    *pst_netbuf_copy = OAL_PTR_NULL;
    dmac_rx_ctl_stru   *pst_rx_ctrl;
    oal_uint8           uc_vap_idx;
    oal_uint32          ul_rslt;

    mac_ieee80211_frame_stru    *pst_mac_header;
    oal_bool_enum_uint8          en_orig_netbuffer_used = OAL_FALSE;
    oal_uint8                    uc_channel_number;
    oal_uint8                    uc_subtype;

    pst_device = mac_res_get_dev(pst_event_hdr->uc_device_id);
    if (OAL_PTR_NULL == pst_device)
    {
        OAM_ERROR_LOG1(pst_event_hdr->uc_vap_id, OAM_SF_RX,
                       "{dmac_rx_multi_mgmt_frame::pst_device null, uc_device_id=%d.}", pst_event_hdr->uc_device_id);

        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��device�µ��豸��Ϊ0ʱ��ֱ���ͷŽ��յ��İ� */
    if (0 == pst_device->uc_vap_num)
    {
        //OAM_INFO_LOG0(pst_event_hdr->uc_vap_id, OAM_SF_RX, "{dmac_rx_multi_mgmt_frame::the device have none vap.}\r\n");

        oal_netbuf_free(pst_netbuf);
        return OAL_SUCC;
    }

    pst_rx_ctrl       = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
    pst_mac_header    = (mac_ieee80211_frame_stru *)mac_get_rx_cb_mac_hdr(&(pst_rx_ctrl->st_rx_info));
    uc_channel_number = pst_rx_ctrl->st_rx_info.uc_channel_number;
    uc_subtype        = mac_get_frame_type_and_subtype((oal_uint8 *)pst_mac_header);

    if (pst_rx_ctrl->st_rx_info.us_frame_len > WLAN_MGMT_NETBUF_SIZE)
    {
        OAM_WARNING_LOG1(pst_event_hdr->uc_vap_id, OAM_SF_RX, "{dmac_rx_multi_mgmt_frame::rx mgmt frame exceed memblk size.rx_frame_len[%d]}", pst_rx_ctrl->st_rx_info.us_frame_len);

        oal_netbuf_free(pst_netbuf);
        return OAL_SUCC;
    }


    /* ��������BSS�Ĺ㲥����֡��ת����ÿһ��VAPһ�� */
    for (uc_vap_idx = 0; uc_vap_idx < pst_device->uc_vap_num; uc_vap_idx++)
    {
        pst_mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_device->auc_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_mac_vap)
        {
            OAM_WARNING_LOG1(pst_event_hdr->uc_vap_id, OAM_SF_RX, "{dmac_rx_multi_mgmt_frame::pst_mac_vap null, vap id=%d.", pst_device->auc_vap_id[uc_vap_idx]);
            //return OAL_ERR_CODE_PTR_NULL;
            continue;
        }

        if (OAL_FALSE == dmac_rx_multi_mgmt_pre_process(pst_device, pst_mac_vap, uc_channel_number, uc_subtype))
        {
            //OAM_WARNING_LOG0(pst_device->auc_vap_id[uc_vap_idx], OAM_SF_RX, "{dmac_rx_multi_mgmt_frame::probe req or beacon do not dispatch to the vap which do not needed.}");
            continue;
        }

        /* ���һ��VAPֱ�ӷ���ԭ����netbuf */
        if (uc_vap_idx == pst_device->uc_vap_num - 1)
        {
            pst_netbuf_copy = pst_netbuf;
            en_orig_netbuffer_used = OAL_TRUE;
        }
        else
        {
        #ifdef _PRE_WLAN_HW_TEST
            /* ����״̬ */
            hal_to_dmac_device_stru *pst_hal_device;
            pst_hal_device = DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap);
            if (HAL_ALWAYS_RX_RESERVED == pst_hal_device->bit_al_rx_flag)
            {
                pst_netbuf_copy = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, HAL_AL_RX_FRAME_LEN, OAL_NETBUF_PRIORITY_MID);
            }
            else
        #endif
            {
                pst_netbuf_copy = OAL_MEM_NETBUF_ALLOC(OAL_MGMT_NETBUF, WLAN_MGMT_NETBUF_SIZE, OAL_NETBUF_PRIORITY_HIGH);
            }

            if (OAL_PTR_NULL == pst_netbuf_copy)
            {
                OAM_WARNING_LOG0(pst_event_hdr->uc_vap_id, OAM_SF_RX, "{dmac_rx_multi_mgmt_frame::alloc netbuff failed.}");
                OAL_MEM_INFO_PRINT(OAL_MEM_POOL_ID_NETBUF);
                continue;
            }
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
            OAL_MEM_NETBUF_TRACE(pst_netbuf_copy, OAL_TRUE);
#endif

            pst_rx_ctrl = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf_copy);
            /* ��Ϣ���� */
            oal_memcopy(oal_netbuf_cb(pst_netbuf_copy), oal_netbuf_cb(pst_netbuf), OAL_SIZEOF(dmac_rx_ctl_stru));

#if (!defined(_PRE_PRODUCT_ID_HI110X_DEV))
            oal_memcopy(pst_netbuf_copy->head, pst_netbuf->head, (oal_uint32)(pst_netbuf->end - pst_netbuf->head));
#else
            oal_memcopy(oal_netbuf_header(pst_netbuf_copy), oal_netbuf_header(pst_netbuf), (oal_uint32)(pst_rx_ctrl->st_rx_info.uc_mac_header_len));
            oal_memcopy(oal_netbuf_payload(pst_netbuf_copy), oal_netbuf_payload(pst_netbuf),
                       ((oal_uint32)(pst_rx_ctrl->st_rx_info.us_frame_len) - (oal_uint32)(pst_rx_ctrl->st_rx_info.uc_mac_header_len)));
#endif

            /* ����netbuf���ȡ�TAILָ�� */
            oal_netbuf_set_len(pst_netbuf_copy, oal_netbuf_get_len(pst_netbuf));
            oal_set_netbuf_tail(pst_netbuf_copy, oal_netbuf_data(pst_netbuf_copy) + oal_netbuf_get_len(pst_netbuf_copy));

            /* ����MAC֡ͷ��ָ��(copy�󣬶�Ӧ��mac header��ͷ�Ѿ������仯) */
            mac_set_rx_cb_mac_hdr(&(pst_rx_ctrl->st_rx_info), (oal_uint32 *)oal_netbuf_header(pst_netbuf_copy));
        }

        /* ���ô������֡�ӿ� */
        ul_rslt = dmac_rx_mgmt_classify(pst_event_mem, pst_event_hdr, pst_mac_vap, pst_netbuf_copy);
        if (ul_rslt != OAL_SUCC)
        {
            //OAM_WARNING_LOG1(pst_event_hdr->uc_vap_id, OAM_SF_RX, "{dmac_rx_multi_mgmt_frame::dmac_rx_mgmt_classify failed[%d].", ul_rslt);
            OAL_IO_PRINT("\r\n dmac_rx_multi_mgmt_frame::dmac_rx_mgmt_classify failed[%u].\r\n", ul_rslt);

            oal_netbuf_free(pst_netbuf_copy);
        }
    }

    /* δʹ�ô����netbuffer�� ֱ���ͷ� */
    if(OAL_FALSE == en_orig_netbuffer_used)
    {
        oal_netbuf_free(pst_netbuf);
    }
    return OAL_SUCC;
}



oal_uint32  dmac_rx_process_mgmt(
                frw_event_mem_stru             *pst_event_mem,
                frw_event_hdr_stru             *pst_event_hdr,
                hal_to_dmac_device_stru        *pst_hal_device,
                oal_netbuf_stru                *pst_netbuf)
{
    hal_to_dmac_vap_stru   *pst_hal_vap = OAL_PTR_NULL;
    dmac_rx_ctl_stru       *pst_cb_ctrl;
    oal_uint8               uc_vap_id;
    mac_vap_stru           *pst_vap;
    mac_device_stru        *pst_mac_device;

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    mac_ieee80211_frame_stru *pst_mac_ieee80211_frame;
#endif

    pst_cb_ctrl = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);

    pst_mac_device = (mac_device_stru *)mac_res_get_dev(pst_event_hdr->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(pst_event_hdr->uc_vap_id, OAM_SF_RX, "{dmac_rx_process_mgmt::pst_mac_device null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }
#ifdef _PRE_WLAN_DFT_STAT
    /* ����֡����ͳ�� */
    dmac_dft_mgmt_stat_incr(pst_mac_device, oal_netbuf_header(pst_netbuf), MAC_DEV_MGMT_STAT_TYPE_RX);
#endif

    /* ��ȡ��֡��Ӧ��VAP ID */
    uc_vap_id = pst_cb_ctrl->st_rx_info.bit_vap_id;

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    if (mac_is_proxysta_enabled(pst_mac_device) && HAL_VAP_ID_IS_VALID_PSTA(uc_vap_id)
    || !mac_is_proxysta_enabled(pst_mac_device) && HAL_VAP_ID_IS_VALID(uc_vap_id))
#else
    if (HAL_VAP_ID_IS_VALID(uc_vap_id))  /* ���Ա�device�µ�ĳһBSS��֡ */
#endif
    {
        hal_get_hal_vap(pst_hal_device, uc_vap_id, &pst_hal_vap);
        if (OAL_PTR_NULL == pst_hal_vap)
        {
            OAM_ERROR_LOG1(0, OAM_SF_RX, "{dmac_rx_process_mgmt::hal vap(%d) failed!}", uc_vap_id);
            OAM_STAT_VAP_INCR(0, rx_mgmt_abnormal_dropped, 1);

            return OAL_ERR_CODE_PTR_NULL;
        }

        pst_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_hal_vap->uc_mac_vap_id);
        if (OAL_PTR_NULL == pst_vap)
        {
            OAM_ERROR_LOG1(0, OAM_SF_RX, "{dmac_rx_process_mgmt::mac vap is null!}", pst_hal_vap->uc_mac_vap_id);
            OAM_STAT_VAP_INCR(0, rx_mgmt_abnormal_dropped, 1);

            return OAL_ERR_CODE_PTR_NULL;
        }
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
        if (mac_is_proxysta_enabled(pst_mac_device))
        {
            /* ����Ƿ��͸�sta0�Ĺ㲥/�鲥����֡����device�µ�ÿ��vap(����Proxy STA)������һ�� */
            pst_mac_ieee80211_frame = (mac_ieee80211_frame_stru *)oal_netbuf_header(pst_netbuf);
            if ((WLAN_STA0_HAL_VAP_ID == uc_vap_id) && (ETHER_IS_MULTICAST(pst_mac_ieee80211_frame->auc_address1)))
            {
                return dmac_rx_multi_mgmt_frame(pst_event_mem, pst_event_hdr, pst_netbuf);
            }
        }
#endif
        return dmac_rx_mgmt_classify(pst_event_mem, pst_event_hdr, pst_vap, pst_netbuf);
    }
    else if (WLAN_HAL_OHTER_BSS_ID == uc_vap_id)  /* ��������bss�Ĺ㲥����֡ */
    {
        return dmac_rx_multi_mgmt_frame(pst_event_mem, pst_event_hdr, pst_netbuf);
    }
    else                                                /* �쳣֡ */
    {
        OAM_STAT_VAP_INCR(0, rx_mgmt_abnormal_dropped, 1);
        return OAL_ERR_CODE_ARRAY_OVERFLOW;
    }
}


oal_uint8 dmac_tx_process_action(mac_device_stru *pst_device,
                                            dmac_vap_stru *pst_dmac_vap,
                                            mac_tx_ctl_stru *pst_tx_ctl,
                                            oal_netbuf_stru *pst_action_frame)
{
    oal_uint32                  ul_action_info_offset;
    dmac_ctx_action_event_stru *pst_ctx_action_event;
    oal_uint32                  ul_ret               = OAL_SUCC;
    oal_bool_enum_uint8         en_need_go_on        = OAL_FALSE;

    if (MAC_GET_CB_MPDU_LEN(pst_tx_ctl) < OAL_SIZEOF(dmac_ctx_action_event_stru))
    {
        OAM_ERROR_LOG1(0, OAM_SF_TX, "{dmac_tx_process_action::MPDU_LEN[%d] err}", MAC_GET_CB_MPDU_LEN(pst_tx_ctl));
        oal_netbuf_free(pst_action_frame);
        return en_need_go_on;
    }

    /* ����֡�󲿱��������Ϣ */
    ul_action_info_offset   = MAC_GET_CB_MPDU_LEN(pst_tx_ctl) - OAL_SIZEOF(dmac_ctx_action_event_stru);
    pst_ctx_action_event    = (dmac_ctx_action_event_stru *)(oal_netbuf_data(pst_action_frame) + ul_action_info_offset);

    switch (MAC_GET_CB_FRAME_SUBTYPE(pst_tx_ctl))
    {
        case WLAN_ACTION_BA_ADDBA_REQ:
            ul_ret = dmac_mgmt_tx_addba_req(pst_device, pst_dmac_vap, pst_ctx_action_event, pst_action_frame);
            break;

        case WLAN_ACTION_BA_ADDBA_RSP:
            ul_ret = dmac_mgmt_tx_addba_rsp(pst_device, pst_dmac_vap, pst_ctx_action_event, pst_action_frame);
            break;

        case WLAN_ACTION_BA_DELBA:
            ul_ret = dmac_mgmt_tx_delba(pst_device, pst_dmac_vap, pst_ctx_action_event, pst_action_frame);
            break;

#ifdef _PRE_WLAN_FEATURE_WMMAC
        case WLAN_ACTION_BA_WMMAC_ADDTS_REQ:
            ul_ret = dmac_mgmt_tx_addts_req(pst_dmac_vap, pst_ctx_action_event, pst_action_frame);
            break;

        case WLAN_ACTION_BA_WMMAC_ADDTS_RSP:
            ul_ret = dmac_mgmt_tx_addts_rsp(pst_dmac_vap, pst_ctx_action_event, pst_action_frame);
            break;

        case WLAN_ACTION_BA_WMMAC_DELTS:
            ul_ret = dmac_mgmt_tx_delts(pst_dmac_vap, pst_ctx_action_event, pst_action_frame);
            break;
#endif  //_PRE_WLAN_FEATURE_WMMAC

        default:
            en_need_go_on = OAL_TRUE;
            break;
    }

    /* ����֡����ʧ��,ͳһ�ͷ��ڴ� */
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG2(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX,
                         "{dmac_tx_process_action::process action ret[%d] subtype[%d].}",
                          ul_ret, MAC_GET_CB_FRAME_SUBTYPE(pst_tx_ctl));
        oal_netbuf_free(pst_action_frame);
    }

    return en_need_go_on;
}


oal_uint32  dmac_tx_process_mgmt_event(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru          *pst_event;
    frw_event_hdr_stru      *pst_event_hdr;
    dmac_tx_event_stru      *pst_ctx_event;
    dmac_vap_stru           *pst_dmac_vap;
    oal_netbuf_stru         *pst_mgmt_frame;
    mac_tx_ctl_stru         *pst_tx_ctl;
    oal_uint32               ul_ret;
    oal_uint8                uc_mgmt_frm_type;
    mac_user_stru           *pst_mac_user;
    mac_device_stru         *pst_mac_device;

    if ((OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(0, OAM_SF_TX, "{dmac_tx_process_mgmt_event::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡ�¼����¼�ͷ�Լ��¼�payload�ṹ�� */
    pst_event       = frw_get_event_stru(pst_event_mem);
    pst_event_hdr   = &(pst_event->st_event_hdr);
    pst_ctx_event   = (dmac_tx_event_stru *)pst_event->auc_event_data;
    pst_mgmt_frame  = pst_ctx_event->pst_netbuf;

    oal_set_netbuf_next(pst_mgmt_frame, OAL_PTR_NULL);
    oal_set_netbuf_prev(pst_mgmt_frame, OAL_PTR_NULL);

    /* ��ȡdevice�ṹ����Ϣ */
    pst_mac_device = mac_res_get_dev(pst_event_hdr->uc_device_id);
    if(OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG1(pst_event_hdr->uc_vap_id, OAM_SF_TX, "{dmac_tx_process_mgmt_event::pst_device[%d] null.}", pst_event_hdr->uc_device_id);
        oal_netbuf_free(pst_mgmt_frame);
        return OAL_SUCC;
    }

    /* ��ȡvap�ṹ��Ϣ */
    pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_event_hdr->uc_vap_id);
    if(OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG1(pst_event_hdr->uc_vap_id, OAM_SF_TX, "{dmac_tx_process_mgmt_event::pst_dmac_vap null.}", pst_event_hdr->uc_vap_id);
        oal_netbuf_free(pst_mgmt_frame);
        return OAL_SUCC;
    }

    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_mgmt_frame);

    if (OAL_TRUE == MAC_GET_CB_HAS_EHTER_HEAD(pst_tx_ctl))
    {
        OAM_ERROR_LOG1(pst_event_hdr->uc_vap_id, OAM_SF_TX, "{dmac_tx_process_mgmt_event::EER.}", pst_event_hdr->uc_vap_id);
    }

    /* ����host������action֡ */
    if (WLAN_CB_FRAME_TYPE_ACTION == MAC_GET_CB_FRAME_TYPE(pst_tx_ctl))
    {
        if (OAL_TRUE != dmac_tx_process_action(pst_mac_device, pst_dmac_vap, pst_tx_ctl, pst_mgmt_frame))
        {
            return OAL_SUCC;
        }
    }

    /* ��дnetbuf��cb�ֶΣ������͹���֡�ͷ�����ɽӿ�ʹ�� */
    MAC_SET_CB_FRAME_HEADER_ADDR(pst_tx_ctl, (mac_ieee80211_frame_stru *)oal_netbuf_header(pst_mgmt_frame));
    MAC_GET_CB_WME_AC_TYPE(pst_tx_ctl) = WLAN_WME_AC_MGMT;

    uc_mgmt_frm_type = mac_get_frame_type_and_subtype((oal_uint8 *)MAC_GET_CB_FRAME_HEADER_ADDR(pst_tx_ctl));
    if (uc_mgmt_frm_type == (WLAN_FC0_SUBTYPE_AUTH|WLAN_FC0_TYPE_MGT))
    {
        pst_mac_user = mac_res_get_mac_user(MAC_GET_CB_TX_USER_IDX(pst_tx_ctl));
        if (OAL_PTR_NULL != pst_mac_user)
        {
            dmac_tid_clear(pst_mac_user, pst_mac_device);
        }
    }

    ul_ret = dmac_tx_mgmt(pst_dmac_vap, pst_mgmt_frame, pst_ctx_event->us_frame_len);
    if (OAL_SUCC != ul_ret)
    {
        oal_netbuf_free(pst_mgmt_frame);
        return ul_ret;
    }

    return OAL_SUCC;
}


oal_uint32  dmac_rx_process_sync_event(frw_event_mem_stru *pst_event_mem)
{

    frw_event_stru             *pst_event;
    frw_event_hdr_stru         *pst_event_hdr;
    dmac_ctx_action_event_stru *pst_crx_action_sync;
    mac_device_stru            *pst_device;
    mac_vap_stru               *pst_mac_vap;

    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RX, "{dmac_rx_process_sync_event::pst_event_mem null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡ�¼����¼�ͷ�Լ��¼�payload�ṹ�� */
    pst_event               = frw_get_event_stru(pst_event_mem);
    pst_event_hdr           = &(pst_event->st_event_hdr);
    pst_crx_action_sync    = (dmac_ctx_action_event_stru *)pst_event->auc_event_data;

    /* ��ȡdevice�ṹ����Ϣ */
    pst_device = mac_res_get_dev(pst_event_hdr->uc_device_id);

    /* ��ȡvap�ṹ��Ϣ */
    pst_mac_vap = (mac_vap_stru *)mac_res_get_dmac_vap(pst_event_hdr->uc_vap_id);

    OAM_INFO_LOG2(0, OAM_SF_RX, "{dmac_rx_process_sync_event::category %d, action %d.}",
                  pst_crx_action_sync->en_action_category, pst_crx_action_sync->uc_action);
    switch (pst_crx_action_sync->en_action_category)
    {
        case MAC_ACTION_CATEGORY_BA:
            switch (pst_crx_action_sync->uc_action)
            {
                case MAC_BA_ACTION_ADDBA_RSP:
                    dmac_mgmt_rx_addba_rsp(pst_device, MAC_GET_DMAC_VAP(pst_mac_vap), pst_crx_action_sync);
                    break;

                case MAC_BA_ACTION_DELBA:
                    dmac_mgmt_rx_delba(pst_device, MAC_GET_DMAC_VAP(pst_mac_vap), pst_crx_action_sync);
                    break;

                default:
                    OAM_WARNING_LOG0(pst_event_hdr->uc_vap_id, OAM_SF_RX, "{dmac_rx_process_sync_event::invalid ba action type.}");
                    break;
            }
            break;

        default:
            OAM_WARNING_LOG0(pst_event_hdr->uc_vap_id, OAM_SF_RX, "{dmac_rx_process_sync_event::invalid ba action category.}");
            break;
    }

    return OAL_SUCC;
}


oal_uint32  dmac_tx_complete_mgmt_notify(
                hal_to_dmac_device_stru *pst_hal_device,
                dmac_user_stru         *pst_dmac_user,
                hal_tx_dscr_stru       *pst_dscr,
                oal_netbuf_stru        *pst_buf)
{
    mac_tx_ctl_stru                *pst_tx_ctl;
    oal_uint8                       uc_tid;

    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_buf);

    if (OAL_TRUE == mac_is_delba_frame(pst_buf, &uc_tid))
    {
        pst_dmac_user->ast_tx_tid_queue[uc_tid].en_is_delba_ing = OAL_FALSE;
        return OAL_SUCC;
    }

#ifdef _PRE_WLAN_FEATURE_BTCOEX
    dmac_btcoex_tx_addba_rsp_check(pst_buf, &pst_dmac_user->st_user_base_info);
#endif

    if (OAL_TRUE == (oal_bool_enum_uint8)MAC_GET_CB_IS_BAR(pst_tx_ctl))
    {
        dmac_tid_resume(pst_hal_device, &pst_dmac_user->ast_tx_tid_queue[MAC_GET_CB_WME_TID_TYPE(pst_tx_ctl)], DMAC_TID_PAUSE_RESUME_TYPE_BA);
        return OAL_SUCC;
    }

    return OAL_SUCC;
}


oal_void  dmac_mgmt_connect_set_channel(mac_device_stru            *pst_mac_device,
                                                      hal_to_dmac_device_stru    *pst_hal_device,
                                                      mac_vap_stru               *pst_up_vap,
                                                      mac_channel_stru           *pst_channel)
{
    hal_to_dmac_device_stru     *pst_up_vap_hal_device;

    /* ��ǰû�д��ڹ���״̬��VAP������ֱ�����ŵ� */
    if (OAL_PTR_NULL == pst_up_vap)
    {
        OAM_WARNING_LOG4(0, OAM_SF_SCAN, "dmac_mgmt_connect_set_channel: vap_band=%d, chan=%d chan_idx=%d, bw=%d",
                         pst_channel->en_band,
                         pst_channel->uc_chan_number,
                         pst_channel->uc_chan_idx,
                         pst_channel->en_bandwidth);
        /* �л��ŵ���Ҫ��fifo������TRUE */
        dmac_mgmt_switch_channel(pst_hal_device, pst_channel, OAL_TRUE);
        return;
    }

    pst_up_vap_hal_device  = DMAC_VAP_GET_HAL_DEVICE(pst_up_vap);

    /* ���������Ѿ�up�Ĳ�����ͬһ��hal device��,�������Ŀ���ֱ�����Լ���hal device�������ŵ� */
    if ((pst_up_vap_hal_device != pst_hal_device) || (MAC_VAP_STATE_PAUSE == pst_up_vap->en_vap_state))
    {
        OAM_WARNING_LOG4(pst_up_vap->uc_vap_id, OAM_SF_DBDC, "dmac_mgmt_connect_set_channel: up vap[%d]state[%d]hal dev[%d]new hal dev[%d]set channel directly!!!",
                        pst_up_vap->uc_vap_id, pst_up_vap->en_vap_state, pst_up_vap_hal_device->uc_device_id, pst_hal_device->uc_device_id);
        dmac_mgmt_switch_channel(pst_hal_device, pst_channel, OAL_TRUE);
        return;
    }

    OAM_WARNING_LOG4(pst_up_vap->uc_vap_id, OAM_SF_SCAN, "dmac_mgmt_connect_set_channel: has up vap. up_vap_chan:%d bw:%d, new_chan:%d bw:%d",
                         pst_up_vap->st_channel.uc_chan_number,
                         pst_up_vap->st_channel.en_bandwidth,
                         pst_channel->uc_chan_number,
                         pst_channel->en_bandwidth);

    if (pst_channel->uc_chan_number != pst_up_vap->st_channel.uc_chan_number)
    {
        /* ��ͣ�����ŵ���VAP���� */
        dmac_vap_pause_tx_by_chl(pst_mac_device, &(pst_up_vap->st_channel));

        OAM_WARNING_LOG1(pst_up_vap->uc_vap_id, OAM_SF_SCAN, "dmac_mgmt_connect_set_channel: diff chan_num. switch off to chan %d.",
                            pst_channel->uc_chan_number);

        /* ������֡���ŵ����� */
        dmac_switch_channel_off(pst_mac_device, pst_up_vap, pst_channel, 20);
    }
    else
    {
        if (pst_channel->en_bandwidth > pst_up_vap->st_channel.en_bandwidth)
        {
            OAM_WARNING_LOG2(pst_up_vap->uc_vap_id, OAM_SF_SCAN, "dmac_mgmt_connect_set_channel:  same chan_num[%d], switch to bw[%d].",
                            pst_channel->uc_chan_number,
                            pst_channel->en_bandwidth);

            /* �л��ŵ�����Ҫ��fifo������FALSE */
            dmac_mgmt_switch_channel(pst_hal_device, pst_channel, OAL_FALSE);
        }
    }

}


oal_uint32 dmac_join_set_reg_event_process(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru                 *pst_event;
    frw_event_hdr_stru             *pst_event_hdr;
    dmac_ctx_join_req_set_reg_stru *pst_reg_params;
    mac_device_stru                *pst_device;
    dmac_vap_stru                  *pst_dmac_vap;
#ifdef _PRE_WLAN_FEATURE_ROAM
    dmac_user_stru                 *pst_dmac_user;
#endif //_PRE_WLAN_FEATURE_ROAM
    oal_uint8                      *puc_cur_ssid;

    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{dmac_join_set_reg_event_process::pst_event_mem null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡ�¼����¼�ͷ�Լ��¼�payload�ṹ�� */
    pst_event               = frw_get_event_stru(pst_event_mem);
    pst_event_hdr           = &(pst_event->st_event_hdr);
    pst_reg_params          = (dmac_ctx_join_req_set_reg_stru *)pst_event->auc_event_data;

    /* ��ȡdevice�ṹ����Ϣ */
    pst_device = mac_res_get_dev(pst_event_hdr->uc_device_id);

    if (OAL_PTR_NULL == pst_device)
    {
        OAM_ERROR_LOG0(pst_event_hdr->uc_vap_id, OAM_SF_SCAN, "{dmac_join_set_reg_event_process::pst_device null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_event_hdr->uc_vap_id);
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(pst_event_hdr->uc_vap_id, OAM_SF_SCAN, "{dmac_join_set_reg_event_process::pst_dmac_vap null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_dmac_vap->st_vap_base_info.st_cap_flag.bit_dpd_enbale = OAL_TRUE;

    pst_dmac_vap->st_vap_base_info.st_cap_flag.bit_dpd_done   = OAL_FALSE;

    /* ͬ��dmac vap���ŵ���Ϣ */
    pst_dmac_vap->st_vap_base_info.st_channel = pst_reg_params->st_current_channel;

#ifdef _PRE_WLAN_FEATURE_ROAM
    if (MAC_VAP_STATE_ROAMING == pst_dmac_vap->st_vap_base_info.en_vap_state)
    {
        dmac_vap_down_notify(&(pst_dmac_vap->st_vap_base_info));
    }
#endif

/* DBDC�����ŵ��˴�,��֤����ļĴ��������ڶ�Ӧ��hal device�� */
#ifdef _PRE_WLAN_FEATURE_DBDC
    dmac_vap_dbdc_start(pst_device, &(pst_dmac_vap->st_vap_base_info));
#endif

    mac_vap_set_bssid(&pst_dmac_vap->st_vap_base_info, pst_reg_params->auc_bssid);

    /* дSTA BSSID�Ĵ���*/
    hal_set_sta_bssid(pst_dmac_vap->pst_hal_vap, pst_reg_params->auc_bssid);

    /* ͬ��beacon period */
    mac_mib_set_BeaconPeriod(&pst_dmac_vap->st_vap_base_info, pst_reg_params->us_beacon_period);

    if (0 != mac_mib_get_BeaconPeriod(&pst_dmac_vap->st_vap_base_info))
    {
        OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{dmac_join_set_reg_event_process::beacon period[%d]}",pst_reg_params->us_beacon_period);
        /* ��beacon������д��Ĵ��� */
        hal_vap_set_psm_beacon_period(pst_dmac_vap->pst_hal_vap, pst_reg_params->us_beacon_period);
    }
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
    /* ����tbtt offset��λus */
    hal_set_psm_tbtt_offset(pst_dmac_vap->pst_hal_vap, pst_dmac_vap->us_in_tbtt_offset);
#endif
    /* ����sta tbtt��ʱ�� */
    hal_enable_tsf_tbtt(pst_dmac_vap->pst_hal_vap, OAL_FALSE);

#ifdef _PRE_WLAN_FEATURE_ROAM
    if (MAC_VAP_STATE_ROAMING == pst_dmac_vap->st_vap_base_info.en_vap_state)
    {
#ifdef _PRE_WLAN_FEATURE_DBDC
        if (HAL_VAP_STATE_INIT == pst_dmac_vap->pst_hal_vap->en_hal_vap_state)
        {
            hal_device_handle_event(pst_dmac_vap->pst_hal_device, HAL_DEVICE_EVENT_JOIN_COMP, OAL_SIZEOF(hal_to_dmac_vap_stru), (oal_uint8 *)(pst_dmac_vap->pst_hal_vap));\
            hal_device_handle_event(pst_dmac_vap->pst_hal_device, HAL_DEVICE_EVENT_VAP_UP, OAL_SIZEOF(hal_to_dmac_vap_stru), (oal_uint8 *)(pst_dmac_vap->pst_hal_vap));
        }
#endif
        dmac_vap_work_set_channel(pst_dmac_vap);
    }
#endif //_PRE_WLAN_FEATURE_ROAM

/* �����02ģʽ�£���Ҫ���ŵ���Ϣͬ����dmac vap�ṹ���� */
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    oal_memcopy(&(pst_dmac_vap->st_vap_base_info.st_channel), &(pst_reg_params->st_current_channel), OAL_SIZEOF(mac_channel_stru));
#endif

    dmac_vap_init_tx_ucast_data_frame(pst_dmac_vap);

    /* ��ʼ��Ĭ�Ͻ���beacon֡ */
    /* ��ֹ�����������޸ļĴ�������д�Ĵ�������beacon֡ */
    if(OAL_FALSE == pst_reg_params->ul_beacon_filter)
    {
        /* �ر�beacon֡����  */
        hal_disable_beacon_filter(pst_dmac_vap->pst_hal_device);
    }

    /* ��ʼ��Ĭ�ϲ�����non_direct_frame֡*/
    /* Ϊ��ֹ�����������޸ļĴ�����д�Ĵ���������non_direct_frame֡ */
    if(OAL_TRUE == pst_reg_params->ul_non_frame_filter)
    {
        /* ��non frame֡���� */
        hal_enable_non_frame_filter(pst_dmac_vap->pst_hal_device);
    }

    /* �����Ż�����ͬƵ���µ�������һ�� */
    if (WLAN_BAND_2G == pst_dmac_vap->st_vap_base_info.st_channel.en_band)
    {
//        mac_mib_set_ShortPreambleOptionImplemented(&pst_dmac_vap->st_vap_base_info, WLAN_LEGACY_11B_MIB_SHORT_PREAMBLE);
        mac_mib_set_SpectrumManagementRequired(&pst_dmac_vap->st_vap_base_info, OAL_FALSE);
    }
    else
    {
//        mac_mib_set_ShortPreambleOptionImplemented(&pst_dmac_vap->st_vap_base_info, WLAN_LEGACY_11B_MIB_LONG_PREAMBLE);
        mac_mib_set_SpectrumManagementRequired(&pst_dmac_vap->st_vap_base_info, OAL_TRUE);
    }

    /* ͬ������device��FortyMHzOperationImplemented mib�� */
    mac_mib_set_FortyMHzOperationImplemented(&(pst_dmac_vap->st_vap_base_info), pst_reg_params->en_dot11FortyMHzOperationImplemented);

    /* ����ssid */
    puc_cur_ssid = mac_mib_get_DesiredSSID(&pst_dmac_vap->st_vap_base_info);
    oal_memcopy(puc_cur_ssid, pst_reg_params->auc_ssid, WLAN_SSID_MAX_LEN);
    puc_cur_ssid[WLAN_SSID_MAX_LEN - 1] = '\0';

#ifdef _PRE_WLAN_FEATURE_ROAM
    pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user(pst_dmac_vap->st_vap_base_info.us_assoc_vap_id);
    if (pst_dmac_user)
    {
        /* �����û�mac */
        oal_set_mac_addr(pst_dmac_user->st_user_base_info.auc_user_mac_addr, pst_reg_params->auc_bssid);
        hal_ce_del_peer_macaddr(pst_dmac_vap->pst_hal_device, pst_dmac_user->uc_lut_index);
        hal_ce_add_peer_macaddr(pst_dmac_vap->pst_hal_device, pst_dmac_user->uc_lut_index, pst_reg_params->auc_bssid);
    }

#endif //_PRE_WLAN_FEATURE_ROAM

    return OAL_SUCC;
}





oal_uint32  dmac_process_del_sta(oal_uint32 uc_dev_id, oal_uint8 uc_vap_id)
{
    mac_vap_stru    *pst_vap_up;
    mac_device_stru *pst_device = mac_res_get_dev(uc_dev_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_device))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{dmac_process_del_sta::pst_device[%d] is NULL!}", uc_dev_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* �����vap up����ָ����Ǹ�vap���ڵ��ŵ� */
    pst_vap_up = mac_device_find_another_up_vap(pst_device, uc_vap_id);
    if (OAL_PTR_NULL != pst_vap_up)
    {
        if (MAC_VAP_STATE_PAUSE == pst_vap_up->en_vap_state)
        {
            hal_to_dmac_device_stru *pst_hal_device = DMAC_VAP_GET_HAL_DEVICE(pst_vap_up);

            pst_hal_device->st_hal_scan_params.st_home_channel = pst_vap_up->st_channel;
            OAM_WARNING_LOG2(uc_vap_id, OAM_SF_DBAC, "dmac_process_del_sta: conn fail, switch to up vap%d, channel num:%d",
                            pst_vap_up->uc_vap_id, pst_vap_up->st_channel.uc_chan_number);
            dmac_scan_switch_channel_back(pst_device, pst_hal_device);
        }
    }

    return OAL_SUCC;
}

oal_uint32  dmac_mgmt_conn_result_event(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru                      *pst_event;
    frw_event_hdr_stru                  *pst_event_hdr;
    oal_uint32                           ul_result;

    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ASSOC, "pst_event_mem null.");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡ�¼����¼�ͷ�Լ��¼�payload�ṹ�� */
    pst_event               = frw_get_event_stru(pst_event_mem);
    pst_event_hdr           = &(pst_event->st_event_hdr);
    ul_result               = *((oal_uint32 *)pst_event->auc_event_data);

    if (OAL_SUCC != ul_result)
    {
        dmac_process_del_sta(pst_event_hdr->uc_device_id, pst_event_hdr->uc_vap_id);
    }

    return OAL_SUCC;
}


oal_uint32  dmac_rx_process_priv_req_event(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru             *pst_event;
    frw_event_hdr_stru         *pst_event_hdr;
    mac_priv_req_args_stru     *pst_crx_priv_req_args;
    mac_device_stru            *pst_device;
    mac_vap_stru               *pst_mac_vap;

    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RX, "{dmac_rx_process_priv_req_event::pst_event_mem null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡ�¼����¼�ͷ�Լ��¼�payload�ṹ�� */
    pst_event               = frw_get_event_stru(pst_event_mem);
    pst_event_hdr           = &(pst_event->st_event_hdr);
    pst_crx_priv_req_args   = (mac_priv_req_args_stru *)pst_event->auc_event_data;

    /* ��ȡdevice�ṹ����Ϣ */
    pst_device = mac_res_get_dev(pst_event_hdr->uc_device_id);

    /* ��ȡvap�ṹ��Ϣ */
    pst_mac_vap = (mac_vap_stru *)mac_res_get_dmac_vap(pst_event_hdr->uc_vap_id);

    switch (pst_crx_priv_req_args->uc_type)
    {
        case MAC_A_MPDU_START:

            dmac_mgmt_rx_ampdu_start(pst_device, MAC_GET_DMAC_VAP(pst_mac_vap), pst_crx_priv_req_args);
            break;

        case MAC_A_MPDU_END:
            dmac_mgmt_rx_ampdu_end(pst_device, MAC_GET_DMAC_VAP(pst_mac_vap), pst_crx_priv_req_args);
            break;

        default:

            OAM_WARNING_LOG0(pst_event_hdr->uc_vap_id, OAM_SF_RX, "{dmac_rx_process_priv_req_event::invalid priv action type.}");
            break;
    }

    return OAL_SUCC;
}

#if defined(_PRE_WLAN_FEATURE_11K) || defined(_PRE_WLAN_FEATURE_FTM)

oal_void dmac_rrm_proc_rm_request(dmac_vap_stru* pst_dmac_vap, oal_netbuf_stru *pst_netbuf)
{
    mac_action_rm_req_stru          *pst_rm_req;
    mac_meas_req_ie_stru            *pst_meas_req_ie;
    dmac_rx_ctl_stru                *pst_rx_ctrl;
    oal_uint16                       us_framebody_len;
    oal_uint16                       us_framebody_len_tmp;
    oal_uint16                       us_Measurement_ie_len;
    oal_uint8                       *puc_data;

#ifdef _PRE_WLAN_FEATURE_11K
    mac_rrm_info_stru               *pst_rrm_info;
#endif
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{dmac_rrm_proc_rm_request::pst_dmac_vap NULL!}");
        return;
    }

    pst_rx_ctrl     = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
    if (OAL_PTR_NULL == pst_rx_ctrl)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{dmac_rrm_proc_rm_request::pst_rx_ctrl is NULL}");
        return;
    }

    /* ��ȡ֡��ָ�� */
    puc_data = MAC_GET_RX_PAYLOAD_ADDR(&(pst_rx_ctrl->st_rx_info), pst_netbuf);


    /*֡�峤��*/
    us_framebody_len = pst_rx_ctrl->st_rx_info.us_frame_len - pst_rx_ctrl->st_rx_info.uc_mac_header_len;

    /*************************************************************************/
    /*                    Radio Measurement Request Frame - Frame Body       */
    /* --------------------------------------------------------------------- */
    /* |Category |Action |Dialog Token| Number of Repetitions|Meas Req Eles |*/
    /* --------------------------------------------------------------------- */
    /* |1        |1      | 1          | 2                    |var            */
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /*************************************************************************/
    pst_rm_req = (mac_action_rm_req_stru *)puc_data;

#ifdef _PRE_WLAN_FEATURE_11K
    pst_rrm_info = pst_dmac_vap->pst_rrm_info;
    if (OAL_PTR_NULL == pst_rrm_info)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{dmac_rrm_proc_rm_request::pst_rrm_info is NULL}");
        return;
    }
    pst_rrm_info->st_bcn_req_info.uc_dialog_token = pst_rm_req->uc_dialog_token;
    /* �ظ����Դ����ݲ����� */
    pst_rrm_info->st_bcn_req_info.us_repetition = pst_rm_req->us_num_rpt;
#endif

    /* �Ƿ���Meas Req */
    if (us_framebody_len <= MAC_MEASUREMENT_REQUEST_IE_OFFSET)
    {
#ifdef _PRE_WLAN_FEATURE_11K
        /* ���û��MR IE��Ҳ��һ������Meas Rpt��Radio Meas Rpt */
        /* �������֡�ڴ沢���ͷ����Ϣ */
        if ( OAL_SUCC != dmac_rrm_fill_basic_rm_rpt_action(pst_dmac_vap))
        {
            return;
        }
        dmac_rrm_send_rm_rpt_action(pst_dmac_vap);
#endif
        return;
    }

    /*************************************************************************/
    /*                    Measurement Request IE                             */
    /* --------------------------------------------------------------------- */
    /* |Element ID |Length |Meas Token| Meas Req Mode|Meas Type  | Meas Req |*/
    /* --------------------------------------------------------------------- */
    /* |1          |1      | 1        | 1            |1          |var       |*/
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /*************************************************************************/

    /* �����ж��Measurement Req IEs */
    us_framebody_len_tmp = us_framebody_len - MAC_MEASUREMENT_REQUEST_IE_OFFSET;
    while(us_framebody_len_tmp > MAC_IE_HDR_LEN)
    {
        pst_meas_req_ie = (mac_meas_req_ie_stru *)&puc_data[us_framebody_len - us_framebody_len_tmp];
        us_Measurement_ie_len = pst_meas_req_ie->uc_len;
        us_framebody_len_tmp -= us_Measurement_ie_len;

        if (MAC_EID_MEASREQ == pst_meas_req_ie->uc_eid)
        {
            /* Req�в������Ͷ�Ӧ��report */
            if ((1 == pst_meas_req_ie->st_reqmode.bit_enable) && (0 == pst_meas_req_ie->st_reqmode.bit_rpt))
            {
                OAM_WARNING_LOG0(0, OAM_SF_RRM, "{rpt now allowed!}");
                continue;
            }

#ifdef _PRE_WLAN_FEATURE_11K
            pst_rrm_info->st_bcn_req_info.uc_meas_token = pst_meas_req_ie->uc_token;
            pst_rrm_info->st_bcn_req_info.uc_meas_type  = pst_meas_req_ie->uc_reqtype;

            /* ����beacon req */
            if(RM_RADIO_MEAS_BCN == pst_meas_req_ie->uc_reqtype)
            {
                if(OAL_FAIL == dmac_rrm_parse_beacon_req(pst_dmac_vap, pst_meas_req_ie))
                {
                    /* If the STA has no beacon information available */
                    /* then the STA may either refuse the*/
                    /* request or send an empty Beacon Report.*/
                    if ( OAL_SUCC != dmac_rrm_fill_basic_rm_rpt_action(pst_dmac_vap))
                    {
                        oal_netbuf_free(pst_rrm_info->pst_rm_rpt_mgmt_buf);
                        pst_rrm_info->pst_rm_rpt_mgmt_buf = OAL_PTR_NULL;
                        return;
                    }
                    pst_rrm_info->pst_meas_rpt_ie                           = (mac_meas_rpt_ie_stru *)pst_rrm_info->pst_rm_rpt_action->auc_rpt_ies;
                    pst_rrm_info->pst_meas_rpt_ie->st_rptmode.bit_incapable = 1;

                    dmac_rrm_encap_meas_rpt(pst_dmac_vap);
                    dmac_rrm_send_rm_rpt_action(pst_dmac_vap);
                    return;
                }
            }
#endif

#ifdef _PRE_WLAN_FEATURE_FTM
            /* ����FTM Range req */
            if(RM_RADIO_MEASUREMENT_FTM_RANGE == pst_meas_req_ie->uc_reqtype)
            {
                dmac_rrm_parse_ftm_range_req(pst_dmac_vap, pst_meas_req_ie);
            }
#endif
        }
        /* MR IE���󣬲��أ����� */
        else
        {
            OAM_WARNING_LOG1(0, OAM_SF_RRM, "{Error Request, Expect Measurement Request, but got EID[%d]!}", pst_meas_req_ie->uc_eid);
        }
    }
    return;
}
#endif

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

