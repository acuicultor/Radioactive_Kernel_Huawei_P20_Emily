

#ifndef __DMAC_DEVICE_H__
#define __DMAC_DEVICE_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 ����ͷ�ļ�����
*****************************************************************************/
#include "mac_device.h"
#include "dmac_alg_if.h"

#ifdef  _PRE_WLAN_FEATURE_GREEN_AP
#include "dmac_green_ap.h"
#endif

#ifdef  _PRE_WLAN_FEATURE_PACKET_CAPTURE
#include "dmac_pkt_capture.h"
#endif

#ifdef _PRE_WLAN_FEATURE_HWBW_20_40
#include "dmac_acs.h"
#endif
#include "dmac_scan.h"
#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_DEVICE_H

/*****************************************************************************
  2 �궨��
*****************************************************************************/
#define MAX_MAC_ERR_IN_TBTT 5   /*ÿһ��tbtt��϶������ֵ���������*/

#define DMAC_DEV_GET_MST_HAL_DEV(_pst_dmac_device)     ((_pst_dmac_device)->past_hal_device[HAL_DEVICE_ID_MASTER])
#define DMAC_DEV_GET_SLA_HAL_DEV(_pst_dmac_device)     (dmac_device_get_another_h2d_dev((_pst_dmac_device), (_pst_dmac_device)->past_hal_device[HAL_DEVICE_ID_MASTER]))

#define DMAC_RSSI_LIMIT_DELTA  (5)  //���û�����rssi����Ϊa���ѹ����û����޳�rssi����Ϊb��a = b+DMAC_RSSI_LIMIT_DELTA; �������������Ϊb. deltaĬ��Ϊ5�����޸�

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
typedef struct
{
    wlan_nss_enum_uint8                     en_nss_num;              /* Nss �ռ������� */
    oal_uint8                               uc_phy_chain;             /* ����ͨ�� */
    oal_uint8                               uc_single_tx_chain;
    /* ����֡���õ�ͨ������ʱѡ���ͨ��(��ͨ��ʱҪ���ú�uc_phy_chainһ��),��������phy����ͨ������ */
    oal_uint8                               uc_rf_chain;             /* ����ͨ�� */
} dmac_device_capability_stru;


#ifdef _PRE_WLAN_FEATURE_BAND_STEERING

#define     BSD_VAP_LOAD_SAMPLE_MAX_CNT         16      //��������ĸ���ֵ��������ע���ֵΪ2��N�η�
typedef struct
{
    oal_uint16  aus_buf[BSD_VAP_LOAD_SAMPLE_MAX_CNT];
    oal_uint32  ul_size;
    oal_uint32  ul_in;
    oal_uint32  ul_out;
}dmac_device_bsd_ringbuf;

typedef struct
{
    oal_uint8                   uc_state;               //�Ѿ���ʼ���ı��
    oal_uint8                   auc_reserve[3];
    //frw_timeout_stru            st_load_sample_timer;   //���صĲ�����ʱ������ʱ����ʱ��������������һ��
                                                        //���ŵ�ɨ�裬����ȡ��ʱ���ڵ��ŵ�ɨ����
    dmac_device_bsd_ringbuf     st_sample_result_buf;   //�����������Ļ��λ�����
    //mac_scan_req_stru           st_scan_req_param;      //ɨ�����
}dmac_device_bsd_stru;

#endif

#ifdef _PRE_WLAN_FEATURE_GNSS_SCAN
#pragma pack(1)
typedef struct
{
    oal_uint8   auc_bssid[WLAN_MAC_ADDR_LEN];   /* ����bssid */
    oal_uint8   uc_channel_num;
    oal_int8    c_rssi;                       /* bss���ź�ǿ�� */
    oal_uint8   uc_serving_flag;
    oal_uint8   uc_rtt_unit;
    oal_uint8   uc_rtt_acc;
    oal_uint32  ul_rtt_value;
}wlan_ap_measurement_info_stru;

/* �ϱ���gnss��ɨ�����ṹ�� */
typedef struct
{
    oal_uint32                     ul_interval_from_last_scan;
    oal_uint8                      uc_ap_valid_number;
    wlan_ap_measurement_info_stru  ast_wlan_ap_measurement_info[DMAC_SCAN_MAX_AP_NUM_TO_GNSS];
}dmac_gscan_report_info_stru;
#pragma pack()

typedef struct
{
    oal_dlist_head_stru            st_entry;                    /* ����ָ�� */
    wlan_ap_measurement_info_stru  st_wlan_ap_measurement_info; /*�ϱ�gnss��ɨ����Ϣ */
}dmac_scanned_bss_info_stru;

typedef struct
{
    oal_uint32                     ul_scan_end_timstamps;/* ��¼�˴�ɨ���ʱ���,һ��ɨ���¼һ��,����ɨ����ap�ֱ��¼ */
    oal_dlist_head_stru            st_dmac_scan_info_list;
    oal_dlist_head_stru            st_scan_info_res_list;  /* ɨ����Ϣ�洢��Դ���� */
    dmac_scanned_bss_info_stru     ast_scan_bss_info_member[DMAC_SCAN_MAX_AP_NUM_TO_GNSS];
}dmac_scan_for_gnss_stru;
#endif

#ifdef _PRE_FEATURE_FAST_AGING
#define DMAC_FAST_AGING_NUM_LIMIT (10)
#define DMAC_FAST_AGING_TIMEOUT   (1000)

typedef struct
{
    frw_timeout_stru                    st_timer;           /* �����ϻ���ʱ�� */
    oal_bool_enum_uint8                 en_enable;
    oal_uint8                           uc_count_limit;
    oal_uint16                          us_timeout_ms;
}dmac_fast_aging_stru;
#endif
/* dmac device�ṹ�壬��¼ֻ������dmac��device������Ϣ */
typedef struct
{
    mac_device_stru                    *pst_device_base_info;                   /* ָ�򹫹�����mac device */

#if (WLAN_MAX_NSS_NUM >= WLAN_DOUBLE_NSS)
    oal_uint8                           uc_fake_vap_id;
    oal_uint8                           auc_bfer_rev[3];
#endif
#ifdef _PRE_WLAN_MAC_BUGFIX_SW_CTRL_RSP
    oal_uint8                           uc_rsp_frm_rate_val;
    oal_bool_enum_uint8                 en_state_in_sw_ctrl_mode;
    hal_channel_assemble_enum_uint8     en_usr_bw_mode;
    oal_uint8                           auc_rsv[1];
#endif

    oal_bool_enum_uint8                 en_is_fast_scan;     /* �Ƿ��ǲ���ɨ�� */
    oal_bool_enum_uint8                 en_fast_scan_enable; /*�Ƿ���Բ���,XXԭ��ʹӲ��֧��Ҳ���ܿ���ɨ��*/
    oal_bool_enum_uint8                 en_dbdc_enable;      /* ����Ƿ�֧��dbdc */
    oal_uint8                           uc_gscan_mac_vap_id;  /* gscan ��ɨ��vap */

#ifdef _PRE_WLAN_FEATURE_GREEN_AP
    dmac_green_ap_mgr_stru              st_green_ap_mgr;                     /*device�ṹ�µ�green ap���Խṹ��*/
#endif

#ifdef _PRE_WLAN_FEATURE_BAND_STEERING
    dmac_device_bsd_stru                st_bsd;
#endif

    hal_to_dmac_device_stru            *past_hal_device[WLAN_DEVICE_MAX_NUM_PER_CHIP];//03��̬dbdc ����hal device,02/51/03��̬dbdcһ��hal device
    hal_to_dmac_chip_stru              *pst_hal_chip;                        /* Ӳchip�ṹָ�룬HAL�ṩ�������߼�������chip�Ķ�Ӧ */

#ifdef _PRE_WLAN_FEATURE_PACKET_CAPTURE
    dmac_packet_stru                    st_pkt_capture_stat;                 /*device�ṹ�µ�ץ�����Խṹ��*/
#endif

#ifdef _PRE_WLAN_11K_STAT
    dmac_stat_count_stru                *pst_stat_count;
#endif

    //rssi cfg
    mac_cfg_rssi_limit_stru              st_rssi_limit;

#ifdef _PRE_WLAN_FEATURE_DFS
    oal_uint32                           ul_dfs_cnt;                          /*��⵽���״����*/
#endif

#ifdef _PRE_WLAN_FEATURE_HWBW_20_40
    dmac_acs_2040_user                  st_acs_2040_user;
#endif

#ifdef _PRE_WLAN_FEATURE_GNSS_SCAN
    dmac_scan_for_gnss_stru             st_scan_for_gnss_info;
#endif
    /* ROM������Դ��չָ�� */
    oal_void                           *_rom;
#ifdef _PRE_FEATURE_FAST_AGING
    dmac_fast_aging_stru                st_fast_aging;
#endif
}dmac_device_stru;

/*****************************************************************************
  8 UNION����
*****************************************************************************/


/*****************************************************************************
  9 OTHERS����
*****************************************************************************/
#define   DMAC_DEVICE_GET_HAL_CHIP(_pst_device)                  (((dmac_device_stru *)(_pst_device))->pst_hal_chip)


OAL_STATIC OAL_INLINE mac_fcs_mgr_stru* dmac_fcs_get_mgr_stru(mac_device_stru *pst_device)
{
#if IS_DEVICE
    return  &pst_device->st_fcs_mgr;
#else
    return OAL_PTR_NULL;
#endif
}


/*****************************************************************************
  10 ��������
*****************************************************************************/
extern  oal_uint32 dmac_device_exception_report_timeout_fn(oal_void *p_arg);

/*****************************************************************************
  10.1 �����ṹ���ʼ����ɾ��
*****************************************************************************/

extern oal_uint32  dmac_board_exit(mac_board_stru *pst_board);
extern oal_uint32  dmac_board_init(mac_board_stru *pst_board);

/*****************************************************************************
  10.2 ������Ա���ʲ���
*****************************************************************************/
extern oal_uint32  dmac_mac_error_overload(mac_device_stru *pst_mac_device, hal_mac_error_type_enum_uint8 en_error_id);
extern oal_void  dmac_mac_error_cnt_clr(mac_device_stru *pst_mac_device);
extern oal_void  dmac_mac_error_cnt_inc(mac_device_stru *pst_mac_device, oal_uint8 uc_mac_int_bit);
extern hal_to_dmac_device_stru*  dmac_device_get_another_h2d_dev(dmac_device_stru *pst_dmac_device, hal_to_dmac_device_stru *pst_ori_hal_dev);
extern oal_bool_enum_uint8 dmac_device_is_dynamic_dbdc(dmac_device_stru *pst_dmac_device);
extern oal_bool_enum_uint8 dmac_device_is_support_double_hal_device(dmac_device_stru *pst_dmac_device);
extern hal_to_dmac_device_stru *dmac_device_get_work_hal_device(dmac_device_stru *pst_dmac_device);
extern oal_uint8  dmac_device_check_fake_queues_empty(oal_uint8 uc_device_id);
extern oal_void  dmac_free_hd_tx_dscr_queue(hal_to_dmac_device_stru *pst_hal_dev_stru);
extern oal_bool_enum_uint8 dmac_device_check_is_vap_in_assoc(mac_device_stru  *pst_mac_device);
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of dmac_device.h */
