
#ifndef    __DMAC_GREEN_AP_H__
#define    __DMAC_GREEN_AP_H__

#ifdef  __cplusplus
#if     __cplusplus
extern  "C" {
#endif
#endif

#ifdef    _PRE_WLAN_FEATURE_GREEN_AP
/*****************************************************************************
  1 ����ͷ�ļ�����
*****************************************************************************/
#include    "oal_types.h"
#include    "oal_hardware.h"
#include    "frw_ext_if.h"
#include    "mac_device.h"


#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_GREEN_AP_H

/*****************************************************************************
  2 �궨��
*****************************************************************************/
#define  DMAC_DEFAULT_GAP_WORK_TIME     25
#define  DMAC_DEFAULT_GAP_PAUSE_TIME    25
#define  DMAC_DEFAULT_GAP_SLOT_CNT      2*100/(DMAC_DEFAULT_GAP_WORK_TIME + DMAC_DEFAULT_GAP_PAUSE_TIME)
#define  DMAC_DEFAULT_GAP_RESUME_OFFSET 0
#define  DMAC_DEFAULT_GAP_BCN_GUARD_TIME    15

#define  DMAC_GAP_EN_THRPT_HIGH_TH        50                        /* ���������ڴ����ޣ�green ap�ſ��� */
#define  DMAC_GAP_PPS_TO_THRPT_MBPS(pps)  ((pps * 8 * 1460) >> 20)  /* ppsת������(Mbps) */
#define  DMAC_GAP_PPS_MAX_COUNT           15

#define  DMAC_GAP_FREE_RATIO_CALC(_ul_free_time, _ul_total_time) \
        (((_ul_total_time) <= 0 ) ? 0 : (1000 * (_ul_free_time) / (_ul_total_time)))

#define  DMAC_GAP_DEFAULT_FREE_RATIO_CCA      450           /* ccaͳ�Ƶ�free ratio���� */
#define  DMAC_GAP_DEFAULT_FREE_RATIO_GAP      300           /* green apͳ�Ƶ�free ratio���� */

#define  DMAC_GAP_SCAN_TIME_MS                10            /* ������Сɨ��ʱ�� */
#define  DMAC_GAP_SCAN_RESV_TIME_MS           5             /* ɨ��Ԥ��ʱ��, ɨ���������Ӳ����ʱ����ͬ�� */
#define  DMAC_GAP_SCAN_MAX_TBTT_NUM           8             /* 1s��green ap����ɨ���tbtt���ڸ��� */

/*****************************************************************************
  3 ö�ٶ���
*****************************************************************************/
typedef enum
{
    DMAC_GREEN_AP_STATE_NOT_INITED   = 0,
    DMAC_GREEN_AP_STATE_INITED       = 1,
    DMAC_GREEN_AP_STATE_WORK         = 2,
    DMAC_GREEN_AP_STATE_PAUSE        = 3,

    DMAC_GREEN_AP_STATE_BUTT
}dmac_green_ap_state_enum;
typedef oal_uint8 dmac_green_ap_state_enum_uint8;

/* green ap �����¼����� */
typedef enum
{
    DMAC_GREEN_AP_ENABLE     = 0,
    DMAC_GREEN_AP_VAP_UP     = 1,
    DMAC_GREEN_AP_VAP_DOWN   = 2,
    DMAC_GREEN_AP_SCAN_START = 3,

    DMAC_GREEN_AP_SWITCH_BUTT
}dmac_green_ap_switch_type;
typedef oal_uint8 dmac_green_ap_switch_type_uint8;

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
typedef struct tag_dmac_green_ap_scan_info_stru
{
    oal_uint32  ul_total_stat_time;
    oal_uint32  ul_total_free_time_20M;
    oal_uint32  ul_total_free_time_40M;
    oal_uint32  ul_total_free_time_80M;
}dmac_gap_scan_info_stru;

typedef struct tag_dmac_green_ap_mgr_stru
{
    oal_uint8                      uc_green_ap_enable : 1,   /* green apʹ�� */
                                   uc_debug_mode      : 7;   /* green ap debugʹ�� */
    oal_uint8                      uc_green_ap_suspend;      /* green ap�Ƿ�suspend */
    oal_bool_enum_uint8            en_green_ap_dyn_en;       /* green ap ��̬ʹ�ܿ���(�������ж�) */
    oal_bool_enum_uint8            en_green_ap_dyn_en_old;   /* �ϸ����ڵ�green ap ��̬ʹ�ܽ�� */
    dmac_green_ap_state_enum_uint8 uc_state;                 /* green ap״̬ */
    oal_uint8                      uc_chip_id;
    oal_uint8                      uc_device_id;         /* ָ���Ӧ��device id */
    oal_uint8                      uc_hal_vap_id;

    oal_uint8                      uc_vap_id;
    oal_uint8                      uc_work_time;         /* ����ʱ϶��ʱ��ms*/
    oal_uint8                      uc_pause_time;        /* ��ͣʱ϶��ʱ��ms */
    oal_uint8                      uc_max_slot_cnt;      /* beacon���ڷ�Ϊ���ٸ�ʱ϶ = 2*beacon period/(work+pause) */

    oal_uint8                      uc_cur_slot;          /* ��ǰʱ϶ */
    oal_uint8                      uc_resume_offset;     /* resume��Ҫ��ǰ��ʱ�������ڱ���ʱ�䵽ǰ��ǰresume�ָ�RF */
    oal_uint8                      uc_pps_count;         /* ppsͳ�����ڸ��� */
    oal_uint8                      resv1[1];

#if (!defined(_PRE_PRODUCT_ID_HI110X_DEV))
    struct hrtimer st_gap_timer;
#endif

    hal_one_packet_cfg_stru         st_one_packet_cfg;
    oal_uint8                       uc_one_packet_done;
    oal_uint8                       uc_cca_scan_enble;
    oal_uint8                       uc_tbtt_num;         /* tbtt�жϸ���ͳ�� */
    oal_uint8                       uc_scan_req_time;    /* һ��timer�����ڷ���ɨ��Ĵ��� */
    dmac_gap_scan_info_stru         st_scan_info;

    oal_uint32                      ul_pause_count;
    oal_uint32                      ul_resume_count;
    oal_uint32                      ul_total_count;
    oal_uint32                      ul_free_time_th_cca;
    oal_uint32                      ul_free_time_th_gap;
}dmac_green_ap_mgr_stru;

/* green ap �¼��ṹ�� */
typedef struct tag_dmac_green_ap_fcs_event_stru
{
    dmac_green_ap_state_enum_uint8 uc_state_to;     /* Ҫ�л�����״̬ */
    oal_uint8                      uc_chip_id;
    oal_uint8                      uc_device_id;
    oal_uint8                      uc_cur_slot;
}dmac_gap_fcs_event_stru;

/*****************************************************************************
  8 UNION����
*****************************************************************************/


/*****************************************************************************
  9 OTHERS����
*****************************************************************************/

/*****************************************************************************
  10 ��������
*****************************************************************************/
extern oal_uint32  dmac_green_ap_init(mac_device_stru *pst_dmac_device);
extern oal_uint32  dmac_green_ap_exit(mac_device_stru *pst_dmac_device);
extern oal_uint32  dmac_green_ap_timer_event_handler(frw_event_mem_stru *pst_event_mem);
extern oal_uint32  dmac_green_ap_suspend(mac_device_stru *pst_device);
extern oal_uint32  dmac_green_ap_resume(mac_device_stru *pst_device);
extern oal_uint32  dmac_green_ap_dump_info(oal_uint8 uc_device_id);
extern oal_void    dmac_green_ap_pps_process(oal_uint32 ul_pps_rate);
extern oal_void    dmac_green_ap_enable(oal_uint8 uc_device_id);
extern oal_void    dmac_green_ap_disable(oal_uint8 uc_device_id);
extern oal_uint32  dmac_green_ap_switch_auto(oal_uint8 uc_device_id, dmac_green_ap_switch_type_uint8 uc_switch_type);
extern oal_void    dmac_green_ap_optimization(mac_device_stru *pst_device, wlan_scan_chan_stats_stru *pst_chan_result, oal_bool_enum_uint8 en_from_cca);
extern oal_uint32  dmac_green_ap_get_scan_enable(mac_device_stru *pst_device);
extern oal_void    dmac_green_ap_set_scan_enable(oal_uint8 uc_device_id, oal_uint8 uc_cca_en);
extern oal_void    dmac_green_ap_set_free_ratio(oal_uint8 uc_device_id, oal_uint8 uc_green_ap_mode, oal_uint8 uc_free_ratio_th);
extern oal_void    dmac_green_ap_set_debug_mode(oal_uint8 uc_device_id, oal_uint8 uc_debug_en);

#endif /* _PRE_WLAN_FEATURE_GREEN_AP */

#ifdef  __cplusplus
#if     __cplusplus
    }
#endif
#endif

#endif
