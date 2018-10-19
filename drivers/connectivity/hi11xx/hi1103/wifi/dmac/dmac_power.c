


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 ͷ�ļ�����
*****************************************************************************/
#include "dmac_power.h"
#include "dmac_alg.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_POWER_C
/*****************************************************************************
  2 ȫ�ֱ�������
*****************************************************************************/


/*****************************************************************************
  3 ����ʵ��
*****************************************************************************/


oal_uint32  dmac_pow_get_user_target_tx_power(mac_user_stru *pst_mac_user,
                                 hal_tx_dscr_ctrl_one_param *pst_tx_dscr_param, oal_int16 *ps_tx_pow)
{
    mac_vap_stru                        *pst_mac_vap           = OAL_PTR_NULL;
    hal_to_dmac_device_stru             *pst_hal_device        = OAL_PTR_NULL;
    hal_user_pow_info_stru              *pst_hal_user_pow_info = OAL_PTR_NULL;

    oal_uint8                            uc_output_rate_index  = 0;
    oal_uint8                            uc_cur_rate_pow_idx   = 0;

    oal_int16                            s_tx_pow;
    oal_uint32                           ul_ret;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_user
       ||  OAL_PTR_NULL == pst_tx_dscr_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY,
                       "{dmac_power_get_user_target_tx_power:: ERROR INFO: pst_mac_user=0x%p, pst_tx_dscr_param=0x%p.}",
                       pst_mac_user, pst_tx_dscr_param);

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_vap = mac_res_get_mac_vap(pst_mac_user->uc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
    {
         OAM_ERROR_LOG0(0, OAM_SF_ANY, "{dmac_power_get_user_target_tx_power: mac_res_get_mac_vap fail, pst_mac_vap is NULL!}");
         return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hal_device = DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hal_device))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{dmac_power_get_user_target_tx_power::pst_hal_device is NULL}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hal_user_pow_info = DMAC_USER_GET_POW_INFO(pst_mac_user);

    /* ��ȡ����������vap rate_pow code table�е�������,ʧ�ܵĻ�����Ϊ��ȡ��������ʧ�ܣ�ֱ�ӷ��� */
    ul_ret = dmac_alg_get_vap_rate_idx_for_tx_power(pst_mac_user, pst_tx_dscr_param, &uc_output_rate_index ,&uc_cur_rate_pow_idx);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{dmac_power_get_user_target_tx_power: alg_autorate_get_vap_rate_index_for_tx_power fail!}");
        return OAL_FAIL;
    }

    /* ��ȡ���� */
    hal_device_get_tx_pow_from_rate_idx(pst_hal_device, pst_hal_user_pow_info, pst_mac_vap->st_channel.en_band,
                                pst_mac_vap->st_channel.uc_chan_number, uc_cur_rate_pow_idx,
                                &(pst_tx_dscr_param->st_tx_power), &s_tx_pow);

    *ps_tx_pow = s_tx_pow;

    return OAL_SUCC;
}


oal_uint32  dmac_pow_tx_power_process(dmac_vap_stru *pst_dmac_vap, mac_user_stru *pst_user,
                                        mac_tx_ctl_stru *pst_cb, hal_tx_txop_alg_stru *pst_txop_param)
{
#ifndef WIN32
    oal_uint8                           uc_rate_level_idx;
    oal_uint32                          ul_ret = OAL_SUCC;
    oal_uint8                           uc_rate_to_pow_code_idx = 0;
    oal_uint8                           uc_pow_level;
    oal_bool_enum_uint8                 en_ar_enable = OAL_FALSE;
    oal_uint8                           auc_ar_rate_idx[HAL_TX_RATE_MAX_NUM];
    hal_user_pow_info_stru              *pst_user_pow_info = OAL_PTR_NULL;
    hal_to_dmac_device_stru             *pst_hal_device    = OAL_PTR_NULL;
    oal_uint8                           uc_protocol;
    hal_channel_assemble_enum_uint8     en_bw;
    oal_uint32                          aul_pow_code[HAL_TX_RATE_MAX_NUM];
    oal_uint8                           auc_pow_level[HAL_TX_RATE_MAX_NUM] = {HAL_POW_MAX_POW_LEVEL};//�ĸ����ʵȼ�����ʼ��Ϊ����ʵ�λ

    /* �ж�����Ƿ�Ϊ�� */
    if (OAL_UNLIKELY((NULL == (pst_dmac_vap)) || (NULL == (pst_user)) || (NULL == (pst_cb)) || (NULL == (pst_txop_param))))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{dmac_pow_tx_power_process::input pointer is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* �ಥ֡������֡�㷨�����ù��� */
    if ((OAL_TRUE == MAC_GET_CB_IS_MCAST(pst_cb)) ||
        (!(MAC_GET_CB_IS_DATA_FRAME(pst_cb))) ||
            (DMAC_USER_ALG_SMARTANT_NULLDATA_PROBE == MAC_GET_CB_IS_PROBE_DATA(pst_cb)))
    {
        return OAL_SUCC;
    }

    pst_hal_device = DMAC_VAP_GET_HAL_DEVICE(pst_dmac_vap);

    /* ����RF�Ĵ�����ʱ�����ù��� */
    if(OAL_TRUE == pst_hal_device->en_pow_rf_reg_ctl_flag)
    {
        return OAL_SUCC;
    }

    pst_user_pow_info = DMAC_USER_GET_POW_INFO(pst_user);
    pst_user_pow_info->pst_txop_param = pst_txop_param;
    if (OAL_PTR_NULL == DMAC_USER_GET_POW_TABLE(pst_user))
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{dmac_pow_tx_power_process::user_get_pow_table is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ֪ͨ�㷨����ȡAutorate״̬ */
    ul_ret = dmac_alg_get_user_rate_idx_for_tx_power(pst_user, &en_ar_enable, auc_ar_rate_idx);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY,
                       "{dmac_pow_tx_power_process::dmac_alg_get_rate_idx_for_tx_power fail ul_ret[%d]!}", ul_ret);
        return ul_ret;
    }

    /* ����֡����, ���õ�һ�����ʵȼ����������������ʵȼ������������ */
    auc_pow_level[0] = pst_hal_device->uc_fix_power_level;

    /*��ȡ����֡ÿһ���ȼ����ʵĹ�����Ϣ*/
    for (uc_rate_level_idx = 0; uc_rate_level_idx < HAL_TX_RATE_MAX_NUM; uc_rate_level_idx++)
    {
        /*1. ��ȡĳ���ʵȼ�����������*/
        if (OAL_FALSE == en_ar_enable)
        {
            uc_protocol = pst_txop_param->ast_per_rate[0].rate_bit_stru.un_nss_rate.st_vht_nss_mcs.bit_protocol_mode;
            en_bw       = pst_txop_param->st_rate.en_channel_bandwidth;
            hal_device_get_fix_rate_pow_code_idx(uc_protocol, g_auc_bw_idx[en_bw], &(pst_txop_param->ast_per_rate[0]), &uc_rate_to_pow_code_idx, 0xFF);
        }
        else
        {
            uc_rate_to_pow_code_idx = auc_ar_rate_idx[uc_rate_level_idx];
        }

        /*2. ��ȡ�����ʵĹ��ʵȼ�*/
        uc_pow_level = auc_pow_level[uc_rate_level_idx];

        /*3. ��ȡĳ���ʲ�ͬ���ʵȼ���pow code*/
        aul_pow_code[uc_rate_level_idx] = (oal_uint32)HAL_POW_GET_POW_CODE_FROM_TABLE(pst_user_pow_info->pst_rate_pow_table, uc_rate_to_pow_code_idx, uc_pow_level)

    }

    /* ���,��д�������������� */
    hal_pow_set_pow_code_idx_in_tx_power(&(pst_txop_param->st_tx_power), aul_pow_code);
#endif

    return OAL_SUCC;

}


oal_uint32 dmac_pow_set_vap_spec_frame_tx_power(dmac_vap_stru *pst_dmac_vap)
{
#ifndef WIN32
    hal_to_dmac_vap_stru                    *pst_hal_vap        = OAL_PTR_NULL;
    hal_to_dmac_device_stru                 *pst_hal_device     = OAL_PTR_NULL;
    hal_rate_pow_code_gain_table_stru       *pst_rate_pow_table = OAL_PTR_NULL;
    oal_uint8                                uc_rate_pow_idx = 0;
    oal_uint8                                uc_data_rate;
    oal_uint8                                uc_band;
    wlan_vap_mode_enum_uint8                 en_vap_mode;
    oal_uint8                                uc_input_pow_level_idx = 0;
    oal_uint8                                uc_pow_level_idx;
    hal_tx_txop_tx_power_stru               *pst_tx_power;
    oal_uint32                               ul_temp_pow_code  = 0;

    pst_hal_vap  = pst_dmac_vap->pst_hal_vap;
    uc_band      = pst_dmac_vap->st_vap_base_info.st_channel.en_band;
    en_vap_mode  = pst_dmac_vap->st_vap_base_info.en_vap_mode;

    pst_hal_device = DMAC_VAP_GET_HAL_DEVICE(pst_dmac_vap);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hal_device))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{dmac_pow_set_vap_spec_frame_tx_power:pst_hal_device null!}");
        return OAL_FAIL;
    }

    /* ��ȡ����֡�ȼ� */
    uc_pow_level_idx = pst_hal_device->uc_mag_mcast_frm_power_level;
    pst_rate_pow_table = DMAC_VAP_GET_POW_TABLE(pst_dmac_vap);
    if(OAL_PTR_NULL == pst_rate_pow_table)
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{dmac_pow_set_vap_spec_frame_tx_power:: get pst_rate_pow_table is NULL ! }");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* RF LIMIT power��Ҫ����Max Power������ǿ����˳�ʼpow code��Ҫʹ��max power��POW code */
    if(HAL_POW_RF_LIMIT_POW_LEVEL == uc_pow_level_idx)
    {
        uc_input_pow_level_idx = uc_pow_level_idx;
        uc_pow_level_idx       = HAL_POW_MAX_POW_LEVEL;
    }

    /* ����Beacon֡����ģʽ(��POW code), 2G 1M, 5G 6M */
    if (WLAN_VAP_MODE_BSS_AP == en_vap_mode)
    {
        hal_get_bcn_rate(pst_hal_vap, &uc_data_rate);
        hal_pow_get_spec_frame_data_rate_idx(uc_data_rate,&uc_rate_pow_idx);
        ul_temp_pow_code = (oal_uint32)HAL_POW_GET_POW_CODE_FROM_TABLE(pst_rate_pow_table, uc_rate_pow_idx, uc_pow_level_idx);
        hal_set_bcn_phy_tx_mode(pst_hal_vap, ul_temp_pow_code);
    }

    /* ���õ�������֡����,2G 1M, 5G 6M */
    uc_data_rate = HAL_POW_GET_LEGACY_RATE(&(pst_dmac_vap->ast_tx_mgmt_ucast[uc_band].ast_per_rate[0]));
    hal_pow_get_spec_frame_data_rate_idx(uc_data_rate, &uc_rate_pow_idx);

    pst_tx_power = (hal_tx_txop_tx_power_stru *)(&(pst_dmac_vap->ast_tx_mgmt_ucast[uc_band].st_tx_power));
    ul_temp_pow_code = (oal_uint32)HAL_POW_GET_POW_CODE_FROM_TABLE(pst_rate_pow_table, uc_rate_pow_idx, uc_pow_level_idx);
    HAL_POW_SET_RF_LIMIT_POW(uc_input_pow_level_idx, ul_temp_pow_code);
    hal_pow_set_pow_code_idx_same_in_tx_power(pst_tx_power, ul_temp_pow_code);

    /* ���ù㲥/�鲥����֡����,2G 1M, 5G 6M  */
    uc_data_rate = HAL_POW_GET_LEGACY_RATE(&(pst_dmac_vap->ast_tx_mgmt_bmcast[uc_band].ast_per_rate[0]));
    hal_pow_get_spec_frame_data_rate_idx(uc_data_rate, &uc_rate_pow_idx);

    pst_tx_power  = (hal_tx_txop_tx_power_stru *)(&(pst_dmac_vap->ast_tx_mgmt_bmcast[uc_band].st_tx_power));
    ul_temp_pow_code = (oal_uint32)HAL_POW_GET_POW_CODE_FROM_TABLE(pst_rate_pow_table, uc_rate_pow_idx, uc_pow_level_idx);
    HAL_POW_SET_RF_LIMIT_POW(uc_input_pow_level_idx, ul_temp_pow_code);
    hal_pow_set_pow_code_idx_same_in_tx_power(pst_tx_power, ul_temp_pow_code);

    /*ע:�ڣӣԣ�ģʽ��ɨ���ŵ�ʱ�����ڳ�ʼ���ŵ�Ϊ2.4G channel1��ʱ����뱾������
    �ᵼ�º���5G�ŵ�ɨ��ʱ��û�����ö�Ӧ���ʵĹ��ʣ�����ڸó�����ͬʱ����5G����*/
    if((WLAN_VAP_MODE_BSS_STA == en_vap_mode) && (WLAN_BAND_2G == uc_band))
    {
        /* ���õ�������֡����,2G 1M, 5G 6M */
        uc_data_rate = HAL_POW_GET_LEGACY_RATE(&(pst_dmac_vap->ast_tx_mgmt_ucast[WLAN_BAND_5G].ast_per_rate[0]));
        hal_pow_get_spec_frame_data_rate_idx(uc_data_rate, &uc_rate_pow_idx);

        pst_tx_power  = (hal_tx_txop_tx_power_stru *)(&(pst_dmac_vap->ast_tx_mgmt_ucast[WLAN_BAND_5G].st_tx_power));
        ul_temp_pow_code = (oal_uint32)HAL_POW_GET_POW_CODE_FROM_TABLE(pst_rate_pow_table, uc_rate_pow_idx, uc_pow_level_idx);
        HAL_POW_SET_RF_LIMIT_POW(uc_input_pow_level_idx, ul_temp_pow_code);
        hal_pow_set_pow_code_idx_same_in_tx_power(pst_tx_power, ul_temp_pow_code);

        /* ���ù㲥/�鲥����֡����,2G 1M, 5G 6M  */
        uc_data_rate = HAL_POW_GET_LEGACY_RATE(&(pst_dmac_vap->ast_tx_mgmt_bmcast[WLAN_BAND_5G].ast_per_rate[0]));
        hal_pow_get_spec_frame_data_rate_idx(uc_data_rate, &uc_rate_pow_idx);

        pst_tx_power  = (hal_tx_txop_tx_power_stru *)(&(pst_dmac_vap->ast_tx_mgmt_bmcast[WLAN_BAND_5G].st_tx_power));
        ul_temp_pow_code = (oal_uint32)HAL_POW_GET_POW_CODE_FROM_TABLE(pst_rate_pow_table, uc_rate_pow_idx, uc_pow_level_idx);
        HAL_POW_SET_RF_LIMIT_POW(uc_input_pow_level_idx, ul_temp_pow_code);
        hal_pow_set_pow_code_idx_same_in_tx_power(pst_tx_power, ul_temp_pow_code);
    }

    /* �����鲥����֡���� 1M, 6M, 24M*/
    uc_data_rate = HAL_POW_GET_LEGACY_RATE(&(pst_dmac_vap->st_tx_data_mcast.ast_per_rate[0]));
    hal_pow_get_spec_frame_data_rate_idx(uc_data_rate, &uc_rate_pow_idx);

    pst_tx_power  = (hal_tx_txop_tx_power_stru *)(&(pst_dmac_vap->st_tx_data_mcast.st_tx_power));
    ul_temp_pow_code = (oal_uint32)HAL_POW_GET_POW_CODE_FROM_TABLE(pst_rate_pow_table, uc_rate_pow_idx, uc_pow_level_idx);
    hal_pow_set_pow_code_idx_same_in_tx_power(pst_tx_power, ul_temp_pow_code);

    /*���ù㲥����֡ 1M, 6M, 24M*/
    uc_data_rate = HAL_POW_GET_LEGACY_RATE(&(pst_dmac_vap->st_tx_data_bcast.ast_per_rate[0]));
    hal_pow_get_spec_frame_data_rate_idx(uc_data_rate, &uc_rate_pow_idx);

    pst_tx_power  = (hal_tx_txop_tx_power_stru *)(&(pst_dmac_vap->st_tx_data_bcast.st_tx_power));
    ul_temp_pow_code = (oal_uint32)HAL_POW_GET_POW_CODE_FROM_TABLE(pst_rate_pow_table, uc_rate_pow_idx, uc_pow_level_idx);
    hal_pow_set_pow_code_idx_same_in_tx_power(pst_tx_power, ul_temp_pow_code);

    //oal_dump_stack();
#endif
    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_USER_RESP_POWER

oal_uint32  dmac_pow_change_mgmt_power_process(mac_vap_stru *pst_vap, oal_int8 c_rssi)
{
    hal_to_dmac_device_stru                 *pst_hal_device         = OAL_PTR_NULL;
    hal_vap_pow_info_stru                   *pst_hal_vap_pow_info   = OAL_PTR_NULL;
    oal_uint32                               ul_ret = OAL_SUCC;

    /* �����û�����Ĺ��ʵ��� */
    if (0 == pst_vap->us_user_nums)
    {
        return OAL_SUCC;
    }

    pst_hal_device = DMAC_VAP_GET_HAL_DEVICE(pst_vap);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hal_device))
    {
        OAM_ERROR_LOG0(pst_vap->uc_vap_id, OAM_SF_PWR, "{dmac_pow_change_mgmt_power_process::pst_hal_device null!}");
        return OAL_FAIL;
    }

    /* ���TPC�㷨δʹ���������ʸ��� */
    if (OAL_TRUE == pst_hal_device->en_pow_rf_reg_ctl_flag)
    {
        return OAL_SUCC;
    }

    /* ��ȡvap��������Ϣ�ṹ�� */
    pst_hal_vap_pow_info = DMAC_VAP_GET_POW_INFO(pst_vap);

    /* �Ȼָ��������ķ��书�� */
    pst_hal_device->uc_mag_mcast_frm_power_level = HAL_POW_MAX_POW_LEVEL;
    pst_hal_device->uc_control_frm_power_level   = HAL_POW_MAX_POW_LEVEL;

    /* �ж�RSSI�Ƿ��ں�����ϱ���Χ */
    if ((OAL_RSSI_SIGNAL_MIN < c_rssi) && (OAL_RSSI_SIGNAL_MAX > c_rssi))
    {
        if(WLAN_CLOSE_DISTANCE_RSSI < c_rssi)
        {
            /*���÷�����֡�ķ��͹���*/
            pst_hal_device->uc_mag_mcast_frm_power_level = HAL_POW_MIN_POW_LEVEL;
            pst_hal_device->uc_control_frm_power_level   = HAL_POW_MIN_POW_LEVEL;
        }
        else if(WLAN_FAR_DISTANCE_RSSI > c_rssi)
        {
            /*���÷�����֡�ķ��͹���*/
            pst_hal_device->uc_mag_mcast_frm_power_level = HAL_POW_RF_LIMIT_POW_LEVEL;
            pst_hal_device->uc_control_frm_power_level   = HAL_POW_RF_LIMIT_POW_LEVEL;
        }
    }

    /* ���ÿ���֡�ķ��书�� */
    hal_pow_set_band_spec_frame_tx_power(pst_hal_device, pst_hal_device->st_wifi_channel_status.en_band,
                                         pst_hal_device->st_wifi_channel_status.uc_chan_idx, pst_hal_vap_pow_info->pst_rate_pow_table);

    ul_ret = dmac_pow_set_vap_spec_frame_tx_power(MAC_GET_DMAC_VAP(pst_vap));
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_vap->uc_vap_id, OAM_SF_PWR, "{dmac_pow_change_mgmt_power_process::set vap spec frame failed[%d].", ul_ret);
        return ul_ret;
    }

    /* ά�⣬����rx mgmt��rssi��ȷ�ϻظ��Ĺ���֡�Ĺ��� */
    OAM_INFO_LOG2(pst_vap->uc_vap_id, OAM_SF_PWR, "{dmac_pow_change_mgmt_power_process::mgmt rx's RSSI = %d dBm, management power level = %d.}",
                      c_rssi, pst_hal_device->uc_mag_mcast_frm_power_level);

    return OAL_SUCC;
}
#endif


oal_void dmac_pow_set_vap_tx_power(mac_vap_stru *pst_mac_vap, hal_pow_set_type_enum_uint8 uc_type)
{
#ifndef WIN32
    dmac_vap_stru                       *pst_dmac_vap       = OAL_PTR_NULL;
    hal_to_dmac_device_stru             *pst_hal_device     = OAL_PTR_NULL;
    hal_vap_pow_info_stru               *pst_vap_pow_info   = OAL_PTR_NULL;
    mac_regclass_info_stru              *pst_regdom_info;
    oal_uint8                            uc_protocol;
    oal_uint8                            uc_cur_ch_num;
    wlan_channel_band_enum_uint8         en_freq_band;
    wlan_channel_bandwidth_enum_uint8    en_bandwidth;
    oal_uint8                            uc_chan_idx;
    mac_channel_stru                    *pst_channel;                                     /* vap���ڵ��ŵ� */

    pst_dmac_vap = (dmac_vap_stru*)pst_mac_vap;
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{dmac_pow_set_vap_tx_power::pst_dmac_vap null}");
        return;
    }

    pst_hal_device = DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap);
    if (OAL_PTR_NULL == pst_hal_device)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{dmac_pow_set_vap_tx_power::pst_hal_device null}");
        return;
    }

    /* ��ȡvap��������Ϣ�ṹ�� */
    pst_vap_pow_info = DMAC_VAP_GET_POW_INFO(pst_mac_vap);

    if(HAL_POW_SET_TYPE_INIT == uc_type)
    {
        /* ���Կ���Ĭ�Ϲر� */
        pst_dmac_vap->st_vap_pow_info.en_debug_flag = OAL_FALSE;
    }

    pst_channel = &pst_mac_vap->st_channel;
    /*��ȡ�ŵ��ţ�����2G/5G*/
    uc_chan_idx   = pst_channel->uc_chan_idx;
    uc_cur_ch_num = pst_channel->uc_chan_number;
    en_bandwidth  = pst_channel->en_bandwidth;
    en_freq_band  = pst_channel->en_band;

#ifndef _PRE_WLAN_FEATURE_TPC_OPT
    if (0 == uc_cur_ch_num)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{dmac_pow_set_vap_tx_power::uc_cur_ch_num is [%d].}", uc_cur_ch_num);
        return;
    }
#endif

    /* ����RF�Ĵ�����ʱ�������ʸ��� */
    if (OAL_TRUE == pst_hal_device->en_pow_rf_reg_ctl_flag)
    {
        /* �������ŵ�����Ҫ���¼Ĵ������� */
        if(HAL_POW_SET_TYPE_REFRESH == uc_type)
        {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{dmac_pow_set_vap_tx_power::refresh rf regctl enable set regs!}");

            hal_pow_set_rf_regctl_enable(pst_hal_device, pst_hal_device->en_pow_rf_reg_ctl_flag);
            if (OAL_SWITCH_OFF == pst_hal_device->bit_al_tx_flag)
            {
                hal_rf_regctl_enable_set_regs(pst_hal_device, en_freq_band, uc_cur_ch_num, en_bandwidth);
            }
        }

        return;
    }

    if((HAL_POW_SET_TYPE_INIT == uc_type) || (HAL_POW_SET_TYPE_REFRESH == uc_type))
    {
#ifndef _PRE_WLAN_FEATURE_TPC_OPT
        if(IS_P2P_DEV(&(pst_dmac_vap->st_vap_base_info)))
        {
            hal_device_p2p_adjust_upc(pst_hal_device, uc_cur_ch_num, en_freq_band, en_bandwidth);
            return;
        }
#endif  /* _PRE_WLAN_FEATURE_TPC_OPT */

        /*��ȡ�����������, ����������*/
        pst_regdom_info = mac_get_channel_num_rc_info(en_freq_band, uc_cur_ch_num);

        if (OAL_UNLIKELY(OAL_PTR_NULL == pst_regdom_info))
        {
#ifndef _PRE_WLAN_FEATURE_TPC_OPT
            OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{dmac_pow_set_vap_tx_power::this channel isnot support by this country.freq_band = %u,cur_ch_num = %u}",
                                en_freq_band, uc_cur_ch_num);
            pst_vap_pow_info->us_reg_pow = 0xFFFF;
#else
            // TODO:    ��SAR������Ҫ��������

            /* ȡMAX_Power = MIN[MAX_Power@11k, MAX_Power@TEST, MAX_Power@REG] */
            /* Ĭ��ȡ����������͹��� */
            pst_vap_pow_info->us_reg_pow = OAL_MIN(OAL_MIN(g_uc_sar_pwr_limit, MAC_RC_DEFAULT_MAX_TX_PWR), pst_mac_vap->uc_tx_power) * HAL_POW_PRECISION_SHIFT;

            OAM_WARNING_LOG_ALTER(pst_mac_vap->uc_vap_id, OAM_SF_PWR,
                "{dmac_pow_set_vap_tx_power::chn do not support this country band[%u] ch_num[%u] reg_pow[%d] pst_mac_vap->uc_tx_power[%d] sar_limit[%d]}",
                   5, en_freq_band, uc_cur_ch_num, pst_vap_pow_info->us_reg_pow, pst_mac_vap->uc_tx_power, g_uc_sar_pwr_limit);
#endif
        }
        else
        {
            /* ȡSAR��׼��������ֵ���������������ƹ��ʡ�������ʵ������������н�Сֵ */
            pst_vap_pow_info->us_reg_pow = OAL_MIN(OAL_MIN(g_uc_sar_pwr_limit*HAL_POW_PRECISION_SHIFT,
                       OAL_MIN(pst_regdom_info->us_max_tx_pwr, pst_regdom_info->uc_max_reg_tx_pwr*HAL_POW_PRECISION_SHIFT)),
                       pst_mac_vap->uc_tx_power*HAL_POW_PRECISION_SHIFT);
        }

        /* ��ʼ��pow code�� */

#ifndef _PRE_WLAN_FEATURE_TPC_OPT
        hal_device_init_vap_pow_code(pst_hal_device, pst_vap_pow_info, uc_cur_ch_num, en_freq_band, en_bandwidth, uc_type);
#else
        hal_device_init_vap_pow_code(pst_hal_device, pst_vap_pow_info, uc_chan_idx, en_freq_band, en_bandwidth, uc_type);
#endif
    }

    /* ����ģʽ�򿪰����ù������÷��͹��� */
#ifdef _PRE_WLAN_FEATURE_ALWAYS_TX
    if (OAL_SWITCH_ON == pst_dmac_vap->st_vap_base_info.bit_al_tx_flag)
    {
        uc_protocol = (pst_dmac_vap->uc_protocol_rate_dscr >> 6) & 0x3;
        hal_device_set_pow_al_tx(pst_hal_device, pst_vap_pow_info, uc_protocol, &pst_dmac_vap->st_tx_data_mcast);
        return;
    }
#endif

    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_TPC, "{dmac_pow_set_vap_tx_power::uc_pow_code[0x%x].}", *(oal_uint64 *)&(pst_dmac_vap->st_tx_data_mcast.st_tx_power));

    if(OAL_PTR_NULL == pst_vap_pow_info->pst_rate_pow_table)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_TPC, "{dmac_pow_set_vap_tx_power::pst_rate_pow_table null, uc_type[%d]}",
                       uc_type);
        return;
    }

    if(uc_type != HAL_POW_SET_TYPE_MAG_LVL_CHANGE)
    {
#ifdef _PRE_WLAN_FEATURE_TPC_OPT
        /* ����resp֡���书�� */
        hal_pow_set_resp_frame_tx_power(pst_hal_device,
                                        pst_hal_device->st_wifi_channel_status.en_band,
                                        pst_hal_device->st_wifi_channel_status.uc_chan_idx,
                                        pst_vap_pow_info->pst_rate_pow_table);

        /* ���ø�band������֡�ķ��书�� */
        hal_pow_set_band_spec_frame_tx_power(pst_hal_device,
                                             pst_hal_device->st_wifi_channel_status.en_band,
                                             pst_hal_device->st_wifi_channel_status.uc_chan_idx,
                                             pst_vap_pow_info->pst_rate_pow_table);
#else
        /* ����resp֡���书�� */
        hal_pow_set_resp_frame_tx_power(pst_hal_device, en_freq_band, uc_cur_ch_num, pst_vap_pow_info->pst_rate_pow_table);

        /* ���ø�band������֡�ķ��书�� */
        hal_pow_set_band_spec_frame_tx_power(pst_hal_device, en_freq_band, uc_cur_ch_num, pst_vap_pow_info->pst_rate_pow_table);
#endif
    }

    if((HAL_POW_SET_TYPE_INIT == uc_type) ||
        ((uc_type != HAL_POW_SET_TYPE_CTL_LVL_CHANGE) && (WLAN_VAP_MODE_BSS_STA == pst_dmac_vap->st_vap_base_info.en_vap_mode)))
    {
        /*����豸������STAģʽ�����������ŵ���ʱ�����ù���֡�Ĺ���*/
        dmac_pow_set_vap_spec_frame_tx_power(pst_dmac_vap);
    }
#endif
}


oal_void dmac_pow_init_user_info(mac_vap_stru *pst_mac_vap, dmac_user_stru *pst_dmac_user)
{
    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{dmac_pow_init_user_info::init user pow info[%p]!}", DMAC_VAP_GET_POW_TABLE(pst_mac_vap));
    pst_dmac_user->st_user_pow_info.pst_rate_pow_table = DMAC_VAP_GET_POW_TABLE(pst_mac_vap);
}
#if 0

oal_void dmac_pow_init_vap_info(dmac_vap_stru *pst_dmac_vap)
{
    hal_vap_pow_info_stru               *pst_vap_pow_info   = OAL_PTR_NULL;

    OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{dmac_pow_init_vap_info::init vap pow info!}");

    /* ��ȡvap��������Ϣ�ṹ�� */
    pst_vap_pow_info = DMAC_VAP_GET_POW_INFO(pst_dmac_vap);

    pst_vap_pow_info->pst_rate_pow_table = g_ast_rate_pow_table_2g;
}
#endif






#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

