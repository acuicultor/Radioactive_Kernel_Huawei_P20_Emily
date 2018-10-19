

#ifndef __DMAC_FTM__
#define __DMAC_FTM__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_FTM

/*****************************************************************************
  1 ����ͷ�ļ�����
*****************************************************************************/
#include "oal_ext_if.h"
#include "oal_mem.h"
#include "mac_vap.h"
#include "dmac_user.h"
#include "dmac_vap.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_FTM_H

/*****************************************************************************
  2 �궨��
*****************************************************************************/

#define     PERIOD_OF_FTM_TIMER                                 6 //6250    /* Ƥ�� ��ǰΪfpga�汾*/

#define     PHY_TX_ADJUTST_VAL                                  0       /* ����phy��connectorʱ�� */
#define     PHY_RX_ADJUTST_VAL                                  0       /* ����connector��phyʱ�� */

#define     T1_FINE_ADJUST_VAL                                  0       /* ʵ�ʲ���У׼ֵ */
#define     T2_FINE_ADJUST_VAL                                  0
#define     T3_FINE_ADJUST_VAL                                  0
#define     T4_FINE_ADJUST_VAL                                  0

#define     FTM_FRAME_DIALOG_TOKEN_OFFSET                       2       /* ftm֡dialog tokenƫ�� */
#define     FTM_FRAME_IE_OFFSET                                 3       /* ftm req֡ieƫ�� */
#define     FTM_FRAME_FOLLOWUP_DIALOG_TOKEN_OFFSET              3       /* ftm֡follow up dialog tokenƫ�� */
#define     FTM_FRAME_TOD_OFFSET                                4       /* ftm֡todƫ�� */
#define     FTM_FRAME_TOA_OFFSET                                10      /* ftm֡toaƫ�� */
#define     FTM_FRAME_OPTIONAL_IE_OFFSET                        20
#define     FTM_FRAME_TSF_SYNC_INFO_OFFSET                      3
#define     FTM_RANGE_IE_OFFSET                                 3
#define     FTM_REQ_TRIGGER_OFFSET                              2       /* ftm req֡Triggerƫ�� */

#define     FTM_WAIT_TIMEOUT                                    10          /*�ȴ�����FTM_1��ʱ��*/

#define     FTM_MIN_DETECTABLE_DISTANCE                         0          /*FTM��С�ɼ�ⷶΧ*/
#define     FTM_MAX_DETECTABLE_DISTANCE                         8192000    /*FTM���ɼ�ⷶΧ 2km ���㵥λΪ1/4096m*/

#define     FTM_FRAME_TOD_LENGTH                                6
#define     FTM_FRAME_TOA_LENGTH                                6
#define     FTM_FRAME_TOD_ERROR_LENGTH                          2
#define     FTM_FRAME_TOA_ERROR_LENGTH                          2
#define     FTM_TMIE_MASK                                       0xFFFFFFFFFFFF /*48λ*/
#define     MAX_REPEATER_NUM                                    3            /* ֧�ֵ����λap���� */

/*****************************************************************************
  3 ö�ٶ���
*****************************************************************************/
typedef enum
{
    FTM_TIME_1             = 0,
    FTM_TIME_2             = 1,
    FTM_TIME_3             = 2,
    FTM_TIME_4             = 3,

    FTM_TIME_BUTT,
}ftm_time_enum;
/* ����ҵ������ */
typedef enum
{
    NO_LOCATION    = 0,
    ROOT_AP        = 1,
    REPEATER       = 2,
    STA            = 3,
    LOCATION_TYPE_BUTT
}oal_location_type_enum;

/*****************************************************************************
  4 ȫ�ֱ�������
*****************************************************************************/
extern    oal_uint32          g_aul_ftm_correct_time[FTM_TIME_BUTT];
extern    dmac_ftm_stru       g_st_ftm;
extern    oal_uint8           guc_csi_loop_flag;

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
    oal_uint8             uc_location_type;                       /*��λ���� 0:�رն�λ 1:root ap;2:repeater;3station*/
    oal_uint8             auc_location_ap[MAX_REPEATER_NUM][WLAN_MAC_ADDR_LEN];    /*��location ap��mac��ַ����һ��Ϊroot */
    oal_uint8             auc_station[WLAN_MAC_ADDR_LEN];          /*STA MAC��ַ����ʱֻ֧��һ��sta�Ĳ���*/
} dmac_location_stru;

typedef struct
{
    oal_uint8                uc_category;
    oal_uint8                uc_action_code;
    oal_uint8                auc_oui[3];
    oal_uint8                uc_eid;
    oal_uint8                uc_lenth;
    oal_uint8                uc_location_type;
    oal_uint8                auc_mac_server[WLAN_MAC_ADDR_LEN];
    oal_uint8                auc_mac_client[WLAN_MAC_ADDR_LEN];
    oal_uint8                auc_payload[4];
}dmac_location_event_stru;


/*****************************************************************************
  8 UNION����
*****************************************************************************/


/*****************************************************************************
  9 OTHERS����
*****************************************************************************/

/*****************************************************************************
  10 ��������
*****************************************************************************/

extern oal_uint32 dmac_process_ftm_ack_complete(frw_event_mem_stru * pst_event_mem);
extern oal_uint32  dmac_sta_rx_ftm(dmac_vap_stru *pst_dmac_vap, oal_netbuf_stru *pst_netbuf);
extern oal_uint32 dmac_check_tx_ftm(dmac_vap_stru *pst_dmac_vap, oal_netbuf_stru *pst_buf);
extern oal_void dmac_rrm_parse_ftm_range_req(dmac_vap_stru *pst_dmac_vap, mac_meas_req_ie_stru  *pst_meas_req_ie);
extern oal_uint32  dmac_sta_send_ftm_req(dmac_vap_stru *pst_dmac_vap);
extern oal_uint32  dmac_ftm_rsp_send_ftm(dmac_vap_stru *pst_dmac_vap, oal_uint8 uc_session_id);
extern oal_uint32  dmac_ftm_send_trigger(dmac_vap_stru *pst_dmac_vap);
extern oal_uint32 dmac_sta_start_scan_for_ftm(dmac_vap_stru *pst_dmac_vap, oal_uint16 us_scan_time);
extern oal_void  dmac_save_ftm_range(dmac_vap_stru *pst_dmac_vap);
extern oal_uint32  dmac_sta_ftm_wait_start_burst_timeout(void *p_arg);
extern oal_void dmac_rrm_meas_ftm(dmac_vap_stru *pst_dmac_vap);
extern oal_uint16  dmac_encap_ftm_mgmt(dmac_vap_stru *pst_dmac_vap, oal_netbuf_stru *pst_buffer, oal_uint8 uc_session_id);
extern oal_void  dmac_ftm_rsp_rx_ftm_req(dmac_vap_stru *pst_dmac_vap, oal_netbuf_stru *pst_netbuf);
extern oal_void  dmac_tx_set_ftm_ctrl_dscr(dmac_vap_stru *pst_dmac_vap, hal_tx_dscr_stru * p_tx_dscr, oal_netbuf_stru *pst_netbuf);
extern oal_void  dmac_set_ftm_correct_time(dmac_vap_stru *pst_dmac_vap, mac_set_ftm_time_stru st_ftm_time);
extern oal_uint32  dmac_ftm_rsp_set_parameter(dmac_vap_stru *pst_dmac_vap, oal_uint8 uc_session_id);
extern oal_void  dmac_sta_ftm_start_bust(dmac_vap_stru *pst_dmac_vap);
extern oal_void dmac_vap_ftm_int(dmac_vap_stru *pst_dmac_vap);
extern oal_int8 dmac_ftm_find_session_index(dmac_vap_stru *pst_dmac_vap, mac_ftm_mode_enum_uint8 en_ftm_mode, oal_uint8 auc_peer_mac[WLAN_MAC_ADDR_LEN]);
extern oal_void dmac_ftm_enable_session_index(dmac_vap_stru *pst_dmac_vap, mac_ftm_mode_enum_uint8 en_ftm_mode, oal_uint8 auc_peer_mac[WLAN_MAC_ADDR_LEN], oal_uint8 uc_session_id);
extern oal_bool_enum_uint8 dmac_check_ftm_switch_channel(dmac_vap_stru *pst_dmac_vap, mac_ftm_mode_enum_uint8 en_ftm_mode, oal_uint8 uc_chan_number);
extern oal_uint32 dmac_location_rssi_process(dmac_vap_stru *pst_dmac_vap, oal_netbuf_stru *pst_netbuf, oal_uint8 c_rssi);
extern oal_uint32 dmac_location_csi_process(dmac_vap_stru *pst_dmac_vap, oal_uint8 *puc_ta, oal_uint32 ul_csi_info_len, oal_uint32 **pul_csi_start_addr);


#endif
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of dmac_p2p.h */
