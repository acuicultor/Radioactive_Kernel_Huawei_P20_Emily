


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_UAPSD

/*****************************************************************************
  1 ͷ�ļ�����
*****************************************************************************/
#include "oal_net.h"
#include "wlan_spec.h"
#include "hal_ext_if.h"
#include "mac_frame.h"
#include "mac_vap.h"
#include "mac_device.h"
#include "dmac_vap.h"
#include "dmac_main.h"
#include "dmac_user.h"
#include "dmac_psm_ap.h"
#include "dmac_tx_bss_comm.h"
#include "dmac_rx_data.h"
#include "dmac_uapsd.h"
#include "dmac_config.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_UAPSD_ROM_C

#define  DMAC_UAPSD_INVALID_TRIGGER_SEQ 0xffff

oal_void  dmac_uapsd_print_info(dmac_user_stru *pst_dmac_user);

/*****************************************************************************
  2 ȫ�ֱ�������
*****************************************************************************/

/*****************************************************************************
  3 ����ʵ��
*****************************************************************************/


oal_uint32  dmac_uapsd_user_init(dmac_user_stru * pst_dmac_usr)
{
    dmac_user_uapsd_stru       *pst_uapsd_stru;
    /*�����߱�֤���ָ��ǿ�*/
    pst_uapsd_stru = &(pst_dmac_usr->st_uapsd_stru);
#ifdef _PRE_WLAN_DFT_STAT
    if (OAL_PTR_NULL != pst_uapsd_stru->pst_uapsd_statis)
    {
        OAL_MEM_FREE(pst_uapsd_stru->pst_uapsd_statis, OAL_TRUE);
        pst_uapsd_stru->pst_uapsd_statis = OAL_PTR_NULL;
    }
    pst_uapsd_stru->pst_uapsd_statis = OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL,OAL_SIZEOF(dmac_usr_uapsd_statis_stru),OAL_TRUE);
    if(OAL_PTR_NULL == pst_uapsd_stru->pst_uapsd_statis)
    {
        OAM_ERROR_LOG2(pst_dmac_usr->st_user_base_info.uc_vap_id, OAM_SF_PWR, "{dmac_uapsd_user_init:: user[%d] alloc uapsd_statis mem fail, size[%d]!}",
                    pst_dmac_usr->st_user_base_info.us_assoc_id,
                    OAL_SIZEOF(dmac_usr_uapsd_statis_stru));
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }
    OAL_MEMZERO(pst_uapsd_stru->pst_uapsd_statis, OAL_SIZEOF(dmac_usr_uapsd_statis_stru));
#endif
    oal_spin_lock_init(&(pst_uapsd_stru->st_lock_uapsd));
    oal_netbuf_list_head_init(&(pst_uapsd_stru->st_uapsd_queue_head));
    oal_atomic_set(&pst_uapsd_stru->uc_mpdu_num,0);
    pst_uapsd_stru->us_uapsd_trigseq[WLAN_WME_AC_BK] = DMAC_UAPSD_INVALID_TRIGGER_SEQ;
    pst_uapsd_stru->us_uapsd_trigseq[WLAN_WME_AC_BE] = DMAC_UAPSD_INVALID_TRIGGER_SEQ;
    pst_uapsd_stru->us_uapsd_trigseq[WLAN_WME_AC_VI] = DMAC_UAPSD_INVALID_TRIGGER_SEQ;
    pst_uapsd_stru->us_uapsd_trigseq[WLAN_WME_AC_VO] = DMAC_UAPSD_INVALID_TRIGGER_SEQ;

    /*��Ҫ��ʼ��mac_usr�ṹ�в���*/
    OAL_MEMZERO(&(pst_dmac_usr->st_uapsd_status),OAL_SIZEOF(mac_user_uapsd_status_stru));
    pst_dmac_usr->uc_uapsd_flag = 0;

    return OAL_SUCC;
}



oal_uint8 dmac_uapsd_tx_need_enqueue(dmac_vap_stru *pst_dmac_vap,dmac_user_stru *pst_dmac_user, mac_tx_ctl_stru *pst_tx_ctl)
{
    wlan_wme_ac_type_enum_uint8 uc_ac;

    /* ����״̬ʱ��ǿ�Ʒ��͸�����֡ */
#ifdef _PRE_WLAN_FEATURE_ROAM
    if (MAC_VAP_STATE_ROAMING == pst_dmac_vap->st_vap_base_info.en_vap_state)
    {
        return OAL_FALSE;
    }
#endif

    uc_ac = MAC_GET_CB_WME_AC_TYPE(pst_tx_ctl);

    if(mac_vap_get_uapsd_en(&pst_dmac_vap->st_vap_base_info)
        &&(OAL_FALSE == MAC_GET_CB_IS_MCAST(pst_tx_ctl))
        &&(OAL_FALSE == MAC_GET_CB_IS_FROM_PS_QUEUE(pst_tx_ctl))
        &&(MAC_USR_UAPSD_AC_TIGGER(uc_ac,pst_dmac_user)))
    {
        return OAL_TRUE;
    }

    return OAL_FALSE;
}


oal_uint32 dmac_uapsd_tx_enqueue(dmac_vap_stru *pst_dmac_vap,dmac_user_stru *pst_dmac_user,oal_netbuf_stru *pst_net_buf)
{
	oal_netbuf_stru        *pst_first_net_buf;
    oal_netbuf_stru        *pst_netbuf;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
    oal_uint8               uc_netbuf_num_in_mpdu;
#endif
    mac_tx_ctl_stru        *pst_tx_ctrl;
    oal_uint8               uc_max_qdepth;
    mac_ieee80211_frame_stru *pst_frame_hdr = OAL_PTR_NULL;
#ifdef _PRE_WLAN_DFT_STAT
    dmac_usr_uapsd_statis_stru *pst_uapsd_statis;

    pst_uapsd_statis = pst_dmac_user->st_uapsd_stru.pst_uapsd_statis;
    if(OAL_PTR_NULL == pst_uapsd_statis)
    {
        OAM_ERROR_LOG0(pst_dmac_user->st_user_base_info.uc_vap_id, OAM_SF_PWR, "{dmac_uapsd_tx_enqueue::pst_uapsd_statis null.}");
        return OAL_FAIL;
    }
#endif

    DMAC_UAPSD_STATS_INCR(pst_uapsd_statis->ul_uapsd_tx_enqueue_count);

    /* ��ȡU-APSD���е���������Ⱥ͵�ǰ���г��� */
    uc_max_qdepth = pst_dmac_vap->uc_uapsd_max_depth;

    /* �ж��Ƿ���Ҫ����bitmap */
    if ((0 == oal_atomic_read(&pst_dmac_user->st_uapsd_stru.uc_mpdu_num))
        && MAC_USR_UAPSD_USE_TIM(pst_dmac_user))
    {
        /* ����PSM��TIM���ýӿ� */
        //OAM_INFO_LOG1(pst_dmac_user->st_user_base_info.uc_vap_id, OAM_SF_TX,
        //              "dmac_uapsd_rx_trigger_check:set PVB to 1,usr id = %d",
        //              pst_dmac_user->st_user_base_info.us_assoc_id);

        dmac_psm_set_local_bitmap(pst_dmac_vap, pst_dmac_user, 1);
    }

    //OAM_INFO_LOG1(pst_dmac_user->st_user_base_info.uc_vap_id, OAM_SF_TX,
    //               "dmac_uapsd_tx_enqueue:before enqueue, the num of mpdus in uapsd queue are %d:",
    //               oal_atomic_read(&pst_dmac_user->st_uapsd_stru.uc_mpdu_num));


    /* ��UAPSD���ܶ��н��в������������� */
    oal_spin_lock(&pst_dmac_user->st_uapsd_stru.st_lock_uapsd);

    pst_first_net_buf = pst_net_buf;
    while ((oal_atomic_read(&pst_dmac_user->st_uapsd_stru.uc_mpdu_num) < uc_max_qdepth)
            && (OAL_PTR_NULL != pst_first_net_buf))
    {
        /* ��ÿһ��mpdu�е�һ��net_buf��CB�ֶλ�ȡ��mpduһ����������net_buff */
        pst_tx_ctrl = (mac_tx_ctl_stru *)OAL_NETBUF_CB(pst_first_net_buf);

        /*���ʱ����more data bit�����ٳ���ʱ�Ĳ���*/
        pst_frame_hdr = MAC_GET_CB_FRAME_HEADER_ADDR(pst_tx_ctrl);
        pst_frame_hdr->st_frame_control.bit_more_data = 0x01;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
        uc_netbuf_num_in_mpdu = MAC_GET_CB_NETBUF_NUM(pst_tx_ctrl);
#endif

        /* ����mpdu��ÿһ��net_buff���뵽���ܶ����� */
        pst_netbuf = pst_first_net_buf;
        while (
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
            (0 != uc_netbuf_num_in_mpdu) &&
#endif
            (OAL_PTR_NULL != pst_netbuf))
        {
            pst_first_net_buf = oal_get_netbuf_next(pst_netbuf);

            oal_netbuf_add_to_list_tail(pst_netbuf, &pst_dmac_user->st_uapsd_stru.st_uapsd_queue_head);

            pst_netbuf = pst_first_net_buf;

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
            uc_netbuf_num_in_mpdu--;
#endif
        }
        /* ���½��ܶ�����mpdu�ĸ��� */
        oal_atomic_inc(&pst_dmac_user->st_uapsd_stru.uc_mpdu_num);

    }

    oal_spin_unlock(&pst_dmac_user->st_uapsd_stru.st_lock_uapsd);

    /*
       �ж���mpduȫ������˻��Ƕ������ˣ��������Ϊ�������˲��һ���mpduû����ӣ�
       ��ʣ�µ�mpdu�ͷ�
    */
    if ((uc_max_qdepth == oal_atomic_read(&pst_dmac_user->st_uapsd_stru.uc_mpdu_num))
        && (OAL_PTR_NULL != pst_first_net_buf))
    {
        g_st_dmac_tx_bss_comm_rom_cb.p_dmac_tx_excp_free_netbuf(pst_first_net_buf);

        DMAC_UAPSD_STATS_INCR(pst_uapsd_statis->ul_uapsd_tx_enqueue_free_count);

        OAM_WARNING_LOG0(pst_dmac_user->st_user_base_info.uc_vap_id, OAM_SF_PWR,
                         "{dmac_uapsd_tx_enqueue::some mpdus are released due to queue full.}");
    }

    return OAL_SUCC;

}


oal_void dmac_uapsd_tx_complete(dmac_user_stru *pst_dmac_user,mac_tx_ctl_stru *pst_cb)
{
    mac_ieee80211_qos_frame_stru    *pst_mac_header;

    /*lint -save -e774 */
    if(!MAC_GET_CB_FRAME_HEADER_ADDR(pst_cb))
    {
        OAM_ERROR_LOG0(pst_dmac_user->st_user_base_info.uc_vap_id, OAM_SF_TX,
                       "{dmac_uapsd_tx_complete::pst_frame_header null.}");

        return;
    }
    /*lint -restore */

    pst_mac_header = (mac_ieee80211_qos_frame_stru*)(MAC_GET_CB_FRAME_HEADER_ADDR(pst_cb));
    if((WLAN_DATA_BASICTYPE == pst_mac_header->st_frame_control.bit_type)&&
        ((WLAN_QOS_DATA == pst_mac_header->st_frame_control.bit_sub_type)||
        (WLAN_QOS_NULL_FRAME== pst_mac_header->st_frame_control.bit_sub_type)))
        {
            if(1 == pst_mac_header->bit_qc_eosp)
            {
                pst_dmac_user->uc_uapsd_flag &= ~MAC_USR_UAPSD_SP;
            }
        }
}

oal_uint32 dmac_config_set_uapsd_update(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    oal_uint16                      us_user_id;
    oal_uint8                       uc_uapsd_flag;
    mac_user_uapsd_status_stru     *pst_uapsd_status;
    dmac_user_stru                 *pst_dmac_user;
    oal_uint8                       uc_len_tmp;

    us_user_id = *(oal_uint16 *)puc_param;
    uc_len_tmp = OAL_SIZEOF(us_user_id);
    uc_uapsd_flag = *(puc_param + uc_len_tmp);
    uc_len_tmp += OAL_SIZEOF(uc_uapsd_flag);
    pst_uapsd_status = (mac_user_uapsd_status_stru *)(puc_param + uc_len_tmp);

    pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user(us_user_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dmac_user)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{dmac_config_set_uapsd_update::pst_dmac_user[%d] null.}", us_user_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_memcopy(&pst_dmac_user->uc_uapsd_flag, &uc_uapsd_flag, OAL_SIZEOF(oal_uint8));
    oal_memcopy(&pst_dmac_user->st_uapsd_status, pst_uapsd_status, OAL_SIZEOF(mac_user_uapsd_status_stru));

    return OAL_SUCC;
}


oal_uint32  dmac_config_uapsd_debug(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    /* uapsdά����Ϣ��sh hipriv "vap0 uapsd_debug 0|1|2(���û�|all user|���ͳ�Ƽ�����) xx:xx:xx:xx:xx:xx(mac��ַ)" */
    mac_device_stru         *pst_device;
    oal_uint32               ul_off_set = 0;
    oal_int8                 ac_name[DMAC_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint8                auc_user_addr[WLAN_MAC_ADDR_LEN] = {0};
    oal_uint8                uc_debug_all = 0;
    dmac_user_stru          *pst_dmac_user;
    oal_dlist_head_stru            *pst_entry;
    mac_user_stru                  *pst_user_tmp;

    pst_device = mac_res_get_dev(pst_mac_vap->uc_device_id);

    /* ��������������� */
    if(OAL_PTR_NULL == pst_device)
    {
        return OAL_FAIL;
    }

    OAL_MEMZERO(ac_name, DMAC_HIPRIV_CMD_NAME_MAX_LEN);

    /* �����û����ǵ��û�? */
    dmac_get_cmd_one_arg((oal_int8*)puc_param, ac_name, &ul_off_set);
    uc_debug_all = (oal_uint8)oal_atoi(ac_name);
    /* ƫ�ƣ�ȡ��һ������ */
    puc_param = puc_param + ul_off_set;

    if(0 == uc_debug_all)
    {
        /* ��ȡ�û���ַ */
        dmac_get_cmd_one_arg((oal_int8*)puc_param, ac_name, &ul_off_set);
        oal_strtoaddr(ac_name, auc_user_addr);

        pst_dmac_user = mac_vap_get_dmac_user_by_addr(pst_mac_vap, auc_user_addr);
        if (OAL_PTR_NULL == pst_dmac_user)
        {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{dmac_config_uapsd_debug::pst_dmac_user null.}");
            return OAL_ERR_CODE_PTR_NULL;
        }

        dmac_uapsd_print_info(pst_dmac_user);
        return OAL_SUCC;
    }

    /* ����vap�������û� ,�����Ϣ*/
    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_mac_vap->st_mac_user_list_head))
    {
        pst_user_tmp = OAL_DLIST_GET_ENTRY(pst_entry, mac_user_stru, st_user_dlist);
        /*lint -save -e774 */
        if (OAL_PTR_NULL == pst_user_tmp)
        {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{dmac_config_uapsd_debug::pst_user_tmp null.}");
            continue;
        }
        /*lint -restore */

        pst_dmac_user = MAC_GET_DMAC_USER(pst_user_tmp);

        if (1 == uc_debug_all)
        {
            dmac_uapsd_print_info(pst_dmac_user);
        }
        else
        {
            oal_memset(pst_dmac_user->st_uapsd_stru.pst_uapsd_statis, 0, OAL_SIZEOF(dmac_usr_uapsd_statis_stru));
        }
    }

    return OAL_SUCC;
}

oal_void  dmac_uapsd_print_info(dmac_user_stru *pst_dmac_user)
{
   dmac_user_uapsd_stru         *pst_uapsd_stru;
   dmac_usr_uapsd_statis_stru   *pst_uapsd_statis;
   mac_user_stru                *pst_mac_user;
   mac_user_uapsd_status_stru   *pst_uapsd_status;

   pst_mac_user     = &(pst_dmac_user->st_user_base_info);
   pst_uapsd_status = &(pst_dmac_user->st_uapsd_status);
   pst_uapsd_stru   = &(pst_dmac_user->st_uapsd_stru);
   pst_uapsd_statis =  pst_uapsd_stru->pst_uapsd_statis;

   /*������Ϣ*/
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
   OAM_WARNING_LOG_ALTER(pst_dmac_user->st_user_base_info.uc_vap_id, OAM_SF_ANY,
                          "{dmac_uapsd_print_info:User assoc id = %d,macaddr = %02X:XX:XX:%02X:%02X:%02X\n}",
                          5, pst_mac_user->us_assoc_id,
                          pst_mac_user->auc_user_mac_addr[0],pst_mac_user->auc_user_mac_addr[3],
                          pst_mac_user->auc_user_mac_addr[4],pst_mac_user->auc_user_mac_addr[5]);
   OAM_WARNING_LOG_ALTER(pst_dmac_user->st_user_base_info.uc_vap_id, OAM_SF_ANY,
                         "{dmac_uapsd_print_info:uc_uapsd_flag=%d,uc_qos_info=%d,max_sp_len=%d,ac_trigger_ena:BK=%d,BE=%d,Vi=%d,Vo=%d,"
                         "ac_delievy_ena: BK=%d,BE=%d,Vi=%d,Vo=%d\n}",
                         11,
                         pst_dmac_user->uc_uapsd_flag, pst_uapsd_status->uc_qos_info, pst_uapsd_status->uc_max_sp_len,
                         pst_uapsd_status->uc_ac_trigger_ena[WLAN_WME_AC_BK], pst_uapsd_status->uc_ac_trigger_ena[WLAN_WME_AC_BE],
                         pst_uapsd_status->uc_ac_trigger_ena[WLAN_WME_AC_VI], pst_uapsd_status->uc_ac_trigger_ena[WLAN_WME_AC_VO],
                         pst_uapsd_status->uc_ac_delievy_ena[WLAN_WME_AC_BK], pst_uapsd_status->uc_ac_delievy_ena[WLAN_WME_AC_BE],
                         pst_uapsd_status->uc_ac_delievy_ena[WLAN_WME_AC_VI], pst_uapsd_status->uc_ac_delievy_ena[WLAN_WME_AC_VO]);
   OAM_WARNING_LOG1(pst_dmac_user->st_user_base_info.uc_vap_id, OAM_SF_ANY, "{dmac_uapsd_print_info:total mpdu in queue = %d,following is statistic...\n}",
                    oal_atomic_read(&pst_uapsd_stru->uc_mpdu_num));
#endif
      OAL_IO_PRINT("---------User assoc id = %d,macaddr = %02X:XX:XX:%02X:%02X:%02X---------------\n",
                               pst_mac_user->us_assoc_id,
                               pst_mac_user->auc_user_mac_addr[0],pst_mac_user->auc_user_mac_addr[3],
                               pst_mac_user->auc_user_mac_addr[4],pst_mac_user->auc_user_mac_addr[5]);
      OAL_IO_PRINT("uc_uapsd_flag = %d; uc_qos_info = %d; max_sp_len = %d\n",
                    pst_dmac_user->uc_uapsd_flag, pst_uapsd_status->uc_qos_info, pst_uapsd_status->uc_max_sp_len);

      OAL_IO_PRINT("ac_trigger_ena: BK=%d,BE=%d,Vi=%d,Vo=%d\n",
                                          pst_uapsd_status->uc_ac_trigger_ena[WLAN_WME_AC_BK],
                                          pst_uapsd_status->uc_ac_trigger_ena[WLAN_WME_AC_BE],
                                          pst_uapsd_status->uc_ac_trigger_ena[WLAN_WME_AC_VI],
                                          pst_uapsd_status->uc_ac_trigger_ena[WLAN_WME_AC_VO]);
      OAL_IO_PRINT("ac_delievy_ena: BK=%d,BE=%d,Vi=%d,Vo=%d\n",
                                          pst_uapsd_status->uc_ac_delievy_ena[WLAN_WME_AC_BK],
                                          pst_uapsd_status->uc_ac_delievy_ena[WLAN_WME_AC_BE],
                                          pst_uapsd_status->uc_ac_delievy_ena[WLAN_WME_AC_VI],
                                          pst_uapsd_status->uc_ac_delievy_ena[WLAN_WME_AC_VO]);
      OAL_IO_PRINT("total mpdu in queue = %d\n",oal_atomic_read(&pst_uapsd_stru->uc_mpdu_num));

      OAL_IO_PRINT("following is statistic................\n");
   /*ͳ�Ƽ�����*/
   OAL_REFERENCE(pst_uapsd_statis);
   OAL_REFERENCE(pst_mac_user);
   OAL_REFERENCE(pst_uapsd_status);
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
      OAM_WARNING_LOG_ALTER(pst_dmac_user->st_user_base_info.uc_vap_id, OAM_SF_ANY,
                             "{dmac_uapsd_print_info:tx enqueue count=%d free_count=%d,rx trigger_in_sp=%d state_trans=%d dup_seq=%d,"
                             "send qosnull=%d extra_qosnull=%d,process_queue_error=%d,flush_queue_error = %d\n}",
                             9, pst_uapsd_statis->ul_uapsd_tx_enqueue_count,
                             pst_uapsd_statis->ul_uapsd_tx_enqueue_free_count, pst_uapsd_statis->ul_uapsd_rx_trigger_in_sp,
                             pst_uapsd_statis->ul_uapsd_rx_trigger_state_trans, pst_uapsd_statis->ul_uapsd_rx_trigger_dup_seq,
                             pst_uapsd_statis->ul_uapsd_send_qosnull, pst_uapsd_statis->ul_uapsd_send_extra_qosnull,
                             pst_uapsd_statis->ul_uapsd_process_queue_error, pst_uapsd_statis->ul_uapsd_flush_queue_error);
#endif
   OAL_IO_PRINT("tx_enqueue_count = %u\n",pst_uapsd_statis->ul_uapsd_tx_enqueue_count);             /*dmac_uapsd_tx_enqueue���ô���*/
   OAL_IO_PRINT("tx_enqueue_free_count = %u\n",pst_uapsd_statis->ul_uapsd_tx_enqueue_free_count);   /*��ӹ�����MPDU���ͷŵĴ�����һ�ο����ͷŶ��MPDU*/
   OAL_IO_PRINT("rx_trigger_in_sp = %u\n",pst_uapsd_statis->ul_uapsd_rx_trigger_in_sp);             /*trigger��鷢�ִ���SP�еĴ���*/
   OAL_IO_PRINT("rx_trigger_state_trans = %u\n",pst_uapsd_statis->ul_uapsd_rx_trigger_state_trans); /*trigger֡���ظ�֡�ĸ���*/
   OAL_IO_PRINT("rx_trigger_dup_seq = %u\n",pst_uapsd_statis->ul_uapsd_rx_trigger_dup_seq);         /*dmac_uapsd_tx_enqueue���ô���*/
   OAL_IO_PRINT("send_qosnull = %u\n",pst_uapsd_statis->ul_uapsd_send_qosnull);                     /*����Ϊ�գ�����qos null data�ĸ���*/
   OAL_IO_PRINT("send_extra_qosnull = %u\n",pst_uapsd_statis->ul_uapsd_send_extra_qosnull);         /*���һ��Ϊ����֡�����Ͷ���qosnull�ĸ���*/
   OAL_IO_PRINT("process_queue_error = %u\n",pst_uapsd_statis->ul_uapsd_process_queue_error);       /*���д�������г���Ĵ���*/
   OAL_IO_PRINT("flush_queue_error = %u\n",pst_uapsd_statis->ul_uapsd_flush_queue_error);           /*flush���д�������г���Ĵ���*/
}
#endif /* _PRE_WLAN_FEATURE_UAPSD */

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif


