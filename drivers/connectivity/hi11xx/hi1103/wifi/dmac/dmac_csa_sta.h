
#ifndef __DMAC_CSA_STA_H__
#define __DMAC_CSA_STA_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/*****************************************************************************
  1 ����ͷ�ļ�����
*****************************************************************************/
#include "oal_ext_if.h"
#include "frw_ext_if.h"
#include "dmac_vap.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_CSA_STA_H
/*****************************************************************************
  2 �궨��
*****************************************************************************/


/*****************************************************************************
  3 ö�ٶ���
*****************************************************************************/
typedef enum
{
    WLAN_STA_CSA_FSM_INIT = 0,
    WLAN_STA_CSA_FSM_START,
    WLAN_STA_CSA_FSM_SWITCH,
    WLAN_STA_CSA_FSM_WAIT,
    WLAN_STA_CSA_FSM_INVALID,
    WLAN_STA_CSA_FSM_BUTT
}wlan_sta_csa_fsm_enum;
typedef oal_uint8 wlan_sta_csa_fsm_enum_uint8;

typedef enum
{
    WLAN_STA_CSA_EVENT_GET_IE = 0,
    WLAN_STA_CSA_EVENT_TBTT,
    WLAN_STA_CSA_EVENT_RCV_BEACON,
    WLAN_STA_CSA_EVENT_RCV_PROBE_RSP,
    WLAN_STA_CSA_EVENT_SCAN_END,
    WLAN_STA_CSA_EVENT_TO_INIT,
    WLAN_STA_CSA_EVENT_BUTT
}wlan_sta_csa_event_enum;
typedef oal_uint8 wlan_sta_csa_event_enum_uint8;





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
extern oal_uint8 dmac_sta_csa_fsm_get_current_state(mac_vap_stru *pst_mac_vap);
extern oal_bool_enum dmac_sta_csa_is_in_waiting(mac_vap_stru *pst_mac_vap);
extern oal_void dmac_sta_csa_fsm_attach(dmac_vap_stru *pst_dmac_vap);
extern oal_void dmac_sta_csa_fsm_detach(dmac_vap_stru *pst_dmac_vap);
extern oal_uint32 dmac_sta_csa_fsm_post_event(mac_vap_stru* pst_mac_vap, oal_uint16 us_type, oal_uint16 us_datalen, oal_uint8* pst_data);


#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of dmac_chan_mgmt.h */
