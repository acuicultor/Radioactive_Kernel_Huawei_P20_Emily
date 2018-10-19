


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_BTCOEX


/*****************************************************************************
  1 ͷ�ļ�����
*****************************************************************************/
#include "mac_data.h"
#include "hal_chip.h"
#include "hal_rf.h"
#include "hal_coex_reg.h"
#include "hal_device_fsm.h"
#include "dmac_btcoex.h"
#include "dmac_alg_if.h"
#include "dmac_alg.h"
#include "dmac_auto_adjust_freq.h"
#include "dmac_device.h"
#include "dmac_resource.h"
#include "dmac_scan.h"
#include "dmac_config.h"
#ifdef _PRE_WLAN_FEATURE_M2S
#include "dmac_m2s.h"
#endif
#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_BTCOEX_C

/*****************************************************************************
  2 ȫ�ֱ�������
*****************************************************************************/
oal_uint8 g_auc_rx_win_size_siso[BTCOEX_RX_WINDOW_SIZE_GRADE_BUTT][BTCOEX_RX_WINDOW_SIZE_INDEX_BUTT] = {
    {2, 4, 8, 64}, {4, 8, 32, 64}
};
/****************************************
              ����    �绰    ����
    2G/20M      0       0       0
    5G/20M      0       0       0
    2G/40M      0       1       1
    5G/40M      0       0       1
    5G/80M      1       1       1
*****************************************/
oal_uint8 g_auc_rx_win_size_grage_siso[WLAN_BAND_BUTT][WLAN_BW_CAP_BUTT][BTCOEX_ACTIVE_MODE_BUTT] = {
    /* 2G */
    /* 20M */  /* 40M */
    /* ����, �绰, ���� */
    {{0, 0, 0}, {0, 1, 1}, {1, 1, 1}, {1, 1, 1}},
    /* 5G */
    /* 20M */  /* 40M */  /* 80M */
    /* ����, �绰, ���� */
    {{0, 0, 0}, {0, 0, 1}, {1, 1, 1}, {1, 1, 1}}
};
oal_uint16 g_aus_btcoex_rate_thresholds_siso[WLAN_BAND_BUTT][WLAN_BW_CAP_BUTT][BTCOEX_RATE_THRESHOLD_BUTT] = {
    /* 2G */
    /* 20M */  /* 40M */
    {{23, 60}, {50, 130}, {0, 0},     {0, 0}},
    /* 5G */
    /* 20M */  /* 40M */  /* 80M */
    {{23, 80}, {50, 160}, {108, 340}, {0, 0}}
};

#if (WLAN_MAX_NSS_NUM != WLAN_SINGLE_NSS)
oal_uint8 g_auc_rx_win_size_mimo[BTCOEX_RX_WINDOW_SIZE_GRADE_BUTT][BTCOEX_RX_WINDOW_SIZE_INDEX_BUTT] = {
    {2, 4, 8, 64}, {8, 16, 32, 64}
};
/****************************************
              ����    �绰    ����
    2G/20M      0       0       0
    5G/20M      0       0       0
    2G/40M      0       1       1
    5G/40M      0       0       1
    5G/80M      1       1       1
*****************************************/
oal_uint8 g_auc_rx_win_size_grage_mimo[WLAN_BAND_BUTT][WLAN_BW_CAP_BUTT][BTCOEX_ACTIVE_MODE_BUTT] = {
    /* 2G */
    /* 20M */  /* 40M */
    /* ����, �绰, ���� */
    {{1, 0, 1}, {1, 0, 1}, {1, 1, 1}, {1, 1, 1}},
    /* 5G */
    /* 20M */  /* 40M */  /* 80M */
    /* ����, �绰, ���� */
    {{0, 0, 0}, {0, 0, 1}, {1, 1, 1}, {1, 1, 1}}
};

oal_uint16 g_aus_btcoex_rate_thresholds_mimo[WLAN_BAND_BUTT][WLAN_BW_CAP_BUTT][BTCOEX_RATE_THRESHOLD_BUTT] = {
    /* 2G */
    /* 20M */  /* 40M */
    {{23, 70}, {50, 170}, {0, 0},     {0, 0}},
    /* 5G */
    /* 20M */  /* 40M */  /* 80M */
    {{23, 90}, {50, 180}, {108, 360}, {0, 0}}
};
#endif

oal_uint32 g_rx_statistics_print = 0;

oal_uint16 g_us_occupied_point[BTCOEX_LINKLOSS_OCCUPIED_NUMBER];

/*****************************************************************************
  3 ����ʵ��
*****************************************************************************/

oal_void dmac_btcoex_perf_param_show(oal_void)
{
    OAM_WARNING_LOG4(0, OAM_SF_COEX,
        "{dmac_btcoex_perf_param_show::SISO 2G_20M_MIN[%d],2G_20M_MAX[%d],2G_40M_MIN[%d], 2G_40M_MAX[%d].}",
        g_aus_btcoex_rate_thresholds_siso[WLAN_BAND_2G][WLAN_BW_CAP_20M][BTCOEX_RATE_THRESHOLD_MIN],
        g_aus_btcoex_rate_thresholds_siso[WLAN_BAND_2G][WLAN_BW_CAP_20M][BTCOEX_RATE_THRESHOLD_MAX],
        g_aus_btcoex_rate_thresholds_siso[WLAN_BAND_2G][WLAN_BW_CAP_40M][BTCOEX_RATE_THRESHOLD_MIN],
        g_aus_btcoex_rate_thresholds_siso[WLAN_BAND_2G][WLAN_BW_CAP_40M][BTCOEX_RATE_THRESHOLD_MAX]);

    OAM_WARNING_LOG4(0, OAM_SF_COEX,
        "{dmac_btcoex_perf_param_show::SISO RX_SIZE_0_0[%d],RX_SIZE_0_1[%d],RX_SIZE_0_2[%d], RX_SIZE_0_3[%d].}",
        g_auc_rx_win_size_siso[BTCOEX_RX_WINDOW_SIZE_GRADE_0][BTCOEX_RX_WINDOW_SIZE_INDEX_0],
        g_auc_rx_win_size_siso[BTCOEX_RX_WINDOW_SIZE_GRADE_0][BTCOEX_RX_WINDOW_SIZE_INDEX_1],
        g_auc_rx_win_size_siso[BTCOEX_RX_WINDOW_SIZE_GRADE_0][BTCOEX_RX_WINDOW_SIZE_INDEX_2],
        g_auc_rx_win_size_siso[BTCOEX_RX_WINDOW_SIZE_GRADE_0][BTCOEX_RX_WINDOW_SIZE_INDEX_3]);

    OAM_WARNING_LOG4(0, OAM_SF_COEX,
        "{dmac_btcoex_perf_param_show::SISO RX_SIZE_1_0[%d],RX_SIZE_1_1[%d],RX_SIZE_1_2[%d], RX_SIZE_1_3[%d].}",
        g_auc_rx_win_size_siso[BTCOEX_RX_WINDOW_SIZE_GRADE_1][BTCOEX_RX_WINDOW_SIZE_INDEX_0],
        g_auc_rx_win_size_siso[BTCOEX_RX_WINDOW_SIZE_GRADE_1][BTCOEX_RX_WINDOW_SIZE_INDEX_1],
        g_auc_rx_win_size_siso[BTCOEX_RX_WINDOW_SIZE_GRADE_1][BTCOEX_RX_WINDOW_SIZE_INDEX_2],
        g_auc_rx_win_size_siso[BTCOEX_RX_WINDOW_SIZE_GRADE_1][BTCOEX_RX_WINDOW_SIZE_INDEX_3]);

#if (WLAN_MAX_NSS_NUM != WLAN_SINGLE_NSS)
    OAM_WARNING_LOG4(0, OAM_SF_COEX,
        "{dmac_btcoex_perf_param_show::MIMO 2G_20M_MIN[%d],2G_20M_MAX[%d],2G_40M_MIN[%d], 2G_40M_MAX[%d].}",
        g_aus_btcoex_rate_thresholds_mimo[WLAN_BAND_2G][WLAN_BW_CAP_20M][BTCOEX_RATE_THRESHOLD_MIN],
        g_aus_btcoex_rate_thresholds_mimo[WLAN_BAND_2G][WLAN_BW_CAP_20M][BTCOEX_RATE_THRESHOLD_MAX],
        g_aus_btcoex_rate_thresholds_mimo[WLAN_BAND_2G][WLAN_BW_CAP_40M][BTCOEX_RATE_THRESHOLD_MIN],
        g_aus_btcoex_rate_thresholds_mimo[WLAN_BAND_2G][WLAN_BW_CAP_40M][BTCOEX_RATE_THRESHOLD_MAX]);

    OAM_WARNING_LOG4(0, OAM_SF_COEX,
        "{dmac_btcoex_perf_param_show::MIMO RX_SIZE_0_0[%d],RX_SIZE_0_1[%d],RX_SIZE_0_2[%d], RX_SIZE_0_3[%d].}",
        g_auc_rx_win_size_mimo[BTCOEX_RX_WINDOW_SIZE_GRADE_0][BTCOEX_RX_WINDOW_SIZE_INDEX_0],
        g_auc_rx_win_size_mimo[BTCOEX_RX_WINDOW_SIZE_GRADE_0][BTCOEX_RX_WINDOW_SIZE_INDEX_1],
        g_auc_rx_win_size_mimo[BTCOEX_RX_WINDOW_SIZE_GRADE_0][BTCOEX_RX_WINDOW_SIZE_INDEX_2],
        g_auc_rx_win_size_mimo[BTCOEX_RX_WINDOW_SIZE_GRADE_0][BTCOEX_RX_WINDOW_SIZE_INDEX_3]);

    OAM_WARNING_LOG4(0, OAM_SF_COEX,
        "{dmac_btcoex_perf_param_show::MIMO RX_SIZE_1_0[%d],RX_SIZE_1_1[%d],RX_SIZE_1_2[%d], RX_SIZE_1_3[%d].}",
        g_auc_rx_win_size_mimo[BTCOEX_RX_WINDOW_SIZE_GRADE_1][BTCOEX_RX_WINDOW_SIZE_INDEX_0],
        g_auc_rx_win_size_mimo[BTCOEX_RX_WINDOW_SIZE_GRADE_1][BTCOEX_RX_WINDOW_SIZE_INDEX_1],
        g_auc_rx_win_size_mimo[BTCOEX_RX_WINDOW_SIZE_GRADE_1][BTCOEX_RX_WINDOW_SIZE_INDEX_2],
        g_auc_rx_win_size_mimo[BTCOEX_RX_WINDOW_SIZE_GRADE_1][BTCOEX_RX_WINDOW_SIZE_INDEX_3]);
#endif
}


oal_void  dmac_btcoex_perf_param_update(mac_btcoex_mgr_stru *pst_btcoex_mgr)
{
    /* �޸����� */
    if(0 == pst_btcoex_mgr->uc_cfg_btcoex_type)
    {
        if(WLAN_SINGLE_NSS == pst_btcoex_mgr->en_btcoex_nss)
        {
            g_aus_btcoex_rate_thresholds_siso[WLAN_BAND_2G][WLAN_BW_CAP_20M][BTCOEX_RATE_THRESHOLD_MIN] =
                pst_btcoex_mgr->pri_data.threhold.uc_20m_low;
            g_aus_btcoex_rate_thresholds_siso[WLAN_BAND_2G][WLAN_BW_CAP_20M][BTCOEX_RATE_THRESHOLD_MAX] =
                pst_btcoex_mgr->pri_data.threhold.uc_20m_high;
            g_aus_btcoex_rate_thresholds_siso[WLAN_BAND_2G][WLAN_BW_CAP_40M][BTCOEX_RATE_THRESHOLD_MIN] =
                pst_btcoex_mgr->pri_data.threhold.uc_40m_low;
            g_aus_btcoex_rate_thresholds_siso[WLAN_BAND_2G][WLAN_BW_CAP_40M][BTCOEX_RATE_THRESHOLD_MAX] =
                pst_btcoex_mgr->pri_data.threhold.us_40m_high;
        }
        else
        {
#if (WLAN_MAX_NSS_NUM != WLAN_SINGLE_NSS)
            g_aus_btcoex_rate_thresholds_mimo[WLAN_BAND_2G][WLAN_BW_CAP_20M][BTCOEX_RATE_THRESHOLD_MIN] =
                pst_btcoex_mgr->pri_data.threhold.uc_20m_low;
            g_aus_btcoex_rate_thresholds_mimo[WLAN_BAND_2G][WLAN_BW_CAP_20M][BTCOEX_RATE_THRESHOLD_MAX] =
                pst_btcoex_mgr->pri_data.threhold.uc_20m_high;
            g_aus_btcoex_rate_thresholds_mimo[WLAN_BAND_2G][WLAN_BW_CAP_40M][BTCOEX_RATE_THRESHOLD_MIN] =
                pst_btcoex_mgr->pri_data.threhold.uc_40m_low;
            g_aus_btcoex_rate_thresholds_mimo[WLAN_BAND_2G][WLAN_BW_CAP_40M][BTCOEX_RATE_THRESHOLD_MAX] =
                pst_btcoex_mgr->pri_data.threhold.us_40m_high;
#endif
        }
    }
    else
    {
        if(WLAN_SINGLE_NSS == pst_btcoex_mgr->en_btcoex_nss)
        {
            g_auc_rx_win_size_siso[pst_btcoex_mgr->pri_data.rx_size.uc_grade][BTCOEX_RX_WINDOW_SIZE_INDEX_0] =
                pst_btcoex_mgr->pri_data.rx_size.uc_rx_size0;
            g_auc_rx_win_size_siso[pst_btcoex_mgr->pri_data.rx_size.uc_grade][BTCOEX_RX_WINDOW_SIZE_INDEX_1] =
                pst_btcoex_mgr->pri_data.rx_size.uc_rx_size1;
            g_auc_rx_win_size_siso[pst_btcoex_mgr->pri_data.rx_size.uc_grade][BTCOEX_RX_WINDOW_SIZE_INDEX_2] =
                pst_btcoex_mgr->pri_data.rx_size.uc_rx_size2;
            g_auc_rx_win_size_siso[pst_btcoex_mgr->pri_data.rx_size.uc_grade][BTCOEX_RX_WINDOW_SIZE_INDEX_3] =
                pst_btcoex_mgr->pri_data.rx_size.uc_rx_size3;
        }
        else
        {
#if (WLAN_MAX_NSS_NUM != WLAN_SINGLE_NSS)
            g_auc_rx_win_size_mimo[pst_btcoex_mgr->pri_data.rx_size.uc_grade][BTCOEX_RX_WINDOW_SIZE_INDEX_0] =
                pst_btcoex_mgr->pri_data.rx_size.uc_rx_size0;
            g_auc_rx_win_size_mimo[pst_btcoex_mgr->pri_data.rx_size.uc_grade][BTCOEX_RX_WINDOW_SIZE_INDEX_1] =
                pst_btcoex_mgr->pri_data.rx_size.uc_rx_size1;
            g_auc_rx_win_size_mimo[pst_btcoex_mgr->pri_data.rx_size.uc_grade][BTCOEX_RX_WINDOW_SIZE_INDEX_2] =
                pst_btcoex_mgr->pri_data.rx_size.uc_rx_size2;
            g_auc_rx_win_size_mimo[pst_btcoex_mgr->pri_data.rx_size.uc_grade][BTCOEX_RX_WINDOW_SIZE_INDEX_3] =
                pst_btcoex_mgr->pri_data.rx_size.uc_rx_size3;
#endif
        }
    }

    dmac_btcoex_perf_param_show();
}


oal_uint8 dmac_btcoex_find_all_valid_sta_per_device(
        hal_to_dmac_device_stru *pst_hal_device, oal_uint8 *puc_vap_id)
{
    mac_vap_stru                *pst_mac_vap      = OAL_PTR_NULL;
    oal_uint8                    uc_vap_index     = 0;
    oal_uint8                    uc_up_vap_num    = 0;
    oal_uint8                    uc_valid_sta_num = 0;
    oal_uint8                    auc_mac_vap_id[WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE] = {0};

    /* �ҵ�����up��vap�豸 */
    uc_up_vap_num = hal_device_find_all_up_vap(pst_hal_device, auc_mac_vap_id);
    for (uc_vap_index = 0; uc_vap_index < uc_up_vap_num; uc_vap_index++)
    {
        pst_mac_vap  = (mac_vap_stru *)mac_res_get_mac_vap(auc_mac_vap_id[uc_vap_index]);
        if (OAL_PTR_NULL == pst_mac_vap)
        {
            OAM_ERROR_LOG0(auc_mac_vap_id[uc_vap_index], OAM_SF_COEX, "dmac_btcoex_find_all_valid_sta_per_device::pst_mac_vap IS NULL.");
            continue;
        }

        /* 1.��sta�Ļ�,����Ҫ���� */
        if(IS_AP(pst_mac_vap))
        {
            continue;
        }

        /* 2.valid sta ͳ��(02��03�ڴ�������) */
        if(OAL_TRUE == MAC_BTCOEX_CHECK_VALID_STA(pst_mac_vap))
        {
            /* �ҵ�sta�����и�ֵ */
            puc_vap_id[uc_valid_sta_num++] = pst_mac_vap->uc_vap_id;
        }
    }

    return uc_valid_sta_num;
}


oal_uint8 dmac_btcoex_find_all_valid_ap_per_device(
        hal_to_dmac_device_stru *pst_hal_device, oal_uint8 *puc_vap_id)
{
    mac_vap_stru                *pst_mac_vap      = OAL_PTR_NULL;
    oal_uint8                    uc_vap_index     = 0;
    oal_uint8                    uc_up_vap_num    = 0;
    oal_uint8                    uc_valid_ap_num  = 0;
    oal_uint8                    auc_mac_vap_id[WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE] = {0};

    /* �ҵ�����up��vap�豸 */
    uc_up_vap_num = hal_device_find_all_up_vap(pst_hal_device, auc_mac_vap_id);
    for (uc_vap_index = 0; uc_vap_index < uc_up_vap_num; uc_vap_index++)
    {
        pst_mac_vap  = (mac_vap_stru *)mac_res_get_mac_vap(auc_mac_vap_id[uc_vap_index]);
        if (OAL_PTR_NULL == pst_mac_vap)
        {
            OAM_ERROR_LOG0(auc_mac_vap_id[uc_vap_index], OAM_SF_COEX, "dmac_btcoex_find_all_valid_ap_per_device::pst_mac_vap IS NULL.");
            continue;
        }

        /* 1.��ap�Ļ�,����Ҫ���� */
        if(IS_STA(pst_mac_vap))
        {
            continue;
        }

        /* 2.valid ap ͳ��(02��03�ڴ�������) */
        if(OAL_TRUE == MAC_BTCOEX_CHECK_VALID_AP(pst_mac_vap))
        {
            /* �ҵ�ap�����и�ֵ */
            puc_vap_id[uc_valid_ap_num++] = pst_mac_vap->uc_vap_id;
        }
    }

    return uc_valid_ap_num;
}


oal_void dmac_btcoex_ps_get_vap_check_per_device(
        hal_to_dmac_device_stru *pst_hal_device, mac_vap_stru **ppst_mac_vap)
{
    mac_device_stru   *pst_mac_device;
    oal_uint8          uc_vap_idx;
    mac_vap_stru      *pst_mac_vap_tmp  = OAL_PTR_NULL;

    pst_mac_device = mac_res_get_dev(pst_hal_device->uc_mac_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
       OAM_ERROR_LOG1(0, OAM_SF_COEX, "{dmac_btcoex_ps_get_vap_check_per_device: mac device is null ptr. device id:%d}", pst_hal_device->uc_mac_device_id);
       return;
    }

    /* �ȳ�ʼ����ֵΪ�� */
    *ppst_mac_vap = OAL_PTR_NULL;

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_mac_vap_tmp = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_mac_vap_tmp)
        {
            OAM_ERROR_LOG0(pst_mac_device->auc_vap_id[uc_vap_idx], OAM_SF_COEX, "{dmac_btcoex_ps_get_vap_check_per_device::the vap is null!}");
            continue;
        }

        /* �����ڱ�hal device��vap��ͳ�� */
        if(pst_hal_device != MAC_GET_DMAC_VAP(pst_mac_vap_tmp)->pst_hal_device)
        {
            OAM_WARNING_LOG0(pst_mac_vap_tmp->uc_vap_id, OAM_SF_COEX,
               "{dmac_btcoex_ps_get_vap_check_per_device::the vap not belong to this hal device!}");
            continue;
        }

        /* ���ڷ�legacy sta�Ļ���ֱ�ӷ��أ���ʱ����֧�� */
        if(IS_P2P_CL(pst_mac_vap_tmp) || IS_P2P_GO(pst_mac_vap_tmp) || (IS_LEGACY_AP(pst_mac_vap_tmp)))
        {
            *ppst_mac_vap = OAL_PTR_NULL;
            return;
        }

        /* ��ʱֻ���ǵ�sta�������ſ�p2p gc */
        if(IS_LEGACY_STA(pst_mac_vap_tmp))
        {
            /* �ҵ�legacy sta�����и�ֵ */
            *ppst_mac_vap = pst_mac_vap_tmp;
        }
    }
}


oal_void  dmac_btcoex_set_freq_and_work_state_hal_device(hal_to_dmac_device_stru *pst_hal_device)
{
    mac_device_stru             *pst_mac_device;
    mac_vap_stru                *pst_mac_vap   = OAL_PTR_NULL;
    wlan_channel_band_enum_uint8 en_band = WLAN_BAND_5G;               /* Ĭ����5G��Ӱ��״̬ */
    oal_bool_enum_uint8          en_legacy_connect_state = OAL_FALSE;  /* Ĭ����disconnect */
    oal_bool_enum_uint8          en_p2p_connect_state    = OAL_FALSE;  /* Ĭ����disconnect */
    oal_uint8                    uc_chan_idx             = 0;          /* Ĭ���ŵ������� */
    wlan_channel_bandwidth_enum_uint8   en_bandwidth     = WLAN_BAND_WIDTH_20M;  /* Ĭ�ϴ���ģʽ */
    oal_uint8                    uc_vap_index     = 0;
    oal_uint8                    uc_up_ap_num     = 0;
    oal_uint8                    uc_up_sta_num    = 0;
    oal_uint8                    uc_up_2g_num     = 0;
    oal_uint8                    uc_up_5g_num     = 0;
    oal_bool_enum_uint8          en_sco_status    = OAL_FALSE;
    oal_bool_enum_uint8          en_ps_stop       = OAL_FALSE;  /* ��ʼ�Ǵ�ps */
    oal_uint32                   ul_ret;

    if (OAL_PTR_NULL == pst_hal_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_set_freq_and_work_state_hal_device: pst_hal_device is null ptr.}");
        return;
    }

    /* ��assoc��disassoc�ӿ��е���ʱ����ʱ�Ѿ������ɹ�����ȥ�����ɹ� */
    pst_mac_device = mac_res_get_dev(pst_hal_device->uc_mac_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG1(0, OAM_SF_COEX, "{dmac_btcoex_set_freq_and_work_state_hal_device: mac device is null ptr. device id:%d}", pst_hal_device->uc_mac_device_id);
        return;
    }

    /* Ƶ��ֱ������up����pause��vap��ȡС����֤��2G�豸ʱ��������Ҫ��Ч */
    /* ��ʱsta������WAIT_ASSOC״̬����û�е�up״̬������ʹ��hal_device_find_all_up_vap�ӿ� */
    for (uc_vap_index = 0; uc_vap_index < pst_mac_device->uc_vap_num; uc_vap_index++)
    {
        pst_mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_index]);
        if (OAL_PTR_NULL == pst_mac_vap)
        {
            OAM_ERROR_LOG0(pst_mac_device->auc_vap_id[uc_vap_index], OAM_SF_COEX,
                "{dmac_btcoex_set_freq_and_work_state_hal_device::the vap is null!}");
            continue;
        }

        /* �����ڱ�hal device��vap����ͳ�� */
        if(pst_hal_device != DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap))
        {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_COEX,
               "{dmac_btcoex_set_freq_and_work_state_hal_device::the vap not belong to this hal device!}");
            continue;
        }

        /* ����Ч״̬����ͳ��ȥ����ʱ����ô˽ӿ�ʱ����up״̬ */
        if(MAC_VAP_STATE_UP != pst_mac_vap->en_vap_state &&
            MAC_VAP_STATE_PAUSE != pst_mac_vap->en_vap_state && MAC_VAP_STATE_STA_WAIT_ASOC != pst_mac_vap->en_vap_state)
        {
            continue;
        }

        /* 1. legacy vap connect״̬�ж� */
        if((IS_LEGACY_STA(pst_mac_vap) || IS_LEGACY_AP(pst_mac_vap))&&(0 != pst_mac_vap->us_user_nums))
        {
            en_legacy_connect_state = OAL_TRUE;
        }
        /* 1.1. p2p vap connect״̬�ж� */
        else if((IS_P2P_CL(pst_mac_vap) || IS_P2P_GO(pst_mac_vap))&&(0 != pst_mac_vap->us_user_nums))
        {
            en_p2p_connect_state = OAL_TRUE;
        }

        /* 2.����2g�� */
        if(OAL_TRUE == MAC_BTCOEX_CHECK_VALID_VAP(pst_mac_vap))
        {
            en_band = WLAN_BAND_2G;   /* ����2G�豸���Ǿ�һֱ֪ͨ״̬��2GƵ�Σ���������Ҫ��Ч */

            /* 3.�жϴ��� */
            en_bandwidth = pst_mac_vap->st_channel.en_bandwidth;

            /* 4.�ж��ŵ� */
            ul_ret = mac_get_channel_idx_from_num(pst_mac_vap->st_channel.en_band, pst_mac_vap->st_channel.uc_chan_number, &uc_chan_idx);
            if(OAL_ERR_CODE_INVALID_CONFIG == ul_ret)
            {
                OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_COEX,
                    "{dmac_btcoex_set_freq_and_work_state_hal_device::en_band[%d] uc_chan_number[%d]!}",
                    pst_mac_vap->st_channel.en_band, pst_mac_vap->st_channel.uc_chan_number);
            }
        }

        /* �����û�����Ϊ0 */
        if(0 != pst_mac_vap->us_user_nums)
        {
            (IS_AP(pst_mac_vap)) ?(uc_up_ap_num++):(uc_up_sta_num++);

            (OAL_TRUE == MAC_BTCOEX_CHECK_VALID_VAP(pst_mac_vap))?(uc_up_2g_num++):(uc_up_5g_num++);
        }

        /* 5.1.��apģʽ */
        if(0 != uc_up_ap_num && 0 == uc_up_sta_num)
        {
            en_ps_stop = OAL_TRUE;
        }

        /* 5.2.5gģʽ */
        if(0 != uc_up_5g_num && 0 == uc_up_2g_num)
        {
            en_ps_stop = OAL_TRUE;
        }
    }

    /* 5.3.�绰���� */
    hal_btcoex_get_bt_sco_status(pst_hal_device, &en_sco_status);
    if (OAL_TRUE == en_sco_status)
    {
        en_ps_stop = OAL_TRUE;
    }

    /* 5.4. dbac������,ֱ��return */
    if ((OAL_TRUE == mac_is_dbac_running(pst_mac_device)))
    {
        en_ps_stop = OAL_TRUE;
    }

    if (0 == pst_mac_device->uc_vap_num)
    {
        hal_set_btcoex_soc_gpreg0(OAL_FALSE, BIT13, 13); //wifi off
    }

    if (0 == mac_device_calc_up_vap_num(pst_mac_device))
    {
        /* ״̬�ϱ�BT */
        OAM_WARNING_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_vap_down_handle::Notify BT cancel AFH.}");
    }

    /* ˢ��ps���� */
    GET_HAL_BTCOEX_SW_PREEMPT_PS_STOP(pst_hal_device) = en_ps_stop;

    hal_set_btcoex_soc_gpreg0(en_band, BIT0, 0);    // Ƶ��

    hal_set_btcoex_soc_gpreg1(en_legacy_connect_state, BIT3, 3);  //legacy vap����״̬

    hal_set_btcoex_soc_gpreg0(en_p2p_connect_state, BIT15, 15);  //p2p ����״̬

    hal_set_btcoex_soc_gpreg0(uc_chan_idx, BIT5 | BIT4 | BIT3 | BIT2 | BIT1, 1);    // �ŵ�

    hal_set_btcoex_soc_gpreg0(en_bandwidth, BIT8 | BIT7 | BIT6, 6); // ����

    hal_set_btcoex_soc_gpreg1(en_ps_stop, BIT7, 7);  //ps��ֹ״̬֪ͨ

    hal_coex_sw_irq_set(HAL_COEX_SW_IRQ_BT);

    OAM_WARNING_LOG_ALTER(0, OAM_SF_COEX,
        "{dmac_btcoex_set_freq_and_work_state_hal_device::band[%d]legacy_connect_state[%d]p2p_connect_state[%d]chan_idx[%d]bandwidth[%d]!}",
        5, en_band, en_legacy_connect_state, en_p2p_connect_state, uc_chan_idx, en_bandwidth);
    OAM_WARNING_LOG_ALTER(0, OAM_SF_COEX,
        "{dmac_btcoex_set_freq_and_work_state_hal_device::up_ap_num[%d]up_sta_num[%d]up_2g_num[%d]up_5g_num[%d]sco_status[%d]dbac[%d]ps_stop[%d]!}",
        7, uc_up_ap_num, uc_up_sta_num, uc_up_2g_num, uc_up_5g_num, en_sco_status,
        mac_is_dbac_running(pst_mac_device), GET_HAL_BTCOEX_SW_PREEMPT_PS_STOP(pst_hal_device));
}

oal_uint32 dmac_btcoex_wlan_occupied_timeout_callback(oal_void *p_arg)
{
    dmac_vap_stru *pst_dmac_vap   = (dmac_vap_stru *)p_arg;
    oal_uint8 *puc_occupied_times = &(pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_occupied.uc_occupied_times);

    if ((*puc_occupied_times) > 0)
    {
        OAM_WARNING_LOG1(0, OAM_SF_COEX, "{dmac_btcoex_wlan_occupied_timeout_callback::g_occupied_times = %d}" , *puc_occupied_times);
        (*puc_occupied_times)--;
        dmac_btcoex_set_occupied_period(pst_dmac_vap, OCCUPIED_PERIOD);
    }
    else
    {
        FRW_TIMER_DESTROY_TIMER(&(pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_occupied.bt_coex_occupied_timer));
    }

    return OAL_SUCC;
}


oal_void dmac_btcoex_rx_mgmt_occupied_check(mac_ieee80211_frame_stru *pst_frame_hdr, dmac_vap_stru *pst_dmac_vap)
{
#if(_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV)
    hal_btcoex_btble_status_stru   *pst_btcoex_btble_status = DMAC_VAP_GET_BTCOEX_STATUS(pst_dmac_vap);

    /* �Ƿ���ble scan */
    if(pst_btcoex_btble_status->un_ble_status.st_ble_status.bit_ble_scan)
    {
        if ((WLAN_ASSOC_RSP == pst_frame_hdr->st_frame_control.bit_sub_type)
            ||(WLAN_REASSOC_RSP == pst_frame_hdr->st_frame_control.bit_sub_type))
        {
            dmac_btcoex_set_occupied_period(pst_dmac_vap, OCCUPIED_PERIOD);

            pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_occupied.uc_occupied_times = OCCUPIED_TIMES;
            /* ����occupied��ʱ�� */
            FRW_TIMER_CREATE_TIMER(&(pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_occupied.bt_coex_occupied_timer),
                                   dmac_btcoex_wlan_occupied_timeout_callback,
                                   OCCUPIED_INTERVAL,
                                   (oal_void *)pst_dmac_vap,
                                   OAL_TRUE,
                                   OAM_MODULE_ID_DMAC,
                                   pst_dmac_vap->st_vap_base_info.ul_core_id);
        }
    }
#endif
}


oal_void  dmac_btcoex_tx_mgmt_process(dmac_vap_stru *pst_dmac_vap, mac_ieee80211_frame_stru *pst_mac_header)
{
#if(_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV)
    oal_uint8                       uc_mgmt_subtype;

    if (DMAC_VAP_GET_BTCOEX_STATUS(pst_dmac_vap)->un_ble_status.st_ble_status.bit_ble_scan)
    {
        uc_mgmt_subtype = mac_frame_get_subtype_value((oal_uint8 *)pst_mac_header);

        if ((WLAN_AUTH == uc_mgmt_subtype) || (WLAN_ASSOC_REQ == uc_mgmt_subtype))
        {
            dmac_btcoex_set_occupied_period(pst_dmac_vap, 50000); //50ms
        }
    }
#endif
}


oal_void dmac_btcoex_tx_addba_rsp_check(oal_netbuf_stru *pst_netbuf, mac_user_stru *pst_mac_user)
{
    dmac_user_btcoex_delba_stru      *pst_dmac_user_btcoex_delba;
    dmac_user_stru  *pst_dmac_user    = OAL_PTR_NULL;
    oal_uint8       *puc_mac_header   = oal_netbuf_header(pst_netbuf);
    oal_uint8       *puc_mac_payload  = mac_netbuf_get_payload(pst_netbuf);

    pst_dmac_user  = MAC_GET_DMAC_USER(pst_mac_user);

    /* Management frame */
    if ((WLAN_FC0_SUBTYPE_ACTION|WLAN_FC0_TYPE_MGT) == mac_get_frame_type_and_subtype(puc_mac_header))
    {
        if ((MAC_ACTION_CATEGORY_BA == puc_mac_payload[0]) && (MAC_BA_ACTION_ADDBA_RSP == puc_mac_payload[1]))
        {
            pst_dmac_user_btcoex_delba = &(pst_dmac_user->st_dmac_user_btcoex_stru.st_dmac_user_btcoex_delba);
            pst_dmac_user_btcoex_delba->en_get_addba_req_flag = OAL_TRUE;

            if(OAL_TRUE == pst_dmac_user_btcoex_delba->en_delba_trigger)
            {
                pst_dmac_user_btcoex_delba->en_ba_size_real_index = pst_dmac_user_btcoex_delba->en_ba_size_expect_index;
            }
            else
            {
                pst_dmac_user_btcoex_delba->en_ba_size_real_index = BTCOEX_RX_WINDOW_SIZE_INDEX_3;
            }
        }
    }
}


OAL_STATIC OAL_INLINE oal_void dmac_btcoex_rx_average_rate_calculate (dmac_user_btcoex_rx_info_stru *pst_btcoex_wifi_rx_rate_info,
                                                                                oal_uint32 *pul_rx_rate, oal_uint16 *pus_rx_count)
{
    if (0 != pst_btcoex_wifi_rx_rate_info->us_rx_rate_stat_count)
    {
         pst_btcoex_wifi_rx_rate_info->ull_rx_rate_mbps /= 1000;
        *pus_rx_count = pst_btcoex_wifi_rx_rate_info->us_rx_rate_stat_count;
        *pul_rx_rate = (oal_uint32)(pst_btcoex_wifi_rx_rate_info->ull_rx_rate_mbps / pst_btcoex_wifi_rx_rate_info->us_rx_rate_stat_count);
    }
    else
    {
        pst_btcoex_wifi_rx_rate_info->ull_rx_rate_mbps = 0;
        *pus_rx_count = 0;
        return;
    }

    /* ������֮����Ҫ�������㣬����һ��ͳ�Ƽ��� */
    OAL_MEMZERO(pst_btcoex_wifi_rx_rate_info, OAL_SIZEOF(dmac_user_btcoex_rx_info_stru));
}


oal_void dmac_btcoex_update_ba_size(mac_vap_stru *pst_mac_vap, dmac_user_btcoex_delba_stru *pst_dmac_user_btcoex_delba, hal_btcoex_btble_status_stru *pst_btble_status)
{
    bt_status_stru   *pst_bt_status;
    ble_status_stru  *pst_ble_status;
    wlan_channel_band_enum_uint8   en_band;
    wlan_bw_cap_enum_uint8         en_bandwidth;
    btcoex_active_mode_enum_uint8          en_bt_active_mode;
    btcoex_rx_window_size_grade_enum_uint8 en_bt_rx_win_size_grade;

    pst_bt_status = &(pst_btble_status->un_bt_status.st_bt_status);
    pst_ble_status = &(pst_btble_status->un_ble_status.st_ble_status);

    en_band = pst_mac_vap->st_channel.en_band;
    mac_vap_get_bandwidth_cap(pst_mac_vap, &en_bandwidth);
    if ((en_band >= WLAN_BAND_BUTT) || (en_bandwidth >= WLAN_BW_CAP_BUTT))
    {
        OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_update_ba_size::band %d, bandwidth %d exceed scope!}",
                         en_band, en_bandwidth);
        return;
    }

    if (pst_dmac_user_btcoex_delba->en_ba_size_expect_index >= BTCOEX_RX_WINDOW_SIZE_INDEX_BUTT)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_update_ba_size::ba_size_expect_index %d exceed scope!}",
                         pst_dmac_user_btcoex_delba->en_ba_size_expect_index);
        return;
    }

    if (pst_bt_status->bit_bt_sco)
    {
        en_bt_active_mode = BTCOEX_ACTIVE_MODE_SCO;
        /* 6slot �豸 */
        if (2 == pst_ble_status->bit_bt_6slot)
        {
            pst_dmac_user_btcoex_delba->uc_ba_size = 1;
            return;
        }
    }
    else if (pst_bt_status->bit_bt_a2dp)
    {
        en_bt_active_mode = BTCOEX_ACTIVE_MODE_A2DP;
    }
    else if (pst_ble_status->bit_bt_transfer)
    {
        en_bt_active_mode = BTCOEX_ACTIVE_MODE_TRANSFER;
    }
    else
    {
        en_bt_active_mode = BTCOEX_ACTIVE_MODE_BUTT;
    }

    /* BTû��ҵ��, �ۺ�64 */
    if (en_bt_active_mode >= BTCOEX_ACTIVE_MODE_BUTT)
    {
        en_bt_active_mode = BTCOEX_ACTIVE_MODE_A2DP;
        pst_dmac_user_btcoex_delba->en_ba_size_expect_index = BTCOEX_RX_WINDOW_SIZE_INDEX_3;
    }

#if (WLAN_MAX_NSS_NUM != WLAN_SINGLE_NSS)
    if(WLAN_DOUBLE_NSS == pst_dmac_user_btcoex_delba->en_user_nss_num)
    {
        en_bt_rx_win_size_grade = g_auc_rx_win_size_grage_mimo[en_band][en_bandwidth][en_bt_active_mode];
        pst_dmac_user_btcoex_delba->uc_ba_size = g_auc_rx_win_size_mimo[en_bt_rx_win_size_grade][pst_dmac_user_btcoex_delba->en_ba_size_expect_index];
    }
    else
#endif
    {
        en_bt_rx_win_size_grade = g_auc_rx_win_size_grage_siso[en_band][en_bandwidth][en_bt_active_mode];
        pst_dmac_user_btcoex_delba->uc_ba_size = g_auc_rx_win_size_siso[en_bt_rx_win_size_grade][pst_dmac_user_btcoex_delba->en_ba_size_expect_index];
    }
}


oal_uint32 dmac_btcoex_rx_rate_statistics_flag_callback(oal_void *p_arg)
{
    hal_btcoex_btble_status_stru       *pst_btcoex_btble_status;
    dmac_vap_btcoex_rx_statistics_stru *pst_dmac_vap_btcoex_rx_statistics;
    dmac_user_btcoex_rx_info_stru      *pst_dmac_user_btcoex_rx_info;
    mac_vap_stru                       *pst_mac_vap;
    dmac_vap_stru                      *pst_dmac_vap;
    dmac_user_stru                     *pst_dmac_user;

    pst_mac_vap = (mac_vap_stru *)p_arg;
    if (OAL_FALSE == MAC_BTCOEX_CHECK_VALID_STA(pst_mac_vap))
    {
        return OAL_SUCC;
    }

    pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user(pst_mac_vap->us_assoc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dmac_user))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_rx_rate_statistics_flag_callback::pst_dmac_user[%d] null.}",
            pst_mac_vap->us_assoc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_dmac_vap  = MAC_GET_DMAC_VAP(pst_mac_vap);
    pst_btcoex_btble_status = DMAC_VAP_GET_BTCOEX_STATUS(pst_dmac_vap);
    pst_dmac_vap_btcoex_rx_statistics = &(pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_rx_statistics);
    pst_dmac_user_btcoex_rx_info = &(pst_dmac_user->st_dmac_user_btcoex_stru.st_dmac_user_btcoex_rx_info);

    /* BTҵ����� */
    if (OAL_FALSE == pst_btcoex_btble_status->un_ble_status.st_ble_status.bit_bt_ba)
    {
        dmac_user_btcoex_delba_stru *pst_dmac_user_btcoex_delba;
        pst_dmac_user_btcoex_delba = &(pst_dmac_user->st_dmac_user_btcoex_stru.st_dmac_user_btcoex_delba);
        pst_dmac_vap_btcoex_rx_statistics->en_rx_rate_statistics_flag = OAL_FALSE;
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_dmac_vap_btcoex_rx_statistics->bt_coex_statistics_timer));
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_dmac_vap_btcoex_rx_statistics->bt_coex_low_rate_timer));

        if (BTCOEX_RX_WINDOW_SIZE_INDEX_3 != pst_dmac_user_btcoex_delba->en_ba_size_real_index)
        {
            pst_dmac_user_btcoex_delba->en_ba_size_real_index = BTCOEX_RX_WINDOW_SIZE_INDEX_3;
            pst_dmac_user_btcoex_delba->en_ba_size_expect_index = BTCOEX_RX_WINDOW_SIZE_INDEX_3;
            dmac_btcoex_update_ba_size(pst_mac_vap, pst_dmac_user_btcoex_delba, pst_btcoex_btble_status);
            dmac_btcoex_delba_trigger(pst_mac_vap, OAL_TRUE, pst_dmac_user_btcoex_delba);
        }

        OAL_MEMZERO(pst_dmac_user_btcoex_rx_info, OAL_SIZEOF(dmac_user_btcoex_rx_info_stru));

        return OAL_SUCC;
    }

    if (pst_dmac_user_btcoex_rx_info->us_rx_rate_stat_count < BTCOEX_RX_COUNT_LIMIT)
    {
        OAL_MEMZERO(pst_dmac_user_btcoex_rx_info, OAL_SIZEOF(dmac_user_btcoex_rx_info_stru));

        return OAL_SUCC;
    }
    else
    {

    }

    pst_dmac_vap_btcoex_rx_statistics->en_rx_rate_statistics_timeout = OAL_TRUE;

    return OAL_SUCC;
}


oal_uint32 dmac_btcoex_sco_rx_rate_statistics_flag_callback(oal_void *p_arg)
{

    dmac_user_btcoex_sco_rx_rate_status_stru *pst_dmac_user_btcoex_sco_rx_rate_status;
    hal_btcoex_btble_status_stru             *pst_btcoex_btble_status;
    dmac_vap_btcoex_rx_statistics_stru       *pst_dmac_vap_btcoex_rx_statistics;
    dmac_user_btcoex_rx_info_stru            *pst_btcoex_wifi_rx_rate_info;
    hal_to_dmac_chip_stru                    *pst_hal_chip;
    hal_to_dmac_device_stru                  *pst_hal_device = OAL_PTR_NULL;
    mac_vap_stru                             *pst_mac_vap;
    dmac_vap_stru                            *pst_dmac_vap;
    dmac_user_stru                           *pst_dmac_user;
    oal_uint32                                ul_rx_rate = 0;
    oal_uint16                                us_rx_count = 0;
    btcoex_rate_state_enum_uint8              en_notify_bt_value = 0;
    oal_bool_enum_uint8                       en_notify_bt = OAL_FALSE;

    pst_mac_vap = (mac_vap_stru *)p_arg;
    pst_dmac_vap = (dmac_vap_stru *)p_arg;
    if (OAL_FALSE == MAC_BTCOEX_CHECK_VALID_STA(pst_mac_vap))
    {
        return OAL_SUCC;
    }

    pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user(pst_mac_vap->us_assoc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dmac_user))
    {
        OAM_WARNING_LOG1(0, OAM_SF_COEX, "{dmac_btcoex_sco_rx_rate_statistics_flag_callback::pst_dmac_user[%d] null.}",
            pst_mac_vap->us_assoc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hal_chip = DMAC_VAP_GET_HAL_CHIP(pst_mac_vap);
    if (OAL_PTR_NULL == pst_hal_chip)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_sco_rx_rate_statistics_flag_callback:: DMAC_VAP_GET_HAL_DEVICE null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_btcoex_btble_status           = &(pst_hal_chip->st_btcoex_btble_status);
    pst_dmac_vap_btcoex_rx_statistics = &(pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_rx_statistics);
    pst_btcoex_wifi_rx_rate_info      = &(pst_dmac_user->st_dmac_user_btcoex_stru.st_dmac_user_btcoex_sco_rx_info);

    /* BTҵ����� */
    if (!pst_btcoex_btble_status->un_bt_status.st_bt_status.bit_bt_sco)
    {
        /* ����Ҳ����֮��ֱ��ɾ��64 */
        if (!pst_btcoex_btble_status->un_bt_status.st_bt_status.bit_bt_a2dp)
        {
            dmac_user_btcoex_delba_stru *pst_dmac_user_btcoex_delba;
            pst_dmac_user_btcoex_delba = &(pst_dmac_user->st_dmac_user_btcoex_stru.st_dmac_user_btcoex_delba);
            pst_dmac_user_btcoex_delba->en_ba_size_real_index = BTCOEX_RX_WINDOW_SIZE_INDEX_3;
            pst_dmac_user_btcoex_delba->en_ba_size_expect_index = BTCOEX_RX_WINDOW_SIZE_INDEX_3;
            dmac_btcoex_update_ba_size(pst_mac_vap, pst_dmac_user_btcoex_delba, pst_btcoex_btble_status);
            dmac_btcoex_delba_trigger(pst_mac_vap, OAL_TRUE, pst_dmac_user_btcoex_delba);
        }

        pst_dmac_vap_btcoex_rx_statistics->en_sco_rx_rate_statistics_flag = OAL_FALSE;
        OAL_MEMZERO(pst_btcoex_wifi_rx_rate_info, OAL_SIZEOF(dmac_user_btcoex_rx_info_stru));

        return OAL_SUCC;
    }

    /* ����������� */
    dmac_btcoex_rx_average_rate_calculate(pst_btcoex_wifi_rx_rate_info, &ul_rx_rate, &us_rx_count);

    pst_hal_device = DMAC_VAP_GET_HAL_DEVICE(pst_dmac_vap);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hal_device))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_sco_rx_rate_statistics_flag_callback:: hal device nullpointer!!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ����������� */
    if (pst_hal_device->st_hal_alg_stat.en_alg_distance_stat < HAL_ALG_USER_DISTANCE_FAR)
    {
        pst_dmac_user_btcoex_sco_rx_rate_status = &(pst_dmac_user->st_dmac_user_btcoex_stru.st_dmac_user_btcoex_sco_rx_rate_status);

        /* ���ݲ�ͬ����ȷ����ͬ�����ʵȼ�:���١����١����١������� */
        if (0 == us_rx_count)
        {
            switch (pst_dmac_user_btcoex_sco_rx_rate_status->en_status)
            {
                case BTCOEX_RATE_STATE_M:
                case BTCOEX_RATE_STATE_L:
                case BTCOEX_RATE_STATE_SL:
                    pst_dmac_user_btcoex_sco_rx_rate_status->en_status = BTCOEX_RATE_STATE_H;
                    en_notify_bt = OAL_TRUE;
                    break;
                default:
                    break;
            }
        }
        else if (us_rx_count <= 2)
        {
            switch (pst_dmac_user_btcoex_sco_rx_rate_status->en_status)
            {
            case BTCOEX_RATE_STATE_SL:
                pst_dmac_user_btcoex_sco_rx_rate_status->en_status = BTCOEX_RATE_STATE_L;
                en_notify_bt = OAL_TRUE;
                break;
            default:
                break;
            }
        }
        else if (1 == ul_rx_rate)
        {
            switch (pst_dmac_user_btcoex_sco_rx_rate_status->en_status)
            {
            case BTCOEX_RATE_STATE_H:
            case BTCOEX_RATE_STATE_M:
            case BTCOEX_RATE_STATE_L:
                pst_dmac_user_btcoex_sco_rx_rate_status->en_status = BTCOEX_RATE_STATE_SL;
                en_notify_bt = OAL_TRUE;
                break;
            default:
                break;
            }
        }
        else if (ul_rx_rate <= 11)
        {
            switch (pst_dmac_user_btcoex_sco_rx_rate_status->en_status)
            {
            case BTCOEX_RATE_STATE_H:
            case BTCOEX_RATE_STATE_M:
                pst_dmac_user_btcoex_sco_rx_rate_status->en_status = BTCOEX_RATE_STATE_L;
                en_notify_bt = OAL_TRUE;
                break;
            default:
                break;
            }
        }
        else if (ul_rx_rate < 35)
        {
            switch (pst_dmac_user_btcoex_sco_rx_rate_status->en_status)
            {
            case BTCOEX_RATE_STATE_H:
            case BTCOEX_RATE_STATE_L:
                pst_dmac_user_btcoex_sco_rx_rate_status->en_status = BTCOEX_RATE_STATE_M;
                en_notify_bt = OAL_TRUE;
                break;
            case BTCOEX_RATE_STATE_SL:
                pst_dmac_user_btcoex_sco_rx_rate_status->en_status = BTCOEX_RATE_STATE_L;
                en_notify_bt = OAL_TRUE;
                break;
            default:
                break;
            }
        }
        /* ���ʴ���50Mb/s������հ�������500������Ϊ��ǰ������������֪BT��ǰWifi���ڸ���״̬ */
        else if (us_rx_count >= 250 || ul_rx_rate >= 50)
        {
            switch (pst_dmac_user_btcoex_sco_rx_rate_status->en_status)
            {
            case BTCOEX_RATE_STATE_M:
            case BTCOEX_RATE_STATE_L:
                pst_dmac_user_btcoex_sco_rx_rate_status->en_status = BTCOEX_RATE_STATE_H;
                en_notify_bt = OAL_TRUE;
                break;
            case BTCOEX_RATE_STATE_SL:
                pst_dmac_user_btcoex_sco_rx_rate_status->en_status = BTCOEX_RATE_STATE_L;
                en_notify_bt = OAL_TRUE;
                break;
            default:
                break;
            }
        }
        else
        {}
        en_notify_bt_value = pst_dmac_user_btcoex_sco_rx_rate_status->en_status;

        /* ��ֹһֱ�ڳ����ٵ��µ绰̫�������ó�ʱ�ָ��ɵ��٣�����״̬���䣬��֪ͨBT��Ϣ�ı� */
        if (BTCOEX_RATE_STATE_SL == pst_dmac_user_btcoex_sco_rx_rate_status->en_status)
        {
            pst_dmac_user_btcoex_sco_rx_rate_status->uc_status_sl_time++;
            if (pst_dmac_user_btcoex_sco_rx_rate_status->uc_status_sl_time > (ALL_MID_PRIO_TIME + ALL_HIGH_PRIO_TIME))
            {
                pst_dmac_user_btcoex_sco_rx_rate_status->uc_status_sl_time = 0;
                en_notify_bt = OAL_TRUE;
                en_notify_bt_value = BTCOEX_RATE_STATE_SL;
            }
            else if (pst_dmac_user_btcoex_sco_rx_rate_status->uc_status_sl_time > ALL_MID_PRIO_TIME)
            {
                en_notify_bt = OAL_TRUE;
                en_notify_bt_value = BTCOEX_RATE_STATE_L;
            }
        }
        else
        {
            pst_dmac_user_btcoex_sco_rx_rate_status->uc_status_sl_time = 0;
        }

        if (OAL_TRUE == en_notify_bt)
        {
            hal_set_btcoex_soc_gpreg1(en_notify_bt_value, BIT4 | BIT5, 4);
            hal_coex_sw_irq_set(HAL_COEX_SW_IRQ_BT);
            OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_COEX,
                "{dmac_btcoex_sco_rx_rate_statistics_flag_callback::en_notify_bt_status: %d, en_status %d, uc_rate: %d, count %d.}",
                en_notify_bt_value, pst_dmac_user_btcoex_sco_rx_rate_status->en_status, ul_rx_rate, us_rx_count);
        }
    }

    return OAL_SUCC;
}


oal_uint32 dmac_config_btcoex_assoc_state_syn(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user)
{
    hal_to_dmac_chip_stru         *pst_hal_chip;
    hal_to_dmac_device_stru       *pst_hal_device;
    dmac_user_stru                *pst_dmac_user;
    dmac_vap_btcoex_stru          *pst_dmac_vap_btcoex;
    dmac_user_btcoex_delba_stru   *pst_dmac_user_btcoex_delba;
    hal_btcoex_btble_status_stru  *pst_btcoex_btble_status;
    ble_status_stru               *pst_ble_status;
    bt_status_stru                *pst_bt_status;
#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV)
    config_device_freq_h2d_stru    st_device_freq;
#endif
#endif
    oal_bool_enum_uint8            en_need_delba = OAL_FALSE;

    pst_hal_chip   = DMAC_VAP_GET_HAL_CHIP(pst_mac_vap);
    pst_hal_device = DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap);
    if (OAL_PTR_NULL == pst_hal_device)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_config_btcoex_assoc_state_syn:: DMAC_VAP_GET_HAL_DEVICE null}");
        return OAL_FAIL;
    }

    if(IS_STA(pst_mac_vap) || (IS_AP(pst_mac_vap) && (1 == pst_mac_vap->us_user_nums)))
    {
        /* ����ʱˢ��״̬��Ϣ֪ͨbt,sta����ʱ */
        dmac_btcoex_set_freq_and_work_state_hal_device(pst_hal_device);
    }

    pst_dmac_vap_btcoex = &(MAC_GET_DMAC_VAP(pst_mac_vap)->st_dmac_vap_btcoex);

    /* ��ʼ��premmpt���� */
    dmac_btcoex_init_preempt(pst_mac_vap, pst_mac_user);

    /* BTCOEX STA����Ҫ���� */
    if (OAL_FALSE == MAC_BTCOEX_CHECK_VALID_STA(pst_mac_vap))
    {
        OAM_WARNING_LOG3(0, OAM_SF_COEX, "{dmac_config_btcoex_assoc_state_syn::ba process skip! vap mode is %d, p2p mode is %d, vap_state: %d.}",
            pst_mac_vap->en_vap_mode, pst_mac_vap->en_p2p_mode, pst_mac_vap->en_vap_state);
        return OAL_SUCC;
    }

    pst_dmac_user = MAC_GET_DMAC_USER(pst_mac_user);

    pst_dmac_user_btcoex_delba = &(pst_dmac_user->st_dmac_user_btcoex_stru.st_dmac_user_btcoex_delba);

    pst_dmac_user_btcoex_delba->en_ba_size_expect_index = BTCOEX_RX_WINDOW_SIZE_INDEX_3;
    pst_dmac_user_btcoex_delba->en_ba_size_real_index   = BTCOEX_RX_WINDOW_SIZE_INDEX_3;

    /* ��ʼ��delba�ռ���������ѡ�����ɾ��BA���� */
    pst_dmac_user_btcoex_delba->en_user_nss_num         = pst_dmac_user->st_user_base_info.en_avail_num_spatial_stream;

    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_config_btcoex_assoc_state_syn::pst_dmac_user_btcoex_delba->en_user_nss_num[%d].}",
        pst_dmac_user_btcoex_delba->en_user_nss_num);

    dmac_btcoex_update_rx_rate_threshold(pst_mac_vap, pst_dmac_user_btcoex_delba);

    pst_btcoex_btble_status = &(pst_hal_chip->st_btcoex_btble_status);
#ifdef _PRE_WLAN_FEATURE_SMARTANT
    pst_hal_chip->st_dual_antenna_check_status.bit_bt_on = pst_btcoex_btble_status->un_bt_status.st_bt_status.bit_bt_on;
#endif
    pst_ble_status = &(pst_btcoex_btble_status->un_ble_status.st_ble_status);
    pst_bt_status = &(pst_btcoex_btble_status->un_bt_status.st_bt_status);

    if (0 == pst_ble_status->bit_bt_transfer && 0 == pst_ble_status->bit_bt_ba)
    {
        pst_dmac_user_btcoex_delba->uc_ba_size = 0;
        dmac_btcoex_delba_trigger(pst_mac_vap, OAL_FALSE, pst_dmac_user_btcoex_delba);
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_config_btcoex_assoc_state_syn::bt not working, ba size to default.}");
        return OAL_SUCC;
    }

    /* �绰�����½�������ͳ�ƣ�����ĿǰΪ0.5�� */
    if (pst_bt_status->bit_bt_sco)
    {
#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
        /* 02��rf����ָ�rf�Ĵ���ʱ����Ҫ��wifi��Ƶ */
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV)
        /* �ر��Զ���Ƶ */
        st_device_freq.uc_set_type = FREQ_SET_MODE;
        st_device_freq.uc_device_freq_enable = OAL_FALSE;
        dmac_config_set_device_freq(pst_mac_vap, 0, (oal_uint8 *)&st_device_freq);
#endif
#endif

        /* �绰�£���Ҫ��������ɾ��BA��ˢ��hmac���BA��� */
        en_need_delba = OAL_TRUE;

        pst_dmac_user_btcoex_delba->en_ba_size_expect_index = BTCOEX_RX_WINDOW_SIZE_INDEX_1;
        pst_dmac_vap_btcoex->st_dmac_vap_btcoex_rx_statistics.en_sco_rx_rate_statistics_flag = OAL_TRUE;

        if(OAL_TRUE == pst_dmac_vap_btcoex->st_dmac_vap_btcoex_rx_statistics.bt_coex_sco_statistics_timer.en_is_registerd)
        {
            FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_dmac_vap_btcoex->st_dmac_vap_btcoex_rx_statistics.bt_coex_sco_statistics_timer));
        }
        FRW_TIMER_CREATE_TIMER(&(pst_dmac_vap_btcoex->st_dmac_vap_btcoex_rx_statistics.bt_coex_sco_statistics_timer),
                                   dmac_btcoex_sco_rx_rate_statistics_flag_callback,
                                   BTCOEX_SCO_CALCULATE_TIME,
                                   (void *)pst_mac_vap,
                                   OAL_TRUE,
                                   OAM_MODULE_ID_DMAC,
                                   pst_mac_vap->ul_core_id);
    }
    else if (pst_bt_status->bit_bt_a2dp
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)
        || pst_ble_status->bit_bt_transfer
#endif
    )
    {
        pst_dmac_user_btcoex_delba->en_ba_size_expect_index = BTCOEX_RX_WINDOW_SIZE_INDEX_2;
        pst_dmac_vap_btcoex->st_dmac_vap_btcoex_rx_statistics.en_rx_rate_statistics_flag = OAL_TRUE;
        pst_dmac_vap_btcoex->st_dmac_vap_btcoex_rx_statistics.en_rx_rate_statistics_timeout = OAL_FALSE;

        if(OAL_TRUE == pst_dmac_vap_btcoex->st_dmac_vap_btcoex_rx_statistics.bt_coex_statistics_timer.en_is_registerd)
        {
            FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_dmac_vap_btcoex->st_dmac_vap_btcoex_rx_statistics.bt_coex_statistics_timer));
        }
        FRW_TIMER_CREATE_TIMER(&(pst_dmac_vap_btcoex->st_dmac_vap_btcoex_rx_statistics.bt_coex_statistics_timer),
                                   dmac_btcoex_rx_rate_statistics_flag_callback,
                                   BTCOEX_RX_STATISTICS_TIME,
                                   (void *)pst_mac_vap,
                                   OAL_TRUE,
                                   OAM_MODULE_ID_DMAC,
                                   pst_mac_vap->ul_core_id);
    }
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV)
    else if(pst_ble_status->bit_bt_transfer)
    {
        dmac_alg_cfg_btcoex_state_notify(pst_hal_device, BT_TRANSFER_ON);

        pst_dmac_user_btcoex_delba->en_ba_size_expect_index = BTCOEX_RX_WINDOW_SIZE_INDEX_2;
        pst_dmac_user_btcoex_delba->en_ba_size_real_index = BTCOEX_RX_WINDOW_SIZE_INDEX_2;

        en_need_delba = OAL_TRUE;
    }
#endif

    dmac_btcoex_update_ba_size(pst_mac_vap, pst_dmac_user_btcoex_delba, pst_btcoex_btble_status);

    OAM_WARNING_LOG_ALTER(pst_mac_vap->uc_vap_id, OAM_SF_COEX,
        "{dmac_config_btcoex_assoc_state_syn::status sco:%d, a2dp:%d, data_transfer:%d, experc_ba_size:%d, ba_size:%d, en_need_delba:%d.}",
        6, pst_bt_status->bit_bt_sco, pst_bt_status->bit_bt_a2dp, pst_ble_status->bit_bt_transfer,
           pst_dmac_user_btcoex_delba->en_ba_size_expect_index, pst_dmac_user_btcoex_delba->uc_ba_size, en_need_delba);

    dmac_btcoex_delba_trigger(pst_mac_vap, en_need_delba, pst_dmac_user_btcoex_delba);

    return OAL_SUCC;
}


oal_uint32 dmac_config_btcoex_disassoc_state_syn(mac_vap_stru *pst_mac_vap)
{
    dmac_vap_btcoex_rx_statistics_stru *pst_dmac_vap_btcoex_rx_statistics;
    dmac_vap_btcoex_occupied_stru      *pst_dmac_vap_btcoex_occupied;

    if(IS_STA(pst_mac_vap) || (IS_AP(pst_mac_vap) && (0 == pst_mac_vap->us_user_nums)))
    {
        /* sta downʱ,ˢ��Ƶ�Σ�legacy vap��p2p ����״̬,������ŵ� */
        dmac_btcoex_set_freq_and_work_state_hal_device(DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap));
    }

#if 0 //���εȳ������л���5G����Ҫ���ͳ����Ϣ
    if (OAL_FALSE == MAC_BTCOEX_CHECK_VALID_STA(pst_mac_vap))
    {
        OAM_WARNING_LOG2(0, OAM_SF_COEX, "{dmac_config_btcoex_disassoc_state_syn::ba process skip! vap mode is %d, p2p mode is %d.}",
                pst_mac_vap->en_vap_mode, pst_mac_vap->en_p2p_mode);
        return OAL_SUCC;
    }
#endif

    pst_dmac_vap_btcoex_rx_statistics = &(MAC_GET_DMAC_VAP(pst_mac_vap)->st_dmac_vap_btcoex.st_dmac_vap_btcoex_rx_statistics);
    pst_dmac_vap_btcoex_occupied = &(MAC_GET_DMAC_VAP(pst_mac_vap)->st_dmac_vap_btcoex.st_dmac_vap_btcoex_occupied);

    FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_dmac_vap_btcoex_rx_statistics->bt_coex_statistics_timer));
    FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_dmac_vap_btcoex_rx_statistics->bt_coex_low_rate_timer));
    FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_dmac_vap_btcoex_rx_statistics->bt_coex_sco_statistics_timer));
    FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_dmac_vap_btcoex_occupied->bt_coex_occupied_timer));
    FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_dmac_vap_btcoex_occupied->bt_coex_priority_timer));

    return OAL_SUCC;
}


oal_void dmac_btcoex_clear_fake_queue_check(mac_vap_stru *pst_mac_vap)
{
    hal_to_dmac_device_stru         *pst_hal_device;

    pst_hal_device = DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap);

    if(OAL_TRUE == HAL_BTCOEX_CHECK_SW_PREEMPT_ON(pst_hal_device))
    {
        /* ������ٶ�����֡�ķ�����������buf�ͷ� */
        dmac_vap_clear_fake_queue(pst_mac_vap);
    }

    OAM_WARNING_LOG1(0, OAM_SF_COEX, "dmac_btcoex_clear_fake_queue_check:: sw preempt type[%d]!", GET_HAL_BTCOEX_SW_PREEMPT_TYPE(pst_hal_device));
}


OAL_STATIC oal_uint32 dmac_btcoex_sco_status_handler(frw_event_mem_stru *pst_event_mem)
{
    dmac_vap_btcoex_rx_statistics_stru *pst_dmac_vap_btcoex_rx_statistics;
    dmac_user_btcoex_delba_stru        *pst_dmac_user_btcoex_delba;
    hal_btcoex_btble_status_stru       *pst_btble_status;
    frw_event_stru                     *pst_event;
    hal_chip_stru                      *pst_hal_chip;
    hal_device_stru                    *pst_hal_device;
    dmac_user_stru                     *pst_dmac_user;
    bt_status_stru                     *pst_bt_status;
    oal_bool_enum_uint8                 en_need_delba = OAL_TRUE;
    oal_bool_enum_uint8                 en_periodic;
    oal_uint16                          us_timeout_ms;
#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV)
    device_pps_freq_level_stru*         device_ba_pps_freq_level;
    config_device_freq_h2d_stru         st_device_freq;
    oal_uint8                           uc_index;
#endif
#endif
    oal_uint8                           uc_vap_idx;
    oal_uint8                           uc_valid_vap_num;
    oal_uint8                           auc_mac_vap_id[WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE];
    mac_vap_stru                       *pst_mac_vap[WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE] = {OAL_PTR_NULL};

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_sco_status_handler::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event = frw_get_event_stru(pst_event_mem);

    /* ��ȡchipָ�� */
    pst_hal_chip = HAL_GET_CHIP_POINTER(pst_event->st_event_hdr.uc_chip_id);

    /* ��ʱֻ�Ǵ�����·��STA ��·TBD */
    pst_hal_device = &(pst_hal_chip->ast_device[HAL_DEVICE_ID_MASTER]);

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)
#ifdef _PRE_WLAN_1103_PILOT

#else
        
        hal_btcoex_open_5g_upc();
#endif
        /* ˢ��ps stop��� */
        dmac_btcoex_ps_stop_check_and_notify(HAL_DEV_GET_HAL2DMC_DEV(pst_hal_device));
#endif

    pst_btble_status = &(pst_hal_chip->st_hal_chip_base.st_btcoex_btble_status);
    pst_bt_status    = &(pst_btble_status->un_bt_status.st_bt_status);

    /* �ҵ�����Ҫ���vap���� */
    uc_valid_vap_num = dmac_btcoex_find_all_valid_sta_per_device(HAL_DEV_GET_HAL2DMC_DEV(pst_hal_device), auc_mac_vap_id);

    /* valid��vap�豸������Ӧ���� 02ֻ����legacy sta   03Ҫ����gc sta */
    for (uc_vap_idx = 0; uc_vap_idx < uc_valid_vap_num; uc_vap_idx++)
    {
        pst_mac_vap[uc_vap_idx]  = (mac_vap_stru *)mac_res_get_mac_vap(auc_mac_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_mac_vap[uc_vap_idx])
        {
            OAM_ERROR_LOG1(0, OAM_SF_COEX, "{dmac_btcoex_sco_status_handler::pst_mac_vap[%d] IS NULL.}", auc_mac_vap_id[uc_vap_idx]);
            return OAL_ERR_CODE_PTR_NULL;
        }

        /* ���valid sta����Ӧ���� */
#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
        /* 02��rf����ָ�rf�Ĵ���ʱ����Ҫ��wifi��Ƶ */
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV)
        st_device_freq.uc_set_type = FREQ_SET_MODE;
        if (!pst_bt_status->bit_bt_sco)
        {
            /* �����Զ���Ƶ */
            st_device_freq.uc_device_freq_enable = OAL_TRUE;
            device_ba_pps_freq_level = dmac_get_ba_pps_freq_level();
            for(uc_index = 0; uc_index < 4; uc_index++)
            {
                st_device_freq.st_device_data[uc_index].ul_speed_level = device_ba_pps_freq_level->ul_speed_level;
                st_device_freq.st_device_data[uc_index].ul_cpu_freq_level = device_ba_pps_freq_level->ul_cpu_freq_level;
                device_ba_pps_freq_level++;
            }

            dmac_config_set_device_freq(pst_mac_vap[uc_vap_idx], 0, (oal_uint8 *)&st_device_freq);
        }
#endif
#endif

        pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user((pst_mac_vap[uc_vap_idx])->us_assoc_vap_id);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dmac_user))
        {
            return OAL_ERR_CODE_PTR_NULL;
        }

        pst_dmac_vap_btcoex_rx_statistics = &(MAC_GET_DMAC_VAP(pst_mac_vap[uc_vap_idx])->st_dmac_vap_btcoex.st_dmac_vap_btcoex_rx_statistics);
        pst_dmac_user_btcoex_delba = &(pst_dmac_user->st_dmac_user_btcoex_stru.st_dmac_user_btcoex_delba);

        /* ����bt�򿪺͹رճ����£��ԾۺϽ��д��� */
        if (pst_bt_status->bit_bt_sco)
        {
#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
            /* 02��rf����ָ�rf�Ĵ���ʱ����Ҫ��wifi��Ƶ */
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV)
            /* �ر��Զ���Ƶ */
            st_device_freq.uc_device_freq_enable = OAL_FALSE;
            dmac_config_set_device_freq(pst_mac_vap[uc_vap_idx], 0, (oal_uint8 *)&st_device_freq);
#endif
#endif
            /* �绰ģʽ��Ҫ�̶���BTCOEX_RX_WINDOW_SIZE_INDEX_1�ۺϷ�ʽ���͵�ǰ�ۺϸ���һ�²���Ҫ����ɾ��BA����������ɾ��BA */
            pst_dmac_user_btcoex_delba->en_ba_size_expect_index = BTCOEX_RX_WINDOW_SIZE_INDEX_1;
            if (pst_dmac_user_btcoex_delba->en_ba_size_expect_index == pst_dmac_user_btcoex_delba->en_ba_size_real_index)
            {
                en_need_delba = OAL_FALSE;
            }

            pst_dmac_vap_btcoex_rx_statistics->en_sco_rx_rate_statistics_flag = OAL_TRUE;
            us_timeout_ms = BTCOEX_SCO_CALCULATE_TIME;
            en_periodic = OAL_TRUE;
        }
        else
        {
            /* ֻ�е绰���ֶ�û�е�����Ž��лָ���64�ľۺ� */
            if(!pst_btble_status->un_bt_status.st_bt_status.bit_bt_a2dp)
            {
                pst_dmac_user_btcoex_delba->en_ba_size_expect_index = BTCOEX_RX_WINDOW_SIZE_INDEX_3;
                hal_set_btcoex_soc_gpreg1(0, BIT4 | BIT5, 4);
                en_need_delba = OAL_FALSE;
                pst_dmac_vap_btcoex_rx_statistics->en_sco_rx_rate_statistics_flag = OAL_FALSE;
                us_timeout_ms = BTCOEX_RX_STATISTICS_TIME;
                en_periodic = OAL_FALSE;
            }
            /* �绰�ҶϺ������ֵĳ�����������BA��ɾ���������������̴��� */
            else
            {
                return OAL_SUCC;
            }
        }

        if(OAL_TRUE == pst_dmac_vap_btcoex_rx_statistics->bt_coex_sco_statistics_timer.en_is_registerd)
        {
            FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_dmac_vap_btcoex_rx_statistics->bt_coex_sco_statistics_timer));
        }

        FRW_TIMER_CREATE_TIMER(&(pst_dmac_vap_btcoex_rx_statistics->bt_coex_sco_statistics_timer),
                                           dmac_btcoex_sco_rx_rate_statistics_flag_callback,
                                           us_timeout_ms,
                                           (void *)(pst_mac_vap[uc_vap_idx]),
                                           en_periodic,
                                           OAM_MODULE_ID_DMAC,
                                           (pst_mac_vap[uc_vap_idx])->ul_core_id);

        dmac_btcoex_update_ba_size(pst_mac_vap[uc_vap_idx], pst_dmac_user_btcoex_delba, pst_btble_status);

        dmac_btcoex_delba_trigger(pst_mac_vap[uc_vap_idx], en_need_delba, pst_dmac_user_btcoex_delba);
    }

    return OAL_SUCC;
}

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)

oal_void  dmac_btcoex_ps_timeout_update_time(hal_to_dmac_device_stru *pst_hal_device)
{
    hal_btcoex_ps_status_enum_uint8 en_ps_status = 0;

    /* ��ȡ��ǰpsҵ��״̬ */
    hal_btcoex_get_ps_service_status(pst_hal_device, &en_ps_status);

    if(HAL_BTCOEX_PS_STATUE_ACL == en_ps_status)
    {
        pst_hal_device->st_btcoex_sw_preempt.us_timeout_ms = BTCOEX_POWSAVE_ACL_TIMEOUT;
    }
    else if(HAL_BTCOEX_PS_STATUE_PAGE_INQ == en_ps_status)
    {
        pst_hal_device->st_btcoex_sw_preempt.us_timeout_ms = BTCOEX_POWSAVE_PAGE_TIMEOUT;
    }
    else if(HAL_BTCOEX_PS_STATUE_BOTH == en_ps_status)
    {
        pst_hal_device->st_btcoex_sw_preempt.us_timeout_ms = BTCOEX_POWSAVE_ACL_PAGE_TIMEOUT;
    }
    else
    {
        /* ������page��inquiryҵ��Ļ���Ĭ�ϰ���acl������ */
        pst_hal_device->st_btcoex_sw_preempt.us_timeout_ms = BTCOEX_POWSAVE_ACL_TIMEOUT;
        OAM_INFO_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_ps_timeout_update_time:: en_ps_status[0], uc_timeout_ms keep acl.}");
    }
}


oal_uint32 dmac_btcoex_rx_data_bt_acl_check(mac_vap_stru *pst_mac_vap)
{
    hal_to_dmac_device_stru            *pst_hal_device;
    mac_device_stru                    *pst_mac_device;

    pst_hal_device = DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap);
    if (OAL_PTR_NULL == pst_hal_device)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_rx_data_bt_acl_check::pst_hal_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ֻ����STA */
    if (1 != hal_device_calc_up_vap_num(pst_hal_device))
    {
        return OAL_SUCC;
    }

    /* ��Ϊlegacy staֱ�ӷ��أ��ɹ����ҵ�, һ��hal device�ϲ�����������legacy STA,��������GC�ٿ�����չ */
    if (OAL_FALSE == MAC_BTCOEX_CHECK_VALID_STA(pst_mac_vap))
    {
        return OAL_SUCC;
    }

    /* ��ȡdevice */
    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_rx_data_bt_acl_check::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��̬slot����ʹ�� TBD */

    return OAL_SUCC;
}


oal_void dmac_btcoex_ps_stop_check_and_notify(hal_to_dmac_device_stru *pst_hal_device)
{
    mac_device_stru      *pst_mac_device = OAL_PTR_NULL;
    mac_vap_stru         *pst_mac_vap    = OAL_PTR_NULL;
    oal_bool_enum_uint8   en_sco_status  = OAL_FALSE;
    oal_bool_enum_uint8   en_ps_stop     = OAL_FALSE;  /* ��ʼ�Ǵ�ps */
    oal_uint8             uc_vap_idx;
    oal_uint8             uc_up_vap_num;
    oal_uint8             uc_up_ap_num     = 0;
    oal_uint8             uc_up_sta_num    = 0;
    oal_uint8             uc_up_2g_num     = 0;
    oal_uint8             uc_up_5g_num     = 0;
    oal_uint8             auc_mac_vap_id[WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE];

    pst_mac_device = mac_res_get_dev(pst_hal_device->uc_mac_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG1(0, OAM_SF_COEX, "{dmac_btcoex_ps_stop_check_and_notify: mac device is null ptr. device id:%d}", pst_hal_device->uc_mac_device_id);
        return;
    }

    /* 1.�绰���� */
    hal_btcoex_get_bt_sco_status(pst_hal_device, &en_sco_status);
    if (OAL_TRUE == en_sco_status)
    {
        en_ps_stop = OAL_TRUE;
    }

    /* 2. dbac������,ֱ��return */
    if ((OAL_TRUE == mac_is_dbac_running(pst_mac_device)))
    {
        en_ps_stop = OAL_TRUE;
    }

    /* Hal Device����work״̬vap���� */
    uc_up_vap_num = hal_device_find_all_up_vap(pst_hal_device, auc_mac_vap_id);
    for (uc_vap_idx = 0; uc_vap_idx < uc_up_vap_num; uc_vap_idx++)
    {
        pst_mac_vap  = (mac_vap_stru *)mac_res_get_mac_vap(auc_mac_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_mac_vap)
        {
            continue;
        }

        /* �����û�����Ϊ0 */
        if(0 != pst_mac_vap->us_user_nums)
        {
            (IS_AP(pst_mac_vap)) ?(uc_up_ap_num++):(uc_up_sta_num++);

            (OAL_TRUE == MAC_BTCOEX_CHECK_VALID_VAP(pst_mac_vap))?(uc_up_2g_num++):(uc_up_5g_num++);
        }
    }

    /* 3.��apģʽ  */
    if(0 != uc_up_ap_num && 0 == uc_up_sta_num)
    {
        en_ps_stop = OAL_TRUE;
    }

    /* 4.5gģʽ  */
    if(0 != uc_up_5g_num && 0 == uc_up_2g_num)
    {
        en_ps_stop = OAL_TRUE;
    }

    /* ˢ��ps���� */
    GET_HAL_BTCOEX_SW_PREEMPT_PS_STOP(pst_hal_device) = en_ps_stop;

    hal_set_btcoex_soc_gpreg1(en_ps_stop, BIT7, 7);  //ps��ֹ״̬֪ͨ

    hal_coex_sw_irq_set(HAL_COEX_SW_IRQ_BT);

    OAM_WARNING_LOG_ALTER(0, OAM_SF_COEX,
        "{dmac_btcoex_ps_stop_check_and_notify::up_ap_num[%d]up_sta_num[%d]5g_num[%d]2g_num[%d]sco_status[%d]dbac_status[%d]ps_stop[%d]!}",
        7, uc_up_ap_num, uc_up_sta_num, uc_up_5g_num, uc_up_2g_num, en_sco_status,
        mac_is_dbac_running(pst_mac_device), GET_HAL_BTCOEX_SW_PREEMPT_PS_STOP(pst_hal_device));
}


oal_void dmac_btcoex_ps_pause_check_and_notify(hal_to_dmac_device_stru *pst_hal_device)
{
    mac_vap_stru         *pst_mac_vap    = OAL_PTR_NULL;
    oal_bool_enum_uint8   en_ps_pause     = OAL_FALSE;  /* ��ʼ�ǲ���ͣps */
    oal_uint8             uc_vap_idx;
    oal_uint8             uc_up_vap_num;
    oal_uint8             auc_mac_vap_id[WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE];

    /* Hal Device����work״̬vap���� */
    uc_up_vap_num = hal_device_find_all_up_vap(pst_hal_device, auc_mac_vap_id);
    for (uc_vap_idx = 0; uc_vap_idx < uc_up_vap_num; uc_vap_idx++)
    {
        pst_mac_vap  = (mac_vap_stru *)mac_res_get_mac_vap(auc_mac_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_mac_vap)
        {
            continue;
        }

        /* 1.�������ι�������Ҫ��ͣps */
        if(MAC_VAP_STATE_ROAMING == pst_mac_vap->en_vap_state)
        {
            en_ps_pause = OAL_TRUE;
        }
    }

    /* ˢ��ps���� */
    GET_HAL_BTCOEX_SW_PREEMPT_PS_PAUSE(pst_hal_device) = en_ps_pause;

    OAM_WARNING_LOG1(0, OAM_SF_COEX, "{dmac_btcoex_ps_pause_check_and_notify::en_ps_pause[%d]!}",
        GET_HAL_BTCOEX_SW_PREEMPT_PS_PAUSE(pst_hal_device));
}


oal_void  dmac_btcoex_set_resp_bit_ctrl(hal_to_dmac_device_stru *pst_hal_device, oal_bool_enum_uint8 en_enable)
{
#ifdef _PRE_WLAN_1103_PILOT
    oal_uint8                           uc_vap_idx;
    oal_uint8                           uc_valid_vap_num;
    oal_uint8                           auc_mac_vap_id[WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE];
    mac_vap_stru                       *pst_mac_vap[WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE] = {OAL_PTR_NULL};
    dmac_user_stru                     *pst_dmac_user;

    /* �ҵ�����Ҫ���vap������ֻ����sta */
    uc_valid_vap_num = dmac_btcoex_find_all_valid_sta_per_device(pst_hal_device, auc_mac_vap_id);

    /* valid��vap�豸������Ӧ���� */
    for (uc_vap_idx = 0; uc_vap_idx < uc_valid_vap_num; uc_vap_idx++)
    {
        pst_mac_vap[uc_vap_idx]  = (mac_vap_stru *)mac_res_get_mac_vap(auc_mac_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_mac_vap[uc_vap_idx])
        {
            OAM_ERROR_LOG1(auc_mac_vap_id[uc_vap_idx], OAM_SF_COEX, "{dmac_btcoex_set_resp_bit_ctrl::pst_mac_vap[%d] IS NULL.}", auc_mac_vap_id[uc_vap_idx]);
            return;
        }

        pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user((pst_mac_vap[uc_vap_idx])->us_assoc_vap_id);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dmac_user))
        {
            OAM_WARNING_LOG1(auc_mac_vap_id[uc_vap_idx], OAM_SF_COEX, "{dmac_btcoex_set_resp_bit_ctrl::pst_dmac_user[%d] IS NULL.}", (pst_mac_vap[uc_vap_idx])->us_assoc_vap_id);
            continue;
        }

        if(OAL_TRUE == en_enable)
        {
            hal_tx_enable_resp_ps_bit_ctrl(pst_hal_device, pst_dmac_user->uc_lut_index);
        }
        else
        {
            hal_tx_disable_resp_ps_bit_ctrl(pst_hal_device, pst_dmac_user->uc_lut_index);
        }
    }
#endif
}


oal_uint32 dmac_btcoex_pow_save_callback(oal_void *p_arg)
{
    hal_to_dmac_device_stru   *pst_h2d_device = (hal_to_dmac_device_stru *)p_arg;
    mac_device_stru           *pst_mac_device;

    pst_mac_device = mac_res_get_dev(pst_h2d_device->uc_mac_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_pow_save_callback::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* �ǹ���״̬����ֱ�ӻָ�,����Ҫ���� */
    if (GET_HAL_DEVICE_STATE(pst_h2d_device) == HAL_DEVICE_IDLE_STATE)
    {

    }
    /* ��scan״̬ʱ��ɨ�費����͹��ģ���ʱ���õ��ĵ͹��ģ����ɨ����ʱ */
    else if(GET_HAL_DEVICE_STATE(pst_h2d_device) == HAL_DEVICE_SCAN_STATE)
    {
        switch(GET_HAL_BTCOEX_SW_PREEMPT_TYPE(pst_h2d_device))
        {
            case HAL_BTCOEX_SW_POWSAVE_SCAN_BEGIN:
                /* ���ɨ��һ��ʼ��ps��ϣ���ʱps���ָ�����ɨ�� */
                dmac_scan_begin(pst_mac_device, pst_h2d_device);
                OAM_INFO_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_pow_save_callback::dmac_scan_begin start.}");
                break;

            case HAL_BTCOEX_SW_POWSAVE_SCAN_WAIT:
                /* ���ɨ����ʱ��home channel������������ϣ���ʱps���ָ�������home channel���� */
                dmac_scan_switch_home_channel_work(pst_mac_device, pst_h2d_device);
                OAM_WARNING_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_pow_save_callback::dmac_scan_switch_home_channel_work start.}");
                break;

            case HAL_BTCOEX_SW_POWSAVE_SCAN_END:
                /* ���ɨ���������ϣ���ʱps���ָ�����ɨ�� */
                OAM_WARNING_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_pow_save_callback::dmac_scan_handle_switch_channel_back start.}");
                //ͳһ���ýӿ�dmac_scan_prepare_endʵ��
                //dmac_scan_handle_switch_channel_back(pst_mac_device, pst_h2d_device, &(pst_h2d_device->st_hal_scan_params));
                //hal_device_handle_event(pst_h2d_device, HAL_DEVICE_EVENT_SCAN_END, 0, OAL_PTR_NULL);
                dmac_scan_prepare_end(pst_mac_device, pst_h2d_device);

                break;

            case HAL_BTCOEX_SW_POWSAVE_SCAN_ABORT:
                /* abort״̬һ���ǣ���save״̬ʱ��scan baort��������ps=0Ҫ�ָ�ps=1�����ã���Ϊscan abort��ǰresume�ˣ��˴�����Ҫ����  */
                OAM_WARNING_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_pow_save_callback::scan is already abort and resume.}");

                /* abort�Ѿ�ǿ����Ϊ�ָ����˴β�������vap�Ѿ�vap�˲���Ҫ������ */
                /* �ָ����ͺͽ��� */
                //dmac_vap_resume_tx_by_chl(pst_mac_device, pst_h2d_device, &(pst_h2d_device->st_wifi_channel_status));
                break;

            case HAL_BTCOEX_SW_POWSAVE_IDLE:
                /* ������״̬1�¼�û�м�ʱ�����°벿��������״̬0����ʱ����ɨ�裬�ָ�����ɨ���������� */
                OAM_WARNING_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_pow_save_callback::HAL_BTCOEX_SW_POWSAVE_IDLE.}");
                break;

            case HAL_BTCOEX_SW_POWSAVE_WORK:
                /* ���ɨ���ڼ�����1������0����ʱ�ָ�����ɨ���Լ��ָ�,ps=0�������� */
                OAM_INFO_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_pow_save_callback::scan is running.}");
                break;

            default:
                OAM_WARNING_LOG1(0, OAM_SF_COEX, "{dmac_btcoex_pow_save_callback::en_sw_preempt_type[%d] error.}",
                    GET_HAL_BTCOEX_SW_PREEMPT_TYPE(pst_h2d_device));
        }
    }
    else
    {
        switch(GET_HAL_BTCOEX_SW_PREEMPT_TYPE(pst_h2d_device))
        {
            case HAL_BTCOEX_SW_POWSAVE_PSM_END:
             /* ��ִ���˵͹��Ļָ�����Ҫ������ */
             OAM_WARNING_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_pow_save_callback:: powerdown already resume.}");
             break;

            case HAL_BTCOEX_SW_POWSAVE_IDLE:
             /* �����ǵ͹���ps=1�¼��ſ�ʼִ�У���ʱ��ȡ�Ĵ���״̬=0����ǰ��0״̬���Ͱ���0�������ɣ�������������0���¼����� */
             OAM_WARNING_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_pow_save_callback:: current is normal!.}");
             break;

            case HAL_BTCOEX_SW_POWSAVE_SCAN_ABORT:
             /* ǿ��scan abort�ָ��ˣ��˴�����Ҫ�ָ� */
             OAM_WARNING_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_pow_save_callback:: work state scan abort already resume.}");
             break;

            case HAL_BTCOEX_SW_POWSAVE_WORK:
             /* �ָ����ͺͽ��� */
             if(HAL_BTCOEX_SW_POWSAVE_SUB_IDLE == GET_HAL_BTCOEX_SW_PREEMPT_SUBTYPE(pst_h2d_device))
             {
                 GET_HAL_BTCOEX_SW_PREEMPT_SUBTYPE(pst_h2d_device) = HAL_BTCOEX_SW_POWSAVE_SUB_ACTIVE;
                 OAM_WARNING_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_pow_save_callback:: work HAL_BTCOEX_SW_POWSAVE_SUB_IDLE.}");
             }
             else if(HAL_BTCOEX_SW_POWSAVE_SUB_SCAN == GET_HAL_BTCOEX_SW_PREEMPT_SUBTYPE(pst_h2d_device))
             {
                 GET_HAL_BTCOEX_SW_PREEMPT_SUBTYPE(pst_h2d_device) = HAL_BTCOEX_SW_POWSAVE_SUB_ACTIVE;
                 OAM_WARNING_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_pow_save_callback:: work HAL_BTCOEX_SW_POWSAVE_SUB_SCAN.}");
             }
             else
             {
                 dmac_m2s_switch_device_end(pst_h2d_device);
             }
             break;

            default:
             OAM_WARNING_LOG1(0, OAM_SF_COEX, "{dmac_btcoex_pow_save_callback::en_sw_preempt_type[%d] error.}",
                 GET_HAL_BTCOEX_SW_PREEMPT_TYPE(pst_h2d_device));
        }
    }

#ifdef _PRE_WLAN_1103_PILOT
    if(OAL_TRUE == HAL_BTCOEX_CHECK_SW_PREEMPT_RSP_FRAME_PS_ON(pst_h2d_device))
    {
        /* �ָ���Ӧ֡ps��λ */
        hal_tx_disable_resp_ps_bit_ctrl_all(pst_h2d_device);
        //dmac_btcoex_set_resp_bit_ctrl(pst_h2d_device, OAL_FALSE);
    }
#endif

    if(OAL_FALSE == HAL_BTCOEX_CHECK_SW_PREEMPT_REPLY_CTS_ON(pst_h2d_device))
    {
        /* �ָ�Ӳ����cts */
        hal_enable_machw_cts_trans(pst_h2d_device);
    }

    /* preempt������ΪNONE��ʽ */
    GET_HAL_BTCOEX_SW_PREEMPT_TYPE(pst_h2d_device) = HAL_BTCOEX_SW_POWSAVE_TIMEOUT;

    OAM_WARNING_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_pow_save_callback::time is up.}");

    return OAL_SUCC;
}


OAL_STATIC oal_void dmac_btcoex_ps_start_handle(hal_to_dmac_device_stru *pst_hal_device, mac_device_stru *pst_mac_device)
{
    /* preempt���ƻ�����Ϊ�����ʽ */
    GET_HAL_BTCOEX_SW_PREEMPT_TYPE(pst_hal_device) = HAL_BTCOEX_SW_POWSAVE_WORK;

    /* ״̬��Ǩ,������ʱ��50ms��Ҫ���ps��������ֹ����wifi������ */
    if(OAL_TRUE == pst_hal_device->st_btcoex_powersave_timer.en_is_registerd)
    {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_hal_device->st_btcoex_powersave_timer));
    }

    /* ps��������ʱ����Ҫ���ݵ�ǰ״̬��ˢ�³�ʱ��ʱ��ʱ�� */
    dmac_btcoex_ps_timeout_update_time(pst_hal_device);

    FRW_TIMER_CREATE_TIMER(&(pst_hal_device->st_btcoex_powersave_timer),
                           dmac_btcoex_pow_save_callback,
                           pst_hal_device->st_btcoex_sw_preempt.us_timeout_ms,
                           (oal_void *)pst_hal_device,
                           OAL_FALSE,
                           OAM_MODULE_ID_DMAC,
                           pst_hal_device->ul_core_id);

    /* ������ܻ��ƴ��� */
    dmac_m2s_switch_device_begin(pst_mac_device, pst_hal_device);

    if(OAL_FALSE == HAL_BTCOEX_CHECK_SW_PREEMPT_REPLY_CTS_ON(pst_hal_device))
    {
        /* ��ֹӲ����cts */
        hal_disable_machw_cts_trans(pst_hal_device);
    }
}


oal_void dmac_btcoex_ps_stop_handle(hal_to_dmac_device_stru *pst_hal_device)
{
    if(OAL_FALSE == HAL_BTCOEX_CHECK_SW_PREEMPT_REPLY_CTS_ON(pst_hal_device))
    {
        /* �ָ�Ӳ����cts */
        hal_enable_machw_cts_trans(pst_hal_device);
    }

    /* ps״̬0�жϱ����Σ�û�м�ʱ��������ʱ��Ҫ�ָ��������ã�Ҫ��Ҫ�ָ������ɵ͹����Լ����� */
    if(OAL_TRUE == pst_hal_device->st_btcoex_powersave_timer.en_is_registerd)
    {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_hal_device->st_btcoex_powersave_timer));
    }

    /* �������ps״̬����Ҫresume */
    if(HAL_BTCOEX_SW_POWSAVE_WORK == GET_HAL_BTCOEX_SW_PREEMPT_TYPE(pst_hal_device))
    {
        OAM_WARNING_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_ps_stop_handle: device resume start.}");

        dmac_m2s_switch_device_end(pst_hal_device);
    }

    /* �͹��������ps���ƻָ��� ps�Լ����ж���ʱ���ж�Ϊ�ˣ��Ͳ������� */
    GET_HAL_BTCOEX_SW_PREEMPT_TYPE(pst_hal_device) = HAL_BTCOEX_SW_POWSAVE_PSM_END;

}


oal_void dmac_btcoex_ps_powerdown_recover_handle(hal_to_dmac_device_stru *pst_hal_device)
{
    mac_device_stru                *pst_mac_device   = OAL_PTR_NULL;
    oal_bool_enum_uint8             en_bt_acl_status = OAL_FALSE;

    /* �������forbit״̬��˵���Ѿ���������Ҫ�ȵ�soc ps=0�ж������ѣ�ֱ�ӷ��أ���ֹwifiҵ���쳣 */
    if(GET_HAL_BTCOEX_SW_PREEMPT_SUBTYPE(pst_hal_device) == HAL_BTCOEX_SW_POWSAVE_SUB_PSM_FORBIT)
    {
        return;
    }

    /* 1.���psδʹ�� */
    if (OAL_FALSE == HAL_BTCOEX_CHECK_SW_PREEMPT_ON(pst_hal_device))
    {
        /* δʹ�ܣ�ֱ�ӷ��� */
        return;
    }

    /* 2.wifi����idle״̬ */
    if (HAL_DEVICE_IDLE_STATE == GET_HAL_DEVICE_STATE(pst_hal_device))
    {
        return;
    }

    /* 3.ҵ��ps stop��� */
    if(OAL_TRUE == GET_HAL_BTCOEX_SW_PREEMPT_PS_STOP(pst_hal_device))
    {
        return;
    }

    /* 4.ҵ��ps pause��� */
    if(OAL_TRUE == GET_HAL_BTCOEX_SW_PREEMPT_PS_PAUSE(pst_hal_device))
    {
        return;
    }

    pst_mac_device = mac_res_get_dev(pst_hal_device->uc_mac_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
       OAM_ERROR_LOG1(0, OAM_SF_COEX, "{dmac_btcoex_ps_powerdown_recover_handle: mac device is null ptr. device id:%d}", pst_hal_device->uc_mac_device_id);
       return;
    }

    /* ��ȡ��ǰps״̬ */
    hal_btcoex_get_bt_acl_status(pst_hal_device, &en_bt_acl_status);

    /* ����ҵ�񶼿��ܻ��ѵ͹��ģ�������ɨ�裬���Բ����� */
    if(OAL_TRUE == en_bt_acl_status)
    {
        /* ps״̬1�жϱ����Σ�û�м�ʱ������������ˢһ��Ҳ��ϵ���󣬻��ڵ͹����£�����Ҫ��ps֡����ʱ��Ҫ���û�������������Ҫ��ps֡ */
        dmac_btcoex_ps_start_handle(pst_hal_device, pst_mac_device);

        /* ʵʱ����acl en�Ĵ��� */
        oal_atomic_inc(&(pst_hal_device->st_btcoex_sw_preempt.ul_acl_en_cnt));
    }
    else
    {
        dmac_btcoex_ps_stop_handle(pst_hal_device);

        /* ʵʱ�ָ�����Ϊ0 */
        oal_atomic_set(&pst_hal_device->st_btcoex_sw_preempt.ul_acl_en_cnt, 0);
    }

    /* ˢ��acl״̬ */
    pst_hal_device->st_btcoex_sw_preempt.en_last_acl_status = en_bt_acl_status;

    /* ��ֹ������һֱ�ش�, ��ʱ��Ϊps=1״̬��wifi��Ҫ��˯�߻���һ������ʱ���������ƣ���ֹwifiҵ��ͨ */
    if(pst_hal_device->st_btcoex_sw_preempt.ul_acl_en_cnt > BTCOEX_POW_SAVE_CNT)
    {
        /* ǿ�ƻָ�������״̬Ϊforbit���͹��Ļ��Ѳ�������ֱ��soc�ж��������£����¼��� */
        GET_HAL_BTCOEX_SW_PREEMPT_SUBTYPE(pst_hal_device) = HAL_BTCOEX_SW_POWSAVE_SUB_PSM_FORBIT;
    }
}


OAL_STATIC oal_uint32 dmac_btcoex_ps_status_handler(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru                     *pst_event;
    hal_chip_stru                      *pst_hal_chip;
    hal_device_stru                    *pst_hal_device;
    hal_to_dmac_device_stru            *pst_h2d_device;
    mac_device_stru                    *pst_mac_device;
    oal_bool_enum_uint8                 en_bt_acl_status;
    oal_uint32                          ul_ps_enqueue_time;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_ps_status_handler::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event = frw_get_event_stru(pst_event_mem);

    /* ��ȡchipָ�� */
    pst_hal_chip = HAL_GET_CHIP_POINTER(pst_event->st_event_hdr.uc_chip_id);

    /* ��ʱֻ�Ǵ�����·��STA ��·TBD */
    pst_hal_device = &(pst_hal_chip->ast_device[HAL_DEVICE_ID_MASTER]);
    pst_h2d_device = HAL_DEV_GET_HAL2DMC_DEV(pst_hal_device);

    /* ��¼�¼����ʱ�� */
    ul_ps_enqueue_time = pst_h2d_device->st_btcoex_sw_preempt.ul_ps_cur_time;
    pst_h2d_device->st_btcoex_sw_preempt.ul_ps_cur_time = glbcnt_read_low32();
    if(pst_h2d_device->st_btcoex_sw_preempt.ul_ps_cur_time - ul_ps_enqueue_time > 20)
    {
        /* �ж����°벿ִ��ʱ������20 * 31.25usʱ������ά�� */
        OAM_WARNING_LOG1(0, OAM_SF_COEX, "{dmac_btcoex_ps_status_handler::ps start to end time beyond cnt[%d].}",
            pst_h2d_device->st_btcoex_sw_preempt.ul_ps_cur_time - ul_ps_enqueue_time);
    }

    /* �¼�����ʱ���ϰ벿����ʱ�Ѿ���Ϊ��0������Ҫ�ӻ�ȥ���ָ���1 */
    oal_atomic_inc(&(pst_h2d_device->st_btcoex_sw_preempt.ul_ps_event_num));

    pst_mac_device = mac_res_get_dev(pst_h2d_device->uc_mac_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
       OAM_ERROR_LOG1(0, OAM_SF_COEX, "{dmac_btcoex_ps_status_handler: mac device is null ptr. device id:%d}", pst_h2d_device->uc_mac_device_id);
       return OAL_ERR_CODE_PTR_NULL;
    }

    /* 1.���psδʹ�� */
    if (OAL_FALSE == HAL_BTCOEX_CHECK_SW_PREEMPT_ON(pst_h2d_device))
    {
        /* δʹ�ܣ�ֱ�ӷ��� */
        return OAL_SUCC;
    }

    /* 2.wifi����idle״̬ */
    if (HAL_DEVICE_IDLE_STATE == GET_HAL_DEVICE_STATE(pst_h2d_device))
    {
        return OAL_SUCC;
    }

    /* 3.ҵ��ps stop��� */
    if(OAL_TRUE == GET_HAL_BTCOEX_SW_PREEMPT_PS_STOP(pst_h2d_device))
    {
        return OAL_SUCC;
    }

    /* 4.ҵ��ps pause��� */
    if(OAL_TRUE == GET_HAL_BTCOEX_SW_PREEMPT_PS_PAUSE(pst_h2d_device))
    {
        OAM_WARNING_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_ps_status_handler::ps need to pause.}");
        return OAL_SUCC;
    }

    /* ��ǰ�°벿�¼�����ȡ��ǰps״̬ */
    hal_btcoex_get_bt_acl_status(pst_h2d_device, &en_bt_acl_status);

    /* ��Ϊ�͹����������������ͬ���°벿��ִ�У�������֤ps״̬��������ǽ���ִ�У�ֱ��return, wifi�°벿���ȵ������޸�info��ӡ */
    if(en_bt_acl_status == pst_h2d_device->st_btcoex_sw_preempt.en_last_acl_status)
    {
        OAM_INFO_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_ps_status_handler::en_bt_acl_status is the same.}");
        return OAL_SUCC;
    }

    if (en_bt_acl_status)
    {
        /* preempt������Ϊ�����ʽ */
        GET_HAL_BTCOEX_SW_PREEMPT_TYPE(pst_h2d_device) = HAL_BTCOEX_SW_POWSAVE_WORK;

        /* ʵʱ����acl en�Ĵ��� */
        oal_atomic_inc(&(pst_h2d_device->st_btcoex_sw_preempt.ul_acl_en_cnt));

        /* С��3���ſ�����·������DLINK816·������null֧�ֲ��ã��ۺ�����Ӧ�Ϻã��ظ�CTS���������� */
        if(OAL_FALSE == HAL_BTCOEX_CHECK_SW_PREEMPT_REPLY_CTS_ON(pst_h2d_device))
        {
            /* ��ֹӲ����cts */
            hal_disable_machw_cts_trans(pst_h2d_device);
        }

#ifdef _PRE_WLAN_1103_PILOT
        if(OAL_TRUE == HAL_BTCOEX_CHECK_SW_PREEMPT_RSP_FRAME_PS_ON(pst_h2d_device))
        {
            /* ��Ӧ֡ps��λ, Ϊ���������Ч�ʣ�ֱ��hal device���ã����ñ����û�����lut */
            hal_tx_enable_resp_ps_bit_ctrl_all(pst_h2d_device);
            //dmac_btcoex_set_resp_bit_ctrl(pst_h2d_device, OAL_TRUE);
        }
#endif

        /* �ǹ���״̬����ֱ������,����Ҫ����,����ps��ʱ������ps״̬���� */
        if (GET_HAL_DEVICE_STATE(pst_h2d_device) == HAL_DEVICE_IDLE_STATE)
        {
            GET_HAL_BTCOEX_SW_PREEMPT_SUBTYPE(pst_h2d_device) = HAL_BTCOEX_SW_POWSAVE_SUB_IDLE;
        }
        /* ��scan״̬ʱ�� */
        else if(GET_HAL_DEVICE_STATE(pst_h2d_device) == HAL_DEVICE_SCAN_STATE)
        {
            /* ��ɨ��ִ�й����У�����Ҫ����ɨ��ʱ�Զ��Ѿ����ڽ���״̬����tx pause״̬��ɨ�����ʱ��Ҫ�����ǲ�����ps���ָ����͹�����200ms��
             ��ʹɨ����ps���ָ���Ҳ���ü���ps����0�϶��ڵ͹���ǰ�������߻ָ�֮�󣬲��ᱻ�͹��Ĵ��
            */
            GET_HAL_BTCOEX_SW_PREEMPT_SUBTYPE(pst_h2d_device) = HAL_BTCOEX_SW_POWSAVE_SUB_SCAN;
        }
        else
        {
            /* staģʽ����һ��ʼ��û�����ϣ����ڷ�up״̬������������up״̬��ps=0ʱ��ָ�����Ч��ֱ���жϲ������� */

            /* ������ڵ͹���״̬(��Ҫ��work״̬�µ�awake��״̬���յ�ps�жϣ���Ȼ�Զ���˯��״̬)��wifi����Ҫ���⴦��ִ��pause ����Ҫ��ps֡��
            �ȵ͹����Լ�������ps״̬ */
            /* sta vap���з���pause����֪ͨ�Զ˻���������ͣ�������ݣ� ����idle״̬���շ�һ֡Ҳû��ϵ */

            /* ��Ĭ��Ϊactive״̬���ܱ�֤�͹��Ķ���ʱ���ж�ps=1Ҳ�ܻ��� */
            GET_HAL_BTCOEX_SW_PREEMPT_SUBTYPE(pst_h2d_device) = HAL_BTCOEX_SW_POWSAVE_SUB_ACTIVE;

            /* ps��Ч��priority����֤��ʱwifi�յ��ۺ�֡�ܽϸ߸��ʻ�BA�����one pkt֡���͵ĳɹ���(���쾺�����ŵ�)*/
            //hal_set_btcoex_priority_period(pst_h2d_device, BTCOEX_PRIO_TIMEOUT_ALWAYS_ON);

            /* ��vapʱ��ĳһ��vap�ڹ��������У���ʱû��pause�� ����resumeʱ��ͳ������Ѿ���up״̬ */
            pst_mac_device->st_fcs_mgr.en_fcs_service_type = HAL_FCS_SERVICE_TYPE_BTCOEX;

            dmac_m2s_switch_device_begin(pst_mac_device, pst_h2d_device);

            /* one pkt֡���ͽ�������priority */
            //hal_set_btcoex_priority_period(pst_h2d_device, BTCOEX_PRIO_TIMEOUT_ALWAYS_OFF);
        }

        /* ״̬��Ǩ,������ʱ�����ps��������ֹɨ����״̬֮�󣬳���wifi������ */
        if(OAL_TRUE == pst_h2d_device->st_btcoex_powersave_timer.en_is_registerd)
        {
            FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_h2d_device->st_btcoex_powersave_timer));
        }

        /* ps��������ʱ����Ҫ���ݵ�ǰ״̬��ˢ�³�ʱ��ʱ��ʱ�� */
        dmac_btcoex_ps_timeout_update_time(pst_h2d_device);

        FRW_TIMER_CREATE_TIMER(&(pst_h2d_device->st_btcoex_powersave_timer),
                               dmac_btcoex_pow_save_callback,
                               pst_h2d_device->st_btcoex_sw_preempt.us_timeout_ms,
                               (oal_void *)pst_h2d_device,
                               OAL_FALSE,
                               OAM_MODULE_ID_DMAC,
                               pst_h2d_device->ul_core_id);
    }
    else
    {
        if(OAL_TRUE == pst_h2d_device->st_btcoex_powersave_timer.en_is_registerd)
        {
            FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_h2d_device->st_btcoex_powersave_timer));
        }

        /* �ǹ���״̬����ֱ������,����Ҫ���� */
        if (GET_HAL_DEVICE_STATE(pst_h2d_device) == HAL_DEVICE_IDLE_STATE)
        {

        }
        /* ��scan״̬ʱ��ɨ�費����͹��ģ���ʱ���õ��ĵ͹��ģ����ɨ����ʱ */
        else if(GET_HAL_DEVICE_STATE(pst_h2d_device) == HAL_DEVICE_SCAN_STATE)
        {
            switch(GET_HAL_BTCOEX_SW_PREEMPT_TYPE(pst_h2d_device))
            {
                case HAL_BTCOEX_SW_POWSAVE_SCAN_BEGIN:
                    /* ���ɨ��һ��ʼ��ps��ϣ���ʱps���ָ�����ɨ�� */
                    dmac_scan_begin(pst_mac_device, pst_h2d_device);
                    OAM_INFO_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_ps_status_handler::dmac_scan_begin start.}");
                    break;

                case HAL_BTCOEX_SW_POWSAVE_SCAN_WAIT:
                    /* ���ɨ����ʱ��home channel������������ϣ���ʱps���ָ�������home channel���� */
                    dmac_scan_switch_home_channel_work(pst_mac_device, pst_h2d_device);
                    OAM_WARNING_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_ps_status_handler::dmac_scan_switch_home_channel_work start.}");
                    break;

                case HAL_BTCOEX_SW_POWSAVE_SCAN_END:
                    /* ���ɨ���������ϣ���ʱps���ָ�����ɨ�� */
                    OAM_WARNING_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_ps_status_handler::dmac_scan_handle_switch_channel_back start.}");
                    //ͳһ���ýӿ�dmac_scan_prepare_endʵ��
                    //dmac_scan_handle_switch_channel_back(pst_mac_device, pst_h2d_device, &(pst_h2d_device->st_hal_scan_params));
                    //hal_device_handle_event(pst_h2d_device, HAL_DEVICE_EVENT_SCAN_END, 0, OAL_PTR_NULL);
                    dmac_scan_prepare_end(pst_mac_device, pst_h2d_device);

                    break;

                case HAL_BTCOEX_SW_POWSAVE_SCAN_ABORT:
                    /* abort״̬һ���ǣ���save״̬ʱ��scan baort��������ps=0Ҫ�ָ�ps=1�����ã���Ϊscan abort��ǰresume�ˣ��˴�����Ҫ����  */
                    OAM_WARNING_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_ps_status_handler::scan is already abort and resume.}");

                    /* abort�Ѿ�ǿ����Ϊ�ָ����˴β�������vap�Ѿ�vap�˲���Ҫ������ */
                    /* �ָ����ͺͽ��� */
                    //dmac_vap_resume_tx_by_chl(pst_mac_device, pst_h2d_device, &(pst_h2d_device->st_wifi_channel_status));
                    break;

                case HAL_BTCOEX_SW_POWSAVE_IDLE:
                    /* ������״̬1�¼�û�м�ʱ�����°벿��������״̬0����ʱ����ɨ�裬�ָ�����ɨ���������� */
                    OAM_WARNING_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_ps_status_handler::HAL_BTCOEX_SW_POWSAVE_IDLE.}");
                    break;

                case HAL_BTCOEX_SW_POWSAVE_WORK:
                    /* ���ɨ���ڼ�����1������0����ʱ�ָ�����ɨ���Լ��ָ�,ps=0�������� */
                    OAM_INFO_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_ps_status_handler::scan is running.}");
                    break;

                case HAL_BTCOEX_SW_POWSAVE_TIMEOUT:
                    /* time is up�� ���ߵ͹����Ѿ���ǰ�ָ����������� */
                    OAM_INFO_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_ps_status_handler:: scan state timeout already resume.}");
                    break;

                default:
                    OAM_WARNING_LOG1(0, OAM_SF_COEX, "{dmac_btcoex_ps_status_handler::en_sw_preempt_type[%d] error.}",
                        GET_HAL_BTCOEX_SW_PREEMPT_TYPE(pst_h2d_device));
            }
        }
        else
        {
            switch(GET_HAL_BTCOEX_SW_PREEMPT_TYPE(pst_h2d_device))
            {
                case HAL_BTCOEX_SW_POWSAVE_PSM_END:
                    /* ��ִ���˵͹��Ļָ�����Ҫ������ */
                    OAM_WARNING_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_ps_status_handler:: powerdown has already resume.}");
                    break;

                case HAL_BTCOEX_SW_POWSAVE_IDLE:
                    /* �����ǵ͹���ps=1�¼��ſ�ʼִ�У���ʱ��ȡ�Ĵ���״̬=0����ǰ��0״̬���Ͱ���0�������ɣ�������������0���¼����� */
                    OAM_WARNING_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_ps_status_handler:: current is normal!.}");
                    break;

                case HAL_BTCOEX_SW_POWSAVE_SCAN_ABORT:
                    /* ǿ��scan abort�ָ��ˣ��˴�����Ҫ�ָ� */
                    OAM_WARNING_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_ps_status_handler:: work state scan abort already resume.}");
                    break;

                case HAL_BTCOEX_SW_POWSAVE_WORK:
                    /* �ָ����ͺͽ��� */
                    if(HAL_BTCOEX_SW_POWSAVE_SUB_IDLE == GET_HAL_BTCOEX_SW_PREEMPT_SUBTYPE(pst_h2d_device))
                    {
                        GET_HAL_BTCOEX_SW_PREEMPT_SUBTYPE(pst_h2d_device) = HAL_BTCOEX_SW_POWSAVE_SUB_ACTIVE;
                        OAM_WARNING_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_ps_status_handler:: work HAL_BTCOEX_SW_POWSAVE_SUB_IDLE.}");
                    }
                    else if(HAL_BTCOEX_SW_POWSAVE_SUB_SCAN == GET_HAL_BTCOEX_SW_PREEMPT_SUBTYPE(pst_h2d_device))
                    {
                        GET_HAL_BTCOEX_SW_PREEMPT_SUBTYPE(pst_h2d_device) = HAL_BTCOEX_SW_POWSAVE_SUB_ACTIVE;
                        OAM_WARNING_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_ps_status_handler:: work HAL_BTCOEX_SW_POWSAVE_SUB_SCAN.}");
                    }
                    else if(HAL_BTCOEX_SW_POWSAVE_SUB_PSM_FORBIT == GET_HAL_BTCOEX_SW_PREEMPT_SUBTYPE(pst_h2d_device))
                    {
                        /* �����ǵ͹��Ķ���֮����Ҫsoc�ж�ps=0������ */
                        GET_HAL_BTCOEX_SW_PREEMPT_SUBTYPE(pst_h2d_device) = HAL_BTCOEX_SW_POWSAVE_SUB_ACTIVE;
                        OAM_WARNING_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_ps_status_handler:: work HAL_BTCOEX_SW_POWSAVE_SUB_PSM_FORBIT.}");
                    }
                    else
                    {
                        dmac_m2s_switch_device_end(pst_h2d_device);
                    }
                    break;

                case HAL_BTCOEX_SW_POWSAVE_TIMEOUT:
                    /* time is up�� ���ߵ͹����Ѿ���ǰ�ָ����������� */
                    OAM_INFO_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_ps_status_handler:: work state timeout already resume.}");
                    break;

                default:
                    OAM_WARNING_LOG1(0, OAM_SF_COEX, "{dmac_btcoex_ps_status_handler::en_sw_preempt_type[%d] error.}",
                        GET_HAL_BTCOEX_SW_PREEMPT_TYPE(pst_h2d_device));
            }
        }

#ifdef _PRE_WLAN_1103_PILOT
        if(OAL_TRUE == HAL_BTCOEX_CHECK_SW_PREEMPT_RSP_FRAME_PS_ON(pst_h2d_device))
        {
            /* �ָ���Ӧ֡ps��λ */
            hal_tx_disable_resp_ps_bit_ctrl_all(pst_h2d_device);
            //dmac_btcoex_set_resp_bit_ctrl(pst_h2d_device, OAL_FALSE);
        }
#endif

        if(OAL_FALSE == HAL_BTCOEX_CHECK_SW_PREEMPT_REPLY_CTS_ON(pst_h2d_device))
        {
            /* �ָ�Ӳ����cts */
            hal_enable_machw_cts_trans(pst_h2d_device);
        }

        /* ʵʱ�ָ�����Ϊ0 */
        oal_atomic_set(&pst_h2d_device->st_btcoex_sw_preempt.ul_acl_en_cnt, 0);

        /* preempt������ΪIDLE��ʽ */
        GET_HAL_BTCOEX_SW_PREEMPT_TYPE(pst_h2d_device) = HAL_BTCOEX_SW_POWSAVE_IDLE;
    }

    /* �����¼��һ�ε�acl״̬ */
    pst_h2d_device->st_btcoex_sw_preempt.en_last_acl_status = en_bt_acl_status;

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 dmac_btcoex_ldac_status_handler(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru                     *pst_event;
    hal_chip_stru                      *pst_hal_chip;
    hal_device_stru                    *pst_hal_device;
    hal_to_dmac_device_stru            *pst_h2d_device;
    bt_status_stru                     *pst_bt_status;
    ble_status_stru                    *pst_ble_status;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_ldac_status_handler::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event = frw_get_event_stru(pst_event_mem);

    /* ��ȡchipָ�� */
    pst_hal_chip = HAL_GET_CHIP_POINTER(pst_event->st_event_hdr.uc_chip_id);

    /* ��ʱֻ�Ǵ�����·��STA ��·TBD */
    pst_hal_device = &(pst_hal_chip->ast_device[HAL_DEVICE_ID_MASTER]);
    pst_h2d_device = HAL_DEV_GET_HAL2DMC_DEV(pst_hal_device);

    pst_bt_status = &(pst_hal_chip->st_hal_chip_base.st_btcoex_btble_status.un_bt_status.st_bt_status);
    pst_ble_status = &(pst_hal_chip->st_hal_chip_base.st_btcoex_btble_status.un_ble_status.st_ble_status);

#if 0 //�ŵ��ϰ벿�������°벿��ʱֻ��ά�⣬�����ſ������������TBD.  �°벿����̫�����ldac������ǰ��
    /* ldacҵ�������null�ָ����ȼ����� */
    if (OAL_FALSE == pst_ble_status->bit_bt_ldac)
    {
        /* �ָ����ȼ����� */
        pst_h2d_device->st_btcoex_sw_preempt.en_coex_pri_forbit = OAL_FALSE;
    }
    else
    {
        /* ����Ҳʹ�� */
        if (OAL_TRUE == pst_bt_status->bit_bt_a2dp)
        {
            /* �ر����ȼ����� */
            pst_h2d_device->st_btcoex_sw_preempt.en_coex_pri_forbit = OAL_TRUE;
        }
    }
#endif

    OAM_WARNING_LOG3(0, OAM_SF_COEX, "{dmac_btcoex_ldac_status_handler::ldac state[%d],a2dp[%d],coex_pri_forbit[%d].}",
        pst_ble_status->bit_bt_ldac, pst_bt_status->bit_bt_a2dp, pst_h2d_device->st_btcoex_sw_preempt.en_coex_pri_forbit);

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 dmac_btcoex_delba_event_process(hal_btcoex_btble_status_stru *pst_btble_status, mac_vap_stru *pst_mac_vap)
{
    dmac_vap_btcoex_rx_statistics_stru *pst_dmac_vap_btcoex_rx_statistics;
    dmac_user_btcoex_delba_stru        *pst_dmac_user_btcoex_delba;
    dmac_vap_stru                      *pst_dmac_vap;
    dmac_user_stru                     *pst_dmac_user;
    oal_bool_enum_uint8                 en_need_delba = OAL_FALSE;

    /* ���������ɾ�ۺ� ֱ�ӷ��� */
    if(OAL_FALSE == HAL_BTCOEX_CHECK_SW_PREEMPT_DELBA_ON((DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap))))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "dmac_btcoex_delba_event_process::sw preempt not support delba.");
        return OAL_SUCC;
    }

    pst_dmac_vap  = MAC_GET_DMAC_VAP(pst_mac_vap);
    pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user(pst_mac_vap->us_assoc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dmac_user))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "dmac_btcoex_delba_event_process::pst_mac_user IS NULL.");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_dmac_vap_btcoex_rx_statistics = &(pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_rx_statistics);
    pst_dmac_user_btcoex_delba = &(pst_dmac_user->st_dmac_user_btcoex_stru.st_dmac_user_btcoex_delba);

    /* ���ֺ�������ʱ����ʽһ�� */
    /* ����bt���ֺ������򿪺͹رճ����£��ԾۺϽ��д��� */
    if (pst_btble_status->un_bt_status.st_bt_status.bit_bt_a2dp || pst_btble_status->un_ble_status.st_ble_status.bit_bt_transfer)
    {
        if (BTCOEX_RX_WINDOW_SIZE_INDEX_3 == pst_dmac_user_btcoex_delba->en_ba_size_real_index)
        {
            pst_dmac_user_btcoex_delba->en_ba_size_expect_index = BTCOEX_RX_WINDOW_SIZE_INDEX_2;
        }
        else
        {
            pst_dmac_user_btcoex_delba->en_ba_size_expect_index = pst_dmac_user_btcoex_delba->en_ba_size_real_index;
        }

        /* RX����ͳ�ƿ��� */
        pst_dmac_vap_btcoex_rx_statistics->en_rx_rate_statistics_flag = OAL_TRUE;
    }
    else
    {
        /* ֻ�е绰, ���ֺ�������û�е�����Ž��лָ���64�ľۺ� */
        if(!pst_btble_status->un_bt_status.st_bt_status.bit_bt_sco)
        {
            pst_dmac_user_btcoex_delba->en_ba_size_expect_index = BTCOEX_RX_WINDOW_SIZE_INDEX_3;
            pst_dmac_vap_btcoex_rx_statistics->en_rx_rate_statistics_flag = OAL_FALSE;
        }
        /* ���ֽ�����ʱ���е绰�ĳ����������ﲻ����BAɾ�������ɵ绰�����̿��� */
        else
        {
            return OAL_SUCC;
        }
    }

#if 0
    /* ���Ż����£���Ҫ����ɾ����32���ȶ������Դ�����ʱ���� */
    if(pst_btble_status->un_ble_status.st_ble_status.bit_bt_transfer)
    {
        en_need_delba = OAL_TRUE;
    }
#endif

    dmac_btcoex_update_ba_size(pst_mac_vap, pst_dmac_user_btcoex_delba, pst_btble_status);

    OAM_WARNING_LOG4(0, OAM_SF_COEX, "{dmac_btcoex_delba_event_process:: a2dp status changed:%d, transfer status changed:%d, en_need_delba:%d,ba_size:%d.}",
            pst_btble_status->un_bt_status.st_bt_status.bit_bt_a2dp, pst_btble_status->un_ble_status.st_ble_status.bit_bt_transfer,
            en_need_delba, pst_dmac_user_btcoex_delba->uc_ba_size);

    /* Ĭ�϶�������ɾ��BA, ��������ͳ���߼���������ɾ��BA���� */
    dmac_btcoex_delba_trigger(pst_mac_vap, en_need_delba, pst_dmac_user_btcoex_delba);

    pst_dmac_vap_btcoex_rx_statistics->en_rx_rate_statistics_timeout = OAL_FALSE;

    if(OAL_TRUE == pst_dmac_vap_btcoex_rx_statistics->bt_coex_statistics_timer.en_is_registerd)
    {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_dmac_vap_btcoex_rx_statistics->bt_coex_statistics_timer));
    }
    FRW_TIMER_CREATE_TIMER(&(pst_dmac_vap_btcoex_rx_statistics->bt_coex_statistics_timer),
                               dmac_btcoex_rx_rate_statistics_flag_callback,
                               BTCOEX_RX_STATISTICS_TIME,
                               (void *)pst_mac_vap,
                               OAL_TRUE,
                               OAM_MODULE_ID_DMAC,
                               pst_mac_vap->ul_core_id);

    return OAL_SUCC;
}


oal_void dmac_btcoex_set_wlan_priority(mac_vap_stru *pst_mac_vap, oal_bool_enum_uint8 en_value, oal_uint8 uc_timeout_ms)
{
    dmac_vap_btcoex_occupied_stru *pst_dmac_vap_btcoex_occupied;
    hal_to_dmac_device_stru       *pst_hal_device;
    dmac_vap_stru                 *pst_dmac_vap;
    oal_uint32                     ul_now_ms;

    pst_dmac_vap = MAC_GET_DMAC_VAP(pst_mac_vap);
    pst_dmac_vap_btcoex_occupied = &(pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_occupied);

    pst_hal_device  = DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap);
    if (OAL_PTR_NULL == pst_hal_device)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_set_wlan_priority:: DMAC_VAP_GET_HAL_DEVICE null}");
        return;
    }

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)
    if(WLAN_BAND_5G == pst_dmac_vap->st_vap_base_info.st_channel.en_band)
    {
        /* 5G����Ҫ��priority���� */
        return;
    }
#endif

    /* ��Ҫ����prio */
    if (OAL_TRUE == en_value)
    {
        /* ��ǰû������prio���ҿ�������prio */
        if (OAL_FALSE == pst_dmac_vap_btcoex_occupied->uc_prio_occupied_state)
        {
            /* ������Ҫ����һ���������̫�� */
            ul_now_ms = (oal_uint32)OAL_TIME_GET_STAMP_MS();
            if (ul_now_ms - pst_dmac_vap_btcoex_occupied->ul_timestamp < BTCOEX_PRI_DURATION_TIME)
            {
                OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_set_wlan_priority:: priority disable only[%d]ms!}",
                    (ul_now_ms - pst_dmac_vap_btcoex_occupied->ul_timestamp));
            }

            pst_dmac_vap_btcoex_occupied->uc_prio_occupied_state = OAL_TRUE;
        }
        else
        {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_set_wlan_priority:: priority already setting!}");
        }

        if(OAL_TRUE == pst_dmac_vap_btcoex_occupied->bt_coex_priority_timer.en_is_registerd)
        {
            FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&pst_dmac_vap_btcoex_occupied->bt_coex_priority_timer);
        }

        /* ����priority��ʱ�� */
        FRW_TIMER_CREATE_TIMER(&pst_dmac_vap_btcoex_occupied->bt_coex_priority_timer,
                                   dmac_btcoex_wlan_priority_timeout_callback,
                                   uc_timeout_ms,
                                   (oal_void *)pst_dmac_vap,
                                   OAL_FALSE,
                                   OAM_MODULE_ID_DMAC,
                                   pst_mac_vap->ul_core_id);
    }
    /* ��Ҫ����prio */
    else
    {
        if (OAL_TRUE == pst_dmac_vap_btcoex_occupied->uc_prio_occupied_state)
        {
            pst_dmac_vap_btcoex_occupied->uc_prio_occupied_state = OAL_FALSE;

            pst_dmac_vap_btcoex_occupied->ul_timestamp = (oal_uint32)OAL_TIME_GET_STAMP_MS();
        }

        if(OAL_TRUE == pst_dmac_vap_btcoex_occupied->bt_coex_priority_timer.en_is_registerd)
        {
            FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&pst_dmac_vap_btcoex_occupied->bt_coex_priority_timer);
        }

        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_set_wlan_priority:: priority need to lowerdown!}");
    }

    /* ��������������ȼ� */
    hal_set_btcoex_hw_priority_en(pst_hal_device, ((en_value == OAL_TRUE) ? OAL_FALSE : OAL_TRUE));
}
#else

oal_void dmac_btcoex_set_wlan_priority(mac_vap_stru *pst_mac_vap, oal_bool_enum_uint8 en_value, oal_uint8 uc_timeout_ms)
{
    dmac_vap_btcoex_occupied_stru *pst_dmac_vap_btcoex_occupied;
    hal_to_dmac_device_stru       *pst_hal_device;
    dmac_vap_stru                 *pst_dmac_vap;
    oal_uint32                     ul_now_ms;
    oal_bool_enum_uint8            en_set_on = OAL_FALSE;

    pst_dmac_vap = MAC_GET_DMAC_VAP(pst_mac_vap);
    pst_dmac_vap_btcoex_occupied = &(pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_occupied);

    pst_hal_device  = DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap);
    if (OAL_PTR_NULL == pst_hal_device)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_set_wlan_priority:: DMAC_VAP_GET_HAL_DEVICE null}");
        return;
    }

    /* ��Ҫ����prio */
    if (OAL_TRUE == en_value)
    {
        /* ��ǰû������prio���ҿ�������prio */
        if (OAL_FALSE == pst_dmac_vap_btcoex_occupied->uc_prio_occupied_state)
        {

            /* ��Ҫ����һ������ʱ��������100ms */
            ul_now_ms = (oal_uint32)OAL_TIME_GET_STAMP_MS();
            if (ul_now_ms - pst_dmac_vap_btcoex_occupied->ul_timestamp > BTCOEX_PRI_DURATION_TIME)
            {
                /* ����priority��ʱ�� */
                FRW_TIMER_CREATE_TIMER(&pst_dmac_vap_btcoex_occupied->bt_coex_priority_timer,
                                           dmac_btcoex_wlan_priority_timeout_callback,
                                           uc_timeout_ms,
                                           (oal_void *)pst_dmac_vap,
                                           OAL_FALSE,
                                           OAM_MODULE_ID_DMAC,
                                           pst_mac_vap->ul_core_id);
                pst_dmac_vap_btcoex_occupied->uc_prio_occupied_state = OAL_TRUE;

                en_set_on = OAL_TRUE;
            }
            else
            {
            }
        }
        else
        {

        }
    }
    /* ��Ҫ����prio */
    else
    {
        if (OAL_TRUE == pst_dmac_vap_btcoex_occupied->uc_prio_occupied_state)
        {
            pst_dmac_vap_btcoex_occupied->uc_prio_occupied_state = OAL_FALSE;

            pst_dmac_vap_btcoex_occupied->ul_timestamp = (oal_uint32)OAL_TIME_GET_STAMP_MS();
        }
        en_set_on = OAL_TRUE;
    }

    if (OAL_TRUE == en_set_on)
    {
        /* uc_value == 1, ������ø����ȼ���uc_value == 0, ���ظ�Ӳ������ */
        hal_set_btcoex_hw_rx_priority_dis(pst_hal_device, ((en_value == OAL_TRUE) ? OAL_FALSE : OAL_TRUE));

        /* ��������������ȼ� */
        hal_set_btcoex_hw_priority_en(pst_hal_device, ((en_value == OAL_TRUE) ? OAL_FALSE : OAL_TRUE));

        /* ����������ȼ� */
        hal_set_btcoex_sw_priority_flag(pst_hal_device, en_value);
    }

}

#endif


OAL_STATIC oal_uint32 dmac_btcoex_page_scan_handler(frw_event_mem_stru *pst_event_mem)
{
    dmac_vap_btcoex_occupied_stru *pst_dmac_vap_btcoex_occupied;
    frw_event_stru                *pst_event;
    hal_chip_stru                 *pst_hal_chip;
    hal_to_dmac_device_stru       *pst_hal_device;
    dmac_vap_stru                 *pst_dmac_vap;
    oal_uint8                      uc_vap_idx;
    oal_uint8                      uc_valid_vap_num;
    oal_uint8                      auc_mac_vap_id[WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE];
    mac_vap_stru                  *pst_mac_vap[WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE] = {OAL_PTR_NULL};


    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_page_scan_handler::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_event = frw_get_event_stru(pst_event_mem);

    /* ��ȡchipָ�� */
    pst_hal_chip = HAL_GET_CHIP_POINTER(pst_event->st_event_hdr.uc_chip_id);

    /* ��ʱֻ�Ǵ�����·��STA ��·TBD */
    pst_hal_device = HAL_DEV_GET_HAL2DMC_DEV(&(pst_hal_chip->ast_device[HAL_DEVICE_ID_MASTER]));

    /* �ҵ�����Ҫ���vap���� */
    uc_valid_vap_num = dmac_btcoex_find_all_valid_sta_per_device(pst_hal_device, auc_mac_vap_id);

    /* valid��vap�豸������Ӧ���� 02ֻ����legacy sta   03Ҫ����gc sta */
    for (uc_vap_idx = 0; uc_vap_idx < uc_valid_vap_num; uc_vap_idx++)
    {
        pst_mac_vap[uc_vap_idx]  = (mac_vap_stru *)mac_res_get_mac_vap(auc_mac_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_mac_vap[uc_vap_idx])
        {
            OAM_ERROR_LOG1(0, OAM_SF_COEX, "{dmac_btcoex_page_scan_handler::pst_mac_vap[%d] IS NULL.}", auc_mac_vap_id[uc_vap_idx]);
            return OAL_ERR_CODE_PTR_NULL;
        }

        /* �ҵ�valid sta������beacon miss�߼����� */
        pst_dmac_vap = MAC_GET_DMAC_VAP(pst_mac_vap[uc_vap_idx]);
        pst_dmac_vap_btcoex_occupied = &(pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_occupied);

        hal_btcoex_update_ap_beacon_count(pst_hal_device, &(pst_dmac_vap_btcoex_occupied->ul_ap_beacon_count));
        pst_dmac_vap_btcoex_occupied->uc_beacon_miss_cnt = 0;
    }

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)
    /* �����ʱ�������ˣ���Ҫ�����µ�ʱ����ˢ�� */
    if(OAL_TRUE == pst_hal_device->st_btcoex_powersave_timer.en_is_registerd)
    {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_hal_device->st_btcoex_powersave_timer));

        /* ps��������ʱ����Ҫ���ݵ�ǰ״̬��ˢ�³�ʱ��ʱ��ʱ��,��Ϊ��Ӧҵ�����ʱ����������ps�жϣ���Ҫ�˴���ˢʱ�� */
        dmac_btcoex_ps_timeout_update_time(pst_hal_device);

        FRW_TIMER_CREATE_TIMER(&(pst_hal_device->st_btcoex_powersave_timer),
                           dmac_btcoex_pow_save_callback,
                           pst_hal_device->st_btcoex_sw_preempt.us_timeout_ms,
                           (oal_void *)pst_hal_device,
                           OAL_FALSE,
                           OAM_MODULE_ID_DMAC,
                           pst_hal_device->ul_core_id);
    }
#endif

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 dmac_btcoex_inquiry_status_handler(frw_event_mem_stru *pst_event_mem)
{
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)
    frw_event_stru                *pst_event;
    hal_chip_stru                 *pst_hal_chip;
    hal_to_dmac_device_stru       *pst_hal_device;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_inquiry_status_handler::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_event = frw_get_event_stru(pst_event_mem);

    /* ��ȡchipָ�� */
    pst_hal_chip = HAL_GET_CHIP_POINTER(pst_event->st_event_hdr.uc_chip_id);

    /* ��ʱֻ�Ǵ�����·��STA ��·TBD */
    pst_hal_device = HAL_DEV_GET_HAL2DMC_DEV(&(pst_hal_chip->ast_device[HAL_DEVICE_ID_MASTER]));

    /* �����ʱ�������ˣ���Ҫ�����µ�ʱ����ˢ�� */
    if(OAL_TRUE == pst_hal_device->st_btcoex_powersave_timer.en_is_registerd)
    {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_hal_device->st_btcoex_powersave_timer));

        /* ps��������ʱ����Ҫ���ݵ�ǰ״̬��ˢ�³�ʱ��ʱ��ʱ��,��Ϊ��Ӧҵ�����ʱ����������ps�жϣ���Ҫ�˴���ˢʱ�� */
        dmac_btcoex_ps_timeout_update_time(pst_hal_device);

        FRW_TIMER_CREATE_TIMER(&(pst_hal_device->st_btcoex_powersave_timer),
                           dmac_btcoex_pow_save_callback,
                           pst_hal_device->st_btcoex_sw_preempt.us_timeout_ms,
                           (oal_void *)pst_hal_device,
                           OAL_FALSE,
                           OAM_MODULE_ID_DMAC,
                           pst_hal_device->ul_core_id);
    }
#endif

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 dmac_btcoex_a2dp_status_handler(frw_event_mem_stru *pst_event_mem)
{
    hal_btcoex_btble_status_stru       *pst_btble_status;
    frw_event_stru                     *pst_event;
    hal_chip_stru                      *pst_hal_chip;
    hal_to_dmac_device_stru            *pst_hal_device;
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV)
    dmac_vap_btcoex_rx_statistics_stru *pst_dmac_vap_btcoex_rx_statistics;
    dmac_user_btcoex_delba_stru        *pst_dmac_user_btcoex_delba;
    dmac_vap_stru                      *pst_dmac_vap;
    dmac_user_stru                     *pst_dmac_user;
    oal_bool_enum_uint8                 en_need_delba = OAL_FALSE;
#endif
    oal_uint8                           uc_vap_idx;
    oal_uint8                           uc_valid_vap_num;
    oal_uint8                           auc_mac_vap_id[WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE];
    mac_vap_stru                       *pst_mac_vap[WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE] = {OAL_PTR_NULL};

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_a2dp_status_handler::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)
#ifdef _PRE_WLAN_1103_PILOT

#else
    
    hal_btcoex_open_5g_upc();
#endif
#endif

    pst_event = frw_get_event_stru(pst_event_mem);

    /* ��ȡchipָ�� */
    pst_hal_chip = HAL_GET_CHIP_POINTER(pst_event->st_event_hdr.uc_chip_id);

    /* ��ʱֻ�Ǵ�����·��STA ��·TBD */
    pst_hal_device = HAL_DEV_GET_HAL2DMC_DEV(&(pst_hal_chip->ast_device[HAL_DEVICE_ID_MASTER]));

    pst_btble_status = &(pst_hal_chip->st_hal_chip_base.st_btcoex_btble_status);

    /* �ҵ�����Ҫ���vap���� */
    uc_valid_vap_num = dmac_btcoex_find_all_valid_sta_per_device(pst_hal_device, auc_mac_vap_id);

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)
    /* �����ʱ�������ˣ���Ҫ�����µ�ʱ����ˢ�� */
    if(OAL_TRUE == pst_hal_device->st_btcoex_powersave_timer.en_is_registerd)
    {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_hal_device->st_btcoex_powersave_timer));

        /* ps��������ʱ����Ҫ���ݵ�ǰ״̬��ˢ�³�ʱ��ʱ��ʱ��,��Ϊ��Ӧҵ�����ʱ����������ps�жϣ���Ҫ�˴���ˢʱ�� */
        dmac_btcoex_ps_timeout_update_time(pst_hal_device);

        FRW_TIMER_CREATE_TIMER(&(pst_hal_device->st_btcoex_powersave_timer),
                           dmac_btcoex_pow_save_callback,
                           pst_hal_device->st_btcoex_sw_preempt.us_timeout_ms,
                           (oal_void *)pst_hal_device,
                           OAL_FALSE,
                           OAM_MODULE_ID_DMAC,
                           pst_hal_device->ul_core_id);
    }
#endif

    /* valid��vap�豸������Ӧ���� */
    for (uc_vap_idx = 0; uc_vap_idx < uc_valid_vap_num; uc_vap_idx++)
    {
        pst_mac_vap[uc_vap_idx]  = (mac_vap_stru *)mac_res_get_mac_vap(auc_mac_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_mac_vap[uc_vap_idx])
        {
            OAM_ERROR_LOG1(0, OAM_SF_COEX, "{dmac_btcoex_a2dp_status_handler::pst_mac_vap[%d] IS NULL.}", auc_mac_vap_id[uc_vap_idx]);
            return OAL_ERR_CODE_PTR_NULL;
        }

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)
        /* �ҵ�valid sta������ɾ��BA�߼����� */
        dmac_btcoex_delba_event_process(pst_btble_status, pst_mac_vap[uc_vap_idx]);
#else

        pst_dmac_vap  = MAC_GET_DMAC_VAP(pst_mac_vap[uc_vap_idx]);
        pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user((pst_mac_vap[uc_vap_idx])->us_assoc_vap_id);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dmac_user))
        {
            return OAL_ERR_CODE_PTR_NULL;
        }

        pst_dmac_vap_btcoex_rx_statistics = &(pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_rx_statistics);
        pst_dmac_user_btcoex_delba = &(pst_dmac_user->st_dmac_user_btcoex_stru.st_dmac_user_btcoex_delba);

        /* ����bt�򿪺͹رճ����£��ԾۺϽ��д��� */
        if (pst_btble_status->un_bt_status.st_bt_status.bit_bt_a2dp)
        {
            if (BTCOEX_RX_WINDOW_SIZE_INDEX_3 == pst_dmac_user_btcoex_delba->en_ba_size_real_index)
            {
                {
                    pst_dmac_user_btcoex_delba->en_ba_size_expect_index = BTCOEX_RX_WINDOW_SIZE_INDEX_1;
                }
            }
            else
            {
                pst_dmac_user_btcoex_delba->en_ba_size_expect_index = pst_dmac_user_btcoex_delba->en_ba_size_real_index;
            }

            /* RX����ͳ�ƿ��� */
            pst_dmac_vap_btcoex_rx_statistics->en_rx_rate_statistics_flag = OAL_TRUE;
        }
        else
        {
            /* ֻ�е绰���ֶ�û�е�����Ž��лָ���64�ľۺ� */
            if(!pst_btble_status->un_bt_status.st_bt_status.bit_bt_sco)
            {
                pst_dmac_user_btcoex_delba->en_ba_size_expect_index = BTCOEX_RX_WINDOW_SIZE_INDEX_3;
                pst_dmac_vap_btcoex_rx_statistics->en_rx_rate_statistics_flag = OAL_FALSE;
            }
            /* ���ֽ�����ʱ���е绰�ĳ����������ﲻ����BAɾ�������ɵ绰�����̿��� */
            else
            {
                return OAL_SUCC;
            }
        }

        dmac_btcoex_update_ba_size(pst_mac_vap[uc_vap_idx], pst_dmac_user_btcoex_delba, pst_btble_status);

        OAM_WARNING_LOG3(0, OAM_SF_COEX, "{dmac_btcoex_a2dp_status_handler:: ba status changed:%d,en_need_delba:%d,ba_size:%d.}",
                pst_btble_status->un_bt_status.st_bt_status.bit_bt_a2dp,
                en_need_delba, pst_dmac_user_btcoex_delba->uc_ba_size);

        /* Ĭ�϶�������ɾ��BA, ��������ͳ���߼���������ɾ��BA���� */
        dmac_btcoex_delba_trigger(pst_mac_vap[uc_vap_idx], en_need_delba, pst_dmac_user_btcoex_delba);

        pst_dmac_vap_btcoex_rx_statistics->en_rx_rate_statistics_timeout = OAL_FALSE;

        if(OAL_TRUE == pst_dmac_vap_btcoex_rx_statistics->bt_coex_statistics_timer.en_is_registerd)
        {
            FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_dmac_vap_btcoex_rx_statistics->bt_coex_statistics_timer));
        }
        FRW_TIMER_CREATE_TIMER(&(pst_dmac_vap_btcoex_rx_statistics->bt_coex_statistics_timer),
                                   dmac_btcoex_rx_rate_statistics_flag_callback,
                                   BTCOEX_RX_STATISTICS_TIME,
                                   (void *)(pst_mac_vap[uc_vap_idx]),
                                   OAL_TRUE,
                                   OAM_MODULE_ID_DMAC,
                                   (pst_mac_vap[uc_vap_idx])->ul_core_id);
#endif
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 dmac_btcoex_transfer_status_handler(frw_event_mem_stru *pst_event_mem)
{
    hal_btcoex_btble_status_stru *pst_btcoex_btble_status;
    frw_event_stru               *pst_event;
    hal_chip_stru                *pst_hal_chip;
    hal_to_dmac_device_stru      *pst_hal_device;
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV)
    dmac_user_btcoex_delba_stru  *pst_dmac_user_btcoex_delba;
    dmac_user_stru               *pst_dmac_user;
#endif
    oal_uint8                     uc_vap_idx;
    oal_uint8                     uc_valid_vap_num;
    oal_uint8                     auc_mac_vap_id[WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE];
    mac_vap_stru                 *pst_mac_vap[WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE] = {OAL_PTR_NULL};

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_transfer_status_handler::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)
#ifdef _PRE_WLAN_1103_PILOT

#else
    
    hal_btcoex_open_5g_upc();
#endif
#endif

    pst_event = frw_get_event_stru(pst_event_mem);

    /* ��ȡchipָ�� */
    pst_hal_chip = HAL_GET_CHIP_POINTER(pst_event->st_event_hdr.uc_chip_id);

    /* ��ʱֻ�Ǵ�����·��STA ��·TBD */
    pst_hal_device = HAL_DEV_GET_HAL2DMC_DEV(&(pst_hal_chip->ast_device[HAL_DEVICE_ID_MASTER]));

    pst_btcoex_btble_status = &(pst_hal_chip->st_hal_chip_base.st_btcoex_btble_status);

    /* �ҵ�����Ҫ���vap���� */
    uc_valid_vap_num = dmac_btcoex_find_all_valid_sta_per_device(pst_hal_device, auc_mac_vap_id);

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)
    /* �����ʱ�������ˣ���Ҫ�����µ�ʱ����ˢ�� */
    if(OAL_TRUE == pst_hal_device->st_btcoex_powersave_timer.en_is_registerd)
    {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_hal_device->st_btcoex_powersave_timer));

        /* ps��������ʱ����Ҫ���ݵ�ǰ״̬��ˢ�³�ʱ��ʱ��ʱ��,��Ϊ��Ӧҵ�����ʱ����������ps�жϣ���Ҫ�˴���ˢʱ�� */
        dmac_btcoex_ps_timeout_update_time(pst_hal_device);

        FRW_TIMER_CREATE_TIMER(&(pst_hal_device->st_btcoex_powersave_timer),
                           dmac_btcoex_pow_save_callback,
                           pst_hal_device->st_btcoex_sw_preempt.us_timeout_ms,
                           (oal_void *)pst_hal_device,
                           OAL_FALSE,
                           OAM_MODULE_ID_DMAC,
                           pst_hal_device->ul_core_id);
    }
#endif

    /* valid��vap�豸������Ӧ���� */
    for (uc_vap_idx = 0; uc_vap_idx < uc_valid_vap_num; uc_vap_idx++)
    {
        pst_mac_vap[uc_vap_idx]  = (mac_vap_stru *)mac_res_get_mac_vap(auc_mac_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_mac_vap[uc_vap_idx])
        {
            OAM_ERROR_LOG1(0, OAM_SF_COEX, "{dmac_btcoex_transfer_status_handler::pst_mac_vap[%d] IS NULL.}", auc_mac_vap_id[uc_vap_idx]);
            return OAL_ERR_CODE_PTR_NULL;
        }

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)
        /* �ҵ�valid sta������ɾ��BA�߼����� */
        dmac_btcoex_delba_event_process(pst_btcoex_btble_status, pst_mac_vap[uc_vap_idx]);
#else

        if (pst_btcoex_btble_status->un_ble_status.st_ble_status.bit_bt_transfer)
        {
            dmac_alg_cfg_btcoex_state_notify(pst_hal_device, BT_TRANSFER_ON);
        }
        else
        {
            dmac_alg_cfg_btcoex_state_notify(pst_hal_device, BT_TRANSFER_OFF);
        }

        pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user((pst_mac_vap[uc_vap_idx])->us_assoc_vap_id);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dmac_user))
        {
            OAM_WARNING_LOG0(auc_mac_vap_id[uc_vap_idx], OAM_SF_COEX, "{dmac_btcoex_transfer_status_handler:: pst_dmac_user null}");
            return OAL_ERR_CODE_PTR_NULL;
        }

        pst_dmac_user_btcoex_delba = &(pst_dmac_user->st_dmac_user_btcoex_stru.st_dmac_user_btcoex_delba);

        /* ����bt�򿪺͹رճ����£��ԾۺϽ��д��� */
        if (pst_btcoex_btble_status->un_ble_status.st_ble_status.bit_bt_transfer)
        {
            pst_dmac_user_btcoex_delba->en_ba_size_expect_index = BTCOEX_RX_WINDOW_SIZE_INDEX_2;
            pst_dmac_user_btcoex_delba->en_ba_size_real_index = BTCOEX_RX_WINDOW_SIZE_INDEX_2;
        }
        else
        {
            pst_dmac_user_btcoex_delba->en_ba_size_expect_index = BTCOEX_RX_WINDOW_SIZE_INDEX_3;
            pst_dmac_user_btcoex_delba->en_ba_size_real_index = BTCOEX_RX_WINDOW_SIZE_INDEX_3;
        }

        dmac_btcoex_update_ba_size(pst_mac_vap[uc_vap_idx], pst_dmac_user_btcoex_delba, pst_btcoex_btble_status);

        OAM_WARNING_LOG2(auc_mac_vap_id[uc_vap_idx], OAM_SF_COEX, "{dmac_btcoex_transfer_status_handler::bt transfer status changed:%d, bar_size: %d}",
                pst_btcoex_btble_status->un_ble_status.st_ble_status.bit_bt_transfer,
                pst_dmac_user_btcoex_delba->uc_ba_size);

        dmac_btcoex_delba_trigger(pst_mac_vap[uc_vap_idx], OAL_TRUE, pst_dmac_user_btcoex_delba);
#endif

    }

    return OAL_SUCC;
}


oal_uint32 dmac_btcoex_register_dmac_misc_event(hal_dmac_misc_sub_type_enum en_event_type, oal_uint32 (*p_func)(frw_event_mem_stru *))
{
    if(en_event_type >= HAL_EVENT_DMAC_MISC_SUB_TYPE_BUTT || NULL == p_func)
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "dmac_alg_register_dmac_misc_event fail");
        return  OAL_FAIL;
    }

    g_ast_dmac_misc_event_sub_table[en_event_type].p_func = p_func;

    return OAL_SUCC;
}


oal_uint32  dmac_btcoex_unregister_dmac_misc_event(hal_dmac_misc_sub_type_enum en_event_type)
{
    if(en_event_type >= HAL_EVENT_DMAC_MISC_SUB_TYPE_BUTT)
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "dmac_alg_unregister_dmac_misc_event fail");
        return  OAL_FAIL;
    }

    g_ast_dmac_misc_event_sub_table[en_event_type].p_func = NULL;
    return OAL_SUCC;
}


oal_uint32 dmac_btcoex_init(oal_void)
{
    if (OAL_SUCC != dmac_btcoex_register_dmac_misc_event(HAL_EVENT_DMAC_BT_A2DP, dmac_btcoex_a2dp_status_handler))
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_register_dmac_misc_event HAL_EVENT_DMAC_BT_A2DP fail!}");
        return OAL_FAIL;
    }

    if (OAL_SUCC != dmac_btcoex_register_dmac_misc_event(HAL_EVENT_DMAC_BT_TRANSFER, dmac_btcoex_transfer_status_handler))
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_register_dmac_misc_event HAL_EVENT_DMAC_BT_TRANSFER fail!}");
        dmac_btcoex_unregister_dmac_misc_event(HAL_EVENT_DMAC_BT_A2DP);
        return OAL_FAIL;
    }

    if (OAL_SUCC != dmac_btcoex_register_dmac_misc_event(HAL_EVENT_DMAC_BT_PAGE_SCAN, dmac_btcoex_page_scan_handler))
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_register_dmac_misc_event HAL_EVENT_DMAC_PAGE_SCAN fail!}");
        dmac_btcoex_unregister_dmac_misc_event(HAL_EVENT_DMAC_BT_A2DP);
        dmac_btcoex_unregister_dmac_misc_event(HAL_EVENT_DMAC_BT_TRANSFER);
        return OAL_FAIL;
    }

    if (OAL_SUCC != dmac_btcoex_register_dmac_misc_event(HAL_EVENT_DMAC_BT_SCO, dmac_btcoex_sco_status_handler))
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_register_dmac_misc_event HAL_EVENT_DMAC_BT_SCO fail!}");
        dmac_btcoex_unregister_dmac_misc_event(HAL_EVENT_DMAC_BT_A2DP);
        dmac_btcoex_unregister_dmac_misc_event(HAL_EVENT_DMAC_BT_TRANSFER);
        dmac_btcoex_unregister_dmac_misc_event(HAL_EVENT_DMAC_BT_PAGE_SCAN);
        return OAL_FAIL;
    }

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)
    if (OAL_SUCC != dmac_btcoex_register_dmac_misc_event(HAL_EVENT_DMAC_BT_ACL, dmac_btcoex_ps_status_handler))
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_register_dmac_misc_event HAL_EVENT_DMAC_BT_ACL fail!}");
        dmac_btcoex_unregister_dmac_misc_event(HAL_EVENT_DMAC_BT_A2DP);
        dmac_btcoex_unregister_dmac_misc_event(HAL_EVENT_DMAC_BT_TRANSFER);
        dmac_btcoex_unregister_dmac_misc_event(HAL_EVENT_DMAC_BT_PAGE_SCAN);
        dmac_btcoex_unregister_dmac_misc_event(HAL_EVENT_DMAC_BT_SCO);
        return OAL_FAIL;
    }

    if (OAL_SUCC != dmac_btcoex_register_dmac_misc_event(HAL_EVENT_DMAC_BT_INQUIRY, dmac_btcoex_inquiry_status_handler))
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_register_dmac_misc_event HAL_EVENT_DMAC_BT_INQUIRY fail!}");
        dmac_btcoex_unregister_dmac_misc_event(HAL_EVENT_DMAC_BT_A2DP);
        dmac_btcoex_unregister_dmac_misc_event(HAL_EVENT_DMAC_BT_TRANSFER);
        dmac_btcoex_unregister_dmac_misc_event(HAL_EVENT_DMAC_BT_PAGE_SCAN);
        dmac_btcoex_unregister_dmac_misc_event(HAL_EVENT_DMAC_BT_SCO);
        dmac_btcoex_unregister_dmac_misc_event(HAL_EVENT_DMAC_BT_ACL);
        return OAL_FAIL;
    }

    if (OAL_SUCC != dmac_btcoex_register_dmac_misc_event(HAL_EVENT_DMAC_BT_LDAC, dmac_btcoex_ldac_status_handler))
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_register_dmac_misc_event HAL_EVENT_DMAC_BT_LDAC fail!}");
        dmac_btcoex_unregister_dmac_misc_event(HAL_EVENT_DMAC_BT_A2DP);
        dmac_btcoex_unregister_dmac_misc_event(HAL_EVENT_DMAC_BT_TRANSFER);
        dmac_btcoex_unregister_dmac_misc_event(HAL_EVENT_DMAC_BT_PAGE_SCAN);
        dmac_btcoex_unregister_dmac_misc_event(HAL_EVENT_DMAC_BT_SCO);
        dmac_btcoex_unregister_dmac_misc_event(HAL_EVENT_DMAC_BT_ACL);
        dmac_btcoex_unregister_dmac_misc_event(HAL_EVENT_DMAC_BT_INQUIRY);
        return OAL_FAIL;
    }
#endif

    return OAL_SUCC;
}


oal_uint32 dmac_btcoex_exit(oal_void)
{
    dmac_btcoex_unregister_dmac_misc_event(HAL_EVENT_DMAC_BT_A2DP);
    dmac_btcoex_unregister_dmac_misc_event(HAL_EVENT_DMAC_BT_TRANSFER);
    dmac_btcoex_unregister_dmac_misc_event(HAL_EVENT_DMAC_BT_PAGE_SCAN);
    dmac_btcoex_unregister_dmac_misc_event(HAL_EVENT_DMAC_BT_SCO);
    dmac_btcoex_unregister_dmac_misc_event(HAL_EVENT_DMAC_BT_ACL);
    dmac_btcoex_unregister_dmac_misc_event(HAL_EVENT_DMAC_BT_INQUIRY);
    dmac_btcoex_unregister_dmac_misc_event(HAL_EVENT_DMAC_BT_LDAC);

    return OAL_SUCC;
}


oal_uint32 dmac_btcoex_mgmt_priority_timeout_callback(oal_void *p_arg)
{
    mac_vap_stru *pst_mac_vap = (mac_vap_stru *)p_arg;

    dmac_btcoex_set_mgmt_priority(pst_mac_vap, 0);
    return OAL_SUCC;
}


oal_uint32 dmac_btcoex_wlan_priority_timeout_callback(oal_void *p_arg)
{
    mac_vap_stru *pst_mac_vap = (mac_vap_stru *)p_arg;

    if(OAL_PTR_NULL == pst_mac_vap)
    {
        OAM_WARNING_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_wlan_priority_timeout_callback:: pst_mac_vap null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAM_WARNING_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_wlan_priority_timeout_callback:: timeout!}");

    dmac_btcoex_set_wlan_priority(pst_mac_vap, OAL_FALSE, 0);

    return OAL_SUCC;
}


oal_void dmac_btcoex_change_state_syn(mac_vap_stru *pst_mac_vap)
{
    hal_to_dmac_device_stru *pst_hal_device;

    pst_hal_device = DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap);
    if (OAL_PTR_NULL == pst_hal_device)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_change_state_syn:: DMAC_VAP_GET_HAL_DEVICE null}");
        return;
    }

    switch (pst_mac_vap->en_vap_state)
    {
    case MAC_VAP_STATE_STA_WAIT_SCAN:
    case MAC_VAP_STATE_STA_WAIT_AUTH_SEQ2:
    case MAC_VAP_STATE_STA_WAIT_AUTH_SEQ4:
    case MAC_VAP_STATE_STA_AUTH_COMP:
    case MAC_VAP_STATE_STA_WAIT_ASOC:
    case MAC_VAP_STATE_STA_OBSS_SCAN:
    case MAC_VAP_STATE_STA_BG_SCAN:
        hal_set_btcoex_hw_rx_priority_dis(pst_hal_device, 0);
        break;
    default:
        hal_set_btcoex_hw_rx_priority_dis(pst_hal_device, 1);
    }
}


oal_void dmac_btcoex_delba_trigger(mac_vap_stru *pst_mac_vap, oal_bool_enum_uint8 en_need_delba,
    dmac_user_btcoex_delba_stru  *pst_dmac_user_btcoex_delba)
{
    dmac_to_hmac_btcoex_rx_delba_trigger_event_stru  st_dmac_to_hmac_btcoex_rx_delba;

    /* ������һ������ɾ��BA��ɾ�����ʹ�ܣ�������wifi�Լ�ɾ��BA��Ҫ���ù������õ����� */
    if(OAL_TRUE == en_need_delba)
    {
        pst_dmac_user_btcoex_delba->en_delba_trigger = OAL_TRUE;
    }

    st_dmac_to_hmac_btcoex_rx_delba.en_need_delba     = en_need_delba;
    st_dmac_to_hmac_btcoex_rx_delba.uc_ba_size        = pst_dmac_user_btcoex_delba->uc_ba_size;
    st_dmac_to_hmac_btcoex_rx_delba.us_user_id        = pst_mac_vap->us_assoc_vap_id;

    dmac_send_sys_event(pst_mac_vap,
                        WLAN_CFGID_BTCOEX_RX_DELBA_TRIGGER,
                        OAL_SIZEOF(dmac_to_hmac_btcoex_rx_delba_trigger_event_stru),
                        (oal_uint8 *)&st_dmac_to_hmac_btcoex_rx_delba);
}


oal_void dmac_btcoex_vap_up_handle(mac_vap_stru *pst_mac_vap)
{
    hal_to_dmac_chip_stru   *pst_hal_chip;
    hal_to_dmac_device_stru *pst_hal_device;
    mac_device_stru         *pst_mac_device;
    oal_bool_enum_uint8      en_state_change = OAL_FALSE;

    pst_hal_chip = DMAC_VAP_GET_HAL_CHIP(pst_mac_vap);
    if (OAL_PTR_NULL == pst_hal_chip)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_vap_up_handle:: DMAC_VAP_GET_HAL_CHIP null}");
        return;
    }

    pst_hal_device = DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap);
    if (OAL_PTR_NULL == pst_hal_device)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_vap_up_handle:: DMAC_VAP_GET_HAL_DEVICE null}");
        return;
    }

    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_vap_up_handle::pst_mac_device null.}");

        return;
    }

    if (WLAN_VAP_MODE_BSS_AP == pst_mac_vap->en_vap_mode)
    {
        hal_set_btcoex_soc_gpreg1(OAL_FALSE, BIT0, 0); //�ϵ����

        /* ״̬�����仯 */
        en_state_change = OAL_TRUE;
    }

    if (1 == pst_mac_device->uc_vap_num)
    {
        hal_set_btcoex_soc_gpreg0(OAL_TRUE, BIT13, 13); //wifi on
        hal_set_btcoex_soc_gpreg1(OAL_TRUE, BIT6, 6);   //wifi ���ڷǵ͹���״̬

        /* ״̬�����仯 */
        en_state_change = OAL_TRUE;
    }

    if(OAL_TRUE == en_state_change)
    {
        /* ״̬�仯��֪ͨbt */
        hal_coex_sw_irq_set(HAL_COEX_SW_IRQ_BT);
    }

    /* ����btble״̬ */
    hal_update_btcoex_btble_status(pst_hal_chip);

    OAM_WARNING_LOG_ALTER(pst_mac_vap->uc_vap_id, OAM_SF_COEX,
        "{dmac_btcoex_vap_up_handle::::status resv:%d, send:%d, data_transfer:%d, a2dp:%d, bt_on:%d.}",
        5, pst_hal_chip->st_btcoex_btble_status.un_bt_status.st_bt_status.bit_bt_data_trans,
           pst_hal_chip->st_btcoex_btble_status.un_bt_status.st_bt_status.bit_bt_acl,
           pst_hal_chip->st_btcoex_btble_status.un_ble_status.st_ble_status.bit_bt_transfer,
           pst_hal_chip->st_btcoex_btble_status.un_bt_status.st_bt_status.bit_bt_a2dp,
           pst_hal_chip->st_btcoex_btble_status.un_bt_status.st_bt_status.bit_bt_on);

    if (pst_hal_chip->st_btcoex_btble_status.un_bt_status.st_bt_status.bit_bt_on)
    {
        /* ����MAC BT���� */
        hal_set_btcoex_sw_all_abort_ctrl(pst_hal_device, OAL_TRUE);
    }
}


oal_void dmac_btcoex_update_rx_rate_threshold (mac_vap_stru *pst_mac_vap, dmac_user_btcoex_delba_stru *pst_dmac_user_btcoex_delba)
{
    wlan_channel_band_enum_uint8 en_band;
    wlan_bw_cap_enum_uint8       en_bandwidth;

    en_band = pst_mac_vap->st_channel.en_band;
    mac_vap_get_bandwidth_cap(pst_mac_vap, &en_bandwidth);

    if ((en_band >= WLAN_BAND_BUTT) || (en_bandwidth >= WLAN_BW_CAP_BUTT))
    {
        OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_update_rx_rate_threshold::band %d, bandwidth %d exceed scale!}",
                         en_band, en_bandwidth);
        return;
    }

#if (WLAN_MAX_NSS_NUM == WLAN_DOUBLE_NSS)
    if(WLAN_DOUBLE_NSS == pst_dmac_user_btcoex_delba->en_user_nss_num)
    {
        pst_dmac_user_btcoex_delba->ul_rx_rate_threshold_min =
        g_aus_btcoex_rate_thresholds_mimo[en_band][en_bandwidth][BTCOEX_RATE_THRESHOLD_MIN];
        pst_dmac_user_btcoex_delba->ul_rx_rate_threshold_max =
        g_aus_btcoex_rate_thresholds_mimo[en_band][en_bandwidth][BTCOEX_RATE_THRESHOLD_MAX];
    }
    else
#endif
    {
        pst_dmac_user_btcoex_delba->ul_rx_rate_threshold_min =
        g_aus_btcoex_rate_thresholds_siso[en_band][en_bandwidth][BTCOEX_RATE_THRESHOLD_MIN];
        pst_dmac_user_btcoex_delba->ul_rx_rate_threshold_max =
        g_aus_btcoex_rate_thresholds_siso[en_band][en_bandwidth][BTCOEX_RATE_THRESHOLD_MAX];
    }

    OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_update_rx_rate_threshold:: min: %d, max: %d, band %d, bandwidth %d.}",
        pst_dmac_user_btcoex_delba->ul_rx_rate_threshold_min, pst_dmac_user_btcoex_delba->ul_rx_rate_threshold_max, en_band, en_bandwidth);
}


oal_void dmac_btcoex_user_spatial_stream_change_notify(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user)
{
    dmac_user_stru                *pst_dmac_user;
    dmac_user_btcoex_delba_stru   *pst_dmac_user_btcoex_delba;

    if (OAL_FALSE == MAC_BTCOEX_CHECK_VALID_STA(pst_mac_vap))
    {
        return;
    }

    pst_dmac_user = MAC_GET_DMAC_USER(pst_mac_user);

    pst_dmac_user_btcoex_delba = &(pst_dmac_user->st_dmac_user_btcoex_stru.st_dmac_user_btcoex_delba);

    pst_dmac_user_btcoex_delba->en_user_nss_num  = pst_dmac_user->st_user_base_info.en_avail_num_spatial_stream;

    /* user�ռ��������仯������ˢ���������� */
    dmac_btcoex_update_rx_rate_threshold(pst_mac_vap, pst_dmac_user_btcoex_delba);
}


OAL_STATIC oal_uint32 dmac_btcoex_lower_rate_callback(oal_void *p_arg)
{
    hal_to_dmac_chip_stru         *pst_hal_chip;
    hal_to_dmac_device_stru       *pst_hal_device  = OAL_PTR_NULL;
    hal_btcoex_btble_status_stru  *pst_btble_status;
    dmac_user_btcoex_delba_stru   *pst_dmac_user_btcoex_delba;
    dmac_user_btcoex_rx_info_stru *pst_btcoex_wifi_rx_rate_info;
    dmac_user_stru                *pst_dmac_user;
    mac_vap_stru                  *pst_mac_vap;
    oal_uint32                     ul_rx_rate = 0;
    oal_uint16                     us_rx_count = 0;
    oal_uint32                     ul_rate_threshold_min;
    oal_uint32                     ul_rate_threshold_max;

    pst_mac_vap   = (mac_vap_stru *)p_arg;
    pst_hal_chip  = DMAC_VAP_GET_HAL_CHIP(pst_mac_vap);
    if (OAL_PTR_NULL == pst_hal_chip)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_init_preempt:: DMAC_VAP_GET_HAL_CHIP null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user(pst_mac_vap->us_assoc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dmac_user))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_lower_rate_callback::pst_dmac_user[%d] is NULL}", pst_mac_vap->us_assoc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hal_device = DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hal_device))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_lower_rate_callback::pst_hal_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_dmac_user_btcoex_delba = &(pst_dmac_user->st_dmac_user_btcoex_stru.st_dmac_user_btcoex_delba);
    pst_btble_status = &(pst_hal_chip->st_btcoex_btble_status);
    ul_rate_threshold_min = pst_dmac_user_btcoex_delba->ul_rx_rate_threshold_min;
    ul_rate_threshold_max = pst_dmac_user_btcoex_delba->ul_rx_rate_threshold_max;
    pst_btcoex_wifi_rx_rate_info = &(pst_dmac_user->st_dmac_user_btcoex_stru.st_dmac_user_btcoex_rx_info);

    if (OAL_FALSE == pst_dmac_user_btcoex_delba->en_get_addba_req_flag)
    {
        dmac_btcoex_rx_average_rate_calculate(pst_btcoex_wifi_rx_rate_info, &ul_rx_rate, &us_rx_count);

        if (pst_hal_device->st_hal_alg_stat.en_alg_distance_stat < HAL_ALG_USER_DISTANCE_FAR)
        {
            /* BA�ۺϸ������� 1.DEC������С��min��2.INC�����ʴ���max��3.DEC��ʵ�ʾۺϵ���index3������С�ڵ��� min+max%2 */
            if ((BTCOEX_RX_WINDOW_SIZE_DECREASE == pst_dmac_user_btcoex_delba->en_ba_size_tendence
                    && (ul_rx_rate < ul_rate_threshold_min))
                || ((BTCOEX_RX_WINDOW_SIZE_INCREASE == pst_dmac_user_btcoex_delba->en_ba_size_tendence)
                    && (ul_rx_rate > ul_rate_threshold_max))
                || ((BTCOEX_RX_WINDOW_SIZE_DECREASE == pst_dmac_user_btcoex_delba->en_ba_size_tendence)
                    && (BTCOEX_RX_WINDOW_SIZE_INDEX_3 == pst_dmac_user_btcoex_delba->en_ba_size_real_index)
                    && (ul_rx_rate < (ul_rate_threshold_min + (ul_rate_threshold_max >> 1)))))
            {
                pst_dmac_user_btcoex_delba->en_ba_size_real_index = pst_dmac_user_btcoex_delba->en_ba_size_expect_index;
                dmac_btcoex_update_ba_size(pst_mac_vap, pst_dmac_user_btcoex_delba, pst_btble_status);

                /* ��������ɾ��BA�߼�����Ҫ���յ�addba rsp֡ʱ��ָ��˱�� */
                dmac_btcoex_delba_trigger(pst_mac_vap, OAL_TRUE, pst_dmac_user_btcoex_delba);

                OAM_WARNING_LOG_ALTER(pst_dmac_user->st_user_base_info.uc_vap_id, OAM_SF_COEX,
                    "{dmac_btcoex_lower_rate_callback::ba_size change to:%d,rate:%d,tendence: %d,real ba size index:%d,rate_threshold_min:%d,rate_threshold_max:%d.}",
                    6, pst_dmac_user_btcoex_delba->uc_ba_size, ul_rx_rate, pst_dmac_user_btcoex_delba->en_ba_size_tendence,
                    pst_dmac_user_btcoex_delba->en_ba_size_real_index, ul_rate_threshold_min, ul_rate_threshold_max);
            }
            else
            {
                /* �������оۺϲ��� */
                pst_dmac_user_btcoex_delba->en_ba_size_expect_index = pst_dmac_user_btcoex_delba->en_ba_size_real_index;
                dmac_btcoex_update_ba_size(pst_mac_vap, pst_dmac_user_btcoex_delba, pst_btble_status);
                OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_lower_rate_callback::ba_size donot change, keep at:%d, rate:%d.}",
                    pst_dmac_user_btcoex_delba->uc_ba_size, ul_rx_rate);
            }
        }
    }
    else
    {
        pst_dmac_user_btcoex_delba->en_ba_size_real_index = pst_dmac_user_btcoex_delba->en_ba_size_expect_index;
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_lower_rate_callback::receive addba req before delba.}");
    }

    FRW_TIMER_RESTART_TIMER(&(MAC_GET_DMAC_VAP(pst_mac_vap)->st_dmac_vap_btcoex.st_dmac_vap_btcoex_rx_statistics.bt_coex_statistics_timer),
                                BTCOEX_RX_STATISTICS_TIME, OAL_TRUE);

    return OAL_SUCC;
}


oal_void dmac_btcoex_lower_rate_process (mac_vap_stru *pst_vap)
{
    hal_btcoex_btble_status_stru       *pst_btcoex_btble_status;
    bt_status_stru                     *pst_bt_status;
    dmac_vap_btcoex_rx_statistics_stru *pst_dmac_vap_btcoex_rx_statistics;
    dmac_user_btcoex_rx_info_stru      *pst_dmac_user_btcoex_rx_info;
    dmac_user_btcoex_delba_stru        *pst_dmac_user_btcoex_delba;
    hal_to_dmac_chip_stru              *pst_hal_chip;
    dmac_vap_stru                      *pst_dmac_vap;
    dmac_user_stru                     *pst_dmac_user;
    oal_uint32                          ul_rate_threshold_min;
    oal_uint32                          ul_rate_threshold_max;
    oal_uint32                          ul_rx_rate = 0;
    oal_uint16                          us_rx_count = 0;

    if (OAL_FALSE == MAC_BTCOEX_CHECK_VALID_STA(pst_vap))
    {
        /* ����ͳ�Ʒ��ֲ�����Ҫ��vapʱӦ�ö�vap������ͳ�ƽ�����㣬userȥ�������Զ��� */
        return;
    }

    pst_dmac_vap = MAC_GET_DMAC_VAP(pst_vap);
    pst_hal_chip = DMAC_VAP_GET_HAL_CHIP(pst_vap);
    if (OAL_PTR_NULL == pst_hal_chip)
    {
        OAM_WARNING_LOG0(pst_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_lower_rate_process:: DMAC_VAP_GET_HAL_CHIP null}");
        return;
    }

    /* STAģʽ��ȡuser */
    pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user(pst_vap->us_assoc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dmac_user))
    {
        OAM_WARNING_LOG1(pst_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_lower_rate_process::pst_dmac_user[%d] null.}",
            pst_vap->us_assoc_vap_id);
        return;
    }

    pst_dmac_vap_btcoex_rx_statistics = &(pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_rx_statistics);
    pst_dmac_user_btcoex_rx_info      = &(pst_dmac_user->st_dmac_user_btcoex_stru.st_dmac_user_btcoex_rx_info);
    pst_dmac_user_btcoex_delba        = &(pst_dmac_user->st_dmac_user_btcoex_stru.st_dmac_user_btcoex_delba);

    /* �ۼӽ������� */
    if (OAL_TRUE == pst_dmac_vap_btcoex_rx_statistics->en_rx_rate_statistics_flag)
    {
        if (0 != pst_dmac_user->ul_rx_rate)
        {
            /* ����û������ڣ�ȥ������pst_dmac_user_btcoex_rx_infoҲ�ᱻ���� */
            pst_dmac_user_btcoex_rx_info->ull_rx_rate_mbps += pst_dmac_user->ul_rx_rate;
            pst_dmac_user_btcoex_rx_info->us_rx_rate_stat_count++;
        }
    }
    else
    {
        return;
    }

    /* ����������� */
    if (OAL_TRUE == pst_dmac_vap_btcoex_rx_statistics->en_rx_rate_statistics_timeout)
    {
        pst_dmac_vap_btcoex_rx_statistics->en_rx_rate_statistics_timeout = OAL_FALSE;

        dmac_btcoex_rx_average_rate_calculate(pst_dmac_user_btcoex_rx_info, &ul_rx_rate, &us_rx_count);

        if (g_rx_statistics_print)
        {
            OAM_WARNING_LOG2(pst_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_lower_rate_process:: rate:%d,count:%d.}", ul_rx_rate, us_rx_count);
        }
    }
    else
    {
        return;
    }

    /* ����������� */
    if (DMAC_VAP_GET_HAL_DEVICE(pst_vap)->st_hal_alg_stat.en_alg_distance_stat < HAL_ALG_USER_DISTANCE_FAR)
    {
        pst_btcoex_btble_status = &(pst_hal_chip->st_btcoex_btble_status);
        pst_bt_status = &(pst_btcoex_btble_status->un_bt_status.st_bt_status);

        /*1. �绰�̶��ۺ� */
        if (pst_bt_status->bit_bt_sco)
        {
            return;
        }

        /* ��ʼ���� */
        pst_dmac_user_btcoex_delba->en_ba_size_tendence = BTCOEX_RX_WINDOW_SIZE_HOLD;
        pst_dmac_user_btcoex_delba->en_ba_size_expect_index = pst_dmac_user_btcoex_delba->en_ba_size_real_index;
        ul_rate_threshold_min = pst_dmac_user_btcoex_delba->ul_rx_rate_threshold_min;
        ul_rate_threshold_max = pst_dmac_user_btcoex_delba->ul_rx_rate_threshold_max;

        /*2. ����2G mimo 20M��ʱ�̶�2��,40M�̶�3�� */
        if ((WLAN_BAND_2G == pst_vap->st_channel.en_band)
            &&(pst_bt_status->bit_bt_a2dp)&&(WLAN_DOUBLE_NSS == pst_dmac_user_btcoex_delba->en_user_nss_num))
        {
            /* 40M, �����ȶ���2�� */
            if((pst_vap->st_channel.en_bandwidth == WLAN_BAND_WIDTH_40MINUS) || (pst_vap->st_channel.en_bandwidth == WLAN_BAND_WIDTH_40PLUS))
            {
                if ((BTCOEX_RX_WINDOW_SIZE_INDEX_3 == pst_dmac_user_btcoex_delba->en_ba_size_expect_index)
                        && (ul_rx_rate < (ul_rate_threshold_min + (ul_rate_threshold_max >> 1))))
                {
                    pst_dmac_user_btcoex_delba->en_ba_size_expect_index--;
                    pst_dmac_user_btcoex_delba->en_ba_size_tendence = BTCOEX_RX_WINDOW_SIZE_DECREASE;
                }
                else if (pst_dmac_user_btcoex_delba->en_ba_size_expect_index < BTCOEX_RX_WINDOW_SIZE_INDEX_2)
                {
                    pst_dmac_user_btcoex_delba->en_ba_size_expect_index++;
                    pst_dmac_user_btcoex_delba->en_ba_size_tendence = BTCOEX_RX_WINDOW_SIZE_INCREASE;
                }
            }
            else /* 20M */
            {
                switch (pst_dmac_user_btcoex_delba->en_ba_size_expect_index)
                {
                    case BTCOEX_RX_WINDOW_SIZE_INDEX_3:
                        if (ul_rx_rate < (ul_rate_threshold_min + (ul_rate_threshold_max >> 1)))
                        {
                            pst_dmac_user_btcoex_delba->en_ba_size_expect_index--;
                            pst_dmac_user_btcoex_delba->en_ba_size_tendence = BTCOEX_RX_WINDOW_SIZE_DECREASE;
                        }
                        break;

                    /* ���͵�2��֮�������ȶ���2����ֻ�ܽ� */
                    case BTCOEX_RX_WINDOW_SIZE_INDEX_2:
                        if (ul_rx_rate < ul_rate_threshold_min)
                        {
                            pst_dmac_user_btcoex_delba->en_ba_size_expect_index--;
                            pst_dmac_user_btcoex_delba->en_ba_size_tendence = BTCOEX_RX_WINDOW_SIZE_DECREASE;
                        }
                        break;

                    /* ���͵�1��֮�󣬿����˵�0����0��֮���ٱ仯��Ҳ����̽�⵽2���������ȶ���2�� */
                    case BTCOEX_RX_WINDOW_SIZE_INDEX_1:
                        if (ul_rx_rate < ul_rate_threshold_min)
                        {
                            pst_dmac_user_btcoex_delba->en_ba_size_expect_index--;
                            pst_dmac_user_btcoex_delba->en_ba_size_tendence = BTCOEX_RX_WINDOW_SIZE_DECREASE;
                        }
                        else if (ul_rx_rate > ul_rate_threshold_max)
                        {
                            pst_dmac_user_btcoex_delba->en_ba_size_expect_index++;
                            pst_dmac_user_btcoex_delba->en_ba_size_tendence = BTCOEX_RX_WINDOW_SIZE_INCREASE;
                        }
                        break;

                    case BTCOEX_RX_WINDOW_SIZE_INDEX_0:
                        OAM_WARNING_LOG0(pst_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_lower_rate_process::KEEP at BTCOEX_RX_WINDOW_SIZE_INDEX_0.}");
                        break;

                    default:
                        OAM_ERROR_LOG1(pst_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_lower_rate_process::ba_size_expect_index error:%d.}",
                                    pst_dmac_user_btcoex_delba->en_ba_size_expect_index);
                    break;
                }
            }
        }
#if(_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV)
        /*3. 5G80M���̶ֹ�2�� */
        else if ((WLAN_BAND_5G == pst_vap->st_channel.en_band) && (pst_vap->st_channel.en_bandwidth > WLAN_BAND_WIDTH_40MINUS)
                && (pst_bt_status->bit_bt_a2dp))
        {
            if ((BTCOEX_RX_WINDOW_SIZE_INDEX_3 == pst_dmac_user_btcoex_delba->en_ba_size_expect_index)
                    && (ul_rx_rate < (ul_rate_threshold_min + ul_rate_threshold_max >> 1)))
            {
                pst_dmac_user_btcoex_delba->en_ba_size_expect_index--;
                pst_dmac_user_btcoex_delba->en_ba_size_tendence = BTCOEX_RX_WINDOW_SIZE_DECREASE;
            }
            else if (pst_dmac_user_btcoex_delba->en_ba_size_expect_index < BTCOEX_RX_WINDOW_SIZE_INDEX_2)
            {
                pst_dmac_user_btcoex_delba->en_ba_size_expect_index++;
                pst_dmac_user_btcoex_delba->en_ba_size_tendence = BTCOEX_RX_WINDOW_SIZE_INCREASE;
            }
        }
#else
        /*3.1103 5G������ */
        else if (WLAN_BAND_5G == pst_vap->st_channel.en_band)
        {

        }
#endif
        else /* 4. siso���� */
        {
            switch (pst_dmac_user_btcoex_delba->en_ba_size_expect_index)
            {
                case BTCOEX_RX_WINDOW_SIZE_INDEX_3:
                    if (ul_rx_rate < (ul_rate_threshold_min + (ul_rate_threshold_max >> 1)))
                    {
                        pst_dmac_user_btcoex_delba->en_ba_size_expect_index--;
                        pst_dmac_user_btcoex_delba->en_ba_size_tendence = BTCOEX_RX_WINDOW_SIZE_DECREASE;
                    }
                    break;

                case BTCOEX_RX_WINDOW_SIZE_INDEX_2:
                    if (ul_rx_rate < ul_rate_threshold_min)
                    {
                        pst_dmac_user_btcoex_delba->en_ba_size_expect_index--;
                        pst_dmac_user_btcoex_delba->en_ba_size_tendence = BTCOEX_RX_WINDOW_SIZE_DECREASE;
                    }
                    break;

                case BTCOEX_RX_WINDOW_SIZE_INDEX_1:
                    if (ul_rx_rate < ul_rate_threshold_min)
                    {
                        pst_dmac_user_btcoex_delba->en_ba_size_expect_index--;
                        pst_dmac_user_btcoex_delba->en_ba_size_tendence = BTCOEX_RX_WINDOW_SIZE_DECREASE;
                    }
                    else if (ul_rx_rate > ul_rate_threshold_max)
                    {
                        pst_dmac_user_btcoex_delba->en_ba_size_expect_index++;
                        pst_dmac_user_btcoex_delba->en_ba_size_tendence = BTCOEX_RX_WINDOW_SIZE_INCREASE;
                    }
                    break;

                case BTCOEX_RX_WINDOW_SIZE_INDEX_0:
                    if (ul_rx_rate > ul_rate_threshold_max)
                    {
                        pst_dmac_user_btcoex_delba->en_ba_size_expect_index++;
                        pst_dmac_user_btcoex_delba->en_ba_size_tendence = BTCOEX_RX_WINDOW_SIZE_INCREASE;
                    }
                    break;
                default:
                    OAM_ERROR_LOG1(pst_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_lower_rate_process::ba_size_expect_index error:%d.}",
                                pst_dmac_user_btcoex_delba->en_ba_size_expect_index);
                    break;
            }
        }

        if (BTCOEX_RX_WINDOW_SIZE_HOLD != pst_dmac_user_btcoex_delba->en_ba_size_tendence)
        {
            dmac_btcoex_update_ba_size(pst_vap, pst_dmac_user_btcoex_delba, pst_btcoex_btble_status);

            dmac_btcoex_delba_trigger(pst_vap, OAL_FALSE, pst_dmac_user_btcoex_delba);

            OAM_WARNING_LOG3(pst_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_lower_rate_process::ba_size will change to:%d in 5s,rate:%d,count:%d.}",
                                pst_dmac_user_btcoex_delba->uc_ba_size, ul_rx_rate, us_rx_count);

            pst_dmac_user_btcoex_delba->en_get_addba_req_flag = OAL_FALSE;

            FRW_TIMER_STOP_TIMER(&(pst_dmac_vap_btcoex_rx_statistics->bt_coex_statistics_timer));
            FRW_TIMER_CREATE_TIMER(&(pst_dmac_vap_btcoex_rx_statistics->bt_coex_low_rate_timer),
                                           dmac_btcoex_lower_rate_callback,
                                           BTCOEX_RX_LOW_RATE_TIME,
                                           (oal_void *)pst_vap,
                                           OAL_FALSE,
                                           OAM_MODULE_ID_DMAC,
                                           pst_vap->ul_core_id);
        }
    }
}


oal_void dmac_btcoex_release_rx_prot(mac_vap_stru *pst_mac_vap, oal_uint8 uc_data_type)
{
    bt_status_stru                  *pst_bt_status;
    hal_to_dmac_chip_stru           *pst_hal_chip;

    pst_hal_chip = DMAC_VAP_GET_HAL_CHIP(pst_mac_vap);
    if (OAL_PTR_NULL == pst_hal_chip)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_release_rx_prot:: DMAC_VAP_GET_HAL_CHIP null}");
        return;
    }

    pst_bt_status = &(pst_hal_chip->st_btcoex_btble_status.un_bt_status.st_bt_status);
    if (!pst_bt_status->bit_bt_on || (uc_data_type == MAC_DATA_BUTT))
    {
        return;
    }

    if(MAC_DATA_ARP_RSP >= uc_data_type)
    {
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV)
        /* �յ�EAPOL֡�Ժ�ˢ��occupied_period�Ա�֤BT������ */
        if ((uc_data_type == MAC_DATA_EAPOL) && !(pst_bt_status->bit_bt_sco))
        {
            dmac_btcoex_set_occupied_period(MAC_GET_DMAC_VAP(pst_mac_vap), 0);
        }
        else/* �����ؼ�֡����priority��arp rsp��dhcp֡ ��tx_vip_frame��Ӧ */
#endif
        {
            dmac_btcoex_set_wlan_priority(pst_mac_vap, OAL_FALSE, 0);
        }
    }
}


oal_void dmac_btcoex_tx_mgmt_frame(mac_vap_stru *pst_mac_vap, oal_netbuf_stru *pst_netbuf_mgmt)
{
    hal_to_dmac_chip_stru    *pst_hal_chip;
    oal_uint8                 uc_mgmt_type;
    bt_status_stru           *pst_bt_status;
    mac_ieee80211_frame_stru *pst_mac_header   = (mac_ieee80211_frame_stru *)oal_netbuf_header(pst_netbuf_mgmt);
    oal_uint8                *puc_mac_payload  = mac_netbuf_get_payload(pst_netbuf_mgmt);

    pst_hal_chip   = DMAC_VAP_GET_HAL_CHIP(pst_mac_vap);
    if (OAL_PTR_NULL == pst_hal_chip)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_tx_mgmt_frame:: DMAC_VAP_GET_HAL_CHIP null}");
        return;
    }

    /* ����Թ���֡���������������ǿ���֡ ,ͬʱAP��Ҳ����   TBD */
    pst_bt_status = &(pst_hal_chip->st_btcoex_btble_status.un_bt_status.st_bt_status);
    if (!pst_bt_status->bit_bt_on
#ifdef _PRE_WLAN_FEATURE_LTECOEX
        && !pst_hal_chip->ul_lte_coex_status
#endif
        )
    {
        return;
    }

    uc_mgmt_type = pst_mac_header->st_frame_control.bit_sub_type;

    /* STAģʽ */
    if(WLAN_VAP_MODE_BSS_STA == pst_mac_vap->en_vap_mode)
    {
        /* delba��addba resp֡��Ҫ��֤�������� */
        if ((WLAN_FC0_SUBTYPE_ACTION|WLAN_FC0_TYPE_MGT) == mac_get_frame_type_and_subtype((oal_uint8 *)pst_mac_header))
        {
            if ((MAC_ACTION_CATEGORY_BA == puc_mac_payload[0]) &&
                ((MAC_BA_ACTION_DELBA == puc_mac_payload[1])||(MAC_BA_ACTION_ADDBA_RSP == puc_mac_payload[1])))
            {
                dmac_btcoex_set_mgmt_priority(pst_mac_vap, BTCOEX_PRIO_TIMEOUT_10MS);
                return;
            }
        }

        if (WLAN_MANAGEMENT == pst_mac_header->st_frame_control.bit_type)
        {
            switch (uc_mgmt_type)
            {
                case WLAN_PROBE_REQ:
                    dmac_btcoex_set_mgmt_priority(pst_mac_vap, BTCOEX_PRIO_TIMEOUT_20MS);
                    break;

                case WLAN_AUTH:
                case WLAN_ASSOC_REQ:
                case WLAN_REASSOC_REQ:
                    dmac_btcoex_set_mgmt_priority(pst_mac_vap, BTCOEX_PRIO_TIMEOUT_10MS);
                    break;

                case WLAN_DEAUTH:
                case WLAN_DISASOC:
                case WLAN_ACTION:
                    dmac_btcoex_set_mgmt_priority(pst_mac_vap, BTCOEX_PRIO_TIMEOUT_10MS);
                    break;

                default:
                    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_tx_mgmt_frame:: uc_mgmt_type[%d] notsupport!}", uc_mgmt_type);
                    break;
            }
        }
    }
    else if(WLAN_VAP_MODE_BSS_AP == pst_mac_vap->en_vap_mode)
    {
        if (WLAN_MANAGEMENT == pst_mac_header->st_frame_control.bit_type)
        {
            switch (uc_mgmt_type)
            {
                case WLAN_PROBE_RSP:
                case WLAN_ASSOC_RSP:
                case WLAN_REASSOC_RSP:
                case WLAN_BEACON:
                case WLAN_AUTH:
                case WLAN_DEAUTH:
                case WLAN_DISASOC:
                case WLAN_ACTION:
                    dmac_btcoex_set_mgmt_priority(pst_mac_vap, BTCOEX_PRIO_TIMEOUT_10MS);
                    break;

                default:
                    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_tx_mgmt_frame:: uc_mgmt_type[%d] notsupport!}", uc_mgmt_type);
                    break;
            }
        }
    }
}


oal_void dmac_btcoex_tx_vip_frame(mac_vap_stru *pst_mac_vap, oal_dlist_head_stru *pst_tx_dscr_list_hdr)
{
    oal_dlist_head_stru     *pst_dscr_entry;
    hal_to_dmac_chip_stru   *pst_hal_chip;
    hal_tx_dscr_stru        *pst_dscr_temp;
    oal_uint8                uc_data_type;
    bt_status_stru          *pst_bt_status;
    mac_tx_ctl_stru         *pst_cb;

    pst_hal_chip   = DMAC_VAP_GET_HAL_CHIP(pst_mac_vap);
    if (OAL_PTR_NULL == pst_hal_chip)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_tx_vip_frame:: DMAC_VAP_GET_HAL_CHIP null}");
        return;
    }

    /* EAPOL֡��DHCP��ARP֡�Ĺ��汣�� */
    pst_bt_status = &(pst_hal_chip->st_btcoex_btble_status.un_bt_status.st_bt_status);
    if (!pst_bt_status->bit_bt_on
#ifdef _PRE_WLAN_FEATURE_LTECOEX
        && !pst_hal_chip->ul_lte_coex_status
#endif
        )
    {
        return;
    }

    OAL_DLIST_SEARCH_FOR_EACH(pst_dscr_entry, pst_tx_dscr_list_hdr)
    {
        pst_dscr_temp = (hal_tx_dscr_stru *)OAL_DLIST_GET_ENTRY(pst_dscr_entry, hal_tx_dscr_stru, st_entry);
        pst_cb = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_dscr_temp->pst_skb_start_addr);
        if (WLAN_CB_FRAME_TYPE_DATA != MAC_GET_CB_FRAME_TYPE(pst_cb))
        {
            continue;
        }

        uc_data_type = MAC_GET_CB_FRAME_SUBTYPE(pst_cb);
        if (uc_data_type > MAC_DATA_ARP_REQ)
        {
            continue;
        }

        /* STAģʽ */
        if(WLAN_VAP_MODE_BSS_STA == pst_mac_vap->en_vap_mode)
        {
            switch (uc_data_type)
            {
                case MAC_DATA_DHCP:
                    /* ��Ҫ�ȴ��Զ���Ӧ֡ */
                    dmac_btcoex_set_wlan_priority(pst_mac_vap, OAL_TRUE, BTCOEX_PRIO_TIMEOUT_100MS);
                    break;

                case MAC_DATA_ARP_REQ:
                    /* ��Ҫ�ȴ��Զ���Ӧ֡ */
                    dmac_btcoex_set_wlan_priority(pst_mac_vap, OAL_TRUE, BTCOEX_PRIO_TIMEOUT_60MS);
                    break;

                case MAC_DATA_ARP_RSP:
                    /* arp rsp֡������û����Ӧ֡�����ͣ���ʱ��֤�����������ɣ����ù���֡���ͱ������� */
                    dmac_btcoex_set_mgmt_priority(pst_mac_vap, BTCOEX_PRIO_TIMEOUT_10MS);
                    break;

                case MAC_DATA_EAPOL:
                    /* Ϊ���p2p��Գɹ���/���γɹ��ʣ��ڷ�BT�绰�����£�����EAPOL֡�շ� */
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV)
                    if (!(pst_bt_status->bit_bt_sco))
                    {
                        dmac_btcoex_set_occupied_period(MAC_GET_DMAC_VAP(pst_mac_vap), OCCUPIED_PERIOD);
                    }
                    else
#endif
                    {
                        /* ��Ҫ�ȴ��Զ���Ӧ֡, ����eapol֡��������,����dhcp�ἰʱ����!!!! */
                        dmac_btcoex_set_wlan_priority(pst_mac_vap, OAL_TRUE, BTCOEX_PRIO_TIMEOUT_100MS);
                    }
                    break;

                default:
                    break;
            }
        }
        else if(WLAN_VAP_MODE_BSS_AP == pst_mac_vap->en_vap_mode)
        {
           switch (uc_data_type)
            {
                case MAC_DATA_DHCP:
                    dmac_btcoex_set_mgmt_priority(pst_mac_vap, BTCOEX_PRIO_TIMEOUT_10MS);
                    break;

                case MAC_DATA_ARP_REQ:
                    dmac_btcoex_set_wlan_priority(pst_mac_vap, OAL_TRUE, BTCOEX_PRIO_TIMEOUT_60MS);
                    break;

                case MAC_DATA_ARP_RSP:
                    /* arp rsp֡������û����Ӧ֡�����ͣ���ʱ��֤�����������ɣ����ù���֡���ͱ������� */
                    dmac_btcoex_set_mgmt_priority(pst_mac_vap, BTCOEX_PRIO_TIMEOUT_10MS);
                    break;

                case MAC_DATA_EAPOL:
                    /* Ϊ���p2p��Գɹ���/���γɹ��ʣ��ڷ�BT�绰�����£�����EAPOL֡�շ� */
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV)
                    if (!(pst_bt_status->bit_bt_sco))
                    {
                        dmac_btcoex_set_occupied_period(MAC_GET_DMAC_VAP(pst_mac_vap), OCCUPIED_PERIOD);
                    }
                    else
#endif
                    {
                        /* ��Ҫ�ȴ��Զ���Ӧ֡ */
                        dmac_btcoex_set_wlan_priority(pst_mac_vap, OAL_TRUE, BTCOEX_PRIO_TIMEOUT_100MS);
                    }
                    break;

                default:
                    break;
            }
        /* ����ؼ�֡����ʱ��̫�̣�����ؼ�֡����״̬֪ͨ��bt���岻�� */
        }
    }
}


oal_void dmac_btcoex_sco_rx_rate_process (mac_vap_stru *pst_vap)
{
    dmac_vap_btcoex_rx_statistics_stru *pst_dmac_vap_btcoex_rx_statistics;
    dmac_user_btcoex_rx_info_stru      *pst_dmac_user_btcoex_sco_rx_info;
    dmac_vap_stru                      *pst_dmac_vap;
    dmac_user_stru                     *pst_dmac_user;

    if (OAL_FALSE == MAC_BTCOEX_CHECK_VALID_STA(pst_vap))
    {
        return;
    }

    pst_dmac_vap = MAC_GET_DMAC_VAP(pst_vap);

    pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user(pst_vap->us_assoc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dmac_user))
    {
        OAM_WARNING_LOG1(pst_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_sco_rx_rate_process::pst_dmac_user[%d] null.}",
            pst_vap->us_assoc_vap_id);
        return;
    }

    pst_dmac_vap_btcoex_rx_statistics = &(pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_rx_statistics);
    pst_dmac_user_btcoex_sco_rx_info  = &(pst_dmac_user->st_dmac_user_btcoex_stru.st_dmac_user_btcoex_sco_rx_info);

    /* �ۼӽ������� */
    if (pst_dmac_vap_btcoex_rx_statistics->en_sco_rx_rate_statistics_flag)
    {
        if (0 != pst_dmac_user->ul_rx_rate)
        {
            pst_dmac_user_btcoex_sco_rx_info->ull_rx_rate_mbps += pst_dmac_user->ul_rx_rate;
            pst_dmac_user_btcoex_sco_rx_info->us_rx_rate_stat_count++;
        }
    }
    else
    {
        return;
    }

}


oal_void dmac_btcoex_linkloss_init(dmac_vap_stru *pst_dmac_vap)
{
    oal_uint8                      uc_loop_idx;
    dmac_vap_btcoex_occupied_stru *pst_dmac_vap_btcoex_occupied = &(pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_occupied);

    g_us_occupied_point[0] = GET_LINKLOSS_INT_THRESHOLD(pst_dmac_vap, WLAN_LINKLOSS_MODE_BT) >> 3;

    for (uc_loop_idx = 0; uc_loop_idx < BTCOEX_LINKLOSS_OCCUPIED_NUMBER - 1; uc_loop_idx++)
    {
        g_us_occupied_point[uc_loop_idx + 1] = g_us_occupied_point[uc_loop_idx] + g_us_occupied_point[0];
    }

    pst_dmac_vap_btcoex_occupied->uc_linkloss_index = 1;
    pst_dmac_vap_btcoex_occupied->uc_linkloss_occupied_times = 0;
}


oal_void dmac_btcoex_linkloss_update_threshold(dmac_vap_stru *pst_dmac_vap)
{
    oal_uint8                      uc_loop_idx;
    dmac_vap_btcoex_occupied_stru *pst_dmac_vap_btcoex_occupied = &(pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_occupied);

    g_us_occupied_point[0] = GET_CURRENT_LINKLOSS_THRESHOLD(pst_dmac_vap) >> 3;

    for (uc_loop_idx = 0; uc_loop_idx < BTCOEX_LINKLOSS_OCCUPIED_NUMBER - 1; uc_loop_idx++)
    {
        g_us_occupied_point[uc_loop_idx + 1] = g_us_occupied_point[uc_loop_idx] + g_us_occupied_point[0];
    }

    pst_dmac_vap_btcoex_occupied->uc_linkloss_index = 1;
    pst_dmac_vap_btcoex_occupied->uc_linkloss_occupied_times = 0;
}


oal_void dmac_btcoex_beacon_occupied_handler(hal_to_dmac_device_stru *pst_hal_device, dmac_vap_stru *pst_dmac_vap)
{
    oal_uint32                     ul_beacon_count_new;

    dmac_vap_btcoex_occupied_stru *pst_dmac_vap_btcoex_occupied = &(pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_occupied);

    if(OAL_FALSE == pst_dmac_vap->pst_hal_chip->st_btcoex_btble_status.un_bt_status.st_bt_status.bit_bt_on)
    {
        return;
    }

    /* ���ȷ�ϲ�Ϊ�� */
    hal_btcoex_update_ap_beacon_count(pst_hal_device, &ul_beacon_count_new);
    pst_dmac_vap_btcoex_occupied = &(pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_occupied);

    /* ÿ5��beacon����ʧ���ϱ�һ��BEACON MISS�жϣ������ڷ�����5��beacon�󣬼�����1 */
    if (ul_beacon_count_new - pst_dmac_vap_btcoex_occupied->ul_ap_beacon_count >= 5)
    {
        pst_dmac_vap_btcoex_occupied->ul_ap_beacon_count += 5;
        pst_dmac_vap_btcoex_occupied->uc_beacon_miss_cnt = OAL_SUB(pst_dmac_vap_btcoex_occupied->uc_beacon_miss_cnt, 1);

        /* ������ڴ���5�����������ܳ��ֹ�TBTT�ж϶�ʧ��ʵ��ȷ�з���beacon miss�����޷��������� */
        if (ul_beacon_count_new > pst_dmac_vap_btcoex_occupied->ul_ap_beacon_count)
        {
            OAM_WARNING_LOG2(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_COEX,
                "{dmac_btcoex_beacon_occupied_handler::TBTT interrupt miss may be happened: new beacon cnt [%d] & record beacon cnt [%d]!}",
                ul_beacon_count_new, pst_dmac_vap_btcoex_occupied->ul_ap_beacon_count);
        }
    }
    else if (ul_beacon_count_new < pst_dmac_vap_btcoex_occupied->ul_ap_beacon_count)
    {
        /* ��ֹӲ����beacon������ת */
        pst_dmac_vap_btcoex_occupied->ul_ap_beacon_count = ul_beacon_count_new;
    }

    if (DMAC_VAP_GET_BTCOEX_STATUS(pst_dmac_vap)->un_bt_status.st_bt_status.bit_bt_page)
    {
        /* �ﵽ30��beacon miss������occupied */
        if (pst_dmac_vap_btcoex_occupied->uc_beacon_miss_cnt > 5)
        {
            dmac_btcoex_set_occupied_period(pst_dmac_vap, 10000);// 10ms
        }
    }
}


oal_void dmac_btcoex_beacon_miss_handler(hal_to_dmac_device_stru  *pst_hal_device)
{
    dmac_vap_stru    *pst_dmac_vap = OAL_PTR_NULL;
    oal_uint8         uc_beacon_miss_cnt;
    oal_uint8         uc_vap_idx;
    oal_uint8         uc_valid_vap_num;
    oal_uint8         auc_mac_vap_id[WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE];
    mac_vap_stru     *pst_mac_vap[WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE] = {OAL_PTR_NULL};

    /* valid��vap�豸������Ӧ���� */
    uc_valid_vap_num = dmac_btcoex_find_all_valid_ap_per_device(pst_hal_device, auc_mac_vap_id);

    for (uc_vap_idx = 0; uc_vap_idx < uc_valid_vap_num; uc_vap_idx++)
    {
        pst_mac_vap[uc_vap_idx]  = (mac_vap_stru *)mac_res_get_mac_vap(auc_mac_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_mac_vap[uc_vap_idx])
        {
            OAM_ERROR_LOG1(auc_mac_vap_id[uc_vap_idx], OAM_SF_COEX, "{dmac_btcoex_beacon_miss_handler::pst_mac_vap[%d] IS NULL.}", auc_mac_vap_id[uc_vap_idx]);
            return;
        }

        pst_dmac_vap = MAC_GET_DMAC_VAP(pst_mac_vap[uc_vap_idx]);
        uc_beacon_miss_cnt = pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_occupied.uc_beacon_miss_cnt;
        /* ÿ�ϱ�5��Beacon miss(��Ӧ30��beacon֡��ʧ)�����һ��ȫ���Ĵ��� */
        uc_beacon_miss_cnt++;
        if (0 == (uc_beacon_miss_cnt % 4))
        {
            hal_dft_report_all_reg_state(pst_hal_device);
        }

        OAM_WARNING_LOG1(auc_mac_vap_id[uc_vap_idx], OAM_SF_COEX,
            "{dmac_btcoex_beacon_miss_handler:: beacon miss cnt: %d}\r\n", uc_beacon_miss_cnt);
    }
}


oal_void dmac_btcoex_linkloss_occupied_process(
      hal_to_dmac_chip_stru *pst_hal_chip, hal_to_dmac_device_stru *pst_hal_device, dmac_vap_stru *pst_dmac_vap)
{
    dmac_vap_linkloss_stru        *pst_linkloss_info;
    dmac_vap_btcoex_occupied_stru *pst_dmac_vap_btcoex_occupied;
    oal_uint16                     us_linkloss_threshold;
    oal_uint16                     us_linkloss;
    oal_uint16                     us_linkloss_tmp;
    oal_uint8                      uc_linkloss_times;

    if (OAL_FALSE == pst_hal_chip->st_btcoex_btble_status.un_bt_status.st_bt_status.bit_bt_on)
    {
        return;
    }

    pst_linkloss_info = &(pst_dmac_vap->st_linkloss_info);
    pst_dmac_vap_btcoex_occupied = &(pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_occupied);
    us_linkloss_threshold = GET_CURRENT_LINKLOSS_THRESHOLD(pst_dmac_vap);
    us_linkloss = GET_CURRENT_LINKLOSS_CNT(pst_dmac_vap);
    uc_linkloss_times = pst_linkloss_info->uc_linkloss_times;

    if (0 == us_linkloss_threshold)
    {
        return;
    }
    /* linkloss ����1/4����ֵ���д��� */
    if (us_linkloss < (us_linkloss_threshold >> 2))
    {
        return;
    }

    /* ����ֵΪ�쳣ֵ����beacon���ںܴ� */
    if (us_linkloss_threshold <= WLAN_LINKLOSS_MIN_THRESHOLD)
    {
        dmac_btcoex_set_occupied_period(pst_dmac_vap, COEX_LINKLOSS_OCCUP_PERIOD);

        OAM_WARNING_LOG3(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_COEX,
                "{dmac_btcoex_linkloss_occupied_process::occupied time == [%d], linkloss == [%d], threshold == [%d].}",
                COEX_LINKLOSS_OCCUP_PERIOD, us_linkloss, us_linkloss_threshold);
        return;
    }

    us_linkloss_tmp = g_us_occupied_point[pst_dmac_vap_btcoex_occupied->uc_linkloss_index] + pst_dmac_vap_btcoex_occupied->uc_occupied_times;

    if (us_linkloss > us_linkloss_tmp)
    {
        pst_dmac_vap_btcoex_occupied->uc_occupied_times = 0;
        pst_dmac_vap_btcoex_occupied->uc_linkloss_index++;
    }
    /* ������linkloss�׶�����occupied 6-7���׶�, ÿ���׶�Ϊ300ms��ÿ100ms��һ��20ms, ������3�� */
    else if (us_linkloss == us_linkloss_tmp)
    {
        pst_dmac_vap_btcoex_occupied->uc_occupied_times += uc_linkloss_times;
        if (pst_dmac_vap_btcoex_occupied->uc_occupied_times > (uc_linkloss_times << 1))
        {
            pst_dmac_vap_btcoex_occupied->uc_occupied_times = 0;
            pst_dmac_vap_btcoex_occupied->uc_linkloss_index++;
            if (pst_dmac_vap_btcoex_occupied->uc_linkloss_index > BTCOEX_LINKLOSS_OCCUPIED_NUMBER - 1)
            {
                pst_dmac_vap_btcoex_occupied->uc_linkloss_index = 1;
            }
        }

        dmac_btcoex_set_occupied_period(pst_dmac_vap, COEX_LINKLOSS_OCCUP_PERIOD);

        OAM_WARNING_LOG3(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_COEX,
                "{dmac_btcoex_linkloss_occupied_process::occupied time == [%d], linkloss == [%d], threshold == [%d].}",
                COEX_LINKLOSS_OCCUP_PERIOD, us_linkloss, us_linkloss_threshold);

    }
    else
    {
    }
}


oal_void dmac_btcoex_rx_rate_process_check(mac_vap_stru *pst_mac_vap,
        oal_uint8 uc_frame_subtype, oal_uint8 uc_data_type, oal_bool_enum_uint8 en_ampdu)
{
#if(_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)
    /* ҵ������֡����ͳ�ƣ�qos null data��Ĭ�ϲ���������ʷ��ͣ�������ͳ�ƣ����ҵ������֡׼ȷ�� */
    if( WLAN_NULL_FRAME != uc_frame_subtype && WLAN_QOS_NULL_FRAME != uc_frame_subtype && MAC_DATA_ARP_REQ < uc_data_type)
#endif
    {
        dmac_btcoex_sco_rx_rate_process(pst_mac_vap);

        if(OAL_TRUE == en_ampdu)
        {
            dmac_btcoex_lower_rate_process(pst_mac_vap);
        }
    }
}


oal_void dmac_btcoex_roam_succ_handler(hal_to_dmac_device_stru *pst_hal_device, mac_vap_stru *pst_mac_vap)
{
#if(_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)
    hal_to_dmac_chip_stru              *pst_hal_chip;
    dmac_user_stru                     *pst_dmac_user;
    dmac_user_btcoex_delba_stru        *pst_dmac_user_btcoex_delba;

    /* ����һ����legacy sta��ֱ�Ӱ������·�ʽ��ȡ�û� */
    pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user(pst_mac_vap->us_assoc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dmac_user))
    {
        OAM_ERROR_LOG1(0, OAM_SF_COEX, "{dmac_btcoex_roam_succ_handler::pst_dmac_user[%d] null.}", pst_mac_vap->us_assoc_vap_id);
        return;
    }

    hal_chip_get_chip(pst_hal_device->uc_chip_id, &pst_hal_chip);

    /* ���γɹ�֮�󣬴���2G,��Ҫ����ҵ������ */
    if(OAL_TRUE == MAC_BTCOEX_CHECK_VALID_STA(pst_mac_vap))
    {
        dmac_config_btcoex_assoc_state_syn(pst_mac_vap, &pst_dmac_user->st_user_base_info);

        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_roam_succ_handler::wifi is on 2G, ba size to bt control.}");
    }
    else /* ����5G,��Ҫɾ�����ã�Ĭ��5G������Ϣɾ�� */
    {
        dmac_config_btcoex_disassoc_state_syn(pst_mac_vap);

        /* ��btҵ����Ҫ�ָ�wifi�� */
        if(pst_hal_chip->st_btcoex_btble_status.un_bt_status.st_bt_status.bit_bt_on)
        {
            pst_dmac_user_btcoex_delba = &(pst_dmac_user->st_dmac_user_btcoex_stru.st_dmac_user_btcoex_delba);

            pst_dmac_user_btcoex_delba->uc_ba_size = 0;
            dmac_btcoex_delba_trigger(pst_mac_vap, OAL_FALSE, pst_dmac_user_btcoex_delba);

            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_roam_succ_handler::wifi is on 5G, ba size to default.}");
        }
    }
#endif
}

#endif /* end of _PRE_WLAN_FEATURE_BTCOEX */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

