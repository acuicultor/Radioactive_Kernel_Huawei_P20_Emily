

#ifndef __DMAC_MAIN_H__
#define __DMAC_MAIN_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 ����ͷ�ļ�����
*****************************************************************************/
#include "oal_ext_if.h"
#include "oam_ext_if.h"
#include "wlan_spec.h"
#include "frw_ext_if.h"
#include "mac_device.h"
#include "dmac_vap.h"
#include "mac_resource.h"
#include "mac_vap.h"
#include "dmac_resource.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_MAIN_H

/*****************************************************************************
  2 �궨��
*****************************************************************************/
typedef oal_void  (*dmac_set_dscr_func)(oal_int32,oal_uint8,dmac_vap_stru*);

/*��麯������ֵ�Ƿ���OAL_SUCC*/
/*lint -e607*/
#define DMAC_CHECK_RET(ul_ret)        \
    if (OAL_SUCC != ul_ret)                      \
    {                                            \
        return ul_ret;                           \
    }

#define DMAC_CHECK_NULL(ptr)        \
    if (OAL_PTR_NULL == ptr)                     \
    {                                            \
        return OAL_ERR_CODE_PTR_NULL;            \
    }

/*lint +e607*/

/* ��дDMAC��HMAC����ͬ����Ϣͷ */
#define DMAC_INIT_SYN_MSG_HDR(_pst_syn_msg, _en_syn_id, _us_len) \
    do {                                            \
        (_pst_syn_msg)->en_syn_id = (_en_syn_id);   \
        (_pst_syn_msg)->us_len = (_us_len);   \
    } while(0)
#ifdef _PRE_WLAN_FEATURE_CSI
#define DMAC_CSI_20M_NONHT_NUM                (52)
#define DMAC_CSI_20M_SUBCARRIER_NUM           (56)
#define DMAC_CSI_40M_SUBCARRIER_NUM           (114)
#define DMAC_CSI_80M_SUBCARRIER_NUM           (242)
#define DMAC_CSI_160M_SUBCARRIER_NUM          (484)

#endif


/*****************************************************************************
  3 ö�ٶ���
*****************************************************************************/
typedef enum
{
    FRW_DMAC_TO_HMAC_UPDATE_USER_TIMESTAMP,  /* ����USERʱ��� */

    FRW_DMAC_TO_HMAC_BUTT
}frw_dmac_to_hmac_syn_enum;
typedef oal_uint16 frw_dmac_to_hmac_syn_enum_uint16;

/*dmac��ʱ��ά���ṹ��*/
typedef struct
{
    oal_time_us_stru  st_timestamp_us;        /* ���������ṩ��ʱ��� */
    oal_uint32        ul_last_timestamp;      /* �������һ�ε�Ӳ��ʱ���*/
    frw_timeout_stru  st_timer;               /* ��ʱ�� */

}dmac_timeStamp_stru;
#if 0
#ifdef _PRE_WLAN_FEATURE_CSI
/*  csi info�ṹ�� */
typedef struct
{
    oal_uint32        ul_nsts0_nrx0;
    oal_uint32        ul_nsts0_nrx1;
    oal_uint32        ul_nsts1_nrx0;
    oal_uint32        ul_nsts1_nrx1;

}dmac_csi_info_stru;
#endif
#endif
/*****************************************************************************
  4 ȫ�ֱ�������
*****************************************************************************/
extern mac_board_stru g_st_dmac_board;


/*****************************************************************************
  5 ��Ϣͷ����
*****************************************************************************/


/*****************************************************************************
  6 ��Ϣ����
*****************************************************************************/


/*****************************************************************************
  7 STRUCT����
*****************************************************************************/


/* ��ȡ�����üĴ����Ľṹ�� */
typedef struct
{
    oal_uint32      ul_addr;
    oal_uint16      us_len;
    oal_uint8       uc_mode;
    oal_uint8       auc_resv[1];
    oal_uint32      ul_reg_val;
}dmac_sdt_reg_frame_stru;

#ifdef _PRE_WLAN_DFT_REG
oal_uint32 dmac_reg_report(frw_event_mem_stru *pst_event_mem);
#endif

/*****************************************************************************
  8 UNION����
*****************************************************************************/


/*****************************************************************************
  9 OTHERS����
*****************************************************************************/

OAL_STATIC OAL_INLINE oal_void  dmac_excp_free_mgmt_frame(oal_netbuf_stru *pst_netbuf)
{
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    oal_netbuf_free(pst_netbuf);
#endif
}


OAL_STATIC OAL_INLINE oal_void dmac_param_check(oal_void)
{
    /*netbuf's cb size!*/
    OAL_BUILD_BUG_ON(OAL_NETBUF_CB_SIZE() < OAL_SIZEOF(mac_tx_ctl_stru));

    OAL_BUILD_BUG_ON(OAL_NETBUF_CB_SIZE() < OAL_SIZEOF(mac_rx_ctl_stru));
}

/*****************************************************************************
  10 ��������
*****************************************************************************/
extern oal_void  dmac_main_exit(oal_void);
extern oal_int32  dmac_main_init(oal_void);
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
extern oal_uint32 dmac_init_event_process(frw_event_mem_stru *pst_event_mem);
#endif
#ifdef _PRE_WLAN_REALTIME_CALI
extern oal_uint32  dmac_rf_realtime_cali_timeout(oal_void * p_arg);
extern oal_uint16 g_us_dync_cali_num;
#endif
extern oal_uint32  dmac_sdt_recv_reg_cmd(frw_event_mem_stru *pst_event_mem);
extern oal_void dmac_timestamp_init(oal_void);
extern oal_void dmac_timestamp_exit(oal_void);
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of dmac_main */
