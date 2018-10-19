

#ifndef __DMAC_STS_PM_H__
#define __DMAC_STS_PM_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_STA_PM
/*****************************************************************************
  1 ����ͷ�ļ�����
*****************************************************************************/
#include "oal_ext_if.h"
#include "mac_device.h"
#include "mac_frame.h"
#include "mac_pm.h"
#include "dmac_vap.h"
#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_STA_PM_H
/*****************************************************************************
  2 �궨��
*****************************************************************************/
#define WLAN_NBBY      8
//#define WLAN_TIM_ISSET(a,i) ((a)[(i)/WLAN_NBBY] & (1<<((i)%WLAN_NBBY)))
#define WLAN_TIM_ISSET(a,i) ((a)[(i)>>3] & (1<<((i)&0x7)))
#define LISTEN_INTERVAL_TO_DITM_TIMES   4
#define MIN_ACTIVITY_TIME_OUT 20000   /* 500 ms */
#define MAX_ACTIVITY_TIME_OUT 20000 /* 10 sec */
#define WLAN_PS_KEEPALIVE_MAX_NUM   300 /* 300 DTIM1 interval*/
#define WLAN_MAX_NULL_SENT_NUM      10  /* NULL֡����ش����� */

#define DMAC_TIMER_DOZE_TRANS_FAIL_NUM        10
#define DMAC_STATE_DOZE_TRANS_FAIL_NUM        2          //����N����dozeʧ�����ά��
#define DMAC_BEACON_DOZE_TRANS_FAIL_NUM       2
#define DMAC_BEACON_TIMEOUT_MAX_TIME          1000 //�ղ���beacon���˯��ʱ��,��λms
#define DMAC_RECE_MCAST_TIMEOUT               20   //���չ㲥�鲥֡��ʱ--û�յ�

#if defined(_PRE_DEBUG_MODE)
#define DMAC_STA_UAPSD_STATS_INCR(_member)    ((_member)++)
#define DMAC_STA_PSM_STATS_INCR(_member)      ((_member)++)
#elif defined(_PRE_PSM_DEBUG_MODE)
#define DMAC_STA_PSM_STATS_INCR(_member)    ((_member)++)
#define DMAC_STA_UAPSD_STATS_INCR(_member)
#else
#define DMAC_STA_UAPSD_STATS_INCR(_member)
#define DMAC_STA_PSM_STATS_INCR(_member)
#endif
/*****************************************************************************
  3 ö�ٶ���
*****************************************************************************/
/* TIM processing result */
typedef enum
{
    DMAC_TIM_IS_SET  = 1,
    DMAC_DTIM_IS_SET = 2,

    DMAC_TIM_PROC_BUTT
} dmac_tim_proc_enum;

typedef enum {
    STA_PWR_SAVE_STATE_ACTIVE = 0,         /* active״̬ */
    STA_PWR_SAVE_STATE_DOZE,               /* doze״̬ */
    STA_PWR_SAVE_STATE_AWAKE,              /* wake״̬*/

    STA_PWR_SAVE_STATE_BUTT                /*���״̬*/
} sta_pwr_save_state_info;

typedef enum {
    STA_PWR_EVENT_TX_DATA = 0,              /* DOZE״̬�µķ����¼� */
    STA_PWR_EVENT_TBTT,                     /* DOZE״̬�µ�TBTT�¼� */
    STA_PWR_EVENT_FORCE_AWAKE,              /* DOZE״̬�º�tbtt���ѻ�û����,�ֶ�ǿ�ƻ��� */
    STA_PWR_EVENT_RX_UCAST,                 /* AWAKE״̬�½��յ���  */
    STA_PWR_EVENT_LAST_MCAST = 4,           /* AWAKE״̬�����һ���鲥/�㲥 */
    STA_PWR_EVENT_TIM,                      /* AWAKE״̬�µ�TIM�¼� */
    STA_PWR_EVENT_DTIM,                     /* AWAKE״̬�µ�DTIM�¼� */
    STA_PWR_EVENT_NORMAL_SLEEP,
    STA_PWR_EVENT_BEACON_TIMEOUT  = 8,      /* AWAKE״̬��˯���¼� */
    STA_PWR_EVENT_SEND_NULL_SUCCESS,        /* ����״̬�¶���NULL֡���ͳɹ��¼� */
    STA_PWR_EVENT_TIMEOUT,                  /* ACTIVE/AWAKE״̬�³�ʱ�¼� */
    STA_PWR_EVENT_KEEPALIVE,                /* ACTIVE״̬�·ǽ���ģʽ��keepalive�¼� */
    STA_PWR_EVENT_NO_POWERSAVE = 12,        /* DOZE/AWAKE״̬�·ǽ���ģʽ */
    STA_PWR_EVENT_P2P_SLEEP,                /* P2P SLEEP */
    STA_PWR_EVENT_P2P_AWAKE,                /* P2P AWAKE */
    STA_PWR_EVENT_DETATCH,                  /* ���ٵ͹���״̬���¼� */
    STA_PWR_EVENT_DEASSOCIATE,              /* ȥ����*/
    STA_PWR_EVENT_TX_MGMT,                  /* ���͹���֡����ǰ�� */
    STA_PWR_EVENT_TX_COMPLETE,              /* ��������¼� */
    STA_PWR_EVENT_ENABLE_FRONT_END,         /* ��ǰ���¼���״̬���е�active״̬ */
    STA_PWR_EVENT_RX_BEACON,                /* doze״̬���յ�beacon */
    STA_PWR_EVENT_RX_DATA,                  /* doze״̬���յ�����֡ */

#ifdef _PRE_WLAN_DOWNLOAD_PM
    STA_PWR_EVENT_NOT_EXCEED_MAX_SLP_TIME,  /* ����ģʽ�¾����ϴο�ʼ��������֡��ʱ��δ�����������˯��ʱ���¼� */
#endif
    STA_PWR_EVENT_NO_PS_FRM,
    STA_PWR_EVENT_BUTT
}sta_pwr_save_event;

/*****************************************************************************
  4 ȫ�ֱ�������
*****************************************************************************/
extern    oal_uint32   g_lightsleep_fe_awake_cnt;
extern    oal_uint32   g_deepsleep_fe_awake_cnt;
/*****************************************************************************
  5 ��Ϣͷ����
*****************************************************************************/


/*****************************************************************************
  6 ��Ϣ����
*****************************************************************************/


/*****************************************************************************
  7 STRUCT����
*****************************************************************************/

typedef enum
{
    STA_PS_DEEP_SLEEP   = 0,
    STA_PS_LIGHT_SLEEP  = 1
}ps_mode_state_enum;

typedef  enum
{
    DMAC_SP_NOT_IN_PROGRESS         = 0,
    DMAC_SP_WAIT_START              = 1,
    DMAC_SP_IN_PROGRESS             = 2,
    DMAC_SP_UAPSD_STAT_BUTT
}dmac_uapsd_sp_stat_sta_enum;

/*****************************************************************************
  8 UNION����
*****************************************************************************/


/*****************************************************************************
  9 OTHERS����
*****************************************************************************/

/*****************************************************************************
  10 ��������
*****************************************************************************/
extern oal_void dmac_pm_sta_state_trans(mac_sta_pm_handler_stru* pst_handler,oal_uint8 uc_state, oal_uint16 us_event);
extern oal_uint32 dmac_pm_sta_post_event(dmac_vap_stru *pst_dmac_vap, oal_uint16 us_type, oal_uint16 us_datalen, oal_uint8* pst_data);
extern oal_void dmac_pm_sta_attach(dmac_vap_stru *pst_dmac_vap);
extern oal_void dmac_pm_sta_detach(dmac_vap_stru *pst_dmac_vap);
extern oal_void dmac_sta_initialize_psm_globals(mac_sta_pm_handler_stru *p_handler);
extern oal_void dmac_pm_key_info_dump(dmac_vap_stru  *pst_dmac_vap);
extern oal_void dmac_pm_change_to_active_state(dmac_vap_stru *pst_dmac_vap, oal_uint8 uc_event);
extern oal_void dmac_pm_sta_wait_for_mcast(dmac_vap_stru *pst_dmac_vap, mac_sta_pm_handler_stru *pst_mac_sta_pm_handle);
#endif /* _PRE_WLAN_FEATURE_STA_PM*/
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of dmac_sts_pm.h */
