


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_M2S
/*****************************************************************************
  1 ͷ�ļ�����
*****************************************************************************/
#include "hal_ext_if.h"
#include "hal_device.h"
#include "hal_device_fsm.h"
#include "dmac_alg.h"
#include "dmac_vap.h"
#include "dmac_scan.h"
#include "dmac_tx_complete.h"
#include "dmac_tx_bss_comm.h"
#include "dmac_smps.h"
#include "dmac_opmode.h"
#include "dmac_config.h"
#include "dmac_fcs.h"
#include "dmac_m2s.h"
#include "dmac_txopps.h"
#include "dmac_power.h"
#ifdef _PRE_WLAN_FEATURE_BTCOEX
#include "dmac_btcoex.h"
#endif
#ifdef _PRE_WLAN_FEATURE_STA_PM
#include "hal_pm.h"
#include "dmac_psm_sta.h"
#endif
#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_M2S_C

/*****************************************************************************
  1 ��������
*****************************************************************************/
OAL_STATIC oal_void dmac_m2s_state_idle_entry(oal_void *p_ctx);
OAL_STATIC oal_void dmac_m2s_state_idle_exit(oal_void *p_ctx);
OAL_STATIC oal_uint32 dmac_m2s_state_idle_event(oal_void  *p_ctx, oal_uint16  us_event,
                                    oal_uint16 us_event_data_len,  oal_void  *p_event_data);
OAL_STATIC oal_void dmac_m2s_state_mimo_entry(oal_void *p_ctx);
OAL_STATIC oal_void dmac_m2s_state_mimo_exit(oal_void *p_ctx);
OAL_STATIC oal_uint32 dmac_m2s_state_mimo_event(oal_void  *p_ctx, oal_uint16  us_event,
                                    oal_uint16 us_event_data_len,  oal_void  *p_event_data);
OAL_STATIC oal_void dmac_m2s_state_miso_entry(oal_void *p_ctx);
OAL_STATIC oal_void dmac_m2s_state_miso_exit(oal_void *p_ctx);
OAL_STATIC oal_uint32 dmac_m2s_state_miso_event(oal_void  *p_ctx, oal_uint16  us_event,
                                    oal_uint16 us_event_data_len,  oal_void  *p_event_data);
OAL_STATIC oal_void dmac_m2s_state_siso_entry(oal_void *p_ctx);
OAL_STATIC oal_void dmac_m2s_state_siso_exit(oal_void *p_ctx);
OAL_STATIC oal_uint32 dmac_m2s_state_siso_event(oal_void  *p_ctx, oal_uint16  us_event,
                                    oal_uint16 us_event_data_len,  oal_void  *p_event_data);
OAL_STATIC oal_void dmac_m2s_state_simo_entry(oal_void *p_ctx);
OAL_STATIC oal_void dmac_m2s_state_simo_exit(oal_void *p_ctx);
OAL_STATIC oal_uint32 dmac_m2s_state_simo_event(oal_void  *p_ctx, oal_uint16  us_event,
                                    oal_uint16 us_event_data_len,  oal_void  *p_event_data);
OAL_STATIC oal_uint32 dmac_m2s_fsm_trans_to_state(hal_m2s_fsm_stru *pst_m2s_fsm, oal_uint8 uc_state);

/*****************************************************************************
  2 ȫ�ֱ�������
*****************************************************************************/
/* ��״̬��ڳ��ں��� */
OAL_STATIC oal_fsm_state_info  g_ast_hal_m2s_fsm_info[] = {
    {
        HAL_M2S_STATE_IDLE,
        "IDLE",
        dmac_m2s_state_idle_entry,
        dmac_m2s_state_idle_exit,
        dmac_m2s_state_idle_event,
    },
    {
        HAL_M2S_STATE_SISO,
        "SISO",
        dmac_m2s_state_siso_entry,
        dmac_m2s_state_siso_exit,
        dmac_m2s_state_siso_event,
    },
    {
        HAL_M2S_STATE_MIMO,
        "MIMO",
        dmac_m2s_state_mimo_entry,
        dmac_m2s_state_mimo_exit,
        dmac_m2s_state_mimo_event,
    },
    {
        HAL_M2S_STATE_MISO,
        "MISO",
        dmac_m2s_state_miso_entry,
        dmac_m2s_state_miso_exit,
        dmac_m2s_state_miso_event,
    },
    {
        HAL_M2S_STATE_SIMO,
        "SIMO",
        dmac_m2s_state_simo_entry,
        dmac_m2s_state_simo_exit,
        dmac_m2s_state_simo_event,
    },
};

/*****************************************************************************
  3 ��������
*****************************************************************************/

oal_void  dmac_m2s_mgr_param_info(hal_to_dmac_device_stru *pst_hal_device)
{
    OAM_WARNING_LOG_ALTER(0, OAM_SF_M2S,
        "{dmac_m2s_switch_mgr_param_info::m2s_type[%d],cur state[%d],mode_stru[%d],miso_hold[%d].}",
        4, GET_HAL_M2S_SWITCH_TPYE(pst_hal_device), GET_HAL_M2S_CUR_STATE(pst_hal_device),
        GET_HAL_M2S_MODE_TPYE(pst_hal_device), GET_HAL_DEVICE_M2S_DEL_SWI_MISO_HOLD(pst_hal_device));

    OAM_WARNING_LOG_ALTER(0, OAM_SF_M2S,
        "{dmac_m2s_switch_mgr_param_info::nss_num[%d]uc_phy_chain[%d]single_txrx_chain[%d]uc_rf_chain[%d]uc_phy2dscr_chain[%d].}",
        5, pst_hal_device->st_cfg_cap_info.en_nss_num,
        pst_hal_device->st_cfg_cap_info.uc_phy_chain, pst_hal_device->st_cfg_cap_info.uc_single_tx_chain,
        pst_hal_device->st_cfg_cap_info.uc_rf_chain, pst_hal_device->st_cfg_cap_info.uc_phy2dscr_chain);
}


hal_m2s_state_uint8  dmac_m2s_event_state_classify(hal_m2s_event_tpye_uint16 en_m2s_event)
{
    hal_m2s_state_uint8    uc_new_m2s_state = HAL_M2S_STATE_BUTT;

    /* ��������ҵ�����󣬽��ж�Ӧcase��� TBD */
    switch(en_m2s_event)
    {
        /* ����ɨ���л���SIMO,�������mimo,Ӳ����siso, ҵ�񳡾��²����棬siso����c0 */
        case HAL_M2S_EVENT_SCAN_BEGIN:
            uc_new_m2s_state = HAL_M2S_STATE_SIMO;
            break;

        /* ����ɨ������л���MIMO,�����Ӳ������mimo */
        case HAL_M2S_EVENT_SCAN_CHANNEL_BACK:
        case HAL_M2S_EVENT_SCAN_END:
        case HAL_M2S_EVENT_BT_SISO_TO_MIMO:
        case HAL_M2S_EVENT_TEST_SISO_C0_TO_MIMO:
        case HAL_M2S_EVENT_TEST_SISO_C1_TO_MIMO:
        case HAL_M2S_EVENT_DBDC_STOP:
        case HAL_M2S_EVENT_DBDC_SISO_TO_MIMO:
        case HAL_M2S_EVENT_COMMAND_MISO_TO_MIMO:
        case HAL_M2S_EVENT_COMMAND_SISO_TO_MIMO:
            uc_new_m2s_state = HAL_M2S_STATE_MIMO;
            break;

        /* RSSI�л���MISO,����л���SISO,Ӳ������mimo */
        case HAL_M2S_EVENT_ANT_RSSI_MIMO_TO_MISO:
        case HAL_M2S_EVENT_ANT_RSSI_SISO_TO_MISO:
        case HAL_M2S_EVENT_COMMAND_MIMO_TO_MISO_C0:
        case HAL_M2S_EVENT_COMMAND_MIMO_TO_MISO_C1:
        case HAL_M2S_EVENT_COMMAND_MISO_C0_TO_MISO_C1:
        case HAL_M2S_EVENT_COMMAND_MISO_C1_TO_MISO_C0:
            uc_new_m2s_state = HAL_M2S_STATE_MISO;
            break;

        case HAL_M2S_EVENT_ANT_RSSI_MISO_TO_C0_SISO:
        case HAL_M2S_EVENT_TEST_MIMO_TO_SISO_C0:
        case HAL_M2S_EVENT_TEST_SISO_C1_TO_SISO_C0:
        case HAL_M2S_EVENT_DBDC_MIMO_TO_SISO:
        case HAL_M2S_EVENT_DBDC_START:
        case HAL_M2S_EVENT_BT_MIMO_TO_SISO:
        case HAL_M2S_EVENT_ANT_RSSI_MISO_TO_C1_SISO:
        case HAL_M2S_EVENT_TEST_MIMO_TO_SISO_C1:
        case HAL_M2S_EVENT_TEST_SISO_C0_TO_SISO_C1:
        case HAL_M2S_EVENT_COMMAND_MISO_TO_SISO_C0:
        case HAL_M2S_EVENT_COMMAND_MISO_TO_SISO_C1:
        case HAL_M2S_EVENT_COMMAND_MIMO_TO_SISO_C0:
        case HAL_M2S_EVENT_COMMAND_MIMO_TO_SISO_C1:
        case HAL_M2S_EVENT_COMMAND_SISO_C0_TO_SISO_C1:
        case HAL_M2S_EVENT_COMMAND_SISO_C1_TO_SISO_C0:
            uc_new_m2s_state = HAL_M2S_STATE_SISO;
            break;

        default:
           OAM_ERROR_LOG1(0, OAM_SF_M2S, "{dmac_m2s_event_state_classify: en_m2s_event[%d] error!}", en_m2s_event);
    }

    return uc_new_m2s_state;
}


hal_m2s_state_uint8  dmac_m2s_chain_state_classify(hal_to_dmac_device_stru *pst_hal_device)
{
    /* ���ж����״̬ */
    if (WLAN_PHY_CHAIN_DOUBLE == pst_hal_device->st_cfg_cap_info.uc_phy2dscr_chain)
    {
        if(WLAN_PHY_CHAIN_DOUBLE == pst_hal_device->st_cfg_cap_info.uc_phy_chain)
        {
            return HAL_M2S_STATE_MIMO;
        }
        else
        {
            return HAL_M2S_STATE_SIMO;
        }
    }
    else /* �������siso */
    {
        if(WLAN_PHY_CHAIN_DOUBLE == pst_hal_device->st_cfg_cap_info.uc_phy_chain)
        {
            return HAL_M2S_STATE_MISO;
        }
        else
        {
            return HAL_M2S_STATE_SISO;
        }
    }
}


wlan_m2s_trigger_mode_enum_uint8  dmac_m2s_event_trigger_mode_classify(hal_m2s_event_tpye_uint16 en_m2s_event)
{
    wlan_m2s_trigger_mode_enum_uint8 uc_trigger_mode = WLAN_M2S_TRIGGER_MODE_BUTT;

    switch(en_m2s_event)
    {
        case HAL_M2S_EVENT_SCAN_BEGIN:
        case HAL_M2S_EVENT_SCAN_CHANNEL_BACK:
        case HAL_M2S_EVENT_SCAN_END:
            /* ����ɨ�裬��Ҫ�����жϿ���ɨ�迪��ʱ���ñ�ǣ���Ϊ������SCAN_BEGIN��� */
            //uc_trigger_mode = WLAN_M2S_TRIGGER_MODE_FAST_SCAN;
            break;

        case HAL_M2S_EVENT_DBDC_START:
        case HAL_M2S_EVENT_DBDC_STOP:
        case HAL_M2S_EVENT_DBDC_SISO_TO_MIMO:
        case HAL_M2S_EVENT_DBDC_MIMO_TO_SISO:
            uc_trigger_mode = WLAN_M2S_TRIGGER_MODE_DBDC;
            break;

        case HAL_M2S_EVENT_ANT_RSSI_MISO_TO_C1_SISO:
        case HAL_M2S_EVENT_ANT_RSSI_MISO_TO_MIMO:
        case HAL_M2S_EVENT_ANT_RSSI_MISO_TO_C0_SISO:
        case HAL_M2S_EVENT_ANT_RSSI_MIMO_TO_MISO:
        case HAL_M2S_EVENT_ANT_RSSI_SISO_TO_MISO:
            uc_trigger_mode = WLAN_M2S_TRIGGER_MODE_RSSI_SNR;
            break;

        case HAL_M2S_EVENT_BT_SISO_TO_MIMO:
        case HAL_M2S_EVENT_BT_MIMO_TO_SISO:
            uc_trigger_mode = WLAN_M2S_TRIGGER_MODE_BTCOEX;
            break;

        case HAL_M2S_EVENT_COMMAND_MIMO_TO_MISO_C0:
        case HAL_M2S_EVENT_COMMAND_MIMO_TO_MISO_C1:
        case HAL_M2S_EVENT_COMMAND_MISO_TO_MIMO:
        case HAL_M2S_EVENT_COMMAND_MISO_TO_SISO_C0:
        case HAL_M2S_EVENT_COMMAND_MISO_TO_SISO_C1:
        case HAL_M2S_EVENT_COMMAND_SISO_TO_MIMO:
        case HAL_M2S_EVENT_COMMAND_MIMO_TO_SISO_C0:
        case HAL_M2S_EVENT_COMMAND_MIMO_TO_SISO_C1:
            uc_trigger_mode = WLAN_M2S_TRIGGER_MODE_COMMAND;
            break;

        case HAL_M2S_EVENT_TEST_SISO_C0_TO_MIMO:
        case HAL_M2S_EVENT_TEST_SISO_C1_TO_MIMO:
        case HAL_M2S_EVENT_TEST_MIMO_TO_SISO_C0:
        case HAL_M2S_EVENT_TEST_SISO_C1_TO_SISO_C0:
        case HAL_M2S_EVENT_TEST_MIMO_TO_SISO_C1:
        case HAL_M2S_EVENT_TEST_SISO_C0_TO_SISO_C1:
            uc_trigger_mode = WLAN_M2S_TRIGGER_MODE_TEST;
            break;

        default:
           OAM_WARNING_LOG1(0, OAM_SF_M2S, "{dmac_m2s_event_trigger_mode_classify: en_m2s_event[%d] other service!}", en_m2s_event);
    }

    return uc_trigger_mode;
}


oal_void  dmac_m2s_update_switch_mgr_param(hal_to_dmac_device_stru *pst_hal_device, hal_m2s_event_tpye_uint16 en_m2s_event)
{
    /* ��������ҵ�����󣬽��ж�Ӧcase��� TBD */
    switch(en_m2s_event)
    {
        /* ����ɨ���л���SIMO,�������mimo,Ӳ����siso, ҵ�񳡾��²����棬siso����c0 */
        case HAL_M2S_EVENT_SCAN_BEGIN:
            pst_hal_device->st_cfg_cap_info.en_nss_num         = WLAN_DOUBLE_NSS;
            pst_hal_device->st_cfg_cap_info.uc_phy2dscr_chain  = WLAN_PHY_CHAIN_DOUBLE;
            pst_hal_device->st_cfg_cap_info.uc_single_tx_chain = WLAN_PHY_CHAIN_ZERO;
            pst_hal_device->st_cfg_cap_info.uc_rf_chain        = WLAN_RF_CHAIN_ZERO;
            pst_hal_device->st_cfg_cap_info.uc_phy_chain       = WLAN_PHY_CHAIN_ZERO;
            break;

        /* ����ɨ������л���MIMO,�����Ӳ������mimo */
        case HAL_M2S_EVENT_SCAN_CHANNEL_BACK:
        case HAL_M2S_EVENT_SCAN_END:
        case HAL_M2S_EVENT_TEST_SISO_C1_TO_MIMO:
        case HAL_M2S_EVENT_TEST_SISO_C0_TO_MIMO:
        case HAL_M2S_EVENT_COMMAND_MISO_TO_MIMO:
        case HAL_M2S_EVENT_COMMAND_SISO_TO_MIMO:
            pst_hal_device->st_cfg_cap_info.en_nss_num         = WLAN_DOUBLE_NSS;
            pst_hal_device->st_cfg_cap_info.uc_phy2dscr_chain  = WLAN_PHY_CHAIN_DOUBLE;
            pst_hal_device->st_cfg_cap_info.uc_single_tx_chain = WLAN_PHY_CHAIN_ZERO;
            pst_hal_device->st_cfg_cap_info.uc_rf_chain        = WLAN_RF_CHAIN_DOUBLE;
            pst_hal_device->st_cfg_cap_info.uc_phy_chain       = WLAN_PHY_CHAIN_DOUBLE;
            break;

        /* C0 MISO ����RSSI�л���MISO,����л���SISO,Ӳ������mimo */
        case HAL_M2S_EVENT_ANT_RSSI_MIMO_TO_MISO:
        case HAL_M2S_EVENT_ANT_RSSI_SISO_TO_MISO:
        case HAL_M2S_EVENT_COMMAND_MIMO_TO_MISO_C0:
        case HAL_M2S_EVENT_COMMAND_MISO_C1_TO_MISO_C0:
            pst_hal_device->st_cfg_cap_info.en_nss_num         = WLAN_SINGLE_NSS;
            pst_hal_device->st_cfg_cap_info.uc_phy2dscr_chain  = WLAN_PHY_CHAIN_ZERO;
            pst_hal_device->st_cfg_cap_info.uc_single_tx_chain = WLAN_PHY_CHAIN_ZERO;
            pst_hal_device->st_cfg_cap_info.uc_rf_chain        = WLAN_RF_CHAIN_DOUBLE;
            pst_hal_device->st_cfg_cap_info.uc_phy_chain       = WLAN_PHY_CHAIN_DOUBLE;
            break;

        /* C1 MISO */
        case HAL_M2S_EVENT_COMMAND_MIMO_TO_MISO_C1:
        case HAL_M2S_EVENT_COMMAND_MISO_C0_TO_MISO_C1:
            pst_hal_device->st_cfg_cap_info.en_nss_num         = WLAN_SINGLE_NSS;
            pst_hal_device->st_cfg_cap_info.uc_phy2dscr_chain  = WLAN_PHY_CHAIN_ONE;
            pst_hal_device->st_cfg_cap_info.uc_single_tx_chain = WLAN_PHY_CHAIN_ONE;
            pst_hal_device->st_cfg_cap_info.uc_rf_chain        = WLAN_RF_CHAIN_DOUBLE;
            pst_hal_device->st_cfg_cap_info.uc_phy_chain       = WLAN_PHY_CHAIN_DOUBLE;
            break;

        /* ��phy0��siso,�����Ӳ������siso */
        case HAL_M2S_EVENT_COMMAND_MIMO_TO_SISO_C0:
        case HAL_M2S_EVENT_COMMAND_MISO_TO_SISO_C0:
        case HAL_M2S_EVENT_COMMAND_SISO_C1_TO_SISO_C0:
        case HAL_M2S_EVENT_ANT_RSSI_MISO_TO_C0_SISO:
        case HAL_M2S_EVENT_TEST_MIMO_TO_SISO_C0:
        case HAL_M2S_EVENT_TEST_SISO_C1_TO_SISO_C0:
        case HAL_M2S_EVENT_DBDC_MIMO_TO_SISO:
        case HAL_M2S_EVENT_DBDC_START:
        case HAL_M2S_EVENT_BT_MIMO_TO_SISO:
            pst_hal_device->st_cfg_cap_info.en_nss_num         = WLAN_SINGLE_NSS;
            pst_hal_device->st_cfg_cap_info.uc_phy2dscr_chain  = WLAN_PHY_CHAIN_ZERO;
            pst_hal_device->st_cfg_cap_info.uc_single_tx_chain = WLAN_PHY_CHAIN_ZERO;
            pst_hal_device->st_cfg_cap_info.uc_rf_chain        = WLAN_RF_CHAIN_ZERO;
            pst_hal_device->st_cfg_cap_info.uc_phy_chain       = WLAN_PHY_CHAIN_ZERO;
            break;

        /* ��phy1��siso,�����Ӳ������siso */
        case HAL_M2S_EVENT_ANT_RSSI_MISO_TO_C1_SISO:
        case HAL_M2S_EVENT_TEST_MIMO_TO_SISO_C1:
        case HAL_M2S_EVENT_TEST_SISO_C0_TO_SISO_C1:
        case HAL_M2S_EVENT_COMMAND_SISO_C0_TO_SISO_C1:
        case HAL_M2S_EVENT_COMMAND_MIMO_TO_SISO_C1:
        case HAL_M2S_EVENT_COMMAND_MISO_TO_SISO_C1:
            pst_hal_device->st_cfg_cap_info.en_nss_num         = WLAN_SINGLE_NSS;
            pst_hal_device->st_cfg_cap_info.uc_phy2dscr_chain  = WLAN_PHY_CHAIN_ONE;
            pst_hal_device->st_cfg_cap_info.uc_single_tx_chain = WLAN_PHY_CHAIN_ONE;
            pst_hal_device->st_cfg_cap_info.uc_rf_chain        = WLAN_RF_CHAIN_ONE;
            pst_hal_device->st_cfg_cap_info.uc_phy_chain       = WLAN_PHY_CHAIN_ONE;
            break;

        default:
           OAM_ERROR_LOG1(0, OAM_SF_M2S, "{dmac_m2s_update_switch_mgr_param: en_m2s_event[%d] error!}", en_m2s_event);
    }

    dmac_m2s_mgr_param_info(pst_hal_device);
}


oal_bool_enum_uint8 dmac_m2s_switch_priority_check(hal_to_dmac_device_stru *pst_hal_device,
    wlan_m2s_trigger_mode_enum_uint8 uc_trigger_mode, hal_m2s_state_uint8 uc_new_m2s_state)
{
    oal_bool_enum_uint8 en_allow = OAL_TRUE; /* ��ʼ����ҵ������ */

    /* 1.�����л�miso/siso ��Ҫ�ж�����ҵ���Ƿ��Ѿ����л� */
    if(HAL_M2S_STATE_BUTT == uc_new_m2s_state)
    {
        OAM_ERROR_LOG2(0, OAM_SF_M2S, "{dmac_m2s_switch_priority_check::uc_trigger_mode[%d] uc_new_m2s_state[%d]error.}", uc_trigger_mode, uc_new_m2s_state);
    }
    else if(HAL_M2S_STATE_MIMO == uc_new_m2s_state || HAL_M2S_STATE_MISO == uc_new_m2s_state)
    {
        /* Ҫ�л���miso ����mimoʱ�����������ҵ���Ѿ���ִ���ˣ�״̬��������ִ��(����ҵ����ִ�У�������siso״̬�����ָܻ�)
         ����ҵ����Ը����ǻָ���mimoʱ�����ʧ�ܣ��Կ����ͷ��Լ�ҵ��๦����Դ����Ҫһ������ TBD */
        switch(uc_trigger_mode)
        {
            /* dbdc��������������ɨ��������� */
            case WLAN_M2S_TRIGGER_MODE_FAST_SCAN:
                if(0 != (*(oal_uint8 *)(&pst_hal_device->st_hal_m2s_fsm.st_m2s_mode)& (BIT2 | BIT3 | BIT4 | BIT5)))
                {
                    en_allow = OAL_FALSE;
                }
              break;

            case WLAN_M2S_TRIGGER_MODE_DBDC:
            case WLAN_M2S_TRIGGER_MODE_RSSI_SNR:
            case WLAN_M2S_TRIGGER_MODE_BTCOEX:
            case WLAN_M2S_TRIGGER_MODE_COMMAND:
            case WLAN_M2S_TRIGGER_MODE_TEST:
                /* ���ڱ�״̬������ */
                if(0 != ((*(oal_uint8 *)(&pst_hal_device->st_hal_m2s_fsm.st_m2s_mode)& (~uc_trigger_mode))))
                {
                    en_allow = OAL_FALSE;
                }
              break;

            default:
                OAM_WARNING_LOG1(0, OAM_SF_M2S, "{dmac_m2s_switch_priority_check::uc_trigger_mode[%d] error.}", uc_trigger_mode);
              break;
         }
    }
    else if(HAL_M2S_STATE_SIMO == uc_new_m2s_state || HAL_M2S_STATE_SISO == uc_new_m2s_state)
    {
        /* Ҫ�л���simo ����sisoʱ�����������ҵ���Ѿ���ִ���ˣ� һ��ֻҪ���Ǵ���miso״̬��������ִ�У����miso���������߼��ж� */
        OAM_WARNING_LOG2(0, OAM_SF_M2S, "{dmac_m2s_switch_priority_check::uc_trigger_mode[%d] uc_new_m2s_state[%d]error.}", uc_trigger_mode, uc_new_m2s_state);
    }

    return en_allow;
}


oal_void dmac_m2s_switch_back_to_mixo_check(hal_m2s_fsm_stru *pst_m2s_fsm, hal_to_dmac_device_stru *pst_hal_device,
    wlan_m2s_trigger_mode_enum_uint8 uc_trigger_mode, hal_m2s_state_uint8 *pen_new_state)
{
    if(HAL_M2S_STATE_MIMO == GET_HAL_M2S_CUR_STATE(pst_hal_device) || HAL_M2S_STATE_BUTT == GET_HAL_M2S_CUR_STATE(pst_hal_device))
    {
        OAM_ERROR_LOG1(0, OAM_SF_M2S, "{dmac_m2s_switch_back_to_mixo_check::cur_m2s_state[%d] error.}", GET_HAL_M2S_CUR_STATE(pst_hal_device));
        return;
    }

    /* Ҫ�л���miso ����mimoʱ�����������ҵ���Ѿ���ִ���ˣ�״̬��������ִ��(����ҵ����ִ�У�������siso״̬�����ָܻ�)
     ����ҵ����Ը����ǻָ���mimoʱ�����ʧ�ܣ��Կ����ͷ��Լ�ҵ��๦����Դ����Ҫһ������ TBD */

    /* 1.���㱾ҵ��״̬ */
    GET_HAL_M2S_MODE_TPYE(pst_hal_device) &= (~uc_trigger_mode);

    /* 2.�ж��Ƿ�״̬�Ѿ������㣬���ݴ��ڵ�ҵ���л����µ�״̬����һ���ǻص�mimo */
    /*(1)simo�˳������������btcoex����test���л���siso,�����л���mimo
      (2)miso�˳������������btcoex����test���л���siso����command������miso
      (3)siso�˳������������btcoex��test��command������siso */

    OAM_WARNING_LOG2(0, OAM_SF_M2S, "{dmac_m2s_switch_back_to_mixo_check::uc_cur_m2s_state[%d] mode_stru[%d].}",
        *pen_new_state, *(oal_uint8 *)(&pst_hal_device->st_hal_m2s_fsm.st_m2s_mode));

    /* û������ҵ��ʱ�����Իص�mimo */
    if(0 == (*(oal_uint8 *)(&pst_hal_device->st_hal_m2s_fsm.st_m2s_mode)& (BIT1 | BIT2 | BIT3 | BIT4 | BIT5)))
    {
        dmac_m2s_fsm_trans_to_state(pst_m2s_fsm, HAL_M2S_STATE_MIMO);
    }
    else
    {
        if(HAL_M2S_STATE_SIMO == GET_HAL_M2S_CUR_STATE(pst_hal_device))
        {
            dmac_m2s_fsm_trans_to_state(pst_m2s_fsm, HAL_M2S_STATE_MIMO);

            /* 1. ��ǰ��simo���ڴ���mimo����commandҵ����Ҫ�ص�mimo����miso����Ҫֱ��miso��������ת��֧�� */
            if(OAL_TRUE == HAL_M2S_CHECK_COMMAND_ON(pst_hal_device))
            {
                dmac_m2s_handle_event(pst_hal_device, HAL_M2S_EVENT_COMMAND_MIMO_TO_MISO_C0, 0, OAL_PTR_NULL);
            }
        }
        else if(HAL_M2S_STATE_MISO == GET_HAL_M2S_CUR_STATE(pst_hal_device))
        {
            dmac_m2s_fsm_trans_to_state(pst_m2s_fsm, HAL_M2S_STATE_MIMO);

            /* 1. ��ǰ��miso���ڴ���mimo����testҵ����Ҫ�ص�mimo����siso����Ҫֱ��siso��������ת��֧�� */
            if(OAL_TRUE == HAL_M2S_CHECK_TEST_ON(pst_hal_device))
            {
                dmac_m2s_handle_event(pst_hal_device, HAL_M2S_EVENT_TEST_MIMO_TO_SISO_C0, 0, OAL_PTR_NULL);
            }
        }
        else
        {
            /* 1. ��ǰ��mimo���ڴ���mimo����testҵ�񱣳�siso */
            if(OAL_TRUE == HAL_M2S_CHECK_TEST_ON(pst_hal_device) || OAL_TRUE == HAL_M2S_CHECK_COMMAND_ON(pst_hal_device))
            {
                OAM_WARNING_LOG1(pst_hal_device->uc_device_id, OAM_SF_M2S, "{dmac_m2s_switch_back_to_mixo_check:: need to keep siso mode_stru[%d]!}",
                    GET_HAL_M2S_MODE_TPYE(pst_hal_device));
            }
            else
            {
                dmac_m2s_fsm_trans_to_state(pst_m2s_fsm, HAL_M2S_STATE_MIMO);

            }
        }
    }
}


oal_bool_enum_uint8  dmac_m2s_switch_apply_and_confirm(hal_to_dmac_device_stru *pst_hal_device,
    oal_uint16 us_m2s_type, wlan_m2s_trigger_mode_enum_uint8 uc_trigger_mode)
{
    hal_m2s_state_uint8    uc_new_m2s_state = HAL_M2S_STATE_BUTT;
    oal_bool_enum_uint8    en_allow = OAL_TRUE;

    /* 1.�����л�ҵ��ȷ����״̬���� */
    uc_new_m2s_state = dmac_m2s_event_state_classify(us_m2s_type);
    if(HAL_M2S_STATE_BUTT == uc_new_m2s_state)
    {
        return OAL_FALSE;
    }

    /* 2. old��new״̬һ�£�ֱ�ӷ���true����㲢��ʾ,���Լ���ִ�У�������Ȼ״̬����ת�����ǿ�����sisoʱ�л�ͨ�� */
    if(uc_new_m2s_state == GET_HAL_M2S_CUR_STATE(pst_hal_device))
    {
        OAM_WARNING_LOG2(0, OAM_SF_M2S, "{dmac_m2s_switch_apply_and_confirm: cur state[%d] conform us_m2s_type[%d]!}",
            GET_HAL_M2S_CUR_STATE(pst_hal_device), us_m2s_type);
        return OAL_TRUE;
    }

    /* 3.��Ҫ�ж�ҵ�����ͣ���������������c1 siso��rssi�߼���ҵ����ִ�� command��test����c0 siso����֧��  TBD Ҳҵ���Լ��ж� */

    return en_allow;
}


wlan_nss_enum_uint8  dmac_m2s_get_bss_max_nss(mac_vap_stru *pst_mac_vap, oal_netbuf_stru *pst_netbuf,
                                                 oal_uint16 us_frame_len, oal_bool_enum_uint8 en_assoc_status)
{
    oal_uint8           *puc_frame_body;
    oal_uint8           *puc_ie = OAL_PTR_NULL;
    oal_uint8           *puc_ht_mcs_bitmask;
    wlan_nss_enum_uint8  en_nss = WLAN_SINGLE_NSS;
    oal_uint16           us_vht_mcs_map;
    oal_uint16           us_msg_idx = 0;
    oal_uint16           us_offset;

    /* ��ȡ֡����ʼָ�� */
    puc_frame_body = MAC_GET_RX_PAYLOAD_ADDR(0, pst_netbuf);

    /* ����Beacon֡��fieldƫ���� */
    us_offset = MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN + MAC_CAP_INFO_LEN;

    /* ��ht ie */
    puc_ie = mac_find_ie(MAC_EID_HT_CAP, puc_frame_body + us_offset, us_frame_len - us_offset);
    if (OAL_PTR_NULL != puc_ie)
    {
        us_msg_idx = MAC_IE_HDR_LEN + MAC_HT_CAPINFO_LEN + MAC_HT_AMPDU_PARAMS_LEN;
        puc_ht_mcs_bitmask = &puc_ie[us_msg_idx];
        for (en_nss = WLAN_SINGLE_NSS; en_nss < WLAN_NSS_LIMIT; en_nss++)
        {
            if (0 == puc_ht_mcs_bitmask[en_nss])
            {
                break;
            }
        }
        if(WLAN_SINGLE_NSS == en_nss)
        {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_M2S, "dmac_m2s_get_bss_max_nss::ht cap's mcs error!");
        }
        else
        {
            en_nss--;
        }

        /* ����ǿ���up״̬��������htЭ��ʱ�����մ�ֵ���� */
        if(OAL_TRUE == en_assoc_status &&
            (WLAN_HT_MODE == pst_mac_vap->en_protocol || WLAN_HT_ONLY_MODE == pst_mac_vap->en_protocol ||
             WLAN_HT_11G_MODE == pst_mac_vap->en_protocol))
        {
            return en_nss;
        }
    }

    /* ��vht ie */
    puc_ie = mac_find_ie(MAC_EID_VHT_CAP, puc_frame_body + us_offset, us_frame_len - us_offset);
    if (OAL_PTR_NULL != puc_ie)
    {
        us_msg_idx = MAC_IE_HDR_LEN + MAC_VHT_CAP_INFO_FIELD_LEN;

        us_vht_mcs_map = OAL_MAKE_WORD16(puc_ie[us_msg_idx], puc_ie[us_msg_idx + 1]);

        for (en_nss = WLAN_SINGLE_NSS; en_nss < WLAN_NSS_LIMIT; en_nss++)
        {
            if (WLAN_INVALD_VHT_MCS == WLAN_GET_VHT_MAX_SUPPORT_MCS(us_vht_mcs_map & 0x3))
            {
                break;
            }
            us_vht_mcs_map >>= 2;
        }
        if(WLAN_SINGLE_NSS == en_nss)
        {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_M2S, "dmac_m2s_get_bss_max_nss::ht cap's mcs error!");
        }
        else
        {
            en_nss--;
        }

        /* ����ǿ���up״̬��������vhtЭ��ʱ�����մ�ֵ���� */
        if(OAL_TRUE == en_assoc_status &&
            (WLAN_VHT_MODE == pst_mac_vap->en_protocol || WLAN_VHT_ONLY_MODE == pst_mac_vap->en_protocol
#ifdef _PRE_WLAN_FEATURE_11AX
                 || WLAN_HE_MODE == pst_mac_vap->en_protocol
#endif
            ))
        {
            return en_nss;
        }
    }

    return en_nss;
}


oal_uint8 dmac_m2s_scan_get_num_sounding_dim(oal_netbuf_stru *pst_netbuf, oal_uint16 us_frame_len)
{
    oal_uint8      *puc_vht_cap_ie = OAL_PTR_NULL;
    oal_uint8      *puc_frame_body = OAL_PTR_NULL;
    oal_uint16      us_offset;
    oal_uint16      us_vht_cap_filed_low;
    oal_uint16      us_vht_cap_filed_high;
    oal_uint32      ul_vht_cap_field;
    oal_uint16      us_msg_idx;
    oal_uint8       uc_num_sounding_dim = 0;

    /* ��ȡ֡����ʼָ�� */
    puc_frame_body = MAC_GET_RX_PAYLOAD_ADDR(0, pst_netbuf);

    /* ����Beacon֡��fieldƫ���� */
    us_offset = MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN + MAC_CAP_INFO_LEN;
    puc_vht_cap_ie = mac_find_ie(MAC_EID_VHT_CAP, puc_frame_body + us_offset, us_frame_len - us_offset);
    if(OAL_PTR_NULL == puc_vht_cap_ie)
    {
        return uc_num_sounding_dim;
    }

    us_msg_idx = MAC_IE_HDR_LEN;

    /* ����VHT capablities info field */
    us_vht_cap_filed_low    = OAL_MAKE_WORD16(puc_vht_cap_ie[us_msg_idx], puc_vht_cap_ie[us_msg_idx + 1]);
    us_vht_cap_filed_high   = OAL_MAKE_WORD16(puc_vht_cap_ie[us_msg_idx + 2], puc_vht_cap_ie[us_msg_idx + 3]);
    ul_vht_cap_field        = OAL_MAKE_WORD32(us_vht_cap_filed_low, us_vht_cap_filed_high);
    /* ����num_sounding_dim */
    uc_num_sounding_dim = ((ul_vht_cap_field & (BIT18 | BIT17 | BIT16)) >> 16);
    return uc_num_sounding_dim;
}


oal_bool_enum_uint8 dmac_m2s_get_bss_support_opmode(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_frame_body, oal_uint16 us_frame_len)
{
    oal_uint8                  *puc_ie;
    oal_uint8                   us_offset;

    us_offset = MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN + MAC_CAP_INFO_LEN;
    puc_ie = mac_find_ie(MAC_EID_EXT_CAPS, puc_frame_body + us_offset, us_frame_len - us_offset);
    if(OAL_PTR_NULL == puc_ie || MAC_HT_EXT_CAP_OPMODE_LEN > puc_ie[1])
    {
        return OAL_FALSE;
    }
    return ((mac_ext_cap_ie_stru *)(puc_ie + MAC_IE_HDR_LEN))->bit_operating_mode_notification;
}


oal_void dmac_m2s_show_blacklist_in_list(hal_to_dmac_device_stru *pst_hal_device)
{
    hal_device_m2s_mgr_stru            *pst_device_m2s_mgr;
    hal_m2s_mgr_blacklist_stru         *pst_device_m2s_mgr_blacklist;
    oal_uint8                           uc_index;

    pst_device_m2s_mgr = GET_HAL_DEVICE_M2S_MGR(pst_hal_device);

    for (uc_index = 0; uc_index < pst_device_m2s_mgr->uc_blacklist_bss_cnt; uc_index++)
    {
        pst_device_m2s_mgr_blacklist = &(pst_device_m2s_mgr->ast_m2s_blacklist[uc_index]);

        OAM_WARNING_LOG_ALTER(pst_hal_device->uc_device_id, OAM_SF_M2S,
            "{dmac_m2s_show_blacklist_in_list::Find in blacklist, ap index[%d],action type[%d],addr->%02x:XX:XX:%02x:%02x:%02x.}",
            6, uc_index, pst_device_m2s_mgr_blacklist->en_action_type,
            pst_device_m2s_mgr_blacklist->auc_user_mac_addr[0], pst_device_m2s_mgr_blacklist->auc_user_mac_addr[3],
            pst_device_m2s_mgr_blacklist->auc_user_mac_addr[4], pst_device_m2s_mgr_blacklist->auc_user_mac_addr[5]);
    }
}


oal_uint32 dmac_m2s_check_blacklist_in_list(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_index)
{
    oal_uint8                           uc_index;
    hal_to_dmac_device_stru            *pst_hal_device;
    hal_device_m2s_mgr_stru            *pst_device_m2s_mgr;
    hal_m2s_mgr_blacklist_stru         *pst_device_m2s_mgr_blacklist;

    pst_hal_device = DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap);
    if (OAL_PTR_NULL == pst_hal_device)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_M2S, "{dmac_m2s_check_blacklist_in_list:: DMAC_VAP_GET_HAL_DEVICE null}");
        return OAL_FAIL;
    }

    pst_device_m2s_mgr = GET_HAL_DEVICE_M2S_MGR(pst_hal_device);

     /* ��ʼ��Ϊ��Чindex */
    *puc_index = WLAN_BLACKLIST_MAX;

    for (uc_index = 0; uc_index < WLAN_BLACKLIST_MAX; uc_index++)
    {
        pst_device_m2s_mgr_blacklist = &(pst_device_m2s_mgr->ast_m2s_blacklist[uc_index]);

        if (0 == oal_compare_mac_addr(pst_device_m2s_mgr_blacklist->auc_user_mac_addr, pst_mac_vap->auc_bssid))
        {
            OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_M2S,
                "{dmac_m2s_check_blacklist_in_list::Find in blacklist, addr->%02x:XX:XX:%02x:%02x:%02x.}",
                pst_mac_vap->auc_bssid[0], pst_mac_vap->auc_bssid[3], pst_mac_vap->auc_bssid[4], pst_mac_vap->auc_bssid[5]);

            *puc_index = uc_index;
            return OAL_TRUE;
        }
    }

    return OAL_FALSE;
}


OAL_STATIC oal_void dmac_m2s_add_and_update_blacklist_to_list(dmac_vap_stru *pst_dmac_vap)
{
    oal_uint8                          uc_index;
    hal_to_dmac_device_stru            *pst_hal_device;
    mac_vap_stru                       *pst_mac_vap;
    hal_device_m2s_mgr_stru            *pst_device_m2s_mgr;
    hal_m2s_mgr_blacklist_stru         *pst_device_m2s_mgr_blacklist;

    pst_mac_vap    = &pst_dmac_vap->st_vap_base_info;
    pst_hal_device = DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap);
    if (OAL_PTR_NULL == pst_hal_device)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_M2S, "{dmac_m2s_add_and_update_blacklist_to_list:: DMAC_VAP_GET_HAL_DEVICE null}");
        return;
    }

    pst_device_m2s_mgr = GET_HAL_DEVICE_M2S_MGR(pst_hal_device);

    if (pst_device_m2s_mgr->uc_blacklist_bss_index >= WLAN_BLACKLIST_MAX)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_M2S, "{dmac_m2s_add_and_update_blacklist_to_list::already reach max num:%d.}", WLAN_BLACKLIST_MAX);
        pst_device_m2s_mgr->uc_blacklist_bss_index = 0;
    }

    /* ����ap�Ƿ����б��У��ڵĻ�ˢ���л�ģʽ���ɣ����ڵĻ�����Ҫ������� */
    if(OAL_TRUE == dmac_m2s_check_blacklist_in_list(pst_mac_vap, &uc_index))
    {
        pst_device_m2s_mgr_blacklist = &(pst_device_m2s_mgr->ast_m2s_blacklist[uc_index]);

        /* ����Ѿ��ǲ���3���Ͳ�ˢ�� */
        if(HAL_M2S_ACTION_TYPR_NONE == pst_device_m2s_mgr_blacklist->en_action_type)
        {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_M2S, "{dmac_m2s_add_and_update_blacklist_to_list:: HAL_M2S_ACTION_TYPR_NONE already in MISO.}");
        }
        else
        {
#if 1
            /* smps��opmode����֧�֣���Ҫ������miso״̬���������豸��none�Ļ�����ֻ���л���miso״̬ */
            /* �ڶ�����Ӳ���noneģʽ */
            pst_device_m2s_mgr_blacklist->en_action_type = HAL_M2S_ACTION_TYPR_NONE;

            /* ��ǰ����ap��none�û�����Ҫ��hal deviceˢ�³��л�����miso״̬����̽��,��Ҫ���Ƕ�sta+gc����vstaȥ����ʱ����Ҫ�����Ƿ���Ҫ���� */
            GET_HAL_DEVICE_M2S_DEL_SWI_MISO_HOLD(pst_hal_device) = OAL_TRUE;

            OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_M2S,
                "{dmac_m2s_add_and_update_blacklist_to_list::ap use SWITCH_MODE_NONE, keep at miso, addr->%02x:XX:XX:%02x:%02x:%02x.}",
                pst_mac_vap->auc_bssid[0], pst_mac_vap->auc_bssid[3], pst_mac_vap->auc_bssid[4], pst_mac_vap->auc_bssid[5]);
#else

            /* �����л�����Ҫ��ֱ��miso�л���mimo��Ȼ��ֱ�ӵ�siso���� */
            (WLAN_PHY_CHAIN_ZERO == pst_hal_device->st_cfg_cap_info.uc_phy2dscr_chain)?
                (en_event_type = HAL_M2S_EVENT_COMMAND_MIMO_TO_SISO_C0): (en_event_type = HAL_M2S_EVENT_COMMAND_MIMO_TO_SISO_C1);

            /* ֱ�Ӵ�miso�л�mimo��siso�����ɣ�˵�����豸��opmode��smps֧�ֶ����ã�ֱ�Ӳ���action֡�����Զ˽��͵�siso */
            dmac_m2s_handle_event(pst_hal_device, HAL_M2S_EVENT_COMMAND_MISO_TO_MIMO, 0, OAL_PTR_NULL);

            /* �ڶ�����Ӳ���noneģʽ */
            pst_device_m2s_mgr_blacklist->en_m2s_switch_mode = HAL_M2S_SWITCH_MODE_NONE;

            OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_M2S,
                "{dmac_m2s_add_and_update_blacklist_to_list::ap use SWITCH_MODE_NONE, MIMO to SISO, addr->%02x:XX:XX:%02x:%02x:%02x.}",
                pst_mac_vap->auc_bssid[0], pst_mac_vap->auc_bssid[3], pst_mac_vap->auc_bssid[4], pst_mac_vap->auc_bssid[5]);

            dmac_m2s_handle_event(DMAC_VAP_GET_HAL_DEVICE(pst_dmac_vap), en_event_type, 0, OAL_PTR_NULL);
#endif
        }
    }
    else
    {
        pst_device_m2s_mgr_blacklist = &(pst_device_m2s_mgr->ast_m2s_blacklist[pst_device_m2s_mgr->uc_blacklist_bss_index]);
        oal_set_mac_addr(pst_device_m2s_mgr_blacklist->auc_user_mac_addr, pst_mac_vap->auc_bssid);

        /* ��һ������ǲ���OTHER��ʽ,Ϊ�˼���С��3·�������״���smps������ʱֱ�ӵ�none,
           �״���opmode�Ļ�,�ż�����smps ��none�Ļ����ͱ�����miso״̬�� ����������Ҫ��miso��siso�Ե���������ֹ��ִ�� */
       (HAL_M2S_ACTION_TYPR_SMPS == DMAC_VAP_GET_VAP_M2S_ACTION_ORI_TYPE(pst_mac_vap))?
        (pst_device_m2s_mgr_blacklist->en_action_type = HAL_M2S_ACTION_TYPR_OPMODE) :(pst_device_m2s_mgr_blacklist->en_action_type = HAL_M2S_ACTION_TYPR_SMPS);

        pst_device_m2s_mgr->uc_blacklist_bss_index++;

        /* ����¼WLAN_BLACKLIST_MAX���û�������ֱ�Ӹ��ǵ�һ�����������ֲ��� */
        if(WLAN_BLACKLIST_MAX > pst_device_m2s_mgr->uc_blacklist_bss_cnt)
        {
            pst_device_m2s_mgr->uc_blacklist_bss_cnt++;
        }

        OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_M2S,
            "{dmac_m2s_add_and_update_blacklist_to_list::new ap write in blacklist, addr->%02x:XX:XX:%02x:%02x:%02x.}",
            pst_mac_vap->auc_bssid[0], pst_mac_vap->auc_bssid[3], pst_mac_vap->auc_bssid[4], pst_mac_vap->auc_bssid[5]);

        /* ��һ��action֡����,���Ƿ�Զ����˻���siso���ݷ�ʽ */
        dmac_m2s_send_action_frame(pst_mac_vap);
        dmac_m2s_switch_protect_trigger(pst_dmac_vap);
    }
}


oal_bool_enum_uint8 dmac_m2s_assoc_vap_find_in_device_blacklist(hal_to_dmac_device_stru *pst_hal_device)
{
    mac_vap_stru                       *pst_mac_vap    = OAL_PTR_NULL;
    oal_uint8                           uc_vap_idx     = 0;
    oal_uint8                           uc_index       = 0;
    oal_uint8                           uc_up_vap_num;
    oal_uint8                           auc_mac_vap_id[WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE] = {0};
    hal_m2s_mgr_blacklist_stru         *pst_device_m2s_mgr_blacklist;

    /* ֻ���Լ���hal device���� */
    uc_up_vap_num = hal_device_find_all_up_vap(pst_hal_device, auc_mac_vap_id);
    for (uc_vap_idx = 0; uc_vap_idx < uc_up_vap_num; uc_vap_idx++)
    {
        pst_mac_vap = mac_res_get_mac_vap(auc_mac_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_mac_vap)
        {
            OAM_ERROR_LOG0(auc_mac_vap_id[uc_vap_idx], OAM_SF_M2S, "{dmac_m2s_command_switch_event_type_query::pst_mac_vap null.}");
            continue;
        }

        if(OAL_TRUE == dmac_m2s_check_blacklist_in_list(pst_mac_vap, &uc_index))
        {
            /* �ҵ���һ������������ap */
            pst_device_m2s_mgr_blacklist = &(GET_HAL_DEVICE_M2S_MGR(pst_mac_vap)->ast_m2s_blacklist[uc_index]);

            /* �����Ҫ�л���noneģʽ����������miso״̬ */
            if(HAL_M2S_ACTION_TYPR_NONE == pst_device_m2s_mgr_blacklist->en_action_type)
            {
                return OAL_TRUE;
            }
        }
    }

    return OAL_FALSE;
}


oal_void dmac_m2s_action_frame_type_query(mac_vap_stru *pst_mac_vap, hal_m2s_action_type_uint8 *pen_action_type)
{
    oal_uint8                           uc_index;
    hal_to_dmac_device_stru            *pst_hal_device;
    hal_device_m2s_mgr_stru            *pst_device_m2s_mgr;
    hal_m2s_mgr_blacklist_stru         *pst_device_m2s_mgr_blacklist;

    pst_hal_device = DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap);
    if (OAL_PTR_NULL == pst_hal_device)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_M2S, "{dmac_m2s_action_frame_type_query:: DMAC_VAP_GET_HAL_DEVICE null.}");
        return;
    }

    pst_device_m2s_mgr = GET_HAL_DEVICE_M2S_MGR(pst_hal_device);

    if(OAL_TRUE == dmac_m2s_check_blacklist_in_list(pst_mac_vap, &uc_index))
    {
        pst_device_m2s_mgr_blacklist = &(pst_device_m2s_mgr->ast_m2s_blacklist[uc_index]);

        /* ����ap�ں�����֮�У���Ҫ�����µ�action֡�������������� */
       *pen_action_type = pst_device_m2s_mgr_blacklist->en_action_type;
    }
}


oal_void dmac_m2s_send_action_complete_check(mac_vap_stru *pst_mac_vap, mac_tx_ctl_stru *pst_tx_ctl)
{
    /* ��������ж�succʱ���ж�cb֡���ͷ���smps����opmode��m2s�л��߼�����ִ����ȥ */
    if (MAC_GET_CB_IS_OPMODE_FRAME(pst_tx_ctl) || MAC_GET_CB_IS_SMPS_FRAME(pst_tx_ctl))
    {
        DMAC_VAP_GET_VAP_M2S_ACTION_SEND_STATE(pst_mac_vap) = OAL_TRUE;
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_M2S, "{dmac_m2s_send_action_complete_check:: action send succ.}");
    }
}


oal_void  dmac_m2s_send_action_frame(mac_vap_stru *pst_mac_vap)
{
    hal_m2s_action_type_uint8      en_action_type  = HAL_M2S_ACTION_TYPR_BUTT;

    /* ��ʱֻ��STAģʽ��֧�ַ���action֡ */
    if ((MAC_VAP_STATE_UP == pst_mac_vap->en_vap_state || MAC_VAP_STATE_PAUSE == pst_mac_vap->en_vap_state))
    {
        /* 1.��ʼ�л����� */
        /* ���ڶԶ�beaconЯ��vht�����չ�����htЭ���£���Ȼ��Ҫ����smps֡��߼����� */
        if(OAL_TRUE == mac_mib_get_OperatingModeNotificationImplemented(pst_mac_vap) &&
            (OAL_TRUE == pst_mac_vap->st_cap_flag.bit_opmode) &&
            (WLAN_VHT_MODE == pst_mac_vap->en_protocol || WLAN_VHT_ONLY_MODE == pst_mac_vap->en_protocol
#ifdef _PRE_WLAN_FEATURE_11AX
            || WLAN_HE_MODE == pst_mac_vap->en_protocol
#endif
        ))
        {
            en_action_type = HAL_M2S_ACTION_TYPR_OPMODE;
        }
        else
        {
            en_action_type = HAL_M2S_ACTION_TYPR_SMPS;
        }

        /* 2.ˢ��sta��ʼ�л�action����ģʽ�����ں���������·���������⴦�� */
        DMAC_VAP_GET_VAP_M2S_ACTION_ORI_TYPE(pst_mac_vap) = en_action_type;

        /* 3.���ݺ�������һ������֡�������� */
        dmac_m2s_action_frame_type_query(pst_mac_vap, &en_action_type);

#if 0
        /* 4.�л�����������ʱ��miso�л�mimoʱ������Ҫ��action֡ */
        if(OAL_TRUE == GET_HAL_DEVICE_M2S_SWITCH_PROT(DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap)) &&
            WLAN_DOUBLE_NSS == DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap)->st_cfg_cap_info.en_nss_num)
        {
            en_action_type = HAL_M2S_ACTION_TYPR_NONE;
        }
#endif

        /* 5.����action֡���Ϳ��� */
        DMAC_VAP_GET_VAP_M2S_ACTION_SEND_STATE(pst_mac_vap) = OAL_FALSE;

        /* 6.����action֡ */
        if(HAL_M2S_ACTION_TYPR_OPMODE == en_action_type)
        {
        #ifdef _PRE_WLAN_FEATURE_OPMODE_NOTIFY
            dmac_mgmt_send_opmode_notify_action(pst_mac_vap, pst_mac_vap->en_vap_rx_nss, OAL_TRUE);
        #endif
        }
        else if(HAL_M2S_ACTION_TYPR_SMPS == en_action_type)
        {
        #ifdef _PRE_WLAN_FEATURE_SMPS
            dmac_smps_send_action(pst_mac_vap, MAC_M2S_CALI_SMPS_MODE(pst_mac_vap->en_vap_rx_nss), OAL_TRUE);
        #endif
        }
        else
        {
#if 0
            //���շ�����none�������ʼ������opmode�Ļ����ͻ��Ƿ�smps��  С��3�ǳ�ʼ��smps��noneʱ����֡��������
            if(HAL_M2S_ACTION_TYPR_OPMODE == DMAC_VAP_GET_VAP_M2S_ORI_MODE(pst_mac_vap))
            {
            #ifdef _PRE_WLAN_FEATURE_SMPS
                dmac_smps_send_action(pst_mac_vap, MAC_M2S_CALI_SMPS_MODE(pst_mac_vap->en_vap_rx_nss), OAL_TRUE);
            #endif
            }
#endif
        }
    }
}


oal_uint32 dmac_m2s_d2h_device_info_syn(mac_device_stru *pst_mac_device)
{
    oal_uint32                  ul_ret;
    mac_device_m2s_stru         st_m2s_device;
    mac_vap_stru               *pst_mac_vap;

    /* ע���Ϊ�˻�ȡ����VAP */
    pst_mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->uc_cfg_vap_id);
    if(OAL_PTR_NULL == pst_mac_vap)
    {
        OAM_ERROR_LOG2(0, OAM_SF_M2S, "{dmac_m2s_d2h_device_info_syn::Cannot find cfg_vap by vapID[%d],when devID[%d].}",
            pst_mac_device->uc_cfg_vap_id, pst_mac_device->uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* m2s device��Ϣͬ��hmac,Ŀǰֻ��Ӳ�л�ʱ���nss������Ҫͬ��(���ܱ��ı�)st_device_cap�������� TBD */
    /* ��Ӳ�л���ͬ����ȥ��ֻ�����л���������ά�ֲ��� */
    st_m2s_device.uc_device_id   = pst_mac_device->uc_device_id;
    st_m2s_device.en_nss_num     = MAC_DEVICE_GET_NSS_NUM(pst_mac_device);
    st_m2s_device.en_smps_mode   = MAC_DEVICE_GET_MODE_SMPS(pst_mac_device);

    /***************************************************************************
        ���¼���HMAC��, ͬ��USER m2s������HMAC
    ***************************************************************************/
    ul_ret = dmac_send_sys_event(pst_mac_vap, WLAN_CFGID_DEVICE_M2S_INFO_SYN, OAL_SIZEOF(st_m2s_device), (oal_uint8 *)(&st_m2s_device));
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_M2S,
                         "{dmac_m2s_d2h_device_info_syn::dmac_send_sys_event failed[%d].}", ul_ret);
    }

    return OAL_SUCC;
}


oal_uint32 dmac_m2s_d2h_vap_info_syn(mac_vap_stru *pst_mac_vap)
{
    wlan_mib_mimo_power_save_enum_uint8 en_smps_mode;
    oal_uint32                          ul_ret;
    mac_vap_m2s_stru                    st_m2s_vap;

    /* m2s vap��Ϣͬ��hmac */
    st_m2s_vap.uc_vap_id   = pst_mac_vap->uc_vap_id;

    /* ���л�֮�����¹����ϣ�vap��nss����Ҫ����hal device��nss��������ˢ�� */
    mac_vap_set_rx_nss(pst_mac_vap, DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap)->st_cfg_cap_info.en_nss_num);

    st_m2s_vap.en_vap_rx_nss  = pst_mac_vap->en_vap_rx_nss;
#ifdef _PRE_WLAN_FEATURE_SMPS
    /* GO������vap init֮���л���sisoʱ���Ǹ�ֵ��mimo��smps����ʱ��Ҫ����vap nssȡС */
    en_smps_mode = mac_mib_get_smps(pst_mac_vap);
    en_smps_mode = OAL_MIN(en_smps_mode, MAC_M2S_CALI_SMPS_MODE(pst_mac_vap->en_vap_rx_nss));

    mac_mib_set_smps(pst_mac_vap, en_smps_mode);

    st_m2s_vap.en_sm_power_save = en_smps_mode;
#endif
#ifdef _PRE_WLAN_FEATURE_TXBF_HT
    st_m2s_vap.en_transmit_stagger_sounding = mac_mib_get_TransmitStaggerSoundingOptionImplemented(pst_mac_vap);
#endif
    st_m2s_vap.en_tx_stbc = mac_mib_get_TxSTBCOptionImplemented(pst_mac_vap);
    st_m2s_vap.en_vht_ctrl_field_supported = mac_mib_get_vht_ctrl_field_cap(pst_mac_vap); /* ����Ŀǰ����дFALSE,�����ٿ��Ż��������� */
#ifdef _PRE_WLAN_FEATURE_TXBF
    st_m2s_vap.en_tx_vht_stbc_optionimplemented = mac_mib_get_VHTTxSTBCOptionImplemented(pst_mac_vap);
    st_m2s_vap.en_vht_number_sounding_dimensions = mac_mib_get_VHTNumberSoundingDimensions(pst_mac_vap);
    st_m2s_vap.en_vht_su_beamformer_optionimplemented = mac_mib_get_VHTSUBeamformerOptionImplemented(pst_mac_vap);
#endif

    /* ��Ӳ���л�����ͬ����host�� */
    st_m2s_vap.en_m2s_type = DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap)->st_hal_m2s_fsm.en_m2s_type;

    /* �ؼ���Ϣͬ����ʾ */
    OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_M2S,
                       "{dmac_m2s_d2h_vap_info_syn::en_vap_rx_nss:%d,en_sm_power_save:%d, en_m2s_type[%d].}",
                         st_m2s_vap.en_vap_rx_nss, st_m2s_vap.en_sm_power_save, st_m2s_vap.en_m2s_type);

    /***************************************************************************
        ���¼���HMAC��, ͬ��USER m2s������HMAC
    ***************************************************************************/
    ul_ret = dmac_send_sys_event(pst_mac_vap, WLAN_CFGID_VAP_M2S_INFO_SYN, OAL_SIZEOF(st_m2s_vap), (oal_uint8 *)(&st_m2s_vap));
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_M2S,
                         "{dmac_m2s_d2h_vap_info_syn::dmac_send_sys_event failed[%d].}", ul_ret);
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 dmac_m2s_rx_ucast_nss_count_handle(mac_vap_stru *pst_mac_vap)
{
    hal_to_dmac_device_stru            *pst_hal_device;
    dmac_vap_m2s_rx_statistics_stru    *pst_dmac_vap_m2s_rx_statistics;
    dmac_vap_stru                      *pst_dmac_vap = MAC_GET_DMAC_VAP(pst_mac_vap);
    hal_m2s_event_tpye_uint16           en_event_type;

    pst_dmac_vap_m2s_rx_statistics = DMAC_VAP_GET_VAP_M2S_RX_STATISTICS(pst_mac_vap);

    OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_M2S,
                       "{dmac_m2s_rx_ucast_nss_count_handle::pst_dmac_vap_m2s_rx_statistics->us_rx_nss_mimo_count:%d,pst_dmac_vap_m2s_rx_statistics->us_rx_nss_siso_counte:%d.}",
                         pst_dmac_vap_m2s_rx_statistics->us_rx_nss_mimo_count, pst_dmac_vap_m2s_rx_statistics->us_rx_nss_siso_count);

    pst_hal_device = DMAC_VAP_GET_HAL_DEVICE(pst_dmac_vap);
    if (OAL_PTR_NULL == pst_hal_device)
    {
       OAM_ERROR_LOG0(0, OAM_SF_M2S, "{dmac_m2s_rx_ucast_nss_count_handle: pst_hal_device is null ptr}");
       return OAL_ERR_CODE_PTR_NULL;
    }

    /*
       ���л���2s֮��3sͳ��ʱ���ڣ���Ҫ�Զ˲���mimo��(������siso��)
       1.�ù�����Ҫ�ϲ���ϣ���������10��ping���� ����ʱ����Ҫ��������ping��
       2. û��mimo�����кܶ�siso��������Ŀǰ���޶���5��
       3.��Ի������ܱȽϲ�Զ�ֻ��siso�������������ʱ�����ǣ�������Ҳ��̫����
    */
    if(pst_dmac_vap_m2s_rx_statistics->us_rx_nss_mimo_count < M2S_MIMO_RX_CNT_THRESHOLD)
    {
        /* �����л�����Ҫ��ֱ��miso�л���siso���� */
        (WLAN_PHY_CHAIN_ZERO == pst_hal_device->st_cfg_cap_info.uc_phy2dscr_chain)?
            (en_event_type = HAL_M2S_EVENT_COMMAND_MISO_TO_SISO_C0): (en_event_type = HAL_M2S_EVENT_COMMAND_MISO_TO_SISO_C1);

        dmac_m2s_handle_event(DMAC_VAP_GET_HAL_DEVICE(pst_dmac_vap), en_event_type, 0, OAL_PTR_NULL);

        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_M2S, "{dmac_m2s_rx_ucast_nss_count_handle::delay switch to siso SUCC.}");
    }
    else
    {
        /* �Զ˻��Ƿ�mimo����Ϊ����˵���Զ˶Դ�action֡�����ã���Ҫ���л�����,��������� */
        dmac_m2s_add_and_update_blacklist_to_list(pst_dmac_vap);
    }

    return OAL_SUCC;
}


oal_void dmac_m2s_rx_rate_nss_process (mac_vap_stru *pst_vap,
    dmac_rx_ctl_stru *pst_cb_ctrl, mac_ieee80211_frame_stru *pst_frame_hdr)
{
    dmac_vap_m2s_rx_statistics_stru    *pst_dmac_vap_m2s_rx_statistics;
    wlan_phy_protocol_enum_uint8        en_phy_protocol;

    pst_dmac_vap_m2s_rx_statistics = DMAC_VAP_GET_VAP_M2S_RX_STATISTICS(pst_vap);

    /* 1.ͳ�ƿ���ʱ��ͳ�ƣ��������շ�֮��ſ�ʼͳ�� */
    if(OAL_FALSE == pst_dmac_vap_m2s_rx_statistics->en_rx_nss_statistics_start_flag)
    {
        return;
    }

    /* 2.vip֡���߹㲥֡��ͳ�ƣ���������siso֡ */
    if(OAL_TRUE == pst_cb_ctrl->st_rx_info.bit_is_key_frame || ETHER_IS_MULTICAST(pst_frame_hdr->auc_address1))
    {
        return;
    }

    /* 3.��ȡ����֡��nss���ͣ���Ҫ���ж�mimo֡������Լ�siso�ǲ���ͨ
         (1)����Ҫ������ʱ������Ϊ���ܱ����ڵ͹��ģ�û�������շ����Ǿ���miso�±����㹻��
         (2)�������շ��ˣ�ͳ������10������֡���ж���10������mimo��siso��� */
    pst_dmac_vap_m2s_rx_statistics->us_rx_nss_ucast_count++;

    en_phy_protocol = pst_cb_ctrl->st_rx_statistic.un_nss_rate.st_legacy_rate.bit_protocol_mode;
    if(WLAN_HT_PHY_PROTOCOL_MODE == en_phy_protocol)
    {
        if(pst_cb_ctrl->st_rx_statistic.un_nss_rate.st_ht_rate.bit_ht_mcs >= WLAN_HT_MCS8 &&
            pst_cb_ctrl->st_rx_statistic.un_nss_rate.st_ht_rate.bit_ht_mcs != WLAN_HT_MCS32)
        {
            pst_dmac_vap_m2s_rx_statistics->us_rx_nss_mimo_count++;
        }
        else
        {
            pst_dmac_vap_m2s_rx_statistics->us_rx_nss_siso_count++;
        }
    }
    else if(WLAN_VHT_PHY_PROTOCOL_MODE == en_phy_protocol)
    {
        if(pst_cb_ctrl->st_rx_statistic.un_nss_rate.st_vht_nss_mcs.bit_nss_mode == 1)
        {
            pst_dmac_vap_m2s_rx_statistics->us_rx_nss_mimo_count++;
        }
        else
        {
            pst_dmac_vap_m2s_rx_statistics->us_rx_nss_siso_count++;
        }
    }
    else
    {
        pst_dmac_vap_m2s_rx_statistics->us_rx_nss_siso_count++;
    }

    /* 4. �ﵽ����ͳ�����ޣ���ʼ���� */
    if(M2S_RX_UCAST_CNT_THRESHOLD == pst_dmac_vap_m2s_rx_statistics->us_rx_nss_ucast_count)
    {
        pst_dmac_vap_m2s_rx_statistics->en_rx_nss_statistics_start_flag = OAL_FALSE;
        dmac_m2s_rx_ucast_nss_count_handle(pst_vap);
    }
}


oal_uint32 dmac_m2s_delay_switch_nss_statistics_callback(oal_void *p_arg)
{
    hal_to_dmac_device_stru            *pst_hal_device;
    dmac_vap_m2s_rx_statistics_stru    *pst_dmac_vap_m2s_rx_statistics;
    mac_vap_stru                       *pst_mac_vap;

    pst_mac_vap = (mac_vap_stru *)p_arg;
    pst_hal_device = DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap);

    pst_dmac_vap_m2s_rx_statistics = DMAC_VAP_GET_VAP_M2S_RX_STATISTICS(pst_mac_vap);

    /* 1.ҵ����� */
    if (OAL_FALSE == GET_HAL_DEVICE_M2S_SWITCH_PROT(pst_hal_device))
    {
        return OAL_SUCC;
    }

    /* 2.action֡���ͳɹ����ż���ִ����ȥ�����������action֡�����ٴδ�ͳ�� */
    if(OAL_FALSE == DMAC_VAP_GET_VAP_M2S_ACTION_SEND_STATE(pst_mac_vap) &&
        M2S_ACTION_SENT_CNT_THRESHOLD > DMAC_VAP_GET_VAP_M2S_ACTION_SEND_CNT(pst_mac_vap))
    {
        dmac_m2s_send_action_frame(pst_mac_vap);

        /* ��ʼ��0����������ۼ� */
        DMAC_VAP_GET_VAP_M2S_ACTION_SEND_CNT(pst_mac_vap)++;

        FRW_TIMER_CREATE_TIMER(&(pst_dmac_vap_m2s_rx_statistics->m2s_delay_switch_statistics_timer),
                           dmac_m2s_delay_switch_nss_statistics_callback,
                           M2S_RX_STATISTICS_START_TIME,
                           (oal_void *)pst_mac_vap,
                           OAL_FALSE,
                           OAM_MODULE_ID_DMAC,
                           pst_mac_vap->ul_core_id);
    }
    else
    {
        /* 3. action֡���ͳɹ������ͳ��ֵ����ʼͳ��nss */
        pst_dmac_vap_m2s_rx_statistics->us_rx_nss_mimo_count  = 0;
        pst_dmac_vap_m2s_rx_statistics->us_rx_nss_siso_count  = 0;
        pst_dmac_vap_m2s_rx_statistics->us_rx_nss_ucast_count = 0;
        pst_dmac_vap_m2s_rx_statistics->en_rx_nss_statistics_start_flag = OAL_TRUE;
        DMAC_VAP_GET_VAP_M2S_ACTION_SEND_CNT(pst_mac_vap)     = 0;
    }

    return OAL_SUCC;
}


oal_void dmac_m2s_switch_protect_trigger(dmac_vap_stru *pst_dmac_vap)
{
    dmac_vap_m2s_rx_statistics_stru *pst_dmac_vap_m2s_rx_statistics;

    if(!IS_STA(&pst_dmac_vap->st_vap_base_info))
    {
        return;
    }

    pst_dmac_vap_m2s_rx_statistics = DMAC_VAP_GET_VAP_M2S_RX_STATISTICS(pst_dmac_vap);

    /* 1.nssͳ����ʱ���ܴ�,�ȳ�ʱ��ʱ���ڲſ�ʼͳ�� */
    pst_dmac_vap_m2s_rx_statistics->en_rx_nss_statistics_start_flag = OAL_FALSE;

    /* 2.����ͳ�ƶ�ʱ��,1s֮��ʼͳ��˫�����ݰ�����,���в����豸��Ӳ�����л��в���mimo��Ҫ�ȷ�����л���siso���� */
    if(OAL_TRUE == pst_dmac_vap_m2s_rx_statistics->m2s_delay_switch_statistics_timer.en_is_registerd)
    {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_dmac_vap_m2s_rx_statistics->m2s_delay_switch_statistics_timer));
    }

    FRW_TIMER_CREATE_TIMER(&(pst_dmac_vap_m2s_rx_statistics->m2s_delay_switch_statistics_timer),
                           dmac_m2s_delay_switch_nss_statistics_callback,
                           M2S_RX_STATISTICS_START_TIME,
                           (oal_void *)(&pst_dmac_vap->st_vap_base_info),
                           OAL_FALSE,
                           OAM_MODULE_ID_DMAC,
                           pst_dmac_vap->st_vap_base_info.ul_core_id);
}


oal_uint32 dmac_m2s_disassoc_state_syn(mac_vap_stru *pst_mac_vap)
{
    dmac_vap_m2s_stru                  *pst_dmac_vap_m2s;
    dmac_vap_m2s_rx_statistics_stru    *pst_dmac_vap_m2s_rx_statistics;
    hal_to_dmac_device_stru            *pst_hal_device;

    if(!IS_STA(pst_mac_vap))
    {
        return OAL_SUCC;
    }

    pst_hal_device = DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap);
    if (OAL_PTR_NULL == pst_hal_device)
    {
        OAM_WARNING_LOG0(0, OAM_SF_M2S, "{dmac_m2s_disassoc_state_syn: pst_hal_device is null ptr.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_dmac_vap_m2s = DMAC_VAP_GET_VAP_M2S(pst_mac_vap);

    pst_dmac_vap_m2s_rx_statistics = &(pst_dmac_vap_m2s->st_dmac_vap_m2s_rx_statistics);
    FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_dmac_vap_m2s_rx_statistics->m2s_delay_switch_statistics_timer));

    /* 1.ͳ����Ϣͳһ���� */
    OAL_MEMZERO(pst_dmac_vap_m2s, OAL_SIZEOF(dmac_vap_m2s_stru));

    /* 2.ˢ��hal device���Ƿ񱣳�miso��� */
    if(OAL_FALSE == dmac_m2s_assoc_vap_find_in_device_blacklist(pst_hal_device))
    {
        GET_HAL_DEVICE_M2S_DEL_SWI_MISO_HOLD(pst_hal_device) = OAL_FALSE;
    }

    return OAL_SUCC;
}


oal_void  dmac_m2s_update_user_capbility(mac_user_stru *pst_mac_user, mac_vap_stru *pst_mac_vap)
{
    wlan_nss_enum_uint8        en_old_avail_num_spatial_stream;

    /* pause tid */
    //dmac_user_pause(pst_dmac_user);
    //�����л���ֻ�ı�vap���������޸�user��������
    //mac_user_set_sm_power_save(pst_user, MAC_M2S_CALI_SMPS_MODE(en_nss));

    /* �����Ǵ���HT֧���л�������user����״̬Ϊ��ht��vht�����߶Զ˲�֧��mimo��Ҳ����֧���л���������ʾ�쳣 */
    if ((WLAN_SINGLE_NSS == pst_mac_user->en_user_num_spatial_stream)||
        (WLAN_HT_MODE > pst_mac_user->en_cur_protocol_mode))
    {
        OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_M2S,
            "{dmac_m2s_update_vap_capbility::user isnot support HT/VHT user id[%d],vap protocol[%d],user_nss[%d],user protocol[%d]!}",
             pst_mac_user->us_assoc_id, pst_mac_vap->en_vap_mode,
             pst_mac_user->en_user_num_spatial_stream, pst_mac_user->en_protocol_mode);

        return;
    }

    /* �û��ռ��� */
    en_old_avail_num_spatial_stream           = pst_mac_user->en_avail_num_spatial_stream;

    /* ����֮���user�ռ������� */
    pst_mac_user->en_avail_num_spatial_stream = OAL_MIN(pst_mac_vap->en_vap_rx_nss, pst_mac_user->en_user_num_spatial_stream);

    /* �û��ռ�������û�з����仯Ҳ��Ҫͬ�������ܴ�ʱhal phy chain�����仯���ض��㷨ģ����Ҫ��Ӧ���� */
#ifdef _PRE_WLAN_FEATURE_BTCOEX
    /* user�ռ����仯֪ͨbtcoexˢ���������� */
    dmac_btcoex_user_spatial_stream_change_notify(pst_mac_vap, pst_mac_user);
#endif
    /* �����㷨���Ӻ���,Ŀǰֻ��֪ͨautorate����������TXBF TBD */
    dmac_alg_cfg_user_spatial_stream_notify(pst_mac_user);

    /* �û��ռ������������仯����Ҫ֪ͨ�㷨 */
    if(en_old_avail_num_spatial_stream != pst_mac_user->en_avail_num_spatial_stream)
    {
        /* user����ͬ����hmac */
        if (OAL_SUCC != dmac_config_d2h_user_m2s_info_syn(pst_mac_vap, pst_mac_user))
        {
            OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_M2S,
                      "{dmac_m2s_update_user_capbility::dmac_config_d2h_user_m2s_info_syn failed.}");
        }
    }
}


oal_void  dmac_m2s_update_vap_capbility(mac_device_stru *pst_mac_device, mac_vap_stru *pst_mac_vap)
{
    hal_to_dmac_device_stru *pst_hal_device = DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap);
    oal_dlist_head_stru     *pst_entry      = OAL_PTR_NULL;
    dmac_vap_stru           *pst_dmac_vap   = OAL_PTR_NULL;
    mac_user_stru           *pst_mac_user   = OAL_PTR_NULL;
    oal_bool_enum            en_send_action = OAL_TRUE;

    /* vap��ǰɾ����ָ���Ѿ���Ϊ�� */
    if(OAL_PTR_NULL == pst_hal_device)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_M2S, "{dmac_m2s_update_vap_capbility::the hal devcie is null!}");
        return;
    }
    pst_dmac_vap = MAC_GET_DMAC_VAP(pst_mac_vap);

    /* mimo/siso�л�ʱ�������ѹ���sta������bcn��ͨ�� */
    if (WLAN_VAP_MODE_BSS_STA == pst_mac_vap->en_vap_mode
      && (MAC_VAP_STATE_UP == pst_mac_vap->en_vap_state || MAC_VAP_STATE_PAUSE == pst_mac_vap->en_vap_state))
    {
#ifdef _PRE_WLAN_FEATURE_SINGLE_RF_RX_BCN
        dmac_psm_update_bcn_rf_chain(pst_dmac_vap, oal_get_real_rssi(pst_dmac_vap->st_query_stats.s_signal));
        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_m2s_update_vap_capbility:bcn_rx_chain[%d],rssi[%d]}",
                 pst_dmac_vap->pst_hal_vap->st_pm_info.uc_bcn_rf_chain, oal_get_real_rssi(pst_dmac_vap->st_query_stats.s_signal));
#else
        hal_pm_set_bcn_rf_chain(pst_dmac_vap->pst_hal_vap, pst_hal_device->st_cfg_cap_info.uc_rf_chain);
#endif
    }

    //����л���ͨ��2��siso����ʱҲ��Ҫ��ʼ�����Ͳ���ͨ������Ҫ�����ߵ�init_rate ��������ȥ; siso֧�ּ����ߣ�����֧��mimo���������е�mimo
    if ((pst_mac_vap->en_protocol < WLAN_HT_MODE)&&(WLAN_DOUBLE_NSS == pst_hal_device->st_cfg_cap_info.en_nss_num))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_M2S, "{dmac_m2s_update_vap_capbility::the vap is not support HT/VHT!}");
        return;
    }

    //����ʱ����Ҫ����ͬ����host�࣬�������ȣ�����Լ������֤����ҵ������
    /* 1. �ı�vap�¿ռ�����������,���ں����û�����ʱ��user����������֪ͨ�㷨,����ģʽ֪ͨie�ֶ���д TBD ��ЩmibҪ����siso�仯����С��ȷ��*/
    /* �ռ����������䣬������c0 siso�л���c1 siso����Ҫ֧�� */
    if(pst_hal_device->st_cfg_cap_info.en_nss_num == pst_mac_vap->en_vap_rx_nss)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_M2S,
            "{dmac_m2s_update_vap_capbility::en_nss value[%d] is the same with pst_mac_vap.}", pst_mac_vap->en_vap_rx_nss);
        en_send_action = OAL_FALSE;
        //return;
    }

    mac_vap_set_rx_nss(pst_mac_vap, pst_hal_device->st_cfg_cap_info.en_nss_num);

    /* 2.�޸�HT���� */
#ifdef _PRE_WLAN_FEATURE_SMPS
      /* �޸�smps����, ֱ��֪ͨsmps�����޸����̸��ӣ�����mimo/siso ���mib����ͳһ���� */
      mac_mib_set_smps(pst_mac_vap, MAC_M2S_CALI_SMPS_MODE(pst_hal_device->st_cfg_cap_info.en_nss_num));
#endif
      /* �޸�tx STBC���� */
      mac_mib_set_TxSTBCOptionImplemented(pst_mac_vap,
          pst_hal_device->st_cfg_cap_info.en_tx_stbc_is_supp & MAC_DEVICE_GET_CAP_TXSTBC(pst_mac_device));
#ifdef _PRE_WLAN_FEATURE_TXBF_HT
      mac_mib_set_TransmitStaggerSoundingOptionImplemented(pst_mac_vap,
          pst_hal_device->st_cfg_cap_info.en_su_bfmer_is_supp & MAC_DEVICE_GET_CAP_SUBFER(pst_mac_device));
#endif
      mac_mib_set_vht_ctrl_field_cap(pst_mac_vap, pst_hal_device->st_cfg_cap_info.en_nss_num); /* ����Ŀǰ����дFALSE,�����ٿ��Ż��������� */
#ifdef _PRE_WLAN_FEATURE_TXBF
      /* û�ж�Ӧhal device��mibֵ����ʱ����nss����ֵ�� �������Ƿ��Ż� */
      mac_mib_set_VHTNumberSoundingDimensions(pst_mac_vap,
          OAL_MIN(pst_hal_device->st_cfg_cap_info.en_nss_num, MAC_DEVICE_GET_NSS_NUM(pst_mac_device)));
      mac_mib_set_VHTSUBeamformerOptionImplemented(pst_mac_vap,
          pst_hal_device->st_cfg_cap_info.en_su_bfmer_is_supp & MAC_DEVICE_GET_CAP_SUBFER(pst_mac_device));
      mac_mib_set_VHTTxSTBCOptionImplemented(pst_mac_vap,
          pst_hal_device->st_cfg_cap_info.en_tx_stbc_is_supp & MAC_DEVICE_GET_CAP_TXSTBC(pst_mac_device));
#endif

    /* Ӳ�л� */
    if(WLAN_M2S_TYPE_HW == pst_hal_device->st_hal_m2s_fsm.en_m2s_type)
    {
        /* 4. �������ʼ���vap�Ŀռ�������ʱδ����������Ҫ���µĵط� */
        mac_vap_init_rates(pst_mac_vap);
    }

    /* 5. ���³�ʼ������֡/����֡���Ͳ��� */
    /* ��ʼ����������֡�ķ��Ͳ��� */
    dmac_vap_init_tx_ucast_data_frame(MAC_GET_DMAC_VAP(pst_mac_vap));

    /* ��ʼ������������֡��������֡�ķ��Ͳ��� */
    dmac_vap_init_tx_frame_params(MAC_GET_DMAC_VAP(pst_mac_vap), OAL_TRUE);

    /* 6.ͬ��vap ��m2s������hmac */
    if (OAL_SUCC != dmac_m2s_d2h_vap_info_syn(pst_mac_vap))
    {
         OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_M2S,
                  "{dmac_m2s_update_vap_capbility::dmac_m2s_d2h_vap_info_syn failed.}");
    }

    if(WLAN_M2S_TYPE_SW == pst_hal_device->st_hal_m2s_fsm.en_m2s_type)
    {
        /* 7.ˢ���û���Ϣ����֪ͨ�㷨����tx���� */
        OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_mac_vap->st_mac_user_list_head))
        {
            pst_mac_user = OAL_DLIST_GET_ENTRY(pst_entry, mac_user_stru, st_user_dlist);
            /*lint -save -e774 */
            if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_user))
            {
                OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_M2S, "{dmac_m2s_update_vap_capbility::pst_mac_user null pointer.}");
                continue;
            }
            /*lint -restore */

            dmac_m2s_update_user_capbility(pst_mac_user, pst_mac_vap);

            /* DBDC����vap�Լ�user��������Ҫ��action֡���滢���Ż� */
            if(WLAN_VAP_MODE_BSS_STA == pst_mac_vap->en_vap_mode && OAL_TRUE == en_send_action)
            {
                /* 8.(1)����siso״̬�������л�����ʹ�ܣ������л���������
                     (2)����Ѿ��Ǳ����Ҫhold�û���device(��vap��blacklist��none)��Ҳ����Ҫ�����л����� */
                if(OAL_TRUE == GET_HAL_DEVICE_M2S_SWITCH_PROT(pst_hal_device) &&
                    WLAN_SINGLE_NSS == pst_hal_device->st_cfg_cap_info.en_nss_num &&
                    OAL_FALSE == GET_HAL_DEVICE_M2S_DEL_SWI_MISO_HOLD(pst_hal_device))
                {
                    dmac_m2s_switch_protect_trigger(pst_dmac_vap);
                }

                /* 9. ����action֡��֪ͨ�Զ��޸�m2s���� */
                dmac_m2s_send_action_frame(pst_mac_vap);
            }
        }
    }
}


oal_void  dmac_m2s_switch_update_vap_capbility(hal_to_dmac_device_stru *pst_hal_device)
{
    mac_device_stru         *pst_mac_device = OAL_PTR_NULL;
    mac_vap_stru            *pst_mac_vap    = OAL_PTR_NULL;
    oal_uint8                uc_vap_idx     = 0;

    pst_mac_device = mac_res_get_dev(pst_hal_device->uc_mac_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
       OAM_ERROR_LOG1(0, OAM_SF_M2S, "{dmac_m2s_switch_update_vap_capbility: mac device is null ptr. device id:%d}", pst_hal_device->uc_mac_device_id);
       return;
    }

    /* ����mac device������ҵ��vap��ˢ���������ڱ�hal device��vap������ */
    /* ע��:Ӳ�л�������ȥ���������ܲ���hal_device_find_all_up_vap����ȡ��hal device�ϵ�vap�����ܸ���ɨ��/��������ģʽ */
    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_mac_vap)
        {
            OAM_ERROR_LOG0(pst_mac_device->auc_vap_id[uc_vap_idx], OAM_SF_M2S, "{dmac_m2s_switch_update_vap_capbility::the vap is null!}");
            continue;
        }

        /* �����ڱ�hal device��vap������mimo-siso����(��hal device����֧��mimo) */
        if(pst_hal_device != MAC_GET_DMAC_VAP(pst_mac_vap)->pst_hal_device)
        {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_M2S,
               "{dmac_m2s_switch_update_vap_capbility::the vap not belong to this hal device!}");
            continue;
        }

        /* ����vap���� */
        dmac_m2s_update_vap_capbility(pst_mac_device, pst_mac_vap);
    }
}


oal_void  dmac_m2s_switch_update_hal_chain_capbility(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_set_channel)
{
    /* ���ö�Ӧphy��ana dbbͨ��ӳ���ϵ */
    hal_set_phy_rf_chain_relation(pst_hal_device, en_set_channel);

    OAM_WARNING_LOG4(0, OAM_SF_M2S, "{dmac_m2s_switch_update_hal_chain_capbility::phy_chain[%d],uc_phy2hw_chain[%d],single_tx_chain[%d],rf_chain[%d].}",
         pst_hal_device->st_cfg_cap_info.uc_phy2dscr_chain, pst_hal_device->st_cfg_cap_info.uc_phy_chain,
         pst_hal_device->st_cfg_cap_info.uc_single_tx_chain, pst_hal_device->st_cfg_cap_info.uc_rf_chain);
}


oal_void  dmac_m2s_switch_update_device_capbility(hal_to_dmac_device_stru *pst_hal_device, mac_device_stru *pst_mac_device)
{
    /* 1.�����㷨������ */
    switch(pst_hal_device->st_cfg_cap_info.en_nss_num)
    {
        case WLAN_SINGLE_NSS:
            pst_hal_device->st_cfg_cap_info.en_tx_stbc_is_supp  = OAL_FALSE;
            pst_hal_device->st_cfg_cap_info.en_su_bfmer_is_supp = OAL_FALSE;
            break;

        case WLAN_DOUBLE_NSS:
            pst_hal_device->st_cfg_cap_info.en_tx_stbc_is_supp  = OAL_TRUE;
            pst_hal_device->st_cfg_cap_info.en_su_bfmer_is_supp = OAL_TRUE;
            break;

        default:
            OAM_WARNING_LOG1(0, OAM_SF_M2S, "{dmac_m2s_switch_update_device_capbility: en_nss[%d] error!}", pst_hal_device->st_cfg_cap_info.en_nss_num);
            return;
    }

    if(WLAN_M2S_TYPE_HW == pst_hal_device->st_hal_m2s_fsm.en_m2s_type)
    {
        /* 2.Ӳ�л�׼��������mac device��nss���� */
        MAC_DEVICE_GET_NSS_NUM(pst_mac_device) = pst_hal_device->st_cfg_cap_info.en_nss_num;
    }

    /* 3.ͬ��mac device ��smps������hmac */
    MAC_DEVICE_GET_MODE_SMPS(pst_mac_device) = MAC_M2S_CALI_SMPS_MODE(pst_hal_device->st_cfg_cap_info.en_nss_num);

    /* 4.ͬ��device ��m2s������hmac */
    if (OAL_SUCC != dmac_m2s_d2h_device_info_syn(pst_mac_device))
    {
        OAM_WARNING_LOG0(0, OAM_SF_M2S,
               "{dmac_m2s_switch_update_device_capbility::dmac_m2s_d2h_device_info_syn failed.}");
    }

    OAM_WARNING_LOG3(0, OAM_SF_M2S, "{dmac_m2s_switch_update_device_capbility::nss_num[%d],mac_dev_nss[%d],mac_dev_smps_mode[%d].}",
         pst_hal_device->st_cfg_cap_info.en_nss_num, MAC_DEVICE_GET_NSS_NUM(pst_mac_device), MAC_DEVICE_GET_MODE_SMPS(pst_mac_device));

}


oal_void dmac_m2s_switch_same_channel_vaps_begin(hal_to_dmac_device_stru *pst_hal_device,
                   mac_device_stru *pst_mac_device, mac_vap_stru *pst_mac_vap1, mac_vap_stru *pst_mac_vap2)
{
    mac_vap_stru                   *pst_vap_sta;
    mac_fcs_mgr_stru               *pst_fcs_mgr;
    mac_fcs_cfg_stru               *pst_fcs_cfg;

    if (MAC_VAP_STATE_UP == pst_mac_vap1->en_vap_state)
    {
        /* ��ͣvapҵ�� */
        dmac_vap_pause_tx(pst_mac_vap1);
    }

    if (MAC_VAP_STATE_UP == pst_mac_vap2->en_vap_state)
    {
        /* ��ͣvapҵ�� */
        dmac_vap_pause_tx(pst_mac_vap2);
    }

    /* ���ڵ͹���״̬���Ѿ�����null֡��ֱ�ӷ��� */
    if(HAL_DEVICE_WORK_STATE == GET_HAL_DEVICE_STATE(pst_hal_device) &&
         HAL_DEVICE_WORK_SUB_STATE_ACTIVE != GET_WORK_SUB_STATE(pst_hal_device))
    {
        OAM_INFO_LOG1(0, OAM_SF_M2S, "{dmac_m2s_switch_same_channel_vaps_begin::powersave state[%d].}", GET_WORK_SUB_STATE(pst_hal_device));
        return;
    }

    pst_fcs_mgr = dmac_fcs_get_mgr_stru(pst_mac_device);
    pst_fcs_cfg = MAC_DEV_GET_FCS_CFG(pst_mac_device);
    OAL_MEMZERO(pst_fcs_cfg, OAL_SIZEOF(mac_fcs_cfg_stru));

    pst_fcs_cfg->st_dst_chl = pst_hal_device->st_wifi_channel_status;
    pst_fcs_cfg->pst_hal_device = pst_hal_device;
    pst_fcs_cfg->pst_src_fake_queue = DMAC_VAP_GET_FAKEQ(pst_mac_vap1);

    /* ͬƵ˫STAģʽ����Ҫ������one packet */
    if (WLAN_VAP_MODE_BSS_STA == pst_mac_vap1->en_vap_mode && WLAN_VAP_MODE_BSS_STA == pst_mac_vap2->en_vap_mode)
    {
        /* ׼��VAP1��fcs���� */
        pst_fcs_cfg->st_src_chl = pst_mac_vap1->st_channel;
        dmac_fcs_prepare_one_packet_cfg(pst_mac_vap1, &(pst_fcs_cfg->st_one_packet_cfg), pst_hal_device->st_hal_scan_params.us_scan_time);

        /* ׼��VAP2��fcs���� */
        pst_fcs_cfg->st_src_chl2 = pst_mac_vap2->st_channel;
        dmac_fcs_prepare_one_packet_cfg(pst_mac_vap2, &(pst_fcs_cfg->st_one_packet_cfg2), pst_hal_device->st_hal_scan_params.us_scan_time);

        pst_fcs_cfg->st_one_packet_cfg2.us_timeout = MAC_FCS_DEFAULT_PROTECT_TIME_OUT2;     /* ��С�ڶ���one packet�ı���ʱ�����Ӷ�������ʱ�� */
        pst_fcs_cfg->pst_hal_device = pst_hal_device;

        dmac_fcs_start_enhanced_same_channel(pst_fcs_mgr, pst_fcs_cfg);
        mac_fcs_release(pst_fcs_mgr);
    }
    /* ͬƵSTA+GOģʽ��ͬƵSTA+APģʽ��ֻ��ҪSTA��һ��one packet */
    else
    {
        if (WLAN_VAP_MODE_BSS_STA == pst_mac_vap1->en_vap_mode)
        {
            pst_vap_sta = pst_mac_vap1;
        }
        else
        {
            pst_vap_sta = pst_mac_vap2;
        }

        pst_fcs_cfg->st_src_chl = pst_vap_sta->st_channel;
        dmac_fcs_prepare_one_packet_cfg(pst_vap_sta, &(pst_fcs_cfg->st_one_packet_cfg), pst_hal_device->st_hal_scan_params.us_scan_time);

        /* ����FCS���ŵ��ӿ� ���浱ǰӲ�����е�֡��ɨ����ٶ��� */
        dmac_fcs_start_same_channel(pst_fcs_mgr, pst_fcs_cfg, 0);
        mac_fcs_release(pst_fcs_mgr);
    }
}

#ifdef _PRE_WLAN_FEATURE_DBAC

oal_void dmac_m2s_switch_dbac_vaps_begin(hal_to_dmac_device_stru *pst_hal_device,
                     mac_device_stru  *pst_mac_device, mac_vap_stru  *pst_mac_vap1, mac_vap_stru *pst_mac_vap2)
{
    mac_fcs_mgr_stru               *pst_fcs_mgr;
    mac_fcs_cfg_stru               *pst_fcs_cfg;
    mac_vap_stru                   *pst_current_chan_vap;

    if (pst_hal_device->uc_current_chan_number == pst_mac_vap1->st_channel.uc_chan_number)
    {
        pst_current_chan_vap = pst_mac_vap1;
    }
    else
    {
        pst_current_chan_vap = pst_mac_vap2;
    }

    /* ��ͣDBAC���ŵ� */
    if (mac_is_dbac_running(pst_mac_device))
    {
        dmac_alg_dbac_pause(pst_hal_device);
    }

    dmac_vap_pause_tx(pst_mac_vap1);
    dmac_vap_pause_tx(pst_mac_vap2);

    /* ���ڵ͹���״̬���Ѿ�����null֡��ֱ�ӷ��� */
    if(HAL_DEVICE_WORK_STATE == GET_HAL_DEVICE_STATE(pst_hal_device) &&
         HAL_DEVICE_WORK_SUB_STATE_ACTIVE != GET_WORK_SUB_STATE(pst_hal_device))
    {
        OAM_WARNING_LOG1(0, OAM_SF_M2S, "{dmac_m2s_switch_dbac_vaps_begin::powersave state[%d].}", GET_WORK_SUB_STATE(pst_hal_device));
        return;
    }

    pst_fcs_mgr = dmac_fcs_get_mgr_stru(pst_mac_device);
    pst_fcs_cfg = MAC_DEV_GET_FCS_CFG(pst_mac_device);
    OAL_MEMZERO(pst_fcs_cfg, OAL_SIZEOF(mac_fcs_cfg_stru));

    pst_fcs_cfg->st_dst_chl     = pst_hal_device->st_wifi_channel_status;
    pst_fcs_cfg->pst_hal_device = pst_hal_device;

    pst_fcs_cfg->pst_src_fake_queue = DMAC_VAP_GET_FAKEQ(pst_current_chan_vap);

    pst_fcs_cfg->st_src_chl = pst_hal_device->st_wifi_channel_status;
    dmac_fcs_prepare_one_packet_cfg(pst_current_chan_vap, &(pst_fcs_cfg->st_one_packet_cfg), pst_hal_device->st_hal_scan_params.us_scan_time);

    if (pst_hal_device->uc_current_chan_number != pst_current_chan_vap->st_channel.uc_chan_number)
    {
        OAM_WARNING_LOG2(0, OAM_SF_M2S, "{dmac_m2s_switch_dbac_vap_begin::switch dbac. hal chan num:%d, curr vap chan num:%d. not same,do not send protect frame}",
                        pst_hal_device->uc_current_chan_number, pst_current_chan_vap->st_channel.uc_chan_number);
        pst_fcs_cfg->st_one_packet_cfg.en_protect_type = HAL_FCS_PROTECT_TYPE_NONE;
    }

    dmac_fcs_start_same_channel(pst_fcs_mgr, pst_fcs_cfg, 0);
    mac_fcs_release(pst_fcs_mgr);
}
#endif

#ifdef _PRE_WLAN_FEATURE_DBDC

oal_uint32 dmac_dbdc_vap_reg_shift(dmac_device_stru *pst_dmac_device, mac_vap_stru *pst_shift_vap,
                        hal_to_dmac_device_stru  *pst_shift_hal_device, hal_to_dmac_vap_stru *pst_shift_hal_vap)
{
    dmac_vap_stru                *pst_dmac_vap;
    oal_uint8                     uc_cwmax;
    oal_uint8                     uc_cwmin;
    wlan_wme_ac_type_enum_uint8   en_ac_type;
    oal_uint32                    ul_aid_value;
    oal_uint32                    ul_tsf_valh;
    oal_uint32                    ul_tsf_vall;
    oal_uint32                    ul_led_tbtt_timer;
    oal_uint32                    ul_beacon_rate;

    pst_dmac_vap = MAC_GET_DMAC_VAP(pst_shift_vap);

#ifdef _PRE_WLAN_FEATURE_P2P
    /* P2P ����MAC ��ַ */
    if ((WLAN_P2P_DEV_MODE == pst_shift_vap->en_p2p_mode) && (WLAN_VAP_MODE_BSS_STA == pst_shift_vap->en_vap_mode))
    {
        /* ����p2p0 MAC ��ַ */
        hal_vap_set_macaddr(pst_shift_hal_vap, pst_shift_vap->pst_mib_info->st_wlan_mib_sta_config.auc_p2p0_dot11StationID);
    }
    else
    {
        /* ��������vap ��mac ��ַ */
        hal_vap_set_macaddr(pst_shift_hal_vap, mac_mib_get_StationID(pst_shift_vap));
    }
#else
    /* ����MAC��ַ */
    hal_vap_set_macaddr(pst_shift_hal_vap, mac_mib_get_StationID(pst_shift_vap));
#endif

    /* apģʽ�漰�Ĵ�������������Щ */
    if (WLAN_VAP_MODE_BSS_AP == pst_shift_vap->en_vap_mode)
    {
        /* ����MAC EIFS_TIME �Ĵ��� */
        hal_config_eifs_time(pst_shift_hal_device, pst_shift_vap->en_protocol);

        /* ʹ��PA_CONTROL��vap_controlλ */
        hal_vap_set_opmode(pst_shift_hal_vap, pst_shift_vap->en_vap_mode);

        /* ����beacon period */
        hal_vap_set_machw_beacon_period(pst_shift_hal_vap, (oal_uint16)mac_mib_get_BeaconPeriod(&pst_dmac_vap->st_vap_base_info));

        /* ����beacon�ķ��Ͳ��� */
        if ((WLAN_BAND_2G == pst_shift_vap->st_channel.en_band) || (WLAN_BAND_5G == pst_shift_vap->st_channel.en_band))
        {
            ul_beacon_rate = pst_dmac_vap->ast_tx_mgmt_ucast[pst_shift_vap->st_channel.en_band].ast_per_rate[0].ul_value;
            hal_vap_set_beacon_rate(pst_shift_hal_vap, ul_beacon_rate);
        }
        else
        {
            OAM_WARNING_LOG1(pst_shift_vap->uc_vap_id, OAM_SF_DBDC, "{dmac_dbdc_vap_reg_shift::en_band=%d!!!", pst_shift_vap->st_channel.en_band);
        }

#ifndef _PRE_WLAN_MAC_BUGFIX_MCAST_HW_Q
        /* ����APUT��tbtt offset */
        hal_set_psm_tbtt_offset(pst_shift_hal_vap, pst_dmac_vap->us_in_tbtt_offset);

        /* beacon���ͳ�ʱ���������鲥���� */
        hal_set_bcn_timeout_multi_q_enable(pst_shift_hal_vap, OAL_TRUE);
#endif
    }
    else if (WLAN_VAP_MODE_BSS_STA == pst_shift_vap->en_vap_mode)
    {
        /* 1��stautģʽ�Ĵ�������,�ֶ�������������Լ�����д��ļĴ��� */
        hal_set_sta_bssid(pst_shift_hal_vap, pst_shift_vap->auc_bssid);
        hal_vap_set_psm_beacon_period(pst_shift_hal_vap, mac_mib_get_BeaconPeriod(pst_shift_vap)); /* ����beacon���� */
        hal_set_beacon_timeout_val(pst_shift_hal_device, pst_dmac_vap->us_beacon_timeout); /* beacon��ʱ�ȴ�ֵ */
        //hal_init_pm_info(pst_shift_hal_vap);                        /* �͹������³�ʼ��offset�Լ���̬ѵ�����¿�ʼ */
        oal_memcopy(pst_shift_hal_vap->st_pm_info.ast_training_handle, pst_dmac_vap->pst_hal_vap->st_pm_info.ast_training_handle, OAL_SIZEOF(pst_shift_hal_vap->st_pm_info.ast_training_handle));
        pst_shift_hal_vap->st_pm_info.us_inner_tbtt_offset_mimo = pst_dmac_vap->pst_hal_vap->st_pm_info.us_inner_tbtt_offset_mimo;
        pst_shift_hal_vap->st_pm_info.us_inner_tbtt_offset_siso = pst_dmac_vap->pst_hal_vap->st_pm_info.us_inner_tbtt_offset_siso;
        hal_pm_set_tbtt_offset(pst_shift_hal_vap, 0);            /* ����tbtt offset */
        hal_disable_beacon_filter(pst_shift_hal_device);            /* �ر�beacon֡���� */
        hal_enable_non_frame_filter(pst_shift_hal_device);          /* ��non frame֡���� */
        hal_get_txop_ps_partial_aid(pst_dmac_vap->pst_hal_vap, &ul_aid_value);
        hal_set_txop_ps_partial_aid(pst_shift_hal_vap, ul_aid_value); /* ����mac partial aid�Ĵ��� */
        hal_set_mac_aid(pst_shift_hal_vap, pst_shift_vap->us_sta_aid);/* ����aid�Ĵ��� */
        //hal_machw_seq_num_index_update_per_tid(pst_shift_hal_device, pst_dmac_user->uc_lut_index, OAL_TRUE);*/
        hal_set_pmf_crypto(pst_shift_hal_vap, (oal_bool_enum_uint8)IS_OPEN_PMF_REG(pst_dmac_vap));

#ifdef _PRE_WLAN_MAC_BUGFIX_SW_CTRL_RSP
        hal_cfg_rsp_dyn_bw(OAL_TRUE, pst_dmac_device->en_usr_bw_mode);
#endif

        for (en_ac_type = 0; en_ac_type < WLAN_WME_AC_BUTT; en_ac_type++)
        {
            hal_vap_get_edca_machw_cw(pst_dmac_vap->pst_hal_vap, &uc_cwmax, &uc_cwmin, en_ac_type);
            hal_vap_set_edca_machw_cw(pst_shift_hal_vap, uc_cwmax, uc_cwmin, en_ac_type);
        }

        /* ����MAC EIFS_TIME �Ĵ��� */
        hal_config_eifs_time(pst_shift_hal_device, pst_shift_vap->en_protocol);

        /* ʹ��PA_CONTROL��vap_controlλ */
        hal_vap_set_opmode(pst_shift_hal_vap, pst_shift_vap->en_vap_mode);

#ifdef _PRE_WLAN_FEATURE_TXOPPS
        /* ��ʼ��TXOP PS */
        dmac_txopps_set_machw(pst_shift_vap);
#endif
        /* ����P2P noa���ܼĴ��� */
        if (IS_P2P_NOA_ENABLED(pst_dmac_vap))
        {
            hal_vap_set_noa(pst_dmac_vap->pst_hal_vap, 0, 0, 0, 0);
            hal_vap_set_noa(pst_shift_hal_vap,
                    pst_dmac_vap->st_p2p_noa_param.ul_start_time,
                    pst_dmac_vap->st_p2p_noa_param.ul_duration,
                    pst_dmac_vap->st_p2p_noa_param.ul_interval,
                    pst_dmac_vap->st_p2p_noa_param.uc_count);
        }

        /* ����P2P ops �Ĵ��� */
        if (IS_P2P_OPPPS_ENABLED(pst_dmac_vap))
        {
            hal_vap_set_ops(pst_dmac_vap->pst_hal_vap, 0, 0);
            hal_vap_set_ops(pst_shift_hal_vap, pst_dmac_vap->st_p2p_ops_param.en_ops_ctrl, pst_dmac_vap->st_p2p_ops_param.uc_ct_window);
        }
    }

    /* ����Ǩ�Ƶ�vap�����Ƿ�Ҫ��11b�����Ǩ�Ƶ���hal device */
    if (WLAN_BAND_2G == pst_shift_vap->st_channel.en_band)
    {
        hal_set_11b_reuse_sel(pst_shift_hal_device);
    }

    /* ��ȡ��pst_ori_hal_vap tsf */
    hal_vap_tsf_get_64bit(pst_dmac_vap->pst_hal_vap, &ul_tsf_valh, &ul_tsf_vall);

    /* ����pst_shift_hal_vap��tsf */
    hal_vap_tsf_set_64bit(pst_shift_hal_vap, ul_tsf_valh, ul_tsf_vall);

    hal_vap_read_tbtt_timer(pst_dmac_vap->pst_hal_vap, &ul_led_tbtt_timer);

    /* ����pst_shift_hal_vap��TBTT TIMER */
    hal_vap_write_tbtt_timer(pst_shift_hal_vap, ul_led_tbtt_timer);

    /* ����ʹ�� */
    hal_ce_enable_key(pst_shift_hal_device);

    /* ͬ����TSFʱ�����tsf */
    hal_disable_tsf_tbtt(pst_dmac_vap->pst_hal_vap);
    hal_enable_tsf_tbtt(pst_shift_hal_vap, OAL_FALSE);

    return OAL_SUCC;
}

oal_void  dmac_vap_clear_hal_device_statis(mac_vap_stru *pst_shift_vap)
{
    dmac_user_stru            *pst_dmac_user;
    hal_to_dmac_device_stru   *pst_hal_device;

    pst_hal_device = DMAC_VAP_GET_HAL_DEVICE(pst_shift_vap);
    if (OAL_PTR_NULL == pst_hal_device)
    {
        OAM_ERROR_LOG1(pst_shift_vap->uc_vap_id, OAM_SF_DBDC, "{dmac_vap_clear_hal_device_statis::vap[%d]get hal device null!}", pst_shift_vap->uc_vap_id);
        return;
    }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    if (WLAN_VAP_MODE_BSS_STA == pst_shift_vap->en_vap_mode)
    {
        pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user(pst_shift_vap->us_assoc_vap_id);
        if (OAL_PTR_NULL == pst_dmac_user)
        {
            OAM_WARNING_LOG1(pst_shift_vap->uc_vap_id, OAM_SF_DBDC,
                "{dmac_vap_clear_hal_device_statis::pst_dmac_user[%d] NULL.}", pst_shift_vap->us_assoc_vap_id);
            return;
        }

        dmac_full_phy_freq_user_del(pst_shift_vap, pst_dmac_user);
    }
    else if (WLAN_VAP_MODE_BSS_AP == pst_shift_vap->en_vap_mode)
    {
        dmac_full_phy_freq_user_del(pst_shift_vap, OAL_PTR_NULL);
    }
#endif

    if (pst_hal_device->uc_assoc_user_nums >= pst_shift_vap->us_user_nums)
    {
        pst_hal_device->uc_assoc_user_nums -=(oal_uint8)(pst_shift_vap->us_user_nums);
    }
    else
    {
        OAM_ERROR_LOG3(pst_shift_vap->uc_vap_id, OAM_SF_DBDC, "{dmac_vap_clear_hal_device_statis::hal device[%d],hal device asoc user[%d]<vap user[%d]!!!}",
                            pst_hal_device->uc_device_id, pst_hal_device->uc_assoc_user_nums, pst_shift_vap->us_user_nums);
    }

}

oal_void  dmac_vap_add_hal_device_statis(mac_vap_stru *pst_shift_vap)
{
    dmac_user_stru            *pst_dmac_user;
    hal_to_dmac_device_stru   *pst_hal_device;

    pst_hal_device = DMAC_VAP_GET_HAL_DEVICE(pst_shift_vap);
    if (OAL_PTR_NULL == pst_hal_device)
    {
        OAM_ERROR_LOG1(pst_shift_vap->uc_vap_id, OAM_SF_DBDC, "{dmac_vap_add_hal_device_statis::vap[%d]get hal device null!}", pst_shift_vap->uc_vap_id);
        return;
    }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    if (WLAN_VAP_MODE_BSS_STA == pst_shift_vap->en_vap_mode)
    {
        pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user(pst_shift_vap->us_assoc_vap_id);
        if (OAL_PTR_NULL == pst_dmac_user)
        {
            OAM_WARNING_LOG1(pst_shift_vap->uc_vap_id, OAM_SF_DBDC,
                "{dmac_vap_add_hal_device_statis::pst_dmac_user[%d] NULL.}", pst_shift_vap->us_assoc_vap_id);
            return;
        }

        dmac_full_phy_freq_user_add(pst_shift_vap, pst_dmac_user);
    }
    else if (WLAN_VAP_MODE_BSS_AP == pst_shift_vap->en_vap_mode)
    {
        dmac_full_phy_freq_user_add(pst_shift_vap, OAL_PTR_NULL);
    }
#endif

    pst_hal_device->uc_assoc_user_nums +=(oal_uint8)(pst_shift_vap->us_user_nums);

}


oal_uint32 dmac_dbdc_vap_hal_device_shift(dmac_device_stru  *pst_dmac_device, mac_vap_stru *pst_shift_vap)
{
    hal_to_dmac_device_stru      *pst_ori_hal_device;
    hal_to_dmac_device_stru      *pst_shift_hal_device;
    hal_to_dmac_vap_stru         *pst_ori_hal_vap;
    hal_to_dmac_vap_stru         *pst_shift_hal_vap;

    pst_ori_hal_device   = DMAC_VAP_GET_HAL_DEVICE(pst_shift_vap);
    pst_shift_hal_device = dmac_device_get_another_h2d_dev(pst_dmac_device, pst_ori_hal_device);
    if (OAL_PTR_NULL  == pst_shift_hal_device)
    {
        OAM_ERROR_LOG2(pst_shift_vap->uc_vap_id, OAM_SF_DBDC, "dmac_dbdc_vap_hal_device_shift::pst_shift_hal_device NULL, chip id[%d],ori hal device id[%d]",
                        pst_shift_vap->uc_chip_id, pst_ori_hal_device->uc_device_id);

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_ori_hal_vap      = DMAC_VAP_GET_HAL_VAP(pst_shift_vap);
    hal_get_hal_vap(pst_shift_hal_device, pst_ori_hal_vap->uc_vap_id, &pst_shift_hal_vap);

    /* 1��ԭhal device�ϵ�ͳ�Ʊ������ */
    dmac_vap_clear_hal_device_statis(pst_shift_vap);

    /* 2���Ĵ���Ǩ�� */
    dmac_dbdc_vap_reg_shift(pst_dmac_device, pst_shift_vap, pst_shift_hal_device, pst_shift_hal_vap);

    /* 3��hal device,hal vapָ���滻 */
    DMAC_VAP_GET_HAL_DEVICE(pst_shift_vap) = pst_shift_hal_device;
    DMAC_VAP_GET_HAL_VAP(pst_shift_vap)    = pst_shift_hal_vap;

    /* 4�����µ�hal device��ע��ͳ�� */
    dmac_vap_add_hal_device_statis(pst_shift_vap);

    /* 5����ԭ��hal device״̬����ȥע�� */
    hal_device_handle_event(pst_ori_hal_device, HAL_DEVICE_EVENT_VAP_DOWN, OAL_SIZEOF(hal_to_dmac_vap_stru), (oal_uint8 *)pst_ori_hal_vap);

    /* 6��֪ͨ�㷨,�㷨��Ǩ�������Ƿ����? */
    dmac_pow_set_vap_tx_power(pst_shift_vap, HAL_POW_SET_TYPE_INIT);
    dmac_alg_cfg_channel_notify(pst_shift_vap, CH_BW_CHG_TYPE_MOVE_WORK);
    dmac_alg_cfg_bandwidth_notify(pst_shift_vap, CH_BW_CHG_TYPE_MOVE_WORK);

    /* 7��ע�ᵽǨ�Ƶ���hal device��ȥ */
    if (WLAN_VAP_MODE_BSS_STA == pst_shift_vap->en_vap_mode)
    {
        hal_device_handle_event(pst_shift_hal_device, HAL_DEVICE_EVENT_JOIN_COMP, OAL_SIZEOF(hal_to_dmac_vap_stru), (oal_uint8 *)pst_shift_hal_vap);
    }
    hal_device_handle_event(pst_shift_hal_device, HAL_DEVICE_EVENT_VAP_UP, OAL_SIZEOF(hal_to_dmac_vap_stru), (oal_uint8 *)pst_shift_hal_vap);

    /* 8�����µ�hal device��������ҪǨ�Ƶ�vap��channel,new hal device��ֻ��һ��up vap,���򲻷���Ŀǰ������ */
    if (hal_device_calc_up_vap_num(pst_shift_hal_device) != 1)
    {
        OAM_ERROR_LOG2(pst_shift_vap->uc_vap_id, OAM_SF_DBDC, "dmac_dbdc_vap_hal_device_shift::device id[%d],work bitmap[0x%x] up vap > 1!!!",
                        pst_shift_hal_device->uc_device_id, pst_shift_hal_device->ul_work_vap_bitmap);
        return OAL_FAIL;
    }

    dmac_mgmt_switch_channel(pst_shift_hal_device, &(pst_shift_vap->st_channel), OAL_TRUE);

    return OAL_SUCC;
}


oal_void dmac_dbdc_switch_vaps_begin(hal_to_dmac_device_stru *pst_hal_device,
                     mac_device_stru  *pst_mac_device, mac_vap_stru  *pst_mac_vap1, mac_vap_stru *pst_mac_vap2)
{
    mac_vap_stru                   *pst_up_vap;

    if (pst_hal_device->uc_current_chan_number == pst_mac_vap1->st_channel.uc_chan_number)
    {
        pst_up_vap    = pst_mac_vap1;
    }
    else
    {
        pst_up_vap    = pst_mac_vap2;
    }

    OAM_WARNING_LOG2(pst_up_vap->uc_vap_id, OAM_SF_DBDC, "dmac_dbdc_switch_vaps_begin::device id[%d],work bitmap[0x%x]",
                    pst_up_vap->uc_device_id, pst_hal_device->ul_work_vap_bitmap);

    /* up��vap��Ҫpause�����뱣�����Ƶ��Լ�����ٶ��� */
    dmac_m2s_switch_vap_off(pst_hal_device, pst_mac_device, pst_up_vap);
}

oal_void dmac_dbdc_switch_vap_back(mac_device_stru *pst_mac_device, mac_vap_stru *pst_mac_vap)
{
    hal_to_dmac_device_stru        *pst_hal_device;

    pst_hal_device = DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap);

    /*****************************************************************************
           hal device��vap�ָ���ٶ��еİ���Ӳ������,���������ŵ����ָ�����
    *****************************************************************************/
    OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_DBDC, "{dmac_dbdc_switch_vap_back::hal device id[%d],current channel[%d]vap[%d]channel[%d]}",
                        pst_hal_device->uc_device_id, pst_hal_device->uc_current_chan_number, pst_mac_vap->uc_vap_id,pst_mac_vap->st_channel.uc_chan_number);

    /********************************************************************************************************
        dbdcǨ�ƽ����жϴ�hal device���ŵ��빤����vap�Ƿ�һ��,��ͳһ��������,�����ٿ��ܷ���,lzhqi todo
        tx power��init ����REFRESH?
    *********************************************************************************************************/
    //if (pst_hal_device->uc_current_chan_number != pst_mac_vap->st_channel.uc_chan_number)
    {
        /* ֪ͨ�㷨 */
        dmac_pow_set_vap_tx_power(pst_mac_vap, HAL_POW_SET_TYPE_INIT);
        dmac_alg_cfg_channel_notify(pst_mac_vap, CH_BW_CHG_TYPE_MOVE_WORK);
        dmac_alg_cfg_bandwidth_notify(pst_mac_vap, CH_BW_CHG_TYPE_MOVE_WORK);

        /* ���ŵ� */
        dmac_mgmt_switch_channel(pst_hal_device, &(pst_mac_vap->st_channel), OAL_TRUE);
    }

    /* �ָ�home�ŵ��ϱ���ͣ�ķ��� */
    dmac_vap_resume_tx_by_chl(pst_mac_device, pst_hal_device, &(pst_hal_device->st_wifi_channel_status));
}

oal_void  dmac_dbdc_switch_vaps_back(mac_device_stru *pst_mac_device, mac_vap_stru *pst_keep_vap, mac_vap_stru *pst_shift_vap)
{
    /*****************************************************************************
                   ��Ǩ��hal device��vap,Ǩ�ƽ�������(2G)
    *****************************************************************************/
    dmac_dbdc_switch_vap_back(pst_mac_device, pst_shift_vap);

    /*****************************************************************************
        ������ԭ��hal device��vap�ָ���ٶ��еİ���Ӳ������,���������ŵ�(5G)
    *****************************************************************************/
    dmac_dbdc_switch_vap_back(pst_mac_device, pst_keep_vap);
}

oal_void dmac_dbdc_down_vap_back_to_master(mac_vap_stru  *pst_down_vap)
{
    hal_to_dmac_device_stru   *pst_slave_hal_device;
    hal_to_dmac_device_stru   *pst_master_hal_device;
    hal_to_dmac_vap_stru      *pst_ori_hal_vap;
    hal_to_dmac_vap_stru      *pst_shift_hal_vap;
    dmac_device_stru          *pst_dmac_device;
    dmac_vap_stru             *pst_dmac_vap = MAC_GET_DMAC_VAP(pst_down_vap);

    pst_dmac_device      = dmac_res_get_mac_dev(pst_down_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_dmac_device)
    {
        OAM_ERROR_LOG0(pst_down_vap->uc_device_id, OAM_SF_DBDC, "{dmac_dbdc_down_vap_back_to_master::pst_dmac_device null.}");
        return;
    }
    pst_slave_hal_device   = DMAC_VAP_GET_HAL_DEVICE(pst_down_vap);
    pst_master_hal_device = dmac_device_get_another_h2d_dev(pst_dmac_device, pst_slave_hal_device);
    if ((OAL_PTR_NULL == pst_master_hal_device) || (OAL_FALSE == (oal_uint8)HAL_CHECK_DEV_IS_MASTER(pst_master_hal_device)))
    {
        OAM_ERROR_LOG2(pst_down_vap->uc_vap_id, OAM_SF_DBDC, "dmac_dbdc_down_vap_back_to_master::master dev NULL or not master,chip id[%d],slave id[%d]",
                        pst_down_vap->uc_chip_id, pst_slave_hal_device->uc_device_id);
        return;
    }

    /* add vapʱ�ҽӵ��㷨���ٴλص���· */
    dmac_alg_delete_vap_notify(pst_dmac_vap);

    pst_ori_hal_vap      = DMAC_VAP_GET_HAL_VAP(pst_down_vap);
    hal_get_hal_vap(pst_master_hal_device, pst_ori_hal_vap->uc_vap_id, &pst_shift_hal_vap);

    /* �滻hal device,hal vapָ���㷨����ҪǨ�ƻ�ȥ */
    pst_dmac_vap->pst_hal_device = pst_master_hal_device;
    pst_dmac_vap->pst_hal_vap    = pst_shift_hal_vap;

    dmac_alg_create_vap_notify(pst_dmac_vap);

    OAM_WARNING_LOG4(pst_down_vap->uc_vap_id, OAM_SF_DBDC, "dmac_dbdc_down_vap_back_to_master::down vap state[%d]master dev bitmap[0x%x]slave dev[%d]workbimap[0x%x]",
               pst_down_vap->en_vap_state, pst_master_hal_device->ul_work_vap_bitmap,
               pst_slave_hal_device->uc_device_id, pst_slave_hal_device->ul_work_vap_bitmap);

}

oal_void dmac_dbdc_renew_pm_tbtt_offset(dmac_dbdc_state_enum_uint8 en_dbdc_state)
{
    if (DMAC_DBDC_START == en_dbdc_state)
    {
        g_us_ext_inner_offset_diff     += HI1103_DBDC_EXT_INNER_OFFSET_DIFF;
        g_us_inner_tbtt_offset_mimo    += HI1103_DBDC_INNER_TBTT_OFFSET_RESV_DIFF;
        g_us_inner_tbtt_offset_siso    += HI1103_DBDC_INNER_TBTT_OFFSET_RESV_DIFF;
    #ifdef _PRE_PM_DYN_SET_TBTT_OFFSET
        g_us_inner_tbtt_offset_resv    += HI1103_DBDC_INNER_TBTT_OFFSET_RESV_DIFF;
    #endif
    }
    else if (DMAC_DBDC_STOP == en_dbdc_state)
    {
        /*���ⲿtbtt offset�ָ�����ֵ*/
        g_us_ext_inner_offset_diff      = HI1103_EXT_INNER_OFFSET_DIFF;
        g_us_inner_tbtt_offset_mimo     = PM_MIMO_STA_INNER_TBTT_OFFSET;
        g_us_inner_tbtt_offset_siso     = PM_SISO_STA_INNER_TBTT_OFFSET;
    #ifdef _PRE_PM_DYN_SET_TBTT_OFFSET
        g_us_inner_tbtt_offset_resv     = HI1103_INNER_TBTT_OFFSET_RESV;
    #endif
    }
}

oal_void dmac_dbdc_switch_device_end(mac_device_stru *pst_mac_device, hal_to_dmac_device_stru   *pst_hal_device)
{
    oal_uint8                       uc_up_vap_num = 0;
    oal_uint8                       uc_vap_index  = 0;
    oal_uint8                       auc_mac_vap_id[WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE] = {0};
    mac_vap_stru                   *pst_mac_vap;

    uc_up_vap_num = hal_device_find_all_up_vap(pst_hal_device, auc_mac_vap_id);
    for (uc_vap_index = 0; uc_vap_index < uc_up_vap_num; uc_vap_index++)
    {
        pst_mac_vap  = (mac_vap_stru *)mac_res_get_mac_vap(auc_mac_vap_id[uc_vap_index]);
        if (OAL_PTR_NULL == pst_mac_vap)
        {
            OAM_ERROR_LOG1(0, OAM_SF_DBDC, "dmac_dbdc_switch_device_end::pst_mac_vap[%d] IS NULL.", auc_mac_vap_id[uc_vap_index]);
            continue;
        }

        dmac_dbdc_switch_vap_back(pst_mac_device, pst_mac_vap);
    }
}

oal_void dmac_dbdc_start_renew_dev(hal_to_dmac_device_stru  *pst_hal_device)
{
    oal_uint8                uc_up_vap_num;
    oal_uint8                uc_mac_vap_id;
    mac_vap_stru            *pst_mac_vap;
    hal_to_dmac_vap_stru    *pst_hal_vap;

    uc_up_vap_num = hal_device_calc_up_vap_num(pst_hal_device);
    if (uc_up_vap_num != 1)
    {
        OAM_ERROR_LOG1(0, OAM_SF_DBDC, "{dmac_dbdc_start_renew_dev::master hal dev up vap num[%d]!=1!!!}", uc_up_vap_num);
    }

    if (OAL_SUCC == hal_device_find_one_up_vap(pst_hal_device, &uc_mac_vap_id))
    {
        pst_mac_vap  = (mac_vap_stru *)mac_res_get_mac_vap(uc_mac_vap_id);
        if (OAL_PTR_NULL == pst_mac_vap)
        {
            OAM_ERROR_LOG1(0, OAM_SF_DBDC, "{dmac_dbdc_start_renew_dev::mac_res_get_mac_vap[%d]null}", uc_mac_vap_id);
            return;
        }

        if (WLAN_VAP_MODE_BSS_STA == pst_mac_vap->en_vap_mode)
        {
            pst_hal_vap = DMAC_VAP_GET_HAL_VAP(pst_mac_vap);
            pst_hal_vap->st_pm_info.us_inner_tbtt_offset_mimo += HI1103_DBDC_INNER_TBTT_OFFSET_RESV_DIFF;
            pst_hal_vap->st_pm_info.us_inner_tbtt_offset_siso += HI1103_DBDC_INNER_TBTT_OFFSET_RESV_DIFF;

            OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_DBDC, "{dmac_dbdc_start_renew_dev::inner mimo tbtt offset[%d], inner siso tbtt offset[%d]}",
                            pst_hal_vap->st_pm_info.us_inner_tbtt_offset_mimo, pst_hal_vap->st_pm_info.us_inner_tbtt_offset_siso);

            hal_pm_set_tbtt_offset(pst_hal_vap, 0);
        }
    }
}

OAL_STATIC oal_void dmac_dbdc_stop_renew_dev(hal_to_dmac_device_stru  *pst_hal_device)
{
    oal_uint8                uc_up_vap_num;
    oal_uint8                uc_mac_vap_id;
    mac_vap_stru            *pst_mac_vap;
    hal_to_dmac_vap_stru    *pst_hal_vap;

    /* ��ʱ��·������һ������û��up vap���� */
    uc_up_vap_num = hal_device_calc_up_vap_num(pst_hal_device);
    if (uc_up_vap_num > 1)
    {
        OAM_ERROR_LOG1(0, OAM_SF_DBDC, "{dmac_dbdc_stop_renew_dev::master hal dev up vap num[%d] > 1!!!}", uc_up_vap_num);
    }

    if ((1 == uc_up_vap_num) && (OAL_SUCC == hal_device_find_one_up_vap(pst_hal_device, &uc_mac_vap_id)))
    {
        pst_mac_vap  = (mac_vap_stru *)mac_res_get_mac_vap(uc_mac_vap_id);
        if (OAL_PTR_NULL == pst_mac_vap)
        {
            OAM_ERROR_LOG1(0, OAM_SF_DBDC, "{dmac_dbdc_stop_renew_dev::mac_res_get_mac_vap[%d]null}", uc_mac_vap_id);
            return;
        }

        if (pst_mac_vap->en_vap_mode != WLAN_VAP_MODE_BSS_STA)
        {
            return;
        }

        pst_hal_vap = DMAC_VAP_GET_HAL_VAP(pst_mac_vap);
        if ((pst_hal_vap->st_pm_info.us_inner_tbtt_offset_mimo <= HI1103_DBDC_INNER_TBTT_OFFSET_RESV_DIFF) ||
                    (pst_hal_vap->st_pm_info.us_inner_tbtt_offset_siso <= HI1103_DBDC_INNER_TBTT_OFFSET_RESV_DIFF))
        {
            OAM_ERROR_LOG2(uc_mac_vap_id, OAM_SF_DBDC, "{dmac_dbdc_stop_renew_dev::inner mimo tbtt offset[%d],siso tbtt offset[%d],too short!!!}",
                                pst_hal_vap->st_pm_info.us_inner_tbtt_offset_mimo, pst_hal_vap->st_pm_info.us_inner_tbtt_offset_siso);
            return;
        }
        pst_hal_vap->st_pm_info.us_inner_tbtt_offset_mimo -= HI1103_DBDC_INNER_TBTT_OFFSET_RESV_DIFF;
        pst_hal_vap->st_pm_info.us_inner_tbtt_offset_siso -= HI1103_DBDC_INNER_TBTT_OFFSET_RESV_DIFF;

        OAM_WARNING_LOG2(uc_mac_vap_id, OAM_SF_DBDC, "{dmac_dbdc_stop_renew_dev::inner mimo tbtt offset[%d], inner siso tbtt offset[%d]}",
                        pst_hal_vap->st_pm_info.us_inner_tbtt_offset_mimo, pst_hal_vap->st_pm_info.us_inner_tbtt_offset_siso);

        hal_pm_set_tbtt_offset(pst_hal_vap, 0);
    }
}

OAL_STATIC oal_void dmac_dbdc_handle_stop_in_master(dmac_device_stru *pst_dmac_device, hal_to_dmac_device_stru *pst_master_hal_device)
{
    oal_uint8                      uc_up_vap_num;
    oal_uint8                      uc_mac_vap_id = 0;
    hal_to_dmac_device_stru       *pst_slave_hal_device;
    mac_vap_stru                  *pst_shift_mac_vap = OAL_PTR_NULL;

    if (!HAL_CHECK_DEV_IS_MASTER(pst_master_hal_device))
    {
        OAM_ERROR_LOG1(0, OAM_SF_DBDC, "{dmac_dbdc_handle_stop_in_master::dev[%d],not in mastet hal dev!!!}", pst_master_hal_device->uc_device_id);
        return;
    }

    /* ��·��ȥ����,��·����Ҫ�л���·Ȼ�����е�mimo */
    pst_slave_hal_device = dmac_device_get_another_h2d_dev(pst_dmac_device, pst_master_hal_device);
    uc_up_vap_num = hal_device_calc_up_vap_num(pst_slave_hal_device);
    if (uc_up_vap_num != 1)
    {
        OAM_ERROR_LOG3(0, OAM_SF_DBDC, "{dmac_dbdc_handle_stop_in_master::slave dev[%d]up vap[%d]!=1,bitmap[0x%x]}",
                            pst_slave_hal_device->uc_device_id, uc_up_vap_num, pst_slave_hal_device->ul_work_vap_bitmap);
        return;
    }

    if (OAL_SUCC == hal_device_find_one_up_vap(pst_slave_hal_device, &uc_mac_vap_id))
    {
        pst_shift_mac_vap  = (mac_vap_stru *)mac_res_get_mac_vap(uc_mac_vap_id);
    }

    if (OAL_PTR_NULL == pst_shift_mac_vap)
    {
        OAM_ERROR_LOG1(uc_mac_vap_id, OAM_SF_DBDC, "{dmac_dbdc_handle_stop_in_master::pst_shift_mac_vap[%d] null}", uc_mac_vap_id);
        return;
    }

    dmac_up_vap_change_hal_dev(pst_shift_mac_vap);

    OAM_WARNING_LOG2(pst_shift_mac_vap->uc_vap_id, OAM_SF_DBDC, "{dmac_dbdc_handle_stop_in_master::slave workbitmap[0x%x]master work_bitmap[0x%x]}",
        pst_slave_hal_device->ul_work_vap_bitmap, pst_master_hal_device->ul_work_vap_bitmap);
}

oal_void dmac_dbdc_handle_stop_event(hal_to_dmac_device_stru *pst_hal_device)
{
    oal_uint8                uc_vap_idx      = 0;
    mac_device_stru         *pst_mac_device  = OAL_PTR_NULL;
    dmac_device_stru        *pst_dmac_device = OAL_PTR_NULL;
    mac_vap_stru            *pst_mac_vap     = OAL_PTR_NULL;

    pst_mac_device = mac_res_get_dev(pst_hal_device->uc_mac_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
       OAM_ERROR_LOG1(0, OAM_SF_DBDC, "{dmac_dbdc_handle_stop_event: mac device is null ptr. device id:%d}", pst_hal_device->uc_mac_device_id);
       return;
    }

    pst_dmac_device = dmac_res_get_mac_dev(pst_mac_device->uc_device_id);
    if (OAL_PTR_NULL == pst_dmac_device)
    {
        OAM_ERROR_LOG0(pst_mac_device->uc_device_id, OAM_SF_DBDC, "{dmac_dbdc_handle_siso_event::pst_dmac_device null.}");
        return;
    }

    dmac_dbdc_renew_pm_tbtt_offset(DMAC_DBDC_STOP);

    /* ���11b��Ҫ�������· */
    hal_set_11b_reuse_sel(DMAC_DEV_GET_MST_HAL_DEV(pst_dmac_device));
    dmac_dbdc_stop_renew_dev(DMAC_DEV_GET_MST_HAL_DEV(pst_dmac_device));
    dmac_dbdc_stop_renew_dev(DMAC_DEV_GET_SLA_HAL_DEV(pst_dmac_device));

    /* ��·��ȥ����,��·��VAP��Ҫ�л���·�����л�mimo */
    if (HAL_CHECK_DEV_IS_MASTER(pst_hal_device))
    {
        dmac_dbdc_handle_stop_in_master(pst_dmac_device, pst_hal_device);

        return;
    }

    /* ��·ȥ������Ҫ����·�ϵ�vap�л���·,��·��siso->mimo */
    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_mac_vap)
        {
            OAM_ERROR_LOG0(pst_mac_device->auc_vap_id[uc_vap_idx], OAM_SF_DBDC, "{dmac_dbdc_handle_siso_event::mac vap is null!}");
            continue;
        }

        /* ��·��vap��Ҫ�л���· */
        if(pst_hal_device != MAC_GET_DMAC_VAP(pst_mac_vap)->pst_hal_device)
        {
            continue;
        }

        dmac_dbdc_down_vap_back_to_master(pst_mac_vap);
    }
    OAM_WARNING_LOG2(0, OAM_SF_DBDC, "{dmac_dbdc_handle_stop_event::vap down in dev[%d],master work_bitmap[0x%x]}",
        pst_hal_device->uc_device_id, DMAC_DEV_GET_MST_HAL_DEV(pst_dmac_device)->ul_work_vap_bitmap);
}


oal_void dmac_dbdc_switch_vap_to_slave(dmac_device_stru  *pst_dmac_device, mac_vap_stru *pst_shift_mac_vap)
{
    // TODO: ����״̬��dbdc,���¿�ʼ��before up�ӿ��Ƿ�ok?
    if ((MAC_VAP_STATE_UP == pst_shift_mac_vap->en_vap_state) || (MAC_VAP_STATE_PAUSE == pst_shift_mac_vap->en_vap_state))
    {
        dmac_up_vap_change_hal_dev(pst_shift_mac_vap);
    }
    else
    {
        dmac_vap_change_hal_dev_before_up(pst_shift_mac_vap, pst_dmac_device);
    }
}
#endif


oal_uint32  dmac_m2s_switch_vap_off(hal_to_dmac_device_stru *pst_hal_device,
    mac_device_stru *pst_mac_device, mac_vap_stru  *pst_mac_vap)
{
    mac_fcs_mgr_stru                    *pst_fcs_mgr;
    mac_fcs_cfg_stru                    *pst_fcs_cfg;
    hal_one_packet_status_stru           st_status;

    if (MAC_VAP_STATE_UP == pst_mac_vap->en_vap_state)
    {
        /* ��ͣvapҵ�� */
        dmac_vap_pause_tx(pst_mac_vap);
    }

    /* �ⲿ��֤��β�Ϊ�� */
    pst_fcs_mgr = dmac_fcs_get_mgr_stru(pst_mac_device);
    pst_fcs_cfg = MAC_DEV_GET_FCS_CFG(pst_mac_device);

    OAL_MEMZERO(pst_fcs_cfg, OAL_SIZEOF(mac_fcs_cfg_stru));

    pst_fcs_cfg->st_src_chl = pst_mac_vap->st_channel;
    pst_fcs_cfg->st_dst_chl = pst_mac_vap->st_channel;
    pst_fcs_cfg->pst_hal_device = pst_hal_device;

    pst_fcs_cfg->pst_src_fake_queue = DMAC_VAP_GET_FAKEQ(pst_mac_vap);

    /* ���ڵ͹���״̬���Ѿ�����null֡��ֱ�ӷ��� */
    if(HAL_DEVICE_WORK_STATE == GET_HAL_DEVICE_STATE(pst_hal_device) &&
         HAL_DEVICE_WORK_SUB_STATE_ACTIVE != GET_WORK_SUB_STATE(pst_hal_device))
    {
        OAM_INFO_LOG1(0, OAM_SF_M2S, "{dmac_m2s_switch_vap_off::powersave state[%d].}", GET_WORK_SUB_STATE(pst_hal_device));
        return OAL_SUCC;
    }

    dmac_fcs_prepare_one_packet_cfg(pst_mac_vap, &(pst_fcs_cfg->st_one_packet_cfg), pst_hal_device->st_hal_scan_params.us_scan_time);

    /* ����FCS���ŵ��ӿ�,���浱ǰӲ�����е�֡��ɨ����ٶ��� */
    dmac_fcs_start_same_channel(pst_fcs_mgr, pst_fcs_cfg, &st_status);
    mac_fcs_release(pst_fcs_mgr);

    if (HAL_FCS_PROTECT_TYPE_NULL_DATA == pst_fcs_cfg->st_one_packet_cfg.en_protect_type  && !st_status.en_null_data_success)
    {
        OAM_WARNING_LOG1(0, OAM_SF_M2S, "{dmac_m2s_switch_vap_off::null data failed, sending chan:%d}",
                         pst_fcs_cfg->st_src_chl.uc_chan_number);
    }

    return OAL_SUCC;
}


oal_uint32 dmac_m2s_switch_device_begin(mac_device_stru *pst_mac_device, hal_to_dmac_device_stru *pst_hal_device)
{
    oal_uint8                    uc_vap_idx;
    oal_uint8                    uc_up_vap_num;
    oal_uint8                    auc_mac_vap_id[WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE];
    mac_vap_stru                *pst_mac_vap[WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE] = {OAL_PTR_NULL};

    /* Hal Device����work״̬����Ҫȥ���up vap���� */
    uc_up_vap_num = hal_device_find_all_up_vap(pst_hal_device, auc_mac_vap_id);
    for (uc_vap_idx = 0; uc_vap_idx < uc_up_vap_num; uc_vap_idx++)
    {
        pst_mac_vap[uc_vap_idx]  = (mac_vap_stru *)mac_res_get_mac_vap(auc_mac_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_mac_vap[uc_vap_idx])
        {
            OAM_ERROR_LOG1(0, OAM_SF_M2S, "{dmac_m2s_switch_device_begin::pst_mac_vap[%d] IS NULL.}", auc_mac_vap_id[uc_vap_idx]);
            return OAL_ERR_CODE_PTR_NULL;
        }
    }

    if (0 == uc_up_vap_num)
    {
        /* û��work��vap����ʾ���ڴ���ɨ��״̬��ǰ���Ѿ���ɨ��abort�ˣ�������idle״̬������ֱ�����л�������Ҫvap pause���� */
    }
    else if (1 == uc_up_vap_num)
    {
        /* vapΪm2s�л���׼�� */
        dmac_m2s_switch_vap_off(pst_hal_device, pst_mac_device, pst_mac_vap[0]);
    }
    else if (2 == uc_up_vap_num)
    {
        /* vapΪm2s�л���׼�� */
        if (pst_mac_vap[0]->st_channel.uc_chan_number != pst_mac_vap[1]->st_channel.uc_chan_number)
        {
            if (OAL_TRUE == mac_is_dbac_running(pst_mac_device))
            {
                dmac_m2s_switch_dbac_vaps_begin(pst_hal_device, pst_mac_device, pst_mac_vap[0], pst_mac_vap[1]);
            }
            else
            {
                dmac_dbdc_switch_vaps_begin(pst_hal_device, pst_mac_device, pst_mac_vap[0], pst_mac_vap[1]);
            }
        }
        else
        {
            dmac_m2s_switch_same_channel_vaps_begin(pst_hal_device, pst_mac_device, pst_mac_vap[0], pst_mac_vap[1]);
        }
    }
    else
    {
        /* m2s���������������51��̬MIMO����ʱ�����Ǵ˽ӿ� */
        OAM_ERROR_LOG0(0, OAM_SF_M2S, "{dmac_m2s_switch_device_begin: cannot support 3 and more vaps!}");
        return OAL_FAIL;
    }

    return OAL_SUCC;
}


oal_void  dmac_m2s_switch_device_end(hal_to_dmac_device_stru *pst_hal_device)
{
    oal_uint8           uc_up_vap_num;
    mac_device_stru    *pst_mac_device;

    pst_mac_device = mac_res_get_dev(pst_hal_device->uc_mac_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_WARNING_LOG1(0, OAM_SF_M2S, "{dmac_m2s_switch_device_end::pst_mac_device[%d] is null.}", pst_hal_device->uc_mac_device_id);
        return;
    }

    /* Hal Device����work״̬����Ҫȥ���up vap���� */
    uc_up_vap_num = hal_device_calc_up_vap_num(pst_hal_device);
    if (0 == uc_up_vap_num)
    {
        /* ����ɨ��״ִ̬���л���û��ִ��pause���˴�Ҳ����Ҫ�ָ����ͣ�ֱ�ӷ��ؼ��� */
        return;
    }

    if (mac_is_dbac_running(pst_mac_device))
    {
        /* dbac����ֻ��ָ�dbac����dbac�����е������ŵ� */
        dmac_alg_dbac_resume(pst_hal_device, OAL_TRUE);
        return;
    }

    /* �ָ�home�ŵ��ϱ���ͣ�ķ��� */
    dmac_vap_resume_tx_by_chl(pst_mac_device, pst_hal_device, &(pst_hal_device->st_wifi_channel_status));
}


oal_void dmac_m2s_nss_and_bw_alg_notify(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user,
     oal_bool_enum_uint8 en_nss_change, oal_bool_enum_uint8 en_bw_change)
{
    hal_to_dmac_device_stru    *pst_hal_device;

    /* �������nss��û�б仯������Ҫˢ��Ӳ�� */
    if(OAL_FALSE == en_nss_change && OAL_FALSE == en_bw_change)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_M2S, "{dmac_m2s_nss_and_bw_alg_notify: nss and bw is not change.}");
        return;
    }

    pst_hal_device = DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap);
    if (OAL_PTR_NULL == pst_hal_device)
    {
       OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_M2S, "{dmac_m2s_nss_and_bw_alg_notify: pst_mac_device is null ptr.}");
       return;
    }


    /*1.���ռ����������ͱ仯��������㷨���Ӻ���,�������Ϳռ���ͬʱ�ı䣬Ҫ�ȵ��ÿռ������㷨����*/
    if(OAL_TRUE == en_nss_change)
    {
        dmac_alg_cfg_user_spatial_stream_notify(pst_mac_user);
    }

    /* 2.opmode����ı�֪ͨ�㷨,��ͬ��������Ϣ��HOST */
    if(OAL_TRUE == en_bw_change)
    {
        /* �����㷨�ı����֪ͨ�� */
        dmac_alg_cfg_user_bandwidth_notify(pst_mac_vap, pst_mac_user);
    }

    /* user����ͬ����hmac */
    if (OAL_SUCC != dmac_config_d2h_user_m2s_info_syn(pst_mac_vap, pst_mac_user))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_M2S,
                  "{dmac_m2s_nss_and_bw_alg_notify::dmac_config_d2h_user_m2s_info_syn failed.}");
    }

    OAM_WARNING_LOG2(0, OAM_SF_M2S, "{dmac_m2s_nss_and_bw_alg_notify: nss[%d]bw[%d] finish.}",
        en_nss_change, en_bw_change);
}


oal_uint32  dmac_m2s_switch(hal_to_dmac_device_stru       *pst_hal_device,
                                             wlan_nss_enum_uint8 en_nss,
                                             hal_m2s_event_tpye_uint16 en_m2s_event,
                                             oal_bool_enum_uint8   en_hw_switch)
{
    mac_device_stru               *pst_mac_device;
    oal_uint32                     ul_ret;

    if(WLAN_MAX_NSS_NUM < en_nss)
    {
        OAM_WARNING_LOG1(0, OAM_SF_M2S, "{dmac_m2s_switch::nss[%d] cannot support!.}", en_nss);
        return OAL_ERR_CODE_CONFIG_UNSUPPORT;
    }

    pst_mac_device = mac_res_get_dev(pst_hal_device->uc_mac_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
       OAM_ERROR_LOG1(0, OAM_SF_M2S, "{dmac_m2s_switch: mac device is null ptr. device id:%d}", pst_hal_device->uc_mac_device_id);
       return OAL_ERR_CODE_PTR_NULL;
    }

    /* �л�ֻ������·ִ�� */
    if (HAL_DEVICE_ID_MASTER != pst_hal_device->uc_device_id)
    {
        OAM_ERROR_LOG1(0, OAM_SF_M2S, "{dmac_m2s_switch::current hal device id[%d],not in master.}", pst_hal_device->uc_device_id);
        return OAL_FAIL;
    }

    /* wifi���ܴ���ɨ��״̬����ʱֱ��abort���������ɨ�����ȼ��߲������л����ٿ���ֱ�ӷ��� */
    dmac_scan_abort(pst_mac_device);

    /* 1.�������� */
    if(WLAN_M2S_TYPE_SW == pst_hal_device->st_hal_m2s_fsm.en_m2s_type)
    {
        /* ���л�׼����Ӳ�����а����浽��ٶ����� */
        ul_ret = dmac_m2s_switch_device_begin(pst_mac_device, pst_hal_device);
        if(OAL_SUCC != ul_ret)
        {
            OAM_ERROR_LOG0(0, OAM_SF_M2S, "{dmac_m2s_switch::dmac_m2s_switch_device_begin return fail.}");
            return OAL_FAIL;
        }
    }

    /* ���ñ�־���������л���׼������ */
    pst_hal_device->en_m2s_excute_flag = OAL_TRUE;

    /* 2.����device ���� */
    dmac_m2s_switch_update_device_capbility(pst_hal_device, pst_mac_device);

    /* ��ҪӲ���л����� */
    if(OAL_TRUE == en_hw_switch)
    {
        /* 2.1����hal device chain���� */
        dmac_m2s_switch_update_hal_chain_capbility(pst_hal_device, OAL_TRUE);
    }

    /* 3.����ԭ��hal device��vap��user����,vap��������hal device������, user��������vap������ */
    dmac_m2s_switch_update_vap_capbility(pst_hal_device);

#if 0 //�������֮������device vap user���Ե���ͬ����host����ͬ���ӿڱ�������������Ҫ��ɾ��
    /* 5.ͬ��host��mac device�������� */
    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(dmac_to_hmac_m2s_event_stru));
    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG0(0, OAM_SF_M2S, "{dmac_m2s_switch::alloc event failed!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event = frw_get_event_stru(pst_event_mem);

    /* ��д�¼�ͷ */
    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_HOST_SDT_REG,
                       DMAC_TO_HMAC_M2S_DATA,
                       OAL_SIZEOF(dmac_to_hmac_m2s_event_stru),
                       FRW_EVENT_PIPELINE_STAGE_1,
                       pst_mac_device->uc_chip_id,
                       pst_mac_device->uc_device_id,
                       0);

    pst_m2s_event = (dmac_to_hmac_m2s_event_stru *)(pst_event->auc_event_data);
    pst_m2s_event->en_m2s_nss   = en_nss;
    pst_m2s_event->uc_device_id = pst_mac_device->uc_device_id;

    if(OAL_SUCC != frw_event_dispatch_event(pst_event_mem))
    {
        OAM_ERROR_LOG0(0, OAM_SF_M2S, "{dmac_m2s_switch::post event failed}");
    }

    FRW_EVENT_FREE(pst_event_mem);
#endif

    pst_hal_device->en_m2s_excute_flag = OAL_FALSE;

    if(WLAN_M2S_TYPE_SW == pst_hal_device->st_hal_m2s_fsm.en_m2s_type)
    {
        if (HAL_M2S_EVENT_DBDC_START == en_m2s_event)
        {
            /* DBDC START�ŵ��Ѿ��Ǻ��������Ǹ�vap��,�����������ȥ��·,��·���ŵ����������� */
            dmac_dbdc_switch_device_end(pst_mac_device, pst_hal_device);
        }
        else
        {
#ifdef _PRE_WLAN_FEATURE_BTCOEX
            /* ����Ѿ�����btcoex ps״̬��, ����Ҫ��ִ��restore���� */
            if(HAL_BTCOEX_SW_POWSAVE_WORK == GET_HAL_BTCOEX_SW_PREEMPT_TYPE(pst_hal_device))
            {
                OAM_WARNING_LOG0(0, OAM_SF_M2S, "{dmac_m2s_switch::btcoex ps is working not need to switch end.}");
            }
            else
#endif
            {
                dmac_m2s_switch_device_end(pst_hal_device);
            }
        }
    }

    return OAL_SUCC;
}


oal_void dmac_m2s_idle_to_xixo(hal_to_dmac_device_stru *pst_hal_device, hal_m2s_event_tpye_uint16 en_m2s_event)
{
    dmac_m2s_switch_update_hal_chain_capbility(pst_hal_device, OAL_FALSE);
}


oal_void dmac_m2s_xixo_to_idle(hal_to_dmac_device_stru *pst_hal_device, hal_m2s_event_tpye_uint16 en_m2s_event)
{
    hal_device_free_all_rf_dev(pst_hal_device);
}


oal_void dmac_m2s_idle_to_idle(hal_to_dmac_device_stru *pst_hal_device, hal_m2s_event_tpye_uint16 en_m2s_event)
{
    oal_uint32 ul_ret;

    /* ˢ������ */
    dmac_m2s_update_switch_mgr_param(pst_hal_device, en_m2s_event);

    /* δ����rf����Ҫ��ǰ�ñ�־ */
    RF_RES_SET_IS_MIMO(pst_hal_device->st_cfg_cap_info.uc_rf_chain);

    /* ����л���sisoģʽ��Ӳ������ */
    ul_ret = dmac_m2s_switch(pst_hal_device, WLAN_SINGLE_NSS, en_m2s_event, OAL_FALSE);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG2(0, OAM_SF_M2S, "{dmac_m2s_mimo_to_miso:: FAIL en_m2s_event[%d], cur state[%d].}",
            en_m2s_event, GET_HAL_M2S_CUR_STATE(pst_hal_device));
    }
}


oal_void dmac_m2s_mimo_to_miso(hal_to_dmac_device_stru *pst_hal_device, hal_m2s_event_tpye_uint16 en_m2s_event)
{
    wlan_m2s_trigger_mode_enum_uint8 uc_trigger_mode = WLAN_M2S_TRIGGER_MODE_BUTT;
    oal_uint32 ul_ret;

    uc_trigger_mode = dmac_m2s_event_trigger_mode_classify(en_m2s_event);
    if(WLAN_M2S_TRIGGER_MODE_BUTT == uc_trigger_mode)
    {
        OAM_WARNING_LOG2(0, OAM_SF_M2S, "{dmac_m2s_mimo_to_miso:: device[%d] trigger_mode[%d] error!}",
            pst_hal_device->uc_device_id, uc_trigger_mode);
        return;
    }

    /* ֻҪ���л���miso�����ӳ��л�������������Ҫdevice�������뱣��, �����л���siso�ͻָ��������Ƿ�ִ���л�����������device������ */
    if(WLAN_M2S_TRIGGER_MODE_COMMAND == uc_trigger_mode)
    {
        GET_HAL_DEVICE_M2S_SWITCH_PROT(pst_hal_device) = OAL_TRUE;
    }

    /* ˢ������ */
    dmac_m2s_update_switch_mgr_param(pst_hal_device, en_m2s_event);

    /* ����л���sisoģʽ��Ӳ��������mimo */
    ul_ret = dmac_m2s_switch(pst_hal_device, WLAN_SINGLE_NSS, en_m2s_event, OAL_FALSE);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG2(0, OAM_SF_M2S, "{dmac_m2s_mimo_to_miso:: FAIL en_m2s_event[%d], cur state[%d].}",
            en_m2s_event, GET_HAL_M2S_CUR_STATE(pst_hal_device));
    }
}


oal_void dmac_m2s_miso_to_mimo(hal_to_dmac_device_stru *pst_hal_device, hal_m2s_event_tpye_uint16 en_m2s_event)
{
    wlan_m2s_trigger_mode_enum_uint8 uc_trigger_mode = WLAN_M2S_TRIGGER_MODE_BUTT;
    oal_uint32 ul_ret;

    uc_trigger_mode = dmac_m2s_event_trigger_mode_classify(en_m2s_event);
    if(WLAN_M2S_TRIGGER_MODE_BUTT == uc_trigger_mode)
    {
        OAM_WARNING_LOG2(0, OAM_SF_M2S, "{dmac_m2s_miso_to_mimo:: device[%d] trigger_mode[%d] error!}",
            pst_hal_device->uc_device_id, uc_trigger_mode);
        return;
    }

    /* ֻҪ���л���mimo���ӳ��л������ر� */
    if(WLAN_M2S_TRIGGER_MODE_COMMAND == uc_trigger_mode)
    {
        GET_HAL_DEVICE_M2S_SWITCH_PROT(pst_hal_device) = OAL_FALSE;
    }

    /* ˢ��mimo���� */
    dmac_m2s_update_switch_mgr_param(pst_hal_device, en_m2s_event);

    /* miso�л���mimo��Ӳ����������mimo�ģ���Ҫ����ֱ���л�������� */
   ul_ret = dmac_m2s_switch(pst_hal_device, WLAN_DOUBLE_NSS, en_m2s_event, OAL_FALSE);
   if(OAL_SUCC != ul_ret)
   {
       OAM_WARNING_LOG2(0, OAM_SF_M2S, "{dmac_m2s_miso_to_mimo:: FAIL en_m2s_event[%d], cur state[%d].}",
           en_m2s_event, GET_HAL_M2S_CUR_STATE(pst_hal_device));
   }
}


oal_void dmac_m2s_miso_to_siso(hal_to_dmac_device_stru *pst_hal_device, hal_m2s_event_tpye_uint16 en_m2s_event)
{
    /* ˢ��miso���� */
    dmac_m2s_update_switch_mgr_param(pst_hal_device, en_m2s_event);

    /* ���������Ȼ��siso��Ӳ��������mimo�л���siso */
    dmac_m2s_switch_update_hal_chain_capbility(pst_hal_device, OAL_TRUE);

    OAM_WARNING_LOG2(0, OAM_SF_M2S, "{dmac_m2s_miso_to_siso:: FINISH en_m2s_event[%d], cur state[%d].}",
        en_m2s_event, GET_HAL_M2S_CUR_STATE(pst_hal_device));
}


oal_void dmac_m2s_siso_to_miso(hal_to_dmac_device_stru *pst_hal_device, hal_m2s_event_tpye_uint16 en_m2s_event)
{
    /* ˢ��miso���� */
    dmac_m2s_update_switch_mgr_param(pst_hal_device, en_m2s_event);

    /* �����Ȼ��siso״̬��Ӳ���л�mimo */
    dmac_m2s_switch_update_hal_chain_capbility(pst_hal_device, OAL_TRUE);

    OAM_WARNING_LOG2(0, OAM_SF_M2S, "{dmac_m2s_siso_to_miso:: FINISH en_m2s_event[%d], cur state[%d].}",
        en_m2s_event, GET_HAL_M2S_CUR_STATE(pst_hal_device));
}


oal_void dmac_m2s_mimo_to_siso(hal_to_dmac_device_stru *pst_hal_device, hal_m2s_event_tpye_uint16 en_m2s_event)
{
    oal_uint32 ul_ret;

    /* ˢ��siso���� */
    dmac_m2s_update_switch_mgr_param(pst_hal_device, en_m2s_event);

    /* �����Ӳ��������ˢ��siso */
    ul_ret = dmac_m2s_switch(pst_hal_device, WLAN_SINGLE_NSS, en_m2s_event, OAL_TRUE);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG2(0, OAM_SF_M2S, "{dmac_m2s_mimo_to_siso:: FAIL en_m2s_event[%d], cur state[%d].}",
            en_m2s_event, GET_HAL_M2S_CUR_STATE(pst_hal_device));
    }
}


oal_void dmac_m2s_siso_to_mimo(hal_to_dmac_device_stru *pst_hal_device, hal_m2s_event_tpye_uint16 en_m2s_event)
{
    wlan_m2s_trigger_mode_enum_uint8 uc_trigger_mode = WLAN_M2S_TRIGGER_MODE_BUTT;
    oal_uint32 ul_ret;

    uc_trigger_mode = dmac_m2s_event_trigger_mode_classify(en_m2s_event);
    if(WLAN_M2S_TRIGGER_MODE_BUTT == uc_trigger_mode)
    {
        OAM_WARNING_LOG2(0, OAM_SF_M2S, "{dmac_m2s_siso_to_mimo:: device[%d] trigger_mode[%d] error!}",
            pst_hal_device->uc_device_id, uc_trigger_mode);
        return;
    }

    /* ֻҪ���л���mimo���ӳ��л������ر� */
    if(WLAN_M2S_TRIGGER_MODE_COMMAND == uc_trigger_mode)
    {
        GET_HAL_DEVICE_M2S_SWITCH_PROT(pst_hal_device) = OAL_FALSE;
    }

    /* ˢ��siso���� */
    dmac_m2s_update_switch_mgr_param(pst_hal_device, en_m2s_event);

    /* �����Ӳ��������ˢ��mimo */
    ul_ret = dmac_m2s_switch(pst_hal_device, WLAN_DOUBLE_NSS, en_m2s_event, OAL_TRUE);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG2(0, OAM_SF_M2S, "{dmac_m2s_siso_to_mimo:: FAIL en_m2s_event[%d], cur state[%d].}",
            en_m2s_event, GET_HAL_M2S_CUR_STATE(pst_hal_device));
    }
}


oal_void dmac_m2s_miso_to_miso(hal_to_dmac_device_stru *pst_hal_device, hal_m2s_event_tpye_uint16 en_m2s_event)
{
    /* ˢ��siso���� */
    dmac_m2s_update_switch_mgr_param(pst_hal_device, en_m2s_event);

    /* Ӳ���������䣬���������c0 siso��c1 siso�л� */
    /* ����ԭ��hal device��vap��user��������,vap��������hal device������, user��������vap������ */
    dmac_m2s_switch_update_vap_capbility(pst_hal_device);

    OAM_WARNING_LOG2(0, OAM_SF_M2S, "{dmac_m2s_miso_to_miso:: FINISH en_m2s_event[%d], cur state[%d].}",
        en_m2s_event, GET_HAL_M2S_CUR_STATE(pst_hal_device));
}


oal_void dmac_m2s_siso_to_siso(hal_to_dmac_device_stru *pst_hal_device, hal_m2s_event_tpye_uint16 en_m2s_event)
{
    oal_uint32 ul_ret;

    /* ˢ��siso���� */
    dmac_m2s_update_switch_mgr_param(pst_hal_device, en_m2s_event);

    /* �����Ӳ����������sisoˢ�µ���һ��siso */
    ul_ret = dmac_m2s_switch(pst_hal_device, WLAN_SINGLE_NSS, en_m2s_event, OAL_TRUE);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG2(0, OAM_SF_M2S, "{dmac_m2s_siso_to_siso:: FAIL en_m2s_event[%d], cur state[%d].}",
            en_m2s_event, GET_HAL_M2S_CUR_STATE(pst_hal_device));
    }
}


oal_void dmac_m2s_mimo_to_simo(hal_to_dmac_device_stru *pst_hal_device, hal_m2s_event_tpye_uint16 en_m2s_event)
{
    /* ˢ��siso���� */
    dmac_m2s_update_switch_mgr_param(pst_hal_device, en_m2s_event);

    /* �����Ȼ��mimo״̬��Ӳ���е�simo */
    dmac_m2s_switch_update_hal_chain_capbility(pst_hal_device, OAL_TRUE);

    OAM_WARNING_LOG2(0, OAM_SF_M2S, "{dmac_m2s_mimo_to_simo:: FINISH en_m2s_event[%d], cur state[%d].}",
        en_m2s_event, GET_HAL_M2S_CUR_STATE(pst_hal_device));
}


oal_void dmac_m2s_simo_to_mimo(hal_to_dmac_device_stru *pst_hal_device, hal_m2s_event_tpye_uint16 en_m2s_event)
{
    /* ˢ��siso���� */
    dmac_m2s_update_switch_mgr_param(pst_hal_device, en_m2s_event);

    /* �����Ȼ��siso״̬��Ӳ���л�mimo */
    dmac_m2s_switch_update_hal_chain_capbility(pst_hal_device, OAL_TRUE);

    OAM_WARNING_LOG2(0, OAM_SF_M2S, "{dmac_m2s_mimo_to_simo:: FINISH en_m2s_event[%d], cur state[%d].}",
        en_m2s_event, GET_HAL_M2S_CUR_STATE(pst_hal_device));
}


oal_uint32  dmac_get_hal_m2s_state(hal_to_dmac_device_stru *pst_hal_device)
{
    if (OAL_PTR_NULL == pst_hal_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_M2S, "{dmac_get_hal_m2s_state::hal_to_dmac_device_stru NULL}");
        return HAL_DEVICE_IDLE_STATE;
    }

    if (OAL_FALSE == pst_hal_device->st_hal_dev_fsm.uc_is_fsm_attached)
    {
        OAM_ERROR_LOG1(0, OAM_SF_M2S, "{dmac_get_hal_m2s_state::hal device id[%d],m2s fsm not attached}", pst_hal_device->uc_device_id);
        return HAL_DEVICE_IDLE_STATE;
    }

    return GET_HAL_M2S_CUR_STATE(pst_hal_device);
}


OAL_STATIC oal_uint32 dmac_m2s_fsm_trans_to_state(hal_m2s_fsm_stru *pst_m2s_fsm, oal_uint8 uc_state)
{
    oal_fsm_stru   *pst_oal_fsm = &(pst_m2s_fsm->st_oal_fsm);

    return oal_fsm_trans_to_state(pst_oal_fsm, uc_state);
}


OAL_STATIC  oal_void dmac_m2s_state_idle_entry(oal_void *p_ctx)
{
    hal_to_dmac_device_stru      *pst_hal_device;
    hal_m2s_fsm_stru             *pst_m2s_fsm;
    hal_m2s_event_tpye_uint16     us_m2s_event;

    pst_m2s_fsm = (hal_m2s_fsm_stru *)p_ctx;
    pst_hal_device  = (hal_to_dmac_device_stru *)(pst_m2s_fsm->st_oal_fsm.p_oshandler);
    if (OAL_PTR_NULL == pst_hal_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_M2S, "{dmac_m2s_state_idle_entry::hal_to_dmac_device_stru null.}");
        return;
    }

    /* ��ȡm2s�¼����� */
    us_m2s_event = pst_m2s_fsm->st_oal_fsm.us_last_event;

    switch(us_m2s_event)
    {
        /* ����idle״̬����Ҫ�ͷ�rf��Դ���� */
        case HAL_M2S_EVENT_IDLE_BEGIN:
            hal_device_free_all_rf_dev(pst_hal_device);
            break;

        default:
           OAM_WARNING_LOG2(0, OAM_SF_M2S, "{dmac_m2s_state_idle_entry: hal dev[%d] us_m2s_event[%d] error!}",pst_hal_device->uc_device_id, us_m2s_event);
    }

    OAM_WARNING_LOG3(0, OAM_SF_M2S, "{dmac_m2s_state_idle_entry::hal dev[%d],event[%d],change to idle,last state[%d]}",
                        pst_hal_device->uc_device_id, pst_m2s_fsm->st_oal_fsm.us_last_event, pst_m2s_fsm->st_oal_fsm.uc_cur_state);

}


OAL_STATIC  oal_void dmac_m2s_state_idle_exit(oal_void *p_ctx)
{
    /* TBD */
}


OAL_STATIC oal_uint32 dmac_m2s_state_idle_event(oal_void  *p_ctx, oal_uint16  us_event,
                                    oal_uint16 us_event_data_len,  oal_void  *p_event_data)
{
    hal_m2s_fsm_stru                 *pst_m2s_fsm = (hal_m2s_fsm_stru *)p_ctx;
    hal_to_dmac_device_stru          *pst_hal_device = (hal_to_dmac_device_stru *)(pst_m2s_fsm->st_oal_fsm.p_oshandler);
    hal_m2s_state_uint8               uc_m2s_state;
    wlan_m2s_trigger_mode_enum_uint8  uc_trigger_mode = WLAN_M2S_TRIGGER_MODE_BUTT;

    uc_trigger_mode = dmac_m2s_event_trigger_mode_classify(us_event);

    /* 1. ˢ�¶�Ӧҵ���� */
    if(WLAN_M2S_TRIGGER_MODE_BUTT != uc_trigger_mode)
    {
        GET_HAL_M2S_MODE_TPYE(pst_hal_device) |= uc_trigger_mode;
    }

    /* ����hal idle end������ҵ�񴥷��¼������޸Ķ�Ӧ��chain���� */
    switch(us_event)
    {
        case HAL_M2S_EVENT_SCAN_BEGIN:
        case HAL_M2S_EVENT_WORK_BEGIN:
            uc_m2s_state = dmac_m2s_chain_state_classify(pst_hal_device);

            dmac_m2s_fsm_trans_to_state(pst_m2s_fsm, uc_m2s_state);

            #if 0 //����ɨ����ôʹ�ã���Ҫ��һ��
            if(HAL_M2S_EVENT_SCAN_BEGIN == us_event)
            {
                dmac_device_stru *pst_dmac_device = dmac_res_get_mac_dev(pst_hal_device->uc_mac_device_id);
                hal_device_handle_event(dmac_device_get_another_h2d_dev(pst_dmac_device, pst_hal_device), HAL_DEVICE_EVENT_SCAN_RESUME, 0, OAL_PTR_NULL);
            }
            #endif
        break;

        default:
            uc_m2s_state = dmac_m2s_event_state_classify(us_event);

            /* �ǻָ���mimo���Ǿ���ҵ���������Ҫ���� */
            if(HAL_M2S_STATE_MIMO == uc_m2s_state)
            {
                GET_HAL_M2S_MODE_TPYE(pst_hal_device) &= (~uc_trigger_mode);
            }

            /* �����¼�������Ҫˢ����Ӳ���������������ͷ�rf�����Ǵ���idle״̬����״̬����ʱ�������л� rf�������� */
            dmac_m2s_idle_to_idle(pst_hal_device, us_event);

            OAM_WARNING_LOG2(pst_hal_device->uc_device_id, OAM_SF_M2S, "{dmac_m2s_state_idle_event::event[%d]uc_m2s_state[%d] no handle!!!}",
                us_event, uc_m2s_state);
        break;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_void dmac_m2s_state_mimo_entry(oal_void *p_ctx)
{
    hal_to_dmac_device_stru      *pst_hal_device;
    hal_m2s_fsm_stru             *pst_m2s_fsm;
    hal_m2s_event_tpye_uint16     us_m2s_event;

    pst_m2s_fsm = (hal_m2s_fsm_stru *)p_ctx;
    pst_hal_device  = (hal_to_dmac_device_stru *)(pst_m2s_fsm->st_oal_fsm.p_oshandler);
    if (OAL_PTR_NULL == pst_hal_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_M2S, "{dmac_m2s_state_mimo_entry::hal_to_dmac_device_stru null.}");
        return;
    }

    /* ��ȡm2s�¼����� */
    us_m2s_event = pst_m2s_fsm->st_oal_fsm.us_last_event;

    switch(us_m2s_event)
    {
        case HAL_M2S_EVENT_DBDC_STOP:
        case HAL_M2S_EVENT_DBDC_SISO_TO_MIMO:
            /* m2s״̬�����dbdc״̬ */
            dmac_m2s_siso_to_mimo(pst_hal_device, us_m2s_event);
            break;

        case HAL_M2S_EVENT_TEST_SISO_C0_TO_MIMO:
        case HAL_M2S_EVENT_TEST_SISO_C1_TO_MIMO:
            /* m2s״̬�����test */
            dmac_m2s_siso_to_mimo(pst_hal_device, us_m2s_event);
            break;

        case HAL_M2S_EVENT_COMMAND_SISO_TO_MIMO:
            /* m2s״̬�����command */
            dmac_m2s_siso_to_mimo(pst_hal_device, us_m2s_event);
            break;

        case HAL_M2S_EVENT_COMMAND_MISO_TO_MIMO:
            /* m2s״̬�����test */
            dmac_m2s_miso_to_mimo(pst_hal_device, us_m2s_event);
            break;

        case HAL_M2S_EVENT_ANT_RSSI_MISO_TO_MIMO:
            dmac_m2s_miso_to_mimo(pst_hal_device, us_m2s_event);
            break;

        /* scan������Ҫ�ָ�Ӳ������ */
        case HAL_M2S_EVENT_SCAN_END:
        case HAL_M2S_EVENT_SCAN_CHANNEL_BACK:
            /* m2s״̬���������ɨ�� */
            dmac_m2s_simo_to_mimo(pst_hal_device, us_m2s_event);
            break;

        case HAL_M2S_EVENT_SCAN_BEGIN:
        case HAL_M2S_EVENT_WORK_BEGIN:
            dmac_m2s_idle_to_xixo(pst_hal_device, us_m2s_event);
            break;

        default:
            OAM_WARNING_LOG2(0, OAM_SF_M2S, "{dmac_m2s_state_mimo_entry: hal dev[%d] us_m2s_event[%d] error!}",pst_hal_device->uc_device_id, us_m2s_event);
    }

    OAM_WARNING_LOG3(0, OAM_SF_M2S, "{dmac_m2s_state_mimo_entry::hal dev[%d],event[%d],change to mimo,last state[%d]}",
                        pst_hal_device->uc_device_id, us_m2s_event, pst_m2s_fsm->st_oal_fsm.uc_cur_state);
}


OAL_STATIC oal_void dmac_m2s_state_mimo_exit(oal_void *p_ctx)
{
    /* TBD */
}


OAL_STATIC oal_uint32 dmac_m2s_state_mimo_event(oal_void  *p_ctx,
                                                             oal_uint16 us_event,
                                                             oal_uint16 us_event_data_len,
                                                             oal_void  *p_event_data)
{
    dmac_device_stru                 *pst_dmac_device;
    hal_to_dmac_device_stru          *pst_hal_device;
    hal_m2s_fsm_stru                 *pst_m2s_fsm = (hal_m2s_fsm_stru *)p_ctx;
    wlan_m2s_trigger_mode_enum_uint8  uc_trigger_mode = WLAN_M2S_TRIGGER_MODE_BUTT;

    pst_hal_device  = (hal_to_dmac_device_stru *)(pst_m2s_fsm->st_oal_fsm.p_oshandler);

    uc_trigger_mode = dmac_m2s_event_trigger_mode_classify(us_event);

    /* 1. ˢ�¶�Ӧҵ���� */
    if(WLAN_M2S_TRIGGER_MODE_BUTT != uc_trigger_mode)
    {
        GET_HAL_M2S_MODE_TPYE(pst_hal_device) |= uc_trigger_mode;
    }

    switch (us_event)
    {
        /* hal device����idle״̬��m2sҲ����idle״̬���ͷ�rf��Դ */
        case HAL_M2S_EVENT_IDLE_BEGIN:
            dmac_m2s_fsm_trans_to_state(pst_m2s_fsm, HAL_M2S_STATE_IDLE);
          break;

        case HAL_M2S_EVENT_ANT_RSSI_MIMO_TO_MISO:
        case HAL_M2S_EVENT_COMMAND_MIMO_TO_MISO_C0:
        case HAL_M2S_EVENT_COMMAND_MIMO_TO_MISO_C1:
            dmac_m2s_fsm_trans_to_state(pst_m2s_fsm, HAL_M2S_STATE_MISO);
          break;

        case HAL_M2S_EVENT_DBDC_MIMO_TO_SISO:
        case HAL_M2S_EVENT_DBDC_START:
        case HAL_M2S_EVENT_TEST_MIMO_TO_SISO_C0:
        case HAL_M2S_EVENT_TEST_MIMO_TO_SISO_C1:
        case HAL_M2S_EVENT_COMMAND_MIMO_TO_SISO_C0:
        case HAL_M2S_EVENT_COMMAND_MIMO_TO_SISO_C1:
            dmac_m2s_fsm_trans_to_state(pst_m2s_fsm, HAL_M2S_STATE_SISO);
          break;

        case HAL_M2S_EVENT_SCAN_BEGIN:
            pst_dmac_device = dmac_res_get_mac_dev(pst_hal_device->uc_mac_device_id);
            if (OAL_TRUE == pst_dmac_device->en_is_fast_scan)
            {
                /* m2s״̬��ʹ�ܲ���ɨ�� */
                GET_HAL_M2S_MODE_TPYE(pst_hal_device) |= WLAN_M2S_TRIGGER_MODE_FAST_SCAN;

                dmac_m2s_fsm_trans_to_state(pst_m2s_fsm, HAL_M2S_STATE_SIMO);
                hal_device_handle_event(dmac_device_get_another_h2d_dev(pst_dmac_device, pst_hal_device), HAL_DEVICE_EVENT_SCAN_RESUME, 0, OAL_PTR_NULL);
            }
          break;

        /* mimo״̬Ҳ���յ����¼����ǲ���ɨ��ʱ */
        case HAL_M2S_EVENT_SCAN_END:
        case HAL_M2S_EVENT_SCAN_CHANNEL_BACK:
        case HAL_M2S_EVENT_WORK_BEGIN:
          break;

        default:
            OAM_ERROR_LOG1(pst_hal_device->uc_device_id, OAM_SF_M2S, "{dmac_m2s_state_mimo_event:: us_event[%d] INVALID!}", us_event);
          break;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_void dmac_m2s_state_siso_entry(oal_void *p_ctx)
{
    hal_to_dmac_device_stru      *pst_hal_device;
    hal_m2s_fsm_stru             *pst_m2s_fsm;
    hal_m2s_event_tpye_uint16     us_m2s_event;

    pst_m2s_fsm = (hal_m2s_fsm_stru *)p_ctx;
    pst_hal_device  = (hal_to_dmac_device_stru *)(pst_m2s_fsm->st_oal_fsm.p_oshandler);

    if (OAL_PTR_NULL == pst_hal_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_M2S, "{dmac_m2s_state_siso_entry::hal_to_dmac_device_stru null.}");
        return;
    }

    /* ��ȡm2s�¼����� */
    us_m2s_event = pst_m2s_fsm->st_oal_fsm.us_last_event;

    switch(us_m2s_event)
    {
        case HAL_M2S_EVENT_DBDC_MIMO_TO_SISO:
            dmac_m2s_mimo_to_siso(pst_hal_device, us_m2s_event);
            break;

        case HAL_M2S_EVENT_TEST_MIMO_TO_SISO_C0:
        case HAL_M2S_EVENT_TEST_MIMO_TO_SISO_C1:
            dmac_m2s_mimo_to_siso(pst_hal_device, us_m2s_event);
            break;

        case HAL_M2S_EVENT_COMMAND_MIMO_TO_SISO_C0:
        case HAL_M2S_EVENT_COMMAND_MIMO_TO_SISO_C1:
            dmac_m2s_mimo_to_siso(pst_hal_device, us_m2s_event);
            break;

        case HAL_M2S_EVENT_ANT_RSSI_MISO_TO_C0_SISO:
        case HAL_M2S_EVENT_ANT_RSSI_MISO_TO_C1_SISO:
        case HAL_M2S_EVENT_COMMAND_MISO_TO_SISO_C0:
        case HAL_M2S_EVENT_COMMAND_MISO_TO_SISO_C1:
            dmac_m2s_miso_to_siso(pst_hal_device, us_m2s_event);
            break;

        case HAL_M2S_EVENT_DBDC_START:
            dmac_m2s_mimo_to_siso(pst_hal_device, us_m2s_event);//����·�е�siso
            //dmac_dbdc_switch_vap_to_slave(pst_hal_device);//�ٽ���·��vap�е���·ȥ
            break;

        case HAL_M2S_EVENT_SCAN_BEGIN:
        case HAL_M2S_EVENT_WORK_BEGIN:
            dmac_m2s_idle_to_xixo(pst_hal_device, us_m2s_event);
            break;

        default:
           OAM_WARNING_LOG2(0, OAM_SF_M2S, "{dmac_m2s_state_siso_entry: hal dev[%d] us_m2s_event[%d] error!}",pst_hal_device->uc_device_id, us_m2s_event);
    }

    OAM_WARNING_LOG3(0, OAM_SF_M2S, "{dmac_m2s_state_siso_entry::hal dev[%d],event[%d],change to siso,last state[%d]}",
                        pst_hal_device->uc_device_id, us_m2s_event, pst_m2s_fsm->st_oal_fsm.uc_cur_state);
}


OAL_STATIC oal_void dmac_m2s_state_siso_exit(oal_void *p_ctx)
{

}


OAL_STATIC oal_uint32 dmac_m2s_state_siso_event(oal_void  *p_ctx,
                                                         oal_uint16  us_event,
                                                         oal_uint16 us_event_data_len,
                                                         oal_void  *p_event_data)
{
    hal_m2s_fsm_stru                 *pst_m2s_fsm;
    hal_to_dmac_device_stru          *pst_hal_device;
    dmac_device_stru                 *pst_dmac_device;
    wlan_m2s_trigger_mode_enum_uint8  uc_trigger_mode = WLAN_M2S_TRIGGER_MODE_BUTT;
    hal_m2s_state_uint8               en_new_state;

    pst_m2s_fsm     = (hal_m2s_fsm_stru *)p_ctx;
    pst_hal_device  = (hal_to_dmac_device_stru *)(pst_m2s_fsm->st_oal_fsm.p_oshandler);

    pst_dmac_device = dmac_res_get_mac_dev(pst_hal_device->uc_mac_device_id);
    if (OAL_PTR_NULL == pst_dmac_device)
    {
        OAM_ERROR_LOG0(pst_hal_device->uc_mac_device_id, OAM_SF_M2S, "{dmac_m2s_state_siso_event::pst_dmac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    uc_trigger_mode = dmac_m2s_event_trigger_mode_classify(us_event);

    /* 1. ˢ�¶�Ӧҵ���� */
    if(WLAN_M2S_TRIGGER_MODE_BUTT != uc_trigger_mode)
    {
        GET_HAL_M2S_MODE_TPYE(pst_hal_device) |= uc_trigger_mode;
    }

    /* 2.�ƻ��ص�mixo, ��Ӧҵ������ûر�� */
    switch (us_event)
    {
        /* hal device����idle״̬��m2sҲ����idle״̬���ͷ�rf��Դ */
        case HAL_M2S_EVENT_IDLE_BEGIN:
            dmac_m2s_fsm_trans_to_state(pst_m2s_fsm, HAL_M2S_STATE_IDLE);
          break;

        /* ���չ��������Դ���̽�� */
        case HAL_M2S_EVENT_ANT_RSSI_SISO_TO_MISO:
            dmac_m2s_fsm_trans_to_state(pst_m2s_fsm, HAL_M2S_STATE_MISO);
          break;

        /* NONE��ʽ���Դ����л���������ʱ�������� */
        case HAL_M2S_EVENT_COMMAND_MISO_TO_SISO_C0:
        case HAL_M2S_EVENT_COMMAND_MISO_TO_SISO_C1:
            OAM_WARNING_LOG0(pst_hal_device->uc_device_id, OAM_SF_M2S, "{dmac_m2s_state_siso_event::delay switch already succ to siso.}");
          break;

          /* �����ص�mimo */
        case HAL_M2S_EVENT_TEST_SISO_C0_TO_MIMO:
        case HAL_M2S_EVENT_TEST_SISO_C1_TO_MIMO:
        case HAL_M2S_EVENT_DBDC_SISO_TO_MIMO:
        case HAL_M2S_EVENT_COMMAND_SISO_TO_MIMO:
            /* ҵ���������״̬�л����� */
            dmac_m2s_switch_back_to_mixo_check(pst_m2s_fsm, pst_hal_device, uc_trigger_mode, &en_new_state);
          break;

        case HAL_M2S_EVENT_TEST_SISO_C1_TO_SISO_C0:
        case HAL_M2S_EVENT_TEST_SISO_C0_TO_SISO_C1:
        case HAL_M2S_EVENT_COMMAND_SISO_C1_TO_SISO_C0:
        case HAL_M2S_EVENT_COMMAND_SISO_C0_TO_SISO_C1:
              /* c0��c1֮����л���״̬�����䣬ֻ�����¼���ͨ���������� */
              dmac_m2s_siso_to_siso(pst_hal_device, us_event);
          break;

        /* siso���յ�dbdc start����Ҫmimo��siso,ֱ����·vap�е���· */
        case HAL_M2S_EVENT_DBDC_START:
            //dmac_dbdc_switch_vap_to_slave(pst_hal_device);
          break;

        case HAL_M2S_EVENT_DBDC_STOP:
            /* ��·��m2s fsmʼ����siso״̬������Ҫ�� */
            /* ��·��m2s fsm�л�mimo */
            dmac_m2s_fsm_trans_to_state(&(DMAC_DEV_GET_MST_HAL_DEV(pst_dmac_device)->st_hal_m2s_fsm), HAL_M2S_STATE_MIMO);
          break;

        /* siso״̬�յ�ɨ���¼�����Ҫ����, scan�ָ���work�Ļ���Ҳ����Ҫ���� */
        case HAL_M2S_EVENT_SCAN_BEGIN:
        case HAL_M2S_EVENT_SCAN_CHANNEL_BACK:
        case HAL_M2S_EVENT_SCAN_END:
        case HAL_M2S_EVENT_WORK_BEGIN:
          break;

        default:
            OAM_ERROR_LOG1(pst_hal_device->uc_device_id, OAM_SF_M2S, "{dmac_m2s_state_siso_event:: us_event[%d] INVALID!}", us_event);
          break;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_void dmac_m2s_state_miso_entry(oal_void *p_ctx)
{
    hal_to_dmac_device_stru      *pst_hal_device;
    hal_m2s_fsm_stru             *pst_m2s_fsm;
    hal_m2s_event_tpye_uint16     us_m2s_event;

    pst_m2s_fsm = (hal_m2s_fsm_stru *)p_ctx;
    pst_hal_device  = (hal_to_dmac_device_stru *)(pst_m2s_fsm->st_oal_fsm.p_oshandler);

    if (OAL_PTR_NULL == pst_hal_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_M2S, "{dmac_m2s_state_miso_entry::hal_to_dmac_device_stru null.}");
        return;
    }

    /* ��ȡm2s�¼����� */
    us_m2s_event = pst_m2s_fsm->st_oal_fsm.us_last_event;

    switch(us_m2s_event)
    {
        case HAL_M2S_EVENT_ANT_RSSI_SISO_TO_MISO:
            dmac_m2s_siso_to_miso(pst_hal_device, us_m2s_event);
            break;

        case HAL_M2S_EVENT_ANT_RSSI_MIMO_TO_MISO:
        case HAL_M2S_EVENT_COMMAND_MIMO_TO_MISO_C0:
        case HAL_M2S_EVENT_COMMAND_MIMO_TO_MISO_C1:
            dmac_m2s_mimo_to_miso(pst_hal_device, us_m2s_event);
            break;

        case HAL_M2S_EVENT_SCAN_BEGIN:
        case HAL_M2S_EVENT_WORK_BEGIN:
            dmac_m2s_idle_to_xixo(pst_hal_device, us_m2s_event);
            break;

        default:
            OAM_ERROR_LOG2(0, OAM_SF_M2S, "{dmac_m2s_state_miso_entry: hal dev[%d] us_m2s_event[%d] error!}",pst_hal_device->uc_device_id, us_m2s_event);
            return;
    }

    OAM_WARNING_LOG3(0, OAM_SF_M2S, "{dmac_m2s_state_miso_entry::hal dev[%d],event[%d],change to miso,last state[%d]}",
                    pst_hal_device->uc_device_id, us_m2s_event, pst_m2s_fsm->st_oal_fsm.uc_cur_state);
}


OAL_STATIC oal_void dmac_m2s_state_miso_exit(oal_void *p_ctx)
{
    /* ��˫ͨ��, Э����ȻΪSISO */
}


OAL_STATIC oal_uint32 dmac_m2s_state_miso_event(oal_void  *p_ctx,
                                                            oal_uint16  us_event,
                                                            oal_uint16 us_event_data_len,
                                                            oal_void  *p_event_data)
{
    hal_to_dmac_device_stru          *pst_hal_device;
    hal_m2s_fsm_stru                 *pst_m2s_fsm;
    wlan_m2s_trigger_mode_enum_uint8  uc_trigger_mode = WLAN_M2S_TRIGGER_MODE_BUTT;
    hal_m2s_state_uint8               en_new_state;

    pst_m2s_fsm = (hal_m2s_fsm_stru *)p_ctx;
    pst_hal_device  = (hal_to_dmac_device_stru *)(pst_m2s_fsm->st_oal_fsm.p_oshandler);

    uc_trigger_mode = dmac_m2s_event_trigger_mode_classify(us_event);

    /* 1. ˢ�¶�Ӧҵ���� */
    if(WLAN_M2S_TRIGGER_MODE_BUTT != uc_trigger_mode)
    {
        GET_HAL_M2S_MODE_TPYE(pst_hal_device) |= uc_trigger_mode;
    }

    switch (us_event)
    {
        /* hal device����idle״̬��m2sҲ����idle״̬���ͷ�rf��Դ */
        case HAL_M2S_EVENT_IDLE_BEGIN:
            dmac_m2s_fsm_trans_to_state(pst_m2s_fsm, HAL_M2S_STATE_IDLE);
          break;

        case HAL_M2S_EVENT_ANT_RSSI_MISO_TO_C0_SISO:
        case HAL_M2S_EVENT_ANT_RSSI_MISO_TO_C1_SISO:
        case HAL_M2S_EVENT_COMMAND_MISO_TO_SISO_C0:
        case HAL_M2S_EVENT_COMMAND_MISO_TO_SISO_C1:
            dmac_m2s_fsm_trans_to_state(pst_m2s_fsm, HAL_M2S_STATE_SISO);
          break;

        case HAL_M2S_EVENT_ANT_RSSI_MISO_TO_MIMO:
        case HAL_M2S_EVENT_COMMAND_MISO_TO_MIMO:
            /* �����л�misx�����лأ����򱣳���siso״̬ */
            dmac_m2s_switch_back_to_mixo_check(pst_m2s_fsm, pst_hal_device, uc_trigger_mode, &en_new_state);
          break;

        case HAL_M2S_EVENT_COMMAND_MISO_C1_TO_MISO_C0:
        case HAL_M2S_EVENT_COMMAND_MISO_C0_TO_MISO_C1:
              /* c0��c1֮����л���״̬�����䣬ֻ�����¼���ͨ���������� */
              dmac_m2s_miso_to_miso(pst_hal_device, us_event);
          break;

        /* mimo״̬Ҳ���յ����¼����ǲ���ɨ��ʱ */
        case HAL_M2S_EVENT_WORK_BEGIN:
        case HAL_M2S_EVENT_SCAN_BEGIN:
        case HAL_M2S_EVENT_SCAN_END:
        case HAL_M2S_EVENT_SCAN_CHANNEL_BACK:
          break;

        default:
            OAM_ERROR_LOG2(pst_hal_device->uc_device_id, OAM_SF_M2S, "{dmac_m2s_state_miso_event:cur state[%d] wrong event[%d].}",
                GET_HAL_M2S_CUR_STATE(pst_hal_device), us_event);
    }

    return OAL_SUCC;

}

OAL_STATIC oal_void dmac_m2s_state_simo_entry(oal_void *p_ctx)
{
    hal_to_dmac_device_stru      *pst_hal_device;
    hal_m2s_fsm_stru             *pst_m2s_fsm;
    hal_m2s_event_tpye_uint16     us_m2s_event;
    dmac_device_stru             *pst_dmac_device;

    pst_m2s_fsm = (hal_m2s_fsm_stru *)p_ctx;
    pst_hal_device  = (hal_to_dmac_device_stru *)(pst_m2s_fsm->st_oal_fsm.p_oshandler);

    if (OAL_PTR_NULL == pst_hal_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_M2S, "{dmac_m2s_state_simo_entry::hal_to_dmac_device_stru null.}");
        return;
    }

    /* ��ȡm2s�¼����� */
    us_m2s_event = pst_m2s_fsm->st_oal_fsm.us_last_event;

    switch(us_m2s_event)
    {
        /* ����ɨ�����ͨǰ������ɨ����Ҫ���ֿ����� TBD */
        case HAL_M2S_EVENT_SCAN_BEGIN:
            pst_dmac_device = dmac_res_get_mac_dev(pst_hal_device->uc_mac_device_id);
            if (OAL_TRUE == pst_dmac_device->en_is_fast_scan)
            {
                dmac_m2s_mimo_to_simo(pst_hal_device, us_m2s_event);
            }
            else
            {
                dmac_m2s_idle_to_xixo(pst_hal_device, us_m2s_event);
            }
            break;

        case HAL_M2S_EVENT_WORK_BEGIN:
            dmac_m2s_idle_to_xixo(pst_hal_device, us_m2s_event);
            break;

        default:
            OAM_ERROR_LOG2(0, OAM_SF_M2S, "{dmac_m2s_state_simo_entry: hal dev[%d] us_m2s_event[%d] error!}",pst_hal_device->uc_device_id, us_m2s_event);
            return;
    }

    OAM_WARNING_LOG3(0, OAM_SF_M2S, "{dmac_m2s_state_simo_entry::hal dev[%d],event[%d],change to simo,last state[%d]}",
                        pst_hal_device->uc_device_id, us_m2s_event, pst_m2s_fsm->st_oal_fsm.uc_cur_state);
}


OAL_STATIC oal_void dmac_m2s_state_simo_exit(oal_void *p_ctx)
{
    return;
}


OAL_STATIC oal_uint32 dmac_m2s_state_simo_event(oal_void  *p_ctx,
                                                            oal_uint16  us_event,
                                                            oal_uint16 us_event_data_len,
                                                            oal_void  *p_event_data)
{
    hal_to_dmac_device_stru          *pst_hal_device;
    dmac_device_stru                 *pst_dmac_device;
    hal_m2s_fsm_stru                 *pst_m2s_fsm = (hal_m2s_fsm_stru *)p_ctx;;
    wlan_m2s_trigger_mode_enum_uint8  uc_trigger_mode = WLAN_M2S_TRIGGER_MODE_BUTT;
    hal_m2s_state_uint8               en_new_state;

    pst_hal_device  = (hal_to_dmac_device_stru *)(pst_m2s_fsm->st_oal_fsm.p_oshandler);
    pst_dmac_device = dmac_res_get_mac_dev(pst_hal_device->uc_mac_device_id);

    uc_trigger_mode = dmac_m2s_event_trigger_mode_classify(us_event);

    /* 1. ˢ�¶�Ӧҵ���� */
    if(WLAN_M2S_TRIGGER_MODE_BUTT != uc_trigger_mode)
    {
        GET_HAL_M2S_MODE_TPYE(pst_hal_device) |= uc_trigger_mode;
    }

    switch (us_event)
    {
        /* hal device����idle״̬��m2sҲ����idle״̬���ͷ�rf��Դ */
        case HAL_M2S_EVENT_IDLE_BEGIN:
            dmac_m2s_fsm_trans_to_state(pst_m2s_fsm, HAL_M2S_STATE_IDLE);
          break;

        case HAL_M2S_EVENT_SCAN_CHANNEL_BACK:
            /*���¼�����һ��hal device״̬��,��ͣ��hal device��ɨ��,�ó�rf��Դ����·��home channel���� */
            hal_device_handle_event(dmac_device_get_another_h2d_dev(pst_dmac_device, pst_hal_device), HAL_DEVICE_EVENT_SCAN_PAUSE, 0, OAL_PTR_NULL);

            /* �����л�misx�����лأ����򱣳���siso״̬ */
            dmac_m2s_switch_back_to_mixo_check(pst_m2s_fsm, pst_hal_device, uc_trigger_mode, &en_new_state);

          break;

        case HAL_M2S_EVENT_SCAN_END:
            /* �����л�misx�����лأ����򱣳���siso״̬ */
            dmac_m2s_switch_back_to_mixo_check(pst_m2s_fsm, pst_hal_device, uc_trigger_mode, &en_new_state);

          break;

        case HAL_M2S_EVENT_WORK_BEGIN:
          break;

        default:
            OAM_ERROR_LOG2(pst_hal_device->uc_device_id, OAM_SF_M2S, "{dmac_m2s_state_simo_event:cur state[%d] wrong event[%d].}",
                GET_HAL_M2S_CUR_STATE(pst_hal_device), us_event);
    }

    return OAL_SUCC;
}


oal_uint32 dmac_m2s_handle_event(hal_to_dmac_device_stru *pst_hal_device, oal_uint16 us_type,
    oal_uint16 us_datalen, oal_uint8* pst_data)
{
    oal_uint32                 ul_ret;
    hal_m2s_fsm_stru          *pst_m2s_fsm;

    pst_m2s_fsm = &(pst_hal_device->st_hal_m2s_fsm);
    if (OAL_FALSE == pst_m2s_fsm->uc_is_fsm_attached)
    {
        OAM_ERROR_LOG1(0, OAM_SF_M2S, "{dmac_m2s_handle_event::hal device[%d]fsm not attached!}", pst_hal_device->uc_device_id);
        return OAL_FAIL;
    }

    ul_ret = oal_fsm_event_dispatch(&(pst_m2s_fsm->st_oal_fsm), us_type, us_datalen, pst_data);
    if (ul_ret != OAL_SUCC)
    {
        OAM_ERROR_LOG3(0, OAM_SF_M2S, "{dmac_m2s_handle_event::state[%d]dispatch event[%d]not succ[%d]!}",
                        GET_HAL_M2S_CUR_STATE(pst_hal_device), us_type, ul_ret);
    }

    return ul_ret;
}


oal_void dmac_m2s_fsm_attach(hal_to_dmac_device_stru   *pst_hal_device)
{
    oal_uint8                auc_fsm_name[6] = {0};
    oal_uint32               ul_ret;
    hal_m2s_fsm_stru        *pst_m2s_fsm  = OAL_PTR_NULL;
    hal_m2s_state_uint8      uc_state = HAL_M2S_STATE_IDLE;  /* ��ʼ����״̬ */

    if (OAL_TRUE == pst_hal_device->st_hal_m2s_fsm.uc_is_fsm_attached)
    {
        OAM_WARNING_LOG1(0, OAM_SF_M2S, "{dmac_m2s_fsm_attach::hal device id[%d]fsm have attached.}", pst_hal_device->uc_device_id);
        return;
    }

    pst_m2s_fsm = &(pst_hal_device->st_hal_m2s_fsm);

    /* ��Ϊhal device��ʼ��idle״̬״̬��m2sҲ��Ҫ��ʼΪ��״̬ */
    //uc_state = dmac_m2s_chain_state_classify(pst_hal_device);

    OAL_MEMZERO(pst_m2s_fsm, OAL_SIZEOF(hal_m2s_fsm_stru));

    /* ׼��һ��Ψһ��fsmname */
    auc_fsm_name[0] = (oal_uint8)pst_hal_device->ul_core_id;
    auc_fsm_name[1] = pst_hal_device->uc_chip_id;
    auc_fsm_name[2] = pst_hal_device->uc_mac_device_id;
    auc_fsm_name[3] = pst_hal_device->uc_device_id;

    ul_ret = oal_fsm_create((oal_void*)pst_hal_device,
                              auc_fsm_name,
                              pst_m2s_fsm,
                              &(pst_m2s_fsm->st_oal_fsm),
                              uc_state,
                              g_ast_hal_m2s_fsm_info,
                              OAL_SIZEOF(g_ast_hal_m2s_fsm_info)/OAL_SIZEOF(oal_fsm_state_info));

    if (OAL_SUCC == ul_ret)
    {
        /* oal fsm create succ */
        pst_m2s_fsm->uc_is_fsm_attached    = OAL_TRUE;
        pst_m2s_fsm->en_m2s_type           = WLAN_M2S_TYPE_SW;
        OAL_MEMZERO(&pst_m2s_fsm->st_m2s_mode, OAL_SIZEOF(wlan_m2s_mode_stru)); /* ��ʱ��û��ҵ�������л� */
    }
}


oal_void dmac_m2s_fsm_detach(hal_to_dmac_device_stru *pst_hal_device)
{
    hal_m2s_fsm_stru         *pst_m2s_fsm    = OAL_PTR_NULL;

    pst_m2s_fsm = &(pst_hal_device->st_hal_m2s_fsm);

    if (OAL_FALSE == pst_m2s_fsm->uc_is_fsm_attached)
    {
        OAM_ERROR_LOG1(0, OAM_SF_M2S, "{dmac_m2s_fsm_detach::hal device id[%d]fsm not attatched}", pst_hal_device->uc_device_id);
        return;
    }

    /* ����IDLE״̬�л���IDLE״̬ */
    if (GET_HAL_M2S_CUR_STATE(pst_hal_device) != HAL_M2S_STATE_IDLE)
    {
        dmac_m2s_fsm_trans_to_state(pst_m2s_fsm, HAL_M2S_STATE_IDLE);
    }

    return;
}



oal_void dmac_m2s_mgmt_switch(hal_to_dmac_device_stru *pst_hal_device, dmac_vap_stru *pst_dmac_vap, oal_uint8 uc_single_tx_chain)
{
    pst_hal_device->st_cfg_cap_info.uc_single_tx_chain = uc_single_tx_chain;
    /* 1.�޶��Ĵ�������Ӧ֡����ͨ�� */
    hal_update_datarate_by_chain(pst_hal_device);
    /* 2.�޶��������ع���֡����ͨ�� */
    /* ��ʼ����������֡���� */
    dmac_vap_init_tx_mgmt_rate(pst_dmac_vap, pst_dmac_vap->ast_tx_mgmt_ucast);

    /* ��ʼ���鲥���㲥����֡���� */
    dmac_vap_init_tx_mgmt_rate(pst_dmac_vap, pst_dmac_vap->ast_tx_mgmt_bmcast);
}

#endif


#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif
