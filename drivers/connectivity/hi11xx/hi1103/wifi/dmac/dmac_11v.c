


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_11V

/*****************************************************************************
  1 ͷ�ļ�����
*****************************************************************************/
#include "oal_ext_if.h"
#include "oal_net.h"
#include "mac_frame.h"
#include "mac_resource.h"
#include "mac_ie.h"
#include "mac_vap.h"
#include "mac_user.h"
#include "frw_ext_if.h"
#include "hal_ext_if.h"
#include "mac_resource.h"
#include "wlan_types.h"
#include "dmac_main.h"
#include "dmac_ext_if.h"
#include "dmac_chan_mgmt.h"
#include "dmac_tx_bss_comm.h"
#include "dmac_scan.h"
#include "dmac_11v.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_11V_C

/*****************************************************************************
  2 ȫ�ֱ�������
*****************************************************************************/

/*****************************************************************************
  3 ����ʵ��
*****************************************************************************/


oal_uint32  dmac_tx_bsst_query_action(dmac_vap_stru *pst_dmac_vap, dmac_user_stru *pst_dmac_user, dmac_bsst_query_info_stru *pst_bsst_query_info)
{
    oal_netbuf_stru        *pst_mgmt_buf;
    oal_uint16              us_mgmt_len;
    mac_tx_ctl_stru        *pst_tx_ctl;
    oal_uint32              ul_ret;

    if ((OAL_PTR_NULL == pst_dmac_vap) || (OAL_PTR_NULL == pst_dmac_user) || (OAL_PTR_NULL == pst_bsst_query_info))
    {
        OAM_ERROR_LOG3(0, OAM_SF_BSSTRANSITION, "{dmac_tx_bsst_query_action:: null pointer, vap[%x] user[%x] info[%x].}",pst_dmac_vap, pst_dmac_user, pst_bsst_query_info);
        return OAL_ERR_CODE_PTR_NULL;
    }
    if(OAL_PTR_NULL == pst_dmac_vap->st_vap_base_info.pst_mib_info)
    {
        OAM_ERROR_LOG1(0, OAM_SF_BSSTRANSITION, "{dmac_tx_bsst_query_action:: null pointer, mib info[%x].}",pst_dmac_vap->st_vap_base_info.pst_mib_info);
        return OAL_ERR_CODE_PTR_NULL;
    }
    /* ��������λ֧�ַ����·��Ҽ�����wireless management �� bss transition���� */
    if ( (OAL_TRUE != mac_mib_get_MgmtOptionBSSTransitionImplemented(&pst_dmac_vap->st_vap_base_info)) ||
         (OAL_TRUE != mac_mib_get_MgmtOptionBSSTransitionActivated(&pst_dmac_vap->st_vap_base_info)) ||
         (OAL_TRUE != mac_mib_get_WirelessManagementImplemented(&pst_dmac_vap->st_vap_base_info)) )
    {
        OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BSSTRANSITION, "{dmac_tx_bsst_query_action::vap not support.}");
        return OAL_FAIL;
    }
    /* �û�����λҲ��Ҫ֧�ַ����·� */
    if ( OAL_TRUE != pst_dmac_user->st_user_base_info.st_cap_info.bit_bss_transition)
    {
        OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BSSTRANSITION, "{dmac_tx_bsst_query_action::user not support.}");
        return OAL_FAIL;
    }

    /* �������֡�ڴ� */
    pst_mgmt_buf = OAL_MEM_NETBUF_ALLOC(OAL_MGMT_NETBUF, WLAN_MGMT_NETBUF_SIZE, OAL_NETBUF_PRIORITY_HIGH);
    if (OAL_PTR_NULL == pst_mgmt_buf)
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BSSTRANSITION, "{dmac_tx_bsst_query_action::pst_mgmt_buf null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_set_netbuf_prev(pst_mgmt_buf, OAL_PTR_NULL);
    oal_set_netbuf_next(pst_mgmt_buf,OAL_PTR_NULL);
    OAL_MEM_NETBUF_TRACE(pst_mgmt_buf, OAL_TRUE);

    /* ��װ BSS Transition Management Query ֡ */
#ifdef _PRE_DEBUG_MODE
    OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BSSTRANSITION, "{dmac_tx_bsst_query_action::encap btq start.}");
#endif
    us_mgmt_len = dmac_encap_bsst_query_action(pst_dmac_vap, pst_dmac_user, pst_bsst_query_info, pst_mgmt_buf);
    if (0 == us_mgmt_len)
    {
        oal_netbuf_free(pst_mgmt_buf);
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BSSTRANSITION, "{dmac_tx_bsst_query_action::encap btq action failed.}");
        return OAL_FAIL;
    }
    /* ��дnetbuf��cb�ֶΣ������͹���֡�ͷ�����ɽӿ�ʹ�� */
    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_mgmt_buf);
    OAL_MEMZERO(pst_tx_ctl, sizeof(mac_tx_ctl_stru));
    MAC_GET_CB_TX_USER_IDX(pst_tx_ctl) = pst_dmac_user->st_user_base_info.us_assoc_id;
    MAC_GET_CB_WME_AC_TYPE(pst_tx_ctl) = WLAN_WME_AC_MGMT;

    /* ���÷��͹���֡�ӿ� */
    ul_ret = dmac_tx_mgmt(pst_dmac_vap, pst_mgmt_buf, us_mgmt_len);
    if (OAL_SUCC != ul_ret)
    {
        oal_netbuf_free(pst_mgmt_buf);
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BSSTRANSITION, "{dmac_tx_bsst_query_action::tx btq action failed.}");
        return ul_ret;
    }
    return OAL_SUCC;
}


oal_uint16  dmac_encap_bsst_query_action(dmac_vap_stru *pst_dmac_vap, dmac_user_stru *pst_dmac_user, dmac_bsst_query_info_stru *pst_bsst_query_info, oal_netbuf_stru *pst_buffer)
{
    oal_uint16       us_len           = 0;
    oal_uint8       *puc_mac_header   = OAL_PTR_NULL;
    oal_uint8       *puc_payload_addr = OAL_PTR_NULL;
    oal_uint8       *puc_payload_addr_origin = OAL_PTR_NULL;
    oal_uint16       us_frame_length  = 0;
    oal_uint8        uc_frame_index   = 0;
    /* ��μ�� */
    if ((OAL_PTR_NULL == pst_dmac_vap) || (OAL_PTR_NULL == pst_dmac_user) || (OAL_PTR_NULL == pst_bsst_query_info) || (OAL_PTR_NULL == pst_buffer))
    {
        OAM_ERROR_LOG4(0, OAM_SF_BSSTRANSITION, "{dmac_encap_bsst_query_action::null param.vap:%x user:%x query info:%x buf:%x}",
            pst_dmac_vap,pst_dmac_user,pst_bsst_query_info,pst_buffer);
        return 0;
    }
    if (OAL_PTR_NULL == pst_dmac_user->pst_11v_ctrl_info)
    {
        OAM_ERROR_LOG1(0, OAM_SF_BSSTRANSITION, "{dmac_encap_bsst_query_action::null param. pst_11v_ctrl_info:%x}",pst_dmac_user->pst_11v_ctrl_info);
        return 0;
    }
    puc_mac_header   = oal_netbuf_header(pst_buffer);
    puc_payload_addr = mac_netbuf_get_payload(pst_buffer);
    puc_payload_addr_origin = puc_payload_addr;
    /*************************************************************************/
    /*                        Management Frame Format                        */
    /* --------------------------------------------------------------------  */
    /* |Frame Control|Duration|DA|SA|BSSID|Sequence Control|Frame Body|FCS|  */
    /* --------------------------------------------------------------------  */
    /* | 2           |2       |6 |6 |6    |2               |0 - 2312  |4  |  */
    /* --------------------------------------------------------------------  */
    /*                                                                       */
    /*************************************************************************/

    /*************************************************************************/
    /*                Set the fields in the frame header                     */
    /*************************************************************************/
    /* ֡�����ֶ�ȫΪ0������type��subtype */
    mac_hdr_set_frame_control(puc_mac_header, WLAN_PROTOCOL_VERSION| WLAN_FC0_TYPE_MGT | WLAN_FC0_SUBTYPE_ACTION);
    /* ���÷�Ƭ���Ϊ0 */
    mac_hdr_set_fragment_number(puc_mac_header, 0);
    /* ���õ�ַ1����STA���ӵ�AP MAC��ַ*/
    oal_set_mac_addr(puc_mac_header + WLAN_HDR_ADDR1_OFFSET, pst_dmac_user->st_user_base_info.auc_user_mac_addr);
    /* ���õ�ַ2Ϊ�Լ���MAC��ַ */
    oal_set_mac_addr(puc_mac_header + WLAN_HDR_ADDR2_OFFSET, mac_mib_get_StationID(&pst_dmac_vap->st_vap_base_info));
    /* ��ַ3��ΪVAP��MAC��ַ */
    oal_set_mac_addr(puc_mac_header + WLAN_HDR_ADDR3_OFFSET, pst_dmac_vap->st_vap_base_info.auc_bssid);

    /*************************************************************************/
    /*                Set the fields in the frame body                     */
    /*************************************************************************/

    /*************************************************************************/
    /*                       Channel Switch Announcement Frame - Frame Body  */
    /* --------------------------------------------------------------------- */
    /* |Category |Action |Dialog Token| BSS Tran Reason|BSS Candidate List Entry */
    /* --------------------------------------------------------------------- */
    /* |1        |1      | 1          |  1             |Variable             */
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /*************************************************************************/

    /* ����Action��Category   */
    /* 10: WNM */
    puc_payload_addr[uc_frame_index++] = MAC_ACTION_CATEGORY_WNM;
    /* ����WNM Action Field */
    /* 6: BSS Transition Query Frame */
    puc_payload_addr[uc_frame_index++] = MAC_WNM_ACTION_BSS_TRANSITION_MGMT_QUERY;
    /* ����Dialog Token ��1����Ҫ���ڽ��շ�����Response�� ���߳�ʱ�� */
    puc_payload_addr[uc_frame_index++] = pst_dmac_user->pst_11v_ctrl_info->uc_user_bsst_token;
    /* ����Query Reason */
    puc_payload_addr[uc_frame_index++] = pst_bsst_query_info->uc_reason;
    puc_payload_addr += uc_frame_index;
    /* ����Neighbor Report IE */
    dmac_set_neighbor_ie(pst_bsst_query_info->pst_neighbor_bss_list, pst_bsst_query_info->uc_bss_list_num, puc_payload_addr, &us_len);
    puc_payload_addr += us_len;
    us_frame_length = (oal_uint16)( puc_payload_addr - puc_payload_addr_origin + MAC_80211_FRAME_LEN);

#ifdef _PRE_DEBUG_MODE
    OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BSSTRANSITION, "{dmac_encap_bsst_query_action::LEN = %d.}", us_frame_length);
#endif
    return us_frame_length;
}


oal_uint32  dmac_rx_bsst_query_action(dmac_vap_stru *pst_dmac_vap, dmac_user_stru *pst_dmac_user, oal_netbuf_stru *pst_netbuf)
{
    oal_uint16                  us_query_frame_len   = 0;
    oal_uint8                   uc_bss_neighbor_num  = 0;
    oal_uint8                   *puc_data;
    dmac_rx_ctl_stru            *pst_rx_ctrl;
    mac_rx_ctl_stru             *pst_rx_info;
    //oal_uint8                   uc_reason            = 0;
    dmac_bsst_query_info_stru   st_bsst_query_info;
    dmac_bsst_req_info_stru     st_bsst_req_info;

    if ((OAL_PTR_NULL == pst_dmac_vap) || (OAL_PTR_NULL == pst_dmac_user) || (OAL_PTR_NULL == pst_netbuf))
    {
        OAM_ERROR_LOG3(0, OAM_SF_BSSTRANSITION, "{dmac_rx_bsst_query_action::null param, vap:0x%x user:0x%x netbuf:0x%x.}", pst_dmac_vap, pst_dmac_user, pst_netbuf);
        return OAL_ERR_CODE_PTR_NULL;
    }
    if (OAL_PTR_NULL == pst_dmac_user->pst_11v_ctrl_info)
    {
        OAM_ERROR_LOG1(0, OAM_SF_BSSTRANSITION, "{dmac_rx_bsst_query_action::null param. pst_11v_ctrl_info:%x}",pst_dmac_user->pst_11v_ctrl_info);
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_rx_ctrl         = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
    if (OAL_PTR_NULL == pst_rx_ctrl)
    {
        OAM_ERROR_LOG1(0, OAM_SF_BSSTRANSITION, "{dmac_rx_bsst_query_action::null param, rx_ctrl:0x%x.}", pst_rx_ctrl);
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_rx_info         = (mac_rx_ctl_stru *)(&(pst_rx_ctrl->st_rx_info));
    /* ��ȡ֡��ָ�� */
    puc_data = MAC_GET_RX_PAYLOAD_ADDR(pst_rx_info,pst_netbuf);
    us_query_frame_len = MAC_GET_RX_CB_PAYLOAD_LEN(pst_rx_info);  /*֡�峤��*/
#ifdef _PRE_DEBUG_MODE
    OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BSSTRANSITION, "{dmac_rx_bsst_query_action::bsst query frame len =%d.}",us_query_frame_len);
#endif
    /* ֡�����С����Ϊ4 С��4���ʽ�쳣 */
    if(DMAC_11V_QUERY_FRAME_BODY_FIX > us_query_frame_len)
    {
        OAM_ERROR_LOG1(0, OAM_SF_BSSTRANSITION, "{dmac_rx_bsst_query_action:: frame length error %d.}", us_query_frame_len);
        return OAL_FAIL;
    }
    /* ����֡ tokenֱֵ�Ӹ��µ��û��µı��� ˢ�������û�һ�µ�token */
    pst_dmac_user->pst_11v_ctrl_info->uc_user_bsst_token = puc_data[2];
	/* ��ʱ��ģ��ʹ�øñ��� ��ʱ���θñ��� */
    //uc_reason = puc_data[3];
    /* ����Neighbor Report IE */
    puc_data += DMAC_11V_QUERY_FRAME_BODY_FIX;
    st_bsst_query_info.pst_neighbor_bss_list = OAL_PTR_NULL;
    dmac_get_neighbor_ie(&st_bsst_query_info.pst_neighbor_bss_list,puc_data, us_query_frame_len-4, &uc_bss_neighbor_num);

    /* ���bss transition request֡ ��ֱ�ӵ��÷��ͽӿڷ���request֡��STA  */
#ifdef _PRE_DEBUG_MODE
    OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BSSTRANSITION, "{dmac_rx_bsst_query_action::neighbor list num [%d].}",uc_bss_neighbor_num);
#endif
    st_bsst_req_info.uc_bss_list_num = uc_bss_neighbor_num;
    st_bsst_req_info.st_request_mode.bit_candidate_list_include = 1;                /* request mode ��candidate list include Ϊ1 */
    st_bsst_req_info.st_request_mode.bit_abridged = 0;
    st_bsst_req_info.st_request_mode.bit_bss_disassoc_imminent = 0;
    st_bsst_req_info.st_request_mode.bit_termination_include = 0;
    st_bsst_req_info.st_request_mode.bit_ess_disassoc_imminent = 0;
    st_bsst_req_info.us_disassoc_time = 0xffff;                 /* �Ͽ�ʱ����65535 ��ʾ����Ͽ� */
    st_bsst_req_info.uc_validity_interval = 0;              /* IntervalΪ0 Ԥ��ֵ */
    st_bsst_req_info.puc_session_url = OAL_PTR_NULL;
    st_bsst_req_info.st_term_duration.uc_sub_ie_id = DMAC_11V_SUBELEMENT_ID_RESV;   /* ����Ϊ0 ��ʾ����������Ԫ�� */
    if(OAL_PTR_NULL != st_bsst_query_info.pst_neighbor_bss_list)
    {
        st_bsst_req_info.pst_neighbor_bss_list = st_bsst_query_info.pst_neighbor_bss_list;      /* ֱ�ӽ�query֡�ĺ�ѡ�б����͸�STA */
    }
    /* ���ýӿڷ���request֡ */
    if(OAL_SUCC != dmac_tx_bsst_req_action(pst_dmac_vap,pst_dmac_user,&st_bsst_req_info,OAL_PTR_NULL))
    {
        OAM_ERROR_LOG0(0, OAM_SF_BSSTRANSITION, "{dmac_rx_bsst_query_action:: tx bsst request error.}");
        if(OAL_PTR_NULL != st_bsst_query_info.pst_neighbor_bss_list)
        {
            OAL_MEM_FREE(st_bsst_query_info.pst_neighbor_bss_list, OAL_TRUE);
            st_bsst_query_info.pst_neighbor_bss_list = OAL_PTR_NULL;
            st_bsst_req_info.pst_neighbor_bss_list = OAL_PTR_NULL;
        }
        return OAL_FAIL;
    }
    /* �ͷ�֡����ʱ������ڴ� */
    if(OAL_PTR_NULL != st_bsst_query_info.pst_neighbor_bss_list)
    {
        OAL_MEM_FREE(st_bsst_query_info.pst_neighbor_bss_list, OAL_TRUE);
        st_bsst_query_info.pst_neighbor_bss_list = OAL_PTR_NULL;
        st_bsst_req_info.pst_neighbor_bss_list = OAL_PTR_NULL;
    }

    return OAL_SUCC;
}


oal_uint32  dmac_tx_bsst_req_action_one_bss(dmac_vap_stru *pst_dmac_vap, dmac_user_stru *pst_dmac_user, dmac_neighbor_bss_info_stru *pst_bss_neighbor_list, dmac_user_callback_func_11v p_fun_callback)
{
    oal_uint32          ul_ret = 0;
    dmac_bsst_req_info_stru st_bsst_req_info;

    if ((OAL_PTR_NULL == pst_dmac_vap) || (OAL_PTR_NULL == pst_dmac_user) || (OAL_PTR_NULL == pst_bss_neighbor_list))
    {
        OAM_ERROR_LOG3(0, OAM_SF_BSSTRANSITION, "{dmac_tx_bsst_req_action::null param, vap:%x user:%x list:%x.}", pst_dmac_vap, pst_dmac_user, pst_bss_neighbor_list);
        return OAL_ERR_CODE_PTR_NULL;
    }
    /* �ӿں���ֻ����һ���ھ��б��˽ӿڽ���������Ҫ�Ĳ�����дΪĬ��ֵ */
    st_bsst_req_info.pst_neighbor_bss_list = pst_bss_neighbor_list;
    st_bsst_req_info.puc_session_url = OAL_PTR_NULL;            /* URL�ÿ� ��URL��Ϣ */
    st_bsst_req_info.st_request_mode.bit_candidate_list_include = OAL_TRUE;                     /* ������ѡ�ھ��б� */
    st_bsst_req_info.st_request_mode.bit_abridged   = OAL_TRUE;                                /* �޸���AP��Ϣ */
    st_bsst_req_info.st_request_mode.bit_bss_disassoc_imminent = OAL_FALSE;                      /* �������������Ϣ �����ʱ����Ϊ1:�����Ͽ� */
    st_bsst_req_info.st_request_mode.bit_ess_disassoc_imminent = OAL_FALSE;                     /* ��ESS��ֹʱ�� ������URL��Ϣ */
    st_bsst_req_info.st_request_mode.bit_termination_include = OAL_FALSE;                       /* ��BSS��ֹʱ�� */
    st_bsst_req_info.st_term_duration.uc_sub_ie_id = DMAC_11V_SUBELEMENT_ID_RESV;               /* ID��0 ֡�в���������Ԫ�� */
    st_bsst_req_info.uc_bss_list_num = 1;                                                       /* ��ѡAP����ֻ��һ�� */
    st_bsst_req_info.uc_validity_interval = 0;                                                  /* ��ֵΪԤ�� */
    st_bsst_req_info.us_disassoc_time = 0;  //���˱���һ��                                    /* 0ֵΪԤ�� ��дΪ1��ʾ�����ߵ�STA */

    /* ���ýӿڷ���Request֡ */
    ul_ret = dmac_tx_bsst_req_action(pst_dmac_vap, pst_dmac_user, &st_bsst_req_info, p_fun_callback);
    return ul_ret;
}



oal_uint32  dmac_tx_bsst_req_action(dmac_vap_stru *pst_dmac_vap, dmac_user_stru *pst_dmac_user, dmac_bsst_req_info_stru *pst_bsst_req_info, dmac_user_callback_func_11v p_fun_callback)
{
    oal_netbuf_stru                  *pst_bsst_req_buf;
    oal_uint16                        us_frame_len;
    oal_uint32                        ul_ret;
    mac_tx_ctl_stru                  *pst_tx_ctl;
    dmac_device_stru                 *pst_dmac_device;

    if ((OAL_PTR_NULL == pst_dmac_vap) || (OAL_PTR_NULL == pst_dmac_user) || (OAL_PTR_NULL == pst_bsst_req_info))
    {
        OAM_ERROR_LOG3(0, OAM_SF_BSSTRANSITION, "{dmac_tx_bsst_req_action::null param, vap:%x user:%x info:%x.}", pst_dmac_vap, pst_dmac_user, pst_bsst_req_info);
        return OAL_ERR_CODE_PTR_NULL;
    }
    if((OAL_PTR_NULL == pst_dmac_vap->st_vap_base_info.pst_mib_info) || (OAL_PTR_NULL == pst_dmac_user->pst_11v_ctrl_info))
    {
        OAM_ERROR_LOG2(0, OAM_SF_BSSTRANSITION, "{dmac_tx_bsst_req_action:: null param, mib[0x%x] ctrl[0x%x].}",pst_dmac_vap->st_vap_base_info.pst_mib_info, pst_dmac_user->pst_11v_ctrl_info);
        return OAL_ERR_CODE_PTR_NULL;
    }
    /* ��������λ֧�ַ����·��Ҽ�����wireless management �� bss transition���� */
    if ( (OAL_TRUE != mac_mib_get_MgmtOptionBSSTransitionImplemented(&pst_dmac_vap->st_vap_base_info)) ||
         (OAL_TRUE != mac_mib_get_MgmtOptionBSSTransitionActivated(&pst_dmac_vap->st_vap_base_info)) ||
         (OAL_TRUE != mac_mib_get_WirelessManagementImplemented(&pst_dmac_vap->st_vap_base_info)) )
    {
        OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BSSTRANSITION, "{dmac_tx_bsst_req_action::vap not support.}");
        return OAL_FAIL;
    }
    /* �û�����λҲ��Ҫ֧�ַ����·� */
    if ( OAL_TRUE != pst_dmac_user->st_user_base_info.st_cap_info.bit_bss_transition)
    {
        OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BSSTRANSITION, "{dmac_tx_bsst_req_action::user not support.}");
        return OAL_FAIL;
    }
    /* �����ǰ�û�״̬�Ѿ��ǵȴ�Response̬ ���������ٴη��� */
    if(DMAC_11V_AP_STATUS_INIT != pst_dmac_user->pst_11v_ctrl_info->uc_user_status)
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BSSTRANSITION, "{dmac_tx_bsst_req_action::user busy now.}");
        return OAL_ERR_CODE_11V_REPEAT;
    }
    pst_dmac_device = dmac_res_get_mac_dev(pst_dmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_PTR_NULL == pst_dmac_device)
    {
        OAM_ERROR_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BSSTRANSITION, "{dmac_tx_bsst_req_action::dev null, dev id[%d].}", pst_dmac_vap->st_vap_base_info.uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }
    /* ����κ���ָ�뱣�浽�û��ṹ���� */
    pst_dmac_user->pst_11v_ctrl_info->dmac_11v_callback_fn = p_fun_callback;
    /* ����bss transition request����֡�ڴ� */
    pst_bsst_req_buf = OAL_MEM_NETBUF_ALLOC(OAL_MGMT_NETBUF, WLAN_MGMT_NETBUF_SIZE, OAL_NETBUF_PRIORITY_HIGH);
    if (OAL_PTR_NULL == pst_bsst_req_buf)
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BSSTRANSITION, "{dmac_tx_bsst_req_action::pst_bsst_req_buf null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAL_MEM_NETBUF_TRACE(pst_bsst_req_buf, OAL_TRUE);
    OAL_NETBUF_PREV(pst_bsst_req_buf) = OAL_PTR_NULL;
    OAL_NETBUF_NEXT(pst_bsst_req_buf) = OAL_PTR_NULL;
#ifdef _PRE_DEBUG_MODE
    OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BSSTRANSITION, "{dmac_tx_bsst_req_action::encap bt req action start.}");
#endif
    /* ���÷�װ����֡�ӿ� */
    us_frame_len = dmac_encap_bsst_req_action(pst_dmac_vap, pst_dmac_user, pst_bsst_req_info, pst_bsst_req_buf);
    if (0 == us_frame_len)
    {
        oal_netbuf_free(pst_bsst_req_buf);
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BSSTRANSITION, "{dmac_tx_bsst_req_action::encap btq action failed.}");
        return OAL_FAIL;
    }
    /* ��дnetbuf��cb�ֶΣ������͹���֡�ͷ�����ɽӿ�ʹ�� */
    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_bsst_req_buf);
    OAL_MEMZERO(pst_tx_ctl, sizeof(mac_tx_ctl_stru));
    MAC_GET_CB_TX_USER_IDX(pst_tx_ctl) = pst_dmac_user->st_user_base_info.us_assoc_id;
    MAC_GET_CB_WME_AC_TYPE(pst_tx_ctl) = WLAN_WME_AC_MGMT;

    /* ������ʱ�� �ȴ�����response֡ */
    FRW_TIMER_CREATE_TIMER(&(pst_dmac_user->pst_11v_ctrl_info->st_status_wait_timer),
                        dmac_rx_bsst_rsp_timeout,
                        DMAC_11V_WAIT_STATUS_TIMEOUT,
                        (oal_void *)pst_dmac_user,
                        OAL_FALSE,
                        OAM_MODULE_ID_DMAC,
                        pst_dmac_device->pst_device_base_info->ul_core_id);
    /* ���÷��͹���֡�ӿ� */
    ul_ret = dmac_tx_mgmt(pst_dmac_vap, pst_bsst_req_buf, us_frame_len);
    if (OAL_SUCC != ul_ret)
    {
        oal_netbuf_free(pst_bsst_req_buf);
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BSSTRANSITION, "{dmac_tx_bsst_req_action::tx bsst request action failed.}");
        /* ����ʧ�ܺ�رռ�ʱ��  */
        if(pst_dmac_user->pst_11v_ctrl_info->st_status_wait_timer.en_is_registerd)
        {
            FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_dmac_user->pst_11v_ctrl_info->st_status_wait_timer));
        }
        return ul_ret;
    }
    // ������ɺ� ���û���11V״̬����Ϊ�ȴ�����Response
    pst_dmac_user->pst_11v_ctrl_info->uc_user_status = DMAC_11V_AP_STATUS_WAIT_RSP;
    return OAL_SUCC;

}


oal_uint16  dmac_encap_bsst_req_action(dmac_vap_stru *pst_dmac_vap,
                                       dmac_user_stru *pst_dmac_user,
                                       dmac_bsst_req_info_stru *pst_bsst_req_info,
                                       oal_netbuf_stru *pst_buffer)
{
    oal_uint16      us_index = 0;
    oal_uint16      us_len = 0;
    oal_uint8       *puc_mac_header   = OAL_PTR_NULL;
    oal_uint8       *puc_payload_addr = OAL_PTR_NULL;
    dmac_bsst_req_mode_stru *pst_mode_info = OAL_PTR_NULL;

    /* ��μ�� */
    if ((OAL_PTR_NULL == pst_dmac_vap) || (OAL_PTR_NULL == pst_dmac_user) || (OAL_PTR_NULL == pst_bsst_req_info) || (OAL_PTR_NULL == pst_buffer))
    {
        OAM_ERROR_LOG4(0, OAM_SF_BSSTRANSITION, "{dmac_encap_bsst_req_action::null param.vap:%x user:%x req info:%x buf:%x}",
            pst_dmac_vap,pst_dmac_user,pst_bsst_req_info,pst_buffer);
        return 0;
    }
    if(OAL_PTR_NULL == pst_dmac_user->pst_11v_ctrl_info)
    {
        OAM_ERROR_LOG1(0, OAM_SF_BSSTRANSITION, "{dmac_encap_bsst_req_action::null param. ctrl info:0x%x}", pst_dmac_user->pst_11v_ctrl_info);
        return 0;
    }
    /* URL��Ϊ�ַ�����ʽ�ҳ��Ȳ��ܳ���50 */
    if(pst_bsst_req_info->puc_session_url)
    {
        if (DMAC_11V_MAX_URL_LENGTH < OAL_STRLEN((oal_int8 *)pst_bsst_req_info->puc_session_url))
        {
            OAM_ERROR_LOG0(0, OAM_SF_BSSTRANSITION, "{dmac_encap_bsst_req_action::session url length over 50 or abnormal format of string.}");
            return 0;
        }
    }
    puc_mac_header   = oal_netbuf_header(pst_buffer);
    puc_payload_addr = mac_netbuf_get_payload(pst_buffer);
    /*************************************************************************/
    /*                        Management Frame Format                        */
    /* --------------------------------------------------------------------  */
    /* |Frame Control|Duration|DA|SA|BSSID|Sequence Control|Frame Body|FCS|  */
    /* --------------------------------------------------------------------  */
    /* | 2           |2       |6 |6 |6    |2               |0 - 2312  |4  |  */
    /* --------------------------------------------------------------------  */
    /*                                                                       */
    /*************************************************************************/

    /*************************************************************************/
    /*                Set the fields in the frame header                     */
    /*************************************************************************/

    /* Frame Control Field ��ֻ��Ҫ����Type/Subtypeֵ����������Ϊ0 */
    mac_hdr_set_frame_control(puc_mac_header, WLAN_PROTOCOL_VERSION| WLAN_FC0_TYPE_MGT | WLAN_FC0_SUBTYPE_ACTION);
    /* DA is address of STA addr */
    oal_set_mac_addr(puc_mac_header + WLAN_HDR_ADDR1_OFFSET, pst_dmac_user->st_user_base_info.auc_user_mac_addr);
    /* SA��ֵΪ�����MAC��ַ */
    oal_set_mac_addr(puc_mac_header + WLAN_HDR_ADDR2_OFFSET, mac_mib_get_StationID(&pst_dmac_vap->st_vap_base_info));
    /* TA��ֵΪVAP��BSSID */
    oal_set_mac_addr(puc_mac_header + WLAN_HDR_ADDR3_OFFSET, pst_dmac_vap->st_vap_base_info.auc_bssid);

    /*************************************************************************************************************/
    /*                                  Set the contents of the frame body                                       */
    /*************************************************************************************************************/
    /*************************************************************************************************************/
    /*                            BSS Transition Request Frame - Frame Body                                      */
    /* ----------------------------------------------------------------------------------------------------------*/
    /* |Category |Action | Token| Mode | Disassoc Timer | V Inter | Duration |    URL  |BSS Candidate List Entry */
    /* --------------------------------------------------------------------------------------------------------- */
    /* |1        |1      | 1    |  1   | 2              | 1       | 0~12     |Variable | Variable                */
    /* --------------------------------------------------------------------------------------------------------- */
    /*                                                                                                           */
    /*************************************************************************************************************/


    /* ������ָ��frame body��ʼλ�� */
    us_index = 0;
    /* ����Category */
    puc_payload_addr[us_index++] = MAC_ACTION_CATEGORY_WNM;
    /* ����Action */
    puc_payload_addr[us_index++] = MAC_WNM_ACTION_BSS_TRANSITION_MGMT_REQUEST;
    /* ����Dialog Token */
    puc_payload_addr[us_index++] = pst_dmac_user->pst_11v_ctrl_info->uc_user_bsst_token;
    /* ����Request mode */
    pst_mode_info = (dmac_bsst_req_mode_stru*)(puc_payload_addr+us_index);
    pst_mode_info->bit_candidate_list_include = pst_bsst_req_info->st_request_mode.bit_candidate_list_include;
    pst_mode_info->bit_abridged = pst_bsst_req_info->st_request_mode.bit_abridged;
    pst_mode_info->bit_bss_disassoc_imminent = pst_bsst_req_info->st_request_mode.bit_bss_disassoc_imminent;
    pst_mode_info->bit_termination_include = pst_bsst_req_info->st_request_mode.bit_termination_include;
    pst_mode_info->bit_ess_disassoc_imminent = pst_bsst_req_info->st_request_mode.bit_ess_disassoc_imminent;
    pst_mode_info->bit_rev = 0;
    us_index++;
    /* ����Disassociation timer */
    puc_payload_addr[us_index] = (oal_uint8)(pst_bsst_req_info->us_disassoc_time);
    puc_payload_addr[us_index+1] = (oal_uint8)(pst_bsst_req_info->us_disassoc_time >> 8);
    us_index += 2;
    /* ����Vilidity Interval */
    puc_payload_addr[us_index++] = pst_bsst_req_info->uc_validity_interval;
    /* ����Termination */
    if(pst_mode_info->bit_termination_include)
    {
        puc_payload_addr[us_index++] = DMAC_NEIGH_SUB_ID_TERM_DURATION;                 /* termination duration Ԫ��IDΪ4 */
        puc_payload_addr[us_index++] = DMAC_11V_TERMINATION_ELEMENT_LEN;                /* termination duration ���ȹ̶�Ϊ10 */
        oal_memcopy(puc_payload_addr+us_index, pst_bsst_req_info->st_term_duration.auc_termination_tsf, DMAC_11V_TERMINATION_TSF_LENGTH);
        us_index += DMAC_11V_TERMINATION_TSF_LENGTH;
        puc_payload_addr[us_index++] = (oal_uint8)(pst_bsst_req_info->st_term_duration.us_duration_min);
        puc_payload_addr[us_index++] = (oal_uint8)(pst_bsst_req_info->st_term_duration.us_duration_min >> 8);
    }
    /* ����URL ��mode��ӦλΪ1ʱ ������url */
    if(pst_mode_info->bit_ess_disassoc_imminent)
    {
        /* ��Ĭ�Ͻ���������Ϊ0 ��URLָ�벻Ϊ��ʱ������URL���� */
        puc_payload_addr[us_index] = 0;
        if(pst_bsst_req_info->puc_session_url)
        {
            puc_payload_addr[us_index] = (oal_uint8)OAL_STRLEN((oal_int8 *)pst_bsst_req_info->puc_session_url);
            oal_memcopy(puc_payload_addr+(us_index+1), pst_bsst_req_info->puc_session_url, OAL_STRLEN((oal_int8 *)pst_bsst_req_info->puc_session_url));
            us_index += (oal_uint16)OAL_STRLEN((oal_int8 *)pst_bsst_req_info->puc_session_url);
        }
        us_index ++;
    }
    /* ����Candidate BSS List */
    if(pst_mode_info->bit_candidate_list_include)
    {
#ifdef _PRE_DEBUG_MODE
        OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BSSTRANSITION, "{dmac_encap_bsst_req_action:: set neighbor rpt ie.}");
#endif
        puc_payload_addr += us_index;
        dmac_set_neighbor_ie(pst_bsst_req_info->pst_neighbor_bss_list, pst_bsst_req_info->uc_bss_list_num, puc_payload_addr, &us_len);
        us_index += us_len;
    }
#ifdef _PRE_DEBUG_MODE
    OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BSSTRANSITION, "{dmac_encap_bsst_req_action::LEN = %d.}", us_index + MAC_80211_FRAME_LEN);
#endif
    return us_index + MAC_80211_FRAME_LEN;
}


oal_uint32  dmac_rx_bsst_req_action(dmac_vap_stru *pst_dmac_vap, dmac_user_stru *pst_dmac_user, oal_netbuf_stru *pst_netbuf)
{
    oal_uint16                      us_handle_len = 0;
    dmac_rx_ctl_stru                *pst_rx_ctrl;
    mac_rx_ctl_stru                 *pst_rx_info;
    oal_uint16                      us_frame_len = 0;
    oal_uint8                       *puc_data = OAL_PTR_NULL;
    dmac_bsst_req_info_stru         st_bsst_req_info;
    dmac_bsst_rsp_info_stru         st_bsst_rsp_info;
    oal_uint16                      us_url_count = 0;

    if ((OAL_PTR_NULL == pst_dmac_vap) || (OAL_PTR_NULL == pst_dmac_user) || (OAL_PTR_NULL == pst_netbuf))
    {
        OAM_ERROR_LOG3(0, OAM_SF_BSSTRANSITION, "{dmac_rx_bsst_req_action::null param, vap:0x%x user:0x%x buf:0x%x.}", pst_dmac_vap, pst_dmac_user, pst_netbuf);
        return OAL_ERR_CODE_PTR_NULL;
    }
    if (OAL_PTR_NULL == pst_dmac_user->pst_11v_ctrl_info)
    {
        OAM_ERROR_LOG1(0, OAM_SF_BSSTRANSITION, "{dmac_rx_bsst_req_action::null param. pst_11v_ctrl_info:%x}",pst_dmac_user->pst_11v_ctrl_info);
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_rx_ctrl         = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
    pst_rx_info         = (mac_rx_ctl_stru *)(&(pst_rx_ctrl->st_rx_info));
    /* ��ȡ֡��ָ�� */
    puc_data = MAC_GET_RX_PAYLOAD_ADDR(pst_rx_info,pst_netbuf);
    us_frame_len = MAC_GET_RX_CB_PAYLOAD_LEN(pst_rx_info);  /*֡�峤��*/
#ifdef _PRE_DEBUG_MODE
    OAM_WARNING_LOG0(0, OAM_SF_BSSTRANSITION, "{dmac_rx_bsst_req_action::handle up bt req start.}");
#endif
    /* ֡�����С����Ϊ7 С��7���ʽ�쳣 */
    if(DMAC_11V_REQUEST_FRAME_BODY_FIX > us_frame_len)
    {
        OAM_ERROR_LOG1(0, OAM_SF_BSSTRANSITION, "{dmac_rx_bsst_req_action:: frame length error %d.}", us_frame_len);
        return OAL_FAIL;
    }

    /* ��֡�ĸ��ֲ����������� ���ϲ���� */
    /* ����Token ����뵱ǰ�û��²�һ�� ˢ��Token */
    if(puc_data[2] != pst_dmac_user->pst_11v_ctrl_info->uc_user_bsst_token)
    {
        pst_dmac_user->pst_11v_ctrl_info->uc_user_bsst_token = puc_data[2];
    }
    /* ����request mode */
    st_bsst_req_info.st_request_mode.bit_candidate_list_include = puc_data[3]&BIT0;
    st_bsst_req_info.st_request_mode.bit_abridged = (puc_data[3]&BIT1)? OAL_TRUE : OAL_FALSE;
    st_bsst_req_info.st_request_mode.bit_bss_disassoc_imminent = (puc_data[3]&BIT2)? OAL_TRUE : OAL_FALSE;
    st_bsst_req_info.st_request_mode.bit_termination_include = (puc_data[3]&BIT3)? OAL_TRUE : OAL_FALSE;
    st_bsst_req_info.st_request_mode.bit_ess_disassoc_imminent = (puc_data[3]&BIT4)? OAL_TRUE : OAL_FALSE;

    st_bsst_req_info.us_disassoc_time = ((oal_uint16)(puc_data[5]) << 8 ) | puc_data[4];
    st_bsst_req_info.uc_validity_interval = puc_data[6];
    us_handle_len = 7;              /* ǰ��7���ֽ��ѱ������� */
    /* 12�ֽڵ�termination duration ����еĻ� */
    if (st_bsst_req_info.st_request_mode.bit_termination_include)
    {
        us_handle_len += MAC_IE_HDR_LEN;                /* ȥ��Ԫ��ͷ */
        oal_memcopy(st_bsst_req_info.st_term_duration.auc_termination_tsf, puc_data+us_handle_len, DMAC_11V_TERMINATION_TSF_LENGTH);
        us_handle_len += DMAC_11V_TERMINATION_TSF_LENGTH;
        st_bsst_req_info.st_term_duration.us_duration_min = (((oal_uint16)puc_data[us_handle_len+1]) << 8) | (puc_data[us_handle_len]);
        us_handle_len += 2;
    }
    /* ����URL */
    /* URL�ֶ� ����еĻ� URL��һ���ֽ�ΪURL���� ���붯̬�ڴ汣�� */
    st_bsst_req_info.puc_session_url = OAL_PTR_NULL;
    if (st_bsst_req_info.st_request_mode.bit_ess_disassoc_imminent)
    {
        if (0 != puc_data[us_handle_len])
        {
            /* �����ڴ�������1 ���ڴ���ַ��������� */
            us_url_count = puc_data[us_handle_len]*OAL_SIZEOF(oal_uint8)+1;
            st_bsst_req_info.puc_session_url = (oal_uint8 *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL,us_url_count,OAL_TRUE);
            if(OAL_PTR_NULL == st_bsst_req_info.puc_session_url)
            {
                OAM_ERROR_LOG0(0, OAM_SF_BSSTRANSITION, "{dmac_rx_bsst_req_action:: puc_session_url alloc fail.}");
                return OAL_FAIL;
            }
            oal_memcopy(st_bsst_req_info.puc_session_url, puc_data+(us_handle_len+1), puc_data[us_handle_len]);
            // ת�����ַ���
            st_bsst_req_info.puc_session_url[puc_data[us_handle_len]] = '\0';
        }
        us_handle_len += (puc_data[us_handle_len] + 1);
    }
    /* Candidate bss list����STA��Response frameΪ��ѡ ��Ҫ�����������ڴ˽ṹ���� ���ϲ㴦�� */
    st_bsst_req_info.pst_neighbor_bss_list = OAL_PTR_NULL;
    if (st_bsst_req_info.st_request_mode.bit_candidate_list_include)
    {
        puc_data += us_handle_len;
        dmac_get_neighbor_ie(&st_bsst_req_info.pst_neighbor_bss_list, puc_data, us_frame_len-us_handle_len, &st_bsst_req_info.uc_bss_list_num);
    }

    /* ѡȡ��һ��BSS ��ΪĿ��BSS ����Response��AP */
    st_bsst_rsp_info.uc_status_code = 0;                            /* Ĭ������Ϊͬ���л� */
    st_bsst_rsp_info.uc_termination_delay = 0;                      /* ����״̬��Ϊ5ʱ��Ч���˴���������Ϊ0 */
    /* �����ò����ж� ��Ϊrequest֡�д��к�ѡAP�б� */
    if(OAL_PTR_NULL != st_bsst_req_info.pst_neighbor_bss_list)
    {
        oal_memcopy(st_bsst_rsp_info.auc_target_bss_addr, st_bsst_req_info.pst_neighbor_bss_list->auc_mac_addr, WLAN_MAC_ADDR_LEN);
    }
    /* ����Response */
    dmac_tx_bsst_rsp_action(pst_dmac_vap, pst_dmac_user, &st_bsst_rsp_info);

    /* �ͷ�ָ�� */
    if(OAL_PTR_NULL != st_bsst_req_info.puc_session_url)
    {
        OAL_MEM_FREE(st_bsst_req_info.puc_session_url, OAL_TRUE);
        st_bsst_req_info.puc_session_url = OAL_PTR_NULL;
    }
    if(OAL_PTR_NULL != st_bsst_req_info.pst_neighbor_bss_list)
    {
        OAL_MEM_FREE(st_bsst_req_info.pst_neighbor_bss_list, OAL_TRUE);
        st_bsst_req_info.pst_neighbor_bss_list = OAL_PTR_NULL;
    }

    return OAL_SUCC;
}


oal_uint32  dmac_tx_bsst_rsp_action(dmac_vap_stru *pst_dmac_vap, dmac_user_stru *pst_dmac_user,  dmac_bsst_rsp_info_stru *pst_bsst_rsp_info)
{
    oal_netbuf_stru                  *pst_bsst_rsp_buf;
    oal_uint16                        us_frame_len;
    mac_tx_ctl_stru                  *pst_tx_ctl;
    oal_uint32                        ul_ret;

    if ((OAL_PTR_NULL == pst_dmac_vap) || (OAL_PTR_NULL == pst_dmac_user) || (OAL_PTR_NULL == pst_bsst_rsp_info))
    {
        OAM_ERROR_LOG3(0, OAM_SF_BSSTRANSITION, "{dmac_tx_bsst_rsp_action::null param, %x %x %x.}", pst_dmac_vap, pst_dmac_user, pst_bsst_rsp_info);
        return OAL_ERR_CODE_PTR_NULL;
    }
    if (OAL_PTR_NULL == pst_dmac_user->pst_11v_ctrl_info)
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BSSTRANSITION, "{dmac_tx_bsst_rsp_action::pst_11v_ctrl_info null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    /* ����bss transition request����֡�ڴ� */
    pst_bsst_rsp_buf = OAL_MEM_NETBUF_ALLOC(OAL_MGMT_NETBUF, WLAN_MGMT_NETBUF_SIZE, OAL_NETBUF_PRIORITY_HIGH);
    if (OAL_PTR_NULL == pst_bsst_rsp_buf)
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BSSTRANSITION, "{dmac_tx_bsst_rsp_action::pst_bsst_rsq_buf null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAL_MEM_NETBUF_TRACE(pst_bsst_rsp_buf, OAL_TRUE);
    OAL_NETBUF_PREV(pst_bsst_rsp_buf) = OAL_PTR_NULL;
    OAL_NETBUF_NEXT(pst_bsst_rsp_buf) = OAL_PTR_NULL;
#ifdef _PRE_DEBUG_MODE
    OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BSSTRANSITION, "{dmac_tx_bsst_rsp_action::encap rsp start.}");
#endif
    /* ���÷�װ����֡�ӿ� */
    us_frame_len = dmac_encap_bsst_rsp_action(pst_dmac_vap, pst_dmac_user, pst_bsst_rsp_info, pst_bsst_rsp_buf);
    if (0 == us_frame_len)
    {
        oal_netbuf_free(pst_bsst_rsp_buf);
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BSSTRANSITION, "{dmac_tx_bsst_rsp_action::encap btq action failed.}");
        return OAL_FAIL;
    }
    /* ��ʼ��CB */
    OAL_MEMZERO(oal_netbuf_cb(pst_bsst_rsp_buf), OAL_NETBUF_CB_SIZE());
    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_bsst_rsp_buf);
    MAC_GET_CB_TX_USER_IDX(pst_tx_ctl) = pst_dmac_user->st_user_base_info.us_assoc_id;
    MAC_GET_CB_WME_AC_TYPE(pst_tx_ctl) = WLAN_WME_AC_MGMT;

    /* ���÷��͹���֡�ӿ� */
    ul_ret = dmac_tx_mgmt(pst_dmac_vap, pst_bsst_rsp_buf, us_frame_len);
    if (OAL_SUCC != ul_ret)
    {
        oal_netbuf_free(pst_bsst_rsp_buf);
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BSSTRANSITION, "{dmac_tx_bsst_req_action::tx bsst request action failed.}");
        return ul_ret;
    }
    /* STA������Response�� һ�ν������̾������ ��Ҫ��user�µ�Tokenֵ��1 ���´η���ʹ�� */
    if( DMAC_11V_TOKEN_MAX_VALUE == pst_dmac_user->pst_11v_ctrl_info->uc_user_bsst_token)
    {
        pst_dmac_user->pst_11v_ctrl_info->uc_user_bsst_token = 1;
    }
    else
    {
        pst_dmac_user->pst_11v_ctrl_info->uc_user_bsst_token++;
    }

    return OAL_SUCC;
}


oal_uint16  dmac_encap_bsst_rsp_action(dmac_vap_stru *pst_dmac_vap,
                                       dmac_user_stru *pst_dmac_user,
                                       dmac_bsst_rsp_info_stru *pst_bsst_rsp_info,
                                       oal_netbuf_stru *pst_buffer)
{
    oal_uint16  us_index = 0;
    oal_uint8       *puc_mac_header   = OAL_PTR_NULL;
    oal_uint8       *puc_payload_addr = OAL_PTR_NULL;

    if ((OAL_PTR_NULL == pst_dmac_vap) || (OAL_PTR_NULL == pst_dmac_user) || (OAL_PTR_NULL == pst_bsst_rsp_info) || (OAL_PTR_NULL == pst_buffer))
    {
        OAM_ERROR_LOG4(0, OAM_SF_BSSTRANSITION, "{dmac_encap_bsst_rsp_action::null param.vap:%x user:%x info:%x buf:%x}",
            pst_dmac_vap,pst_dmac_user,pst_bsst_rsp_info,pst_buffer);
        return 0;
    }
    if (OAL_PTR_NULL == pst_dmac_user->pst_11v_ctrl_info)
    {
        OAM_ERROR_LOG0(0, OAM_SF_BSSTRANSITION, "{dmac_encap_bsst_rsp_action::pst_11v_ctrl_info null.}");
        return 0;
    }
    puc_mac_header   = oal_netbuf_header(pst_buffer);
    puc_payload_addr = mac_netbuf_get_payload(pst_buffer);
    /*************************************************************************/
    /*                        Management Frame Format                        */
    /* --------------------------------------------------------------------  */
    /* |Frame Control|Duration|DA|SA|BSSID|Sequence Control|Frame Body|FCS|  */
    /* --------------------------------------------------------------------  */
    /* | 2           |2       |6 |6 |6    |2               |0 - 2312  |4  |  */
    /* --------------------------------------------------------------------  */
    /*                                                                       */
    /*************************************************************************/

    /*************************************************************************/
    /*                Set the fields in the frame header                     */
    /*************************************************************************/

    /* Frame Control Field ��ֻ��Ҫ����Type/Subtypeֵ����������Ϊ0 */
    mac_hdr_set_frame_control(puc_mac_header, WLAN_PROTOCOL_VERSION| WLAN_FC0_TYPE_MGT | WLAN_FC0_SUBTYPE_ACTION);
    /* DA is address of STA addr */
    oal_set_mac_addr(puc_mac_header + WLAN_HDR_ADDR1_OFFSET, pst_dmac_user->st_user_base_info.auc_user_mac_addr);
    /* SA��ֵΪ�����MAC��ַ */
    oal_set_mac_addr(puc_mac_header + WLAN_HDR_ADDR2_OFFSET, mac_mib_get_StationID(&pst_dmac_vap->st_vap_base_info));
    /* TA��ֵΪVAP��BSSID */
    oal_set_mac_addr(puc_mac_header + WLAN_HDR_ADDR3_OFFSET, pst_dmac_vap->st_vap_base_info.auc_bssid);

    /*************************************************************************************************************/
    /*                                  Set the contents of the frame body                                       */
    /*************************************************************************************************************/
    /*************************************************************************************************************/
    /*                       BSS Transition Response Frame - Frame Body                                      */
    /* ----------------------------------------------------------------------------------------------------------*/
    /* |Category |Action | Token| Status Code | Termination Delay | Target BSSID |   BSS Candidate List Entry    */
    /* --------------------------------------------------------------------------------------------------------- */
    /* |1        |1      | 1    |  1          | 1                 | 0-6          |    Optional                   */
    /* --------------------------------------------------------------------------------------------------------- */
    /*                                                                                                           */
    /*************************************************************************************************************/

    /* ������ָ��frame body��ʼλ�� */
    us_index = 0;
    /* ����Category */
    puc_payload_addr[us_index] = MAC_ACTION_CATEGORY_WNM;
    us_index ++;
    /* ����Action */
    puc_payload_addr[us_index] = MAC_WNM_ACTION_BSS_TRANSITION_MGMT_RESPONSE;
    us_index ++;
    /* ����Dialog Token */
    puc_payload_addr[us_index] = pst_dmac_user->pst_11v_ctrl_info->uc_user_bsst_token;
    us_index ++;
    /* ����Status Code */
    puc_payload_addr[us_index] = pst_bsst_rsp_info->uc_status_code;
    us_index ++;
    /* ����Termination Delay */
    puc_payload_addr[us_index] = pst_bsst_rsp_info->uc_termination_delay;
    us_index ++;
    /* ����Target BSSID */
    if( 0 == pst_bsst_rsp_info->uc_status_code)
    {
        oal_memcopy(puc_payload_addr+us_index, pst_bsst_rsp_info->auc_target_bss_addr, WLAN_MAC_ADDR_LEN);
        us_index += WLAN_MAC_ADDR_LEN;
    }
    /* ��ѡ�ĺ�ѡAP�б� ����� ���ٴ���ռ�� */
#ifdef _PRE_DEBUG_MODE
    OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BSSTRANSITION, "{dmac_encap_bsst_rsp_action::LEN = %d.}", us_index + MAC_80211_FRAME_LEN);
#endif
    return us_index + MAC_80211_FRAME_LEN;
}


oal_uint32  dmac_rx_bsst_rsp_action(dmac_vap_stru *pst_dmac_vap, dmac_user_stru *pst_dmac_user, oal_netbuf_stru *pst_netbuf)
{
    oal_uint16                  us_frame_len  = 0;
    oal_uint16                  us_handle_len   = 0;
    oal_uint8                   *puc_data;
    dmac_rx_ctl_stru            *pst_rx_ctrl;
    mac_rx_ctl_stru             *pst_rx_info;
    dmac_bsst_rsp_info_stru     st_bsst_rsp_info;

    if ((OAL_PTR_NULL == pst_dmac_vap) || (OAL_PTR_NULL == pst_dmac_user) || (OAL_PTR_NULL == pst_netbuf))
    {
        OAM_ERROR_LOG3(0, OAM_SF_BSSTRANSITION, "{dmac_rx_bsst_rsp_action::null param, 0x%x 0x%x 0x%x.}", pst_dmac_vap, pst_dmac_user, pst_netbuf);
        return OAL_ERR_CODE_PTR_NULL;
    }
    if (OAL_PTR_NULL == pst_dmac_user->pst_11v_ctrl_info)
    {
        OAM_ERROR_LOG1(0, OAM_SF_BSSTRANSITION, "{dmac_rx_bsst_rsp_action::null param. pst_11v_ctrl_info:%x}",pst_dmac_user->pst_11v_ctrl_info);
        return OAL_ERR_CODE_PTR_NULL;
    }
    /* �����ʱ״̬���ѱ�� ��ResponseҲӦ�ö��� */
    if(DMAC_11V_AP_STATUS_WAIT_RSP != pst_dmac_user->pst_11v_ctrl_info->uc_user_status)
    {
#ifdef _PRE_DEBUG_MODE
        OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BSSTRANSITION, "{dmac_rx_bsst_rsp_action::ap status change =%d.}",pst_dmac_user->pst_11v_ctrl_info->uc_user_status);
#endif
        return OAL_SUCC;
    }
    pst_rx_ctrl         = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
    pst_rx_info         = (mac_rx_ctl_stru *)(&(pst_rx_ctrl->st_rx_info));
    /* ��ȡ֡��ָ�� */
    puc_data = MAC_GET_RX_PAYLOAD_ADDR(pst_rx_info,pst_netbuf);
    us_frame_len = MAC_GET_RX_CB_PAYLOAD_LEN(pst_rx_info);  /*֡�峤��*/
#ifdef _PRE_DEBUG_MODE
    OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BSSTRANSITION, "{dmac_rx_bsst_rsp_action::bsst response frame len =%d.}",us_frame_len);
#endif
    /* ֡�����С����Ϊ5 С��4���ʽ�쳣 */
    if(DMAC_11V_RESPONSE_FRAME_BODY_FIX > us_frame_len)
    {
        OAM_ERROR_LOG1(0, OAM_SF_BSSTRANSITION, "{dmac_rx_bsst_rsp_action:: frame length error %d.}", us_frame_len);
        return OAL_FAIL;
    }
    /* ������һ�� ������֡ */
    if(puc_data[2] != pst_dmac_user->pst_11v_ctrl_info->uc_user_bsst_token)
    {
#ifdef _PRE_DEBUG_MODE
        OAM_WARNING_LOG2(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BSSTRANSITION, "{dmac_rx_bsst_rsp_action::different token receive[%d] origin[%d].}",
        puc_data[2],pst_dmac_user->pst_11v_ctrl_info->uc_user_bsst_token);
#endif
        return OAL_SUCC;
    }
    /* ����֡���� ��������Ӧģ��Ļص��������д��� */
    st_bsst_rsp_info.uc_status_code = puc_data[3];
    st_bsst_rsp_info.uc_termination_delay = puc_data[4];
    us_handle_len = DMAC_11V_RESPONSE_FRAME_BODY_FIX;
    /* ������ڲŴ���  */
    if(DMAC_11V_RESPONSE_ACCEPT == st_bsst_rsp_info.uc_status_code)
    {
        oal_memcopy(st_bsst_rsp_info.auc_target_bss_addr, puc_data+us_handle_len, WLAN_MAC_ADDR_LEN);
        us_handle_len += WLAN_MAC_ADDR_LEN;
    }
    st_bsst_rsp_info.pst_neighbor_bss_list = OAL_PTR_NULL;
    dmac_get_neighbor_ie(&st_bsst_rsp_info.pst_neighbor_bss_list,puc_data+us_handle_len, us_frame_len-us_handle_len, &st_bsst_rsp_info.uc_bss_list_num);
    /* �����ʱ�������� �����ٶ�ʱ�� �ͷ��ڴ� */
    if(pst_dmac_user->pst_11v_ctrl_info->st_status_wait_timer.en_is_registerd)
    {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&pst_dmac_user->pst_11v_ctrl_info->st_status_wait_timer);
    }
    pst_dmac_user->pst_11v_ctrl_info->uc_user_status = DMAC_11V_AP_STATUS_INIT;
    /* AP���յ�response���ʾһ�ν������ �����1 ���´�ʹ�� */
    if( DMAC_11V_TOKEN_MAX_VALUE == pst_dmac_user->pst_11v_ctrl_info->uc_user_bsst_token)
    {
        pst_dmac_user->pst_11v_ctrl_info->uc_user_bsst_token = 1;
    }
    else
    {
        pst_dmac_user->pst_11v_ctrl_info->uc_user_bsst_token++;
    }
    // ���ûص��������в����ش�
    if(pst_dmac_user->pst_11v_ctrl_info->dmac_11v_callback_fn)
    {
        pst_dmac_user->pst_11v_ctrl_info->dmac_11v_callback_fn((oal_void*)pst_dmac_user,DMAC_11V_CALLBACK_RETURN_REICEVE_RSP, (oal_void*)&st_bsst_rsp_info);
    }
    // �ͷ��ڴ�
    if(OAL_PTR_NULL != st_bsst_rsp_info.pst_neighbor_bss_list)
    {
        OAL_MEM_FREE(st_bsst_rsp_info.pst_neighbor_bss_list,OAL_TRUE);
        st_bsst_rsp_info.pst_neighbor_bss_list = OAL_PTR_NULL;
    }
    return OAL_SUCC;
}



oal_void  dmac_set_neighbor_ie(dmac_neighbor_bss_info_stru *pst_neighbor_bss, oal_uint8 uc_bss_num, oal_uint8 *puc_buffer, oal_uint16 *pus_total_ie_len)
{
    oal_uint8   uc_bss_list_num = 0;
    oal_uint8   uc_ie_fix_len = 13;      /* ������ѡ��Ԫ�� ��Neighbor Report IE����Ϊ13���ֽ� */
    oal_uint16  us_total_ie_len = 0;
    oal_uint8   uc_ie_len = 0;
    oal_uint8   uc_bss_list_index = 0;
    oal_bssid_infomation_stru      *pst_bss_info = OAL_PTR_NULL;
    if (OAL_UNLIKELY((OAL_PTR_NULL == puc_buffer) || (OAL_PTR_NULL == pus_total_ie_len) || (OAL_PTR_NULL == pst_neighbor_bss)))
    {
        if(OAL_PTR_NULL != pus_total_ie_len)
        {
            *pus_total_ie_len = 0;
        }
        OAM_ERROR_LOG3(0, OAM_SF_BSSTRANSITION, "{dmac_set_neighbor_ie::null param.buf:%x ie_len:%x bss:%x}",
            puc_buffer,pus_total_ie_len,pst_neighbor_bss);
        return ;
    }
    /*  Neighbor Report Information Element Format                                                                */
    /* ---------------------------------------------------------------------------------------------------------- */
    /* | Element ID | Length | BSSID | BSSID Info | Operating Class | Chnl Num | PHY TYPE | Optional Subelement | */
    /* ---------------------------------------------------------------------------------------------------------- */
    /* | 1          | 1      | 6     | 4          | 1               | 1        | 1        | Variable            | */
    /* ---------------------------------------------------------------------------------------------------------- */

    /* ����Neighbor Report Element */
    uc_bss_list_num = uc_bss_num;        /* �û��ڽ���BSS���� */
#ifdef _PRE_DEBUG_MODE
        OAM_WARNING_LOG1(0, OAM_SF_BSSTRANSITION, "{dmac_set_neighbor_ie:: neighbor rpt ie number [%d].}",uc_bss_list_num);
#endif
    if(DMAC_MAX_BSS_NEIGHBOR_LIST < uc_bss_list_num)
    {
        /* Ŀǰ����֡�����ڴ�800�ֽ� ֡�Դ���󳤶�19(frame boady)+50(url)+ N*(15(neighbor)+3(sub)+12(sub))  �����ڳ��� �޸���������ʱ��ע�� */
        uc_bss_list_num = DMAC_MAX_BSS_NEIGHBOR_LIST;           /* ��������Ϊ���ֵ ������IE�������� */
    }
    for(uc_bss_list_index=0; uc_bss_list_index<uc_bss_list_num; uc_bss_list_index++)
    {
        uc_ie_len = 0;
        puc_buffer[0] = MAC_EID_NEIGHBOR_REPORT;
        /* ����ֻ��ȡ��IEԪ�ص�һ����,���ȱ�����Ϣ��ʧЧ,��Ҫ���¼��� */
        /* Neighbor BSSID Adress */
        oal_memcopy(puc_buffer+2, pst_neighbor_bss[uc_bss_list_index].auc_mac_addr, WLAN_MAC_ADDR_LEN);
        /* Neighbor BSSID informatin */
        pst_bss_info = (oal_bssid_infomation_stru *)(puc_buffer+8);
        oal_memcopy((oal_void*)pst_bss_info,(oal_void*)&(pst_neighbor_bss[uc_bss_list_index].st_bssid_info),OAL_SIZEOF(oal_bssid_infomation_stru));

        puc_buffer[12] = pst_neighbor_bss[uc_bss_list_index].uc_opt_class;
        puc_buffer[13] = pst_neighbor_bss[uc_bss_list_index].uc_chl_num;
        puc_buffer[14] = pst_neighbor_bss[uc_bss_list_index].uc_phy_type;
        uc_ie_len = uc_ie_fix_len + MAC_IE_HDR_LEN;
        /* candidate perference��Ԫ�� �����Ԫ�� Ĭ�ϱ�����ڸ���Ԫ�� */
        puc_buffer[uc_ie_len] = DMAC_NEIGH_SUB_ID_BSS_CANDIDATE_PERF;
        puc_buffer[uc_ie_len+1] = 1;                        //���ȹ̶�Ϊ1
        puc_buffer[uc_ie_len+2] = pst_neighbor_bss[uc_bss_list_index].uc_candidate_perf;
        uc_ie_len += 3;
        /* ����terminatin duration��Ԫ�� �������Ԫ�� */
        if(DMAC_NEIGH_SUB_ID_TERM_DURATION == pst_neighbor_bss[uc_bss_list_index].st_term_duration.uc_sub_ie_id)
        {
            puc_buffer[uc_ie_len++] = DMAC_NEIGH_SUB_ID_TERM_DURATION;
            puc_buffer[uc_ie_len++] = DMAC_11V_TERMINATION_ELEMENT_LEN;
            oal_memcopy(puc_buffer+uc_ie_len, pst_neighbor_bss[uc_bss_list_index].st_term_duration.auc_termination_tsf, DMAC_11V_TERMINATION_TSF_LENGTH);
            uc_ie_len += DMAC_11V_TERMINATION_TSF_LENGTH;
            puc_buffer[uc_ie_len++] = pst_neighbor_bss[uc_bss_list_index].st_term_duration.us_duration_min & 0x00FF;
            puc_buffer[uc_ie_len++] = (pst_neighbor_bss[uc_bss_list_index].st_term_duration.us_duration_min >> 8) & 0x00FF;
        }
        /* ����IE lengthֵ �����ܳ��ȼ�ȥIE ͷ���� */
        puc_buffer[1] = uc_ie_len - MAC_IE_HDR_LEN;

        /* ������һ��IE ������Ӧ��ָ��ͳ��� */
        puc_buffer += uc_ie_len;
        /* ����IE�������ӵ������ݳ����� */
        us_total_ie_len += uc_ie_len;
    }

    *pus_total_ie_len = us_total_ie_len;
#ifdef _PRE_DEBUG_MODE
    OAM_WARNING_LOG1(0, OAM_SF_BSSTRANSITION, "{dmac_set_neighbor_ie::Neighbor ie len = %d.}", *pus_total_ie_len);
#endif

}


oal_void  dmac_get_neighbor_ie(dmac_neighbor_bss_info_stru **pst_bss_list, oal_uint8 *puc_data, oal_uint16 us_len, oal_uint8 *puc_bss_num)
{
    oal_uint8   *puc_ie_data_find = OAL_PTR_NULL;
    oal_uint8   *puc_ie_data = OAL_PTR_NULL;
    oal_uint16  us_len_find = us_len;
    oal_uint8   uc_minmum_ie_len = 13;
    oal_uint8   uc_bss_number = 0;
    oal_uint8   uc_bss_list_index = 0;
    oal_uint8   uc_sub_ie_len = 0;
    oal_uint8   uc_neighbor_ie_len = 0;
    dmac_neighbor_bss_info_stru *pst_bss_list_alloc;

    if ( (OAL_PTR_NULL == puc_data) || (OAL_PTR_NULL == puc_bss_num) )
    {
        OAM_WARNING_LOG2(0, OAM_SF_BSSTRANSITION, "{dmac_get_neighbor_ie::null pointer puc_data[%x] puc_bss_num[%x].}", puc_data,puc_bss_num);
        if(OAL_PTR_NULL != puc_bss_num)
        {
            *puc_bss_num = 0;
        }
        return ;
    }
    /* �����֡����Ϊ0������Ҫ���н����� */
    if(0 == us_len)
    {
        *puc_bss_num = 0;
        return;
    }
    puc_ie_data_find = puc_data;
    *pst_bss_list = OAL_PTR_NULL;
    /* ��ȷ�Ϻ��ж��ٸ�neighbor list */
    while(OAL_PTR_NULL != puc_ie_data_find)
    {
        puc_ie_data =  mac_find_ie(MAC_EID_NEIGHBOR_REPORT, puc_ie_data_find, us_len_find);
        /* û�ҵ����˳�ѭ�� */
        if(OAL_PTR_NULL == puc_ie_data)
        {
            break;
        }
        uc_bss_number ++;                                   /* Neighbor Report IE ������1 */
        puc_ie_data_find += (puc_ie_data[1] + MAC_IE_HDR_LEN);
        us_len_find -= (puc_ie_data[1] + MAC_IE_HDR_LEN);
    }
#ifdef _PRE_DEBUG_MODE
    OAM_WARNING_LOG1(0, OAM_SF_BSSTRANSITION, "{dmac_get_neighbor_ie::find neighbor ie= [%d].}", uc_bss_number);
#endif
    /* ���neighbor ie ����Ϊ0 ֱ�ӷ��� */
    if(0 == uc_bss_number)
    {
        *puc_bss_num = 0;
        return ;
    }
    /* ���ݻ�ԭ���ٴδ�ͷ�������� */
    puc_ie_data_find = puc_data;
    us_len_find = us_len;
    pst_bss_list_alloc = (dmac_neighbor_bss_info_stru *) OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, uc_bss_number*OAL_SIZEOF(dmac_neighbor_bss_info_stru), OAL_TRUE);
    if(OAL_PTR_NULL == pst_bss_list_alloc)
    {
        OAM_WARNING_LOG0(0, OAM_SF_BSSTRANSITION, "{dmac_get_neighbor_ie::pst_bss_list null pointer.}");
        *puc_bss_num = 0;
        return ;
    }
    for(uc_bss_list_index=0; uc_bss_list_index<uc_bss_number; uc_bss_list_index++)
    {
        /* ǰ���Ѿ���ѯ��һ�Σ����ﲻ�᷵�ؿգ����Բ����ж� */
        puc_ie_data =  mac_find_ie(MAC_EID_NEIGHBOR_REPORT, puc_ie_data_find, us_len_find);
        uc_neighbor_ie_len = puc_ie_data[1];            // Ԫ�س���
        /* ����Neighbor Report IE�ṹ�� ֡��ֻ����subelement 3 4������subelement�ѱ����˵� */
        oal_memcopy(pst_bss_list_alloc[uc_bss_list_index].auc_mac_addr, puc_ie_data+2, WLAN_MAC_ADDR_LEN);
        /* ����BSSID Information */
        pst_bss_list_alloc[uc_bss_list_index].st_bssid_info.bit_ap_reachability = (puc_ie_data[8]&BIT1)|(puc_ie_data[8]&BIT0);        /* bit0-1 */
        pst_bss_list_alloc[uc_bss_list_index].st_bssid_info.bit_security = (puc_ie_data[8]&BIT2)? OAL_TRUE: OAL_FALSE;                /* bit2 */
        pst_bss_list_alloc[uc_bss_list_index].st_bssid_info.bit_key_scope = (puc_ie_data[8]&BIT3)? OAL_TRUE: OAL_FALSE;               /* bit3 */
        pst_bss_list_alloc[uc_bss_list_index].st_bssid_info.bit_spectrum_mgmt  = (puc_ie_data[8]&BIT4)? OAL_TRUE: OAL_FALSE;          /* bit4 */
        pst_bss_list_alloc[uc_bss_list_index].st_bssid_info.bit_qos  = (puc_ie_data[8]&BIT5)? OAL_TRUE: OAL_FALSE;                    /* bit5 */
        pst_bss_list_alloc[uc_bss_list_index].st_bssid_info.bit_apsd = (puc_ie_data[8]&BIT6)? OAL_TRUE: OAL_FALSE;                    /* bit6 */
        pst_bss_list_alloc[uc_bss_list_index].st_bssid_info.bit_radio_meas = (puc_ie_data[8]&BIT7)? OAL_TRUE: OAL_FALSE;              /* bit7 */
        pst_bss_list_alloc[uc_bss_list_index].st_bssid_info.bit_delay_block_ack = (puc_ie_data[9]&BIT0)? OAL_TRUE: OAL_FALSE;         /* bit0 */
        pst_bss_list_alloc[uc_bss_list_index].st_bssid_info.bit_immediate_block_ack = (puc_ie_data[9]&BIT1)? OAL_TRUE: OAL_FALSE;     /* bit1 */
        pst_bss_list_alloc[uc_bss_list_index].st_bssid_info.bit_mobility_domain = (puc_ie_data[9]&BIT2)? OAL_TRUE: OAL_FALSE;         /* bit2 */
        pst_bss_list_alloc[uc_bss_list_index].st_bssid_info.bit_high_throughput = (puc_ie_data[9]&BIT3)? OAL_TRUE: OAL_FALSE;         /* bit3 */
        /* �����ֶβ����� */
        pst_bss_list_alloc[uc_bss_list_index].uc_opt_class = puc_ie_data[12];
        pst_bss_list_alloc[uc_bss_list_index].uc_chl_num = puc_ie_data[13];
        pst_bss_list_alloc[uc_bss_list_index].uc_phy_type = puc_ie_data[14];
        /* ����Subelement ���ȴ�����Сie���ȲŴ���subelement ֻ����3 4 subelement */
        if(uc_neighbor_ie_len > uc_minmum_ie_len)
        {
            uc_sub_ie_len = uc_neighbor_ie_len - uc_minmum_ie_len;        /* subelement���� */
            puc_ie_data += (uc_minmum_ie_len + MAC_IE_HDR_LEN);                                 /* ֡�������ƶ���subelement�� */
            while(0 < uc_sub_ie_len)
            {
                switch(puc_ie_data[0])
                {
                case DMAC_NEIGH_SUB_ID_BSS_CANDIDATE_PERF:      /* ռ��3���ֽ� */
                    {
                        pst_bss_list_alloc[uc_bss_list_index].uc_candidate_perf = puc_ie_data[2];
                        uc_sub_ie_len -= (DMAC_11V_PERFERMANCE_ELEMENT_LEN +MAC_IE_HDR_LEN );
                        puc_ie_data += (DMAC_11V_PERFERMANCE_ELEMENT_LEN +MAC_IE_HDR_LEN );
                    }
                    break;
                case DMAC_NEIGH_SUB_ID_TERM_DURATION:   /* ռ��12���ֽ� */
                    {
                        oal_memcopy(pst_bss_list_alloc[uc_bss_list_index].st_term_duration.auc_termination_tsf, puc_ie_data+2, 8);
                        pst_bss_list_alloc[uc_bss_list_index].st_term_duration.us_duration_min = (((oal_uint16)puc_ie_data[11])<<8) | (puc_ie_data[10]);
                        uc_sub_ie_len -= (DMAC_11V_TERMINATION_ELEMENT_LEN + MAC_IE_HDR_LEN);
                        puc_ie_data +=  (DMAC_11V_TERMINATION_ELEMENT_LEN + MAC_IE_HDR_LEN);
                    }
                    break;
                /* ����IE���� ������ */
                default:
                    {
                        uc_sub_ie_len -= (puc_ie_data[1] +MAC_IE_HDR_LEN );
                        puc_ie_data += (puc_ie_data[1] +MAC_IE_HDR_LEN );
                    }
                    break;
                }
            }
        }
        puc_ie_data_find += (puc_ie_data[1] + MAC_IE_HDR_LEN);
        us_len_find -= (puc_ie_data[1] + MAC_IE_HDR_LEN);
    }

    *puc_bss_num = uc_bss_number;
    *pst_bss_list = pst_bss_list_alloc;

    return;
}



oal_uint32 dmac_rx_bsst_rsp_timeout(oal_void *p_arg)
{
    dmac_user_stru *pst_dmac_user = OAL_PTR_NULL;
    pst_dmac_user = (dmac_user_stru*) p_arg;
    if(OAL_PTR_NULL == pst_dmac_user)
    {
        OAM_ERROR_LOG0(0, OAM_SF_BSSTRANSITION, "{dmac_rx_bsst_rsp_timeout::dmac user pointer null}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    if (OAL_PTR_NULL == pst_dmac_user->pst_11v_ctrl_info)
    {
        OAM_ERROR_LOG1(0, OAM_SF_BSSTRANSITION, "{dmac_rx_bsst_rsp_timeout::null param. pst_11v_ctrl_info:%x}",pst_dmac_user->pst_11v_ctrl_info);
        return OAL_ERR_CODE_PTR_NULL;
    }
    // ���û���״̬���ָ��ɳ�ʼ״̬
    pst_dmac_user->pst_11v_ctrl_info->uc_user_status = DMAC_11V_AP_STATUS_INIT;
    // ɾ����ʱ��
    if(pst_dmac_user->pst_11v_ctrl_info->st_status_wait_timer.en_is_registerd)
    {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER( &(pst_dmac_user->pst_11v_ctrl_info->st_status_wait_timer));
    }
    /* ����response��ʱ�� �����1 �´�ʹ���µ������ */
    if( DMAC_11V_TOKEN_MAX_VALUE == pst_dmac_user->pst_11v_ctrl_info->uc_user_bsst_token)
    {
        pst_dmac_user->pst_11v_ctrl_info->uc_user_bsst_token = 1;
    }
    else
    {
        pst_dmac_user->pst_11v_ctrl_info->uc_user_bsst_token++;
    }
#ifdef _PRE_DEBUG_MODE
    OAM_WARNING_LOG0(0, OAM_SF_BSSTRANSITION, "{dmac_rx_bsst_rsp_timeout::time out.}");
#endif
    /* �ص�����ָ�벻Ϊ�� ֱ��ʹ��user����Ļص��������� */
    if(OAL_PTR_NULL != pst_dmac_user->pst_11v_ctrl_info->dmac_11v_callback_fn)
    {
        pst_dmac_user->pst_11v_ctrl_info->dmac_11v_callback_fn((oal_void*)pst_dmac_user,DMAC_11V_CALLBACK_RETURN_WAIT_RSP_TIMEOUT, OAL_PTR_NULL);
    }
    return OAL_SUCC;
}

#endif//_PRE_WLAN_FEATURE_11V
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

