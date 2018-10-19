

#ifndef __DMAC_11W_H__
#define __DMAC_11W_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 ����ͷ�ļ�����
*****************************************************************************/
#include "oal_ext_if.h"
#include "wlan_types.h"
#include "dmac_vap.h"



/*****************************************************************************
  2 �궨��
*****************************************************************************/


/*****************************************************************************
  3 ö�ٶ���
*****************************************************************************/


/*****************************************************************************
  4 ȫ�ֱ�������
*****************************************************************************/
#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)
typedef oal_void (*bip_crypto)(dmac_vap_stru *pst_dmac_vap,
                                 oal_netbuf_stru *pst_netbuf_mgmt,
                                 wlan_security_txop_params_stru  *pst_security,
                                 hal_tx_mpdu_stru *pst_mpdu);

extern bip_crypto  bip_crypto_cb;

typedef oal_bool_enum_uint8  (*p_dmac_11w_robust_action_cb)(oal_netbuf_stru *pst_mgmt_buf);

typedef struct
{
    p_dmac_11w_robust_action_cb   p_dmac_11w_robust_action;
}dmac_11w_rom_cb;

#endif
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

#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)

extern oal_void dmac_bip_crypto(dmac_vap_stru *pst_dmac_vap,
                                           oal_netbuf_stru *pst_netbuf_mgmt,
                                           wlan_security_txop_params_stru  *pst_security,
                                           hal_tx_mpdu_stru *pst_mpdu);
extern oal_uint32 dmac_11w_rx_filter(dmac_vap_stru *pst_dmac_vap, oal_netbuf_stru  *pst_netbuf);
extern oal_void dmac_11w_set_ucast_mgmt_frame(dmac_vap_stru  *pst_dmac_vap,mac_tx_ctl_stru *pst_tx_ctl,
                                            hal_tx_txop_feature_stru *pst_txop_feature,
                                            oal_netbuf_stru *pst_netbuf, mac_ieee80211_frame_stru *puc_frame_hdr);
extern oal_void dmac_11w_set_mcast_mgmt_frame(dmac_vap_stru  *pst_dmac_vap,
                                             hal_tx_txop_feature_stru *pst_txop_feature,
                                             oal_netbuf_stru *pst_netbuf, hal_tx_mpdu_stru *pst_mpdu);
extern oal_void dmac_11w_get_pmf_cap(mac_vap_stru *pst_mac_vap, wlan_pmf_cap_status_uint8 *pen_pmf_cap);

extern oal_void dmac_11w_update_users_status(dmac_vap_stru  *pst_dmac_vap, mac_user_stru *pst_mac_user, oal_bool_enum_uint8 en_user_pmf);

#endif /* #if(_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT) */




#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of dmac_11w.h */
