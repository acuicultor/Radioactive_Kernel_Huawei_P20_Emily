

#ifndef __DMAC_11V_H__
#define __DMAC_11V_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_11V

/*****************************************************************************
  1 ����ͷ�ļ�����
*****************************************************************************/
#include "oal_ext_if.h"
#include "oal_mem.h"
#include "mac_vap.h"
#include "dmac_user.h"
#include "dmac_vap.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_11V_H
/*****************************************************************************
  2 �궨��
*****************************************************************************/
/* Ŀǰ����֡�����ڴ�800�ֽ� ֡�Դ���󳤶�19(frame boady)+50(url)+ N*(15(neighbor)+3(sub)+12(sub))  �����ڳ��� �޸���������ʱ��ע�� */
#define DMAC_MAX_BSS_NEIGHBOR_LIST  20          /* BSS Transition ֡�������͵�����ѡAP�б����� */

// 11v�ȴ�֡���صĳ�ʱʱ��
#define DMAC_11V_WAIT_STATUS_TIMEOUT        5000         // 5000ms
#define DMAC_11V_MAX_URL_LENGTH             50          /* Я��URL�ַ�����󳤶�����Ϊ50 */
#define DMAC_11V_TERMINATION_TSF_LENGTH     8           /* termination_tsfʱ�����ֽڳ��� */
#define DMAC_11V_QUERY_FRAME_BODY_FIX       4           /* query֡֡��̶�ͷ���� */
#define DMAC_11V_REQUEST_FRAME_BODY_FIX     7           /* query֡֡��̶�ͷ���� */
#define DMAC_11V_RESPONSE_FRAME_BODY_FIX    5           /* response֡֡��̶�ͷ���� */
#define DMAC_11V_PERFERMANCE_ELEMENT_LEN    1           /* perfermance ie length */
#define DMAC_11V_TERMINATION_ELEMENT_LEN    10          /* termination ie length */
#define DMAC_11V_TOKEN_MAX_VALUE            255         /* ֡������������ֵ */
#define DMAC_11V_SUBELEMENT_ID_RESV         0           /* SUBELEMENTԤ�� ID*/

/*****************************************************************************
  3 ö�ٶ���
*****************************************************************************/
/*****************************************************************************
  Neighbor Report Element����ϢԪ��(Subelement)�� ID
  820.11-2012Э��583ҳ��Table 8-115��SubElement IDs
*****************************************************************************/
typedef enum
{
    DMAC_NEIGH_SUB_ID_RESERVED              = 0,
    DMAC_NEIGH_SUB_ID_TFS_INFO,
    DMAC_NEIGH_SUB_ID_COND_COUNTRY,
    DMAC_NEIGH_SUB_ID_BSS_CANDIDATE_PERF,
    DMAC_NEIGH_SUB_ID_TERM_DURATION,

    DMAC_NEIGH_SUB_ID_BUTT
}dmac_neighbor_sub_eid_enum;
typedef oal_uint8 dmac_neighbor_sub_eid_enum_uint8;

/* 11V response ֡Ӧ����ö�� */
typedef enum
{
    DMAC_11V_RESPONSE_ACCEPT              = 0,      /* 0 �����л� */
    DMAC_11V_RESPONSE_REJECT_UNSPEC,                /* 1 �ܾ���ԭ��δָ�� */
    DMAC_11V_RESPONSE_REJECT_INSUF_BEACON,          /* 2 �ܾ������յ�Beacon���� */
    DMAC_11V_RESPONSE_REJECT_INSUF_CAP,             /* 3 �ܾ���������ƥ�� */
    DMAC_11V_RESPONSE_REJECT_TERM_UNDESIRE,         /* 4 �ܾ�����ϣ��BSS�ս� */
    DMAC_11V_RESPONSE_REJECT_TERM_DELAY,            /* 5 �ܾ��������ӳ�BSS��ֹ */
    DMAC_11V_RESPONSE_REJECT_LIST_PROVIDE,          /* 6 �ܾ����ṩ�ھ�AP�б� */
    DMAC_11V_RESPONSE_REJECT_NO_CANDIDATE,          /* 7 �ܾ����޺��ʺ�ѡAP */
    DMAC_11V_RESPONSE_REJECT_LEAVE_ESS,             /* 8 �ܾ������뿪ESS */

    DMAC_11V_RESPONSE_CODE_BUTT
}dmac_11v_response_code_enum;
typedef oal_uint8 dmac_11v_response_code_enum_uint8;

typedef enum
{
    DMAC_11V_CALLBACK_RETURN_REICEVE_RSP    = 0,    /* �ص�����:���յ�RSP */
    DMAC_11V_CALLBACK_RETURN_WAIT_RSP_TIMEOUT,      /* �ص�����:����RSP��ʱ */

    DMAC_11V_CALLBACK_RETURN_BUTT
}dmac_11v_callback_return_enum;
typedef oal_uint8 dmac_11v_callback_return_enum_uint8;



/*****************************************************************************
  4 STRUCT����
*****************************************************************************/
/* Subelement BSS Transition Termination Duration */
struct dmac_bss_term_duration
{
    oal_uint8       uc_sub_ie_id;                                               /* subelement ID����ID�ó�0 ��ʾ�����ڸ�Ԫ�� */
    oal_uint8       auc_termination_tsf[DMAC_11V_TERMINATION_TSF_LENGTH];                                     /* BSS��ֹʱ��: TSF */
    oal_uint16      us_duration_min;                                            /* BSS��ʧʱ�� time: ���� */
    oal_uint8       uc_resv;                                                    /* ���ֽڶ����� */
}__OAL_DECLARE_PACKED;
typedef struct dmac_bss_term_duration  dmac_bss_term_duration_stru;

/* ��ѡBSS�б���Neighbor Report IE�ṹ�� ����ֻ��Ҫ�õ�subelement 3 4 �ʶ�������subelement */
struct dmac_neighbor_bss_info
{
    oal_uint8       auc_mac_addr[WLAN_MAC_ADDR_LEN];                      /* BSSID MAC��ַ */
    oal_uint8       uc_opt_class;                                         /* Operation Class */
    oal_uint8       uc_chl_num;                                           /* Channel number */
    oal_uint8       uc_phy_type;                                          /* PHY type */
    oal_uint8       uc_candidate_perf;                                    /* perference data BSSIDƫ��ֵ */
    oal_uint16      us_resv;                                              /* ���ֽڶ��� */
    oal_bssid_infomation_stru   st_bssid_info;                       /* BSSID information */
    dmac_bss_term_duration_stru st_term_duration;                   /* ��Ԫ��Termination duration */
}__OAL_DECLARE_PACKED;
typedef struct dmac_neighbor_bss_info  dmac_neighbor_bss_info_stru;

struct dmac_bsst_req_mode
{
    oal_uint8   bit_candidate_list_include:1,                   /* �Ƿ�����ھ�AP�б� */
                bit_abridged:1,                                 /* 1:��ʾû�а������ھ��б��AP���ȼ�����Ϊ0 */
                bit_bss_disassoc_imminent:1,                    /* �Ƿ񼴽��Ͽ�STA ��0 ֡��ʱ����ȫΪFF FF */
                bit_termination_include:1,                      /* BSS�ս�ʱ��  ��0��֡���в�����BSS Termination Duration�ֶ� */
                bit_ess_disassoc_imminent:1,                    /* EES��ֹʱ�� */
                bit_rev:3;
}__OAL_DECLARE_PACKED;
typedef struct dmac_bsst_req_mode  dmac_bsst_req_mode_stru;

/* bss transition query֡����Ϣ�ṹ�� */
struct dmac_bsst_query_info
{
    oal_uint8       uc_reason;
    oal_uint8       uc_bss_list_num;                      /* bss list������ ���������50�� */
    oal_uint16      us_resv;                                              /* ���ֽڶ��� */
    dmac_neighbor_bss_info_stru *pst_neighbor_bss_list;
}__OAL_DECLARE_PACKED;
typedef struct dmac_bsst_query_info  dmac_bsst_query_info_stru;

/* bss transition request֡����Ϣ�ṹ�� */
struct dmac_bsst_req_info
{
    oal_uint8               uc_validity_interval;                              /* �ھ��б���Чʱ�� TBTTs */
    oal_uint16              us_disassoc_time;                                  /* APȡ����STAʱ�� TBTTs */
    oal_uint8               *puc_session_url;                                  /* Ҫ�����ַ��� �������Ϊ100���ַ� */
    dmac_bsst_req_mode_stru st_request_mode;                                   /* request mode */
    oal_uint8               uc_bss_list_num;
    oal_uint8               uc_resv;                                           /* 4�ֽڶ��� */
    dmac_neighbor_bss_info_stru *pst_neighbor_bss_list;                        /* bss list������ ���������50�� */
    dmac_bss_term_duration_stru st_term_duration;                              /* ��Ԫ��Termination duration */
}__OAL_DECLARE_PACKED;
typedef struct dmac_bsst_req_info  dmac_bsst_req_info_stru;

/* bss transition response֡����Ϣ�ṹ�� */
struct dmac_bsst_rsp_info
{
    oal_uint8       uc_status_code;                         /* ״̬�� ���ջ��߾ܾ� */
    oal_uint8       uc_termination_delay;                   /* Ҫ��AP�Ӻ���ֹʱ��:���� */
    oal_uint8       auc_target_bss_addr[WLAN_MAC_ADDR_LEN]; /* �����л���Ŀ��BSSID */
    oal_uint8       uc_bss_list_num;                        /* bss list�������������Ϊ50�� */
    oal_uint16      us_resv;                                /* ���ֽڶ��� */
    dmac_neighbor_bss_info_stru *pst_neighbor_bss_list;
}__OAL_DECLARE_PACKED;
typedef struct dmac_bsst_rsp_info  dmac_bsst_rsp_info_stru;
/*****************************************************************************
  5 ȫ�ֱ�������
*****************************************************************************/


/*****************************************************************************
  6 ��Ϣͷ����
*****************************************************************************/


/*****************************************************************************
  7 ��Ϣ����
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
/* STA ��װBSS TRANSITION QUERY֡ */
extern oal_uint16  dmac_encap_bsst_query_action(dmac_vap_stru *pst_dmac_vap, dmac_user_stru *pst_dmac_user, dmac_bsst_query_info_stru *pst_bsst_query_info, oal_netbuf_stru *pst_buffer);
/* STA ����BSS TRANSITION QUERY֡ */
extern oal_uint32  dmac_tx_bsst_query_action(dmac_vap_stru *pst_dmac_vap, dmac_user_stru *pst_dmac_user, dmac_bsst_query_info_stru *pst_bsst_query_info);
/* AP ����STA���͵�bss transition query frame */
extern oal_uint32  dmac_rx_bsst_query_action(dmac_vap_stru *pst_dmac_vap, dmac_user_stru *pst_dmac_user, oal_netbuf_stru *pst_netbuf);
/* AP ����bss transition request frame ������һ���ھ�AP�б� ����BSD���� */
extern oal_uint32  dmac_tx_bsst_req_action_one_bss(dmac_vap_stru *pst_dmac_vap, dmac_user_stru *pst_dmac_user, dmac_neighbor_bss_info_stru *pst_bss_neighbor_list, dmac_user_callback_func_11v p_fun_callback);
/* AP ����bss transition request frame */
extern oal_uint32  dmac_tx_bsst_req_action(dmac_vap_stru *pst_dmac_vap, dmac_user_stru *pst_dmac_user, dmac_bsst_req_info_stru *pst_bsst_req_info, dmac_user_callback_func_11v p_fun_callback);
/* AP ��װbss transition request frame */
extern oal_uint16  dmac_encap_bsst_req_action(dmac_vap_stru *pst_dmac_vap,
                                              dmac_user_stru *pst_dmac_user,
                                              dmac_bsst_req_info_stru *pst_bsst_req_info,
                                              oal_netbuf_stru *pst_buffer);
/* AP �ȴ�����bss transition response frame ��ʱ */
extern oal_uint32  dmac_rx_bsst_rsp_timeout(oal_void *p_arg);
/* STA ����AP���͵�bss transition request frame */
extern oal_uint32  dmac_rx_bsst_req_action(dmac_vap_stru *pst_dmac_vap, dmac_user_stru *pst_dmac_user, oal_netbuf_stru *pst_netbuf);
/* STA ����bss transition response frame */
extern oal_uint32  dmac_tx_bsst_rsp_action(dmac_vap_stru * pst_dmac_vap, dmac_user_stru * pst_dmac_user, dmac_bsst_rsp_info_stru * pst_bsst_rsq_info);
/* STA ��װbss transition response frame */
extern oal_uint16  dmac_encap_bsst_rsp_action(dmac_vap_stru *pst_dmac_vap,
                                              dmac_user_stru *pst_dmac_user,
                                              dmac_bsst_rsp_info_stru *pst_bsst_rsp_info,
                                              oal_netbuf_stru *pst_buffer);
/* AP ����STA���͵�bss transition response frame */
extern oal_uint32  dmac_rx_bsst_rsp_action(dmac_vap_stru *pst_dmac_vap, dmac_user_stru *pst_dmac_user, oal_netbuf_stru *pst_netbuf);
/* ��Neighbor Report IE�ṹ���װ��֡������ */
extern oal_void  dmac_set_neighbor_ie(dmac_neighbor_bss_info_stru *pst_neighbor_bss, oal_uint8 uc_bss_num, oal_uint8 *puc_buffer, oal_uint16 *pus_total_ie_len);
/* ��Neighbor Report IE�ṹ���֡�����н������� */
extern oal_void  dmac_get_neighbor_ie(dmac_neighbor_bss_info_stru **pst_bss_list, oal_uint8 *puc_data, oal_uint16 us_len, oal_uint8 *pst_bss_num);

extern oal_uint32  dmac_tx_bsst_req_action_one_bss(dmac_vap_stru *pst_dmac_vap, dmac_user_stru *pst_dmac_user, dmac_neighbor_bss_info_stru *pst_bss_neighbor_list, dmac_user_callback_func_11v p_fun_callback);
#endif //_PRE_WLAN_FEATURE_11V

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of dmac_11v.h */
