

#ifndef __DMAC_USER_H__
#define __DMAC_USER_H__

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
#include "dmac_ext_if.h"

#include "mac_user.h"
#include "mac_resource.h"
#include "dmac_tid.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_USER_H

/*****************************************************************************
  2 �궨��
*****************************************************************************/


#ifdef _PRE_WLAN_DFT_STAT
#define DMAC_UAPSD_STATS_INCR(_member)    ((_member)++)
#define DMAC_PSM_STATS_INCR(_member)      ((_member)++)
#else
#define DMAC_UAPSD_STATS_INCR(_member)
#define DMAC_PSM_STATS_INCR(_member)
#endif
#if (defined(_PRE_PRODUCT_ID_HI110X_DEV))
#define   DMAC_USER_STATS_PKT_INCR(_member, _cnt)            ((_member) += (_cnt))
#else
#define   DMAC_USER_STATS_PKT_INCR(_member, _cnt)            ((_member) += (_cnt))
#endif


#define DMAC_COMPATIBILITY_PKT_NUM_LIMIT 2000

#define DMAC_GET_USER_SUPPORT_VHT(_pst_user)    \
        ((1 == (_pst_user)->st_vht_hdl.en_vht_capable)? OAL_TRUE : OAL_FALSE)

#define DMAC_GET_USER_SUPPORT_HT(_pst_user)    \
        ((1 == (_pst_user)->st_ht_hdl.en_ht_capable)? OAL_TRUE : OAL_FALSE)

#define MAC_GET_DMAC_USER(_pst_mac_user)    ((dmac_user_stru *)_pst_mac_user)
#define DMAC_USER_GET_POW_INFO(_pst_mac_user)    &(((dmac_user_stru *)_pst_mac_user)->st_user_pow_info)
#define DMAC_USER_GET_POW_TABLE(_pst_mac_user)    (((dmac_user_stru *)_pst_mac_user)->st_user_pow_info).pst_rate_pow_table
#define DMAC_INVALID_USER_LUT_INDEX     (WLAN_ACTIVE_USER_MAX_NUM)  /* ��Ч��user lut index */

/*****************************************************************************
  3 ö�ٶ���
*****************************************************************************/
typedef enum
{
    DMAC_USER_PS_STATE_ACTIVE   = 0,
    DMAC_USER_PS_STATE_DOZE     = 1,

    DMAC_USER_PS_STATE_BUTT
}dmac_user_ps_state_enum;
typedef oal_uint8 dmac_user_ps_state_enum_uint8;

typedef enum
{
    DMAC_USER_STATE_PS,
    DMAC_USER_STATE_ACTIVE,

    DMAC_USER_STATE_BUTT
}dmac_user_state_enum;
typedef oal_uint8 dmac_user_state_enum_uint8;

typedef enum {
    PSM_QUEUE_TYPE_NORMAL,
    PSM_QUEUE_TYPE_IMPORTANT,

    PSM_QUEUE_TYPE_BUTT
}psm_queue_type_enum;
typedef oal_uint8 psm_queue_type_enum_uint8;

#ifdef _PRE_WLAN_FEATURE_11V
/* 11VAP��״̬�� */
typedef enum
{
    DMAC_11V_AP_STATUS_INIT             = 0,        // ��ʼ״̬
    DMAC_11V_AP_STATUS_WAIT_RSP            ,        // �ȴ�����Response

    DMAC_11V_AP_STATUS_BUTT
}dmac_11v_ap_status_enum;
typedef oal_uint8 dmac_11v_ap_status_enum_uint8;
#endif

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

typedef struct
{
    oal_uint32  ul_uapsd_tx_enqueue_count;              /*dmac_uapsd_tx_enqueue���ô���*/
    oal_uint32  ul_uapsd_tx_dequeue_count;              /* ����֡����ͳ�� */
    oal_uint32  ul_uapsd_tx_enqueue_free_count;         /*��ӹ�����MPDU���ͷŵĴ�����һ�ο����ͷŶ��MPDU*/
    oal_uint32  ul_uapsd_rx_trigger_in_sp;              /*trigger��鷢�ִ���SP�еĴ���*/
    oal_uint32  ul_uapsd_rx_trigger_state_trans;        /*trigger����з�����Ҫ״̬ת���Ĵ���*/
    oal_uint32  ul_uapsd_rx_trigger_dup_seq;            /*trigger֡���ظ�֡�ĸ���*/
    oal_uint32  ul_uapsd_send_qosnull;                  /*����Ϊ�գ�����qos null data�ĸ���*/
    oal_uint32  ul_uapsd_send_extra_qosnull;            /*���һ��Ϊ����֡�����Ͷ���qosnull�ĸ���*/
    oal_uint32  ul_uapsd_process_queue_error;           /*���д�������г���Ĵ���*/
    oal_uint32  ul_uapsd_flush_queue_error;             /*flush���д�������г���Ĵ���*/
    oal_uint32  ul_uapsd_send_qosnull_fail;             /* ����qosnullʧ�ܴ��� */
}dmac_usr_uapsd_statis_stru;

typedef struct
{
    oal_uint32  ul_psm_enqueue_succ_cnt;                  /* psm���гɹ����֡���� */
    oal_uint32  ul_psm_enqueue_fail_cnt;                  /* psm�������ʧ�ܱ��ͷŵ�֡���� */
    oal_uint32  ul_psm_dequeue_fail_cnt;                  /* psm���г���ʧ�ܵ�֡���� */
    oal_uint32  ul_psm_dequeue_succ_cnt;                  /* psm���г��ӳɹ���֡���� */
    oal_uint32  ul_psm_send_data_fail_cnt;                /* psm���г��ӵ�����֡����ʧ�ܸ��� */
    oal_uint32  ul_psm_send_mgmt_fail_cnt;                /* psm���г��ӵĹ���֡����ʧ�ܸ��� */
    oal_uint32  ul_psm_send_null_fail_cnt;                /* AP����null dataʧ�ܵĴ��� */
    oal_uint32  ul_psm_resv_pspoll_send_null;             /* AP�յ��û���pspoll�����Ƕ�����û�л���֡�Ĵ��� */
    oal_uint32  ul_psm_rsp_pspoll_succ_cnt;               /* AP�յ��û���pspoll�����ͻ���֡�ɹ��Ĵ��� */
    oal_uint32  ul_psm_rsp_pspoll_fail_cnt;               /* AP�յ��û���pspoll�����ͻ���֡ʧ�ܵĴ��� */
}dmac_user_psm_stats_stru;


typedef struct
{
    oal_spin_lock_stru          st_lock_uapsd;                      /*uapsd������*/
    oal_netbuf_head_stru        st_uapsd_queue_head;                /*uapsd���ܶ���ͷ*/
    oal_atomic                  uc_mpdu_num;                        /*��ǰ���ܶ������mpdu����*/
    oal_uint16                  us_uapsd_trigseq[WLAN_WME_AC_BUTT]; /*���һ��trigger֡��sequence*/
    dmac_usr_uapsd_statis_stru *pst_uapsd_statis;                   /*������ͳ��ά����Ϣ*/
}dmac_user_uapsd_stru;

typedef struct
{
    oal_spin_lock_stru          st_lock_ps;                   /* �Զ��к�ul_mpdu_num����ʱ��Ҫ���� */
    oal_netbuf_head_stru        st_ps_queue_head;             /* �û����ܻ������ͷ */
    oal_atomic                  uc_mpdu_num;                  /* �û����ܻ���������Ѵ��ڵ�mpdu���� */
    oal_bool_enum_uint8         en_is_pspoll_rsp_processing;  /* TURE:��ǰ��pspoll���ڴ���FALSE:��ǰû��pspoll���ڴ���*/
    oal_uint8                   uc_ps_time_count;
    oal_uint8                   auc_resv[2];
    dmac_user_psm_stats_stru   *pst_psm_stats;
}dmac_user_ps_stru;
typedef struct
{
    oal_int32    l_signal;
    oal_uint32   ul_drv_rx_pkts;     /* ��������(Ӳ���ϱ�������rx���������ɹ���֡)��Ŀ������ͳ������֡ */
    oal_uint32   ul_hw_tx_pkts;      /* ��������ж��ϱ����ͳɹ��ĸ��� ������ͳ������֡ */
    oal_uint32   ul_drv_rx_bytes;    /* ���������ֽ�����������80211ͷβ */
    oal_uint32   ul_hw_tx_bytes;     /* ��������ж��ϱ����ͳɹ��ֽ��� */
    oal_uint32   ul_tx_retries;      /* �����ش����� */
    oal_uint32   ul_rx_dropped_misc; /* ����ʧ��(����������֡)���� */
    oal_uint32   ul_tx_failed;       /* ����ʧ�����ն����Ĵ���������ͳ������֡ */

    oal_uint32   ul_hw_tx_failed;    /* ��������ж��ϱ�����ʧ�ܵĸ���������ͳ������֡ */
    oal_uint32   ul_tx_total;        /* �����ܼƣ�����ͳ������֡ */
#if (defined(_PRE_PRODUCT_ID_HI110X_DEV))
    oal_uint32   ul_rx_replay;       /* �����ط�֡���� */
    oal_uint32   ul_rx_replay_droped;    /* �����ط�֡�����˸��� */
#endif

    oal_uint8    _rom[4];
}dmac_user_query_stats_stru;

/* ���ƽ��rssiͳ����Ϣ�ṹ�� */
typedef struct
{
    oal_int32       l_tx_rssi;                  /* ��¼ACK RSSI���ۼ�ֵ */
    oal_int32       l_rx_rssi;                  /* ��¼����RSSI���ۼ�ֵ */
    oal_uint16      us_tx_rssi_stat_count;      /* ����ƽ��rssiͳ�Ƶķ�����Ŀ */
    oal_uint16      us_rx_rssi_stat_count;      /* ����ƽ��rssiͳ�Ƶķ�����Ŀ */
}dmac_rssi_stat_info_stru;

/* ���ƽ������ͳ����Ϣ�ṹ�� */
typedef struct
{
    oal_uint32      ul_tx_rate;                 /* ��¼�������ʵ��ۼ�ֵ */
    oal_uint32      ul_rx_rate;                 /* ��¼�������ʵ��ۼ�ֵ */
    oal_uint16      us_tx_rate_stat_count;      /* ����ƽ��rateͳ�Ƶķ�����Ŀ */
    oal_uint16      us_rx_rate_stat_count;      /* ����ƽ��rateͳ�Ƶķ�����Ŀ */
}dmac_rate_stat_info_stru;

#ifdef _PRE_WLAN_11K_STAT
typedef struct
{
    oal_uint64      ull_tx_bytes;                   /*��¼�������������ۼ�ֵ*/
    oal_uint64      ull_rx_bytes;                   /*��¼�������������ۼ�ֵ*/
    oal_uint32      ul_tx_thrpt_stat_count;        /* ����ƽ��������ͳ�Ƶķ�����Ŀ */
    oal_uint32      ul_rx_thrpt_stat_count;        /* ����ƽ��������ͳ�Ƶ��հ���Ŀ */
}dmac_thrpt_stat_info_stru;

#endif

typedef struct
{
    /* �û�������Ϣ */
    wlan_protocol_enum_uint8 en_protocol;                        /* Э��ģʽ */
    hal_channel_assemble_enum_uint8 en_bandwidth;                       /* �������� */

    /* �㷨�õ��ĸ�����־λ */
    oal_bool_enum_uint8 en_dmac_rssi_stat_flag;             /* �Ƿ����ƽ��rssiͳ�� */
    oal_bool_enum_uint8 en_dmac_rate_stat_flag;             /* �Ƿ����ƽ������ͳ�� */

    dmac_rssi_stat_info_stru st_dmac_rssi_stat_info;             /* ���ƽ��rssiͳ����Ϣ�ṹ��ָ�� */
    dmac_rate_stat_info_stru st_dmac_rate_stat_info;             /* ���ƽ������ͳ����Ϣ�ṹ��ָ�� */
}dmac_user_rate_info_stru;

#ifdef _PRE_WLAN_FEATURE_USER_EXTEND
/* user pn����Ϣ */
typedef struct
{
    hal_pn_stru     st_ucast_tx_pn;                         /* ����tx pn */
    hal_pn_stru     st_mcast_tx_pn;                         /* �鲥tx pn */
    hal_pn_stru     ast_tid_ucast_rx_pn[WLAN_TID_MAX_NUM];  /* tid��Ӧ�ĵ���rx pn */
    hal_pn_stru     ast_tid_mcast_rx_pn[WLAN_TID_MAX_NUM];  /* tid��Ӧ���鲥rx pn */
}dmac_user_pn_stru;
#endif

#ifdef _PRE_WLAN_FEATURE_BAND_STEERING
/* user�����band steering�������ݽṹ */
typedef struct
{
    oal_uint64              ull_last_tx_bytes;         /*ͳ�ƴ�����ʼ��ķ������������ۼ�ֵ*/
    oal_uint64              ull_last_rx_bytes;         /*ͳ�ƴ�����ʼ��Ľ������������ۼ�ֵ*/
    oal_uint32              ul_associate_timestamp;    //�û�������ʱ���

    oal_uint32              ul_average_tx;            //�������һ������ʱ�䴰�ڵ�ƽ��tx
    oal_uint32              ul_average_rx;            //�������һ������ʱ�䴰�ڵ�ƽ��rx
    oal_uint8               uc_valid;                 //������������ҵ�������Ƿ���Ч��������ʱ�䴰̫Сʱ���Ϊ��Ч
    oal_bool_enum_uint8     en_just_assoc_flag;       //�չ���ʱ��λ
    oal_uint8               auc_reserv[2];
}dmac_user_bsd_stru;
#endif

#ifdef _PRE_WLAN_FEATURE_11V
typedef oal_int32         (*dmac_user_callback_func_11v)(oal_void *,oal_uint32, oal_void *);
/* 11v ������Ϣ�ṹ�� */
typedef struct
{
    oal_uint8                       uc_user_bsst_token;                     /* �û�����bss transition ֡������ */
    oal_uint8                       uc_user_status;                         /* �û�11V״̬ */
    oal_uint16                      us_resv;                                /* 4�ֽڶ����� */
    frw_timeout_stru                st_status_wait_timer;                   /* �ȴ��û���Ӧ֡�ļ�ʱ�� */
    dmac_user_callback_func_11v     dmac_11v_callback_fn;                   /* �ص�����ָ�� */
}dmac_user_11v_ctrl_stru;
#endif

#ifdef _PRE_WLAN_FEATURE_BTCOEX
typedef struct
{
    oal_uint32 ul_rx_rate_threshold_min;
    oal_uint32 ul_rx_rate_threshold_max;
    oal_bool_enum_uint8 en_get_addba_req_flag;
    btcoex_rx_window_size_index_enum_uint8 en_ba_size_expect_index;  /* �����ۺϴ�С���� */
    btcoex_rx_window_size_index_enum_uint8 en_ba_size_real_index;    /* ʵ�ʾۺϴ�С���� */
    oal_uint8 uc_ba_size;
    btcoex_rx_window_size_enum_uint8 en_ba_size_tendence;
    wlan_nss_enum_uint8              en_user_nss_num;
    oal_bool_enum_uint8              en_delba_trigger;
    oal_uint8 auc_resv[1];
} dmac_user_btcoex_delba_stru;

typedef struct
{
    oal_uint64 ull_rx_rate_mbps;
    oal_uint16 us_rx_rate_stat_count;
    oal_uint8  auc_resv[2];
} dmac_user_btcoex_rx_info_stru;

typedef struct
{
    btcoex_rate_state_enum_uint8 en_status;
    oal_uint8                    uc_status_sl_time;
    oal_uint8                    uc_rsv[2];
} dmac_user_btcoex_sco_rx_rate_status_stru;

typedef struct
{
    dmac_user_btcoex_delba_stru st_dmac_user_btcoex_delba;
    dmac_user_btcoex_rx_info_stru st_dmac_user_btcoex_rx_info;
    dmac_user_btcoex_rx_info_stru st_dmac_user_btcoex_sco_rx_info;
    dmac_user_btcoex_sco_rx_rate_status_stru st_dmac_user_btcoex_sco_rx_rate_status;
} dmac_user_btcoex_stru;
#endif

#ifdef _PRE_FEATURE_FAST_AGING
typedef struct
{
    oal_uint8   uc_fast_aging_num;
    oal_uint8   auc_resv[3];
    oal_uint32  ul_tx_msdu_succ; //��¼�ϴζ�ʱ����ʱ���¼�ĳɹ�����
    oal_uint32  ul_tx_msdu_fail; //��¼�ϴζ�ʱ����ʱ���¼��ʧ�ܸ���
    oal_uint32  ul_rx_msdu; //��¼�ϴζ�ʱ����ʱ���¼��rx msdu����
} dmac_user_fast_aging_stru;
#endif

typedef struct
{
    /* ����������ܴ���DMAC USER�ṹ���ڵĵ�һ�� */
    mac_user_stru                               st_user_base_info;                  /* hmac user��dmac user�������� */

    /* ��ǰVAP������AP��STAģʽ�������ֶ�Ϊuser��STA��APʱ�����ֶΣ�������ֶ���ע��!!! */
    oal_uint8                                   auc_txchain_mask[WLAN_NSS_LIMIT];    /* ÿ�ֿռ����¶�Ӧ��TX CHAIN MASK */
    dmac_tid_stru                               ast_tx_tid_queue[WLAN_TID_MAX_NUM]; /* ����tid������� */
    oal_void                                   *p_alg_priv;                         /* �û������㷨˽�нṹ�� */
    oal_uint16                                  us_partial_aid;
    oal_int16                                   s_rx_rssi;                              /* �û�����RSSIͳ���� */
    oal_uint16                                  aus_txseqs_frag[WLAN_TID_MAX_NUM];      /* ��Ƭ���ĵ�seq num */
    oal_uint16                                  aus_txseqs[WLAN_TID_MAX_NUM];           /* ����tid��Ӧ��seq num */

    /* ��ǰVAP������APģʽ�������ֶ�Ϊuser��STAʱ�����ֶΣ�������ֶ���ע��!!! */
    oal_uint8                                   bit_ps_mode     : 1,                /* ap�������û�����ģʽ��PSM�� */
                                                bit_active_user : 1,                /* �Ƿ��Ծ�û����û������� */
                                                bit_forbid_rts  : 1,                /* �Ƿ�ǿ�ƽ���RTS(�ڿ���RTS���ڼ���������ʱ����RTS) */
                                                bit_peer_resp_dis :1,               /* �ظ���Ӧ֡���ܣ�ap��Ϊ������Ϊ1��ʾ����ack policyΪ�ξ����ظ���Ӧ֡ */
                                                bit_new_add_user : 1,                /* 1: �Ƿ����´����û�(��ִ��֪ͨ�㷨)��0Ϊ����״̬��Ҫִ���㷨֪ͨ�������滻״̬ */
                                                bit_resv        : 3;

    oal_bool_enum_uint8                         en_vip_flag;                        /* ֻ�㷨�����ã�ͨ�������������ã�TRUE: VIP�û�, FALSE: ��VIP�û� */
    dmac_user_smartant_training_enum_uint8      en_smartant_training;               /* ��������ѵ��״̬  */
    oal_bool_enum_uint8                         en_delete_ba_flag;                  /* ɾ��ba��־��ֻ�㷨ͨ���ӿ��޸ģ�autorate��Э��ģʽʱ��Ϊtrue */

    dmac_tx_normal_rate_stats_stru              st_smartant_rate_stats;             /* ���������ڷ�������е�ͳ����Ϣ */
    dmac_user_ps_stru                           st_ps_structure;                    /* �û��Ľ��ܽṹ,�����û����鲥�û�������*/

#ifdef _PRE_WLAN_FEATURE_UAPSD
    dmac_user_uapsd_stru                        st_uapsd_stru;                      /* �û���U-APSD���ܽṹ*/
#endif

    hal_tx_txop_alg_stru                        st_tx_data_mcast;                    /* �鲥����֡������Ԥ��11kʹ�� */
    hal_tx_txop_alg_stru                        st_tx_data_bcast;                    /* �㲥����֡����, Ԥ��11kʹ�� */
    /*�����Ϣ�ϱ��ֶ�*/
    dmac_user_query_stats_stru                  st_query_stats;
    mac_user_uapsd_status_stru                  st_uapsd_status;                        /* uapsd״̬ */
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
    oal_uint16                                  us_non_qos_seq_frag_num;                 /* ��¼���û���QOS֡��seq+frag num*/
    oal_uint8                                   uc_resv1[2];

    oal_uint32                                  ul_tx_minrate;                          /* һ��ʱ���ڵ���С���� */
    oal_uint32                                  ul_tx_maxrate;                          /* һ��ʱ���ڵ�������� */
    oal_uint32                                  ul_rx_rate_min;                         /* ͳ���ã��û�����С�������� */
    oal_uint32                                  ul_rx_rate_max;                         /* ͳ���ã��û������������� */
#endif
    oal_uint8                                   uc_groupid;
    oal_uint8                                   uc_lut_index;                       /* user��Ӧ��Ӳ����������Ծ�û�id */
    oal_uint8                                   uc_uapsd_flag;                          /* STA��U-APSD��ǰ�Ĵ���״̬ */
    oal_uint8                                   uc_max_key_index;                       /* ���ptk index */

    oal_uint32                                  ul_last_active_timestamp;               /* user����Ծʱ��ʱ�����user�ϻ��ͻ�Ծ�û��滻ʹ��(ʹ��OAL�ṩ��ʱ���������ֵ) */
    oal_uint32                                  ul_rx_rate;                             /* ͳ���ã�ͳ�ƵĽ���֡���ʣ����Զ˵ķ������� ��λkbps */

#ifdef _PRE_DEBUG_MODE_USER_TRACK
    mac_user_track_txrx_protocol_stru           st_txrx_protocol;                       /* ������һ���շ�֡��ʹ�õ�Э��ģʽ��������α仯�ˣ����ϱ�sdt */
    mac_user_track_ctx_stru                     st_user_track_ctx;
#endif
#ifdef _PRE_WLAN_FEATURE_BTCOEX
    dmac_user_btcoex_stru                       st_dmac_user_btcoex_stru;
#endif
#ifdef _PRE_WLAN_FIT_BASED_REALTIME_CALI
    hal_dyn_cali_usr_record_stru                ast_dyn_cali_pow[WLAN_RF_CHANNEL_NUMS];
#endif

#ifdef _PRE_WLAN_FEATURE_USER_EXTEND
    dmac_user_pn_stru                           st_pn;                                  /* ����PN/TSC�ţ����û��滻ʱʹ�� */
#endif
#ifdef _PRE_WLAN_11K_STAT
    dmac_thrpt_stat_info_stru                   st_dmac_thrpt_stat_info;

    dmac_stat_frm_rpt_stru                      *pst_stat_frm_rpt;
    dmac_stat_count_tid_stru                    *pst_stat_count_tid;
    dmac_stat_count_stru                        *pst_stat_count;
    dmac_stat_tsc_rpt_stru                      *pst_stat_tsc_rpt;
    dmac_stat_tid_tx_delay_stru                 *pst_stat_tid_tx_delay;
#endif
#ifdef _PRE_WLAN_FEATURE_BAND_STEERING
    dmac_user_bsd_stru                          st_bsd;
#endif

    hal_user_pow_info_stru                      st_user_pow_info;                       /* �û���������Ϣ�ṹ�� */

#ifdef _PRE_WLAN_FEATURE_11V
    dmac_user_11v_ctrl_stru                     *pst_11v_ctrl_info;                     /* 11v������Ϣ�ṹ�� */
#endif
#ifdef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT
    oal_uint32                                   ul_sta_sleep_times;                    /** STA���ߴ��� */
#endif
    dmac_user_rate_info_stru                     st_user_rate_info;

    /* ROM������Դ��չָ�� */
    oal_void                           *_rom;
#ifdef _PRE_FEATURE_FAST_AGING
    dmac_user_fast_aging_stru                    st_user_fast_aging;
#endif
    oal_uint8                                   bit_ptk_need_install      : 1,          /* ��Ҫ���µ�����Կ */
                                                bit_is_rx_eapol_key_open  : 1,          /* ��¼���յ�eapol-key �Ƿ�Ϊ���� */
                                                bit_eapol_key_4_4_tx_succ : 1,          /* eapol-key 4/4 ���ͳɹ� */
                                                bit_ptk_key_idx           : 3,          /* ���浥����Կkey_idx */
                                                bit_resv2                 : 2;
}dmac_user_stru;


/*****************************************************************************
  8 UNION����
*****************************************************************************/
/*****************************************************************************
  9 OTHERS����
*****************************************************************************/

/*****************************************************************************
  10 ��������
*****************************************************************************/
#ifdef _PRE_DEBUG_MODE_USER_TRACK

extern oal_uint32  dmac_user_check_txrx_protocol_change(dmac_user_stru *pst_dmac_user,
                                                                      oal_uint8      uc_present_mode,
                                                                      oam_user_info_change_type_enum_uint8  en_type);
#endif
extern oal_uint32 dmac_user_set_bandwith_handler(mac_vap_stru *pst_dmac_vap, dmac_user_stru *pst_dmac_user,
                                                                   wlan_bw_cap_enum_uint8 en_bw_cap);
extern oal_uint32  dmac_config_del_multi_user(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
extern oal_uint32  dmac_user_add_multi_user(mac_vap_stru *pst_mac_vap, oal_uint16 us_multi_user_idx);
extern oal_uint32  dmac_user_add(frw_event_mem_stru *pst_event_mem);
extern oal_uint32  dmac_user_add_notify_alg(frw_event_mem_stru *pst_event_mem);
extern oal_uint32  dmac_user_del(frw_event_mem_stru *pst_event_mem);
extern oal_void    dmac_user_key_search_fail_handler(mac_vap_stru *pst_mac_vap,dmac_user_stru *pst_dmac_user,mac_ieee80211_frame_stru *pst_frame_hdr);
extern oal_uint32  dmac_user_tx_inactive_user_handler(dmac_user_stru *pst_dmac_user);
extern oal_uint32  dmac_user_pause(dmac_user_stru *pst_dmac_user);
extern oal_uint32  dmac_user_resume(dmac_user_stru *pst_dmac_user);
extern oal_uint32  dmac_user_active(dmac_user_stru * pst_dmac_user);
extern oal_uint32  dmac_user_inactive(dmac_user_stru * pst_dmac_user);
extern oal_void dmac_user_init_slottime(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user);
#ifdef _PRE_WLAN_MAC_BUGFIX_SW_CTRL_RSP
extern oal_bool_enum_uint8 dmac_user_check_rsp_soft_ctl(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user);
extern oal_uint32 dmac_user_update_sw_ctrl_rsp(mac_vap_stru *pst_mac_vap, mac_user_stru  *pst_mac_user);
#endif
extern oal_uint32  dmac_user_set_groupid_partial_aid(mac_vap_stru  *pst_mac_vap,
                                             dmac_user_stru *pst_dmac_user);
extern oal_uint32  dmac_user_get_tid_by_num(OAL_CONST mac_user_stru * OAL_CONST pst_mac_user, oal_uint8 uc_tid_num, dmac_tid_stru **ppst_tid_queue);

#ifdef _PRE_WLAN_FEATURE_SMPS
extern oal_uint8  dmac_user_get_smps_mode(mac_vap_stru  *pst_mac_vap, mac_user_stru *pst_mac_user);
#endif
oal_uint32  dmac_user_keepalive_timer(void *p_arg);
extern  oal_void dmac_ap_pause_all_user(mac_vap_stru *pst_mac_vap);
extern  oal_void dmac_rx_compatibility_identify(dmac_user_stru *pst_dmac_user, oal_uint32 ul_rate_kbps);
extern  oal_void dmac_rx_compatibility_show_stat(dmac_user_stru *pst_dmac_user);
extern oal_uint32 dmac_user_alloc(oal_uint16 us_user_idx);
extern oal_uint32 dmac_user_free(oal_uint16 us_user_idx);
extern void*  mac_res_get_dmac_user(oal_uint16 us_idx);
extern void*  mac_res_get_dmac_user_alloc(oal_uint16 us_idx);
oal_bool_enum_uint8 dmac_psm_is_psm_empty(dmac_user_stru *pst_dmac_user);
oal_bool_enum_uint8 dmac_psm_is_uapsd_empty(dmac_user_stru  *pst_dmac_user);
oal_bool_enum_uint8 dmac_psm_is_tid_empty(dmac_user_stru  *pst_dmac_user);
oal_uint32 dmac_psm_tid_mpdu_num(dmac_user_stru  *pst_dmac_user);
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
extern  oal_void dmac_ap_resume_all_user(mac_vap_stru *pst_mac_vap);
oal_void dmac_user_ps_queue_overrun_notify(mac_vap_stru *pst_mac_vap);
#endif
extern oal_void  dmac_tid_tx_queue_exit(dmac_user_stru *pst_dmac_user);
extern dmac_user_stru  *mac_vap_get_dmac_user_by_addr(mac_vap_stru *pst_mac_vap, oal_uint8  *puc_mac_addr);
#ifdef _PRE_WLAN_FEATURE_HILINK
extern oal_void dmac_user_notify_best_rate(dmac_user_stru *pst_dmac_user, oal_uint32 ul_best_rate_kbps);
#endif
#ifdef _PRE_WLAN_DFT_EVENT
extern oal_void  dmac_user_status_change_to_sdt(
                                       dmac_user_stru       *pst_dmac_user,
                                       oal_bool_enum_uint8   en_is_user_paused);
#endif
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
extern oal_void dmac_full_phy_freq_user_add(mac_vap_stru *pst_mac_vap, dmac_user_stru *pst_dmac_user);
extern oal_void dmac_full_phy_freq_user_del(mac_vap_stru *pst_mac_vap, dmac_user_stru *pst_dmac_user);
#endif
#ifdef _PRE_WLAN_FEATURE_AMPDU_TX_HW
extern oal_void dmac_chip_pause_all_user(mac_vap_stru *pst_mac_vap);
extern oal_void dmac_chip_resume_all_user(mac_vap_stru *pst_mac_vap);
#endif

#if defined(_PRE_WLAN_FEATURE_HILINK_TEMP_PROTECT) || defined(_PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT)
extern  oal_uint32 dmac_user_pub_timer_callback_func(oal_void *ptr_null);
#endif
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of dmac_user.h */
