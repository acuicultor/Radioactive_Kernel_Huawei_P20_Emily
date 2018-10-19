


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_QOS_ENHANCE

/*****************************************************************************
  1 ͷ�ļ�����
*****************************************************************************/
#include "oal_ext_if.h"
#include "dmac_ext_if.h"
#include "mac_vap.h"
#include "dmac_user.h"
#include "dmac_main.h"
#include "oal_net.h"
#include "dmac_tx_qos_enhance.h"
#include "dmac_rx_data.h"
#include "dmac_stat.h"



#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_TX_QOS_ENHANCE_C

/*****************************************************************************
  2 ����ԭ������
*****************************************************************************/

/*****************************************************************************
  3 ȫ�ֱ�������
*****************************************************************************/

/*****************************************************************************
  4 ����ʵ��
*****************************************************************************/

oal_uint32 dmac_tx_add_qos_enhance_list(mac_vap_stru *pst_mac_vap ,oal_uint8 *puc_sta_member_addr)
{
    mac_qos_enhance_sta_stru         *pst_qos_enhance_sta = OAL_PTR_NULL;
    mac_qos_enhance_stru             *pst_qos_enhance      = (mac_qos_enhance_stru *)(&pst_mac_vap->st_qos_enhance);

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_qos_enhance))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_QOS, "{dmac_tx_add_qos_enhance_list::pst_qos_enhance null.}");
        return OAL_FAIL;
    }

    /* ���������ǰ���� */
    oal_spin_lock(&(pst_qos_enhance->st_lock));

    /* �ж�STA�Ƿ���qos_enhance������ */
    pst_qos_enhance_sta = mac_tx_find_qos_enhance_list(pst_mac_vap, puc_sta_member_addr);

    /* STA����qos_enhance������ */
    if (OAL_PTR_NULL == pst_qos_enhance_sta)
    {
        if (pst_qos_enhance->uc_qos_enhance_sta_count >= WLAN_ASSOC_USER_MAX_NUM)
        {
            /* ���� */
            oal_spin_unlock(&(pst_qos_enhance->st_lock));
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_QOS, "{dmac_tx_add_qos_enhance_list::pst_qos_enhance->uc_qos_enhance_sta_count is [%d].}",pst_qos_enhance->uc_qos_enhance_sta_count);
            return OAL_FAIL;
        }
        /* ����qos_enhance_sta�ڵ� */
        pst_qos_enhance_sta = OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAL_SIZEOF(mac_qos_enhance_sta_stru), OAL_TRUE);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pst_qos_enhance_sta))
        {
            /* ���� */
            oal_spin_unlock(&(pst_qos_enhance->st_lock));
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_QOS, "{dmac_tx_add_qos_enhance_list::pst_qos_enhance_hash null.}");
            return OAL_FAIL;
        }

        oal_memset(pst_qos_enhance_sta, 0x0, OAL_SIZEOF(mac_qos_enhance_sta_stru));
        oal_set_mac_addr(pst_qos_enhance_sta->auc_qos_enhance_mac, puc_sta_member_addr);
        oal_dlist_add_tail(&(pst_qos_enhance_sta->st_qos_enhance_entry), &(pst_qos_enhance->st_list_head));
        pst_qos_enhance->uc_qos_enhance_sta_count++;

    }
    else /* STA��qos_enhance������ */
    {
        /* ����С��qos_enhance�����������ֵ */
        if (pst_qos_enhance_sta->uc_add_num < MAC_QOS_ENHANCE_ADD_NUM)
        {
            pst_qos_enhance_sta->uc_add_num++;
            pst_qos_enhance_sta->uc_delete_num = 0;
        }
    }
    /* ���� */
    oal_spin_unlock(&(pst_qos_enhance->st_lock));

    return OAL_SUCC;
}


oal_void dmac_tx_delete_qos_enhance_sta(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_sta_member_addr, oal_uint8 uc_flag)
{
    mac_qos_enhance_stru             *pst_qos_enhance = (mac_qos_enhance_stru *)(&pst_mac_vap->st_qos_enhance);
    mac_qos_enhance_sta_stru         *pst_qos_enhance_sta = OAL_PTR_NULL;
    oal_dlist_head_stru              *pst_sta_list_entry;
    oal_dlist_head_stru              *pst_sta_list_entry_temp;

    if (OAL_PTR_NULL == pst_qos_enhance)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_QOS, "{dmac_tx_delete_qos_enhance_sta::pst_qos_enhance null.}");
        return;
    }

    if (0 == pst_qos_enhance->uc_qos_enhance_sta_count)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_QOS, "{dmac_tx_delete_qos_enhance_sta::no sta in qos list while delete.}");
        return ;
    }

    /* ���������ǰ���� */
    oal_spin_lock(&(pst_qos_enhance->st_lock));

    /* ����qos_enhance���ҵ���ַƥ���STA */
    OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_sta_list_entry, pst_sta_list_entry_temp, &(pst_qos_enhance->st_list_head))
    {
        pst_qos_enhance_sta = OAL_DLIST_GET_ENTRY(pst_sta_list_entry, mac_qos_enhance_sta_stru, st_qos_enhance_entry);

        if (!oal_compare_mac_addr(puc_sta_member_addr, pst_qos_enhance_sta->auc_qos_enhance_mac))
        {
            if (uc_flag)
            {
                /* ����>=qos_enhance����ɾ������ֵ����STA�ӱ���ɾ�� */
                if (pst_qos_enhance_sta->uc_delete_num >= MAC_QOS_ENHANCE_QUIT_NUM)
                {
                    oal_dlist_delete_entry(&(pst_qos_enhance_sta->st_qos_enhance_entry));
                    OAL_MEM_FREE(pst_qos_enhance_sta, OAL_TRUE);
                    pst_qos_enhance->uc_qos_enhance_sta_count--;
                }
                else /* ����С��qos_enhance����ɾ������ֵ�������ۼ� */
                {
                    pst_qos_enhance_sta->uc_add_num = 0;
                    pst_qos_enhance_sta->uc_delete_num++;
                }
            }
            else /* ɾ��STA�����qos list */
            {
                oal_dlist_delete_entry(&(pst_qos_enhance_sta->st_qos_enhance_entry));
                OAL_MEM_FREE(pst_qos_enhance_sta, OAL_TRUE);
                pst_qos_enhance->uc_qos_enhance_sta_count--;
            }
        }
    }

    /* �ͷ��� */
    oal_spin_unlock(&(pst_qos_enhance->st_lock));
}


oal_uint32 dmac_tx_qos_enhance_add_or_delete(oal_int8 c_rx_rssi, oal_uint32 ul_tx_rate)
{
    oal_uint32                        ul_mid_interval = 0;

    ul_mid_interval = ((DMAC_QOS_ENHANCE_TP_HIGH_THD - DMAC_QOS_ENHANCE_TP_MID_THD)/(DMAC_QOS_ENHANCE_RSSI_HIGH_THD - DMAC_QOS_ENHANCE_RSSI_LOW_THD));

    /* STA��rssiǿ��Ϊǿ�ȼ� */
    if (c_rx_rssi > DMAC_QOS_ENHANCE_RSSI_HIGH_THD)
    {
        /* ������ */
        if (ul_tx_rate < DMAC_QOS_ENHANCE_TP_HIGH_THD)
        {
            return OAL_TRUE;
        }
        /* ������ */
        else
        {
            return OAL_FALSE;
        }
    }
    /* STA��rssiǿ��Ϊ���ȼ� */
    else if (c_rx_rssi < DMAC_QOS_ENHANCE_RSSI_LOW_THD)
    {
        //STA��rssiǿ�ȴ��ڱ߽�rssi
        if(c_rx_rssi > DMAC_QOS_ENHANCE_RSSI_EDGE_THD)
        {
            /* ������ */
            if (ul_tx_rate < DMAC_QOS_ENHANCE_TP_LOW_THD)
            {
                return OAL_TRUE;
            }
            /* ������ */
            else
            {
                return OAL_FALSE;
            }
        }
        /* ǿ��С�ڱ߽�rssi */
        else
        {
            return OAL_FALSE;
        }
    }
    /* STA��rssiǿ��Ϊ�еȼ� */
    else
    {
        /* ������ */
        if (ul_tx_rate < ul_mid_interval*(c_rx_rssi - DMAC_QOS_ENHANCE_RSSI_LOW_THD) + DMAC_QOS_ENHANCE_TP_MID_THD)
        {
            return OAL_TRUE;
        }
        /* ������ */
        else
        {
            return OAL_FALSE;
        }
    }


}


oal_uint32 dmac_tx_qos_enhance_proc(mac_vap_stru *pst_mac_vap)
{
    mac_user_stru                    *pst_mac_user = OAL_PTR_NULL;
    dmac_user_stru                   *pst_dmac_user = OAL_PTR_NULL;
    oal_dlist_head_stru              *pst_entry;
    oal_uint32                        ul_tx_rate;
    oal_uint32                        ul_ret = OAL_SUCC;

    /* ����ж� */
    if ((OAL_PTR_NULL == pst_mac_vap))
    {
        OAM_ERROR_LOG1(0, OAM_SF_QOS, "{dmac_tx_qos_enhance_proc::param null, pst_mac_vap=%p}", pst_mac_vap);
        return OAL_FAIL;
    }

    /* vap�¹�STA����<2,������qos enhance�ж� */
    if (pst_mac_vap->us_user_nums < 2)
    {
        OAM_WARNING_LOG1(0, OAM_SF_QOS, "{dmac_tx_qos_enhance_proc:associate user num is %d}",pst_mac_vap->us_user_nums);
        return OAL_FAIL;
    }


    /* ����vap�¹�STA�б� */
    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_mac_vap->st_mac_user_list_head))
    {
        pst_mac_user = OAL_DLIST_GET_ENTRY(pst_entry, mac_user_stru, st_user_dlist);

        /*lint -save -e774 */
        if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_user))
        {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_QOS, "{dmac_tx_qos_enhance_proc:pst_user null pointer.}");
            continue;
        }
        /*lint -restore */

        pst_dmac_user = MAC_GET_DMAC_USER(pst_mac_user);

        /* ��ȡAP��STA���TX������ */
        ul_tx_rate = dmac_stat_tx_sta_thrpt(pst_mac_user);

        if (dmac_tx_qos_enhance_add_or_delete(oal_get_real_rssi(pst_dmac_user->s_rx_rssi), ul_tx_rate))
        {
            /* ����qos enhance���� */
            ul_ret = dmac_tx_add_qos_enhance_list(pst_mac_vap, pst_mac_user->auc_user_mac_addr);
            if (OAL_UNLIKELY(OAL_FAIL == ul_ret))
            {
                OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_QOS, "{dmac_tx_qos_enhance_proc::add_qos_enhance_list fail.}");
                return OAL_FAIL;
            }
        }
        else
        {
            /* ��qos enhance������ɾ�� */
            dmac_tx_delete_qos_enhance_sta(pst_mac_vap, pst_mac_user->auc_user_mac_addr, OAL_TRUE);
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_QOS, "{dmac_tx_qos_enhance_proc::delete qos enhance sta.}");
        }

        if (oal_get_real_rssi(pst_dmac_user->s_rx_rssi) > DMAC_QOS_ENHANCE_RSSI_HIGH_THD)
        {
            pst_mac_user->en_qos_enhance_sta_state = MAC_USER_QOS_ENHANCE_NEAR;
        }
        else if (oal_get_real_rssi(pst_dmac_user->s_rx_rssi) < DMAC_QOS_ENHANCE_RSSI_LOW_THD)
        {
            pst_mac_user->en_qos_enhance_sta_state = MAC_USER_QOS_ENHANCE_FAR;
        }
    }
    return OAL_SUCC;

}


oal_uint32 dmac_tx_qos_enhance_time_fn(oal_void *p_arg)
{
    mac_vap_stru      *pst_mac_vap     = (mac_vap_stru *)p_arg;
    oal_uint32         ul_ret           =  OAL_SUCC;

    ul_ret = dmac_tx_qos_enhance_proc(pst_mac_vap);
    return ul_ret;
}


oal_void dmac_tx_qos_enhance_attach(dmac_vap_stru *pst_dmac_vap)
{
    /* ������ʱ�� */
    FRW_TIMER_CREATE_TIMER(&(pst_dmac_vap->st_qos_enhance_timer),
                           dmac_tx_qos_enhance_time_fn,
                           DMAC_DEF_QOS_ENHANCE_TIMER,
                           (oal_void *)(&pst_dmac_vap->st_vap_base_info),
                           OAL_TRUE,
                           OAM_MODULE_ID_DMAC,
                           pst_dmac_vap->st_vap_base_info.ul_core_id);
}


oal_void dmac_tx_qos_enhance_detach(dmac_vap_stru *pst_dmac_vap)
{
    /* �رն�ʱ�� */
    FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_dmac_vap->st_qos_enhance_timer));
}

/*lint -e578*//*lint -e19*/
oal_module_symbol(dmac_tx_qos_enhance_attach);
oal_module_symbol(dmac_tx_qos_enhance_detach);
oal_module_symbol(dmac_tx_delete_qos_enhance_sta);
/*lint +e578*//*lint +e19*/
#endif /* _PRE_WLAN_FEATURE_QOS_ENHANCE */


#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

