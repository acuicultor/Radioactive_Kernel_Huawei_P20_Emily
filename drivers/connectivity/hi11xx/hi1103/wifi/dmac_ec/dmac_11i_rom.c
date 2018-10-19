


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 ͷ�ļ�����
*****************************************************************************/
#include "oal_ext_if.h"
#include "oal_list.h"
#include "oal_net.h"
#include "frw_ext_if.h"
#include "hal_ext_if.h"
#include "mac_resource.h"
#include "wlan_types.h"
#include "mac_vap.h"
#include "mac_user.h"
//#include "mac_11i.h"
#include "dmac_main.h"
#include "oal_net.h"
#include "dmac_tx_bss_comm.h"
#include "dmac_mgmt_ap.h"
#include "dmac_11i.h"
#include "dmac_wep.h"

#ifdef _PRE_WLAN_FEATURE_SOFT_CRYPTO
#include "dmac_crypto_wep.h"
#include "dmac_crypto_tkip.h"
#include "dmac_crypto_aes_ccm.h"
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_11I_ROM_C

/*****************************************************************************
  2 ȫ�ֱ�������
*****************************************************************************/


/*****************************************************************************
  3 ����ʵ��
*****************************************************************************/

oal_uint32 dmac_check_igtk_exist(oal_uint8 uc_igtk_index)
{
    /* igtk��key index Ϊ4��5 */
    if ((WLAN_MAX_IGTK_KEY_INDEX < uc_igtk_index) ||
        ((WLAN_MAX_IGTK_KEY_INDEX - WLAN_NUM_IGTK) >= uc_igtk_index))
    {
        return OAL_ERR_CODE_PMF_IGTK_NOT_EXIST;
    }

    return OAL_SUCC;
}


oal_uint32  dmac_11i_update_key_to_ce(mac_vap_stru *pst_mac_vap, hal_security_key_stru *pst_key, oal_uint8 *puc_addr)
{
    hal_to_dmac_device_stru         *pst_hal_device = OAL_PTR_NULL;
    hal_security_key_stru            st_key;
    oal_uint8                        auc_key[WLAN_CIPHER_KEY_LEN]={0};
    oal_uint8                        auc_zero[WLAN_CIPHER_KEY_LEN]={0};
    oal_uint8                        uc_key_id;

    /*2.1 ��ȡdmac_vap*/
    if ((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == pst_key))
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hal_device = DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap);


#ifdef _PRE_WLAN_FEATURE_RX_AGGR_EXTEND
    /* wave�������Թرմ�log */
    if(pst_hal_device && OAL_FALSE == pst_hal_device->en_test_is_on_waveapp_flag)
#endif
    {
        /* ��Ҫ��Ϣ��ӡwarning */
        OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_WPA, "{dmac_11i_update_key_to_ce::keyid=%u, keytype=%u,lutidx=%u,cipher=%u}",
                         pst_key->uc_key_id, pst_key->en_key_type, pst_key->uc_lut_idx, pst_key->en_cipher_type);
        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_WPA, "{dmac_11i_update_key_to_ce::en_update_key=%u, en_key_origin=%u}",
                         pst_key->en_update_key, pst_key->en_key_origin);
    }


    /* 3.1 дӲ���Ĵ���   */
    hal_ce_add_key(pst_hal_device, pst_key, puc_addr);

    if ((WLAN_80211_CIPHER_SUITE_CCMP == pst_key->en_cipher_type)
        && (WLAN_KEY_TYPE_PTK == pst_key->en_key_type))
    {
        oal_memcopy(&st_key, pst_key, OAL_SIZEOF(hal_security_key_stru));
        st_key.puc_cipher_key = auc_key;

        for (uc_key_id = 0; uc_key_id < WLAN_NUM_TK; uc_key_id++)
        {
            if (uc_key_id != pst_key->uc_key_id)
            {
                st_key.uc_key_id = uc_key_id;
                OAL_MEMZERO(st_key.puc_cipher_key, WLAN_CIPHER_KEY_LEN);
                hal_ce_get_key(pst_hal_device, &st_key);

                if (0 == oal_memcmp(st_key.puc_cipher_key, auc_zero, WLAN_CIPHER_KEY_LEN))
                {
                    /*4. ��������key id����Կ*/
                    oal_memcopy(st_key.puc_cipher_key, pst_key->puc_cipher_key, WLAN_CIPHER_KEY_LEN);

                    hal_ce_add_key(pst_hal_device, &st_key, puc_addr);
                }
            }
        }
    }

#ifdef _PRE_WLAN_INIT_PTK_TX_PN
    /* 3.2 ��ʼ��TX PN */
    hal_init_ptk_tx_pn(pst_hal_device, pst_key,PN_MSB_INIT_VALUE);
    dmac_init_iv_word_lut(pst_hal_device, pst_key,PN_MSB_INIT_VALUE);
#endif
    return OAL_SUCC;
}


oal_uint32  dmac_11i_del_key_to_ce(mac_vap_stru                   *pst_mac_vap,
                                            oal_uint8                       uc_key_id,
                                            hal_cipher_key_type_enum_uint8  en_key_type,
                                            oal_uint8                       uc_lut_index,
                                            wlan_ciper_protocol_type_enum_uint8 en_cipher_type)
{
    hal_security_key_stru    st_security_key = {0};
    hal_to_dmac_device_stru  *pst_hal_device = OAL_PTR_NULL;
    hal_security_key_stru    st_key = {0};
    oal_uint8                auc_key[WLAN_CIPHER_KEY_LEN]={0};
    oal_uint8                auc_crt_key[WLAN_CIPHER_KEY_LEN]={0};
    oal_uint8                uc_other_key_id;

    if (OAL_PTR_NULL == pst_mac_vap)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    st_security_key.uc_key_id      = uc_key_id;
    st_security_key.en_key_type    = en_key_type;
    st_security_key.uc_lut_idx     = uc_lut_index;
    st_security_key.en_update_key  = OAL_TRUE;
    st_security_key.en_cipher_type = en_cipher_type;
    st_security_key.puc_cipher_key = OAL_PTR_NULL;
    st_security_key.puc_mic_key    = OAL_PTR_NULL;

    pst_hal_device = DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap);


#ifdef _PRE_WLAN_FEATURE_RX_AGGR_EXTEND
    /* wave�������Թرմ�log */
    if(pst_hal_device && OAL_FALSE == pst_hal_device->en_test_is_on_waveapp_flag)
#endif
    {
        /* ��Ҫ��Ϣ��ӡwarning */
        OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_WPA,
                     "{dmac_11i_del_key_to_ce::keyid=%u, keytype=%u,lutidx=%u}", uc_key_id, en_key_type, uc_lut_index);
    }


    if ((WLAN_80211_CIPHER_SUITE_CCMP == en_cipher_type)
        && (WLAN_KEY_TYPE_PTK == en_key_type))
    {
        oal_memcopy(&st_key, &st_security_key, OAL_SIZEOF(hal_security_key_stru));
        st_key.puc_cipher_key = auc_key;
        hal_ce_get_key(pst_hal_device, &st_key);
        oal_memcopy(auc_crt_key, st_key.puc_cipher_key, WLAN_CIPHER_KEY_LEN);

        for (uc_other_key_id = 0; uc_other_key_id < WLAN_NUM_TK; uc_other_key_id++)
        {
            if (uc_other_key_id != uc_key_id)
            {
                st_key.uc_key_id = uc_other_key_id;
                OAL_MEMZERO(st_key.puc_cipher_key, WLAN_CIPHER_KEY_LEN);
                hal_ce_get_key(pst_hal_device, &st_key);

                if (0 == oal_memcmp(st_key.puc_cipher_key, auc_crt_key, WLAN_CIPHER_KEY_LEN))
                {
                    /*4. ɾ������key id����Կ*/
                    hal_ce_del_key(pst_hal_device, &st_key);
                }
            }

        }
    }

    /* ɾ����key idӲ���Ĵ�����Կ */
    hal_ce_del_key(pst_hal_device, &st_security_key);

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV)
    /*�����鲥��ԿӦ��ɾ��2��*/
    if ((HAL_KEY_TYPE_RX_GTK == en_key_type) || (HAL_KEY_TYPE_RX_GTK2 == en_key_type))
    {
        st_security_key.en_key_type = (HAL_KEY_TYPE_RX_GTK == en_key_type) ? HAL_KEY_TYPE_RX_GTK2 : HAL_KEY_TYPE_RX_GTK;
        OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_WPA,
                         "{dmac_11i_del_key_to_ce::keyid=%u, keytype=%u,lutidx=%u}", uc_key_id, en_key_type, uc_lut_index);
        hal_ce_del_key(pst_hal_device, &st_security_key);
    }
#endif
    return OAL_SUCC;
}

hal_key_origin_enum_uint8 dmac_11i_get_auth_type(mac_vap_stru *pst_mac_vap)
{
    if (IS_AP(pst_mac_vap))
    {
        return HAL_AUTH_KEY;
    }
    return HAL_SUPP_KEY;
}

hal_cipher_key_type_enum_uint8 dmac_11i_get_gtk_key_type(mac_vap_stru *pst_mac_vap, wlan_ciper_protocol_type_enum_uint8 en_cipher_type)
{
    oal_uint8           uc_rx_gtk        = HAL_KEY_TYPE_RX_GTK;
#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
    mac_user_stru      *pst_multi_user   = OAL_PTR_NULL;
#endif

    if (IS_AP(pst_mac_vap) && (WLAN_80211_CIPHER_SUITE_BIP != en_cipher_type))
    {
#if (defined(_PRE_PRODUCT_ID_HI110X_DEV))
        /* 1102 �鲥����֡��Կ��key type���͸ı� */
        return  HAL_KEY_TYPE_RX_GTK;
#else
        return HAL_KEY_TYPE_TX_GTK;
#endif
    }

    /* 03��51һ�£�����gtk1��gtk2��gtk��ptkһ�����ã�����keyid���� */
#if (defined(_PRE_PRODUCT_ID_HI110X_DEV))
    pst_multi_user = (mac_user_stru *)mac_res_get_mac_user(pst_mac_vap->us_multi_user_idx);
    if (OAL_PTR_NULL == pst_multi_user)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WPA, "{dmac_11i_get_gtk_key_type::pst_multi_user[%d] null.}", pst_mac_vap->us_multi_user_idx);
        return HAL_KEY_TYPE_TX_GTK;
    }

    if (WLAN_80211_CIPHER_SUITE_BIP == en_cipher_type)
    {
        /* Hi1102: HAL_KEY_TYPE_IGTK */
        uc_rx_gtk = HAL_KEY_TYPE_TX_GTK;
    }
    else if (0 == pst_multi_user->st_key_info.bit_gtk)
    {
        uc_rx_gtk = HAL_KEY_TYPE_RX_GTK;
        //pst_mac_vap->st_key_mgmt.bit_gtk = 1;
    }
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV)
    else
    {
        uc_rx_gtk = HAL_KEY_TYPE_RX_GTK2;
        //pst_mac_vap->st_key_mgmt.bit_gtk = 0;
    }
#endif
#endif
    return uc_rx_gtk;
}


oal_uint32 dmac_reset_gtk_token(mac_vap_stru *pst_mac_vap)
{
    mac_user_stru  *pst_multi_user          = OAL_PTR_NULL;

    pst_multi_user = (mac_user_stru *)mac_res_get_mac_user(pst_mac_vap->us_multi_user_idx);
    if (OAL_PTR_NULL == pst_multi_user)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WPA, "{dmac_11i_add_gtk_key::pst_multi_user[%d] null.}", pst_mac_vap->us_multi_user_idx);
        return OAL_ERR_CODE_SECURITY_USER_INVAILD;
    }
    pst_multi_user->st_key_info.bit_gtk = 0;

    return OAL_SUCC;
}


oal_uint32  dmac_11i_del_peer_macaddr(mac_vap_stru *pst_mac_vap, oal_uint8 uc_lut_index)
{
    hal_to_dmac_device_stru  *pst_hal_device = OAL_PTR_NULL;

    if (OAL_PTR_NULL == pst_mac_vap)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hal_device = DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap);

    /* дӲ���Ĵ���   */
    hal_ce_del_peer_macaddr(pst_hal_device, uc_lut_index);
    return OAL_SUCC;
}


 oal_uint32  dmac_11i_add_ptk_key(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_mac_addr, oal_uint8 uc_key_index)
{
    oal_uint32                          ul_ret;
    dmac_user_stru                     *pst_current_dmac_user                  = OAL_PTR_NULL;
    mac_user_stru                      *pst_current_mac_user                   = OAL_PTR_NULL;
    wlan_priv_key_param_stru           *pst_key                                = OAL_PTR_NULL;
    oal_uint8                           auc_mic_key[WLAN_TEMPORAL_KEY_LENGTH]  = {0};
    hal_security_key_stru               st_security_key;

    /*1.0 ptk index ��� */
    if(uc_key_index >= WLAN_NUM_TK)
    {
        return OAL_ERR_CODE_SECURITY_KEY_ID;
    }
    if (OAL_PTR_NULL == puc_mac_addr)
    {
        return OAL_ERR_CODE_SECURITY_USER_INVAILD;
    }

    /*1.1 ����mac��ַ�ҵ�user����*/
    pst_current_dmac_user = mac_vap_get_dmac_user_by_addr(pst_mac_vap, puc_mac_addr);
    if (OAL_PTR_NULL == pst_current_dmac_user)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_WPA,
                       "{dmac_11i_add_ptk_key::pst_current_dmac_user null.}");

        return OAL_ERR_CODE_SECURITY_USER_INVAILD;
    }
    pst_current_mac_user = &pst_current_dmac_user->st_user_base_info;

    /*2.1 ����׼��*/
    pst_key = &pst_current_mac_user->st_key_info.ast_key[uc_key_index];
    if ((WLAN_80211_CIPHER_SUITE_TKIP !=(oal_uint8)pst_key->ul_cipher)
        && (WLAN_80211_CIPHER_SUITE_CCMP != (oal_uint8)pst_key->ul_cipher)
         && (WLAN_80211_CIPHER_SUITE_GCMP != (oal_uint8)pst_key->ul_cipher)
        && (WLAN_80211_CIPHER_SUITE_GCMP_256 != (oal_uint8)pst_key->ul_cipher)
        && (WLAN_80211_CIPHER_SUITE_CCMP_256 != (oal_uint8)pst_key->ul_cipher))
    {
        OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_WPA,
                       "{dmac_11i_add_ptk_key::invalid chiper type[%d], uc_key_index=%d.}", uc_key_index, pst_key->ul_cipher);

        return OAL_ERR_CODE_SECURITY_CHIPER_TYPE;
    }

    st_security_key.uc_key_id      = uc_key_index;
    st_security_key.en_key_type    = HAL_KEY_TYPE_PTK;
    st_security_key.en_key_origin  = dmac_11i_get_auth_type(pst_mac_vap);
    st_security_key.en_update_key  = OAL_TRUE;
    st_security_key.uc_lut_idx     = pst_current_dmac_user->uc_lut_index;
    st_security_key.en_cipher_type = pst_current_mac_user->st_key_info.en_cipher_type;
    st_security_key.puc_cipher_key = pst_key->auc_key;
    st_security_key.puc_mic_key    = OAL_PTR_NULL;



    if (WLAN_TEMPORAL_KEY_LENGTH < pst_key->ul_key_len)
    {
        st_security_key.puc_mic_key = auc_mic_key;
        /* ����TKIPģʽ��MIC����txrx����bit˳����������Ҫת��˳��*/
        if ((WLAN_80211_CIPHER_SUITE_TKIP == st_security_key.en_cipher_type) && (IS_STA(pst_mac_vap)))
        {
            oal_memcopy(auc_mic_key, pst_key->auc_key + WLAN_TEMPORAL_KEY_LENGTH + WLAN_MIC_KEY_LENGTH, WLAN_MIC_KEY_LENGTH);
            oal_memcopy(auc_mic_key + WLAN_MIC_KEY_LENGTH, pst_key->auc_key + WLAN_TEMPORAL_KEY_LENGTH, WLAN_MIC_KEY_LENGTH);
        }
        else
        {
            oal_memcopy(auc_mic_key, pst_key->auc_key + WLAN_TEMPORAL_KEY_LENGTH, WLAN_MIC_KEY_LENGTH);
            oal_memcopy(auc_mic_key + WLAN_MIC_KEY_LENGTH, pst_key->auc_key + WLAN_TEMPORAL_KEY_LENGTH + WLAN_MIC_KEY_LENGTH, WLAN_MIC_KEY_LENGTH);
        }
    }

    /*3.1 �û����MIB��Ϣ����*/
    //us_user_idx = pst_current_mac_user->us_assoc_id;
    //mibset_RSNAStatsSTAAddress(puc_mac_addr,pst_mac_vap, us_user_idx);
    //mibset_RSNAStatsSelectedPairwiseCipher(pst_current_mac_user->st_key_info.en_cipher_type, pst_mac_vap, us_user_idx);
    //mac_mib_set_RSNAPairwiseCipherSelected(pst_mac_vap, pst_current_mac_user->st_key_info.en_cipher_type);

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    if (mac_vap_is_vsta(pst_mac_vap))
    {
        puc_mac_addr = mac_mib_get_StationID(pst_mac_vap);
    }
#endif
    /* 4.1 �����ܷ�ʽ�ͼ�����Կд��CE��, ͬʱ���Ӽ����û� */
    ul_ret = dmac_11i_update_key_to_ce(pst_mac_vap, &st_security_key, puc_mac_addr);

    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WPA,
                       "{dmac_11i_update_key_to_ce::dmac_11i_update_key_to_ce failed[%d].}", ul_ret);

        return ul_ret;
    }


    /* �Ĵ���д��ɹ��󣬸��µ����û���Կ */
    mac_user_set_key(pst_current_mac_user, st_security_key.en_key_type, st_security_key.en_cipher_type, st_security_key.uc_key_id);

    /*5.1 ��1X�˿���֤״̬*/
    mac_user_set_port(pst_current_mac_user, OAL_TRUE);

    /*6.1 �򿪷����������ļ�������*/
    pst_current_mac_user->st_user_tx_info.st_security.en_cipher_key_type = HAL_KEY_TYPE_PTK;

    return OAL_SUCC;
}



oal_uint32  dmac_11i_add_gtk_key(mac_vap_stru *pst_mac_vap, oal_uint8 uc_key_index)
{
    oal_uint32                          ul_ret;
    dmac_vap_stru                      *pst_dmac_vap            = OAL_PTR_NULL;
    mac_user_stru                      *pst_multi_user          = OAL_PTR_NULL;
    wlan_priv_key_param_stru           *pst_key                 = OAL_PTR_NULL;
    oal_uint8                           auc_mic_key[8]          = {0};
    hal_security_key_stru               st_security_key;

    if (OAL_PTR_NULL == pst_mac_vap)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    /*1.1 ���������ҵ��鲥user�ڴ�����*/
    pst_multi_user = (mac_user_stru *)mac_res_get_mac_user(pst_mac_vap->us_multi_user_idx);
    if (OAL_PTR_NULL == pst_multi_user)
    {
        return OAL_ERR_CODE_SECURITY_USER_INVAILD;
    }

    /*1.2 ����mac_vap��ȡdmac_vap*/
    pst_dmac_vap = MAC_GET_DMAC_VAP(pst_mac_vap);

    /*2.1 ����׼��*/
    pst_key = &pst_multi_user->st_key_info.ast_key[uc_key_index];

#if(_PRE_WLAN_FEATURE_PMF == _PRE_PMF_HW_CCMP_SW_BIP)

    if (WLAN_80211_CIPHER_SUITE_BIP == (oal_uint8)pst_key->ul_cipher)
    {
        /* BIP�������ɣ�����Ҫ���ø�mac*/
        return OAL_SUCC;
    }
#endif
    if (WLAN_80211_CIPHER_SUITE_TKIP != (oal_uint8)pst_key->ul_cipher
        && WLAN_80211_CIPHER_SUITE_CCMP != (oal_uint8)pst_key->ul_cipher
        && WLAN_80211_CIPHER_SUITE_BIP != (oal_uint8)pst_key->ul_cipher
        && (WLAN_80211_CIPHER_SUITE_GCMP != (oal_uint8)pst_key->ul_cipher)
        && (WLAN_80211_CIPHER_SUITE_GCMP_256 != (oal_uint8)pst_key->ul_cipher)
        && (WLAN_80211_CIPHER_SUITE_CCMP_256 != (oal_uint8)pst_key->ul_cipher)
        && (WLAN_80211_CIPHER_SUITE_BIP_CMAC_256 != (oal_uint8)pst_key->ul_cipher))
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WPA,
                       "{dmac_11i_add_gtk_key::invalid chiper type[%d].}", pst_key->ul_cipher);

        return OAL_ERR_CODE_SECURITY_CHIPER_TYPE;
    }

    /* ���AP�����鲥��Կʹ�õ�����ͬ��keyid�������鲥��Կ�۵�keyidһ�£��鲥����ʧ��
       keyidһ�£�����ԭ����Կ������������Կ */
    if (uc_key_index == pst_multi_user->st_key_info.uc_last_gtk_key_idx)
    {
        pst_multi_user->st_key_info.bit_gtk ^= BIT0; /* GTK ��λƹ��ʹ�� */
    }

    st_security_key.en_cipher_type = (oal_uint8)pst_key->ul_cipher;
    st_security_key.uc_key_id      = uc_key_index;
    st_security_key.en_key_type    = dmac_11i_get_gtk_key_type(pst_mac_vap, st_security_key.en_cipher_type);
    pst_multi_user->st_key_info.bit_gtk ^= BIT0; /* GTK ��λƹ��ʹ�� */
    pst_multi_user->st_key_info.uc_last_gtk_key_idx = uc_key_index;

    OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_WPA, "{dmac_11i_add_gtk_key::new bit_gtk=%u, keyidx = %u.}",
                     pst_multi_user->st_key_info.bit_gtk, pst_multi_user->st_key_info.uc_last_gtk_key_idx);

    st_security_key.en_key_origin  = dmac_11i_get_auth_type(pst_mac_vap);
    st_security_key.en_update_key  = OAL_TRUE;
    /* ��ȡ��Ӧ�鲥��Կ��Ӧ��rx lut idx����ͬ��Ʒ��ȡ��ʽ��ͬ��ͳһ��װ������ȡ */
    hal_vap_get_gtk_rx_lut_idx(pst_dmac_vap->pst_hal_vap, &st_security_key.uc_lut_idx);

    st_security_key.puc_cipher_key = pst_key->auc_key;
    st_security_key.puc_mic_key    = OAL_PTR_NULL;
    if (WLAN_TEMPORAL_KEY_LENGTH < pst_key->ul_key_len)
    {
        st_security_key.puc_mic_key = pst_key->auc_key + WLAN_TEMPORAL_KEY_LENGTH;
        /* ����TKIPģʽ��MIC����txrx����bit˳����������Ҫת��˳��*/
        if ((WLAN_80211_CIPHER_SUITE_TKIP == st_security_key.en_cipher_type) && (IS_STA(pst_mac_vap)))
        {
            oal_memcopy(auc_mic_key, st_security_key.puc_mic_key, 8);
            oal_memcopy(st_security_key.puc_mic_key, st_security_key.puc_mic_key + 8, 8);
            oal_memcopy(st_security_key.puc_mic_key + 8, auc_mic_key, 8);
        }
    }

    /*3.1 �����ܷ�ʽ�ͼ�����Կд��CE��*/
    ul_ret = dmac_11i_update_key_to_ce(pst_mac_vap, &st_security_key, OAL_PTR_NULL);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WPA,
                       "{dmac_11i_add_gtk_key::dmac_11i_update_key_to_ce failed[%d].}", ul_ret);

        return ul_ret;
    }

    /* ���óɹ��󣬸���һ��multiuser�еİ�ȫ��Ϣ,Ŀǰֻ��keyid�ڷ����鲥֡ʱ��ʹ�õ� */
    mac_user_set_key(pst_multi_user, st_security_key.en_key_type, st_security_key.en_cipher_type, st_security_key.uc_key_id);

    /*4.1 ��1X�˿���֤״̬*/
    mac_user_set_port(pst_multi_user, OAL_TRUE);

    /*5.1 �򿪷����������ļ�������*/
    if (WLAN_KEY_TYPE_TX_GTK == st_security_key.en_key_type)
    {
        pst_multi_user->st_user_tx_info.st_security.en_cipher_key_type = WLAN_KEY_TYPE_TX_GTK;
    }

    return OAL_SUCC;
}


oal_uint32  dmac_11i_add_wep_key(mac_vap_stru *pst_mac_vap, oal_uint8 uc_key_index)
{
    mac_user_stru                      *pst_multi_user;
    wlan_priv_key_param_stru           *pst_key                   = OAL_PTR_NULL;
    hal_security_key_stru               st_security_key;
    dmac_vap_stru                      *pst_dmac_vap              = OAL_PTR_NULL;

    if (OAL_PTR_NULL == pst_mac_vap)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (OAL_TRUE != mac_is_wep_enabled(pst_mac_vap))
    {
        return OAL_SUCC;
    }


    /*1.1 ���������ҵ��鲥user�ڴ�����*/
    pst_multi_user = mac_res_get_mac_user(pst_mac_vap->us_multi_user_idx);
    if (OAL_PTR_NULL == pst_multi_user)
    {
        return OAL_ERR_CODE_SECURITY_USER_INVAILD;
    }

    /*1.2 ����mac_vap��ȡdmac_vap*/
    pst_dmac_vap = MAC_GET_DMAC_VAP(pst_mac_vap);

    if (WLAN_80211_CIPHER_SUITE_WEP_104 != pst_multi_user->st_key_info.en_cipher_type
        && WLAN_80211_CIPHER_SUITE_WEP_40 != pst_multi_user->st_key_info.en_cipher_type)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WPA,
                       "{dmac_11i_add_wep_key::invalid ul_cipher type[%d].}", pst_multi_user->st_key_info.en_cipher_type);

        return OAL_ERR_CODE_SECURITY_CHIPER_TYPE;
    }
    pst_key = &pst_multi_user->st_key_info.ast_key[uc_key_index];
    /*2.1 ����׼��*///δ����
    st_security_key.uc_key_id      = uc_key_index;
    st_security_key.en_cipher_type = pst_multi_user->st_key_info.en_cipher_type;

    st_security_key.en_key_type    = dmac_11i_get_gtk_key_type(pst_mac_vap, st_security_key.en_cipher_type);
    st_security_key.en_key_origin  = dmac_11i_get_auth_type(pst_mac_vap);
    st_security_key.en_update_key  = OAL_TRUE;

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    if (mac_vap_is_vsta(&pst_dmac_vap->st_vap_base_info))
    {
        if (WLAN_KEY_TYPE_PTK == st_security_key.en_key_type)
        {
            st_security_key.uc_lut_idx = dmac_vap_psta_lut_idx(pst_dmac_vap); /* Proxy STA ��Ҫ��lut idx���� */
        }
        else
        {
            st_security_key.uc_lut_idx = 0; /* Proxy STA ��Ҫ��lut idx���� */
        }
    }
    else if(mac_vap_is_msta(&pst_dmac_vap->st_vap_base_info))
    {
        st_security_key.uc_lut_idx = pst_dmac_vap->pst_hal_vap->uc_vap_id;
        if (WLAN_VAP_MODE_BSS_STA == pst_mac_vap->en_vap_mode && WLAN_KEY_TYPE_RX_GTK == st_security_key.en_key_type)
        {
            /* sta0��rx gtk��lut idxΪ0 */
            st_security_key.uc_lut_idx = 0;
        }
    }
    else
#endif
    {
        /* ע��:1151��Ʒ��WEP��Կ������������Ʒ��ͬ��lut idx���������ܷ�ʽ�ֲ�һ����
           ��ӦSTA lut idx������vap id���������Ǵ�0��ʼ�����������������AP��GTK���жϣ�
           �ҳ��������⣬��ֱ�Ӹ�Ϊvap id */
        if (IS_AP(pst_mac_vap) && (WLAN_KEY_TYPE_TX_GTK == st_security_key.en_key_type))
        {
            hal_vap_get_gtk_rx_lut_idx(pst_dmac_vap->pst_hal_vap, &st_security_key.uc_lut_idx);
        }
        else
        {
            st_security_key.uc_lut_idx = pst_dmac_vap->pst_hal_vap->uc_vap_id;
        }
    }
    st_security_key.puc_cipher_key = pst_key->auc_key;
    st_security_key.puc_mic_key    = OAL_PTR_NULL;

    /*3.1 �����ܷ�ʽ�ͼ�����Կд��CE��*/
    if (OAL_SUCC != dmac_11i_update_key_to_ce(pst_mac_vap, &st_security_key, OAL_PTR_NULL))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_WPA,
                       "{dmac_11i_add_wep_key::dmac_11i_update_key_to_ce invalid.}");

        return OAL_FAIL;
    }

    /*5.1 �򿪷����������ļ�������*/
    if (WLAN_KEY_TYPE_TX_GTK == st_security_key.en_key_type)
    {
        pst_multi_user->st_user_tx_info.st_security.en_cipher_key_type = HAL_KEY_TYPE_TX_GTK;
    }

    return OAL_SUCC;
}


oal_uint32  dmac_11i_del_ptk_key(mac_vap_stru *pst_mac_vap, oal_uint8 uc_key_index, oal_uint8 *puc_mac_addr)
{
    oal_uint32                          ul_ret;
    dmac_user_stru                     *pst_current_dmac_user     = OAL_PTR_NULL;
    oal_uint8                           uc_key_id;
    oal_uint8                           uc_ce_lut_index;
    hal_cipher_key_type_enum_uint8      en_key_type;
    wlan_ciper_protocol_type_enum_uint8 en_cipher_type;

    /*1.1 ����mac��ַ�ҵ�user����*/
    pst_current_dmac_user = mac_vap_get_dmac_user_by_addr(pst_mac_vap, puc_mac_addr);
    if (OAL_PTR_NULL == pst_current_dmac_user)
    {
        OAM_WARNING_LOG0(0, OAM_SF_WPA, "{dmac_11i_del_ptk_key::mac_vap_find_user_by_macaddr failed.}");
        return OAL_ERR_CODE_SECURITY_USER_INVAILD;
    }

    /*2.1 ����׼��*/
    en_key_type     = HAL_KEY_TYPE_PTK;
    uc_key_id       = uc_key_index;
    uc_ce_lut_index = pst_current_dmac_user->uc_lut_index;
    en_cipher_type  = pst_current_dmac_user->st_user_base_info.st_key_info.en_cipher_type;

    /*3.2 ɾ��CE�еĶ�Ӧ��Կ*/
    ul_ret = dmac_11i_del_key_to_ce(pst_mac_vap, uc_key_id, en_key_type, uc_ce_lut_index, en_cipher_type);
    if (OAL_SUCC != ul_ret)
    {//weifugai
        OAM_ERROR_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_WPA,
                      "{dmac_11i_del_ptk_key::dmac_11i_del_key_to_cefailed[%d],uc_key_id=%d en_key_type=%d uc_ce_lut_index=%d.}",
                       ul_ret, uc_key_id, en_key_type, uc_ce_lut_index);

        return ul_ret;
    }

    /*4.1 �ر�1X�˿���֤״̬*/
    mac_user_set_port(&pst_current_dmac_user->st_user_base_info, OAL_FALSE);
    /* ��ʼ���û�����Կ��Ϣ */
    mac_user_init_key(&pst_current_dmac_user->st_user_base_info);

    /*5.1 �رշ����������ļ�������*/
    pst_current_dmac_user->st_user_base_info.st_user_tx_info.st_security.en_cipher_key_type = HAL_KEY_TYPE_BUTT;


    return OAL_SUCC;
}


oal_uint32  dmac_11i_del_gtk_key(mac_vap_stru *pst_mac_vap, oal_uint8 uc_key_index)
{
    oal_uint32                          ul_ret;
    dmac_vap_stru                      *pst_dmac_vap;
    mac_user_stru                      *pst_current_mac_user;
    oal_uint8                           uc_key_id;
    oal_uint8                           uc_ce_lut_index;
    hal_cipher_key_type_enum_uint8      en_key_type;
    wlan_ciper_protocol_type_enum_uint8 en_cipher_type;

    if(OAL_PTR_NULL == pst_mac_vap)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    /*1.1 ���������ҵ��鲥user�ڴ�����*/
    pst_current_mac_user = (mac_user_stru *)mac_res_get_mac_user(pst_mac_vap->us_multi_user_idx);
    if(OAL_PTR_NULL == pst_current_mac_user)
    {
        return OAL_ERR_CODE_SECURITY_USER_INVAILD;
    }

    /*1.2 ����mac_vap��ȡdmac_vap*/
    pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_WPA, "{dmac_11i_del_gtk_key::pst_dmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /*2.1 ����׼��*/
    uc_key_id = 0;
    en_key_type = dmac_11i_get_gtk_key_type(pst_mac_vap, pst_current_mac_user->st_key_info.en_cipher_type);
    /* ��ȡ��Ӧ�鲥��Կ��Ӧ��rx lut idx����ͬ��Ʒ��ȡ��ʽ��ͬ��ͳһ��װ������ȡ */
    hal_vap_get_gtk_rx_lut_idx(pst_dmac_vap->pst_hal_vap, &uc_ce_lut_index);
    en_cipher_type  = pst_current_mac_user->st_key_info.en_cipher_type;

    /*3.1 ɾ��CE�еĶ�Ӧ��Կ*/
    ul_ret = dmac_11i_del_key_to_ce(pst_mac_vap, uc_key_id, en_key_type, uc_ce_lut_index, en_cipher_type);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_WPA,
                      "{dmac_11i_del_gtk_key::dmac_11i_del_key_to_ce[%d],uc_key_id=%d en_key_type=%d uc_ce_lut_index=%d.}",
                       ul_ret, uc_key_id, en_key_type, uc_ce_lut_index);

        return ul_ret;
    }

    /*4.1 �ر�1X�˿���֤״̬*/
    mac_user_set_port(pst_current_mac_user, OAL_FALSE);
    mac_user_init_key(pst_current_mac_user);

    /*5.1 �رշ����������ļ�������*/
    if (HAL_KEY_TYPE_TX_GTK == en_key_type)
    {
        pst_current_mac_user->st_user_tx_info.st_security.en_cipher_key_type = WLAN_KEY_TYPE_BUTT;
    }

    return OAL_SUCC;
}


oal_uint32  dmac_11i_add_key_from_user(mac_vap_stru *pst_mac_vap, dmac_user_stru *pst_dmac_user)
{
    oal_uint8                           uc_key_id;
    oal_uint8                          *puc_cipkey;
    oal_uint8                          *puc_mickey;
    oal_uint8                           en_auth_supp;
    oal_uint32                          ul_ret;
    hal_security_key_stru               st_security_key;

    if (OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == pst_dmac_user)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (WLAN_VAP_MODE_BSS_AP == pst_mac_vap->en_vap_mode)
    {
        en_auth_supp = HAL_AUTH_KEY;
    }
    else
    {
        en_auth_supp = HAL_SUPP_KEY;
    }

#ifndef _PRE_WLAN_FEATURE_USER_EXTEND
    
    /* wep����ʱ����Ҫ�ָ�PTK��Կ */
    if(OAL_TRUE == mac_is_wep_allowed(pst_mac_vap))
    {
        return OAL_SUCC;
    }
#endif

    for(uc_key_id=0; uc_key_id <= pst_dmac_user->uc_max_key_index; uc_key_id++)
    {
        puc_cipkey = pst_dmac_user->st_user_base_info.st_key_info.ast_key[uc_key_id].auc_key;        /* ǰ16�ֽ���cipherkey */
        puc_mickey = pst_dmac_user->st_user_base_info.st_key_info.ast_key[uc_key_id].auc_key + 16;   /* ��16�ֽ���mickey */

        st_security_key.uc_key_id      = uc_key_id;
        st_security_key.en_key_type    = HAL_KEY_TYPE_PTK;
        st_security_key.en_key_origin  = en_auth_supp;
        st_security_key.en_update_key  = OAL_TRUE;
        st_security_key.uc_lut_idx     = pst_dmac_user->uc_lut_index;
        st_security_key.en_cipher_type = pst_dmac_user->st_user_base_info.st_key_info.en_cipher_type;
        st_security_key.puc_cipher_key = puc_cipkey;
        st_security_key.puc_mic_key    = puc_mickey;

        ul_ret = dmac_11i_update_key_to_ce(pst_mac_vap, &st_security_key, pst_dmac_user->st_user_base_info.auc_user_mac_addr);
        if (OAL_SUCC != ul_ret)
        {
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WPA,
                           "{dmac_11i_add_key_from_user::dmac_11i_update_key_to_ce failed[%d].}", ul_ret);

            return ul_ret;
        }
    }

    return OAL_SUCC;
}


oal_uint32  dmac_11i_remove_key_from_user(mac_vap_stru *pst_mac_vap, dmac_user_stru *pst_dmac_user)
{
    oal_uint8   uc_key_id;
    oal_uint32  ul_ret;

    /*ֻ��Ҫɾ��TIKP/CCMP������Կ*/
    switch (pst_dmac_user->st_user_base_info.st_key_info.en_cipher_type)
    {
        case WLAN_80211_CIPHER_SUITE_GROUP_CIPHER:
        case WLAN_80211_CIPHER_SUITE_NO_ENCRYP:
        case WLAN_80211_CIPHER_SUITE_BIP:
        case WLAN_80211_CIPHER_SUITE_GROUP_DENYD:
        {
            //OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_11i_remove_key_from_user::cipher type=%d.}", pst_dmac_user->st_user_base_info.st_key_info.en_cipher_type);
            break;
        }
#ifndef _PRE_WLAN_FEATURE_USER_EXTEND
        case WLAN_80211_CIPHER_SUITE_WEP_40:
        case WLAN_80211_CIPHER_SUITE_WEP_104:
        {
            break;
        }
#else
        
        case WLAN_80211_CIPHER_SUITE_WEP_40:
        case WLAN_80211_CIPHER_SUITE_WEP_104:
#endif
        case WLAN_80211_CIPHER_SUITE_TKIP:
        case WLAN_80211_CIPHER_SUITE_CCMP:
        case WLAN_80211_CIPHER_SUITE_GCMP:
        case WLAN_80211_CIPHER_SUITE_GCMP_256:
        case WLAN_80211_CIPHER_SUITE_CCMP_256:
        {
            for(uc_key_id=0; uc_key_id <= pst_dmac_user->uc_max_key_index; uc_key_id++)
            {
                ul_ret = dmac_11i_del_key_to_ce(pst_mac_vap, uc_key_id, HAL_KEY_TYPE_PTK, pst_dmac_user->uc_lut_index, pst_dmac_user->st_user_base_info.st_key_info.en_cipher_type);
                if (OAL_SUCC != ul_ret)
                {//weifugai
                    OAM_ERROR_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_WPA,
                                   "{dmac_11i_remove_key_from_user::dmac_11i_del_key_to_ce failed[%d], uc_key_id=%d uc_lut_index=%d.}",
                                   ul_ret, uc_key_id, pst_dmac_user->uc_lut_index);
                }
            }
            break;
        }
        default:
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WPA,
                           "{dmac_11i_remove_key_from_user::invalid cipher type[%d].}", pst_dmac_user->st_user_base_info.st_key_info.en_cipher_type);

            break;
    }

    return OAL_SUCC;
}


oal_uint32 dmac_config_11i_init_port(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

    mac_user_stru                               *pst_mac_user;

    MAC_11I_ASSERT(OAL_PTR_NULL != pst_mac_vap, OAL_ERR_CODE_PTR_NULL);

    /* ����mac�ҵ���ӦAP USER�ṹ */
    pst_mac_user = mac_vap_get_user_by_addr(pst_mac_vap, puc_param);
    MAC_11I_ASSERT(OAL_PTR_NULL != pst_mac_user, OAL_ERR_CODE_PTR_NULL);

    mac_vap_init_user_security_port(pst_mac_vap, pst_mac_user);
#endif
    return OAL_SUCC;

}


oal_uint32  dmac_config_11i_add_key_set_reg(mac_vap_stru *pst_mac_vap, oal_uint8 uc_key_index, oal_uint8 *puc_mac_addr)
{
    oal_uint32                            ul_ret;

    /*1.1 ��μ��*/
    if (OAL_PTR_NULL == pst_mac_vap)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    /*2.1 ����ǵ�������Ҫ����PTK*/
    if (OAL_PTR_NULL != puc_mac_addr)
    {
        ul_ret = dmac_11i_add_ptk_key(pst_mac_vap, puc_mac_addr, uc_key_index);
    }
    /*2.2 ������鲥����Ҫ����GTK*/
    else
    {
        ul_ret = dmac_11i_add_gtk_key(pst_mac_vap, uc_key_index);
    }
    if (OAL_SUCC != ul_ret)
    {
        return ul_ret;
    }

    return OAL_SUCC;
}


oal_uint32  dmac_config_11i_add_key(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    oal_uint32                       ul_ret;
    mac_addkey_param_stru           *pst_payload_addkey_params;
    oal_uint8                       *puc_mac_addr;
    oal_uint8                        uc_key_index;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    mac_user_stru                   *pst_mac_user;
    dmac_user_stru                  *pst_dmac_user;
    mac_key_params_stru             *pst_key;
    oal_uint16                       us_user_idx;
#endif
    if ((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_param))
    {
        OAM_ERROR_LOG0(0, OAM_SF_WPA, "{dmac_config_11i_add_key::param null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /*2.1 ��ȡ����*/
    pst_payload_addkey_params = (mac_addkey_param_stru *)puc_param;
    uc_key_index = pst_payload_addkey_params->uc_key_index;
    puc_mac_addr = (oal_uint8*)pst_payload_addkey_params->auc_mac_addr;

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

    pst_key      = &(pst_payload_addkey_params->st_key);

    /*2.2 ����ֵ���ֵ���*/
    if(uc_key_index >= WLAN_NUM_TK + WLAN_NUM_IGTK)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WPA, "{dmac_config_11i_add_key::invalid uc_key_index[%d].}", uc_key_index);
        return OAL_ERR_CODE_SECURITY_KEY_ID;
    }

    if (OAL_TRUE == pst_payload_addkey_params->en_pairwise)
    {
        /* ������Կ����ڵ����û��� */
        ul_ret = mac_vap_find_user_by_macaddr(pst_mac_vap, puc_mac_addr, &us_user_idx);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WPA, "{dmac_config_11i_add_key::find_user_by_macaddr fail[%d].}", ul_ret);
            return ul_ret;
        }
    }
    else
    {
        /* �鲥��Կ������鲥�û��� */
        us_user_idx  = pst_mac_vap->us_multi_user_idx;
    }

    pst_mac_user = (mac_user_stru *)mac_res_get_mac_user(us_user_idx);
    if (OAL_PTR_NULL == pst_mac_user)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WPA, "{dmac_config_11i_add_key::pst_mac_user[%d] null.}", us_user_idx);
        return OAL_ERR_CODE_SECURITY_USER_INVAILD;
    }

    pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user(us_user_idx);
    if (pst_dmac_user == OAL_PTR_NULL)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WPA, "{dmac_config_11i_add_key::get_dmac_user null.user_idx [%d]}", us_user_idx);
        return OAL_ERR_CODE_SECURITY_USER_INVAILD;
    }

    /*3.1 ���������Ը��µ��û���*/
    ul_ret = mac_vap_add_key(pst_mac_vap, pst_mac_user, uc_key_index, pst_key);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WPA, "{hmac_config_11i_add_key::mac_11i_add_key fail[%d].}", ul_ret);
        return ul_ret;
    }

#endif /* #if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) */

    /* WEPģʽ�²���Ҫ��addkey�������üĴ��� */
    if (OAL_TRUE == mac_is_wep_enabled(pst_mac_vap))
    {
        return OAL_SUCC;
    }

    if (OAL_FALSE == pst_payload_addkey_params->en_pairwise)
    {
        puc_mac_addr = OAL_PTR_NULL;
    }
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    else
    {
        pst_dmac_user->bit_ptk_need_install = OAL_TRUE;

        /* For the first time connection, first set key then send 4/4 EAPOL frame;
         * for the second rekey connection, first send 4/4 EAPOL frame then set key. */
        if (pst_dmac_user->bit_is_rx_eapol_key_open == OAL_FALSE
            && pst_dmac_user->bit_eapol_key_4_4_tx_succ == OAL_FALSE)
        {
            pst_dmac_user->bit_ptk_key_idx = uc_key_index;
            OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_WPA,
                            "{dmac_config_11i_add_key::delay to add ptk key.user id %d, key_idx %d}",
                            us_user_idx,
                            uc_key_index);
            return OAL_SUCC;
        }
    }
#endif

    /* ����Ӳ���Ĵ��� */
    ul_ret = dmac_config_11i_add_key_set_reg(pst_mac_vap, uc_key_index, puc_mac_addr);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WPA, "{dmac_config_11i_add_key::set_reg fail[%d].}", ul_ret);
        return ul_ret;
    }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    if (OAL_TRUE == pst_payload_addkey_params->en_pairwise)
    {
        pst_dmac_user->bit_ptk_need_install      = OAL_FALSE;
        pst_dmac_user->bit_eapol_key_4_4_tx_succ = OAL_FALSE;
        pst_dmac_user->bit_ptk_key_idx           = 0;
    }
#endif

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_WAPI

oal_uint32  dmac_config_wapi_add_key(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    hal_to_dmac_device_stru  *pst_hal_device = DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap);

    /* �ص�Ӳ���ļӽ��ܹ��� */
    hal_disable_ce(pst_hal_device);

    return OAL_SUCC;
}

#endif




oal_uint32  dmac_config_11i_remove_key(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    oal_uint32                       ul_ret;
    oal_uint8                        uc_key_index;
    oal_bool_enum_uint8              en_pairwise;
    oal_uint8                       *puc_mac_addr = OAL_PTR_NULL;
    mac_removekey_param_stru        *pst_removekey_params         = OAL_PTR_NULL;

    /*1.1 ��μ��*/
    if ((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_param))
    {
        OAM_ERROR_LOG0(0, OAM_SF_WPA, "{dmac_config_11i_remove_key::param null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    /*2.1 ��ȡ����*/
    pst_removekey_params = (mac_removekey_param_stru *)puc_param;
    uc_key_index = pst_removekey_params->uc_key_index;
    en_pairwise  = pst_removekey_params->en_pairwise;
    puc_mac_addr = (oal_uint8*)pst_removekey_params->auc_mac_addr;

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    if (mac_vap_is_vsta(pst_mac_vap) &&(OAL_FALSE == en_pairwise))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_11i_remove_key::proxysta vsta donot need remove multi GTK.}");
        return OAL_SUCC;
    }
#endif

    /*3.1 ����ǵ���*/
    if((OAL_TRUE != mac_addr_is_zero(puc_mac_addr)) && (OAL_TRUE == en_pairwise))
    {
        ul_ret = dmac_11i_del_ptk_key(pst_mac_vap, uc_key_index, puc_mac_addr);
        if (OAL_SUCC != ul_ret)
        {
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WPA,
                           "{dmac_config_11i_remove_key::dmac_11i_del_ptk_key failed[%d].}", ul_ret);

            return ul_ret;
        }
    }
    /*3.2 ������鲥*/
    else
    {
        ul_ret = dmac_11i_del_gtk_key(pst_mac_vap, uc_key_index);
        if (OAL_SUCC != ul_ret)
        {
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WPA,
                           "{dmac_config_11i_remove_key::dmac_11i_del_gtk_key failed[%d].}", ul_ret);

            return ul_ret;
        }
    }
    return OAL_SUCC;
}


oal_uint32  dmac_config_11i_set_default_key(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_setdefaultkey_param_stru    *pst_defaultkey_params = OAL_PTR_NULL;
    oal_uint32                       ul_ret;
    oal_uint8                        uc_key_index;
    oal_bool_enum_uint8              en_unicast;
    oal_bool_enum_uint8              en_multicast;

    /*1.1 ��μ��*/
    if ((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_param))
    {
        OAM_ERROR_LOG0(0, OAM_SF_WPA, "{dmac_config_11i_set_default_key::param null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /*2.1 ��ȡ����*/
    pst_defaultkey_params = (mac_setdefaultkey_param_stru *)puc_param;
    uc_key_index = pst_defaultkey_params->uc_key_index;

    /*2.2 ����ֵ���ֵ���*/
    if(uc_key_index >= (WLAN_NUM_TK + WLAN_NUM_IGTK))
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WPA,
                      "{dmac_config_11i_set_default_key::invalid uc_key_index[%d].}", uc_key_index);

        return OAL_FAIL;
    }
    en_unicast   = pst_defaultkey_params->en_unicast;
    en_multicast = pst_defaultkey_params->en_multicast;
    /*2.3 ������Ч�Լ��*/
    if ((OAL_FALSE == en_multicast) && (OAL_FALSE == en_unicast))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_WPA, "{dmac_config_11i_set_default_key::invalid mode.}");
        return OAL_FAIL;
    }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

    if (uc_key_index >= WLAN_NUM_TK)
    {
        /*3.1 ����default mgmt key����*/
        ul_ret = mac_vap_set_default_mgmt_key(pst_mac_vap, uc_key_index);
    }
    else
    {
        /*3.2 ���� WEP default key����*/
        ul_ret = mac_vap_set_default_key(pst_mac_vap, uc_key_index);
    }

    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_WPA,
                       "{dmac_config_11i_set_default_key::set key[%d] failed[%d].}", uc_key_index, ul_ret);
        return ul_ret;
    }

#endif

    if (uc_key_index < WLAN_NUM_TK)
    {
        ul_ret = dmac_11i_add_wep_key(pst_mac_vap, uc_key_index);
        if (OAL_SUCC != ul_ret)
        {
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WPA,
                        "{dmac_config_11i_set_default_key::dmac_11i_add_wep_key failed[%d].}", ul_ret);
            return ul_ret;
        }
    }


    return OAL_SUCC;
}


oal_void dmac_11i_tkip_mic_failure_handler(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_user_mac, oal_nl80211_key_type en_key_type)
{
    frw_event_mem_stru           *pst_event_mem;          /* �����¼����ص��ڴ�ָ�� */
    frw_event_stru               *pst_dmac_to_hmac_event; /* ָ�������¼���payloadָ�� */
    dmac_to_hmac_mic_event_stru  *pst_mic_event;

    if ((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_user_mac))
    {
        OAM_ERROR_LOG0(0, OAM_SF_WPA, "{dmac_11i_tkip_mic_failure_handler::param null.}");

        return;
    }
    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(dmac_to_hmac_mic_event_stru));
    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_WPA, "{dmac_11i_tkip_mic_failure_handler::pst_event_mem null.}");
        return;
    }

    /* ����¼�ָ�� */
    pst_dmac_to_hmac_event = frw_get_event_stru(pst_event_mem);

    /* ��д�¼�ͷ */
    FRW_EVENT_HDR_INIT(&(pst_dmac_to_hmac_event->st_event_hdr),
                       FRW_EVENT_TYPE_WLAN_DRX,
                       DMAC_WLAN_DRX_EVENT_SUB_TYPE_TKIP_MIC_FAILE,/* DMAC tkip mic faile �ϱ���HMAC */
                       OAL_SIZEOF(dmac_to_hmac_mic_event_stru),
                       FRW_EVENT_PIPELINE_STAGE_1,
                       pst_mac_vap->uc_chip_id,
                       pst_mac_vap->uc_device_id,
                       pst_mac_vap->uc_vap_id);

    /*��mic��Ϣ�ϱ���hmac*/
    pst_mic_event = (dmac_to_hmac_mic_event_stru *)(pst_dmac_to_hmac_event->auc_event_data);
    oal_memcopy(pst_mic_event->auc_user_mac, puc_user_mac, WLAN_MAC_ADDR_LEN);
    pst_mic_event->en_key_type   = en_key_type;
    pst_mic_event->l_key_id     = 0;/*tkip ֻ֧��1����Կ��д��0*/

    /* �ַ� */
    frw_event_dispatch_event(pst_event_mem);

    /* �ͷ��¼��ڴ� */
    FRW_EVENT_FREE(pst_event_mem);
}


#ifdef _PRE_WLAN_FEATURE_USER_EXTEND

oal_uint32  dmac_11i_add_machw_pn(dmac_user_stru *pst_dmac_user)
{
    hal_pn_lut_cfg_stru         st_pn_lut_cfg;
    hal_to_dmac_device_stru *   pst_hal_device;
    oal_uint8                   uc_tid_loop;

    if (OAL_PTR_NULL == pst_dmac_user)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡhal device */
    pst_hal_device = dmac_user_get_hal_device((mac_user_stru *)pst_dmac_user);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hal_device))
    {
        OAM_ERROR_LOG0(pst_dmac_user->st_user_base_info.uc_vap_id, OAM_SF_USER_EXTEND, "dmac_11i_add_machw_pn::pst_hal_device ptr null.");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ����tx pn */
    {
         st_pn_lut_cfg.uc_pn_key_type = 1;
         st_pn_lut_cfg.uc_pn_peer_idx = pst_dmac_user->uc_lut_index;
         st_pn_lut_cfg.ul_pn_msb      = pst_dmac_user->st_pn.st_ucast_tx_pn.ul_pn_msb;
         st_pn_lut_cfg.ul_pn_lsb      = pst_dmac_user->st_pn.st_ucast_tx_pn.ul_pn_lsb;

         hal_set_tx_pn(pst_hal_device, &st_pn_lut_cfg);
    }

    /* ����rx pn */
    for(uc_tid_loop = 0;uc_tid_loop < WLAN_TID_MAX_NUM;uc_tid_loop++)
    {
         st_pn_lut_cfg.uc_pn_key_type = 1;
         st_pn_lut_cfg.uc_pn_peer_idx = pst_dmac_user->uc_lut_index;
         st_pn_lut_cfg.uc_pn_tid      = uc_tid_loop;
         st_pn_lut_cfg.ul_pn_msb      = pst_dmac_user->st_pn.ast_tid_ucast_rx_pn[uc_tid_loop].ul_pn_msb;
         st_pn_lut_cfg.ul_pn_lsb      = pst_dmac_user->st_pn.ast_tid_ucast_rx_pn[uc_tid_loop].ul_pn_lsb;

         hal_set_rx_pn(pst_hal_device, &st_pn_lut_cfg);
    }

    /* �鲥pn���Բ��滻����ʱ�Ȳ��滻 */

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_SOFT_CRYPTO

oal_uint32  dmac_rx_decrypt_data_frame(oal_netbuf_stru *pst_netbuf, oal_uint16 us_user_idx)
{
    dmac_rx_ctl_stru           *pst_cb_ctrl;
    mac_rx_ctl_stru            *pst_rx_info;
    mac_user_stru              *pst_ta_mac_user;
    oal_uint32                  ul_data_len;
    oal_uint8                  *puc_hdr = (oal_uint8*)OAL_NETBUF_HEADER(pst_netbuf);
    hal_rx_status_enum_uint8    uc_dscr_status = HAL_RX_SUCCESS;
    mac_vap_stru               *pst_mac_vap;
    hal_key_origin_enum_uint8   en_auth_supp;

    pst_cb_ctrl = (dmac_rx_ctl_stru*)oal_netbuf_cb(pst_netbuf);
    pst_rx_info = &(pst_cb_ctrl->st_rx_info);

    OAM_INFO_LOG3(0, OAM_SF_SOFT_CRYPTO, "CRYPTO dmac_rx_decrypt_data_frame entry, hdr_len=%d, frame_len=%d, frame_netbuff_nums=%d.",
        pst_rx_info->uc_mac_header_len,
        pst_rx_info->us_frame_len,
        pst_rx_info->bit_buff_nums);

    /* Ӳ�����������ϱ���֡���ᳬ��һ��1600��������֡�ѱ�Ӳ������ */
    if (pst_rx_info->bit_buff_nums > 1)
    {
        OAM_WARNING_LOG3(0, OAM_SF_SOFT_CRYPTO, "dmac_rx_decrypt_data_frame this frame should have been dropped by mac, hdr_len=%d, frame_len=%d, frame_netbuff_nums=%d.",
            pst_rx_info->uc_mac_header_len,
            pst_rx_info->us_frame_len,
            pst_rx_info->bit_buff_nums);
        return OAL_FAIL;
    }

    pst_ta_mac_user = (mac_user_stru*)mac_res_get_mac_user(us_user_idx);

    switch (pst_ta_mac_user->st_key_info.en_cipher_type)
    {
        case WLAN_80211_CIPHER_SUITE_WEP_40:
        case WLAN_80211_CIPHER_SUITE_WEP_104:
            /* wep���� */
            ul_data_len = dmac_ieee80211_wep_decrypt(puc_hdr,
                                                pst_rx_info->uc_mac_header_len,
                                                puc_hdr + pst_rx_info->uc_mac_header_len,
                                                pst_rx_info->us_frame_len,
                                                pst_ta_mac_user->st_key_info.uc_default_index,
                                                pst_ta_mac_user->st_key_info.ast_key[pst_ta_mac_user->st_key_info.uc_default_index].ul_key_len,
                                                pst_ta_mac_user->st_key_info.ast_key[pst_ta_mac_user->st_key_info.uc_default_index].auc_key,
                                                &uc_dscr_status);
            /* ����cb */
            pst_cb_ctrl->st_rx_status.bit_dscr_status = uc_dscr_status;
            pst_cb_ctrl->st_rx_info.us_frame_len = (oal_uint16)ul_data_len;
            break;
        case WLAN_80211_CIPHER_SUITE_TKIP:
            pst_mac_vap = (mac_vap_stru*)mac_res_get_mac_vap(pst_rx_info->uc_mac_vap_id);
            en_auth_supp = (WLAN_VAP_MODE_BSS_AP == pst_mac_vap->en_vap_mode) ? HAL_AUTH_KEY : HAL_SUPP_KEY;
            /* tkip���� */
            ul_data_len = dmac_ieee80211_tkip_decrypt(puc_hdr,
                                                      pst_rx_info->uc_mac_header_len,
                                                      puc_hdr + pst_rx_info->uc_mac_header_len,
                                                      pst_rx_info->us_frame_len,
                                                      pst_ta_mac_user->st_key_info.uc_default_index,
                                                      pst_ta_mac_user->st_key_info.ast_key[pst_ta_mac_user->st_key_info.uc_default_index].ul_key_len,
                                                      pst_ta_mac_user->st_key_info.ast_key[pst_ta_mac_user->st_key_info.uc_default_index].auc_key,
                                                      en_auth_supp,
                                                      &uc_dscr_status,
                                                      1,
                                                      1);
            /* ����cb */
            pst_cb_ctrl->st_rx_status.bit_dscr_status = uc_dscr_status;
            pst_cb_ctrl->st_rx_info.us_frame_len = (oal_uint16)ul_data_len;
            OAM_INFO_LOG3(0, OAM_SF_SOFT_CRYPTO, "dmac_rx_decrypt_data_frame auth_supp=%d, rx status=%d, frame_len=%d.",
                en_auth_supp,
                pst_cb_ctrl->st_rx_status.bit_dscr_status,
                pst_cb_ctrl->st_rx_info.us_frame_len);
            break;
        case WLAN_80211_CIPHER_SUITE_CCMP:
        case WLAN_80211_CIPHER_SUITE_GCMP:
        case WLAN_80211_CIPHER_SUITE_GCMP_256:
        case WLAN_80211_CIPHER_SUITE_CCMP_256:
            /* ccmp���� */
            ul_data_len = dmac_ieee80211_aes_ccmp_decrypt(puc_hdr,
                                                          pst_rx_info->uc_mac_header_len,
                                                          puc_hdr + pst_rx_info->uc_mac_header_len,
                                                          pst_rx_info->us_frame_len,
                                                          pst_ta_mac_user->st_key_info.uc_default_index,
                                                          pst_ta_mac_user->st_key_info.ast_key[pst_ta_mac_user->st_key_info.uc_default_index].ul_key_len,
                                                          pst_ta_mac_user->st_key_info.ast_key[pst_ta_mac_user->st_key_info.uc_default_index].auc_key,
                                                          &uc_dscr_status,
                                                          1,
                                                          0);
            pst_cb_ctrl->st_rx_status.bit_dscr_status = uc_dscr_status;
            pst_cb_ctrl->st_rx_info.us_frame_len = (oal_uint16)ul_data_len;
            OAM_INFO_LOG2(0, OAM_SF_SOFT_CRYPTO, "dmac_rx_decrypt_data_frame ccmp, rx status=%d, frame_len=%d.",
                pst_cb_ctrl->st_rx_status.bit_dscr_status,
                pst_cb_ctrl->st_rx_info.us_frame_len);
            break;
        default:
            return OAL_ERR_CODE_SECURITY_CHIPER_TYPE;
    }

    return OAL_SUCC;
}
#endif

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif



