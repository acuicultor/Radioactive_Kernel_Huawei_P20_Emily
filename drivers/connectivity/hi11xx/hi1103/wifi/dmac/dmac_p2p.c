


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_P2P


/*****************************************************************************
  1 ͷ�ļ�����
*****************************************************************************/
#include "oal_util.h"
#include "mac_resource.h"
#include "mac_frame.h"
#include "dmac_vap.h"
#include "dmac_p2p.h"
#include "dmac_tx_bss_comm.h"
#include "dmac_config.h"
#include "hal_ext_if.h"
#include "dmac_mgmt_ap.h"
#include "dmac_psm_ap.h"
//#include "mac_pm.h"
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#include "pm_extern.h"
#endif

#ifdef _PRE_WLAN_FEATURE_STA_PM
#include "dmac_sta_pm.h"
#include "dmac_psm_sta.h"
#include "hal_device_fsm.h"
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_P2P_C

/*****************************************************************************
  2 ��̬��������
*****************************************************************************/

/*****************************************************************************
  3 ȫ�ֱ�������
*****************************************************************************/

/*****************************************************************************
  4 ����ʵ��
*****************************************************************************/

oal_void dmac_p2p_go_absence_period_start_sta_prot(dmac_vap_stru * pst_dmac_vap)
{
#ifdef _PRE_WLAN_FEATURE_STA_UAPSD
    mac_sta_pm_handler_stru          *pst_mac_sta_pm_handle;

    oal_uint8   uc_tid = WLAN_TID_MAX_NUM;
    pst_mac_sta_pm_handle = &pst_dmac_vap->st_sta_pm_handler;
    if (OAL_FALSE == pst_mac_sta_pm_handle->en_is_fsm_attached)
    {
        OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_P2P, "{dmac_p2p_go_absence_period_start_sta_prot::pm fsm not attached.}");
        return;
    }

    /* If WMM-PS USP is in process, queue a QoS NULL trigger frame in the    */
    /* highest priority trigger enabled queue                                */
    if(dmac_is_uapsd_sp_not_in_progress(pst_mac_sta_pm_handle) == OAL_FALSE)
    {
        uc_tid = dmac_get_highest_trigger_enabled_priority(pst_dmac_vap);

        if(uc_tid != WLAN_TID_MAX_NUM)
        {
            dmac_send_null_frame_to_ap(pst_dmac_vap, STA_PWR_SAVE_STATE_DOZE, OAL_TRUE);
        }
    }
#endif
}

oal_uint32 dmac_p2p_noa_absent_start_event(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru             *pst_event;
    dmac_vap_stru              *pst_dmac_vap = OAL_PTR_NULL;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(0, OAM_SF_P2P, "{dmac_p2p_noa_absent_start_event::pst_event_mem null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event = frw_get_event_stru(pst_event_mem);
    pst_dmac_vap = mac_res_get_dmac_vap(pst_event->st_event_hdr.uc_vap_id);
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(pst_event->st_event_hdr.uc_vap_id, OAM_SF_P2P, "{dmac_p2p_noa_absent_start_event::pst_dmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (WLAN_VAP_MODE_BSS_STA == pst_dmac_vap->st_vap_base_info.en_vap_mode)
    {
    #ifdef _PRE_WLAN_FEATURE_STA_PM
        if(STA_PWR_SAVE_STATE_DOZE != dmac_psm_get_state(pst_dmac_vap))
        {
            /* Protocol dependent processing */
            dmac_p2p_go_absence_period_start_sta_prot(pst_dmac_vap);

            if(OAL_TRUE == dmac_is_ps_poll_rsp_pending(pst_dmac_vap))
            {
                if (OAL_SUCC != dmac_send_pspoll_to_ap(pst_dmac_vap))
                {
                    OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR, "{dmac_p2p_noa_absent_start_event::dmac_send_pspoll_to_ap fail}");
                }

            }
        }
        if(IS_P2P_NOA_ENABLED(pst_dmac_vap))
        {
            dmac_pm_sta_post_event(pst_dmac_vap, STA_PWR_EVENT_P2P_SLEEP, 0, OAL_PTR_NULL);
        }
    #endif
    }

    else
    {
        if(IS_P2P_NOA_ENABLED(pst_dmac_vap))
        {
            /* ��ͣ���� */

            dmac_p2p_handle_ps(pst_dmac_vap, OAL_TRUE);
        }
    }
    return OAL_SUCC;
}


oal_uint8 dmac_p2p_listen_rx_mgmt(dmac_vap_stru   *pst_dmac_vap,
                                  oal_netbuf_stru *pst_netbuf)
{
    mac_ieee80211_frame_stru   *pst_frame_hdr;
    mac_device_stru            *pst_mac_device;
    oal_uint8                  *puc_frame_body;
    oal_uint16                  us_frame_len;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_dmac_vap) || (OAL_PTR_NULL == pst_netbuf)))
    {
        OAM_ERROR_LOG0(0, OAM_SF_P2P, "{dmac_p2p_listen_rx_mgmt::param null.}");
        return OAL_FALSE;
    }

    pst_mac_device  = (mac_device_stru *)mac_res_get_dev(pst_dmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_P2P, "{dmac_p2p_listen_rx_mgmt::pst_mac_device null.}");
        return OAL_FALSE;
    }

    /* ��ȡ֡��Ϣ */
    pst_frame_hdr  = (mac_ieee80211_frame_stru *)OAL_NETBUF_HEADER(pst_netbuf);
    puc_frame_body = OAL_NETBUF_PAYLOAD(pst_netbuf);
    us_frame_len   = (oal_uint16)oal_netbuf_get_len(pst_netbuf);

    if ((WLAN_ACTION == pst_frame_hdr->st_frame_control.bit_sub_type) &&(OAL_TRUE == mac_is_p2p_action_frame(puc_frame_body)))
    {
        /*�ж��Ƿ���presence request action frame*/
        if(OAL_TRUE == dmac_is_p2p_presence_req_frame(puc_frame_body))
        {
            dmac_process_p2p_presence_req(pst_dmac_vap,pst_netbuf);
            return OAL_FALSE;
        }
        else
        {
            if(OAL_TRUE == dmac_is_p2p_go_neg_req_frame(puc_frame_body) || OAL_TRUE == dmac_is_p2p_pd_disc_req_frame(puc_frame_body))
            {
                /* �ӳ�����ʱ�䣬���ڼ�������ɨ��ӿڣ����ӳ�ɨ�趨ʱ�� */
                if (OAL_TRUE == pst_dmac_vap->pst_hal_device->st_hal_scan_params.st_scan_timer.en_is_enabled)
                {
                    FRW_TIMER_STOP_TIMER(&(pst_dmac_vap->pst_hal_device->st_hal_scan_params.st_scan_timer));
                    FRW_TIMER_RESTART_TIMER(&(pst_dmac_vap->pst_hal_device->st_hal_scan_params.st_scan_timer),
                                              (pst_dmac_vap->pst_hal_device->st_hal_scan_params.us_scan_time), OAL_FALSE);
                }
            }

            /* �����ACTION ֡�����ϱ� */
            return OAL_TRUE;
        }
    }
    else if (WLAN_PROBE_REQ== pst_frame_hdr->st_frame_control.bit_sub_type)
    {
        if (OAL_SUCC != dmac_p2p_listen_filter_vap(pst_dmac_vap))
        {
            return OAL_FALSE;
        }
        if (OAL_SUCC != dmac_p2p_listen_filter_frame(pst_dmac_vap, puc_frame_body, us_frame_len))
        {
            return OAL_FALSE;
        }

        /* ���յ�probe req ֡������probe response ֡ */
        dmac_ap_up_rx_probe_req(pst_dmac_vap, pst_netbuf,(oal_uint8*)OAL_PTR_NULL,0);
    }

    return OAL_TRUE;
}


oal_uint32  dmac_process_p2p_presence_req(dmac_vap_stru *pst_dmac_vap, oal_netbuf_stru *pst_netbuf)
{
    oal_netbuf_stru            *pst_mgmt_buf;
    oal_uint16                  us_mgmt_len;
    mac_tx_ctl_stru            *pst_tx_ctl;
    dmac_rx_ctl_stru           *pst_rx_ctl;
    mac_ieee80211_frame_stru   *pst_frame_hdr;
    oal_uint8                  *puc_frame_body;
    oal_uint32                  ul_ret;
    oal_uint16                  us_user_idx = 0;

    /* ��ȡ֡ͷ��Ϣ */
    pst_rx_ctl    = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
    pst_frame_hdr = (mac_ieee80211_frame_stru *)(mac_get_rx_cb_mac_hdr(&(pst_rx_ctl->st_rx_info)));
    puc_frame_body = MAC_GET_RX_PAYLOAD_ADDR(&(pst_rx_ctl->st_rx_info), pst_netbuf);

    ul_ret = mac_vap_find_user_by_macaddr((&pst_dmac_vap->st_vap_base_info), pst_frame_hdr->auc_address2, &us_user_idx);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_P2P, "{dmac_process_p2p_presence_req::no user.}", ul_ret);
        return ul_ret;
    }

    /* �������֡�ڴ� */
    pst_mgmt_buf = OAL_MEM_NETBUF_ALLOC(OAL_MGMT_NETBUF, WLAN_MGMT_NETBUF_SIZE, OAL_NETBUF_PRIORITY_HIGH);
    if (OAL_PTR_NULL == pst_mgmt_buf)
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_P2P, "{dmac_process_p2p_presence_req::pst_mgmt_buf null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_set_netbuf_prev(pst_mgmt_buf, OAL_PTR_NULL);
    oal_set_netbuf_next(pst_mgmt_buf, OAL_PTR_NULL);

    OAL_MEM_NETBUF_TRACE(pst_mgmt_buf, OAL_TRUE);

    /* ��װpresence request֡ */
    us_mgmt_len = dmac_mgmt_encap_p2p_presence_rsp(pst_dmac_vap, pst_mgmt_buf, pst_frame_hdr->auc_address2, puc_frame_body);
    OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_P2P, "{dmac_process_p2p_presence_req::dmac_mgmt_encap_p2p_presence_rsp. length=%d}\r\n",us_mgmt_len);

    /* ���÷��͹���֡�ӿ� */
    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_mgmt_buf);


    OAL_MEMZERO(pst_tx_ctl, sizeof(mac_tx_ctl_stru));
    MAC_GET_CB_TX_USER_IDX(pst_tx_ctl)   = us_user_idx;
    OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_P2P, "{dmac_process_p2p_presence_req::us_user_idx=%d}\r\n",us_user_idx);
    MAC_GET_CB_WME_AC_TYPE(pst_tx_ctl) = WLAN_WME_AC_MGMT;
    ul_ret = dmac_tx_mgmt(pst_dmac_vap, pst_mgmt_buf, us_mgmt_len);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_P2P, "{dmac_process_p2p_presence_req::dmac_tx_mgmt failed[%d].", ul_ret);
        oal_netbuf_free(pst_mgmt_buf);
        return ul_ret;
    }

    return OAL_SUCC;
}

oal_void dmac_process_p2p_noa(dmac_vap_stru *pst_dmac_vap, oal_netbuf_stru *pst_netbuf)
{
    dmac_rx_ctl_stru            *pst_rx_ctrl;
    mac_rx_ctl_stru             *pst_rx_info;
    oal_uint16                   us_frame_len;
    oal_uint8                   *puc_noa_attr = OAL_PTR_NULL;
    oal_uint8                   *puc_payload;
    mac_cfg_p2p_ops_param_stru   st_p2p_ops;
    mac_cfg_p2p_noa_param_stru   st_p2p_noa;
    oal_uint16                   us_attr_index = 0;
    hal_to_dmac_vap_stru        *pst_hal_vap;
    oal_uint16                   us_attr_len = 0;
    oal_uint32                   ul_current_tsf_lo;

    if (OAL_PTR_NULL == pst_dmac_vap || OAL_PTR_NULL == pst_netbuf)
    {
        //OAM_INFO_LOG0(0, OAM_SF_P2P, "{dmac_process_p2p_noa::param null.}");
        return;
    }
    pst_rx_ctrl         = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
    pst_rx_info         = (mac_rx_ctl_stru *)(&(pst_rx_ctrl->st_rx_info));
#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
    us_frame_len     = pst_rx_info->us_frame_len - pst_rx_info->uc_mac_header_len; /*֡�峤��*/
#else
    us_frame_len     = pst_rx_info->us_frame_len - pst_rx_info->uc_mac_header_len; /*֡�峤��*/
#endif
    puc_payload         = OAL_NETBUF_PAYLOAD(pst_netbuf);

    OAL_MEMZERO(&st_p2p_ops, OAL_SIZEOF(st_p2p_ops));
    OAL_MEMZERO(&st_p2p_noa, OAL_SIZEOF(st_p2p_noa));
    /* ȡ��NoA attr*/
    puc_noa_attr = dmac_get_p2p_noa_attr(puc_payload, us_frame_len, MAC_DEVICE_BEACON_OFFSET, &us_attr_len);
    if (OAL_PTR_NULL == puc_noa_attr)
    {
        if(IS_P2P_NOA_ENABLED(pst_dmac_vap) || IS_P2P_OPPPS_ENABLED(pst_dmac_vap))
        {
            /* ֹͣ���� */
            dmac_p2p_handle_ps(pst_dmac_vap, OAL_FALSE);
            OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_P2P,
                            "{dmac_process_p2p_noa::puc_noa_attr null. stop p2p ps}");

#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
    #ifdef _PRE_WLAN_FEATURE_P2P_NOA_DSLEEP
    /*TODO:mpw2��03 p2p noa��˯�ݲ�֧�֣���pilot��Ӳ�����Ĵ���˯���жϵ����̺��ٴ�*/
            /* ȥʹ��p2p noa�����ж� */
            hal_vap_set_ext_noa_disable(pst_dmac_vap->pst_hal_vap);
    #else
            /*ֹͣp2p noa oppps ʱ�ָ���˯ */
            PM_WLAN_EnableDeepSleep();
    #endif

#endif
        }
        else
        {
            return;
        }
    }
    else
    {
        /* ����ops����*/
        if((puc_noa_attr[us_attr_index] & BIT7) != 0)
        {
            st_p2p_ops.en_ops_ctrl = 1;
            st_p2p_ops.uc_ct_window = (puc_noa_attr[us_attr_index] & 0x7F);
        }

        if(us_attr_len > 2)
        {
            /* ����NoA����*/
            us_attr_index++;
            st_p2p_noa.uc_count = puc_noa_attr[us_attr_index++];
            st_p2p_noa.ul_duration = OAL_MAKE_WORD32(OAL_MAKE_WORD16(puc_noa_attr[us_attr_index],
                                             puc_noa_attr[us_attr_index + 1]),
                                             OAL_MAKE_WORD16(puc_noa_attr[us_attr_index + 2],
                                             puc_noa_attr[us_attr_index + 3]));
            us_attr_index += 4;
            st_p2p_noa.ul_interval = OAL_MAKE_WORD32(OAL_MAKE_WORD16(puc_noa_attr[us_attr_index],
                                             puc_noa_attr[us_attr_index + 1]),
                                             OAL_MAKE_WORD16(puc_noa_attr[us_attr_index + 2],
                                             puc_noa_attr[us_attr_index + 3]));
            us_attr_index += 4;
            st_p2p_noa.ul_start_time = OAL_MAKE_WORD32(OAL_MAKE_WORD16(puc_noa_attr[us_attr_index],
                                             puc_noa_attr[us_attr_index + 1]),
                                             OAL_MAKE_WORD16(puc_noa_attr[us_attr_index + 2],
                                             puc_noa_attr[us_attr_index + 3]));
        }
    }

    pst_hal_vap  = pst_dmac_vap->pst_hal_vap;
    /* ����GO���ܲ���������P2P ops �Ĵ��� */
    if((pst_dmac_vap->st_p2p_ops_param.en_ops_ctrl != st_p2p_ops.en_ops_ctrl)||
        (pst_dmac_vap->st_p2p_ops_param.uc_ct_window != st_p2p_ops.uc_ct_window))
    {
        pst_dmac_vap->st_p2p_ops_param.en_ops_ctrl = st_p2p_ops.en_ops_ctrl;
        pst_dmac_vap->st_p2p_ops_param.uc_ct_window = st_p2p_ops.uc_ct_window;
        OAM_WARNING_LOG2(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_P2P, "dmac_process_p2p_noa:ctrl:%d, ct_window:%d\r\n",
                    pst_dmac_vap->st_p2p_ops_param.en_ops_ctrl,
                    pst_dmac_vap->st_p2p_ops_param.uc_ct_window);

#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
        /* ����p2p opppsֻ��ǳ˯ */
        if (st_p2p_ops.en_ops_ctrl)
        {
            PM_WLAN_DisbaleDeepSleep();
        }
#endif
        /* ����P2P ops �Ĵ��� */
        hal_vap_set_ops(pst_hal_vap, pst_dmac_vap->st_p2p_ops_param.en_ops_ctrl, pst_dmac_vap->st_p2p_ops_param.uc_ct_window);
    }

    /* ����GO���ܲ���������P2P NoA �Ĵ��� */
    if((pst_dmac_vap->st_p2p_noa_param.uc_count != st_p2p_noa.uc_count)||
        (pst_dmac_vap->st_p2p_noa_param.ul_duration != st_p2p_noa.ul_duration)||
        (pst_dmac_vap->st_p2p_noa_param.ul_interval != st_p2p_noa.ul_interval) ||
        (pst_dmac_vap->st_p2p_noa_param.ul_start_time != st_p2p_noa.ul_start_time))
    {
        pst_dmac_vap->st_p2p_noa_param.uc_count = st_p2p_noa.uc_count;
        pst_dmac_vap->st_p2p_noa_param.ul_duration = st_p2p_noa.ul_duration;
        pst_dmac_vap->st_p2p_noa_param.ul_interval = st_p2p_noa.ul_interval;
        pst_dmac_vap->st_p2p_noa_param.ul_start_time = st_p2p_noa.ul_start_time;
        OAM_WARNING_LOG4(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_P2P, "dmac_process_p2p_noa:start_time:%u, duration:%d, interval:%d, count:%d\r\n",
                    pst_dmac_vap->st_p2p_noa_param.ul_start_time,
                    pst_dmac_vap->st_p2p_noa_param.ul_duration,
                    pst_dmac_vap->st_p2p_noa_param.ul_interval,
                    pst_dmac_vap->st_p2p_noa_param.uc_count);

        hal_vap_tsf_get_32bit(pst_hal_vap, &ul_current_tsf_lo);
        if((st_p2p_noa.ul_start_time < ul_current_tsf_lo)&&
          (st_p2p_noa.ul_start_time != 0))
        {
            OAM_WARNING_LOG2(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_P2P, "dmac_process_p2p_noa:start time %u is less than tsf time %u\r\n",
                    st_p2p_noa.ul_start_time,
                    ul_current_tsf_lo);
            st_p2p_noa.ul_start_time = ul_current_tsf_lo + (st_p2p_noa.ul_interval - (ul_current_tsf_lo - st_p2p_noa.ul_start_time)% st_p2p_noa.ul_interval);
            OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_P2P, "dmac_process_p2p_noa:start time update to %u\r\n",
                            st_p2p_noa.ul_start_time);

        }

#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
        if (st_p2p_noa.uc_count)
        {
    #ifdef _PRE_WLAN_FEATURE_P2P_NOA_DSLEEP
    /*TODO:mpw2��03 p2p noa��˯�ݲ�֧�֣���pilot��Ӳ�����Ĵ���˯���жϵ����̺��ٴ�*/
            /* ����ext p2p noa����*/
            hal_vap_set_ext_noa_para(pst_dmac_vap->pst_hal_vap, st_p2p_noa.ul_duration, st_p2p_noa.ul_interval);

            /* ����ext p2p noa offset����*/
            hal_vap_set_ext_noa_offset(pst_dmac_vap->pst_hal_vap,PM_DEFAULT_EXT_TBTT_OFFSET);

            /* ʹ��ext p2p noa�����ж� */
            hal_vap_set_ext_noa_enable(pst_dmac_vap->pst_hal_vap);

            /* ���� p2p noa inner offset����*/
            hal_vap_set_noa_offset(pst_dmac_vap->pst_hal_vap,PM_DEFAULT_STA_INTER_TBTT_OFFSET);

            /* ����ext p2p noa bcn out����*/
            hal_vap_set_noa_timeout_val(pst_dmac_vap->pst_hal_vap,pst_dmac_vap->us_beacon_timeout);
    #else
        /* ����p2p NOAֻ��ǳ˯ */
            PM_WLAN_DisbaleDeepSleep();
    #endif

        }
#endif
        hal_vap_set_noa(pst_hal_vap,
                        st_p2p_noa.ul_start_time,
                        st_p2p_noa.ul_duration,
                        st_p2p_noa.ul_interval,
                        st_p2p_noa.uc_count);


    }
}

oal_void dmac_p2p_handle_ps(dmac_vap_stru * pst_dmac_vap, oal_bool_enum_uint8 en_pause)
{

#ifdef _PRE_WLAN_FEATURE_STA_PM

    mac_vap_stru               *pst_mac_vap = OAL_PTR_NULL;
    dmac_user_stru             *pst_dmac_user = OAL_PTR_NULL;
    mac_device_stru            *pst_mac_device;
    mac_sta_pm_handler_stru    *pst_mac_sta_pm_handle;
    oal_uint8                  uc_noa_not_sleep_flag;

    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_P2P, "{dmac_p2p_handle_ps::pst_dmac_vap null.}");
        return;
    }
    pst_mac_sta_pm_handle = &pst_dmac_vap->st_sta_pm_handler;

    pst_mac_vap = &pst_dmac_vap->st_vap_base_info;

    pst_mac_device  = (mac_device_stru *)mac_res_get_dev(pst_dmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(0, OAM_SF_P2P, "{dmac_p2p_handle_ps::pst_mac_device is null.}");
        return;
    }

     /* dbac������,ֱ��return */
    if ((OAL_TRUE == mac_is_dbac_running(pst_mac_device)))
    {
        return;
    }

    if(pst_mac_device->st_p2p_info.en_p2p_ps_pause == en_pause)
    {
        return;
    }

    if (WLAN_VAP_MODE_BSS_STA == pst_mac_vap->en_vap_mode)
    {
        pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user(pst_mac_vap->us_assoc_vap_id);
        if (OAL_PTR_NULL == pst_dmac_user)
        {
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_P2P,
                "{dmac_p2p_handle_ps::pst_dmac_user[%d] null.}", pst_mac_vap->us_assoc_vap_id);
            return;
        }

        /* P2P CLIENT оƬ�������P2P CLIENT ע�ᣬ�����ܶ�P2P DEVICE ע�� */
        if(OAL_TRUE == en_pause)
        {
            dmac_user_pause(pst_dmac_user);

#ifdef _PRE_WLAN_MAC_BUGFIX_PSM_BACK
            /*suspendӲ������*/
            hal_set_machw_tx_suspend(pst_dmac_vap->pst_hal_device);
            /* ����Ӳ�����У������ڸ��û���֡���Ż�tid */
            dmac_psm_flush_txq_to_tid(pst_mac_device, pst_dmac_vap, pst_dmac_user);
            /* �ָ�Ӳ������ */
            hal_set_machw_tx_resume(pst_dmac_vap->pst_hal_device);
#else
            /* ��֪MAC�û��������ģʽ */
            hal_tx_enable_peer_sta_ps_ctrl(pst_dmac_vap->pst_hal_device, pst_dmac_user->uc_lut_index);
#endif
            pst_mac_device->uc_mac_vap_id = pst_dmac_vap->st_vap_base_info.uc_vap_id;  /* ��¼˯��ʱvap id */

            /* ������˯������,�˴�noa��ǳ˯,���ر�ǰ�� */
            uc_noa_not_sleep_flag = (pst_mac_sta_pm_handle->en_beacon_frame_wait) | (pst_mac_sta_pm_handle->st_null_wait.en_doze_null_wait << 1) | (pst_mac_sta_pm_handle->en_more_data_expected << 2)
                    | (pst_mac_sta_pm_handle->st_null_wait.en_active_null_wait << 3) | (pst_mac_sta_pm_handle->en_direct_change_to_active << 4);

            if (uc_noa_not_sleep_flag == 0)
            {
                hal_device_handle_event(DMAC_VAP_GET_HAL_DEVICE(pst_dmac_vap), HAL_DEVICE_EVENT_VAP_CHANGE_TO_DOZE, OAL_SIZEOF(hal_to_dmac_vap_stru), (oal_uint8 *)(pst_dmac_vap->pst_hal_vap));
                pst_mac_sta_pm_handle->aul_pmDebugCount[PM_MSG_PSM_P2P_SLEEP]++;
            }
        }
        else
        {
            hal_device_handle_event(DMAC_VAP_GET_HAL_DEVICE(pst_dmac_vap), HAL_DEVICE_EVENT_VAP_CHANGE_TO_ACTIVE, OAL_SIZEOF(hal_to_dmac_vap_stru), (oal_uint8 *)(pst_dmac_vap->pst_hal_vap));
            pst_mac_sta_pm_handle->aul_pmDebugCount[PM_MSG_PSM_P2P_AWAKE]++;

#ifndef _PRE_WLAN_MAC_BUGFIX_PSM_BACK
            /* �ָ����û���Ӳ�����еķ��� */
            hal_tx_disable_peer_sta_ps_ctrl(pst_dmac_vap->pst_hal_device, pst_dmac_user->uc_lut_index);
#endif
            dmac_user_resume(pst_dmac_user);   /* �ָ�user */
            /* �����еĻ���֡���ͳ�ȥ */
            dmac_psm_queue_flush(pst_dmac_vap, pst_dmac_user);

            dmac_p2p_resume_send_null_to_ap(pst_dmac_vap,pst_mac_sta_pm_handle);
        }

    }
    else if (WLAN_VAP_MODE_BSS_AP == pst_mac_vap->en_vap_mode)
    {   //P2P GO оƬ������Ҫͨ����������
        if (OAL_TRUE == en_pause)
        {
            dmac_ap_pause_all_user(pst_mac_vap);

            pst_mac_device->uc_mac_vap_id = pst_dmac_vap->st_vap_base_info.uc_vap_id;  /* ��¼˯��ʱvap id */

            //PM_WLAN_PsmHandle(pst_dmac_vap->pst_hal_vap->uc_service_id, PM_WLAN_LIGHTSLEEP_PROCESS);/* ͶƱ���� */
        }
        else
        {
            //PM_WLAN_PsmHandle(pst_dmac_vap->pst_hal_vap->uc_service_id, PM_WLAN_WORK_PROCESS);/* ͶƱ���� */

            dmac_ap_resume_all_user(pst_mac_vap);
        }
    }

    /*��¼Ŀǰp2p����״̬*/
    pst_mac_device->st_p2p_info.en_p2p_ps_pause = en_pause;
#endif
}


oal_uint32 dmac_p2p_noa_absent_end_event(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru             *pst_event;
    mac_device_stru            *pst_mac_device;
    mac_vap_stru               *pst_mac_vap = OAL_PTR_NULL;
    dmac_vap_stru              *pst_dmac_vap;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(0, OAM_SF_P2P, "{dmac_p2p_noa_absent_end_event::pst_event_mem null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event = frw_get_event_stru(pst_event_mem);
    pst_mac_device = mac_res_get_dev(pst_event->st_event_hdr.uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(0, OAM_SF_P2P, "{dmac_p2p_noa_absent_end_event::pst_mac_device is null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_vap = mac_res_get_mac_vap(pst_event->st_event_hdr.uc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
    {
        OAM_ERROR_LOG1(0, OAM_SF_P2P, "{dmac_p2p_noa_absent_end_event::mac_res_get_mac_vap fail.vap_id = %u}",pst_event->st_event_hdr.uc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_P2P, "{dmac_p2p_noa_absent_end_event::pst_dmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    //�жϼĴ���״̬noa count�Ƿ������0:���� 1: ������; ����P2P IE
    if(((hal_p2p_pm_event_stru *)(pst_event->auc_event_data))->p2p_noa_status == OAL_FALSE)
    {
        OAM_WARNING_LOG0(0, OAM_SF_P2P, "{dmac_p2p_noa_absent_end_event::p2p NoA count expired}");
        OAL_MEMZERO(&(pst_dmac_vap->st_p2p_noa_param), OAL_SIZEOF(mac_cfg_p2p_noa_param_stru));
    }

    //OAL_IO_PRINT("dmac_p2p_handle_ps_cl:resume\r\n");
    /* �ָ����� */
    if (WLAN_VAP_MODE_BSS_STA == pst_dmac_vap->st_vap_base_info.en_vap_mode)
    {
    #ifdef _PRE_WLAN_FEATURE_STA_PM
        dmac_pm_sta_post_event(pst_dmac_vap, STA_PWR_EVENT_P2P_AWAKE, 0, OAL_PTR_NULL);
    #endif
    }
    else
    {
        dmac_p2p_handle_ps(pst_dmac_vap, OAL_FALSE);
    }

    return OAL_SUCC;
}

oal_uint32 dmac_p2p_oppps_ctwindow_end_event(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru             *pst_event;
    mac_device_stru            *pst_mac_device;
    mac_vap_stru               *pst_mac_vap = OAL_PTR_NULL;
    dmac_vap_stru              *pst_dmac_vap;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(0, OAM_SF_P2P, "{dmac_p2p_ctwindow_end_event::pst_event_mem null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event = frw_get_event_stru(pst_event_mem);

    pst_mac_device = mac_res_get_dev(pst_event->st_event_hdr.uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(0, OAM_SF_P2P, "{dmac_p2p_ctwindow_end_event::pst_mac_device is null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_mac_vap = mac_res_get_mac_vap(pst_event->st_event_hdr.uc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
    {
        OAM_ERROR_LOG1(0, OAM_SF_P2P, "{dmac_p2p_ctwindow_end_event::mac_res_get_mac_vap fail.vap_id = %u}",pst_event->st_event_hdr.uc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_P2P, "{dmac_p2p_ctwindow_end_event::pst_dmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (WLAN_VAP_MODE_BSS_STA == pst_dmac_vap->st_vap_base_info.en_vap_mode)
    {
        /* ��¼��ͣ���ŵ���tbtt�жϺ��л� */
        pst_dmac_vap->pst_hal_device->st_hal_scan_params.st_home_channel = pst_mac_vap->st_channel;
    #ifdef _PRE_WLAN_FEATURE_STA_PM
        dmac_pm_sta_post_event(pst_dmac_vap, STA_PWR_EVENT_P2P_SLEEP, 0, OAL_PTR_NULL);
    #endif
    }
    else
    {
        if(OAL_FALSE == pst_dmac_vap->st_p2p_ops_param.en_pause_ops)
        {
            /* ��¼��ͣ���ŵ���tbtt�жϺ��л� */
            pst_dmac_vap->pst_hal_device->st_hal_scan_params.st_home_channel = pst_mac_vap->st_channel;
#ifdef _PRE_WLAN_FEATURE_STA_PM
            /* ��ͣ���� */
            dmac_p2p_handle_ps(pst_dmac_vap, OAL_TRUE);
#endif

        }
    }
    return OAL_SUCC;
}

oal_void dmac_p2p_oppps_ctwindow_start_event(dmac_vap_stru * pst_dmac_vap)
{
#ifdef _PRE_WLAN_FEATURE_STA_PM
    oal_uint32 ui_retval = 0;
#endif

    if (WLAN_VAP_MODE_BSS_STA == pst_dmac_vap->st_vap_base_info.en_vap_mode)
    {
#ifdef _PRE_WLAN_FEATURE_STA_PM
        ui_retval = dmac_pm_sta_post_event(pst_dmac_vap, STA_PWR_EVENT_P2P_AWAKE, 0, OAL_PTR_NULL);
        if(OAL_SUCC != ui_retval)
        {
            OAM_ERROR_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR, "{dmac_p2p_oppps_ctwindow_start_event::[%d]}", ui_retval);
        }
#endif
    }
    else
    {
        /* �ָ����� */
        dmac_p2p_handle_ps(pst_dmac_vap, OAL_FALSE);
    }
}


oal_void dmac_p2p_reset_ps_status_for_dbac(
                                mac_device_stru  *pst_device,
                                mac_vap_stru     *pst_led_vap,
                                mac_vap_stru     *pst_flw_vap)
{
    dmac_vap_stru *pst_dmac_p2p_vap;
    mac_vap_stru  *pst_p2p_vap;

#ifdef _PRE_WLAN_FEATURE_STA_PM
    mac_cfg_ps_open_stru  st_ps_open;

    st_ps_open.uc_pm_enable      = MAC_STA_PM_SWITCH_OFF;
    st_ps_open.uc_pm_ctrl_type   = MAC_STA_PM_CTRL_TYPE_DBAC;

	/* ����dbacǰ�رյ͹��� */
    if (WLAN_VAP_MODE_BSS_STA == pst_led_vap->en_vap_mode)
    {
        dmac_config_set_sta_pm_on(pst_led_vap, OAL_SIZEOF(mac_cfg_ps_open_stru), (oal_uint8 *)&st_ps_open);
    }

    if(WLAN_VAP_MODE_BSS_STA == pst_flw_vap->en_vap_mode)
    {
        dmac_config_set_sta_pm_on(pst_flw_vap, OAL_SIZEOF(mac_cfg_ps_open_stru), (oal_uint8 *)&st_ps_open);
    }
#endif

    /* ��dbacǰp2p�Ѿ�pause��tid��,��Ҫ�ָ� */
    if (OAL_TRUE == pst_device->st_p2p_info.en_p2p_ps_pause)
    {
        if (WLAN_LEGACY_VAP_MODE != pst_led_vap->en_p2p_mode)
        {
            pst_p2p_vap = pst_led_vap;
        }
        else
        {
            pst_p2p_vap = pst_flw_vap;
        }

        pst_dmac_p2p_vap = mac_res_get_dmac_vap(pst_p2p_vap->uc_vap_id);
        dmac_p2p_handle_ps(pst_dmac_p2p_vap, OAL_FALSE);
    }
}
#ifdef _PRE_WLAN_FEATURE_STA_PM

oal_void dmac_p2p_resume_send_null_to_ap(dmac_vap_stru *pst_dmac_vap,mac_sta_pm_handler_stru *pst_mac_sta_pm_handle)
{
    oal_uint8       uc_power_mgmt = 0xff;

    /* ����Ҫ�ش� */
    if ((0 == pst_mac_sta_pm_handle->en_ps_back_active_pause) && (0 == pst_mac_sta_pm_handle->en_ps_back_doze_pause))
    {
        return;
    }

    /* ����״̬��״̬������Ӧnull֡ */
    if (STA_PWR_SAVE_STATE_ACTIVE == GET_PM_STATE(pst_mac_sta_pm_handle))
    {
        if (OAL_TRUE == pst_mac_sta_pm_handle->en_ps_back_doze_pause)
        {
            uc_power_mgmt = 1;
        }
    }
    else
    {
        if (OAL_TRUE == pst_mac_sta_pm_handle->en_ps_back_active_pause)
        {
            if (STA_PWR_SAVE_STATE_DOZE == GET_PM_STATE(pst_mac_sta_pm_handle))
            {
                dmac_pm_sta_post_event(pst_dmac_vap, STA_PWR_EVENT_TX_DATA, 0, OAL_PTR_NULL);
            }
            uc_power_mgmt = 0;
        }
    }

    if ((0 == uc_power_mgmt) || (1 == uc_power_mgmt))
    {
        if (OAL_SUCC != dmac_psm_process_fast_ps_state_change(pst_dmac_vap, uc_power_mgmt))
        {
            OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR,"dmac_p2p_resume_send_null_to_ap::send[%d]null fail",uc_power_mgmt);

            /* ˯�ߵ�null֡����ʧ����Ҫ������ʱ���ȴ���ʱ����,���ѵ�null֡����ʧ��,�ȴ��´η��ͻ���beacon�ٸ�֪���� */
            if (uc_power_mgmt)
            {
                dmac_psm_start_activity_timer(pst_dmac_vap,pst_mac_sta_pm_handle);
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



