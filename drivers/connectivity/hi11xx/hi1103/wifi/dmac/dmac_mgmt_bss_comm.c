


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 ͷ�ļ�����
*****************************************************************************/
#include "oal_ext_if.h"
#include "oal_aes.h"
#include "wlan_spec.h"
#include "wlan_mib.h"

/* TBD�������� ����hal_ext_if.h*/
#include "frw_ext_if.h"

#include "hal_ext_if.h"

//#include "hal_spec.h"
#include "mac_regdomain.h"
#include "mac_ie.h"
#include "mac_frame.h"
#include "dmac_tx_bss_comm.h"
#include "dmac_blockack.h"
#include "dmac_mgmt_bss_comm.h"
#include "dmac_mgmt_sta.h"
#include "dmac_psm_ap.h"
#include "dmac_scan.h"

#ifdef _PRE_WLAN_FEATURE_STA_PM
#include "dmac_psm_sta.h"
#include "pm_extern.h"
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_MGMT_BSS_COMM_C

/*****************************************************************************
  2 ȫ�ֱ�������
*****************************************************************************/

/*****************************************************************************
  3 ����ʵ��
*****************************************************************************/
#if defined(_PRE_WLAN_MAC_BUGFIX_ADDBA_SSN) && (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)

OAL_STATIC oal_uint32   dmac_addbareq_send_null_data(dmac_vap_stru *pst_dmac_vap, dmac_user_stru *pst_dmac_user, oal_uint8 uc_ac_type)
{
    oal_netbuf_stru                 *pst_net_buf;
    mac_tx_ctl_stru                 *pst_tx_ctrl;
    oal_uint32                       ul_ret;
    oal_uint16                       us_tx_direction = WLAN_FRAME_FROM_AP;
    mac_ieee80211_frame_stru        *pst_mac_header;

    /* ����net_buff */
    pst_net_buf = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, WLAN_SHORT_NETBUF_SIZE, OAL_NETBUF_PRIORITY_HIGH);
    if (OAL_PTR_NULL == pst_net_buf)
    {
        OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA, "{dmac_addbareq_send_null_data::alloc netbuff failed.}");
        OAL_MEM_INFO_PRINT(OAL_MEM_POOL_ID_NETBUF);
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    OAL_MEM_NETBUF_TRACE(pst_net_buf, OAL_TRUE);

    oal_set_netbuf_prev(pst_net_buf, OAL_PTR_NULL);
    oal_set_netbuf_next(pst_net_buf, OAL_PTR_NULL);

    /* null֡���ͷ���From AP || To AP */
    us_tx_direction = (WLAN_VAP_MODE_BSS_AP == pst_dmac_vap->st_vap_base_info.en_vap_mode) ? WLAN_FRAME_FROM_AP : WLAN_FRAME_TO_AP;
    /* ��д֡ͷ,����from dsΪ1��to dsΪ0�����frame control�ĵڶ����ֽ�Ϊ02 */
    mac_null_data_encap(OAL_NETBUF_HEADER(pst_net_buf),
                        ((oal_uint16)(WLAN_PROTOCOL_VERSION | WLAN_FC0_TYPE_DATA | WLAN_FC0_SUBTYPE_NODATA) | us_tx_direction),
                        pst_dmac_user->st_user_base_info.auc_user_mac_addr,
                        mac_mib_get_StationID(&pst_dmac_vap->st_vap_base_info));
    pst_mac_header = (mac_ieee80211_frame_stru*)OAL_NETBUF_HEADER(pst_net_buf);

    pst_mac_header->st_frame_control.bit_power_mgmt = 0;


    /* ��дcb�ֶ� */
    pst_tx_ctrl = (mac_tx_ctl_stru *)OAL_NETBUF_CB(pst_net_buf);
    OAL_MEMZERO(pst_tx_ctrl, OAL_SIZEOF(mac_tx_ctl_stru));


    /* ��дtx���� */
    MAC_GET_CB_ACK_POLACY(pst_tx_ctrl)            = WLAN_TX_NORMAL_ACK;
    MAC_GET_CB_EVENT_TYPE(pst_tx_ctrl)            = FRW_EVENT_TYPE_WLAN_DTX;
    MAC_GET_CB_RETRIED_NUM(pst_tx_ctrl)           = 0;
    MAC_GET_CB_WME_TID_TYPE(pst_tx_ctrl)          =  WLAN_TID_FOR_DATA;
    MAC_GET_CB_TX_VAP_INDEX(pst_tx_ctrl)          = pst_dmac_vap->st_vap_base_info.uc_vap_id;
    MAC_GET_CB_TX_USER_IDX(pst_tx_ctrl)           = pst_dmac_user->st_user_base_info.us_assoc_id;

    if (IS_AP(&(pst_dmac_vap->st_vap_base_info)))
    {
        MAC_GET_CB_IS_FROM_PS_QUEUE(pst_tx_ctrl)   = OAL_FALSE;/* AP ����null ֡������ܶ��� */
    }
    else
    {
        MAC_GET_CB_IS_FROM_PS_QUEUE(pst_tx_ctrl)   = OAL_TRUE;
    }
    MAC_SET_CB_FRAME_HEADER_ADDR(pst_tx_ctrl, (mac_ieee80211_frame_stru *)oal_netbuf_header(pst_net_buf));
    MAC_GET_CB_FRAME_HEADER_LENGTH(pst_tx_ctrl)    = OAL_SIZEOF(mac_ieee80211_frame_stru);
    MAC_GET_CB_MPDU_NUM(pst_tx_ctrl)               = 1;
    MAC_GET_CB_NETBUF_NUM(pst_tx_ctrl)             = 1;
    MAC_GET_CB_MPDU_LEN(pst_tx_ctrl)               = 0;
    MAC_GET_CB_WME_AC_TYPE(pst_tx_ctrl) = uc_ac_type;


    ul_ret = dmac_tx_mgmt(pst_dmac_vap, pst_net_buf, OAL_SIZEOF(mac_ieee80211_frame_stru));

    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA,
                         "{dmac_addbareq_send_null_data::dmac_tx_mgmt failed[%d].}", ul_ret);
        dmac_tx_excp_free_netbuf(pst_net_buf);
        return ul_ret;
    }

    return OAL_SUCC;
}
#endif


oal_uint32  dmac_mgmt_tx_addba_req(
                mac_device_stru                *pst_device,
                dmac_vap_stru                  *pst_dmac_vap,
                dmac_ctx_action_event_stru     *pst_ctx_action_event,
                oal_netbuf_stru                *pst_net_buff)
{
    oal_uint8                   uc_tidno;
    dmac_user_stru             *pst_dmac_user;
    dmac_tid_stru              *pst_tid;
    oal_uint16                  us_frame_len;
    mac_tx_ctl_stru            *pst_tx_ctl;
    oal_uint32                  ul_ret;
    oal_uint8                  *puc_data;
    oal_uint8                   uc_ac;

    if ((OAL_PTR_NULL == pst_ctx_action_event) || (OAL_PTR_NULL == pst_net_buff))
    {
        OAM_ERROR_LOG2(0, OAM_SF_BA, "{dmac_mgmt_tx_addba_req::param null, pst_ctx_action_event=%p,pst_net_buff=%p.}",
                       pst_ctx_action_event, pst_net_buff);
        return OAL_ERR_CODE_PTR_NULL;
    }

    uc_tidno                = pst_ctx_action_event->uc_tidno;
    us_frame_len            = pst_ctx_action_event->us_frame_len;

    /* ��ȡ��Ӧ�û�����Ӧ��TID���� */
    pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user(pst_ctx_action_event->us_user_idx);

    if (OAL_PTR_NULL == pst_dmac_user)
    {
        OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA, "{dmac_mgmt_tx_addba_req::pst_dmac_user[%d] null.}",
            pst_ctx_action_event->us_user_idx);

        return OAL_ERR_CODE_PTR_NULL;
    }
    if (uc_tidno >= WLAN_TID_MAX_NUM)
    {
        OAM_ERROR_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA, "{dmac_mgmt_tx_addba_req::invalid uc_tidno[%d].}", uc_tidno);

        return OAL_ERR_CODE_ARRAY_OVERFLOW;
    }

    pst_tid         = &(pst_dmac_user->ast_tx_tid_queue[uc_tidno]);

    /* �����TID�µĶ�Ӧ�ķ���BA����ṹ */
    if(OAL_PTR_NULL != pst_tid->pst_ba_tx_hdl)
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA, "{dmac_mgmt_tx_addba_req::re-malloc mem, memory leak!}");
        dmac_ba_reset_tx_handle(pst_device, &pst_tid->pst_ba_tx_hdl, uc_tidno);
    }

    pst_tid->pst_ba_tx_hdl = (dmac_ba_tx_stru *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_HI_LOCAL, OAL_SIZEOF(dmac_ba_tx_stru), OAL_TRUE);
    if (OAL_PTR_NULL == pst_tid->pst_ba_tx_hdl)
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA, "{dmac_mgmt_tx_addba_req::pst_ba_tx_hdl null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAL_MEMZERO(pst_tid->pst_ba_tx_hdl,  OAL_SIZEOF(dmac_ba_tx_stru));

    /* ͬ��HMAC��DMAC��BA�Ự����Ϣ */
    pst_tid->pst_ba_tx_hdl->en_is_ba          = OAL_TRUE;
    pst_tid->pst_ba_tx_hdl->en_ba_conn_status = DMAC_BA_INPROGRESS;
    pst_tid->pst_ba_tx_hdl->uc_dialog_token   = pst_ctx_action_event->uc_dialog_token;
    pst_tid->pst_ba_tx_hdl->uc_ba_policy      = pst_ctx_action_event->uc_ba_policy;
    pst_tid->pst_ba_tx_hdl->us_baw_size       = pst_ctx_action_event->us_baw_size;
    pst_tid->pst_ba_tx_hdl->us_ba_timeout     = pst_ctx_action_event->us_ba_timeout;
    pst_tid->pst_ba_tx_hdl->us_mac_user_idx   = pst_dmac_user->st_user_base_info.us_assoc_id;

    if (pst_dmac_user->st_user_base_info.st_ht_hdl.en_ht_capable)
    {
        pst_tid->pst_ba_tx_hdl->en_back_var = MAC_BACK_COMPRESSED;
    }
    else
    {
        pst_tid->pst_ba_tx_hdl->en_back_var = MAC_BACK_BASIC;
    }

    dmac_ba_update_start_seq_num(pst_tid->pst_ba_tx_hdl, pst_dmac_user->aus_txseqs[uc_tidno]);
    OAL_MEM_NETBUF_TRACE(pst_net_buff, OAL_TRUE);

    puc_data = mac_netbuf_get_payload(pst_net_buff);

    *(oal_uint16 *)&puc_data[7] = (oal_uint16)((oal_host_to_le16(pst_dmac_user->aus_txseqs[uc_tidno])) << WLAN_SEQ_SHIFT);

    /* ��дnetbuf��cb�ֶΣ������͹���֡�ͷ�����ɽӿ�ʹ�� */
    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_net_buff);
    MAC_GET_CB_TX_USER_IDX(pst_tx_ctl)      = pst_dmac_user->st_user_base_info.us_assoc_id;
    MAC_GET_CB_MPDU_NUM(pst_tx_ctl)         = 1;              /* ����ֻ֡��һ�� */
    MAC_SET_CB_FRAME_HEADER_ADDR(pst_tx_ctl, (mac_ieee80211_frame_stru *)oal_netbuf_header(pst_net_buff));
    MAC_GET_CB_WME_TID_TYPE(pst_tx_ctl)= uc_tidno;
    uc_ac = WLAN_WME_TID_TO_AC(uc_tidno);
    MAC_GET_CB_WME_AC_TYPE(pst_tx_ctl) = uc_ac;

    dmac_tid_pause(pst_tid, DMAC_TID_PAUSE_RESUME_TYPE_BA);

    /*����Ӧ�÷���֮ǰ��netbuff�е�action infoɾ����ͨ������ʵ�ʳ��ȣ�Ӳ��Ӧ�ò��ᷢ�ͺ���Ĳ���*/
    /* TBD */

    /* ���÷��͹���֡�ӿ� */
    ul_ret = dmac_tx_mgmt(pst_dmac_vap, pst_net_buff, us_frame_len);
    if (OAL_SUCC != ul_ret)
    {
        dmac_ba_reset_tx_handle(pst_device, &pst_tid->pst_ba_tx_hdl, uc_tidno);

        return ul_ret;
    }

#if defined(_PRE_WLAN_MAC_BUGFIX_ADDBA_SSN) && (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    dmac_addbareq_send_null_data(pst_dmac_vap, pst_dmac_user,uc_ac);
#endif

    OAM_WARNING_LOG4(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA, "{dmac_mgmt_tx_addba_req::USER ID[%d]TID NO[%d] BAWSIZE[%d] TXAMSDU[%d].}",
                                    pst_tid->pst_ba_tx_hdl->us_mac_user_idx,uc_tidno,
                                    pst_tid->pst_ba_tx_hdl->us_baw_size,pst_ctx_action_event->en_amsdu_supp);

    return ul_ret;
}



oal_uint32  dmac_mgmt_tx_addba_rsp(
                mac_device_stru                *pst_device,
                dmac_vap_stru                  *pst_dmac_vap,
                dmac_ctx_action_event_stru     *pst_ctx_action_event,
                oal_netbuf_stru                *pst_net_buff)
{
    oal_uint8                   uc_tidno;
    dmac_user_stru             *pst_dmac_user = OAL_PTR_NULL;
    dmac_tid_stru              *pst_tid;
    oal_uint8                   uc_status;
    mac_tx_ctl_stru            *pst_tx_ctl;
    oal_uint32                  ul_ret;
    oal_uint8                   uc_lut_index;
    oal_uint16                  us_baw_start;
    oal_uint16                  us_baw_size;
    oal_uint8                  *puc_hw_addr;
    dmac_ba_rx_stru            *pst_ba_rx_hdl;

#ifdef  _PRE_WLAN_FEATURE_RX_AGGR_EXTEND
    mac_chip_stru              *pst_mac_chip;
    oal_uint8                   uc_index;
    mac_chip_ba_lut_stru       *pst_ba_lut;
    oal_uint32                  ul_addr_h;
    oal_uint32                  ul_addr_l;
#endif

    if ((OAL_PTR_NULL == pst_net_buff) || (OAL_PTR_NULL == pst_ctx_action_event))
    {
        OAM_ERROR_LOG2(0, OAM_SF_BA, "{dmac_mgmt_tx_addba_rsp::param null, pst_net_buff=0x%X pst_ctx_action_event=0x%X.}",
                              pst_net_buff, pst_ctx_action_event);
        return OAL_ERR_CODE_PTR_NULL;
    }

#ifdef  _PRE_WLAN_FEATURE_RX_AGGR_EXTEND
    pst_mac_chip = mac_res_get_mac_chip(pst_device->uc_chip_id);
    if(OAL_PTR_NULL == pst_mac_chip)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }
#endif

    uc_tidno            = pst_ctx_action_event->uc_tidno;     //��������Mac Ba lut��ʹ��
    uc_status           = pst_ctx_action_event->uc_status;
    uc_lut_index        = pst_ctx_action_event->uc_lut_index; //��������Mac Ba lut��ʹ��
    us_baw_start        = pst_ctx_action_event->us_baw_start; //��������Mac Ba lut��ʹ��
    us_baw_size         = pst_ctx_action_event->us_baw_size;  //����Ӳ�����ڴ�С��bufsizeһ�£����fir303b����������

    pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user(pst_ctx_action_event->us_user_idx);
    if (OAL_PTR_NULL == pst_dmac_user)
    {
        OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA, "{dmac_mgmt_tx_addba_rsp::send addba rsp failed, pst_dmac_user null tid[%d].}", uc_tidno);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_tid = &(pst_dmac_user->ast_tx_tid_queue[uc_tidno]);
    pst_ba_rx_hdl = &pst_tid->st_ba_rx_hdl;
    /* �ɹ�ʱ��ͬ����Ϣ��DMACģ�飬ʧ��ʱ��ֱ�ӷ���rsp֡����ͬ����DMACģ��  */
    if (MAC_SUCCESSFUL_STATUSCODE == uc_status)
    {
        /* BA�Ự�쳣����Ҫɾ��lut table */
        if(DMAC_BA_INIT != pst_ba_rx_hdl->en_ba_conn_status)
        {
        #ifdef  _PRE_WLAN_FEATURE_RX_AGGR_EXTEND
            if(oal_is_active_lut_index(pst_mac_chip->st_lut_table.auc_rx_ba_lut_idx_status_table, HAL_MAX_RX_BA_LUT_SIZE, pst_ba_rx_hdl->uc_lut_index))
            {
                hal_remove_machw_ba_lut_entry(pst_dmac_vap->pst_hal_device, pst_ba_rx_hdl->uc_lut_index);
                oal_reset_lut_index_status(pst_mac_chip->st_lut_table.auc_rx_ba_lut_idx_status_table, HAL_MAX_RX_BA_LUT_SIZE,pst_ba_rx_hdl->uc_lut_index);
            }
        #else
            hal_remove_machw_ba_lut_entry(pst_dmac_vap->pst_hal_device, pst_ba_rx_hdl->uc_lut_index);
        #endif
        }

        pst_ba_rx_hdl->en_ba_conn_status   = DMAC_BA_COMPLETE;
        pst_ba_rx_hdl->uc_lut_index        = uc_lut_index;
        pst_ba_rx_hdl->uc_ba_policy        = pst_ctx_action_event->uc_ba_policy;

        /* �����öԶ�(����addba req)�ĵ�ַ */
        puc_hw_addr   = pst_dmac_user->st_user_base_info.auc_user_mac_addr;

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
        /* ����MACӲ����Ҫ��,Root AP���͹�����BA req,BA rsp��Ҫ��дProxy STA�ĵ�ַ��*/
        /* ��ȻӲ���޷�ʶ��BA�Ự�Ǻ��ĸ�Proxy STA������ */
        if(mac_is_proxysta_enabled(pst_device) && mac_vap_is_vsta(&pst_dmac_vap->st_vap_base_info))
        {
            puc_hw_addr = mac_mib_get_StationID(&pst_dmac_vap->st_vap_base_info);
        }
#endif
#ifdef  _PRE_WLAN_FEATURE_RX_AGGR_EXTEND
        /* �����ۺϽ׶� ����WLAN_MAX_RX_BA��lut index  ��Ҫ��֮ǰ���滻��ȥ */
        if(pst_mac_chip->en_waveapp_32plus_user_enable && (pst_ba_rx_hdl->uc_lut_index >= WLAN_MAX_RX_BA))
        {
            uc_index = pst_ba_rx_hdl->uc_lut_index - WLAN_MAX_RX_BA;
            if(oal_is_active_lut_index(pst_mac_chip->st_lut_table.auc_rx_ba_lut_idx_status_table, HAL_MAX_RX_BA_LUT_SIZE, uc_index))
            {
                 //������Ҫ�滻��ba lut
                 pst_ba_lut = &(pst_mac_chip->pst_rx_aggr_extend->ast_rx_ba_lut_entry[uc_index]);
                 hal_get_machw_ba_params(pst_dmac_vap->pst_hal_device,
                                    uc_index%WLAN_MAX_RX_BA,
                                    &ul_addr_h,
                                    &ul_addr_l,
                                    &(pst_ba_lut->ul_bitmap_h), &(pst_ba_lut->ul_bitmap_l),
                                    &(pst_ba_lut->ul_ba_param));
                 OAM_WARNING_LOG2(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX, "{dmac_mgmt_tx_addba_rsp::[%d replaced by->%d].}",
                        uc_index, uc_lut_index);
                 oal_reset_lut_index_status(pst_mac_chip->st_lut_table.auc_rx_ba_lut_idx_status_table, HAL_MAX_RX_BA_LUT_SIZE, uc_index);
             }
        }
#endif
        hal_add_machw_ba_lut_entry(pst_dmac_vap->pst_hal_device, uc_lut_index, puc_hw_addr,
                               uc_tidno, us_baw_start, (oal_uint8)us_baw_size);


#ifdef  _PRE_WLAN_FEATURE_RX_AGGR_EXTEND
        oal_set_lut_index_status(pst_mac_chip->st_lut_table.auc_rx_ba_lut_idx_status_table, HAL_MAX_RX_BA_LUT_SIZE, uc_lut_index);
        if(pst_mac_chip->en_waveapp_32plus_user_enable)
        {
            pst_ba_lut = &(pst_mac_chip->pst_rx_aggr_extend->ast_rx_ba_lut_entry[uc_lut_index]);
            pst_ba_lut->ul_addr_h   =  puc_hw_addr[0] << 8;
            pst_ba_lut->ul_addr_h  |=  puc_hw_addr[1];
            pst_ba_lut->ul_addr_l   =  puc_hw_addr[5];
            pst_ba_lut->ul_addr_l  |= (puc_hw_addr[4] << 8);
            pst_ba_lut->ul_addr_l  |= (puc_hw_addr[3] << 16);
            pst_ba_lut->ul_addr_l  |= (puc_hw_addr[2] << 24);

            pst_mac_chip->pst_rx_aggr_extend->auc_hal_to_dmac_lut_index_map[uc_lut_index%WLAN_MAX_RX_BA] = uc_lut_index;

            /* ����BAʱ�����³�ʼ�����wave���û������ռ��滻������ر��� */
            pst_mac_chip->pst_rx_aggr_extend->uc_rx_ba_seq_index = 0;
            pst_mac_chip->pst_rx_aggr_extend->uc_prev_handle_ba_index = 0xFF;
            pst_mac_chip->pst_rx_aggr_extend->us_rx_ba_seq_phase_one_count = 0;
            oal_memset(pst_mac_chip->pst_rx_aggr_extend->auc_rx_ba_seq_to_lut_index_map, 0xFF, HAL_MAX_RX_BA_LUT_SIZE);
        }
#endif

        OAM_WARNING_LOG4(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX, "{dmac_mgmt_tx_addba_rsp::MAC ADDR [%02X:XX:XX:%02X:%02X:%02X].}",
                        puc_hw_addr[0], puc_hw_addr[3], puc_hw_addr[4], puc_hw_addr[5]);
    }

    OAL_MEM_NETBUF_TRACE(pst_net_buff, OAL_TRUE);

    /* ��дnetbuf��cb�ֶΣ������͹���֡�ͷ�����ɽӿ�ʹ�� */
    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_net_buff);
    MAC_GET_CB_TX_USER_IDX(pst_tx_ctl)   = pst_dmac_user->st_user_base_info.us_assoc_id;
    MAC_SET_CB_FRAME_HEADER_ADDR(pst_tx_ctl, (mac_ieee80211_frame_stru *)oal_netbuf_header(pst_net_buff));
    MAC_GET_CB_WME_TID_TYPE(pst_tx_ctl)= uc_tidno;
    MAC_GET_CB_WME_AC_TYPE(pst_tx_ctl) = WLAN_WME_AC_MGMT;

    /* ���÷��͹���֡�ӿ� */
    ul_ret = dmac_tx_mgmt(pst_dmac_vap, pst_net_buff, pst_ctx_action_event->us_frame_len);
    OAM_WARNING_LOG4(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX, "{dmac_mgmt_tx_addba_rsp::TID[%d] STATUS[%d] LUT INDEX[%d] BAWSIZE[%d].}",
                                             uc_tidno, uc_status, uc_lut_index, us_baw_size);

    return ul_ret;
}


oal_uint32  dmac_mgmt_tx_delba(
                mac_device_stru                *pst_device,
                dmac_vap_stru                  *pst_dmac_vap,
                dmac_ctx_action_event_stru     *pst_ctx_action_event,
                oal_netbuf_stru                *pst_net_buff)
{
    dmac_user_stru             *pst_dmac_user;
    dmac_tid_stru              *pst_tid;
    mac_tx_ctl_stru            *pst_tx_ctl;
    oal_uint32                  ul_ret;

    if ((OAL_PTR_NULL == pst_net_buff) || (OAL_PTR_NULL == pst_ctx_action_event))
    {
        OAM_ERROR_LOG2(0, OAM_SF_BA, "{dmac_mgmt_tx_delba::param null,pst_net_buff=%p pst_ctx_action_event=%p.}",
                       pst_net_buff, pst_ctx_action_event);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user(pst_ctx_action_event->us_user_idx);
    if (OAL_PTR_NULL == pst_dmac_user)
    {
        OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA, "{dmac_mgmt_tx_delba::pst_dmac_user[%d] null.}",
            pst_ctx_action_event->us_user_idx);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_tid  = &(pst_dmac_user->ast_tx_tid_queue[pst_ctx_action_event->uc_tidno]);

    /* ����BA�Ự */
    if (MAC_RECIPIENT_DELBA == pst_ctx_action_event->uc_initiator)
    {
        dmac_ba_reset_rx_handle(pst_device, pst_dmac_user, &pst_tid->st_ba_rx_hdl);
    }
    else
    {
        dmac_ba_reset_tx_handle(pst_device, &(pst_tid->pst_ba_tx_hdl), pst_ctx_action_event->uc_tidno);
    }

    if (OAL_TRUE == pst_tid->en_is_delba_ing)
    {
        OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA, "{dmac_mgmt_tx_delba::pst_tid->en_is_delba_ing TRUE.}");
        OAL_MEM_NETBUF_TRACE(pst_net_buff, OAL_TRUE);
        return OAL_ERR_CODE_CONFIG_BUSY;
    }
    else
    {
        pst_tid->en_is_delba_ing = OAL_TRUE;
    }

    /* ��дnetbuf��cb�ֶΣ������͹���֡�ͷ�����ɽӿ�ʹ�� */
    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_net_buff);
    MAC_GET_CB_TX_USER_IDX(pst_tx_ctl)          = pst_dmac_user->st_user_base_info.us_assoc_id;
    MAC_SET_CB_FRAME_HEADER_ADDR(pst_tx_ctl, (mac_ieee80211_frame_stru *)oal_netbuf_header(pst_net_buff));
    MAC_GET_CB_WME_TID_TYPE(pst_tx_ctl)= pst_ctx_action_event->uc_tidno;
    MAC_GET_CB_WME_AC_TYPE(pst_tx_ctl) = WLAN_WME_AC_MGMT;

    /* ���÷��͹���֡�ӿ� */
    ul_ret = dmac_tx_mgmt(pst_dmac_vap, pst_net_buff, pst_ctx_action_event->us_frame_len);
    if (OAL_SUCC != ul_ret)
    {
        return ul_ret;
    }

    return OAL_SUCC;
}



oal_uint32  dmac_mgmt_rx_ampdu_start(
                mac_device_stru                *pst_device,
                dmac_vap_stru                  *pst_dmac_vap,
                mac_priv_req_args_stru         *pst_crx_req_args)
{
    oal_uint8               uc_tidno;
    dmac_user_stru         *pst_dmac_user;
    mac_chip_stru          *pst_mac_chip;
    dmac_tid_stru          *pst_dmac_tid;
    dmac_ht_handle_stru    *pst_ht_hdl;

    if ((OAL_PTR_NULL == pst_device) || (OAL_PTR_NULL == pst_dmac_vap) || (OAL_PTR_NULL == pst_crx_req_args))
    {
        OAM_ERROR_LOG3(0, OAM_SF_AMPDU,
                       "{dmac_mgmt_rx_ampdu_start::param null, pst_device=%d pst_dmac_vap=%d pst_crx_req_args=%d.}",
                       pst_device, pst_dmac_vap, pst_crx_req_args);

        return OAL_ERR_CODE_PTR_NULL;
    }

    uc_tidno        = pst_crx_req_args->uc_arg1;                    /* AMPDU_STARTʱ��uc_arg1�����Ӧ��tid���к� */
    pst_dmac_user   = (dmac_user_stru *)mac_res_get_dmac_user(pst_crx_req_args->us_user_idx);
    if (OAL_PTR_NULL == pst_dmac_user)
    {
        OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_AMPDU, "{dmac_mgmt_rx_ampdu_start::pst_dmac_user[%d] null.}",
            pst_crx_req_args->us_user_idx);

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_dmac_tid    = &(pst_dmac_user->ast_tx_tid_queue[uc_tidno]);  /* ��ȡ��Ӧ��TID���� */
    pst_ht_hdl      = &(pst_dmac_tid->st_ht_tx_hdl);

    /* AMPDU_STARTʱ��uc_arg3�����Ӧ��ȷ�ϲ��� */
    if (WLAN_TX_NORMAL_ACK == pst_crx_req_args->uc_arg3)
    {
        if ((OAL_PTR_NULL == pst_dmac_tid->pst_ba_tx_hdl) ||
           ((pst_dmac_tid->pst_ba_tx_hdl != OAL_PTR_NULL) && (DMAC_BA_COMPLETE != pst_dmac_tid->pst_ba_tx_hdl->en_ba_conn_status)))
        {
            OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_AMPDU, "{dmac_mgmt_rx_ampdu_start::ba session is not comp.}");
            return OAL_FAIL;
        }
    }

    if (OAL_TRUE == pst_dmac_vap->st_vap_base_info.st_cap_flag.bit_rifs_tx_on)
    {
        pst_dmac_tid->en_tx_mode  = DMAC_TX_MODE_RIFS;
    }
    else
    {
        pst_dmac_tid->en_tx_mode  = DMAC_TX_MODE_AGGR;
    }

    /* ��ȡht����ampdu�ۺϸ��� */
    pst_dmac_tid->st_ht_tx_hdl.us_ampdu_max_size = (oal_uint16)(1U << (13 + pst_dmac_user->st_user_base_info.st_ht_hdl.uc_max_rx_ampdu_factor)) - 1;

    OAM_INFO_LOG2(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA, "{dmac_mgmt_rx_ampdu_start::ampdu max size=%d, en_tx_mode=%d.}",
                  pst_dmac_tid->st_ht_tx_hdl.us_ampdu_max_size, pst_dmac_tid->en_tx_mode);

    if ((WLAN_VHT_MODE == pst_dmac_user->st_user_base_info.en_protocol_mode)
     || (WLAN_VHT_ONLY_MODE == pst_dmac_user->st_user_base_info.en_protocol_mode))
    {
        pst_dmac_tid->st_ht_tx_hdl.ul_ampdu_max_size_vht = (oal_uint32)(1 << (13 + pst_dmac_user->st_user_base_info.st_vht_hdl.bit_max_ampdu_len_exp)) - 1;

        OAM_INFO_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA, "{dmac_mgmt_rx_ampdu_start::ampdu max size=%d}",
                      pst_dmac_tid->st_ht_tx_hdl.us_ampdu_max_size);
    }


    dmac_nontxop_txba_num_updata(pst_dmac_user, uc_tidno, OAL_TRUE);

    pst_ht_hdl->uc_ampdu_max_num = pst_crx_req_args->uc_arg2;   /* AMPDU_STARTʱ��uc_arg2����Զ˿ɽ��յ�MPDU�����ֵ */

    /* NORMAL ACK����£�����BA�Ự����Ϣ */
    if (WLAN_TX_NORMAL_ACK == pst_crx_req_args->uc_arg3)
    {
        if (WLAN_VAP_MODE_BSS_AP == dmac_vap_get_bss_type((mac_vap_stru *)pst_dmac_vap))
        {
            dmac_update_txba_session_params_ap(pst_dmac_tid, pst_ht_hdl->uc_ampdu_max_num);
        }
    }

    pst_mac_chip = dmac_res_get_mac_chip(pst_device->uc_chip_id);
    if(OAL_PTR_NULL == pst_mac_chip)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ����LUT index */
    pst_dmac_tid->pst_ba_tx_hdl->uc_tx_ba_lut = oal_get_lut_index(pst_mac_chip->st_lut_table.auc_tx_ba_index_table, MAC_TX_BA_LUT_BMAP_LEN, HAL_MAX_TX_BA_LUT_SIZE, 0, HAL_MAX_TX_BA_LUT_SIZE);

    /* ��дLUT�� */
    hal_add_machw_tx_ba_lut_entry(pst_dmac_vap->pst_hal_device,
                                  pst_dmac_tid->pst_ba_tx_hdl->uc_tx_ba_lut,
                                  uc_tidno,
                                  pst_dmac_tid->pst_ba_tx_hdl->us_baw_start,
                                  (oal_uint8)pst_dmac_tid->pst_ba_tx_hdl->us_baw_size,
                                  pst_dmac_user->st_user_base_info.st_ht_hdl.uc_min_mpdu_start_spacing);

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  dmac_mgmt_rx_succstatus_addba_rsp(
                mac_device_stru                *pst_device,
                dmac_vap_stru                  *pst_dmac_vap,
                dmac_ctx_action_event_stru     *pst_crx_action_event,
                dmac_user_stru                 *pst_dmac_user)
{
    dmac_tid_stru              *pst_tid;
    oal_uint8                   uc_tidno;
    mac_priv_req_args_stru      st_ampdu_req_arg;
    oal_uint32                  ul_ret;
    oal_uint8                   uc_ampdu_max_num;
#ifdef _PRE_WLAN_FEATURE_AMPDU_TX_HW
    hal_to_dmac_device_stru    *pst_hal_device;
#endif

    uc_tidno      = pst_crx_action_event->uc_tidno;
    pst_tid       = &(pst_dmac_user->ast_tx_tid_queue[uc_tidno]);

    if(((pst_tid->pst_ba_tx_hdl->uc_dialog_token != pst_crx_action_event->uc_dialog_token) ||
       (pst_tid->pst_ba_tx_hdl->uc_ba_policy != pst_crx_action_event->uc_ba_policy))
      &&(pst_tid->pst_ba_tx_hdl->en_is_ba))
    {
        OAM_WARNING_LOG4(0, OAM_SF_BA, "{dmac_mgmt_rx_succstatus_addba_rsp::req token=%d,rsp token=%d,req policy=%d,rsp policy=%d}",
            pst_tid->pst_ba_tx_hdl->uc_dialog_token, pst_crx_action_event->uc_dialog_token,
            pst_tid->pst_ba_tx_hdl->uc_ba_policy, pst_crx_action_event->uc_ba_policy);
        return OAL_FAIL;
    }

    /* ����BA���ڵ�TIMEOUTʱ�� */
    pst_tid->pst_ba_tx_hdl->us_ba_timeout = pst_crx_action_event->us_ba_timeout;

#ifdef _PRE_WLAN_FEATURE_AMPDU_TX_HW
    pst_hal_device = pst_dmac_vap->pst_hal_device;
    if(OAL_PTR_NULL == pst_hal_device)
    {
        OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA, "{dmac_mgmt_rx_succstatus_addba_rsp::pst_hal_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if(OAL_TRUE == pst_hal_device->en_ampdu_tx_hw_en)
    {
        uc_ampdu_max_num = (oal_uint8)pst_crx_action_event->us_baw_size;
    }
    else
#endif
    {
        uc_ampdu_max_num = (oal_uint8)pst_crx_action_event->us_baw_size/WLAN_AMPDU_TX_SCHD_STRATEGY;
    }

    /* ����BA�Ự�£��ۺ�AMPDU�������� */
    pst_tid->pst_ba_tx_hdl->uc_ampdu_max_num = OAL_MAX(uc_ampdu_max_num, 1);

    /* ����BA���ڵ�SIZEֵ */
    if ((0 == pst_tid->pst_ba_tx_hdl->us_baw_size)
      || (pst_tid->pst_ba_tx_hdl->us_baw_size > pst_crx_action_event->us_baw_size))
    {
        pst_tid->pst_ba_tx_hdl->us_baw_size = pst_crx_action_event->us_baw_size;
    }

    /* ����BA�Ự�Ķ�AMSDU��֧�� */
    pst_tid->pst_ba_tx_hdl->en_amsdu_supp = pst_tid->pst_ba_tx_hdl->en_amsdu_supp & pst_crx_action_event->en_amsdu_supp;

    /* ���ý��ն˵�ַ */
    pst_tid->pst_ba_tx_hdl->puc_dst_addr = pst_dmac_user->st_user_base_info.auc_user_mac_addr;

    /* ����BA�ĻỰ״̬ */
    pst_tid->pst_ba_tx_hdl->en_ba_conn_status = DMAC_BA_COMPLETE;

#ifdef _PRE_WLAN_FEATURE_DFR
    /* ����BA������ͳ�Ƴ�ֵ */
    pst_tid->pst_ba_tx_hdl->us_pre_baw_start    = 0xffff;
    pst_tid->pst_ba_tx_hdl->us_pre_last_seq_num = 0xffff;
    pst_tid->pst_ba_tx_hdl->us_ba_jamed_cnt     = 0;
#endif

    /* ��ʼ������babitmap���� */
    OAL_MEMZERO(pst_tid->pst_ba_tx_hdl->aul_tx_buf_bitmap, DMAC_TX_BUF_BITMAP_WORDS * OAL_SIZEOF(oal_uint32));

    /*
        ��ǰ�汾����BA�Ự��ֻ�ܷ���AMPDU֡����ˣ�������ampdu�������Ϣ
        ����AMPDU��ʼʱ��st_req_arg�ṹ������Ա��������
    */
    st_ampdu_req_arg.uc_type  = MAC_A_MPDU_START;
    st_ampdu_req_arg.uc_arg1  = uc_tidno;                                       /* ������֡��Ӧ��TID�� */
    st_ampdu_req_arg.uc_arg2  = (oal_uint8)pst_crx_action_event->us_baw_size;   /* �û������� */
    st_ampdu_req_arg.uc_arg3  = WLAN_TX_NORMAL_ACK;                             /* AMPDU֡��ȷ�ϲ��� */
    st_ampdu_req_arg.us_user_idx  = pst_dmac_user->st_user_base_info.us_assoc_id;  /* ��Ҫ����AMPDU��Ӧ���û� */

    OAM_INFO_LOG4(0, OAM_SF_BA, "{uc_dialog_token %d; us_ba_timeout %d; uc_ba_policy %d; us_baw_size %d .}",
                  pst_crx_action_event->uc_dialog_token, pst_tid->pst_ba_tx_hdl->us_ba_timeout,
                  pst_crx_action_event->uc_ba_policy, pst_tid->pst_ba_tx_hdl->us_baw_size);

    OAM_INFO_LOG2(0, OAM_SF_BA, "{en_amsdu_supp %d; uc_ampdu_max_num %d.}",
                  pst_tid->pst_ba_tx_hdl->en_amsdu_supp, pst_tid->pst_ba_tx_hdl->uc_ampdu_max_num);

    ul_ret = dmac_mgmt_rx_ampdu_start(pst_device, pst_dmac_vap, &st_ampdu_req_arg);
    if (ul_ret != OAL_SUCC)
    {
        OAM_WARNING_LOG1(0, OAM_SF_BA, "{dmac_mgmt_rx_succstatus_addba_rsp::ul_ret=%d}", ul_ret);
        return ul_ret;
    }

    dmac_alg_delete_ba_fail_notify(&(pst_dmac_user->st_user_base_info));

    /* �ɹ�ʱ������TID����״̬ */
    dmac_tid_resume(pst_dmac_vap->pst_hal_device, pst_tid, DMAC_TID_PAUSE_RESUME_TYPE_BA);

    return OAL_SUCC;
}


oal_uint32  dmac_mgmt_rx_addba_rsp(
                mac_device_stru                *pst_device,
                dmac_vap_stru                  *pst_dmac_vap,
                dmac_ctx_action_event_stru     *pst_crx_action_event)
{
    dmac_user_stru             *pst_dmac_user;
    dmac_tid_stru              *pst_tid;
    oal_uint8                   uc_tidno;
    oal_uint32                  ul_ret;

    if ((OAL_PTR_NULL == pst_device) || (OAL_PTR_NULL == pst_dmac_vap) || (OAL_PTR_NULL == pst_crx_action_event))
    {
        OAM_ERROR_LOG3(0, OAM_SF_BA,
                       "{dmac_mgmt_tx_delba::param null, pst_device=%d pst_dmac_vap=%d pst_crx_action_event=%d.}",
                       pst_device, pst_dmac_vap, pst_crx_action_event);

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user(pst_crx_action_event->us_user_idx);
    if (OAL_PTR_NULL == pst_dmac_user)
    {
        OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA, "{dmac_mgmt_rx_addba_rsp::pst_dmac_user[%d] null.}",
            pst_crx_action_event->us_user_idx);

        return OAL_ERR_CODE_PTR_NULL;
    }

    uc_tidno      = pst_crx_action_event->uc_tidno;
    if (uc_tidno >= WLAN_TID_MAX_NUM)
    {
        OAM_ERROR_LOG1(pst_dmac_user->st_user_base_info.uc_vap_id, OAM_SF_BA, "{dmac_mgmt_rx_addba_rsp::tidno over flow:%d}", uc_tidno);
        return OAL_ERR_CODE_ARRAY_OVERFLOW;
    }

    pst_tid   = &(pst_dmac_user->ast_tx_tid_queue[uc_tidno]);
    if (OAL_PTR_NULL == pst_tid->pst_ba_tx_hdl)
    {
        OAM_ERROR_LOG1(pst_dmac_user->st_user_base_info.uc_vap_id, OAM_SF_BA, "{dmac_mgmt_rx_addba_rsp::pst_ba_tx_hdl null. tidno:%d}", uc_tidno);

        return OAL_ERR_CODE_PTR_NULL;
    }

    if (pst_crx_action_event->uc_status == MAC_SUCCESSFUL_STATUSCODE)
    {
        ul_ret = dmac_mgmt_rx_succstatus_addba_rsp(pst_device, pst_dmac_vap, pst_crx_action_event, pst_dmac_user);
        if (OAL_SUCC != ul_ret)
        {
            dmac_mgmt_delba(pst_dmac_vap, pst_dmac_user, uc_tidno, MAC_ORIGINATOR_DELBA, MAC_UNSPEC_QOS_REASON);
            return OAL_SUCC;
        }

        OAM_WARNING_LOG4(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA, "{dmac_mgmt_rx_addba_rsp::USER ID[%d]TID NO[%d] AMPDU[%d] BAWSIZE[%d].}",
                                    pst_tid->pst_ba_tx_hdl->us_mac_user_idx,uc_tidno,
                                    pst_tid->pst_ba_tx_hdl->uc_ampdu_max_num,pst_tid->pst_ba_tx_hdl->us_baw_size);
    }
    else
    {
        OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA, "{dmac_mgmt_rx_addba_rsp::ADDBA failed,rsp status[0x%x].}",pst_crx_action_event->uc_status);

        /* ״̬���ɹ���ɾ����ǰ��BA�Ự�ľ�� */
        dmac_ba_reset_tx_handle(pst_device, &(pst_tid->pst_ba_tx_hdl), uc_tidno);
    }

    return OAL_SUCC;
}


oal_uint32  dmac_mgmt_rx_delba(
                mac_device_stru                *pst_device,
                dmac_vap_stru                  *pst_dmac_vap,
                dmac_ctx_action_event_stru     *pst_crx_action_event)
{
    dmac_user_stru     *pst_dmac_user;
    dmac_tid_stru      *pst_tid;
    oal_uint8           uc_tidno;
    oal_uint8           uc_initiator;

    if ((OAL_PTR_NULL == pst_device) || (OAL_PTR_NULL == pst_dmac_vap) || (OAL_PTR_NULL == pst_crx_action_event))
    {
        OAM_ERROR_LOG3(0, OAM_SF_BA,
                       "{dmac_mgmt_rx_delba::param null, pst_device=%d pst_dmac_vap=%d pst_crx_action_event=%d.}",
                       pst_device, pst_dmac_vap, pst_crx_action_event);

        return OAL_ERR_CODE_PTR_NULL;
    }

    //OAM_INFO_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA, "{dmac_mgmt_rx_delba::enter func.}\r\n");

    uc_tidno      = pst_crx_action_event->uc_tidno;
    uc_initiator  = pst_crx_action_event->uc_initiator;

    pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user(pst_crx_action_event->us_user_idx);
    if (OAL_PTR_NULL == pst_dmac_user)
    {
        OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA, "{dmac_mgmt_rx_delba::pst_dmac_user[%d] null.}",
            pst_crx_action_event->us_user_idx);

        return OAL_ERR_CODE_PTR_NULL;
    }

    if (uc_tidno >= WLAN_TID_MAX_NUM)
    {
        OAM_ERROR_LOG1(pst_dmac_user->st_user_base_info.uc_vap_id, OAM_SF_BA, "{dmac_mgmt_rx_delba::tidno over flow:%d}", uc_tidno);
        return OAL_ERR_CODE_ARRAY_OVERFLOW;
    }

    pst_tid       = &(pst_dmac_user->ast_tx_tid_queue[uc_tidno]);

    if (MAC_RECIPIENT_DELBA == uc_initiator)
    {
        /* ����BA TX����Ϣ */
        dmac_ba_reset_tx_handle(pst_device, &(pst_tid->pst_ba_tx_hdl), uc_tidno);
    }
    else
    {
        /* ����BA RX����Ϣ */
        dmac_ba_reset_rx_handle(pst_device, pst_dmac_user, &pst_tid->st_ba_rx_hdl);
    }

    return OAL_SUCC;
}


oal_uint32  dmac_mgmt_send_csa_action(dmac_vap_stru *pst_dmac_vap, oal_uint8 uc_channel, oal_uint8 uc_csa_cnt, wlan_channel_bandwidth_enum_uint8 en_bw)
{
    oal_netbuf_stru        *pst_mgmt_buf;
    oal_uint16              us_mgmt_len;
    mac_tx_ctl_stru        *pst_tx_ctl;
    oal_uint32              ul_ret;

    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{dmac_mgmt_send_csa_action::pst_dmac_vap null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    //OAM_INFO_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "dmac_mgmt_send_csa_action: prepare to send Channel Switch Announcement frame.");

    /* ����Ӧ����HT(11n)���ܷ��͸�֡ */
    if (OAL_TRUE != mac_mib_get_HighThroughputOptionImplemented(&pst_dmac_vap->st_vap_base_info))
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{dmac_mgmt_send_csa_action::vap not in HT mode.}");

        return OAL_FAIL;
    }

    /* �������֡�ڴ� */
    pst_mgmt_buf = OAL_MEM_NETBUF_ALLOC(OAL_MGMT_NETBUF, WLAN_MGMT_NETBUF_SIZE, OAL_NETBUF_PRIORITY_HIGH);
    if (OAL_PTR_NULL == pst_mgmt_buf)
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{dmac_mgmt_send_csa_action::pst_mgmt_buf null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_set_netbuf_prev(pst_mgmt_buf, OAL_PTR_NULL);
    oal_set_netbuf_next(pst_mgmt_buf,OAL_PTR_NULL);
    OAL_MEM_NETBUF_TRACE(pst_mgmt_buf, OAL_TRUE);

    /* ��װ Channel Switch Announcement ֡ */
    us_mgmt_len = dmac_mgmt_encap_csa_action(pst_dmac_vap, pst_mgmt_buf, uc_channel, uc_csa_cnt, en_bw);
    if (0 == us_mgmt_len)
    {
        oal_netbuf_free(pst_mgmt_buf);
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{dmac_mgmt_send_csa_action::encap csa action failed.}");
        return OAL_FAIL;
    }

    /* ��дnetbuf��cb�ֶΣ������͹���֡�ͷ�����ɽӿ�ʹ�� */
    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_mgmt_buf);

    OAL_MEMZERO(pst_tx_ctl, sizeof(mac_tx_ctl_stru));
    MAC_GET_CB_TX_USER_IDX(pst_tx_ctl)  = pst_dmac_vap->st_vap_base_info.us_multi_user_idx; /* channel switch֡�ǹ㲥֡ */
    MAC_GET_CB_IS_MCAST(pst_tx_ctl)  = OAL_TRUE;
    MAC_GET_CB_WME_AC_TYPE(pst_tx_ctl) = WLAN_WME_AC_MGMT;

    /* ���÷��͹���֡�ӿ� */
    ul_ret = dmac_tx_mgmt(pst_dmac_vap, pst_mgmt_buf, us_mgmt_len);
    if (OAL_SUCC != ul_ret)
    {
        oal_netbuf_free(pst_mgmt_buf);
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{dmac_mgmt_send_csa_action::tx csa action failed.}");
        return ul_ret;
    }
    return OAL_SUCC;
}


oal_uint32  dmac_mgmt_scan_vap_down(mac_vap_stru *pst_mac_vap)
{
    mac_device_stru *pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{dmac_mgmt_scan_vap_down::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* vap down��ʱ������豸����ɨ��״̬������down����vap��ɨ���vap��ͬһ������ֹͣɨ�� */
    if ((MAC_SCAN_STATE_RUNNING == pst_mac_device->en_curr_scan_state) &&
        (pst_mac_vap->uc_vap_id == pst_mac_device->st_scan_params.uc_vap_id))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{dmac_mgmt_scan_vap_down::stop scan.}");
        dmac_scan_abort(pst_mac_device);
    }

    return OAL_SUCC;
}



#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif
