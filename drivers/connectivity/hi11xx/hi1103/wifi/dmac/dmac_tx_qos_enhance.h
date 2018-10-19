

#ifndef __DMAC_TX_QOS_ENHANCE_H__
#define __DMAC_TX_QOS_ENHANCE_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_QOS_ENHANCE

/*****************************************************************************
  1 ����ͷ�ļ�����
*****************************************************************************/
#include "oal_ext_if.h"
#include "dmac_ext_if.h"
#include "dmac_vap.h"
#include "dmac_user.h"
#include "dmac_main.h"
#include "oal_net.h"



#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_TX_QOS_ENHANCE_H
/*****************************************************************************
  2 �궨��
*****************************************************************************/
#define DMAC_QOS_ENHANCE_RSSI_LOW_THD    -75   // xx-100dBm
#define DMAC_QOS_ENHANCE_RSSI_HIGH_THD   -60   // xx-100dBm
#define DMAC_QOS_ENHANCE_RSSI_EDGE_THD   -85   // xx-100dBm
#define DMAC_QOS_ENHANCE_TP_LOW_THD      96    // UNIT:kbps
#define DMAC_QOS_ENHANCE_TP_MID_THD      192   // UNIT:kbps
#define DMAC_QOS_ENHANCE_TP_HIGH_THD     512   // UNIT:kbps


#define DMAC_DEF_QOS_ENHANCE_TIMER        2000     /* timer interval as 2 secs */




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
extern oal_void dmac_tx_qos_enhance_attach(dmac_vap_stru *pst_dmac_vap);
extern oal_void dmac_tx_qos_enhance_detach(dmac_vap_stru *pst_dmac_vap);
extern oal_void dmac_tx_delete_qos_enhance_sta(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_sta_member_addr, oal_uint8 uc_flag);
#endif /* _PRE_WLAN_FEATURE_QOS_ENHANCE */

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of dmac_tx_qos_enhance.h */
