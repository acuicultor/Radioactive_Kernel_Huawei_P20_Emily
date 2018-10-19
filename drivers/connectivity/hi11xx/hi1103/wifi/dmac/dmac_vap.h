

#ifndef __DMAC_VAP_H__
#define __DMAC_VAP_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 ����ͷ�ļ�����
*****************************************************************************/
#include "oal_ext_if.h"
#include "hal_ext_if.h"
#include "mac_vap.h"
#include "dmac_user.h"
#include "dmac_ext_if.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_VAP_H

/*****************************************************************************
  2 �궨��
*****************************************************************************/
#define DMAC_MAX_SW_RETRIES         10  /* ���ݱ����ش����� */
#define DMAC_MGMT_MAX_SW_RETRIES    3   /* �������ش����� */
#define DMAC_MAX_AMPDU_LENGTH_PERFOMANCE_COUNT    32   /* ���м��������ľۺϳ���*/

#define DMAX_FTM_RANGE_ENTRY_COUNT                 15
#define DMAX_FTM_ERROR_ENTRY_COUNT                 11
#define DMAX_MINIMUN_AP_COUNT                      14
#define MAX_FTM_SESSION                            8     /*FTMͬʱ���ߵĻػ�*/

#define DMAC_BW_VERIFY_MAX_THRESHOLD               60

#if(_PRE_WLAN_FEATURE_PMF == _PRE_PMF_HW_CCMP_BIP)

#define IS_OPEN_PMF_REG(_pst_dmac_vap)  (0 != ((_pst_dmac_vap)->ul_user_pmf_status))
#endif

#define GET_CURRENT_LINKLOSS_CNT(_pst_dmac_vap)                      (_pst_dmac_vap->st_linkloss_info.ast_linkloss_info[_pst_dmac_vap->st_linkloss_info.en_linkloss_mode].uc_link_loss)
#define GET_CURRENT_LINKLOSS_THRESHOLD(_pst_dmac_vap)                (_pst_dmac_vap->st_linkloss_info.ast_linkloss_info[_pst_dmac_vap->st_linkloss_info.en_linkloss_mode].uc_linkloss_threshold)
#define GET_CURRENT_LINKLOSS_INT_THRESHOLD(_pst_dmac_vap)            (_pst_dmac_vap->st_linkloss_info.auc_int_linkloss_threshold[_pst_dmac_vap->st_linkloss_info.en_linkloss_mode])
#define GET_CURRENT_LINKLOSS_MIN_THRESHOLD(_pst_dmac_vap)            (_pst_dmac_vap->st_linkloss_info.auc_min_linkloss_threshold[_pst_dmac_vap->st_linkloss_info.en_linkloss_mode])
#define GET_CURRENT_LINKLOSS_THRESHOLD_DECR(_pst_dmac_vap)           (_pst_dmac_vap->st_linkloss_info.auc_linkloss_threshold_decr[_pst_dmac_vap->st_linkloss_info.en_linkloss_mode])

#define MAKE_CURRENT_LINKLOSS_THRESHOLD(_pst_dmac_vap, en_linkloss_mode) ((100*g_auc_int_linkloss_threshold[en_linkloss_mode] / _pst_dmac_vap->st_linkloss_info.ul_dot11BeaconPeriod))

#define INCR_CURRENT_LINKLOSS_CNT(_pst_dmac_vap)                      GET_CURRENT_LINKLOSS_CNT(_pst_dmac_vap)++
#define GET_CURRENT_LINKLOSS_MODE(_pst_dmac_vap)                     (_pst_dmac_vap->st_linkloss_info.en_linkloss_mode)

#define GET_LINKLOSS_CNT(_pst_dmac_vap, en_linkloss_mode)            (_pst_dmac_vap->st_linkloss_info.ast_linkloss_info[en_linkloss_mode].uc_link_loss)
#define GET_LINKLOSS_THRESHOLD(_pst_dmac_vap, en_linkloss_mode)      (_pst_dmac_vap->st_linkloss_info.ast_linkloss_info[en_linkloss_mode].uc_linkloss_threshold)
#define GET_LINKLOSS_INT_THRESHOLD(_pst_dmac_vap, en_linkloss_mode)  (_pst_dmac_vap->st_linkloss_info.auc_int_linkloss_threshold[en_linkloss_mode])
#define GET_LINKLOSS_MIN_THRESHOLD(_pst_dmac_vap, en_linkloss_mode)  (_pst_dmac_vap->st_linkloss_info.auc_min_linkloss_threshold[en_linkloss_mode])
#define GET_LINKLOSS_THRESHOLD_DECR(_pst_dmac_vap, en_linkloss_mode) (_pst_dmac_vap->st_linkloss_info.auc_linkloss_threshold_decr[en_linkloss_mode])

#define DMAC_IS_LINKLOSS(_pst_dmac_vap)                              (GET_CURRENT_LINKLOSS_CNT(_pst_dmac_vap) > GET_CURRENT_LINKLOSS_THRESHOLD(_pst_dmac_vap))

#define DMAC_MAX_TX_SUCCESSIVE_FAIL_PRINT_THRESHOLD_BTCOEX    (40)   /* ��������ʧ�ܵĴ�ӡRF�Ĵ�������*/
#define DMAC_MAX_TX_SUCCESSIVE_FAIL_PRINT_THRESHOLD    (20)   /* ��������ʧ�ܵĴ�ӡRF�Ĵ�������*/
#define GET_PM_FSM_OWNER(_pst_handler) ((dmac_vap_stru *)((_pst_handler)->st_oal_fsm.p_oshandler))/* ��ȡvapָ�� */
#define GET_PM_STATE(_pst_handler)     ((_pst_handler)->st_oal_fsm.uc_cur_state)/* ��ȡ��ǰ�͹���״̬����״̬ */

#define MAC_GET_DMAC_VAP(_pst_mac_vap)    ((dmac_vap_stru *)_pst_mac_vap)
/*****************************************************************************
  3 ö�ٶ���
*****************************************************************************/
typedef enum
{
    DMAC_STA_BW_SWITCH_EVENT_CHAN_SYNC = 0,
    DMAC_STA_BW_SWITCH_EVENT_BEACON_20M,
    DMAC_STA_BW_SWITCH_EVENT_BEACON_40M,
    DMAC_STA_BW_SWITCH_EVENT_RX_UCAST_DATA_COMPLETE,
    DMAC_STA_BW_SWITCH_EVENT_USER_DEL,
    DMAC_STA_BW_SWITCH_EVENT_RSV,
    DMAC_STA_BW_SWITCH_EVENT_BUTT
}wlan_sta_bw_switch_event_enum;
typedef oal_uint8 wlan_sta_bw_switch_event_enum_uint8;

/* beacon֡����ö�� */
typedef enum
{
    DMAC_VAP_BEACON_BUFFER1,
    DMAC_VAP_BEACON_BUFFER2,

    DMAC_VAP_BEACON_BUFFER_BUTT
}dmac_vap_beacon_buffer_enum;
/* ���ղ�ͬ�ۺϳ��ȷ����������ö��ֵ*/
/*0:1~14 */
/*1:15~17 */
/*2:18~30 */
/*3:31~32 */
typedef enum
{
    DMAC_COUNT_BY_AMPDU_LENGTH_INDEX_0,
    DMAC_COUNT_BY_AMPDU_LENGTH_INDEX_1,
    DMAC_COUNT_BY_AMPDU_LENGTH_INDEX_2,
    DMAC_COUNT_BY_AMPDU_LENGTH_INDEX_3,
    DMAC_COUNT_BY_AMPDU_LENGTH_INDEX_BUTT
}dmac_count_by_ampdu_length_enum;
/* ͳ�Ƶ�AMPDU������ֵö��ֵ*/
typedef enum
{
    DMAC_AMPDU_LENGTH_COUNT_LEVEL_1 = 1,
    DMAC_AMPDU_LENGTH_COUNT_LEVEL_14 = 14,
    DMAC_AMPDU_LENGTH_COUNT_LEVEL_15 = 15,
    DMAC_AMPDU_LENGTH_COUNT_LEVEL_17 = 17,
    DMAC_AMPDU_LENGTH_COUNT_LEVEL_18 = 18,
    DMAC_AMPDU_LENGTH_COUNT_LEVEL_30 = 30,
    DMAC_AMPDU_LENGTH_COUNT_LEVEL_31 = 31,
    DMAC_AMPDU_LENGTH_COUNT_LEVEL_32 = 32
}dmac_count_by_ampdu_length_level_enum;

#ifdef _PRE_WLAN_FEATURE_STA_PM
typedef enum _PM_DEBUG_MSG_TYPE_{
    PM_MSG_WAKE_TO_ACTIVE = 0,
    PM_MSG_WAKE_TO_DOZE,
    PM_MSG_ACTIVE_TO_DOZE,
    PM_MSG_TBTT_CNT, /* tbtt count */
    PM_MSG_PSM_BEACON_CNT,
    PM_MSG_BEACON_TIMEOUT_CNT,
    PM_MSG_PROCESS_DOZE_CNT,
    PM_MSG_BCN_TOUT_SLEEP,
    PM_MSG_DEEP_DOZE_CNT = 8,
    PM_MSG_LIGHT_DOZE_CNT,
    PM_MSG_LAST_DTIM_SLEEP,
    PM_MSG_SCAN_DIS_ALLOW_SLEEP,
    PM_MSG_DBAC_DIS_ALLOW_SLEEP,
    PM_MSG_FREQ_DIS_ALLOW_SLEEP,
    PM_MSG_BCNTIMOUT_DIS_ALLOW_SLEEP,
    PM_MSG_HOST_AWAKE,
    PM_MSG_DTIM_AWAKE,
    PM_MSG_TIM_AWAKE,
    /* ��ʱ��ʱ������ */
    PM_MSG_PSM_TIMEOUT_PM_OFF  = 18,
    PM_MSG_PSM_TIMEOUT_QUEUE_NO_EMPTY,
    PM_MSG_PSM_RESTART_A,
    PM_MSG_PSM_RESTART_B,
    PM_MSG_PSM_RESTART_C,
    PM_MSG_PSM_TIMEOUT_PKT_CNT  =23,
    PM_MSG_PSM_RESTART_P2P_PAUSE,
    PM_MSG_PSM_P2P_SLEEP,
    PM_MSG_PSM_P2P_AWAKE,
    PM_MSG_PSM_P2P_PS,
    PM_MSG_NULL_NOT_SLEEP,
    PM_MSG_DTIM_TMOUT_SLEEP,
    PM_MSG_SINGLE_BCN_RX_CNT    = 30,
    PM_MSG_BCN_TIMEOUT_SET_RX_CHAIN,       /* beacon��ʱ������bcn rx chain�ļ��� */
    PM_MSG_COUNT = 32
}PM_DEBUG_MSG_TYPE;
#endif

typedef oal_uint8 dmac_vap_beacon_buffer_enum_uint8;

typedef enum
{
    DMAC_DBDC_START,
    DMAC_DBDC_STOP,
    DMAC_DBDC_STATE_BUTT
}dmac_dbdc_state_enum;
typedef oal_uint8 dmac_dbdc_state_enum_uint8;

typedef enum
{
    DMAC_STA_BW_SWITCH_FSM_INIT = 0,
    DMAC_STA_BW_SWITCH_FSM_NORMAL,
    DMAC_STA_BW_SWITCH_FSM_VERIFY20M,  //20mУ��
    DMAC_STA_BW_SWITCH_FSM_VERIFY40M,  //40mУ��
    DMAC_STA_BW_SWITCH_FSM_INVALID,
    DMAC_STA_BW_SWITCH_FSM_BUTT
}dmac_sta_bw_switch_fsm_enum;

typedef enum
{
    DMAC_STA_BW_VERIFY_20M_TO_40M = 0,
    DMAC_STA_BW_VERIFY_40M_TO_20M,
    DMAC_STA_BW_VERIFY_SWITCH_BUTT
}dmac_sta_bw_switch_type_enum;
typedef oal_uint8 dmac_sta_bw_switch_type_enum_enum_uint8;


#ifdef _PRE_WLAN_DFT_STAT
#define   DMAC_VAP_DFT_STATS_PKT_INCR(_member, _cnt)        ((_member) += (_cnt))
#define   DMAC_VAP_DFT_STATS_PKT_SET_ZERO(_member)        ((_member) = (0))
#else
#define   DMAC_VAP_DFT_STATS_PKT_INCR(_member, _cnt)
#define   DMAC_VAP_DFT_STATS_PKT_SET_ZERO(_member)
#endif
#define   DMAC_VAP_STATS_PKT_INCR(_member, _cnt)            ((_member) += (_cnt))

#define   DMAC_VAP_GET_HAL_CHIP(_pst_vap)                  (((dmac_vap_stru *)(_pst_vap))->pst_hal_chip)
#define   DMAC_VAP_GET_HAL_DEVICE(_pst_vap)                (((dmac_vap_stru *)(_pst_vap))->pst_hal_device)
#define   DMAC_VAP_GET_HAL_VAP(_pst_vap)                   (((dmac_vap_stru *)(_pst_vap))->pst_hal_vap)
#define   DMAC_VAP_GET_FAKEQ(_pst_vap)                     (((dmac_vap_stru *)(_pst_vap))->ast_tx_dscr_queue_fake)
#define   DMAC_VAP_GET_IN_TBTT_OFFSET(_pst_vap)            (((dmac_vap_stru *)(_pst_vap))->us_in_tbtt_offset)

#ifdef _PRE_WLAN_FEATURE_BTCOEX
#define   DMAC_VAP_GET_BTCOEX_STATUS(_pst_vap)             (&(DMAC_VAP_GET_HAL_CHIP(_pst_vap)->st_btcoex_btble_status))
#define   DMAC_VAP_GET_BTCOEX_STATISTICS(_pst_vap)         (&(DMAC_VAP_GET_HAL_CHIP(_pst_vap)->st_btcoex_statistics))
#endif

#ifdef _PRE_WLAN_FEATURE_M2S
#define   DMAC_VAP_GET_VAP_M2S(_pst_vap)                   (&(((dmac_vap_rom_stru *)(MAC_GET_DMAC_VAP(_pst_vap)->_rom))->st_dmac_vap_m2s))
#define   DMAC_VAP_GET_VAP_M2S_RX_STATISTICS(_pst_vap)     (&(DMAC_VAP_GET_VAP_M2S(_pst_vap)->st_dmac_vap_m2s_rx_statistics))
#define   DMAC_VAP_GET_VAP_M2S_ACTION_ORI_TYPE(_pst_vap)   (DMAC_VAP_GET_VAP_M2S(_pst_vap)->en_m2s_switch_ori_type)
#define   DMAC_VAP_GET_VAP_M2S_ACTION_SEND_STATE(_pst_vap) (DMAC_VAP_GET_VAP_M2S(_pst_vap)->en_action_send_state)
#define   DMAC_VAP_GET_VAP_M2S_ACTION_SEND_CNT(_pst_vap)   (DMAC_VAP_GET_VAP_M2S(_pst_vap)->uc_action_send_cnt)
#endif


#define   DMAC_VAP_GET_POW_INFO(_pst_vap)       &(((dmac_vap_stru *)(_pst_vap))->st_vap_pow_info)
#define   DMAC_VAP_GET_POW_TABLE(_pst_vap)      ((((dmac_vap_stru *)(_pst_vap))->st_vap_pow_info).pst_rate_pow_table)

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
/* vap tx�������Ӽܹ������ĵ���ó�Ա */
typedef struct
{
    oal_uint16  us_rts_threshold;       /* rts��ֵ */
    oal_uint8   uc_mcast_rate;          /* �㲥���� */
    oal_uint8   auc_resv[1];            /* �ֽڶ��� */
}dmac_vap_tx_param_stru;

#ifdef _PRE_WLAN_FEATURE_ROAM

#define ROAM_TRIGGER_COUNT_THRESHOLD           (5)
#define ROAM_TRIGGER_RSSI_NE80_DB              (-80)
#define ROAM_TRIGGER_RSSI_NE75_DB              (-75)
#define ROAM_TRIGGER_RSSI_NE70_DB              (-70)
#define ROAM_TRIGGER_INTERVAL_10S              (10 * 1000)
#define ROAM_TRIGGER_INTERVAL_15S              (15 * 1000)
#define ROAM_TRIGGER_INTERVAL_20S              (20 * 1000)
#define ROAM_WPA_CONNECT_INTERVAL_TIME         (5 * 1000)    /* ��������������֮���ʱ������WIFI+ �ϲ��л�Ƶ�� */

typedef struct
{
    oal_int8    c_trigger_2G;           /* 2G���δ�������   */
    oal_int8    c_trigger_5G;           /* 5G���δ�������   */
    oal_uint8   auc_recv[2];
    oal_uint32  ul_cnt;                 /* ���δ���������       */
    oal_uint32  ul_time_stamp;          /* ���δ�����ʱ���     */
    oal_uint32  ul_ip_obtain_stamp;     /* �ϲ��ȡIP��ַʱ��� */
    oal_uint32  ul_ip_addr_obtained;    /* IP��ַ�Ƿ��ѻ�ȡ */
}dmac_vap_roam_trigger_stru;
#endif //_PRE_WLAN_FEATURE_ROAM

/*�޸Ĵ˽ṹ����Ҫͬ��֪ͨSDT�������ϱ��޷�����*/
typedef struct
{
    oal_int16    s_signal;           /* �������հ���¼��RSSIֵ */
    oal_int16    s_free_power;       /*����*/

    oal_uint32   ul_drv_rx_pkts;     /* �����������ݰ���Ŀ */
    oal_uint32   ul_hw_tx_pkts;      /* ��������ж��ϱ����ͳɹ��ĸ��� */
    oal_uint32   ul_drv_rx_bytes;    /* ���������ֽ�����������80211ͷβ */
    oal_uint32   ul_hw_tx_bytes;     /* ��������ж��ϱ����ͳɹ��ֽ��� */
    oal_uint32   ul_tx_retries;      /*�����ش�����*/
    oal_uint32   ul_rx_dropped_misc; /*����ʧ�ܴ���*/
    oal_uint32   ul_tx_failed;     /* ����ʧ�ܴ���������ͳ������֡ */
//    oal_rate_info_stru st_txrate; /*vap��ǰ����*/
    oal_int8     c_snr_ant0;         /* ����0���ϱ���SNR */
    oal_int8     c_snr_ant1;         /* ����1���ϱ���SNR */
    oal_uint8    auc_reserv[2];

    /*ά����Ҫ���ӽ϶��ά�⣬ʹ��ά��Ԥ��������*/
#ifdef _PRE_WLAN_DFT_STAT
    /***************************************************************************
                                ���հ�ͳ��
    ***************************************************************************/

    /* ���������������ش���(�������쳣��)���ͷ�����MPDU��ͳ�� */
    oal_uint32  ul_rx_ppdu_dropped;                             /* Ӳ���ϱ���vap_id�����ͷŵ�MPDU���� */

    /* ���ͳ�ƵĽ��յ�MPDU���������������Ӧ����MACӲ��ͳ��ֵ��ͬ */
    oal_uint32  ul_rx_mpdu_total_num;                           /* ���������ϱ���������д����MPDU�ܸ��� */

    /* MPDU������д���ʱ���������ͷ�MPDU����ͳ�� */
    oal_uint32  ul_rx_ta_check_dropped;                         /* ��鷢�Ͷ˵�ַ�쳣���ͷ�һ��MPDU */
    oal_uint32  ul_rx_key_search_fail_dropped;                  /*  */
    oal_uint32  ul_rx_tkip_mic_fail_dropped;                    /* ����������statusΪ tkip mic fail�ͷ�MPDU */
    oal_uint32  ul_rx_replay_fail_dropped;                      /* �طŹ������ͷ�һ��MPDU */
    oal_uint32  ul_rx_security_check_fail_dropped;              /* ���ܼ��ʧ��*/
    oal_uint32  ul_rx_alg_process_dropped;                      /* �㷨������ʧ�� */
    oal_uint32  ul_rx_null_frame_dropped;                       /* ���յ���֡�ͷ�(֮ǰ���������Ѿ�������д���) */
    oal_uint32  ul_rx_abnormal_dropped;                         /* �����쳣����ͷ�MPDU */
    oal_uint32  ul_rx_mgmt_mpdu_num_cnt;                         /* ���յ��Ĺ���֡�Ϳ���֡ͳ��*/
    oal_uint32  ul_rx_mgmt_abnormal_dropped;                    /* ���յ�����֡�����쳣������vap����devΪ�յ� */

    /***************************************************************************
                                ���Ͱ�ͳ��
    ***************************************************************************/
    oal_uint32  ul_drv_tx_pkts;     /* ����������Ŀ������Ӳ������Ŀ */
    oal_uint32  ul_drv_tx_bytes;    /* ���������ֽ���������80211ͷβ */
    /* �������̷����쳣�����ͷŵ����ݰ�ͳ�ƣ�MSDU���� */
    oal_uint32  ul_tx_abnormal_mpdu_dropped;                    /* �쳣����ͷ�MPDU������ָvap����userΪ�յ��쳣 */
    /* ��������з��ͳɹ���ʧ�ܵ����ݰ�ͳ�ƣ�MPDU���� */
    oal_uint32  ul_tx_mpdu_succ_num;                            /* ����MPDU�ܸ��� */
    oal_uint32  ul_tx_mpdu_fail_num;                            /* ����MPDUʧ�ܸ��� */
    oal_uint32  ul_tx_ampdu_succ_num;                           /* ���ͳɹ���AMPDU�ܸ��� */
    oal_uint32  ul_tx_mpdu_in_ampdu;                            /* ����AMPDU��MPDU�����ܸ��� */
    oal_uint32  ul_tx_ampdu_fail_num;                           /* ����AMPDUʧ�ܸ��� */
    oal_uint32  ul_tx_mpdu_fail_in_ampdu;                       /* ����AMPDU��MPDU����ʧ�ܸ��� */
    oal_uint32  aul_tx_count_per_apmpdu_length[DMAC_COUNT_BY_AMPDU_LENGTH_INDEX_BUTT];/*��Բ�ͬ�ۺϳ��ȵ�֡ͳ�Ʒ��͵ĸ���*/
    oal_uint32  ul_tx_cts_fail;                                  /*����rtsʧ�ܵ�ͳ��*/
    oal_uint8   uc_tx_successive_mpdu_fail_num;                  /*��������ʧ�ܵ�ͳ��*/
    oal_uint8   uc_reserve[3];                                   /*�����ֽ�*/
#endif
    oal_uint8   _rom[4];
}dmac_vap_query_stats_stru;

typedef oal_uint8 dmac_linkloss_status_enum_uint8;

typedef struct
{
    oal_uint8                     uc_linkloss_threshold;       /*  LinkLoss����  */
    oal_uint8                     uc_link_loss;                /*  LinkLoss������ */
    oal_uint8                     auc_resv[2];
}vap_linkloss_stru;

typedef struct
{
    vap_linkloss_stru             ast_linkloss_info[WLAN_LINKLOSS_MODE_BUTT];   /* linkloss�����ṹ*/

    oal_uint8   auc_int_linkloss_threshold[WLAN_LINKLOSS_MODE_BUTT];            /* ������������beacon�����ʼֵ */
    oal_uint8   auc_min_linkloss_threshold[WLAN_LINKLOSS_MODE_BUTT];            /* ��������������Сֵ */
    oal_uint8   auc_linkloss_threshold_decr[WLAN_LINKLOSS_MODE_BUTT];           /* ���ջ��߷���ʧ�����޼�ֵ*/

    oal_uint32                                ul_dot11BeaconPeriod;             /* ��¼dot11BeaconPeriod�Ƿ�仯*/

    oal_uint8                                 uc_linkloss_times;                /* ��¼linkloss��ǰ����ֵ����������ı�������ӳbeacon�����ڱ��� */
    wlan_linkloss_mode_enum_uint8             en_linkloss_mode;                 /*  linkloss����*/
    oal_bool_enum_uint8                       en_roam_scan_done;                /* ��¼�Ƿ���������ɨ��*/
    oal_bool_enum_uint8                       en_vowifi_report;                 /* ��¼�Ƿ��ϱ�vowifi״̬�л�����*/

    oal_uint8                                 uc_allow_send_probe_req_cnt;      /* ���͹�prob req�Ĵ���*/
    oal_uint8                                 auc_resv[3];
}dmac_vap_linkloss_stru;

#ifdef _PRE_WLAN_FEATURE_ARP_OFFLOAD
#define DMAC_MAX_IPV4_ENTRIES         8
#define DMAC_MAX_IPV6_ENTRIES         8

typedef union
{
    oal_uint32                    ul_value;
    oal_uint8                     auc_value[OAL_IPV4_ADDR_SIZE];
}un_ipv4_addr;

typedef struct
{
    un_ipv4_addr        un_local_ip;
    un_ipv4_addr        un_mask;
}dmac_vap_ipv4_addr_stru;

typedef struct
{
    oal_uint8                         auc_ip_addr[OAL_IPV6_ADDR_SIZE];
}dmac_vap_ipv6_addr_stru;

typedef struct
{
    dmac_vap_ipv4_addr_stru           ast_ipv4_entry[DMAC_MAX_IPV4_ENTRIES];
    dmac_vap_ipv6_addr_stru           ast_ipv6_entry[DMAC_MAX_IPV6_ENTRIES];
}dmac_vap_ip_entries_stru;
#endif

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
typedef struct
{
    oal_uint16  aus_last_qos_seq_num[WLAN_TID_MAX_NUM];         /* ��¼��Proxy STA����Ľ�������Root Ap��ǰһ��QoS֡��seq num */
    oal_uint16  us_non_qos_seq_num;                             /* ��¼��Proxy STA����Ľ�������Root Ap��ǰһ����QOS֡��seq num */
    oal_uint8   uc_lut_idx;
    oal_uint8   auc_resv[1];
} dmac_vap_psta_stru;
#define dmac_vap_psta_lut_idx(vap)   ((vap)->st_psta.uc_lut_idx)
#endif

typedef struct
{
    oal_fsm_stru                         st_oal_fsm;                   /* ״̬�� */
    oal_bool_enum_uint8                  en_is_fsm_attached;           /* ״̬���Ƿ��Ѿ�ע�� */
    oal_uint8                            uc_20M_Verify_fail_cnt;
    oal_uint8                            uc_40M_Verify_fail_cnt;
    oal_uint8                            auc_rsv[1];
}dmac_sta_bw_switch_fsm_info_stru;

#ifdef _PRE_WLAN_FEATURE_FTM
typedef struct
{
    oal_uint64      ull_t1;
    oal_uint64      ull_t2;
    oal_uint64      ull_t3;
    oal_uint64      ull_t4;
    oal_uint8       uc_dialog_token;
    oal_uint8       uc_resv[3];
}ftm_timer_stru;

typedef struct
{
    oal_uint32                          ul_measurement_start_time;
    oal_uint8                           auc_bssid[WLAN_MAC_ADDR_LEN];
    oal_uint32                          bit_range                     :24,
                                        bit_max_range_error_exponent  :8;
    oal_uint8                           auc_reserve[1];
} ftm_range_entry_stru;

typedef struct
{
    oal_uint32                          ul_measurement_start_time;
    oal_uint8                           auc_bssid[WLAN_MAC_ADDR_LEN];
    oal_uint8                           uc_error_code;
} ftm_error_entry_stru;

typedef struct
{
    oal_uint8                           uc_range_entry_count;
    ftm_range_entry_stru                ast_ftm_range_entry[DMAX_FTM_RANGE_ENTRY_COUNT];
    oal_uint8                           uc_error_entry_count;
    ftm_error_entry_stru                ast_ftm_error_entry[DMAX_FTM_ERROR_ENTRY_COUNT];
    oal_uint8                           auc_reserve[2];
} ftm_range_report_stru;

typedef struct
{
    oal_uint16                          us_start_delay;
    oal_uint8                           auc_reserve[1];
    oal_uint8                           uc_minimum_ap_count;
    oal_uint8                           auc_bssid[DMAX_MINIMUN_AP_COUNT][WLAN_MAC_ADDR_LEN];
    oal_uint8                           auc_channel[DMAX_MINIMUN_AP_COUNT];
} ftm_range_request_stru;

typedef struct
{
    oal_void        *pst_dmac_vap;
    oal_uint8        uc_session_id;
}ftm_timeout_arg_stru;

typedef struct
{
    oal_uint16                          us_burst_cnt;                                    /*�غϸ���*/
    oal_uint8                           uc_ftms_per_burst;                               /*ÿ���غ�FTM֡�ĸ���*/
    oal_uint8                           uc_burst_duration;                               /*�غϳ���ʱ��*/

    oal_uint16                          us_burst_period;                                 /*�غ�֮��ļ������λ100ms*/
    oal_bool_enum_uint8                 en_ftm_initiator;                                /*STA that supports fine timing measurement as an initiator*/
    oal_bool_enum_uint8                 en_asap;                                         /*ָʾ as soon as posible*/

    wlan_phy_protocol_enum_uint8        en_prot_format;                                  /*ָʾ Э������*/
    wlan_bw_cap_enum_uint8              en_band_cap;                                     /*ָʾ ����*/
    oal_bool_enum_uint8                 en_lci_ie;                                       /*ָʾ Я��LCI Measurement request/response elements */
    oal_bool_enum_uint8                 en_location_civic_ie;                            /*ָʾ Я��Location Civic Measurement request/response elements */

    oal_uint8                           uc_ftm_entry_count;                              /*��Ҫ����ٸ�AP����*/
    oal_uint8                           en_report_range;
    oal_uint16                          us_partial_tsf_timer;

    oal_uint32                          ul_tsf_sync_info;
    frw_timeout_stru                    st_ftm_tsf_timer;                                /* ftm�غϿ�ʼ��ʱ�� */
    mac_channel_stru                    st_channel_ftm;                                  /* FTM���������ŵ�*/
    mac_channel_stru                    st_channel_work;                                 /* �����ŵ�*/

    oal_uint8                           auc_bssid[WLAN_MAC_ADDR_LEN];                    /* FTM����AP��BSSID*/
    oal_bool_enum_uint8                 en_iftmr;
    oal_bool_enum_uint8                 en_ftm_trigger;

    ftm_range_report_stru               st_ftm_range_report;                             /* FTM�������*/
    ftm_range_request_stru              st_ftm_range_request;                            /* FTM����Ҫ��*/

    oal_uint32                          bit_range                    :24,
                                        bit_max_range_rrror_rxponent :8;

    oal_uint8                           uc_dialog_token;
    oal_uint8                           uc_follow_up_dialog_token;
    oal_bool_enum_uint8                 en_cali;
    oal_uint8                           uc_ftms_per_burst_cmd;                          /*��������ÿ���غ�FTM֡�ĸ���*/

    oal_uint32                          ul_ftm_cali_time;
    ftm_timer_stru                      ast_ftm_timer[MAC_FTM_TIMER_CNT];
    ftm_timeout_arg_stru                st_arg;
} dmac_ftm_initiator_stru;
typedef struct
{
    oal_uint8                           uc_ftms_per_burst;                               /*ÿ���غ�FTM֡�ĸ���*/
    oal_uint8                           uc_ftms_per_burst_save;
    oal_uint16                          us_burst_cnt;                                    /*�غϸ���*/

    wlan_phy_protocol_enum_uint8        uc_prot_format;                                  /*ָʾ Э������*/
    wlan_bw_cap_enum_uint8              en_band_cap;                                     /*ָʾ ����*/
    oal_uint8                           auc_resv[2];

    mac_channel_stru                    st_channel_ftm;                                  /* FTM���������ŵ�*/

    oal_bool_enum_uint8                 en_received_iftmr;
    oal_uint8                           uc_min_delta_ftm;                                 /* ��λ100us*/
    oal_bool_enum_uint8                 en_ftm_responder;                                /*STA that supports fine timing measurement as an responder*/
    oal_bool_enum_uint8                 en_asap;                                         /*ָʾ as soon as posible*/

    frw_timeout_stru                    st_ftm_tsf_timer;                                /* ftm�غϿ�ʼ��ʱ�� */
    frw_timeout_stru                    st_ftm_all_burst;                                /* ftm�����غ�ʱ�� */

    oal_uint32                          bit_range                    :24,
                                        bit_max_range_rrror_rxponent :8;

    oal_uint8                           auc_mac_ra[WLAN_MAC_ADDR_LEN];
    oal_uint8                           uc_dialog_token;
    oal_uint8                           uc_follow_up_dialog_token;

    oal_bool_enum_uint8                 en_lci_ie;
    oal_bool_enum_uint8                 en_location_civic_ie;
    oal_bool_enum_uint8                 en_ftm_parameters;
    oal_bool_enum_uint8                 en_ftm_synchronization_information;

    oal_uint64                          ull_tod;
    oal_uint64                          ull_toa;

    oal_uint16                          us_tod_error;
    oal_uint16                          us_toa_error;

    oal_uint32                          ul_tsf;

    oal_uint8                           uc_dialog_token_ack;
    oal_uint8                           uc_burst_duration;                               /*�غϳ���ʱ��*/
    oal_uint16                          us_burst_period;                                 /*�غ�֮��ļ������λ100ms*/

    ftm_timer_stru                      ast_ftm_timer[MAC_FTM_TIMER_CNT];

    ftm_timeout_arg_stru                st_arg;
} dmac_ftm_responder_stru;

typedef struct
{
    dmac_ftm_initiator_stru            ast_ftm_init[MAX_FTM_SESSION];
    dmac_ftm_responder_stru            ast_ftm_rsp[MAX_FTM_SESSION];
}dmac_ftm_stru;
#endif

#ifdef _PRE_WLAN_FEATURE_11K
typedef struct mac_rrm_info_tag
{
    mac_action_rm_rpt_stru              *pst_rm_rpt_action;
    mac_meas_rpt_ie_stru                *pst_meas_rpt_ie;           /* Measurement Report IE Addr */
    mac_bcn_rpt_stru                    *pst_bcn_rpt_item;          /* Beacon Report Addr */
    oal_netbuf_stru                     *pst_rm_rpt_mgmt_buf;       /* Report Frame Addr for Transfer */
    mac_scan_req_stru                   *pst_scan_req;

    oal_uint8                            uc_quiet_count;
    oal_uint8                            uc_quiet_period;
    oal_mac_quiet_state_uint8            en_quiet_state;
    oal_uint8                            uc_link_dialog_token;

    oal_uint8                            uc_ori_max_reg_pwr;
    oal_uint8                            uc_local_pwr_constraint;
    oal_uint8                            uc_ori_max_pwr_flush_flag;
    oal_uint8                            uc_rsv;

    oal_int8                             c_link_tx_pwr_used;
    oal_int8                             c_link_max_tx_pwr;
    oal_uint32                           aul_act_meas_start_time[2];
    oal_uint16                           us_quiet_duration;

    oal_uint16                           us_quiet_offset;
    oal_uint16                           us_rm_rpt_action_len;      /* Report Frame Length for Transfer */

    oal_dlist_head_stru                  st_meas_rpt_list;
    mac_bcn_req_info_stru                st_bcn_req_info;
    frw_timeout_stru                     st_quiet_timer;    /* quiet��ʱ����ÿ�ν���quietʱ������quiet duration��ʱ�����˳�quiet */
    frw_timeout_stru                     st_offset_timer;   /* ���һ��tbtt�жϿ�ʼʱ����offset��ʱ����offsetʱ���ʱ�������quiet��������quiet��ʱ�� */

    oal_uint8                            _rom[4];
}mac_rrm_info_stru;
#endif //_PRE_WLAN_FEATURE_11K

#ifdef _PRE_WLAN_FEATURE_BAND_STEERING

//vap�����band steering�������ݽṹ
typedef struct{
    oal_uint8               uc_enable;              //��ʾ��vap�Ƿ���steeringģʽ��vap
    oal_uint8               auc_reserv[3];
}dmac_vap_bsd_stru;

#endif

#ifdef _PRE_WLAN_FEATURE_VOWIFI
typedef struct
{
    oal_uint8           uc_rssi_trigger_cnt;        /* ͳ���ź����������������޵Ĵ��� ��1��100��*/
    oal_uint8           auc_resv[3];
    oal_uint64          ull_rssi_timestamp_ms;     /* ʱ�����ms���� */
    oal_uint64          ull_arp_timestamp_ms;      /* arp req device��̽��ʧ��(tx 5s�����κ����ݽ���) */

    oal_uint32   ul_arp_rx_succ_pkts;  /* TX arp_reqʱ�̼�¼�ĵ�ʱrx_succ_pktsֵ */
    oal_uint32   ul_tx_failed;         /* ����ʧ�����ն����Ĵ���������ͳ������֡  */
    oal_uint32   ul_tx_total;          /* �����ܼƣ�����ͳ������֡  */

    oal_uint8    _rom[4];
}mac_vowifi_status_stru;

#endif /* _PRE_WLAN_FEATURE_VOWIFI */

/* STA ��pm����ṹ����*/
#ifdef _PRE_WLAN_FEATURE_STA_PM
typedef struct
{
    oal_uint8    en_active_null_wait :1;   /* STA����NULL֡��AP��ʾ����ACTIVE ״̬ */
    oal_uint8    en_doze_null_wait   :1;   /* STA����NULL֡��AP��ʾ����doze״̬*/
    oal_uint8    bit_resv            :6;
} dmac_vap_null_wait_stru;

#ifdef _PRE_DEBUG_MODE
typedef struct
{
    oal_uint32 ul_wmmpssta_tacdl;       /* Trigger enabled Downlink traffic exception.          */
    oal_uint32 ul_wmmpssta_dacul;        /* Delivery enabled Uplink traffic exception.             */
    oal_uint32 ul_wmmpssta_spsw;        /* Wait for service period start.                       */
    oal_uint32 ul_wmmpssta_sps;         /* Service period start                                 */
    oal_uint32 ul_wmmpssta_spe;         /* Service period end                                   */
    oal_uint32 ul_wmmpssta_trigsp;      /* Trigger service period                               */
    oal_uint32 ul_wmmpssta_trspnr;      /* Trigger service period is not required               */
}dmac_wmmps_info_stru;
#elif defined(_PRE_PSM_DEBUG_MODE)
#endif

typedef struct _mac_sta_pm_handler
{
    oal_fsm_stru                    st_oal_fsm;                                  /*����״̬��*/
    frw_timeout_stru                st_inactive_timer;                          /* ��ʱ�� */
    frw_timeout_stru                st_mcast_timer;                             /* ���չ㲥�鲥��ʱ��ʱ�� */

#ifdef _PRE_WLAN_DOWNLOAD_PM
    oal_uint32                      ul_rx_cnt ;                                 /*wifi���ع��Ĳ��԰汾�����ձ��ĵ���ͳ������*/
#endif
    oal_uint32                      ul_tx_rx_activity_cnt;                      /* ACTIVEͳ��ֵ���ɳ�ʱ����DOZE��λ */
    oal_uint32                      ul_activity_timeout;                        /* ˯�߳�ʱ��ʱ����ʱʱ�� */
    oal_uint32                      ul_ps_keepalive_cnt;                        /* STA�����״̬��keepalive����ͳ�ƽ���beacon��*/
    oal_uint32                      ul_ps_keepalive_max_num;                    /* STA�����״̬��keepalive����������beacon�� */

    oal_uint8                       uc_vap_ps_mode;                             /*  sta��ǰʡ��ģʽ */

    oal_uint8                       en_beacon_frame_wait            :1;         /* ��ʾ����beacon֡ */
    oal_uint8                       en_more_data_expected           :1;         /* ��ʾAP���и���Ļ���֡ */
    oal_uint8                       en_send_null_delay              :1;         /* �ӳٷ���NULL֡���� */
    oal_uint8                       bit_resv                        :1;
    oal_uint8                       en_direct_change_to_active      :1;         /* FASTģʽ��ֱ�ӻ��ѵ����ݰ���active״̬ */
    oal_uint8                       en_hw_ps_enable                 :1;         /* ����ȫϵͳ�͹���/Э��ջ�͹��� */
    oal_uint8                       en_ps_back_active_pause         :1;         /* ps back �ӳٷ��ͻ���null֡ */
    oal_uint8                       en_ps_back_doze_pause           :1;         /* ps back �ӳٷ���˯��null֡ */

    oal_uint8                       uc_timer_fail_doze_trans_cnt;               /* ��ʱ�����ڷ�null��dozeʧ�ܴ��� */
    oal_uint8                       uc_state_fail_doze_trans_cnt;               /* ��dozeʱ,��������������ʧ�ܼ��� */

    oal_uint8                       uc_beacon_fail_doze_trans_cnt;              /* ��beacon ͶƱ˯��ȴȴ�޷�˯��ȥ�ļ��� */
    oal_uint8                       uc_doze_event;                              /* ��¼��״̬���¼����� */
    oal_uint8                       uc_awake_event;
    oal_uint8                       uc_active_event;

#ifdef _PRE_WLAN_FEATURE_STA_UAPSD
    oal_uint8                       uc_eosp_timeout_cnt;                        /* uapsdʡ����TBTT������ */
    oal_uint8                       uc_uaspd_sp_status;                         /* UAPSD��״̬ */
#endif
    oal_uint8                       uc_doze_null_retran_cnt;
    oal_uint8                       uc_active_null_retran_cnt;

    dmac_vap_null_wait_stru         st_null_wait;                               /* STA����NULL֡�л�״̬ʱ����״̬NULL֡�ȴ�״̬�Ľṹ��*/
#ifdef _PRE_DEBUG_MODE
    dmac_wmmps_info_stru            st_wmmps_info;                              /* STA��uapsd��ά����Ϣ */
#endif
    oal_uint32                      aul_pmDebugCount[PM_MSG_COUNT];
    oal_uint32                      ul_psm_pkt_cnt;
    oal_uint8                       uc_psm_timer_restart_cnt;                   /* ����˯�߶�ʱ����count */
    oal_uint8                       uc_can_sta_sleep;                           /* Э�������е�doze,�Ƿ���ͶƱ˯�� */
    oal_uint16                      us_mcast_timeout;                           /* ���չ㲥�鲥��ʱ����ʱʱ�� */
    oal_bool_enum_uint8             en_is_fsm_attached;                         /*״̬���Ƿ��Ѿ�ע��*/
    oal_bool_enum_uint8             en_beacon_counting;
    oal_uint8                       uc_max_skip_bcn_cnt;                        /* �����������beacon���� */
    oal_uint8                       uc_tbtt_cnt_since_full_bcn;                 /* �����ϴν�������beacon��tbtt cnt���� */
    oal_uint8                       _rom[2];
} mac_sta_pm_handler_stru;
#endif

/*pm����ṹ����*/
#ifdef _PRE_WLAN_FEATURE_AP_PM
typedef struct _mac_pm_handler{
    oal_fsm_stru         st_oal_fsm;               /*����״̬��*/
    oal_uint32           ul_pwr_arbiter_id;        /*arbiter id*/
    oal_uint32           bit_pwr_sleep_en:1,       /*�Զ�˯��ʹ��*/
                         bit_pwr_wow_en:1,         /*wow����ʹ��*/
                         bit_pwr_siso_en:1,        /*�Զ���ͨ��ģʽʹ��,��û���û����������ϵ�ʱ�л�*/
                         bit_rsv:29;
    oal_uint32           ul_max_inactive_time;     /*work̬����󲻻�Ծʱ��*/
    oal_uint32           ul_inactive_time;         /*��������Ծʱ��*/
    oal_uint32           ul_user_check_count;      /*���û�������*/
    oal_uint32           ul_idle_beacon_txpower;   /*idle̬��beaocn txpower*/
    frw_timeout_stru     st_inactive_timer;        /*��Ծ��ⶨʱ��,ͬʱ����Ϊ�Ƿ����û��ļ�鶨ʱ��*/
    oal_bool_enum_uint8  en_is_fsm_attached;       /*״̬���Ƿ��Ѿ�ע��*/
    oal_uint8            _rom[4];
} mac_pm_handler_stru;
#endif

#ifdef _PRE_WLAN_FEATURE_BTCOEX
typedef struct
{
    frw_timeout_stru bt_coex_low_rate_timer;
    frw_timeout_stru bt_coex_statistics_timer;
    frw_timeout_stru bt_coex_sco_statistics_timer;
    oal_bool_enum_uint8 en_rx_rate_statistics_flag;
    oal_bool_enum_uint8 en_rx_rate_statistics_timeout;
    oal_bool_enum_uint8 en_sco_rx_rate_statistics_flag;
    oal_uint8 uc_resv[1];
} dmac_vap_btcoex_rx_statistics_stru;

typedef struct
{
    frw_timeout_stru bt_coex_priority_timer;                 /* ��ȡ�Ĵ������ڶ�ʱ�� */
    frw_timeout_stru bt_coex_occupied_timer;                 /* ��������occupied�ź��ߣ���֤WiFi����BT��ռ */
    oal_uint32 ul_ap_beacon_count;
    oal_uint32 ul_timestamp;
    oal_uint8 uc_beacon_miss_cnt;
    oal_uint8 uc_occupied_times;
    oal_uint8 uc_prio_occupied_state;
    oal_uint8 uc_linkloss_occupied_times;
    oal_uint8 uc_linkloss_index;
    oal_uint8 auc_resv[3];
} dmac_vap_btcoex_occupied_stru;

typedef struct
{
    oal_uint8                            auc_null_qosnull_frame[32];  /* null&qos null,ȡ��󳤶� */
    oal_uint16                           bit_cfg_coex_tx_vap_index     :4;     /* 03����premmpt֡���ò��� */
    oal_uint16                           bit_cfg_coex_tx_qos_null_tid  :4;
    oal_uint16                           bit_rsv                       :3;
    oal_uint16                           bit_cfg_coex_tx_peer_index    :5;
} dmac_vap_btcoex_null_preempt_stru;

typedef struct
{
    dmac_vap_btcoex_rx_statistics_stru   st_dmac_vap_btcoex_rx_statistics;
    dmac_vap_btcoex_occupied_stru        st_dmac_vap_btcoex_occupied;
    dmac_vap_btcoex_null_preempt_stru    st_dmac_vap_btcoex_null_preempt_param;
    hal_coex_hw_preempt_mode_enum_uint8  en_all_abort_preempt_type;
    oal_uint8                            _rom[4];
} dmac_vap_btcoex_stru;
#endif

#ifdef _PRE_WLAN_FEATURE_M2S
typedef struct
{
    frw_timeout_stru      m2s_delay_switch_statistics_timer;
    oal_uint16            us_rx_nss_mimo_count;
    oal_uint16            us_rx_nss_siso_count;
    oal_uint16            us_rx_nss_ucast_count;
    oal_bool_enum_uint8   en_rx_nss_statistics_start_flag;
} dmac_vap_m2s_rx_statistics_stru;

typedef struct
{
    dmac_vap_m2s_rx_statistics_stru   st_dmac_vap_m2s_rx_statistics;    /* STA��ǰ�л���ʼģʽ������Ҫ��ʱ������ͳ�Ʊ��� */
    hal_m2s_action_type_uint8         en_m2s_switch_ori_type;           /* STA��ǰ����ap���л���ʼģʽ */
    oal_bool_enum_uint8               en_action_send_state;             /* STA action֡�����Ƿ�ɹ� */
    oal_uint8                         uc_action_send_cnt;
} dmac_vap_m2s_stru;
#endif

typedef enum
{
    DMAC_BEACON_TX_POLICY_SINGLE    = 0,
    DMAC_BEACON_TX_POLICY_SWITCH    = 1,
    DMAC_BEACON_TX_POLICY_DOUBLE    = 2,
}dmac_beacon_tx_policy_enum_uint8;

#ifdef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT

#define DMAC_SENSING_BSSID_LIST_MAX_MEMBER_CNT  16

typedef enum mac_sensing_bssid_operate
{
    OAL_SENSING_BSSID_OPERATE_DEL = 0,
    OAL_SENSING_BSSID_OPERATE_ADD = 1,

    OAL_SENSING_BSSID_OPERATE_BUTT
} mac_sensing_bssid_operate_en;
typedef oal_uint8 oal_en_sensing_bssid_operate_uint8;

typedef struct mac_sensing_bssid
{
    oal_uint8  auc_mac_addr[OAL_MAC_ADDR_LEN];
    oal_en_sensing_bssid_operate_uint8  en_operation;   /** 0ɾ���� 1��� */
    oal_uint8  reserved;
} dmac_sensing_bssid_cfg_stru;

typedef struct
{
    oal_spin_lock_stru     st_lock;
    oal_dlist_head_stru    st_list_head;
    oal_uint8              uc_member_nums;
    oal_uint8              auc_reserve[3];
} dmac_sensing_bssid_list_stru;

typedef struct
{
    oal_dlist_head_stru  st_dlist;
    oal_uint8            auc_mac_addr[WLAN_MAC_ADDR_LEN];   /*��Ӧ��MAC��ַ */
    oal_int8             c_rssi;
    oal_uint8            uc_reserved;
    oal_uint32           ul_timestamp;
} dmac_sensing_bssid_list_member_stru;

typedef struct
{
    oal_uint8            auc_mac_addr[WLAN_MAC_ADDR_LEN];   /*��Ӧ��MAC��ַ */
    oal_int8             c_rssi;
    oal_uint8            uc_reserved;
    oal_uint32           ul_timestamp;
} dmac_query_sensing_bssid_stru;

#endif

typedef struct
{
#ifdef _PRE_WLAN_FEATURE_M2S
    dmac_vap_m2s_stru     st_dmac_vap_m2s;
#else
    oal_uint8             auc_resv[4];
#endif
}dmac_vap_rom_stru;
extern dmac_vap_rom_stru g_dmac_vap_rom[];

/* dmac vap */
typedef struct dmac_vap_tag
{
    mac_vap_stru                     st_vap_base_info;                                  /* ����VAP��Ϣ */

    oal_uint32                       ul_active_id_bitmap;                               /* ��Ծuser��bitmap */

    oal_uint8                       *pauc_beacon_buffer[DMAC_VAP_BEACON_BUFFER_BUTT];   /* VAP�¹�����beacon֡ */
    oal_uint8                        uc_beacon_idx;                                     /* ��ǰ����Ӳ��beacon֡����ֵ */
    oal_uint8                        uc_rsv1;
    oal_uint16                       us_beacon_len;                                     /* beacon֡�ĳ��� */

    hal_to_dmac_vap_stru            *pst_hal_vap;                                       /* hal vap�ṹ */
    hal_to_dmac_device_stru         *pst_hal_device;                                    /* hal device�ṹ��������λ�ȡ */
    hal_to_dmac_chip_stru           *pst_hal_chip;                                      /* hal chip�ṹ��������λ�ȡ */

    dmac_vap_linkloss_stru           st_linkloss_info;                                  /* linkloss���������Ϣ */

    oal_bool_enum_uint8              en_is_host_vap;                                    /* TRUE:��VAP��FALSE:��VAP */
    oal_uint8                        uc_default_ant_bitmap;                             /* Ĭ���������bitmap, ������д���������� */
    oal_uint8                        uc_sw_retry_limit;
    oal_uint8                        en_multi_user_multi_ac_flag;			            /* ���û������ȼ�ʱ�Ƿ�ʹ��ӵ������*/

    oal_traffic_type_enum_uint8      uc_traffic_type;                                   /* ҵ�����ͣ��Ƿ��ж��û������ȼ� */
    oal_uint8                        uc_sta_pm_open_by_host;                            /* sta �͹���״̬: HOST���Ƿ���˵͹��� */
    oal_uint8                        uc_cfg_pm_mode;                                    /* �ֶ�������ĵ͹���ģʽ */
    oal_uint8                        uc_sta_pm_close_status;                            /* sta �͹���״̬, �������ģ��ĵ͹��Ŀ�����Ϣ */


#ifdef _PRE_WLAN_FEATURE_WEB_CFG_FIXED_RATE
    hal_tx_txop_alg_stru             st_tx_alg_vht;                                     /* VHT��������֡���Ͳ��� */
    hal_tx_txop_alg_stru             st_tx_alg_ht;                                      /* HT��������֡���Ͳ��� */
    hal_tx_txop_alg_stru             st_tx_alg_11ag;                                    /* 11a/g��������֡���Ͳ��� */
    hal_tx_txop_alg_stru             st_tx_alg_11b;                                     /* 11b��������֡���Ͳ��� */

    union
    {
        oal_uint8                    uc_mode_param_valid;                               /* �Ƿ�������ض�ģʽ�ĵ�������֡����������Ч(0=��, ����0=��) */
        struct{
            oal_uint8                bit_vht_param_vaild  : 1;                          /* VHT��������֡���������Ƿ���Ч(0=����Ч, 1=��Ч) */
            oal_uint8                bit_ht_param_vaild   : 1;                          /* HT��������֡���������Ƿ���Ч(0=����Ч, 1=��Ч) */
            oal_uint8                bit_11ag_param_vaild : 1;                          /* 11a/g��������֡���������Ƿ���Ч(0=����Ч, 1=��Ч) */
            oal_uint8                bit_11b_param_vaild  : 1;                          /* 11b��������֡���������Ƿ���Ч(0=����Ч, 1=��Ч) */
            oal_uint8                bit_reserve          : 4;
        }st_spec_mode;
    }un_mode_valid;
    oal_uint8                        auc_resv1[3];
#endif

    hal_tx_txop_alg_stru             st_tx_alg;                                         /* ��������֡���Ͳ��� */
    hal_tx_txop_alg_stru             st_tx_data_mcast;                                  /* �鲥����֡���� */
    hal_tx_txop_alg_stru             st_tx_data_bcast;                                  /* �㲥����֡����*/
    hal_tx_txop_alg_stru             ast_tx_mgmt_ucast[WLAN_BAND_BUTT];                 /* ��������֡����*/
    hal_tx_txop_alg_stru             ast_tx_mgmt_bmcast[WLAN_BAND_BUTT];                /* �鲥���㲥����֡����*/

    oal_void                        *p_alg_priv;                                        /* VAP�����㷨˽�нṹ�� */

    oal_uint8                       *puc_tim_bitmap;                                    /* ���ر����tim_bitmap��APģʽ��Ч */
    oal_uint8                        uc_tim_bitmap_len;
    oal_uint8                        uc_ps_user_num;                                    /* ���ڽ���ģʽ���û�����Ŀ��APģʽ��Ч */
    oal_uint8                        uc_dtim_count;
    oal_uint8                        uc_uapsd_max_depth;                                /* U-APSD���ܶ��е�������*/
    dmac_beacon_tx_policy_enum_uint8 en_beacon_tx_policy;                               /* beacon���Ͳ���, 0-��ͨ��, 1-˫ͨ������(�����),2-˫ͨ��(�����)*/
    oal_bool_enum_uint8              en_dtim_ctrl_bit0;                                 /* ���ڱ�ʾDTIM Control�ֶεı���0�Ƿ���1�� */
    /* �ش��������� */
    oal_uint8                        bit_bw_cmd:1,                                      /* �Ƿ�ʹ���������ݴ�������� 0:No  1:Yes */
                                     bit_beacon_timeout_times:7;                        /* sta�ȴ�beacon��ʱ���� */

#if defined(_PRE_WLAN_FEATURE_AP_PM) || defined(_PRE_WLAN_FEATURE_STA_PM)
    oal_netbuf_stru                 *pst_wow_probe_resp;                                /* wowʹ��ʱ,׼����probe response֡*/
    oal_netbuf_stru                 *pst_wow_null_data;                                 /* wowʹ��ʱ,׼����null data֡,STAģʽʱ����*/
    oal_uint16                       us_wow_probe_resp_len;
    oal_uint8                        auc_resv2[1];
#endif
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    dmac_vap_psta_stru               st_psta;
#endif

    oal_uint32                       ul_obss_scan_timer_remain;       /* 02���ʱ����ʱ65s, OBSSɨ�趨ʱ������Ϊ�����ӣ�ͨ����������ʵ�ִ�ʱ��*/
    oal_uint8                        uc_obss_scan_timer_started;
    oal_uint8                        uc_bcn_tout_max_cnt;             /* beacon�����ղ������˯�ߴ��� */
#ifdef _PRE_WLAN_FEATURE_STA_PM
    oal_uint8                        uc_null_frm_ofdm_succ_cnt;
    oal_uint8                        uc_null_frm_cnt;
#else
    oal_uint8                        uac_resv3[2];
#endif  /* _PRE_WLAN_FEATURE_STA_PM */

    wlan_channel_bandwidth_enum_uint8 en_40M_bandwidth;               /* ��¼ap���л���20M֮ǰ�Ĵ��� */
    oal_uint8                         uc_ps_poll_pending;
#ifdef _PRE_WLAN_FEATURE_MAC_PARSE_TIM
    oal_uint16                        us_tim_pos;                     /* beacon֡��TIM IE���Payload��ƫ��, ����MAC���� */
#else
    oal_uint8                         uac_resv4[2];
#endif

    /* ��������ʹ�� */
#ifdef _PRE_WLAN_FEATURE_ALWAYS_TX
    oal_uint8                        uc_protocol_rate_dscr;           /* ������������Э��������λ���ֵ�����ڳ���ģʽ�¸���֡�� */
    oal_uint8                        uc_bw_flag;                      /* ������������40M��־ */
    oal_uint8                        uc_short_gi;                     /* short gi�Ƿ�ʹ�� */
#endif
#ifdef _PRE_WLAN_FEATURE_TSF_SYNC
    oal_uint8                        uc_beacon_miss_cnt;              /* beacon miss ���� */
#else
    oal_uint8                        uac_resv5[1];
#endif

#if(_PRE_WLAN_FEATURE_PMF == _PRE_PMF_HW_CCMP_BIP)
    oal_uint32                       ul_user_pmf_status;              /* ��¼��vap��user pmfʹ�ܵ������������Ӳ��vap�Ƿ�򿪼ӽ��ܿ��� */
#endif

#if defined (_PRE_WLAN_FEATURE_STA_PM)
    mac_sta_pm_handler_stru          st_sta_pm_handler;               /* sta��pm����ṹ���� */
#endif

#if defined(_PRE_WLAN_FEATURE_AP_PM)
    mac_pm_handler_stru              st_pm_handler;                   /* ap��pm����ṹ���� */
#endif
    /*ͳ����Ϣ+��Ϣ�ϱ������ֶΣ��޸�����ֶΣ������޸�SDT���ܽ�����ȷ*/
    dmac_vap_query_stats_stru        st_query_stats;

    hal_to_dmac_vap_stru            *pst_p2p0_hal_vap;                /* p2p0 hal vap�ṹ */
#ifdef _PRE_WLAN_FEATURE_P2P
    mac_cfg_p2p_noa_param_stru       st_p2p_noa_param;
    mac_cfg_p2p_ops_param_stru       st_p2p_ops_param;
#endif

#if 0 //�����߼�������vap�������ձ�hal device�����������ڱ�hal device����mimoҲ��siso��vap�ĳ���ʱ���ٷſ������
    oal_uint8                        uc_vap_tx_chain;                                /* Ĭ��ʹ�õķ���ͨ������������֡����ʼ��ʹ�ã�ҵ������TXCS�㷨��д���������ݹ���֡������ʼ��ֵ��д */
    oal_uint8                        uc_vap_single_tx_chain;                         /* ����֡��Ĭ��ʹ�õ��ĸ���ͨ�� */
#endif

#ifdef _PRE_WLAN_FEATURE_ARP_OFFLOAD
    oal_switch_enum_uint8             en_arp_offload_switch;         /* ARP offload�Ŀ��� */
    oal_uint8                         auc_resv9[1];
    dmac_vap_ip_entries_stru         *pst_ip_addr_info;              /* Host��IPv4��IPv6��ַ */
#endif

#ifdef _PRE_WLAN_FEATURE_TSF_SYNC
    oal_uint16                          us_sync_tsf_value;
    oal_uint64                          ul_old_beacon_timestamp;
#endif
#ifdef _PRE_WLAN_FEATURE_ROAM
    dmac_vap_roam_trigger_stru          st_roam_trigger;
#endif  //_PRE_WLAN_FEATURE_ROAM
#ifdef _PRE_WLAN_FEATURE_11K
    mac_rrm_info_stru                  *pst_rrm_info;
#endif //_PRE_WLAN_FEATURE_11K

#ifdef _PRE_WLAN_FEATURE_FTM
    dmac_ftm_stru                           *pst_ftm;
#endif

#ifdef _PRE_WLAN_FEATURE_BTCOEX
    dmac_vap_btcoex_stru                st_dmac_vap_btcoex;
#endif

#ifdef _PRE_WLAN_FEATURE_VOWIFI
    //mac_vowifi_status_stru             st_vowifi_status;
    mac_vowifi_status_stru              *pst_vowifi_status;
#endif /* _PRE_WLAN_FEATURE_VOWIFI */

    oal_uint16                      us_ext_tbtt_offset;             /* �ⲿtbtt offset����ֵ*/
    oal_uint16                      us_in_tbtt_offset;              /* �ڲ�tbtt offset����ֵ*/
    oal_uint16                      us_beacon_timeout;              /* beacon timeout����ֵ*/
    oal_uint8                       uc_psm_dtim_period;             /* ʵ�ʲ��õ�dtim period*/
    oal_uint8                       uc_psm_auto_dtim_cnt;           /* �Զ�dtim�ļ�����*/
    oal_uint16                      us_psm_listen_interval;         /* ʵ�ʲ��õ�listen_interval*/

    oal_bool_enum_uint8             en_non_erp_exist;               /* staģʽ�£��Ƿ���non_erp station */
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
    oal_uint8                       uc_sta_keepalive_cnt;           /* staģʽ�£���ʱ�����ڼ���*/
#endif

#ifdef _PRE_WLAN_FEATURE_TXBF_HW
    oal_uint8                        *puc_vht_bfee_buff;            /* ����txbf vht soundingʹ�õ�buffer,vap��� */
#endif
    frw_timeout_stru                 st_obss_aging_timer;           /* OBSS�����ϻ���ʱ�� */
#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
    frw_timeout_stru                 st_40M_recovery_timer;        /* 40M�ָ���ʱ�� */
#endif

#ifdef _PRE_WLAN_FEATURE_11K
    oal_uint8                        bit_bcn_table_switch: 1;
    oal_uint8                        bit_11k_enable      : 1;
    oal_uint8                        bit_11v_enable      : 1;
    oal_uint8                        bit_rsv1            : 5;
#else
    oal_uint8                        auc_resv11[1];
#endif
#ifdef _PRE_WLAN_FEATURE_11R
    oal_uint8                        bit_11r_enable      : 1;
    oal_uint8                        bit_rsv4            : 7;
#else
    oal_uint8                        auc_resv12[1];
#endif
    oal_switch_enum_uint8            en_auth_received;           /* ������auth */
    oal_switch_enum_uint8            en_assoc_rsp_received;      /* ������assoc */

    hal_tx_dscr_queue_header_stru    ast_tx_dscr_queue_fake[HAL_TX_QUEUE_NUM];

#ifdef _PRE_WLAN_11K_STAT
    dmac_stat_frm_rpt_stru           *pst_stat_frm_rpt;
    dmac_stat_count_tid_stru         *pst_stat_count_tid;
    dmac_stat_count_stru             *pst_stat_count;
    dmac_stat_tsc_rpt_stru           *pst_stat_tsc_rpt;
    dmac_stat_tid_tx_delay_stru      *pst_stat_tid_tx_delay;
    dmac_stat_cap_flag_stru           st_stat_cap_flag;
#endif

#ifdef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT
    dmac_sensing_bssid_list_stru     st_sensing_bssid_list;
#endif
#ifdef _PRE_WLAN_FEATURE_QOS_ENHANCE
    frw_timeout_stru                  st_qos_enhance_timer;  /* qos_enhance���¶�ʱ�� */
#endif
    hal_vap_pow_info_stru             st_vap_pow_info;       /* VAP��������Ϣ�ṹ�� */
    oal_uint8                         *pst_sta_bw_switch_fsm;  /* �����л�״̬�� */

    /* ROM������Դ��չָ�� */
    oal_void                           *_rom;
}dmac_vap_stru;


/*****************************************************************************
  8 UNION����
*****************************************************************************/


/*****************************************************************************
  9 OTHERS����
*****************************************************************************/

OAL_STATIC OAL_INLINE wlan_vap_mode_enum_uint8  dmac_vap_get_bss_type(mac_vap_stru *pst_vap)
{
    return pst_vap->en_vap_mode;
}

#ifdef _PRE_WLAN_FEATURE_PROXYSTA

OAL_STATIC OAL_INLINE oal_void dmac_psta_init_vap(dmac_vap_stru *pst_dmac_vap)
{
    OAL_MEMZERO(&pst_dmac_vap->st_psta, OAL_SIZEOF(pst_dmac_vap->st_psta));
}

OAL_STATIC  OAL_INLINE  oal_void  dmac_psta_update_lut_range(mac_device_stru *pst_dev, dmac_vap_stru *pst_dmac_vap, oal_uint16 *us_start, oal_uint16 *us_stop)
{
    mac_vap_stru *pst_vap = &pst_dmac_vap->st_vap_base_info;

    if (mac_is_proxysta_enabled(pst_dev))
    {
        if(mac_vap_is_vsta(pst_vap))
        {
            *us_start = dmac_vap_psta_lut_idx(pst_dmac_vap);
            *us_stop  = dmac_vap_psta_lut_idx(pst_dmac_vap) + 1; // 32 is not valid for proxysta
        }
        else
        {
            *us_start = 0;
            *us_stop  = HAL_PROXYSTA_MAX_BA_LUT_SIZE; // hardware spec
        }
    }
    // else do nothing
}

#endif

OAL_STATIC OAL_INLINE oal_uint8 dmac_vap_get_hal_device_id(dmac_vap_stru *pst_dmac_vap)
{
    return pst_dmac_vap->pst_hal_vap->uc_device_id;
}


OAL_STATIC OAL_INLINE oal_void dmac_vap_set_hal_device_id(dmac_vap_stru *pst_dmac_vap, oal_uint8 uc_hal_dev_id)
{
    pst_dmac_vap->pst_hal_vap->uc_device_id = uc_hal_dev_id;
}


OAL_STATIC OAL_INLINE hal_to_dmac_device_stru *dmac_user_get_hal_device(mac_user_stru *pst_mac_user)
{
    dmac_vap_stru       *pst_dmac_vap;

    pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_mac_user->uc_vap_id);
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        return OAL_PTR_NULL;
    }

    return DMAC_VAP_GET_HAL_DEVICE(pst_dmac_vap);
}


OAL_STATIC OAL_INLINE hal_to_dmac_chip_stru *dmac_user_get_hal_chip(mac_user_stru *pst_mac_user)
{
    dmac_vap_stru       *pst_dmac_vap;

    pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_mac_user->uc_vap_id);
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        return OAL_PTR_NULL;
    }

    return DMAC_VAP_GET_HAL_CHIP(pst_dmac_vap);
}

/*****************************************************************************
  10 ��������
*****************************************************************************/
extern oal_uint8 dmac_sta_bw_fsm_get_current_state(mac_vap_stru *pst_mac_vap);
extern oal_void dmac_sta_bw_switch_fsm_attach(dmac_vap_stru *pst_dmac_vap);
extern oal_void dmac_sta_bw_switch_fsm_detach(dmac_vap_stru *pst_dmac_vap);
extern oal_void dmac_sta_bw_switch_fsm_init(dmac_vap_stru  *pst_dmac_vap);
extern oal_uint8 dmac_sta_bw_switch_need_new_verify(dmac_vap_stru *pst_dmac_vap, wlan_bw_cap_enum_uint8  en_bw_becaon_new);
extern oal_uint32 dmac_sta_bw_switch_fsm_post_event(dmac_vap_stru* pst_dmac_vap, oal_uint16 us_type, oal_uint16 us_datalen, oal_uint8* pst_data);
extern wlan_bw_cap_enum_uint8 dmac_sta_bw_rx_assemble_to_bandwith(hal_channel_assemble_enum_uint8 uc_bw);
extern oal_uint32  dmac_vap_init(
                       dmac_vap_stru              *pst_vap,
                       oal_uint8                   uc_chip_id,
                       oal_uint8                   uc_device_id,
                       oal_uint8                   uc_vap_id,
                       mac_cfg_add_vap_param_stru *pst_param);
extern oal_uint32  dmac_vap_init_tx_frame_params(dmac_vap_stru *pst_dmac_vap, oal_bool_enum_uint8  en_mgmt_rate_init_flag);
extern oal_void  dmac_vap_init_tx_mgmt_rate(dmac_vap_stru *pst_dmac_vap, hal_tx_txop_alg_stru *pst_tx_mgmt_cast);
extern oal_uint32  dmac_vap_init_tx_ucast_data_frame(dmac_vap_stru *pst_dmac_vap);
extern oal_uint32  dmac_vap_sta_reset(dmac_vap_stru *pst_dmac_vap);
extern oal_uint32  mac_vap_pause_tx(mac_vap_stru *pst_vap);
extern oal_uint32  mac_vap_resume_tx(mac_vap_stru *pst_vap);
extern oal_void  dmac_vap_pause_tx(mac_vap_stru *pst_mac_vap);

extern oal_void  dmac_vap_pause_tx_by_chl(mac_device_stru *pst_device, mac_channel_stru *pst_src_chl);
extern oal_void  dmac_vap_resume_tx_by_chl(mac_device_stru *pst_device, hal_to_dmac_device_stru *pst_hal_device,  mac_channel_stru *pst_dst_channel);
extern oal_void  dmac_vap_update_bi_from_hw(mac_vap_stru *pst_mac_vap);
extern oal_void  dmac_vap_init_tx_data_rate_ucast(dmac_vap_stru *pst_dmac_vap,oal_uint8 uc_protocol_mode, oal_uint8 uc_legacy_rate);
extern oal_uint32  dmac_vap_is_in_p2p_listen(mac_vap_stru *pst_mac_vap);
#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
#ifdef _PRE_WLAN_FEATURE_VOWIFI
extern void dmac_vap_vowifi_init(dmac_vap_stru *pst_dmac_vap);
extern void dmac_vap_vowifi_exit(dmac_vap_stru *pst_dmac_vap);
#endif /* _PRE_WLAN_FEATURE_VOWIFI */
#ifdef _PRE_WLAN_FEATURE_DBAC
extern oal_void dmac_vap_restart_dbac(dmac_vap_stru  *pst_dmac_vap);
#endif
extern oal_void  dmac_one_packet_send_null_data(mac_device_stru *pst_mac_device, mac_vap_stru *pst_mac_vap, oal_bool_enum_uint8 en_ps);
#endif
extern oal_void dmac_vap_down_notify(mac_vap_stru *pst_down_vap);
extern oal_uint32  dmac_vap_fake_queue_empty_assert(mac_vap_stru *pst_mac_vap, oal_uint32 ul_file_id);
extern oal_uint32  dmac_vap_clear_fake_queue(mac_vap_stru  *pst_mac_vap);
extern oal_uint32  dmac_vap_save_tx_queue(mac_vap_stru *pst_mac_vap);
extern oal_uint32  dmac_vap_restore_tx_queue(mac_vap_stru *pst_mac_vap);
extern oal_bool_enum_uint8  dmac_vap_is_fakeq_empty(mac_vap_stru *pst_mac_vap);
extern oal_void   dmac_vap_update_snr_info(dmac_vap_stru *pst_dmac_vap, dmac_rx_ctl_stru *pst_rx_ctrl, mac_ieee80211_frame_stru *pst_frame_hdr);
extern oal_void dmac_vap_work_set_channel(dmac_vap_stru *pst_dmac_vap);
#ifdef _PRE_WLAN_FEATURE_DBDC
extern oal_uint32  dmac_vap_change_hal_dev(mac_vap_stru *pst_shift_vap, mac_dbdc_debug_switch_stru *pst_dbdc_debug_switch);
extern oal_void dmac_vap_dbdc_start(mac_device_stru *pst_mac_device, mac_vap_stru *pst_mac_vap);
extern oal_void dmac_vap_dbdc_stop(mac_device_stru *pst_mac_device, mac_vap_stru *pst_down_vap);
extern oal_uint32 dmac_vap_change_hal_dev_before_up(mac_vap_stru *pst_shift_vap, oal_void *pst_dmac_device);
extern oal_uint32  dmac_up_vap_change_hal_dev(mac_vap_stru *pst_shift_mac_vap);
extern oal_bool_enum_uint8 dmac_dbdc_channel_check(mac_channel_stru *pst_channel1, mac_channel_stru *pst_channel2);

#endif
#ifdef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT
extern oal_void dmac_vap_init_sensing_bssid_list(dmac_vap_stru *pst_dmac_vap);
extern oal_void dmac_vap_clear_sensing_bssid_list(dmac_vap_stru *pst_dmac_vap);
extern oal_uint32 dmac_vap_update_sensing_bssid_list(mac_vap_stru *pst_mac_vap, dmac_sensing_bssid_cfg_stru *pst_sensing_bssid);
#endif
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of dmac_vap.h */
