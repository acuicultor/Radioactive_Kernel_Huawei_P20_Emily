


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 ͷ�ļ�����
*****************************************************************************/
#include "oal_ext_if.h"
#include "oam_ext_if.h"
#include "frw_ext_if.h"
#ifdef _PRE_WLAN_FEATURE_SMP_SUPPORT
#include "frw_task.h"
#endif

#include "wlan_spec.h"

#include "hal_ext_if.h"

#include "mac_vap.h"
#include "mac_resource.h"
#include "dmac_resource.h"
#include "dmac_vap.h"
#include "dmac_dfx.h"
#include "dmac_reset.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_DFX_C

/*****************************************************************************
  2 ȫ�ֱ�������
*****************************************************************************/
dmac_dft_stru       g_st_dmac_dft_info;

/*****************************************************************************
  3 ����ʵ��
*****************************************************************************/


oal_uint32  dmac_dft_init(oal_void)
{
    oal_uint32  ul_core_id;
#ifdef _PRE_WLAN_FEATURE_SMP_SUPPORT
    /* ��һ������wifi�жϵĺ�*/
    for(ul_core_id = 0; ul_core_id < WLAN_FRW_MAX_NUM_CORES; ul_core_id++)
    {
        if(frw_task_get_state(ul_core_id))
        {
            break;
        }
    }
#else
    ul_core_id = OAL_GET_CORE_ID();
#endif
    /*lint -save -e774 */
    if(ul_core_id == WLAN_FRW_MAX_NUM_CORES)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "dmac_timestamp_init:dft timer init failed");
        return OAL_FAIL;
    }
    /*lint -restore */
    /* �����쳣ͳ���ϱ���ʱ�� */
    FRW_TIMER_CREATE_TIMER(&g_st_dmac_dft_info.st_excp_stat_info.st_timer,
                           oam_exception_stat_timeout,
                           OAM_EXCP_STATS_TIMEOUT,
                           OAL_PTR_NULL,
                           OAL_TRUE,
                           OAM_MODULE_ID_DMAC,
                           ul_core_id);
    return OAL_SUCC;
}


oal_uint32  dmac_dft_exit(oal_void)
{
    /* ɾ���쳣ͳ���ϱ���ʱ�� */
    FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&g_st_dmac_dft_info.st_excp_stat_info.st_timer);

    return OAL_SUCC;
}

/*
TBD��ע�ᵽ�¼��У����¼��������ͬ������
*/

oal_uint32 oam_exception_stat_timeout(oal_void *p_arg)
{
    oal_uint8                       uc_vap_idx;

    for (uc_vap_idx = 0; uc_vap_idx < WLAN_VAP_SUPPORT_MAX_NUM_LIMIT; uc_vap_idx++)
    {
        oam_exception_stat_handler(OM_WIFI, uc_vap_idx);
    }

    return OAL_SUCC;
}

oal_uint32 dmac_dfx_init(oal_void)
{
    return dmac_dft_init();
}

oal_uint32 dmac_dfx_exit(oal_void)
{
    return dmac_dft_exit();
}

#ifdef _PRE_WLAN_FEATURE_DFR

oal_uint32 dmac_dfr_init(hal_to_dmac_device_stru *pst_hal_device)
{
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    OAL_MEMZERO(&(pst_hal_device->st_pcie_err_timer), OAL_SIZEOF(frw_timeout_stru));

    /* ��ʼ�����pcie err_nonfatal ��ʱ�� */
    FRW_TIMER_CREATE_TIMER(&(pst_hal_device->st_pcie_err_timer),
                               dmac_pcie_err_timeout,
                               OAL_PCIE_NONFATAL_ERR_TIMEOUT_MS,
                               pst_hal_device,
                               OAL_TRUE,
                               OAM_MODULE_ID_DMAC,
                               pst_hal_device->ul_core_id);

    pst_hal_device->ul_pcie_err_cnt = 0;
#endif

    OAL_MEMZERO(&(pst_hal_device->st_dfr_tx_prot.st_tx_prot_timer), OAL_SIZEOF(frw_timeout_stru));

    /* ��ʼ������޷�������ж϶�ʱ�� */
    FRW_TIMER_CREATE_TIMER(&(pst_hal_device->st_dfr_tx_prot.st_tx_prot_timer),
                               dmac_dfr_tx_comp_timeout_handle,
                               WLAN_TX_PROT_TIMEOUT,
                               pst_hal_device,
                               OAL_FALSE,
                               OAM_MODULE_ID_DMAC,
                               pst_hal_device->ul_core_id);
    FRW_TIMER_STOP_TIMER(&(pst_hal_device->st_dfr_tx_prot.st_tx_prot_timer));

#ifdef _PRE_DEBUG_MODE
    pst_hal_device->ul_cfg_loss_tx_comp_cnt = 0;           /* ��ʼ��Ϊ����ʧ��������ж� */
#endif

    pst_hal_device->en_dfr_enable = OAL_TRUE;              /* Ĭ��ʹ��dfr���� */
    pst_hal_device->en_dfr_hw_reset_enable = OAL_FALSE;    /* Ĭ�ϲ�ʹ��dfr hw reset���� */

    return OAL_SUCC;
}

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)

oal_uint32  dmac_pcie_err_timeout(oal_void *p_arg)
{
    hal_to_dmac_device_stru    *pst_hal_device = (hal_to_dmac_device_stru *)p_arg;
    if (OAL_PTR_NULL == pst_hal_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{dmac_reg_timeout::pst_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (pst_hal_device->ul_pcie_err_cnt >= OAL_PCIE_NONFATAL_ERR_CNT)
    {
        oal_pci_hand_reset(pst_hal_device->uc_chip_id);

        OAM_WARNING_LOG1(0, OAM_SF_TX, "oal_pci_hand_reset complete,chip_id = %d\r\n", pst_hal_device->uc_chip_id);
        OAL_IO_PRINT("oal_pci_hand_reset complete,chip_id = %d\r\n", pst_hal_device->uc_chip_id);
    }

    pst_hal_device->ul_pcie_err_cnt = 0;
    return OAL_SUCC;
}

#endif


oal_uint32  dmac_dfr_tx_comp_timeout_handle(oal_void *p_arg)
{
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    mac_device_stru           *pst_mac_device;
    hal_to_dmac_device_stru   *pst_hal_device;
    hal_dfr_tx_prot_stru      *pst_dfr_tx_prot;
    dmac_reset_para_stru       st_reset_param;
    oal_bool_enum_uint8        en_rlst;

    pst_hal_device = (hal_to_dmac_device_stru *) p_arg;
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hal_device))
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_device = mac_res_get_dev(pst_hal_device->uc_mac_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    /*��λ״̬��ͣ����*/
    if (OAL_TRUE == MAC_DEV_IS_RESET_IN_PROGRESS(pst_mac_device))
    {
        return OAL_FAIL;
    }

    pst_dfr_tx_prot = &(pst_hal_device->st_dfr_tx_prot);

    /* ���û�д�dfr���أ���ֱ��stop��ʱ���󷵻� */
    if (OAL_FALSE == pst_hal_device->en_dfr_enable)
    {
        OAM_WARNING_LOG0(0, OAM_SF_IRQ, "dmac_dfr_tx_comp_timeout_handle: dfr is disabled\r\n");
        FRW_TIMER_STOP_TIMER(&(pst_hal_device->st_dfr_tx_prot.st_tx_prot_timer));
        return OAL_SUCC;
    }
#ifdef _PRE_DEBUG_MODE
    OAM_WARNING_LOG1(0, OAM_SF_IRQ, "dmac_dfr_tx_comp_timeout_handle enter, cnt = %d\r\n", pst_hal_device->ul_cfg_loss_tx_comp_cnt);
#endif

    /* ���������ɶ��л��д������¼���˵�����з�������жϲ�����ֱ�ӷ��� */
    en_rlst = frw_is_event_queue_empty(FRW_EVENT_TYPE_WLAN_TX_COMP);
    if (OAL_FALSE == en_rlst)
    {
        OAM_WARNING_LOG0(0, OAM_SF_IRQ, "dmac_dfr_tx_comp_timeout_handle: tx_comp_event is not empty\r\n");
        return OAL_SUCC;
    }

    st_reset_param.uc_reset_type    = HAL_RESET_HW_TYPE_ALL;
    st_reset_param.uc_reset_mac_mod = HAL_RESET_MAC_ALL;
    st_reset_param.uc_reset_mac_reg = OAL_FALSE;
    st_reset_param.uc_reset_phy_reg = OAL_FALSE;
    st_reset_param.uc_reset_rf_reg  = OAL_FALSE;
    st_reset_param.en_reason        = DMAC_RESET_REASON_TX_COMP_TIMEOUT;

    /* ��λ */
    dmac_reset_hw(pst_mac_device, pst_hal_device, (oal_uint8 *)&st_reset_param);
    OAM_WARNING_LOG0(0, OAM_SF_ANY, "{dmac_dfr_tx_comp_timeout_handle:: reset mac and phy done}");

    /* ֹͣdfr��ʱ�� */
    FRW_TIMER_STOP_TIMER(&(pst_hal_device->st_dfr_tx_prot.st_tx_prot_timer));

    /* ���³�ʼ�����Ͷ��� */
    dmac_tx_reinit_tx_queue(pst_mac_device, pst_hal_device);
    dmac_tx_complete_schedule(pst_hal_device, WLAN_WME_AC_BE);

#endif
    return OAL_SUCC;
}


OAL_STATIC oal_void  dmac_get_reset_plan(hal_mac_error_type_enum_uint8 en_error_id,
                                     dmac_reset_para_stru  *pst_reset_param,
                                     oal_bool_enum_uint8   *pen_reset_immediately)
{
    /* HAL_RESET_HW_TYPE_MAC_PHY : ����A : (MAC���Ը�λ)��λMAC+PHY�߼�
       HAL_RESET_HW_TYPE_MAC_PHY : ����B : (MAC���Ը�λ)�ﵽԤ����ֵ����λMAC+PHY�߼�
       HAL_RESET_HW_TYPE_MAC_PHY : ����C : ��λMAC+PHY���߼�+�Ĵ���
       HAL_RESET_HW_TYPE_PHY     : ����D : ��λPHY�߼�
       HAL_RESET_HW_TYPE_ALL     : ����E : ��λMAC+PHY+RF���߼�+�Ĵ���
    */

    OAL_MEMZERO((oal_uint8 *)pst_reset_param, OAL_SIZEOF(dmac_reset_para_stru));
    switch (en_error_id)
    {
        /* ����A */
        case HAL_MAC_ERROR_TX_ACBK_Q_OVERRUN:
        case HAL_MAC_ERROR_TX_ACBE_Q_OVERRUN:
        case HAL_MAC_ERROR_TX_ACVI_Q_OVERRUN:
        case HAL_MAC_ERROR_TX_ACVO_Q_OVERRUN:
        case HAL_MAC_ERROR_TX_HIPRI_Q_OVERRUN:
        case HAL_MAC_ERROR_BEACON_MISS:
            pst_reset_param->uc_reset_type    = HAL_RESET_HW_TYPE_MAC_PHY;
            pst_reset_param->uc_reset_mac_reg = OAL_FALSE;
            pst_reset_param->uc_reset_phy_reg = OAL_FALSE;
            pst_reset_param->uc_reset_mac_mod = HAL_RESET_MAC_ALL;
            pst_reset_param->en_reason = DMAC_RESET_REASON_HW_ERR;
            *pen_reset_immediately     = OAL_TRUE;
            break;

        /* ����B */
        case HAL_MAC_ERROR_PHY_RX_FIFO_OVERRUN:
        case HAL_MAC_ERROR_TX_DATAFLOW_BREAK:
        case HAL_MAC_ERROR_RX_FSM_ST_TIMEOUT:
        case HAL_MAC_ERROR_TX_FSM_ST_TIMEOUT:
        case HAL_MAC_ERROR_RX_HANDLER_ST_TIMEOUT:
        case HAL_MAC_ERROR_TX_HANDLER_ST_TIMEOUT:
        case HAL_MAC_ERROR_BUS_RLEN_ERR:
        case HAL_MAC_ERROR_BUS_RADDR_ERR:
        case HAL_MAC_ERROR_BUS_WLEN_ERR:
        case HAL_MAC_ERROR_BUS_WADDR_ERR:
        case HAL_MAC_ERROR_MATRIX_CALC_TIMEOUT:
        case HAL_MAC_ERROR_RX_OVERLAP_ERR:
            pst_reset_param->uc_reset_type    = HAL_RESET_HW_TYPE_MAC_PHY;
            pst_reset_param->uc_reset_mac_reg = OAL_FALSE;
            pst_reset_param->uc_reset_phy_reg = OAL_FALSE;
            pst_reset_param->uc_reset_mac_mod = HAL_RESET_MAC_ALL;
            pst_reset_param->en_reason = DMAC_RESET_REASON_HW_ERR;
            *pen_reset_immediately     = OAL_FALSE;
            break;

        /* ����D */
        case HAL_MAC_ERROR_PHY_TRLR_TIME_OUT:
            pst_reset_param->uc_reset_type    = HAL_RESET_HW_TYPE_PHY;
            pst_reset_param->uc_reset_mac_reg = OAL_FALSE;
            pst_reset_param->uc_reset_phy_reg = OAL_FALSE;
            pst_reset_param->uc_reset_mac_mod = HAL_RESET_MAC_ALL;
            pst_reset_param->en_reason = DMAC_RESET_REASON_HW_ERR;
            *pen_reset_immediately     = OAL_FALSE;
            break;

        default:
            pst_reset_param->uc_reset_type    = HAL_RESET_HW_TYPE_BUTT;
            pst_reset_param->uc_reset_mac_reg = OAL_FALSE;
            pst_reset_param->uc_reset_phy_reg = OAL_FALSE;
            pst_reset_param->uc_reset_mac_mod = HAL_RESET_MAC_BUTT;
            pst_reset_param->en_reason = DMAC_RESET_REASON_HW_ERR;
            *pen_reset_immediately     = OAL_FALSE;
            break;
    }

}


oal_uint32  dmac_dfr_process_mac_error(
                mac_device_stru                 *pst_mac_device,
                hal_to_dmac_device_stru         *pst_hal_device,
                hal_mac_error_type_enum_uint8    en_error_id,
                oal_uint32     ul_error1_irq_state,
                oal_uint32     ul_error2_irq_state)
{
    hal_dfr_err_opern_stru    *pst_dft_err;
    dmac_reset_para_stru       st_reset_param;
    oal_bool_enum_uint8        en_reset_immediately;
#ifdef _PRE_WLAN_DFT_STAT
    dmac_vap_stru             *pst_dmac_sta;
#endif

    if ((OAL_PTR_NULL == pst_mac_device)||(OAL_PTR_NULL == pst_hal_device))
    {
        OAM_ERROR_LOG2(0, OAM_SF_DFR, "{dmac_dfr_process_mac_error::PTR NULL mac device[%p],hal device[%p]!.}", pst_mac_device, pst_hal_device);
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (OAL_UNLIKELY(en_error_id >= HAL_MAC_ERROR_TYPE_BUTT))
    {
        OAM_ERROR_LOG1(0, OAM_SF_DFR, "{dmac_dfr_process_mac_error::error_id[%d] OVERFLOW!.}", en_error_id);
        return OAL_ERR_CODE_ARRAY_OVERFLOW;
    }

    if (OAL_FALSE == pst_hal_device->en_dfr_hw_reset_enable)
    {
        return OAL_SUCC;
    }

    pst_dft_err = &(pst_hal_device->st_dfr_err_opern[en_error_id]);

    /* �����жϼ�����1 */
    pst_dft_err->us_err_cnt++;

    /* ��ȡ��λ���� */
    dmac_get_reset_plan(en_error_id, &st_reset_param, &en_reset_immediately);
    if (HAL_RESET_HW_TYPE_BUTT <= st_reset_param.uc_reset_type)
    {
        return OAL_SUCC;
    }

    /* ��������10��TBTT�ж���(ÿ10��TBTT�ж϶Ըü�������)��
       �����жϲ����Ĵ���û�дﵽ��Ҫ�������ֵ��ֱ�ӷ��� */
    if ((pst_dft_err->us_err_cnt < HAL_MAC_ERROR_THRESHOLD) && (OAL_TRUE != en_reset_immediately))
    {
        return OAL_SUCC;
    }

#ifdef _PRE_WLAN_DFT_STAT
    pst_dmac_sta = mac_res_get_dmac_vap(pst_mac_device->uc_cfg_vap_id);
    if (OAL_PTR_NULL == pst_dmac_sta)
    {
        OAM_ERROR_LOG1(0, OAM_SF_DFR, "{dmac_dfr_process_mac_error::Cannot find vap[%d] by dev!}", pst_mac_device->uc_cfg_vap_id);
    }
    else
    {
        /* MAC&PHY&RF�Ĵ����ϱ� */
        dmac_dft_report_all_ota_state(&pst_dmac_sta->st_vap_base_info);
    }
#endif

    /* ��mac�ж�״̬ */
    hal_device_process_mac_error_status(pst_hal_device, ul_error1_irq_state, ul_error2_irq_state);

    OAM_WARNING_LOG2(0, OAM_SF_DFR, "{dmac_dfr_process_mac_error::There are too many errors[id:%d, cnt:%d], so we have to reset the HW!.}",
                    en_error_id, pst_dft_err->us_err_cnt);

    /* ��λ����������쳣ͳ�ƣ����¿�ʼ���� */
    pst_dft_err->us_err_cnt = 0;

    /* ��ʼ��λ */
    dmac_reset_hw(pst_mac_device, pst_hal_device, (oal_uint8 *)&st_reset_param);

    return OAL_SUCC;
}
#else

oal_uint32  dmac_dfr_process_mac_error(
                mac_device_stru                 *pst_mac_device,
                hal_to_dmac_device_stru         *pst_hal_device,
                hal_mac_error_type_enum_uint8    en_error_id,
                oal_uint32     ul_error1_irq_state,
                oal_uint32     ul_error2_irq_state)
{
    return OAL_SUCC;
}

#endif //_PRE_WLAN_FEATURE_DFR

/*lint +e19*/

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

