
#ifdef  __cplusplus
#if     __cplusplus
extern  "C" {
#endif
#endif

/*****************************************************************************
  1 ͷ�ļ�����
 *****************************************************************************/
#include    "wlan_spec.h"
#include    "mac_device.h"
#include    "dmac_main.h"
#include    "dmac_mgmt_bss_comm.h"
#include    "mac_regdomain.h"
#include    "dmac_tx_bss_comm.h"
#include    "dmac_fcs.h"
#ifdef _PRE_WLAN_FEATURE_STA_PM
#include "dmac_sta_pm.h"
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_MAC_FCS_C

/*****************************************************************************
  2 ȫ�ֱ�������
 *****************************************************************************/

/*****************************************************************************
  3 ����ʵ��
****************************************************************************/


oal_uint32  mac_fcs_set_channel(hal_to_dmac_device_stru    *pst_hal_device,
                                        mac_channel_stru           *pst_channel)
{
    oal_uint32               ul_ret;
    oal_uint                 ul_irq_flag;

    if ((OAL_PTR_NULL == pst_hal_device) || (OAL_PTR_NULL == pst_channel))
    {
        OAM_WARNING_LOG2(0, OAM_SF_ANY, "{mac_fcs_set_channel::null param %p %p", pst_hal_device, pst_channel);
        return OAL_FAIL;
    }

    /* ����ŵ����Ƿ�Ϸ� */
    ul_ret = mac_is_channel_num_valid(pst_channel->en_band, pst_channel->uc_chan_number);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG2(0, OAM_SF_ANY, "{mac_fcs_set_channel::invalid channel num. en_band=%d, uc_chan_num=%d", pst_channel->en_band, pst_channel->uc_chan_number);
        return ul_ret;
    }
    /* ���жϣ�����Ӳ��������Ҫ���ж� */
    oal_irq_save(&ul_irq_flag, OAL_5115IRQ_MFSC);

    /* �ر�pa */
    //hal_disable_machw_phy_and_pa(pst_hal_device);

    /* ����Ƶ�� */
    hal_set_freq_band(pst_hal_device, pst_channel->en_band);

    /* ���ô��� */
#if (_PRE_WLAN_CHIP_ASIC == _PRE_WLAN_CHIP_VERSION)
        /*dummy*/
#else
    if (pst_channel->en_bandwidth >= WLAN_BAND_WIDTH_80PLUSPLUS)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RX, "{mac_fcs_set_channel:: fpga is not support 80M.}\r\n");
        pst_channel->en_bandwidth = WLAN_BAND_WIDTH_20M;
    }
#endif
    hal_set_bandwidth_mode(pst_hal_device, pst_channel->en_bandwidth);

    /* �����ŵ��� */
    hal_set_primary_channel(pst_hal_device, pst_channel->uc_chan_number, pst_channel->en_band, pst_channel->uc_chan_idx, pst_channel->en_bandwidth);

    /* ��pa */
    //hal_enable_machw_phy_and_pa(pst_hal_device);
    //OAM_WARNING_LOG0(0, OAM_SF_DBAC, "{mac_fcs_set_channel::after set channel enable pa!}");

    /* ���ж� */
    oal_irq_restore(&ul_irq_flag, OAL_5115IRQ_MFSC);

#ifdef _PRE_WLAN_DFT_EVENT
    oam_event_chan_report(pst_channel->uc_chan_number);
#endif
    OAM_INFO_LOG1(0, OAM_SF_ANY, "fcs_event: switch chan to:%d", pst_channel->uc_chan_number);

    return OAL_SUCC;
}

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)

oal_uint32  dmac_fcs_get_prot_mode(mac_vap_stru *pst_mac_vap, hal_one_packet_cfg_stru *pst_one_packet_cfg)
{
    hal_to_dmac_device_stru  *pst_hal_device = OAL_PTR_NULL;
    oal_uint32                ul_tx_mode = 0;
    oal_uint8                 uc_subband_idx = 0;
    oal_uint8                 uc_pow_level_idx = 0;
    oal_uint8                 uc_rate_pow_idx = HAL_POW_5G_6MBPS_RATE_POW_IDX; /* ���ʳ�ʼ��Ϊ6M��������Ҳ�����������*/

    pst_hal_device = DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap);
    if(OAL_PTR_NULL == pst_hal_device)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "dmac_fcs_get_prot_mode: pst_hal_device is NULL.");
        return ul_tx_mode;
    }

    /* ��ȡ����֡���ʵȼ� */
    uc_pow_level_idx = pst_hal_device->uc_control_frm_power_level;
    if(HAL_POW_RF_LIMIT_POW_LEVEL == uc_pow_level_idx)
    {
        /* RF LIMIT power��Ҫ����Max Power������ǿ����˳�ʼpow code��Ҫʹ��max power��POW code */
        uc_pow_level_idx = HAL_POW_MAX_POW_LEVEL;
    }

    /* ��ȡ��ǰ���� */
    pst_hal_device->st_phy_pow_param.uc_rate_one_pkt = (pst_one_packet_cfg->ul_tx_data_rate >> 16) & 0xFF;

    /* ˢ��one packet֡��ǰ���� */
    hal_pow_get_spec_frame_data_rate_idx(pst_hal_device->st_phy_pow_param.uc_rate_one_pkt, &uc_rate_pow_idx);

    if(WLAN_BAND_2G == pst_mac_vap->st_channel.en_band)
    {
        pst_hal_device->st_phy_pow_param.aul_2g_one_pkt_pow_code[uc_subband_idx]
            = HAL_DEV_GET_PER_RATE_TPC_CODE_BY_LVL(pst_hal_device, pst_mac_vap->st_channel.en_band, uc_rate_pow_idx, uc_pow_level_idx);

        ul_tx_mode |= (pst_hal_device->st_phy_pow_param.aul_2g_one_pkt_pow_code[uc_subband_idx] << 8);
    }
    else
    {
        pst_hal_device->st_phy_pow_param.aul_5g_one_pkt_pow_code[uc_subband_idx]
            = HAL_DEV_GET_PER_RATE_TPC_CODE_BY_LVL(pst_hal_device, pst_mac_vap->st_channel.en_band, uc_rate_pow_idx, uc_pow_level_idx);

        ul_tx_mode |= (pst_hal_device->st_phy_pow_param.aul_5g_one_pkt_pow_code[uc_subband_idx] << 8);
    }

#if 0 //ͳһ����20M������
    /* ������40M������Duplicate Legacy in 40MHz */
    if(WLAN_BAND_WIDTH_40MINUS == pst_mac_vap->st_channel.en_bandwidth || WLAN_BAND_WIDTH_40PLUS == pst_mac_vap->st_channel.en_bandwidth)
    {
        ul_tx_mode |= 0x5 << 3;
    }
#endif

    return ul_tx_mode;
}
#endif


oal_uint32  dmac_fcs_set_prot_datarate(mac_vap_stru *pst_src_vap)
{
    hal_to_dmac_device_stru    *pst_hal_device = OAL_PTR_NULL;

#ifdef _PRE_WLAN_FEATURE_BTCOEX
    hal_to_dmac_chip_stru      *pst_hal_chip;
#endif

    pst_hal_device = DMAC_VAP_GET_HAL_DEVICE(pst_src_vap);
    if(OAL_PTR_NULL == pst_hal_device)
    {
        OAM_ERROR_LOG0(pst_src_vap->uc_vap_id, OAM_SF_ANY, "{dmac_fcs_set_prot_datarate::pst_hal_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��Է�legacy vap����5GƵ��, ����OFDM 6M */
#ifdef _PRE_WLAN_FEATURE_BTCOEX
    pst_hal_chip = DMAC_VAP_GET_HAL_CHIP(pst_src_vap);
    if (OAL_PTR_NULL == pst_hal_chip)
    {
        OAM_ERROR_LOG0(pst_src_vap->uc_vap_id, OAM_SF_ANY, "{dmac_fcs_set_prot_datarate::pst_hal_chip null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (pst_hal_chip->st_btcoex_btble_status.un_bt_status.st_bt_status.bit_bt_on)
    {   /* ������"ͨ��1", �򱣻�ֻ֡��ͨ��1���� */
        if(WLAN_TX_CHAIN_ONE == pst_hal_device->st_cfg_cap_info.uc_phy2dscr_chain)
        {
            return WLAN_PROT_DATARATE_CHN1_24M;
        }
        /* ������"ͨ��0"��"˫ͨ��", �򱣻�ֻ֡��ͨ��0���� */
        else
        {
            return WLAN_PROT_DATARATE_CHN0_24M;
        }
    }
#endif

    if ((!IS_LEGACY_VAP(pst_src_vap)) || (WLAN_BAND_5G == pst_src_vap->st_channel.en_band))
    {
        /* ������"ͨ��1", �򱣻�ֻ֡��ͨ��1���ͣ�51Ҳͳһ�������з�ʽ��ȫ�ֱ����޸�ʱ�򣬱�֤��������ͬ��ˢ�� */
        //if ((1 == g_l_rf_single_tran) && (2 == g_l_rf_channel_num))
        if(WLAN_TX_CHAIN_ONE == pst_hal_device->st_cfg_cap_info.uc_phy2dscr_chain)
        {
            return WLAN_PROT_DATARATE_CHN1_6M;
        }
        /* ������"ͨ��0"��"˫ͨ��", �򱣻�ֻ֡��ͨ��0���� */
        else
        {
            return WLAN_PROT_DATARATE_CHN0_6M;
        }
    }
    /* ����, ����11b 1M */
    else
    {
        /* ������"ͨ��1", �򱣻�ֻ֡��ͨ��1���ͣ�51Ҳͳһ�������з�ʽ��ȫ�ֱ����޸�ʱ�򣬱�֤��������ͬ��ˢ�� */
        //if ((1 == g_l_rf_single_tran) && (2 == g_l_rf_channel_num))
        if(WLAN_TX_CHAIN_ONE == pst_hal_device->st_cfg_cap_info.uc_phy2dscr_chain)
        {
            return WLAN_PROT_DATARATE_CHN1_1M;
        }
        /* ������"ͨ��0"��"˫ͨ��", �򱣻�ֻ֡��ͨ��0���� */
        else
        {
            return WLAN_PROT_DATARATE_CHN0_1M;
        }
    }
}


oal_void  dmac_fcs_set_one_pkt_timeout_time(mac_fcs_mgr_stru *pst_fcs_mgr)
{
    /* ˢ��one pkt��Ӳ����ʱ��ʱ�� */
    if(HAL_FCS_SERVICE_TYPE_BTCOEX == pst_fcs_mgr->en_fcs_service_type)
    {
        /* ����btcoex���������ֳ���������ó�20slot������m2s��null����ʱ��û��Ҫ�� */
        pst_fcs_mgr->pst_fcs_cfg->st_one_packet_cfg.us_timeout = MAC_FCS_DEFAULT_PROTECT_TIME_OUT3;
        pst_fcs_mgr->pst_fcs_cfg->st_one_packet_cfg.us_wait_timeout = MAC_ONE_PACKET_TIME_OUT3;
        pst_fcs_mgr->pst_fcs_cfg->st_one_packet_cfg2.us_timeout = MAC_FCS_DEFAULT_PROTECT_TIME_OUT3;
        pst_fcs_mgr->pst_fcs_cfg->st_one_packet_cfg2.us_wait_timeout = MAC_ONE_PACKET_TIME_OUT3;

    }
    else if(HAL_FCS_SERVICE_TYPE_M2S == pst_fcs_mgr->en_fcs_service_type)
    {
        pst_fcs_mgr->pst_fcs_cfg->st_one_packet_cfg.us_timeout = MAC_FCS_DEFAULT_PROTECT_TIME_OUT4;
        pst_fcs_mgr->pst_fcs_cfg->st_one_packet_cfg.us_wait_timeout = MAC_ONE_PACKET_TIME_OUT4;
        pst_fcs_mgr->pst_fcs_cfg->st_one_packet_cfg2.us_timeout = MAC_FCS_DEFAULT_PROTECT_TIME_OUT3;
        pst_fcs_mgr->pst_fcs_cfg->st_one_packet_cfg2.us_wait_timeout = MAC_ONE_PACKET_TIME_OUT3;
    }
    else
    {
        pst_fcs_mgr->pst_fcs_cfg->st_one_packet_cfg.us_timeout = MAC_FCS_DEFAULT_PROTECT_TIME_OUT;
        pst_fcs_mgr->pst_fcs_cfg->st_one_packet_cfg.us_wait_timeout = MAC_ONE_PACKET_TIME_OUT;
        pst_fcs_mgr->pst_fcs_cfg->st_one_packet_cfg2.us_timeout = MAC_FCS_DEFAULT_PROTECT_TIME_OUT3;
        pst_fcs_mgr->pst_fcs_cfg->st_one_packet_cfg2.us_wait_timeout = MAC_ONE_PACKET_TIME_OUT3;
    }
}


oal_void  dmac_fcs_set_prot_btcoex_priority(hal_to_dmac_device_stru *pst_hal_device,
    hal_one_packet_cfg_stru *pst_one_packet_cfg, oal_bool_enum_uint8 en_open)
{
#ifdef _PRE_WLAN_FEATURE_BTCOEX
    /* ����coex pri����ʱ�Ž��и�ֵ */
    if (OAL_FALSE == pst_hal_device->st_btcoex_sw_preempt.en_coex_pri_forbit)
    {
        if(OAL_TRUE == en_open)
        {
#ifdef _PRE_WLAN_1103_PILOT
            /* �Ĵ������� */
            pst_one_packet_cfg->bit_protect_coex_pri = pst_hal_device->st_btcoex_sw_preempt.en_protect_coex_pri;
#else
            /* ���������one packet���ͳɹ��ʣ���occupy ��pilot��� */
            hal_set_btcoex_occupied_period(pst_hal_device, 50000); //50ms
#endif
        }
        else
        {
#ifdef _PRE_WLAN_1103_PILOT
            /* �Ĵ������� */
#else
            /* ���������one packet���ͳɹ��ʣ��������� */
            hal_set_btcoex_occupied_period(pst_hal_device, 0);
#endif
        }
    }
#endif
}


oal_void  dmac_fcs_prepare_one_packet_cfg(
                mac_vap_stru                *pst_mac_vap,
                hal_one_packet_cfg_stru     *pst_one_packet_cfg,
                oal_uint16                   us_protect_time)
{
    oal_uint32       ul_duration;

    ul_duration = ((oal_uint32)us_protect_time) << 10;

    pst_one_packet_cfg->en_protect_type    = mac_fcs_get_protect_type(pst_mac_vap);
    pst_one_packet_cfg->en_protect_cnt     = mac_fcs_get_protect_cnt(pst_mac_vap);
    pst_one_packet_cfg->ul_tx_data_rate    = dmac_fcs_set_prot_datarate(pst_mac_vap);
    pst_one_packet_cfg->us_duration        = (oal_uint16)OAL_MIN(ul_duration, MAC_FCS_CTS_MAX_DURATION);
    pst_one_packet_cfg->us_timeout         = MAC_FCS_DEFAULT_PROTECT_TIME_OUT;
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)
    pst_one_packet_cfg->ul_tx_mode         = dmac_fcs_get_prot_mode(pst_mac_vap, pst_one_packet_cfg);
#else
    pst_one_packet_cfg->ul_tx_mode         = mac_fcs_get_prot_mode(pst_mac_vap);
#endif

    if (HAL_FCS_PROTECT_TYPE_NULL_DATA == pst_one_packet_cfg->en_protect_type)
    {
        mac_null_data_encap(pst_one_packet_cfg->auc_protect_frame,
                    (WLAN_PROTOCOL_VERSION | WLAN_FC0_TYPE_DATA | WLAN_FC0_SUBTYPE_NODATA | 0x1100),
                    pst_mac_vap->auc_bssid,
                    mac_mib_get_StationID(pst_mac_vap));
    }

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)
    dmac_user_stru  *pst_dmac_user;

    pst_one_packet_cfg->bit_cfg_one_pkt_tx_vap_index = DMAC_VAP_GET_HAL_VAP(pst_mac_vap)->uc_vap_id;

    pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user(pst_mac_vap->us_assoc_vap_id);
    if (OAL_PTR_NULL != pst_dmac_user)
    {
        pst_one_packet_cfg->bit_cfg_one_pkt_tx_peer_index = pst_dmac_user->uc_lut_index;
    }
#endif
}


mac_fcs_err_enum_uint8   dmac_fcs_start( mac_fcs_mgr_stru  *pst_fcs_mgr, mac_fcs_cfg_stru *pst_fcs_cfg,
                                        hal_one_packet_status_stru  *pst_status)
{
    hal_to_dmac_device_stru            *pst_hal_device;
    mac_device_stru                    *pst_mac_device;
#ifdef _PRE_WLAN_FEATURE_BTCOEX
    hal_to_dmac_chip_stru              *pst_hal_chip;
#if 0 //03 wifi��bt����rf���ŵ������²���Ҫ���˲����� ֻ��02FPGA�²���Ҫ
#if (_PRE_WLAN_CHIP_ASIC != _PRE_WLAN_CHIP_VERSION)
    oal_uint32                          ul_mode_sel;
    oal_uint32                          coex_cnt = 0;
#endif
#endif
#endif

    if ((OAL_PTR_NULL == pst_fcs_mgr) || (OAL_PTR_NULL == pst_fcs_cfg))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{dmac_fcs_start::param null.}");
        return  MAC_FCS_ERR_INVALID_CFG;
    }

    pst_mac_device = mac_res_get_dev(pst_fcs_mgr->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{dmac_fcs_start::pst_mac_device null.}");
        return  MAC_FCS_ERR_INVALID_CFG;
    }

#if 0 //03 wifi��bt����rf���ŵ������²���Ҫ���˲����� ֻ��02FPGA�²���Ҫ
#ifdef _PRE_WLAN_FEATURE_BTCOEX
#if (_PRE_WLAN_CHIP_ASIC != _PRE_WLAN_CHIP_VERSION)
    hal_set_btcoex_occupied_period(15000);    // 15ms
    hal_get_btcoex_pa_status(&ul_mode_sel);
    while ((BIT23 == (ul_mode_sel & BIT23)) && (coex_cnt < BT_COEX_RELEASE_TIMEOUT))//wait 10ms for BT release
    {
        oal_udelay(10);
        hal_get_btcoex_pa_status(&ul_mode_sel);
		coex_cnt++;/*10us*/
    }

	if(coex_cnt == BT_COEX_RELEASE_TIMEOUT)
	{
        OAM_WARNING_LOG0(0, OAM_SF_DBAC, "{dmac_fcs_start::bt release timeout!}");
	}
    oal_udelay(50);     // delay 50us
#endif
#endif
#endif

    pst_hal_device = pst_fcs_cfg->pst_hal_device;

    pst_fcs_mgr->pst_fcs_cfg    = pst_fcs_cfg;
    pst_fcs_mgr->en_fcs_state   = MAC_FCS_STATE_IN_PROGESS;

    /* ������װ */
    dmac_fcs_send_one_packet_start(pst_fcs_mgr, &pst_fcs_mgr->pst_fcs_cfg->st_one_packet_cfg, pst_hal_device, pst_status, OAL_TRUE);

#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
    hal_disable_machw_phy_and_pa(pst_hal_device);
    /* 1102��Ҫ��λmac����ֹone packet��btͬʱ������ 1103����Ҫ */
#if defined(_PRE_WLAN_FEATURE_BTCOEX) && defined(_PRE_PRODUCT_ID_HI1102_DEV)
    hal_chip_get_chip(pst_hal_device->uc_chip_id, &pst_hal_chip);
    if(OAL_TRUE == pst_hal_chip->st_btcoex_btble_status.un_bt_status.st_bt_status.bit_bt_on)
    {
        hal_reset_phy_machw(pst_hal_device, HAL_RESET_HW_TYPE_MAC, HAL_RESET_MAC_LOGIC, OAL_FALSE, OAL_FALSE);
    }
#endif
#endif

    /* ����������ж� */
    hal_flush_tx_complete_irq(pst_hal_device);

    /* flush��������¼� */
    mac_fcs_flush_event_by_channel(pst_mac_device, &pst_fcs_cfg->st_src_chl);

    /* ���浱ǰӲ�����е�֡����ٶ��� */
    dmac_tx_save_tx_queue(pst_hal_device, pst_fcs_cfg->pst_src_fake_queue);

    mac_fcs_verify_timestamp(MAC_FCS_STAGE_RESET_HW_START);

    mac_fcs_set_channel(pst_hal_device,  &pst_fcs_cfg->st_dst_chl);
    hal_reset_nav_timer(pst_hal_device);

    hal_clear_hw_fifo(pst_hal_device);
    hal_one_packet_stop(pst_hal_device);

#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
    hal_recover_machw_phy_and_pa(pst_hal_device);
#endif

    mac_fcs_verify_timestamp(MAC_FCS_STAGE_RESET_HW_DONE);

    pst_fcs_mgr->en_fcs_state   = MAC_FCS_STATE_REQUESTED;
    pst_fcs_mgr->pst_fcs_cfg    = OAL_PTR_NULL;

#if 0 //03 wifi��bt����rf���ŵ������²���Ҫ���˲����� ֻ��02FPGA�²���Ҫ
#ifdef _PRE_WLAN_FEATURE_BTCOEX
#if (_PRE_WLAN_CHIP_ASIC != _PRE_WLAN_CHIP_VERSION)
    hal_set_btcoex_occupied_period(0);    // 0us
#endif
#endif
#endif

    return MAC_FCS_SUCCESS;
}


mac_fcs_err_enum_uint8    dmac_fcs_start_enhanced(
                mac_fcs_mgr_stru            *pst_fcs_mgr,
                mac_fcs_cfg_stru            *pst_fcs_cfg)
{
    hal_to_dmac_device_stru            *pst_hal_device;
    mac_device_stru                    *pst_mac_device;
    hal_one_packet_status_stru          st_status;
#ifdef _PRE_WLAN_FEATURE_BTCOEX
    hal_to_dmac_chip_stru              *pst_hal_chip;
#if 0 //03 wifi��bt����rf���ŵ������²���Ҫ���˲����� ֻ��02FPGA�²���Ҫ
#if (_PRE_WLAN_CHIP_ASIC != _PRE_WLAN_CHIP_VERSION)
    oal_uint32                          ul_mode_sel;
    oal_uint32                          coex_cnt = 0;
#endif
#endif
#endif

    if ((OAL_PTR_NULL == pst_fcs_mgr) || (OAL_PTR_NULL == pst_fcs_cfg))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{dmac_fcs_start::param null.}");
        return  MAC_FCS_ERR_INVALID_CFG;
    }

    pst_mac_device = mac_res_get_dev(pst_fcs_mgr->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{dmac_fcs_start::pst_mac_device null.}");
        return  MAC_FCS_ERR_INVALID_CFG;
    }

#if 0 //03 wifi��bt����rf���ŵ������²���Ҫ���˲����� ֻ��02FPGA�²���Ҫ
#ifdef _PRE_WLAN_FEATURE_BTCOEX
#if (_PRE_WLAN_CHIP_ASIC != _PRE_WLAN_CHIP_VERSION)
    hal_set_btcoex_occupied_period(15000);    // 15ms
    hal_get_btcoex_pa_status(&ul_mode_sel);
    while ((BIT23 == (ul_mode_sel & BIT23)) && (coex_cnt < BT_COEX_RELEASE_TIMEOUT))//wait 10ms for BT release
    {
        hal_get_btcoex_pa_status(&ul_mode_sel);
        coex_cnt++;/*10us*/
    }

    if(coex_cnt == BT_COEX_RELEASE_TIMEOUT)
    {
        OAM_WARNING_LOG0(0, OAM_SF_DBAC, "{alg_dbac_fcs_event_handler::bt release timeout!}");
    }
    oal_udelay(50);     // delay 50us
#endif
#endif
#endif

    pst_hal_device = pst_fcs_cfg->pst_hal_device;

    pst_fcs_mgr->pst_fcs_cfg    = pst_fcs_cfg;
    pst_fcs_mgr->en_fcs_state   = MAC_FCS_STATE_IN_PROGESS;

    /* ��һ������one packet */
    pst_fcs_mgr->en_fcs_done    = OAL_FALSE;

    /* �������� */
    hal_one_packet_start(pst_hal_device, &pst_fcs_mgr->pst_fcs_cfg->st_one_packet_cfg);
    mac_fcs_wait_one_packet_done(pst_fcs_mgr);
    hal_one_packet_get_status(pst_hal_device, &st_status);
#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
    hal_disable_machw_phy_and_pa(pst_hal_device);
    /* 1102��Ҫ��λmac����ֹone packet��btͬʱ������ 1103����Ҫ */
#if defined(_PRE_WLAN_FEATURE_BTCOEX) && defined(_PRE_PRODUCT_ID_HI1102_DEV)
    hal_chip_get_chip(pst_hal_device->uc_chip_id, &pst_hal_chip);
    if(OAL_TRUE == pst_hal_chip->st_btcoex_btble_status.un_bt_status.st_bt_status.bit_bt_on)
    {
        hal_reset_phy_machw(pst_hal_device, HAL_RESET_HW_TYPE_MAC, HAL_RESET_MAC_LOGIC, OAL_FALSE, OAL_FALSE);
    }
#endif
#endif

    /* ����������ж� */
    hal_flush_tx_complete_irq(pst_hal_device);

    /* flush��������¼� */
    mac_fcs_flush_event_by_channel(pst_mac_device, &pst_fcs_cfg->st_src_chl);

    dmac_tx_save_tx_queue(pst_hal_device, pst_fcs_cfg->pst_src_fake_queue);

    hal_one_packet_stop(pst_hal_device);
#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
    hal_recover_machw_phy_and_pa(pst_hal_device);
#endif

    /* ��һ������one packetģʽ */
    pst_fcs_mgr->en_fcs_done    = OAL_FALSE;

    hal_one_packet_start(pst_hal_device, &pst_fcs_mgr->pst_fcs_cfg->st_one_packet_cfg2);
    mac_fcs_wait_one_packet_done(pst_fcs_mgr);
    hal_one_packet_get_status(pst_hal_device, &st_status);
#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
    hal_disable_machw_phy_and_pa(pst_hal_device);
    /* 1102��Ҫ��λmac����ֹone packet��btͬʱ������ 1103����Ҫ */
#if defined(_PRE_WLAN_FEATURE_BTCOEX) && defined(_PRE_PRODUCT_ID_HI1102_DEV)
    if(OAL_TRUE == pst_hal_chip->st_btcoex_btble_status.un_bt_status.st_bt_status.bit_bt_on)
    {
        hal_reset_phy_machw(pst_hal_device, HAL_RESET_HW_TYPE_MAC, HAL_RESET_MAC_LOGIC, OAL_FALSE, OAL_FALSE);
    }
#endif
#endif

    mac_fcs_set_channel(pst_hal_device, &pst_fcs_cfg->st_dst_chl);
    hal_reset_nav_timer(pst_hal_device);
    hal_clear_hw_fifo(pst_hal_device);
    hal_one_packet_stop(pst_hal_device);
#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
    hal_recover_machw_phy_and_pa(pst_hal_device);
#endif

    pst_fcs_mgr->en_fcs_state   = MAC_FCS_STATE_REQUESTED;
    pst_fcs_mgr->pst_fcs_cfg    = OAL_PTR_NULL;

#if 0 //03 wifi��bt����rf���ŵ������²���Ҫ���˲����� ֻ��02FPGA�²���Ҫ
#ifdef _PRE_WLAN_FEATURE_BTCOEX
#if (_PRE_WLAN_CHIP_ASIC != _PRE_WLAN_CHIP_VERSION)
    hal_set_btcoex_occupied_period(0);    // 0us
#endif
#endif
#endif

    return MAC_FCS_SUCCESS;
}


oal_uint32  dmac_fcs_wait_one_packet_done_same_channel(mac_fcs_mgr_stru *pst_fcs_mgr)
{
    oal_uint32 ul_delay_cnt = 0;
    oal_uint16 us_timeout   = pst_fcs_mgr->pst_fcs_cfg->st_one_packet_cfg.us_wait_timeout;

    while (OAL_TRUE != pst_fcs_mgr->en_fcs_done)
    {
        /* en_fcs_done will be set 1 in one_packet_done_isr */
        oal_udelay(10);

        ul_delay_cnt++;

        if (ul_delay_cnt > us_timeout)
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "wait one packet done timeout > [%d]us !", us_timeout);
            return OAL_FAIL;
        }
    }

    return OAL_SUCC;
}


oal_void dmac_fcs_send_one_packet_start_same_channel(mac_fcs_mgr_stru *pst_fcs_mgr,
                                            hal_one_packet_cfg_stru *pst_one_packet_cfg,
                                            hal_to_dmac_device_stru *pst_device,
                                         hal_one_packet_status_stru *pst_status,
                                                oal_bool_enum_uint8  en_ps)
{
    oal_uint32                  ul_ret;
    mac_ieee80211_frame_stru   *pst_mac_header;

    /* ׼������ */
    if (HAL_FCS_PROTECT_TYPE_NULL_DATA == pst_one_packet_cfg->en_protect_type)
    {
        pst_mac_header = (mac_ieee80211_frame_stru *)pst_one_packet_cfg->auc_protect_frame;
        pst_mac_header->st_frame_control.bit_power_mgmt = en_ps;
    }

    pst_fcs_mgr->en_fcs_done    = OAL_FALSE;
    mac_fcs_verify_timestamp(MAC_FCS_STAGE_ONE_PKT_START);

    /* �������� */
    hal_one_packet_start(pst_device, pst_one_packet_cfg);
#if ((_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)||(_PRE_OS_VERSION_WIN32_RAW == _PRE_OS_VERSION)) && (_PRE_TEST_MODE == _PRE_TEST_MODE_UT)
    pst_fcs_mgr->en_fcs_done = OAL_TRUE;
#endif

    /* �ȴ����ͽ��� */
    ul_ret = dmac_fcs_wait_one_packet_done_same_channel(pst_fcs_mgr);
    if (OAL_SUCC != ul_ret)
    {
        hal_show_fsm_info(pst_device);
    }

    mac_fcs_verify_timestamp(MAC_FCS_STAGE_ONE_PKT_DONE);
    if (OAL_PTR_NULL != pst_status)
    {
        hal_one_packet_get_status(pst_device, pst_status);
    }
}


mac_fcs_err_enum_uint8   dmac_fcs_start_same_channel(mac_fcs_mgr_stru *pst_fcs_mgr, mac_fcs_cfg_stru *pst_fcs_cfg,
                                        hal_one_packet_status_stru  *pst_status)
{
    hal_to_dmac_device_stru            *pst_hal_device;
    mac_device_stru                    *pst_mac_device;

    if ((OAL_PTR_NULL == pst_fcs_mgr) || (OAL_PTR_NULL == pst_fcs_cfg))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{dmac_fcs_start_same_channel::param null.}");
        return  MAC_FCS_ERR_INVALID_CFG;
    }

    pst_mac_device = mac_res_get_dev(pst_fcs_mgr->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{dmac_fcs_start_same_channel::pst_mac_device null.}");
        return  MAC_FCS_ERR_INVALID_CFG;
    }

    if(pst_fcs_cfg->st_src_chl.uc_chan_number != pst_fcs_cfg->st_dst_chl.uc_chan_number)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{dmac_fcs_start_same_channel:: not the same channel.}");
        return MAC_FCS_ERR_INVALID_CFG;
    }

    pst_hal_device = pst_fcs_cfg->pst_hal_device;

    pst_fcs_mgr->pst_fcs_cfg    = pst_fcs_cfg;
    pst_fcs_mgr->en_fcs_state   = MAC_FCS_STATE_IN_PROGESS;

    /* ������Ӳ����ʱ��ʱ��ʱ�� */
    dmac_fcs_set_one_pkt_timeout_time(pst_fcs_mgr);

    /* ˢ��pri */
    dmac_fcs_set_prot_btcoex_priority(pst_hal_device, &pst_fcs_cfg->st_one_packet_cfg, OAL_TRUE);

    /* ������װ */
    dmac_fcs_send_one_packet_start_same_channel(pst_fcs_mgr, &pst_fcs_cfg->st_one_packet_cfg, pst_hal_device, pst_status, OAL_TRUE);

    /* ����������ж� */
    hal_flush_tx_complete_irq(pst_hal_device);

    /* flush��������¼� */
    mac_fcs_flush_event_by_channel(pst_mac_device, &pst_fcs_cfg->st_src_chl);

    /* ��Ӳ�������е�֡�ŵ���ٶ��У�����m2s�����л����֮�󣬽��з���������ˢ�� */
    dmac_tx_save_tx_queue(pst_hal_device, pst_fcs_cfg->pst_src_fake_queue);

    mac_fcs_verify_timestamp(MAC_FCS_STAGE_RESET_HW_START);

    /* �������Ӳ����������֮֡����Ҫ��fifo */
    hal_clear_hw_fifo(pst_hal_device);

    /* �ָ�one pkt���ָ����� */
    hal_one_packet_stop(pst_hal_device);

    /* ˢ��pri */
    dmac_fcs_set_prot_btcoex_priority(pst_hal_device, &pst_fcs_cfg->st_one_packet_cfg, OAL_FALSE);

    mac_fcs_verify_timestamp(MAC_FCS_STAGE_RESET_HW_DONE);

    pst_fcs_mgr->en_fcs_state   = MAC_FCS_STATE_REQUESTED;
    pst_fcs_mgr->pst_fcs_cfg    = OAL_PTR_NULL;

    return MAC_FCS_SUCCESS;
}


mac_fcs_err_enum_uint8    dmac_fcs_start_enhanced_same_channel(
                mac_fcs_mgr_stru            *pst_fcs_mgr,
                mac_fcs_cfg_stru            *pst_fcs_cfg)
{
    hal_to_dmac_device_stru            *pst_hal_device;
    mac_device_stru                    *pst_mac_device;
    hal_one_packet_status_stru          st_status;

    if ((OAL_PTR_NULL == pst_fcs_mgr) || (OAL_PTR_NULL == pst_fcs_cfg))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{dmac_fcs_start_enhanced_same_channel::param null.}");
        return  MAC_FCS_ERR_INVALID_CFG;
    }

    pst_mac_device = mac_res_get_dev(pst_fcs_mgr->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{dmac_fcs_start_enhanced_same_channel::pst_mac_device null.}");
        return  MAC_FCS_ERR_INVALID_CFG;
    }

    if(pst_fcs_cfg->st_src_chl.uc_chan_number != pst_fcs_cfg->st_dst_chl.uc_chan_number)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{dmac_fcs_start_enhanced_same_channel:: not the same channel.}");
        return MAC_FCS_ERR_INVALID_CFG;
    }

    pst_hal_device = pst_fcs_cfg->pst_hal_device;

    pst_fcs_mgr->pst_fcs_cfg    = pst_fcs_cfg;
    pst_fcs_mgr->en_fcs_state   = MAC_FCS_STATE_IN_PROGESS;

    /* ������Ӳ����ʱ��ʱ��ʱ�� */
    dmac_fcs_set_one_pkt_timeout_time(pst_fcs_mgr);

    /* ��һ������one packet */
    pst_fcs_mgr->en_fcs_done    = OAL_FALSE;

    /* ˢ��pri */
    dmac_fcs_set_prot_btcoex_priority(pst_hal_device, &pst_fcs_cfg->st_one_packet_cfg, OAL_TRUE);

    /* �������� */
    hal_one_packet_start(pst_hal_device, &pst_fcs_cfg->st_one_packet_cfg);
    dmac_fcs_wait_one_packet_done_same_channel(pst_fcs_mgr);
    hal_one_packet_get_status(pst_hal_device, &st_status);
    if (HAL_FCS_PROTECT_TYPE_NULL_DATA == pst_fcs_cfg->st_one_packet_cfg.en_protect_type  && !st_status.en_null_data_success)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{dmac_fcs_start_enhanced_same_channel::null data failed, sending chan:%d}",
                         pst_fcs_cfg->st_src_chl.uc_chan_number);
    }

    /* ����������ж� */
    hal_flush_tx_complete_irq(pst_hal_device);

    /* flush��������¼� */
    mac_fcs_flush_event_by_channel(pst_mac_device, &pst_fcs_cfg->st_src_chl);

    dmac_tx_save_tx_queue(pst_hal_device, pst_fcs_cfg->pst_src_fake_queue);

    /* �������Ӳ����������֮֡����Ҫ��fifo */
    hal_clear_hw_fifo(pst_hal_device);

    /* �ָ�one pkt���ָ����� */
    hal_one_packet_stop(pst_hal_device);

    /* ��һ������one packetģʽ */
    pst_fcs_mgr->en_fcs_done    = OAL_FALSE;

    /* ˢ��pri */
    dmac_fcs_set_prot_btcoex_priority(pst_hal_device, &pst_fcs_cfg->st_one_packet_cfg, OAL_TRUE);

    hal_one_packet_start(pst_hal_device, &pst_fcs_cfg->st_one_packet_cfg2);
    dmac_fcs_wait_one_packet_done_same_channel(pst_fcs_mgr);
    hal_one_packet_get_status(pst_hal_device, &st_status);
    if (HAL_FCS_PROTECT_TYPE_NULL_DATA == pst_fcs_cfg->st_one_packet_cfg2.en_protect_type  && !st_status.en_null_data_success)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{dmac_fcs_start_enhanced_same_channel::null data failed, sending chan:%d}",
                         pst_fcs_cfg->st_src_chl.uc_chan_number);
    }

    hal_one_packet_stop(pst_hal_device);

    /* ˢ��pri */
    dmac_fcs_set_prot_btcoex_priority(pst_hal_device, &pst_fcs_cfg->st_one_packet_cfg, OAL_FALSE);

    pst_fcs_mgr->en_fcs_state   = MAC_FCS_STATE_REQUESTED;
    pst_fcs_mgr->pst_fcs_cfg    = OAL_PTR_NULL;

    return MAC_FCS_SUCCESS;
}

oal_uint32 mac_fcs_notify_chain_register(mac_fcs_mgr_stru               *pst_fcs_mgr,
                                         mac_fcs_notify_type_enum_uint8  uc_notify_type,
                                         mac_fcs_hook_id_enum_uint8      en_hook_id,
                                         mac_fcs_notify_func             p_func)
{
    oal_uint    ul_irq_save;

    if ((uc_notify_type >= MAC_FCS_NOTIFY_TYPE_BUTT) || (en_hook_id >= MAC_FCS_HOOK_ID_BUTT))
    {
        return OAL_FAIL;
    }

    // NOTE:can register new func before old one unregistered
    oal_spin_lock_irq_save(&pst_fcs_mgr->st_lock, &ul_irq_save);
    pst_fcs_mgr->ast_notify_chain[uc_notify_type].ast_notify_nodes[en_hook_id].p_func = p_func;
    oal_spin_unlock_irq_restore(&pst_fcs_mgr->st_lock, &ul_irq_save);

    return  OAL_SUCC;
}

oal_uint32 mac_fcs_notify_chain_unregister(mac_fcs_mgr_stru               *pst_fcs_mgr,
                                           mac_fcs_notify_type_enum_uint8  uc_notify_type,
                                           mac_fcs_hook_id_enum_uint8      en_hook_id)
{
    oal_uint    ul_irq_save;

    if ((uc_notify_type >= MAC_FCS_NOTIFY_TYPE_BUTT) || (en_hook_id >= MAC_FCS_HOOK_ID_BUTT))
    {
        return OAL_FAIL;
    }

    oal_spin_lock_irq_save(&pst_fcs_mgr->st_lock, &ul_irq_save);
    pst_fcs_mgr->ast_notify_chain[uc_notify_type].ast_notify_nodes[en_hook_id].p_func = OAL_PTR_NULL;
    oal_spin_unlock_irq_restore(&pst_fcs_mgr->st_lock, &ul_irq_save);

    return OAL_SUCC;
}

oal_uint32 mac_fcs_notify_chain_destroy(mac_fcs_mgr_stru *pst_fcs_mgr)
{
    oal_uint8   uc_idx;
    oal_uint    ul_irq_save;

    oal_spin_lock_irq_save(&pst_fcs_mgr->st_lock, &ul_irq_save);

    for (uc_idx = 0; uc_idx < MAC_FCS_HOOK_ID_BUTT; uc_idx++)
    {
        mac_fcs_notify_chain_init(pst_fcs_mgr->ast_notify_chain + uc_idx);
    }

    oal_spin_unlock_irq_restore(&pst_fcs_mgr->st_lock, &ul_irq_save);

    return OAL_SUCC;
}

/*lint -e578*//*lint -e19*/
oal_module_symbol(mac_fcs_set_channel);
oal_module_symbol(dmac_fcs_set_prot_datarate);
oal_module_symbol(dmac_fcs_prepare_one_packet_cfg);
oal_module_symbol(dmac_fcs_start_enhanced);
oal_module_symbol(dmac_fcs_start);
oal_module_symbol(dmac_fcs_start_same_channel);
oal_module_symbol(dmac_fcs_start_enhanced_same_channel);
oal_module_symbol(mac_fcs_notify_chain_register);
oal_module_symbol(mac_fcs_notify_chain_unregister);
oal_module_symbol(mac_fcs_notify_chain_destroy);

#if (_PRE_TEST_MODE_BOARD_ST == _PRE_TEST_MODE)

OAL_STATIC mac_fcs_verify_stat_stru   g_st_fcs_verify_stat;


oal_void mac_fcs_verify_init(oal_void)
{
    OAL_MEMZERO(&g_st_fcs_verify_stat, OAL_SIZEOF(g_st_fcs_verify_stat));

#if defined(_PRE_PRODUCT_ID_HI110X_DEV)

#else
    oal_5115timer_init();
#endif
}


oal_void mac_fcs_verify_start(oal_void)
{
    OAL_MEMZERO(&g_st_fcs_verify_stat, OAL_SIZEOF(g_st_fcs_verify_stat));

    g_st_fcs_verify_stat.en_enable = OAL_TRUE;

#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
#if (!defined(_PRE_PC_LINT) && !defined(WIN32))
    enable_cycle_counter();
#endif
#endif

}


oal_void mac_fcs_verify_timestamp(mac_fcs_stage_enum_uint8 en_stage)
{
    if(g_st_fcs_verify_stat.en_enable)
    {
        if(g_st_fcs_verify_stat.ul_item_cnt < MAC_FCS_VERIFY_MAX_ITEMS)
        {
#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
#if (!defined(_PRE_PC_LINT) && !defined(WIN32))
            g_st_fcs_verify_stat.aul_timestamp[g_st_fcs_verify_stat.ul_item_cnt][en_stage] = get_cycle_count();
#endif
#else
            g_st_fcs_verify_stat.aul_timestamp[g_st_fcs_verify_stat.ul_item_cnt][en_stage] = ~0 - oal_5115timer_get_10ns();
#endif
        }
        else
        {
            mac_fcs_verify_stop();
        }

        if(en_stage == MAC_FCS_STAGE_COUNT-1)
        {
            g_st_fcs_verify_stat.ul_item_cnt++;
        }
    }
}


oal_void mac_fcs_verify_stop(oal_void)
{
    oal_uint16  us_item_idx;
    oal_uint16  us_stage_idx;

    g_st_fcs_verify_stat.en_enable = OAL_FALSE;

#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
#if (!defined(_PRE_PC_LINT) && !defined(WIN32))
    disable_cycle_counter();
#endif
#endif

    for(us_item_idx = 0; us_item_idx < g_st_fcs_verify_stat.ul_item_cnt; us_item_idx++)
    {
        for(us_stage_idx = 0; us_stage_idx < MAC_FCS_STAGE_COUNT; us_stage_idx++)
        {
            oal_uint32 *pst_tmp = g_st_fcs_verify_stat.aul_timestamp[us_item_idx];

#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
            OAL_REFERENCE(pst_tmp);
            OAL_IO_PRINT("%d ", us_stage_idx == 0 ? 0 : (pst_tmp[us_stage_idx] - pst_tmp[us_stage_idx-1])/80);
#else
            OAL_IO_PRINT("%d ", us_stage_idx == 0 ? pst_tmp[us_stage_idx] : pst_tmp[us_stage_idx] - pst_tmp[us_stage_idx-1]);
#endif
        }

#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
        OAL_IO_PRINT(" sum=%d us\r\n", (g_st_fcs_verify_stat.aul_timestamp[us_item_idx][MAC_FCS_STAGE_EVENT_DONE] - g_st_fcs_verify_stat.aul_timestamp[us_item_idx][MAC_FCS_STAGE_INTR_START])/80);
#else
        OAL_IO_PRINT(" sum=%d\r\n", g_st_fcs_verify_stat.aul_timestamp[us_item_idx][MAC_FCS_STAGE_EVENT_DONE] - g_st_fcs_verify_stat.aul_timestamp[us_item_idx][MAC_FCS_STAGE_INTR_START]);
#endif

    }
}
/*lint -e19 */
oal_module_symbol(mac_fcs_verify_start);
oal_module_symbol(mac_fcs_verify_timestamp);
oal_module_symbol(mac_fcs_verify_stop);
#endif



#ifdef  __cplusplus
#if     __cplusplus
    }
#endif
#endif
