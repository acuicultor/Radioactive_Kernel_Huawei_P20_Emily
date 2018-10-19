


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 ͷ�ļ�����
*****************************************************************************/
#include "oal_ext_if.h"
#include "wlan_spec.h"
#include "wlan_types.h"
#include "wlan_mib.h"
#include "mac_user.h"
#include "mac_resource.h"
#include "mac_frame.h"
#include "dmac_main.h"
#include "dmac_vap.h"
#include "dmac_ext_if.h"
#include "dmac_11i.h"
#include "dmac_11w.h"
#include "hal_ext_if.h"
#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_11W_ROM_C


#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)

/*****************************************************************************
  2 ȫ�ֱ�������
*****************************************************************************/
dmac_11w_rom_cb g_st_dmac_11w_rom_cb = {OAL_PTR_NULL};


/*****************************************************************************
  3 ����ʵ��
*****************************************************************************/



oal_bool_enum_uint8  dmac_11w_robust_action(oal_netbuf_stru *pst_mgmt_buf)
{
    oal_uint8   uc_frame_type;
    oal_uint8  *puc_mac_header  = oal_netbuf_header(pst_mgmt_buf);
    oal_uint8  *puc_mac_payload = mac_netbuf_get_payload(pst_mgmt_buf);

    uc_frame_type = mac_frame_get_subtype_value(puc_mac_header);
    if (WLAN_ACTION != uc_frame_type)
    {
        return OAL_FALSE;
    }

    switch (puc_mac_payload[MAC_ACTION_OFFSET_CATEGORY])
    {
        case MAC_ACTION_CATEGORY_SPECMGMT:
        case MAC_ACTION_CATEGORY_QOS:
        case MAC_ACTION_CATEGORY_DLS:
        case MAC_ACTION_CATEGORY_BA:
        case MAC_ACTION_CATEGORY_RADIO_MEASURMENT:
        case MAC_ACTION_CATEGORY_FAST_BSS_TRANSITION:
        case MAC_ACTION_CATEGORY_HT:
        case MAC_ACTION_CATEGORY_SA_QUERY:
        case MAC_ACTION_CATEGORY_PROTECTED_DUAL_OF_ACTION:
        case MAC_ACTION_CATEGORY_WNM:
        case MAC_ACTION_CATEGORY_MESH:
        case MAC_ACTION_CATEGORY_MULTIHOP:
        case MAC_ACTION_CATEGORY_VENDOR_SPECIFIC_PROTECTED:
        {
            return OAL_TRUE;
        }

        default:
            break;
    }

    if (g_st_dmac_11w_rom_cb.p_dmac_11w_robust_action)
    {
        return g_st_dmac_11w_rom_cb.p_dmac_11w_robust_action(pst_mgmt_buf);
    }

    return OAL_FALSE;
}


oal_bool_enum_uint8  dmac_11w_robust_non_action(oal_uint8 uc_frame_type, oal_uint8 *puc_payload)
{
    mac_reason_code_enum_uint16 en_reason_code = 0;
    oal_uint16                  us_reason_code = 0;
    us_reason_code = puc_payload[1];
    us_reason_code = (oal_uint16)((us_reason_code << 8) + puc_payload[0]);
    en_reason_code = (mac_reason_code_enum_uint16)us_reason_code;
    switch (uc_frame_type)
    {
        case WLAN_DISASOC:
        case WLAN_DEAUTH:
        {
            //OAM_INFO_LOG1(0, OAM_SF_PMF, "dmac_11w_robust_non_action:: deauth/disassoc reason code:[%d]", en_reason_code);
            /* ��������������ü��� */
            if (MAC_NOT_AUTHED == en_reason_code || MAC_NOT_ASSOCED == en_reason_code)
            {
                return OAL_FALSE;
            }
            return OAL_TRUE;
        }

        default:
            break;
    }

    return OAL_FALSE;
}


oal_bool_enum_uint8  dmac_11w_robust_frame(oal_netbuf_stru *pst_mgmt_frame, oal_uint8 *puc_mac_header)
{
    oal_uint8   uc_frame_type;
    oal_uint8  *puc_payload;

    puc_payload    = mac_netbuf_get_payload(pst_mgmt_frame);

    /*�ǹ���֡*/
    uc_frame_type  = mac_frame_get_type_value(puc_mac_header);
    if (WLAN_MANAGEMENT != uc_frame_type)
    {
        return OAL_FALSE;
    }

    uc_frame_type = mac_frame_get_subtype_value(puc_mac_header);
    if ((OAL_TRUE == dmac_11w_robust_action(pst_mgmt_frame))||
        (OAL_TRUE == dmac_11w_robust_non_action(uc_frame_type, puc_payload)))
    {
        return OAL_TRUE;
    }

    return OAL_FALSE;
}


oal_void dmac_11w_get_pmf_cap(mac_vap_stru *pst_mac_vap, wlan_pmf_cap_status_uint8 *pen_pmf_cap)
{
    oal_bool_enum_uint8 en_MFPC;
    oal_bool_enum_uint8 en_MFPR;

    *pen_pmf_cap = MAC_PMF_DISABLED;
    en_MFPC = mac_mib_get_dot11RSNAMFPC(pst_mac_vap);
    en_MFPR = mac_mib_get_dot11RSNAMFPR(pst_mac_vap);

    /* �ж�vap��PMF���� */
    if ((OAL_FALSE == en_MFPC) && (OAL_TRUE == en_MFPR))
    {
        *pen_pmf_cap = MAC_PMF_DISABLED;
    }
    else if ((OAL_FALSE == en_MFPC) && (OAL_FALSE == en_MFPR))
    {
        *pen_pmf_cap = MAC_PMF_DISABLED;
    }
    else if ((OAL_TRUE == en_MFPC) && (OAL_FALSE == en_MFPR))
    {
        *pen_pmf_cap = MAC_PMF_ENABLED;
    }
    else
    {
        *pen_pmf_cap = MAC_PMF_REQUIRED;
    }

    return ;
}


oal_bool_enum_uint8 dmac_11w_check_multicast_mgmt(oal_netbuf_stru *pst_mgmt_buf)
{
    oal_uint8                 *puc_da;
    mac_ieee80211_frame_stru  *pst_frame_hdr;
    pst_frame_hdr = (mac_ieee80211_frame_stru *)OAL_NETBUF_HEADER(pst_mgmt_buf);
    puc_da        = pst_frame_hdr->auc_address1;

    if (OAL_TRUE != ETHER_IS_MULTICAST(puc_da))
    {
       return OAL_FALSE;
    }

    return dmac_11w_robust_frame(pst_mgmt_buf, (oal_uint8*)pst_frame_hdr);
}


oal_bool_enum_uint8 dmac_11w_check_vap_pmf_cap(dmac_vap_stru *pst_dmac_vap, wlan_pmf_cap_status_uint8  en_pmf_cap, mac_user_stru *pst_multi_user)
{
    /* �߱�pmf����֡���ܵ�ǰ������:
       1) mib���:RSN Active mib
       2) ����֧�� :user ֧��pmf
       3) igtk���� :���ڹ㲥Robust mgmt����
     */
    if ((OAL_TRUE == mac_mib_get_rsnaactivated(&pst_dmac_vap->st_vap_base_info)) &&
        ( MAC_PMF_DISABLED != en_pmf_cap) &&
        (OAL_SUCC == dmac_check_igtk_exist(pst_multi_user->st_key_info.uc_igtk_key_index)))
    {
        return OAL_TRUE;
    }

    return OAL_FALSE;
}





oal_void dmac_bip_crypto(dmac_vap_stru *pst_dmac_vap,
                                 oal_netbuf_stru *pst_netbuf_mgmt,
                                 wlan_security_txop_params_stru  *pst_security,
                                 hal_tx_mpdu_stru *pst_mpdu)
{
#if(_PRE_WLAN_FEATURE_PMF == _PRE_PMF_HW_CCMP_SW_BIP)
    wlan_priv_key_param_stru    *pst_pmf_igtk;
#endif
    mac_user_stru               *pst_multi_user;
    oal_uint8                    uc_pmf_igtk_keyid;
    wlan_pmf_cap_status_uint8    en_pmf_cap;

    pst_multi_user = mac_res_get_mac_user(pst_dmac_vap->st_vap_base_info.us_multi_user_idx);
    if (OAL_PTR_NULL == pst_multi_user)
    {
        return ;
    }

    /* �ж�vap��pmf���� */
    dmac_11w_get_pmf_cap(&pst_dmac_vap->st_vap_base_info,&en_pmf_cap);

    /* �ж��Ƿ���Ҫ���ܵ��鲥/�㲥 ǿ������֡ */
    if ((OAL_TRUE == dmac_11w_check_multicast_mgmt(pst_netbuf_mgmt)) &&
        (OAL_TRUE == dmac_11w_check_vap_pmf_cap(pst_dmac_vap,en_pmf_cap, pst_multi_user)))
    {
        uc_pmf_igtk_keyid = pst_multi_user->st_key_info.uc_igtk_key_index;
        oal_netbuf_set_len(pst_netbuf_mgmt, pst_mpdu->us_mpdu_len);

#if(_PRE_WLAN_FEATURE_PMF == _PRE_PMF_HW_CCMP_BIP)
        pst_security->en_cipher_protocol_type = WLAN_80211_CIPHER_SUITE_BIP;
        pst_security->uc_cipher_key_id        = uc_pmf_igtk_keyid;
#else
        pst_pmf_igtk      = &pst_multi_user->st_key_info.ast_key[uc_pmf_igtk_keyid];
        pst_security->en_cipher_protocol_type = WLAN_80211_CIPHER_SUITE_NO_ENCRYP;
        /* 11w�鲥����֡���� */
        oal_crypto_bip_enmic(uc_pmf_igtk_keyid,
                                       pst_pmf_igtk->auc_key,
                                       pst_pmf_igtk->auc_seq,
                                       pst_netbuf_mgmt,
                                       &pst_mpdu->us_mpdu_len);
#endif

     }

}

/* ROM���� */
 bip_crypto  bip_crypto_cb = dmac_bip_crypto;


oal_void dmac_11w_set_ucast_mgmt_frame(dmac_vap_stru  *pst_dmac_vap,
                                             mac_tx_ctl_stru *pst_tx_ctl,
                                             hal_tx_txop_feature_stru *pst_txop_feature,
                                             oal_netbuf_stru *pst_netbuf,
                                             mac_ieee80211_frame_stru *puc_frame_hdr)
{
    oal_bool_enum_uint8 en_MFPC       = OAL_FALSE;
    oal_bool_enum_uint8 en_pmf_active = OAL_FALSE;
    dmac_user_stru     *pst_dmac_user = OAL_PTR_NULL;
    oal_uint8           auc_mac_addr[WLAN_MAC_ADDR_LEN];
    oal_uint16          us_user_idx;

#if 0
    /* �û����:auth֡����²������û����û�index�Ƿ������������ */
    if (MAC_INVALID_USER_ID == MAC_GET_CB_TX_USER_IDX(pst_tx_ctl))
    {
        return;
    }

    pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user(MAC_GET_CB_TX_USER_IDX(pst_tx_ctl));
    if (OAL_PTR_NULL == pst_dmac_user)
    {
        return;
    }
#else
    mac_get_address1((oal_uint8*)puc_frame_hdr, auc_mac_addr);
    if (OAL_SUCC != mac_vap_find_user_by_macaddr(&pst_dmac_vap->st_vap_base_info, auc_mac_addr, &us_user_idx))
    {
        return;
    }
    pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user(us_user_idx);
    if (OAL_PTR_NULL == pst_dmac_user)
    {
        return;
    }
#endif

    /* ������� */
    en_MFPC = mac_mib_get_dot11RSNAMFPC(&(pst_dmac_vap->st_vap_base_info));
    en_pmf_active = pst_dmac_user->st_user_base_info.st_cap_info.bit_pmf_active;
    if ((OAL_FALSE == en_MFPC) || (OAL_FALSE == en_pmf_active))
    {
        return;
    }

    if (OAL_TRUE == dmac_11w_robust_frame(pst_netbuf, (oal_uint8*)puc_frame_hdr))
    {
        mac_set_protectedframe((oal_uint8 *)puc_frame_hdr);
        pst_txop_feature->st_security.en_cipher_key_type      = pst_dmac_user->st_user_base_info.st_user_tx_info.st_security.en_cipher_key_type;
        pst_txop_feature->st_security.en_cipher_protocol_type = pst_dmac_user->st_user_base_info.st_user_tx_info.st_security.en_cipher_protocol_type;

        return;
    }

}



oal_void dmac_11w_set_mcast_mgmt_frame(dmac_vap_stru  *pst_dmac_vap,
                                             hal_tx_txop_feature_stru *pst_txop_feature,
                                             oal_netbuf_stru *pst_netbuf, hal_tx_mpdu_stru *pst_mpdu)
 {
    mac_ieee80211_frame_stru   *puc_frame_hdr;

    puc_frame_hdr = (mac_ieee80211_frame_stru *)oal_netbuf_header(pst_netbuf);

    if (0 == mac_is_protectedframe((oal_uint8 *)puc_frame_hdr))
    {
        /* ǿ���鲥����֡���� */
        bip_crypto_cb(pst_dmac_vap, pst_netbuf, &(pst_txop_feature->st_security), pst_mpdu);
    }
 }


#if(_PRE_WLAN_FEATURE_PMF == _PRE_PMF_HW_CCMP_SW_BIP)


oal_uint32 dmac_bip_decrypto(dmac_vap_stru  *pst_dmac_vap, oal_netbuf_stru *pst_netbuf)
{
   wlan_priv_key_param_stru          *pst_pmf_igtk;
   mac_user_stru                     *pst_multi_user;
   oal_uint8                          uc_pmf_igtk_keyid = 0xff;
   oal_uint32                         ul_relt;
   wlan_pmf_cap_status_uint8          en_pmf_cap;

   if (OAL_TRUE != dmac_11w_check_multicast_mgmt(pst_netbuf))
   {
       return OAL_SUCC;
   }

   /* �ж�vap��pmf���� */
   dmac_11w_get_pmf_cap(&pst_dmac_vap->st_vap_base_info,&en_pmf_cap);

   pst_multi_user = mac_res_get_mac_user(pst_dmac_vap->st_vap_base_info.us_multi_user_idx);
   if (OAL_PTR_NULL == pst_multi_user)
   {
       return OAL_PTR_NULL;
   }

   /* ǿ���鲥����֡���� */
   if ((WLAN_VAP_MODE_BSS_STA == pst_dmac_vap->st_vap_base_info.en_vap_mode) &&
       (OAL_TRUE == dmac_11w_check_vap_pmf_cap(pst_dmac_vap,en_pmf_cap, pst_multi_user)))
    {
        /* ��ȡigtk��Ϣ */
        uc_pmf_igtk_keyid = pst_multi_user->st_key_info.uc_igtk_key_index;
        pst_pmf_igtk      = &(pst_multi_user->st_key_info.ast_key[uc_pmf_igtk_keyid]);
        /* ����֡���� */
        ul_relt = oal_crypto_bip_demic(uc_pmf_igtk_keyid,
                                       pst_pmf_igtk->auc_key,
                                       pst_pmf_igtk->auc_seq,
                                       pst_netbuf);
        if(OAL_SUCC != ul_relt)
        {
            OAM_ERROR_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PMF, "{dmac_bip_decrypto::oal_crypto_bip_demic failed[%d].}", ul_relt);
            return OAL_ERR_CODE_PMF_BIP_DECRIPTO_FAIL;
        }
    }

   return OAL_SUCC;
}
#endif /* (_PRE_WLAN_FEATURE_PMF == _PRE_PMF_HW_CCMP_SW_BIP) */


oal_void dmac_11w_update_users_status(dmac_vap_stru  *pst_dmac_vap, mac_user_stru *pst_mac_user, oal_bool_enum_uint8 en_user_pmf)
{
#if(_PRE_WLAN_FEATURE_PMF == _PRE_PMF_HW_CCMP_BIP)

    wlan_pmf_cap_status_uint8  en_pmf_cap;
    oal_uint32                 ul_user_pmf_old = 0;
    oal_uint32                 ul_flag = BIT0;
#endif

    mac_user_set_pmf_active(pst_mac_user, en_user_pmf);

#if(_PRE_WLAN_FEATURE_PMF == _PRE_PMF_HW_CCMP_BIP)

    ul_user_pmf_old = pst_dmac_vap->ul_user_pmf_status;
    if (OAL_FALSE == pst_mac_user->st_cap_info.bit_pmf_active)
    {
        pst_dmac_vap->ul_user_pmf_status &= (oal_uint32)(~(ul_flag << (pst_mac_user->us_assoc_id)));
    }
    else
    {
        pst_dmac_vap->ul_user_pmf_status |= ul_flag << (pst_mac_user->us_assoc_id);
    }

    /* ���û�иı� */
    if (ul_user_pmf_old == pst_dmac_vap->ul_user_pmf_status)
    {
        return;
    }

    /* Ӳ��PMF���ƿ�����д */
    dmac_11w_get_pmf_cap(&pst_dmac_vap->st_vap_base_info, &en_pmf_cap);
    if (MAC_PMF_DISABLED != en_pmf_cap)
    {
        hal_set_pmf_crypto(pst_dmac_vap->pst_hal_vap, (oal_bool_enum_uint8)IS_OPEN_PMF_REG(pst_dmac_vap));
    }
#endif
}


oal_uint32 dmac_11w_rx_filter(dmac_vap_stru *pst_dmac_vap, oal_netbuf_stru  *pst_netbuf)
{
    oal_uint32                           ul_relt = OAL_SUCC;
    oal_uint8                           *puc_da;
    dmac_rx_ctl_stru                    *pst_rx_ctl;
    mac_ieee80211_frame_stru            *pst_frame_hdr;
    mac_user_stru                       *pst_user;
    oal_uint16                           us_ta_user_idx;
    wlan_ciper_protocol_type_enum_uint8  en_cipher_protocol_type;

    pst_frame_hdr  = (mac_ieee80211_frame_stru *)OAL_NETBUF_HEADER(pst_netbuf);
    puc_da         = pst_frame_hdr->auc_address1;
    pst_rx_ctl     = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
    us_ta_user_idx = MAC_GET_RX_CB_TA_USER_IDX(&(pst_rx_ctl->st_rx_info));
    en_cipher_protocol_type = hal_ctype_to_cipher_suite(pst_rx_ctl->st_rx_status.bit_cipher_protocol_type);


    pst_user = (mac_user_stru *)mac_res_get_dmac_user(us_ta_user_idx);
    if (OAL_PTR_NULL == pst_user)
    {
        return OAL_SUCC;
    }

    if ((OAL_TRUE != pst_user->st_cap_info.bit_pmf_active) ||
        (OAL_TRUE != dmac_11w_robust_frame(pst_netbuf, (oal_uint8*)pst_frame_hdr)))
    {
       return OAL_SUCC;
    }

    /* �㲥Robust֡���� */
    if (OAL_TRUE == ETHER_IS_MULTICAST(puc_da))
    {
        ul_relt = OAL_SUCC;

#if(_PRE_WLAN_FEATURE_PMF == _PRE_PMF_HW_CCMP_BIP)

        if (WLAN_80211_CIPHER_SUITE_BIP !=  en_cipher_protocol_type)
        {
            OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PMF,
                "{dmac_11w_rx_filter::PMF is open,but is Robust muti frame chipertype is[%d].}", en_cipher_protocol_type);
            return OAL_ERR_CODE_PMF_NO_PROTECTED_ERROR;
        }
#else
        /* 11w�鲥����֡���� */
        ul_relt = dmac_bip_decrypto(pst_dmac_vap, pst_netbuf);
        if (OAL_SUCC != ul_relt)
        {
            /* �鲥����ʧ�ܣ����ϱ�����֡ */
            OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PMF,
                           "{dmac_11w_rx_filter::dmac_bip_decrypto failed[%d].}", ul_relt);
        }
#endif
        return ul_relt;
    }

    /* pmfʹ�ܣ���Ӳ�����ܹ��˵�δ����֡���й��� */
    if (WLAN_80211_CIPHER_SUITE_NO_ENCRYP ==  en_cipher_protocol_type)
    {
        OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PMF,
                  "{dmac_11w_rx_filter::PMF is open,but the Robust frame is CIPHER_SUITE_NO_ENCRYP.}");
        return OAL_ERR_CODE_PMF_NO_PROTECTED_ERROR;
    }

    /* PMF��������֡У�� */
    if ((OAL_FALSE == pst_frame_hdr->st_frame_control.bit_protected_frame)||
        (WLAN_80211_CIPHER_SUITE_CCMP !=  en_cipher_protocol_type))
    {
        OAM_WARNING_LOG2(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PMF,
                      "{dmac_11w_rx_filter::robust_action protecter incorect. bit_protected_frame[%d], cipher_type[%d].}",
                      pst_frame_hdr->st_frame_control.bit_protected_frame,
                      en_cipher_protocol_type);
        return OAL_ERR_CODE_PMF_NO_PROTECTED_ERROR;
    }

    return OAL_SUCC;
}

#endif /* (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT) */










#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

