

#ifndef __DMAC_11i_H__
#define __DMAC_11i_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 ����ͷ�ļ�����
*****************************************************************************/
#include "oal_ext_if.h"
#include "oal_mem.h"
#include "mac_vap.h"
#include "dmac_user.h"
//#include "mac_11i.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_11I_H
/*****************************************************************************
  2 �궨��
*****************************************************************************/
#define MAC_ADDR(_puc_mac)   ((oal_uint32)(((oal_uint32)_puc_mac[2] << 24) |\
                                                      ((oal_uint32)_puc_mac[3] << 16) |\
                                                      ((oal_uint32)_puc_mac[4] << 8) |\
                                                      ((oal_uint32)_puc_mac[5])))
#ifdef _PRE_WLAN_INIT_PTK_TX_PN
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
#define PN_MSB_INIT_VALUE 0xFF00          //Ϊ�˽��ͶԶ�PN���䵼��ping��ͨ�ĸ��ʣ�����ʼֵ���õø���
#else
#define PN_MSB_INIT_VALUE 0x1001          //��ʱluolingzhiΪ�˹��PN�������ȡ�ĳ�ʼֵ
#endif
#endif

/*****************************************************************************
  3 ö�ٶ���
*****************************************************************************/


/*****************************************************************************
  4 ȫ�ֱ�������
*****************************************************************************/


/*****************************************************************************
  5 ��Ϣͷ����
*****************************************************************************/


/*****************************************************************************
  6 ��Ϣ����
*****************************************************************************/


/*****************************************************************************
  7 STRUCT����
*****************************************************************************/




/*****************************************************************************
  8 UNION����
*****************************************************************************/


/*****************************************************************************
  9 OTHERS����
*****************************************************************************/


/*****************************************************************************
  10 ��������
*****************************************************************************/
oal_uint32 dmac_check_igtk_exist(oal_uint8 uc_igtk_index);
oal_uint32  dmac_config_11i_add_key(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
oal_uint32  dmac_config_11i_remove_key(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
oal_uint32  dmac_config_11i_set_default_key(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
oal_uint32  dmac_config_11i_add_cipher_mode(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
oal_uint32 dmac_config_11i_init_port(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);

oal_uint32  dmac_11i_update_key_to_ce(mac_vap_stru *pst_mac_vap, hal_security_key_stru *pst_security_key, oal_uint8 *puc_addr);
oal_uint32  dmac_11i_del_peer_macaddr(mac_vap_stru *pst_mac_vap, oal_uint8 uc_lut_index);
oal_uint32  dmac_11i_add_key_from_user(mac_vap_stru *pst_mac_vap, dmac_user_stru *pst_dmac_user);
oal_uint32  dmac_11i_remove_key_from_user(mac_vap_stru *pst_mac_vap, dmac_user_stru *pst_dmac_user);

oal_void dmac_11i_tkip_mic_failure_handler(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_user_mac, oal_nl80211_key_type en_key_type);
oal_uint32  dmac_11i_add_ptk_key(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_mac_addr, oal_uint8 uc_key_index);
oal_uint32  dmac_11i_add_gtk_key(mac_vap_stru *pst_mac_vap, oal_uint8 uc_key_index);
oal_uint32  dmac_11i_add_wep_key(mac_vap_stru *pst_mac_vap, oal_uint8 uc_key_index);
oal_uint32 dmac_reset_gtk_token(mac_vap_stru *pst_mac_vap);
#ifdef _PRE_WLAN_FEATURE_WAPI
oal_uint32  dmac_config_wapi_add_key(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
#endif

#ifdef _PRE_WLAN_INIT_PTK_TX_PN
extern oal_bool_enum dmac_init_iv_word_lut(hal_to_dmac_device_stru *pst_hal_device, hal_security_key_stru *pst_security_key,oal_uint32 ul_pn_msb);
#endif


#ifdef _PRE_WLAN_FEATURE_USER_EXTEND
extern oal_uint32  dmac_11i_add_machw_pn(dmac_user_stru *pst_dmac_user);
#endif
#ifdef _PRE_WLAN_FEATURE_SOFT_CRYPTO
oal_uint32  dmac_rx_decrypt_data_frame(oal_netbuf_stru *pst_netbuf,oal_uint16 us_user_idx);
#endif

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of dmac_11i_ap.h */
