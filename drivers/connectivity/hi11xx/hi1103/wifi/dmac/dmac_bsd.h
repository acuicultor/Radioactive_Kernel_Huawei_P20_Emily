

#ifndef __DMAC_BSD_H__
#define __DMAC_BSD_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#include "wlan_types.h"
#include "oal_list.h"
#include "dmac_vap.h"
#include "dmac_device.h"
#include "oam_wdk.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_BSD_H

typedef struct{ 
    oal_uint32  ul_average_tx_thru;              //�û���ͳ�Ƶ�ʱ�䴰�ڵ�ƽ������������
    oal_uint32  ul_average_rx_thru;              //�û���ͳ�Ƶ�ʱ�䴰�ڵ�ƽ������������
    oal_uint32  ul_best_rate;                    //�û��ھ���ʱ���������ʣ���autorate�㷨��ȡ
    oal_int8    c_average_rssi;                  //5G�û���ͳ��ʱ�䴰�ڵ�ƽ������rssi
    oal_uint8   uc_reserv[3];
}dmac_user_bsd_info_stru;


typedef enum
{
    DMAC_BSD_STA_CAP_UNKOWN     = 0x0,      //��δʶ��
    DMAC_BSD_STA_CAP_SINGLE_BAND = 0x1,     //��Ƶ����
    DMAC_BSD_STA_CAP_DUAL_BAND  = 0x2,      //˫Ƶ����

    DMAC_BSD_STA_CAP_BUTT
} dmac_bsd_sta_cap_enum;
typedef oal_uint8 dmac_bsd_sta_cap_enum_uint8;



//BSDģ���˫Ƶ����ʶ�����̿������ݽṹ
typedef struct
{
    oal_uint32      ul_block_cnt;
    oal_uint32      ul_last_rec_timestamp;      //���һ�ν��յ����û�֡��ʱ���
    oal_uint32      ul_last_rec_band;           //���һ�ν��յ����û�֡��Ƶ��
} bsd_sta_cap_classify_stru;

//BSDģ���STA��Ϣ
typedef struct
{
    oal_uint8                   auc_user_mac_addr[WLAN_MAC_ADDR_LEN];   //MAC��ַ
    dmac_bsd_sta_cap_enum_uint8 en_band_cap;                            
    oal_uint8                   bit_steering_cfg_enable:1;              //�û��Ƿ�����Ϊ�����л�
    oal_uint8                   bit_lock_state:1;                       //�û��Ƿ����л�����״̬
    oal_uint8                   bit_steering_unfriendly_flag:1;         //�û��Ƿ����л����Ѻ��û�
    oal_uint8                   bit_associate_flag:1;                   //���û��Ƿ��Ѿ����� 
    oal_uint8                   bit_pre_steer_fail_flag:1;
    oal_bool_enum_uint8         en_last_assoc_by_bsd:1;                  //����STA���һ�ι����Ƿ�����ΪBSD�л�����������
    oal_uint8                   bit_reserv:2;
    oal_uint8                   uc_associate_cnt;
    oal_uint8                   uc_last_assoc_vap_id;
    oal_uint8                   auc_reserv[2];
    dmac_vap_stru               *pst_bsd_source_vap;    
    bsd_sta_cap_classify_stru   st_cap_classify_ctl;
    oal_uint32                  ul_pre_steer_succ_cnt;                  //�û������׶������ɹ�����
    oal_uint32                  ul_pre_steer_cnt;                       //�û������׶���������
    oal_uint32                  ul_steering_succ_cnt;                   //�û�����֮�������л��ɹ�����
    oal_uint32                  ul_steering_cnt;                        //�û�����֮�������л�����
    oal_uint32                  ul_steering_last_timestamp;             //�û����һ���л���ʱ���
    oal_uint32                  ul_last_active_timestamp;               //������µ�ʱ���
    oal_uint32                  ul_last_lock_timestamp;                 //���������ʱ���
    oal_dlist_head_stru         st_by_hash_list;                        //����hashֵ�γɵ�����
    oal_dlist_head_stru         st_by_timestamp_list;                   //����ʱ����γɵ�����
} bsd_sta_info_item_stru;

/*TBD �����û���¼�ı��������Կ��Ǹ���mac_chip_get_max_asoc_user ��ȡ����ʱ֧�ֵ��������û���
      ����̬������ڴ���Դ����ʱΪ��̬����*/
#define BSD_STA_INFO_MAX_CNT            (128*2+10)   //ÿ��оƬ������֧��128���û�ͬʱ��������������10�����ڱ�������STA����Ϣ
#define BSD_STA_INFO_HASH_TBL_SIZE      64         //hash��64����¼��

typedef struct
{
    bsd_sta_info_item_stru   ast_array[BSD_STA_INFO_MAX_CNT];           //�����¼���ڴ��
    oal_uint32               ul_item_cnt;                               //��¼�ѱ�����û���
    oal_dlist_head_stru      ast_hash_tbl[BSD_STA_INFO_HASH_TBL_SIZE];  //��������
    oal_dlist_head_stru      st_list_head;                              //�����������ݸ���ʱ������,head->next��ʾ���ϵļ�¼
} bsd_sta_tbl_stru;


typedef enum
{
    CFG_PARAM_INDEX_BUSY_TH = 0x0,      //���ط�æ��ֵ��������
    CFG_PARAM_INDEX_FREE_TH = 0x1,      //���ؿ��з�ֵ��������
    CFG_PARAM_INDEX_THRU_TH  = 0x2,     //�����л���������ֵ��������
    CFG_PARAM_INDEX_RSSI_TH  = 0x3,     //5G STA�����л���RSSI��ֵ��������
    CFG_PARAM_INDEX_SFRE_TH  = 0x4,     //�����������л�Ƶ�ȷ�ֵ��������
    CFG_PARAM_INDEX_LOCK_TIME  = 0x5,   //����ʱ����������
    CFG_PARAM_INDEX_NEW_USER_LOCK_TIME  = 0x6,   //���û�����ʱ����������
    CFG_PARAM_INDEX_BLOCK_TIME  = 0x7,   //�л�ʱ����ʱ����������
    CFG_PARAM_INDEX_BUTT
} dmac_bsd_cfg_param_index_enum;
typedef oal_uint8 dmac_bsd_cfg_param_index_enum_uint8;


typedef struct
{
    oal_int8                                *puc_param_str;
    dmac_bsd_cfg_param_index_enum_uint8     en_param_index;
} bsd_cfg_param_index_stru;


#define DMAC_BSD_CFG_LOAD_TH_MAX               1000
#define DMAC_BSD_LOCK_TIME_UNIT_MS             1000

#define DMAC_BSD_CFG_RSSI_TH_MAX               100
#define DMAC_BSD_CFG_RSSI_TH_MIN               20


//BSD���Ե����ò���
typedef struct
{
    oal_uint32      ul_vap_load_busy_th;     //vap��æ��ֵ
    oal_uint32      ul_vap_load_free_th;     //vap���з�ֵ
    oal_uint32      ul_sta_steering_thr_th;  //����STA�л���������ֵ kbps
    oal_uint32      ul_steering_fre_th;      //�л�Ƶ�ȷ�ֵ����λ����/30min
    oal_uint32      ul_steering_lock_time;   //�л�����ʱ�䣬��λ��min
    oal_int8        c_5g_sta_rssi_th;        //5G STA����steering��RSSI��ֵ
    oal_uint8       uc_reserv;
    oal_uint16      us_new_user_lock_time; //���û��չ���ʱ���л�����ʱ�䣬��λ:s
    oal_uint32      ul_steering_blocktime;  //���õ����ڶ�ʱ���ĳ�ʱʱ��
    
} bsd_cfg_stru;

//bsd ap�����ݽṹ,һ��bsd apʵ���϶�Ӧ�˲�ͬƵ�ε�2��vap
typedef struct
{
    oal_dlist_head_stru st_entry;           //�γ����е�bsd ap����
    dmac_vap_stru       *past_dmac_vap[2];   //ָ����������Steeringģʽ��vap��
                                            //��2��vap��upʱ�����������BSDģ�飬ƫ��0��ʾ2G��vap ƫ��1��ʾ5G��vap
    bsd_cfg_stru        st_bsd_config;      //bsd ap��steering���ò���
} bsd_ap_stru;

//BSDģ��Ĺ���֮��steeringʵʩģ��Ŀ��ƽṹ��
typedef struct
{
    bsd_ap_stru             *pst_bsd_ap;         //����ʵʩsteering��bsd ap
    dmac_vap_stru           *pst_source_vap;     //Դvap
    bsd_sta_info_item_stru  *pst_user_info;      //����ʵʩsteering��sta����Ϣ
    frw_timeout_stru        st_block_timer;      //���ڶ�ʱ��
    oal_uint8               auc_mac_addr[WLAN_MAC_ADDR_LEN];       //���ڱ�Steering��user mac��ַ
    oal_uint8               uc_state;
    oal_uint8               uc_reserv;
} bsd_steering_stru;


//BSDģ��Ĺ����׶�steeringʵʩģ��Ŀ��ƽڵ�
typedef struct
{
    oal_dlist_head_stru st_list_entry;                      //����
    bsd_ap_stru         *pst_bsd_ap;                        //����ʵʩpre-steer��bsd ap
    dmac_vap_stru       *pst_source_vap;                    //Դvap
    frw_timeout_stru    st_block_timer;                     //�����׶�ÿ���û������Ķ�ʱ��
    oal_uint8           auc_mac_addr[WLAN_MAC_ADDR_LEN];    //���ڱ�pre-Steer��user mac��ַ
    oal_uint8           uc_block_timeout;                   //����Ƿ��Ѿ�������ʱ
    oal_uint8           uc_reserv;
    bsd_sta_info_item_stru  *pst_sta_info;
} bsd_pre_steering_node_stru;

typedef struct
{
    oal_dlist_head_stru         st_pre_steering_list;    /* �����û��ڵ�˫������,ʹ��bsd_pre_steering_node_stru�ṹ�ڵ�DLIST */
    bsd_pre_steering_node_stru  *pst_cache_user_node;
} bsd_pre_steering_stru;



//BSDģ��ȫ�ֵĹ���ṹ��
typedef struct
{
    oal_bool_enum_uint8      en_switch;                       //bsd���ԵĶ�̬���أ��˿��عر�ʱ��BandSteeringģ�鲻�ٲ���ؼ��¼�
    oal_uint8                bit_load_status_switch:1;        //���bsd ap�ĸ���״̬����    
    oal_uint8                bit_debug_mode:1;
    oal_uint8                bit_pre_steer_swtich:1;          //�����׶������Ķ�̬����  
    oal_uint8                bit_steer_mode:1;                //steerģʽ��0:����������probe req  1:�ظ�ʧ�ܵ�auth resp  
    oal_uint8                bit_cfg_switch:1;                //bsd���Ե����ÿ��أ���ҳͨ��iwpriv�·������������
    oal_uint8                bit_reserv:3;
    oal_uint8                uc_debug1;                        //for debug
    oal_uint8                uc_debug2;
    oal_dlist_head_stru      st_bsd_ap_list_head;             //�����������е�bsd ap����Ӧ��bsd_ap_stru��st_entry
    bsd_sta_tbl_stru         st_sta_info_tbl;                 //��ģ��ά����STA��Ϣ(�û�����˫Ƶ�������л�ͳ�ơ��л�����)
    bsd_steering_stru        st_steering_ctl;                 //������steeringʵʩģ��Ŀ��ƽṹ��
    bsd_pre_steering_stru    st_pre_steering_ctl;             //�����׶�steeringʵʩģ��Ŀ��ƽṹ��
    frw_timeout_stru         st_bsd_sched_timer;              //bsd task�ĵ��ȶ�ʱ��
} bsd_mgmt_stru;


typedef enum
{
    DMAC_BSD_RET_BLOCK = 0x0,      //��������
    DMAC_BSD_RET_CONTINUE = 0x1,   //��������
    DMAC_BSD_RET_BUTT
}dmac_bsd_handle_result_enum;
typedef oal_uint8 dmac_bsd_handle_result_enum_uint8;

typedef enum
{
    DMAC_BSD_PRE_STEER = 0x0,       //�����׶ε�����
    DMAC_BSD_STEERING = 0x1,        //�����ɹ�֮�������
    DMAC_BSD_STEER_BUTT
}dmac_bsd_steer_type_enum;
typedef oal_uint8 dmac_bsd_steer_type_enum_uint8;




extern oal_void     dmac_bsd_init(void);
extern oal_void     dmac_bsd_exit(void);
extern oal_uint32   dmac_bsd_config_get(bsd_ap_stru *pst_bsd_ap,bsd_cfg_stru *pst_cfg);
//extern oal_void     dmac_bsd_vap_add_handle(dmac_vap_stru * pst_dmac_vap,mac_cfg_add_vap_param_stru *pst_param);
//extern oal_void     dmac_bsd_vap_del_handle(dmac_vap_stru * pst_dmac_vap);
extern oal_void     dmac_bsd_vap_up_handle(dmac_vap_stru * pst_dmac_vap);
extern oal_void     dmac_bsd_vap_down_handle(dmac_vap_stru * pst_dmac_vap);
extern oal_void     dmac_bsd_user_add_handle(dmac_vap_stru * pst_dmac_vap,dmac_user_stru *pst_dmac_user);
extern oal_void     dmac_bsd_user_del_handle(dmac_vap_stru * pst_dmac_vap,dmac_user_stru *pst_dmac_user);
extern oal_void     dmac_bsd_channel_switch_handle(dmac_vap_stru * pst_dmac_vap);
extern oal_void     dmac_bsd_device_load_scan_cb(oal_void *p_param);
extern oal_uint32   dmac_config_bsd(mac_vap_stru *pst_mac_vap, oal_uint8 us_len, oal_uint8 *puc_param);
extern oal_bool_enum_uint8 dmac_bsd_debug_switch(oal_void);
extern dmac_bsd_handle_result_enum_uint8   dmac_bsd_rx_probe_req_frame_handle(dmac_vap_stru *pst_dmac_vap,oal_uint8 *puc_addr,oal_int8  c_rssi);
extern dmac_bsd_handle_result_enum_uint8   dmac_bsd_rx_auth_req_frame_handle(dmac_vap_stru *pst_dmac_vap,oal_uint8 *puc_addr,oal_netbuf_stru *pst_netbuf);
extern oal_bool_enum_uint8 dmac_bsd_get_capability(oal_void);
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
