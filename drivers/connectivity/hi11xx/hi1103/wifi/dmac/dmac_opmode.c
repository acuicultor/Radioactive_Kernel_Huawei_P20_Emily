


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_OPMODE_NOTIFY
/*****************************************************************************
  1 ͷ�ļ�����
*****************************************************************************/
#include "dmac_opmode.h"
#include "dmac_vap.h"
#include "oal_types.h"
#include "oam_ext_if.h"
#include "frw_ext_if.h"
#include "hal_ext_if.h"
#include "dmac_tx_bss_comm.h"
#include "dmac_config.h"
#include "dmac_psm_ap.h"
#ifdef _PRE_WLAN_FEATURE_M2S
#include "dmac_m2s.h"
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_OPMODE_C

/*****************************************************************************
  2 ȫ�ֱ�������
*****************************************************************************/


/*****************************************************************************
  3 ����ʵ��
*****************************************************************************/

oal_uint32 dmac_ie_proc_opmode_notify(mac_user_stru *pst_mac_user, mac_vap_stru *pst_mac_vap, mac_opmode_notify_stru *pst_opmode_notify)
{
    oal_uint32              ul_relt;
    wlan_bw_cap_enum_uint8  en_bwcap_user = 0;                      /* user֮ǰ�Ĵ�����Ϣ */
    wlan_nss_enum_uint8     en_avail_bf_num_spatial_stream;         /* �û�֧�ֵ�Beamforming�ռ������� */
    wlan_nss_enum_uint8     en_avail_num_spatial_stream;            /* Tx��Rx֧��Nss�Ľ���,���㷨���� */

    en_avail_bf_num_spatial_stream = pst_mac_user->en_avail_bf_num_spatial_stream;
    en_avail_num_spatial_stream = pst_mac_user->en_avail_num_spatial_stream;

    en_bwcap_user = pst_mac_user->en_avail_bandwidth;

    ul_relt = mac_ie_proc_opmode_field(pst_mac_vap, pst_mac_user, pst_opmode_notify);
    if (OAL_UNLIKELY(OAL_SUCC != ul_relt))
    {
        OAM_WARNING_LOG1(pst_mac_user->uc_vap_id, OAM_SF_OPMODE, "{dmac_ie_proc_opmode_notify::mac_ie_proc_opmode_field failed[%d].}", ul_relt);
        return ul_relt;
    }

    /*���ռ����������ͱ仯��������㷨���Ӻ���,�������Ϳռ���ͬʱ�ı䣬Ҫ�ȵ��ÿռ������㷨����*/
    if ((pst_mac_user->en_avail_bf_num_spatial_stream != en_avail_bf_num_spatial_stream) ||
          (pst_mac_user->en_avail_num_spatial_stream != en_avail_num_spatial_stream))
    {
        dmac_alg_cfg_user_spatial_stream_notify(pst_mac_user);
    }

    /* opmode����ı�֪ͨ�㷨,��ͬ��������Ϣ��HOST */
    if (pst_mac_user->en_avail_bandwidth != en_bwcap_user)
    {
        /* �����㷨�ı����֪ͨ�� */
        dmac_alg_cfg_user_bandwidth_notify(pst_mac_vap, pst_mac_user);
    }

    return OAL_SUCC;
}


oal_uint32 dmac_check_opmode_notify(
                mac_vap_stru                   *pst_mac_vap,
                oal_uint8                       *puc_payload,
                oal_uint32                       ul_msg_len,
                mac_user_stru                   *pst_mac_user)
{
    mac_opmode_notify_stru *pst_opmode_notify = OAL_PTR_NULL;
    oal_uint8              *puc_opmode_notify_ie;
    wlan_bw_cap_enum_uint8  en_bwcap_user = 0;                      /* user֮ǰ�Ĵ�����Ϣ */
    oal_uint32              ul_change = MAC_NO_CHANGE;
    oal_uint32              ul_ret = 0;

    en_bwcap_user = pst_mac_user->en_avail_bandwidth;

    if ((OAL_FALSE == mac_mib_get_VHTOptionImplemented(pst_mac_vap))
        || (OAL_FALSE == mac_mib_get_OperatingModeNotificationImplemented(pst_mac_vap)))
    {
        return ul_change;
    }

    puc_opmode_notify_ie = mac_find_ie(MAC_EID_OPMODE_NOTIFY, puc_payload, (oal_int32)(ul_msg_len));
    if (OAL_PTR_NULL == puc_opmode_notify_ie)
    {
        return ul_change;
    }

    if (puc_opmode_notify_ie[1] < MAC_OPMODE_NOTIFY_LEN)
    {
        OAM_WARNING_LOG1(0, OAM_SF_OPMODE, "{dmac_check_opmode_notify::invalid opmode notify ie len[%d]!}", puc_opmode_notify_ie[1]);
        return ul_change;
    }

    pst_opmode_notify = (mac_opmode_notify_stru *)(puc_opmode_notify_ie + MAC_IE_HDR_LEN);

    /* SMPS�Ѿ����������¿ռ�����������ռ���������SMPS��OPMODE�Ŀռ�����Ϣ��ͬ */
    if(pst_mac_user->en_avail_num_spatial_stream > pst_opmode_notify->bit_rx_nss ||
        (pst_mac_user->en_avail_num_spatial_stream == WLAN_SINGLE_NSS && pst_opmode_notify->bit_rx_nss != WLAN_SINGLE_NSS))
    {
        OAM_WARNING_LOG0(0, OAM_SF_OPMODE, "{dmac_check_opmode_notify::SMPS and OPMODE show different nss!}");
        if(WLAN_HT_MODE == pst_mac_user->en_cur_protocol_mode || WLAN_HT_ONLY_MODE == pst_mac_user->en_cur_protocol_mode)
        {
            return ul_change;
        }
    }

    ul_ret = mac_ie_proc_opmode_field(pst_mac_vap, pst_mac_user, pst_opmode_notify);
    if(OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_user->uc_vap_id,OAM_SF_OPMODE, "{dmac_check_opmode_notify:: mac_ie_proc_opmode_field failed [%d].}", ul_ret);
    }
    if(pst_mac_user->en_avail_bandwidth != en_bwcap_user)
    {
        ul_change = MAC_BW_OPMODE_CHANGE;
    }
    return ul_change;

}


oal_uint32  dmac_mgmt_send_opmode_notify_action(mac_vap_stru *pst_mac_vap, wlan_nss_enum_uint8 en_nss, oal_bool_enum_uint8 en_bool)
{
    oal_netbuf_stru        *pst_mgmt_buf;
    oal_uint16              us_mgmt_len;
    mac_tx_ctl_stru        *pst_tx_ctl;
    oal_uint32              ul_ret;
    dmac_vap_stru          *pst_dmac_vap;

    pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_OPMODE, "{dmac_mgmt_send_opmode_notify_action::pst_dmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* �������֡�ڴ� */
    pst_mgmt_buf = OAL_MEM_NETBUF_ALLOC(OAL_MGMT_NETBUF, WLAN_MGMT_NETBUF_SIZE, OAL_NETBUF_PRIORITY_HIGH);
    if (OAL_PTR_NULL == pst_mgmt_buf)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_OPMODE, "{dmac_mgmt_send_opmode_notify_action::pst_mgmt_buf null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_set_netbuf_prev(pst_mgmt_buf, OAL_PTR_NULL);
    oal_set_netbuf_next(pst_mgmt_buf,OAL_PTR_NULL);
    OAL_MEM_NETBUF_TRACE(pst_mgmt_buf, OAL_TRUE);

    /* ��װ operating mode notify ֡ */
    dmac_mgmt_encap_opmode_notify_action(pst_mac_vap, pst_mgmt_buf, &us_mgmt_len, en_bool, en_nss);
    if (0 == us_mgmt_len)
    {
        oal_netbuf_free(pst_mgmt_buf);
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_OPMODE, "{dmac_mgmt_send_opmode_notify_action::encap opmode notify action failed.}");
        return OAL_FAIL;
    }

    /* ��дnetbuf��cb�ֶΣ������͹���֡�ͷ�����ɽӿ�ʹ�� */
    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_mgmt_buf);

    OAL_MEMZERO(pst_tx_ctl, sizeof(mac_tx_ctl_stru));
    MAC_GET_CB_TX_USER_IDX(pst_tx_ctl)      = pst_mac_vap->us_assoc_vap_id;
    MAC_GET_CB_WME_AC_TYPE(pst_tx_ctl)      = WLAN_WME_AC_MGMT;
    MAC_GET_CB_FRAME_TYPE(pst_tx_ctl)       = WLAN_CB_FRAME_TYPE_ACTION;
    MAC_GET_CB_FRAME_SUBTYPE(pst_tx_ctl)    = WLAN_ACTION_OPMODE_FRAME_SUBTYPE;

    /* ���÷��͹���֡�ӿ� */
    ul_ret = dmac_tx_mgmt(pst_dmac_vap, pst_mgmt_buf, us_mgmt_len);
    if (OAL_SUCC != ul_ret)
    {
        oal_netbuf_free(pst_mgmt_buf);
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_OPMODE, "{dmac_mgmt_send_opmode_notify_action::tx opmode notify action failed.}");
        return ul_ret;
    }

    return OAL_SUCC;
}


oal_uint32  dmac_mgmt_rx_opmode_notify_frame(dmac_vap_stru *pst_dmac_vap, dmac_user_stru *pst_dmac_user, oal_netbuf_stru *pst_netbuf)
{
    mac_opmode_notify_stru     *pst_opmode_notify = OAL_PTR_NULL;
    mac_vap_stru               *pst_mac_vap;
    mac_user_stru              *pst_mac_user;
    dmac_rx_ctl_stru           *pst_rx_ctl;
    mac_ieee80211_frame_stru   *pst_frame_hdr;
    oal_uint8                  *puc_data;
    oal_uint8                   uc_power_save;
#ifdef _PRE_WLAN_FEATURE_M2S
    wlan_bw_cap_enum_uint8      en_bwcap_user = WLAN_BW_CAP_20M;        /* user֮ǰ�Ĵ�����Ϣ */
    wlan_nss_enum_uint8         en_avail_bf_num_spatial_stream;         /* �û�֧�ֵ�Beamforming�ռ������� */
    wlan_nss_enum_uint8         en_avail_num_spatial_stream;            /* Tx��Rx֧��Nss�Ľ���,���㷨���� */
    oal_bool_enum_uint8         en_nss_change = OAL_FALSE;
    oal_bool_enum_uint8         en_bw_change  = OAL_FALSE;
    oal_uint32                  ul_relt;
#endif

    if ((OAL_PTR_NULL == pst_dmac_vap) || (OAL_PTR_NULL == pst_dmac_user) || (OAL_PTR_NULL == pst_netbuf))
    {
        OAM_ERROR_LOG3(0, OAM_SF_OPMODE, "{dmac_mgmt_rx_opmode_notify_frame::pst_dmac_vap = [%p], pst_dmac_user = [%p],pst_netbuf = [%p]!}\r\n",
                        pst_dmac_vap, pst_dmac_user, pst_netbuf);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_vap  = &(pst_dmac_vap->st_vap_base_info);
    pst_mac_user = &(pst_dmac_user->st_user_base_info);

    /* vap�ǵ��ռ���ʱ�������ռ��� */
    if(IS_VAP_SINGLE_NSS(pst_mac_vap) || IS_USER_SINGLE_NSS(pst_mac_user))
    {
        return OAL_SUCC;
    }

    if ((OAL_FALSE == mac_mib_get_VHTOptionImplemented(pst_mac_vap))
        || (OAL_FALSE == mac_mib_get_OperatingModeNotificationImplemented(pst_mac_vap)))
    {
        OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_OPMODE, "{dmac_mgmt_rx_opmode_notify_frame::the vap is not support OperatingModeNotification!}");
        return OAL_SUCC;
    }

    pst_rx_ctl    = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);

    pst_frame_hdr = (mac_ieee80211_frame_stru *)mac_get_rx_cb_mac_hdr(&(pst_rx_ctl->st_rx_info));
    /* ��ȡ֡��ָ�� */
    puc_data = MAC_GET_RX_PAYLOAD_ADDR(&(pst_rx_ctl->st_rx_info), pst_netbuf);

    /* �Ƿ���Ҫ����Power Management bitλ */
    uc_power_save = (oal_uint8)pst_frame_hdr->st_frame_control.bit_power_mgmt;

    /* �������λ����(bit_power_mgmt == 1),���¼���DMAC�������û�������Ϣ */
    if ((OAL_TRUE == uc_power_save) && (WLAN_VAP_MODE_BSS_AP == pst_mac_vap->en_vap_mode))
    {
        if (OAL_FALSE == pst_dmac_user->bit_ps_mode)
        {
            OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_OPMODE, "{dmac_mgmt_rx_opmode_notify_frame::user changes ps mode to powersave!}\r\n");
            dmac_psm_doze(pst_dmac_vap, pst_dmac_user);
        }
    }

    /****************************************************/
    /*   OperatingModeNotification Frame - Frame Body   */
    /* -------------------------------------------------*/
    /* |   Category   |   Action   |   OperatingMode   |*/
    /* -------------------------------------------------*/
    /* |   1          |   1        |   1               |*/
    /* -------------------------------------------------*/
    /*                                                  */
    /****************************************************/

    /* ��ȡpst_opmode_notify��ָ�� */
    pst_opmode_notify = (mac_opmode_notify_stru *)(puc_data + MAC_ACTION_OFFSET_ACTION + 1);

    OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_OPMODE, "{dmac_mgmt_rx_opmode_notify_frame::user[%d] nss type[%d],rx nss[%d],bandwidch[%d]!}",
        pst_mac_user->us_assoc_id, pst_opmode_notify->bit_rx_nss_type,
        pst_opmode_notify->bit_rx_nss, pst_opmode_notify->bit_channel_width);

#ifdef _PRE_WLAN_FEATURE_M2S
    en_avail_bf_num_spatial_stream = pst_mac_user->en_avail_bf_num_spatial_stream;
    en_avail_num_spatial_stream    = pst_mac_user->en_avail_num_spatial_stream;
    en_bwcap_user                  = pst_mac_user->en_avail_bandwidth;

    ul_relt = mac_ie_proc_opmode_field(pst_mac_vap, pst_mac_user, pst_opmode_notify);
    if (OAL_UNLIKELY(OAL_SUCC != ul_relt))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_OPMODE, "{dmac_mgmt_rx_opmode_notify_frame::mac_ie_proc_opmode_field failed[%d].}", ul_relt);
        return ul_relt;
    }

    /*1.���ռ����������ͱ仯��������㷨���Ӻ���,�������Ϳռ���ͬʱ�ı䣬Ҫ�ȵ��ÿռ������㷨����*/
    if ((pst_mac_user->en_avail_bf_num_spatial_stream != en_avail_bf_num_spatial_stream) ||
          (pst_mac_user->en_avail_num_spatial_stream != en_avail_num_spatial_stream))
    {
        en_nss_change = OAL_TRUE;
    }

    /* 2.opmode����ı�֪ͨ�㷨,��ͬ��������Ϣ��HOST */
    if (pst_mac_user->en_avail_bandwidth != en_bwcap_user)
    {
        en_bw_change = OAL_TRUE;
    }

    if(OAL_TRUE == en_nss_change || OAL_TRUE == en_bw_change)
    {
        /* ���ݴ������nss�仯��ˢ��Ӳ�����еİ�����֪ͨ�㷨��ͬ����host�� */
        dmac_m2s_nss_and_bw_alg_notify(pst_mac_vap, pst_mac_user, en_nss_change, en_bw_change);
    }
#else

    dmac_ie_proc_opmode_notify(pst_mac_user, pst_mac_vap, pst_opmode_notify);

    /* user����ͬ����hmac */
    if (OAL_SUCC != dmac_config_d2h_user_m2s_info_syn(pst_mac_vap, pst_mac_user))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_OPMODE,
                  "{dmac_mgmt_rx_opmode_notify_frame::dmac_config_d2h_user_m2s_info_syn failed.}");
    }
#endif

    return OAL_SUCC;
}
#endif


#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif
