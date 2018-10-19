


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 ͷ�ļ�����
*****************************************************************************/
#include "wlan_spec.h"
#include "wlan_types.h"

#include "oal_ext_if.h"
#include "oam_ext_if.h"
#include "oam_main.h"

#if (_PRE_OS_VERSION_RAW == _PRE_OS_VERSION)
#include "pm_extern.h"
#endif

#include "hal_ext_if.h"
#include "mac_frame.h"
#include "mac_ie.h"
#include "mac_vap.h"
#include "mac_frame.h"


#include "dmac_main.h"
#include "dmac_device.h"
#include "dmac_beacon.h"
#include "dmac_chan_mgmt.h"
#include "dmac_psm_ap.h"
#include "dmac_mgmt_sta.h"
#include "dmac_mgmt_ap.h"
#include "dmac_mgmt_bss_comm.h"
#include "dmac_mgmt_classifier.h"
#include "dmac_device.h"
#include "dmac_scan.h"
#ifdef _PRE_WLAN_CHIP_TEST
#include "dmac_test_main.h"
#include "dmac_lpm_test.h"
#endif
#ifdef _PRE_WLAN_DFT_STAT
#include "mac_device.h"
#include "dmac_dft.h"
#endif
#ifdef _PRE_WLAN_FEATURE_STA_PM
#include "dmac_psm_sta.h"
#include "dmac_sta_pm.h"
#endif
#ifdef _PRE_WLAN_FEATURE_AP_PM
#include "dmac_ap_pm.h"
#endif
#ifdef _PRE_WLAN_FEATURE_P2P
#include "dmac_p2p.h"
#endif
#ifdef _PRE_WLAN_FEATURE_BTCOEX
#include "dmac_btcoex.h"
#endif
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#include "dmac_reset.h"
#endif
#include "dmac_config.h"
#include "dmac_tx_bss_comm.h"
#ifdef _PRE_WLAN_FEATURE_11K
#include "dmac_11k.h"
#endif
#include "dmac_vap.h"
#ifdef _PRE_WLAN_FEATURE_SMPS
#include "dmac_smps.h"
#endif
#ifdef _PRE_WLAN_FEATURE_OPMODE_NOTIFY
#include "dmac_opmode.h"
#endif
#ifdef _PRE_WLAN_FEATURE_M2S
#include "dmac_m2s.h"
#endif

#ifdef _PRE_WLAN_FEATURE_PACKET_CAPTURE
#include "dmac_pkt_capture.h"
#endif
#include "dmac_csa_sta.h"

#include "dmac_rx_data.h"
#ifdef _PRE_PM_DYN_SET_TBTT_OFFSET
#include "hal_pm.h"
#endif
#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_BEACON_C

/*****************************************************************************
  2 ȫ�ֱ�������
*****************************************************************************/
#ifdef _PRE_WLAN_CHIP_TEST
dmac_beacon_test_stru g_beacon_offload_test; /*�����ڲ���*/
#endif

/* �����������޳�ʼֵȫ�ֱ��� */
oal_uint8 g_auc_int_linkloss_threshold[WLAN_LINKLOSS_MODE_BUTT] = {80, 80, 40};

/*****************************************************************************
  3 ����ʵ��
*****************************************************************************/



OAL_STATIC oal_void dmac_set_tx_chain_mask(dmac_vap_stru * pst_dmac_vap, oal_uint32 *pul_tx_chain_mask)
{
    /* �ж��Ƿ���mimo״̬ */
    if(WLAN_TX_CHAIN_DOUBLE == pst_dmac_vap->pst_hal_device->st_cfg_cap_info.uc_phy2dscr_chain)
    {
    /* �ж��Ƿ�����˫·�������� */
        if(DMAC_BEACON_TX_POLICY_SWITCH == pst_dmac_vap->en_beacon_tx_policy)
        {
            /* ��һ��beacon֡��ͨ��0���ͣ��ڶ���beacon֡��ͨ��1���� */
            if(pst_dmac_vap->uc_beacon_idx == DMAC_VAP_BEACON_BUFFER1)
            {
                *pul_tx_chain_mask = WLAN_TX_CHAIN_ZERO;
            }
            else
            {
                *pul_tx_chain_mask = WLAN_TX_CHAIN_ONE;
            }
        }
        /* ˫ͨ������ */
        else if(DMAC_BEACON_TX_POLICY_DOUBLE == pst_dmac_vap->en_beacon_tx_policy)
        {
            if(WLAN_BAND_2G == pst_dmac_vap->st_vap_base_info.st_channel.en_band)
            {//2gģʽ��11bֻ֧�ֵ�ͨ�����ͣ�ǿ�Ƶ�ͨ��
                *pul_tx_chain_mask = pst_dmac_vap->pst_hal_device->st_cfg_cap_info.uc_single_tx_chain;
            }
            else
            {
                *pul_tx_chain_mask = WLAN_TX_CHAIN_DOUBLE;
            }
        }
        /* ��ͨ������ */
        else
        {
            *pul_tx_chain_mask = pst_dmac_vap->pst_hal_device->st_cfg_cap_info.uc_single_tx_chain;
        }
    }
    else
    {
        *pul_tx_chain_mask = pst_dmac_vap->pst_hal_device->st_cfg_cap_info.uc_phy2dscr_chain;
    }
}


oal_uint32  dmac_beacon_alloc(dmac_vap_stru *pst_dmac_vap)
{
    oal_uint8                  *pauc_beacon_buf[DMAC_VAP_BEACON_BUFFER_BUTT];
    oal_uint16                  us_beacon_len = 0;
    hal_beacon_tx_params_stru   st_beacon_tx_params;
    hal_tx_txop_alg_stru       *pst_tx_param;
    wlan_channel_band_enum_uint8 en_band;

    /*
     * BSS�����еĶ�̬RESTART��������Ҫ����beacon������������
     *  -ACS��ʼ�ŵ�
     *  -AP/STA�ŵ�����
     *  -proxysta�ŵ�����
     */
    if((OAL_PTR_NULL != pst_dmac_vap->pauc_beacon_buffer[0]) && (OAL_PTR_NULL != pst_dmac_vap->pauc_beacon_buffer[1]))
    {
        pauc_beacon_buf[0] = pst_dmac_vap->pauc_beacon_buffer[0];
        pauc_beacon_buf[1] = pst_dmac_vap->pauc_beacon_buffer[1];
        OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_WIFI_BEACON, "{dmac_beacon_alloc::vap beacon buf has been alloc.}\r\n");
    }
    else
    {
        /* �����һ��beacon֡�ڴ� */
        pauc_beacon_buf[0] = OAL_MEM_ALLOC(OAL_MEM_POOL_ID_SHARED_MGMT_PKT, WLAN_MEM_SHARED_MGMT_PKT_SIZE1, OAL_TRUE);
        if (OAL_PTR_NULL == pauc_beacon_buf[0])
        {
            OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_WIFI_BEACON, "{dmac_beacon_alloc::pauc_beacon_buf[0] null.}");

            return OAL_ERR_CODE_ALLOC_MEM_FAIL;
        }

        /* ����ڶ���beacon֡�ڴ� */
        pauc_beacon_buf[1] = OAL_MEM_ALLOC(OAL_MEM_POOL_ID_SHARED_MGMT_PKT, WLAN_MEM_SHARED_MGMT_PKT_SIZE1, OAL_TRUE);
        if (OAL_PTR_NULL == pauc_beacon_buf[1])
        {
            OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_WIFI_BEACON, "{dmac_beacon_alloc::pauc_beacon_buf[1] null.}");

            /* �ͷ���������ڴ� */
            OAL_MEM_FREE(pauc_beacon_buf[0], OAL_TRUE);
            pauc_beacon_buf[0] = OAL_PTR_NULL;

            return OAL_ERR_CODE_ALLOC_MEM_FAIL;
        }
    }
    /* ��һ�γ�ʼ��DTIM count,�淶��ʹ���period -1��ʼ */
    dmac_init_dtim_count_ap(pst_dmac_vap);

    /* ��ʼbeacon��֡ */
    dmac_encap_beacon(pst_dmac_vap, pauc_beacon_buf[0], &us_beacon_len);
    oal_memcopy(pauc_beacon_buf[1], pauc_beacon_buf[0], us_beacon_len);

    /* ��beacon֡�ҵ�vap�� */
    pst_dmac_vap->pauc_beacon_buffer[0] = pauc_beacon_buf[0];
    pst_dmac_vap->pauc_beacon_buffer[1] = pauc_beacon_buf[1];

    pst_dmac_vap->uc_beacon_idx = 0;    /* ��ʼ������Ӳ����beacon֡���� */
    pst_dmac_vap->us_beacon_len = us_beacon_len;

    /* ��д����Beacon֡���� */
    en_band  = pst_dmac_vap->st_vap_base_info.st_channel.en_band;
    pst_tx_param = &pst_dmac_vap->ast_tx_mgmt_bmcast[en_band];
    pst_tx_param->ast_per_rate[0].rate_bit_stru.bit_reserve = 0;

    st_beacon_tx_params.ul_pkt_ptr   = (oal_uint32)((pauc_beacon_buf[0]));
    st_beacon_tx_params.us_pkt_len   = us_beacon_len;
    st_beacon_tx_params.pst_tx_param = pst_tx_param;

    /* ����beacon period */
    hal_vap_set_machw_beacon_period(pst_dmac_vap->pst_hal_vap, (oal_uint16)mac_mib_get_BeaconPeriod(&pst_dmac_vap->st_vap_base_info));

    dmac_set_tx_chain_mask(pst_dmac_vap, &st_beacon_tx_params.ul_tx_chain_mask);

    /* HAL����Beacon֡�ӿ� */
    hal_vap_send_beacon_pkt(pst_dmac_vap->pst_hal_vap, &st_beacon_tx_params);

    /* DBACʹ�ܺ���DBAC��������TSF */
#ifdef _PRE_WLAN_FEATURE_DBAC
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
     /* 1102 DBAC todo DBACδ������ʱ��ͨ��enable��Ϣ�жϣ�����GO���ᷢ��Beacon֡*/
    {
        mac_device_stru *pst_device = mac_res_get_dev((oal_uint32)pst_dmac_vap->st_vap_base_info.uc_device_id);

        if(pst_device->en_dbac_enabled)
        {
            /* force PAUSE when DBAC enabled */
            pst_dmac_vap->st_vap_base_info.en_vap_state = MAC_VAP_STATE_PAUSE;
            return OAL_SUCC;
        }
    }
#endif
#endif

    /* ����tsf */
    hal_enable_tsf_tbtt(pst_dmac_vap->pst_hal_vap, OAL_FALSE);

    return OAL_SUCC;
}


oal_uint32  dmac_encap_beacon(
                dmac_vap_stru               *pst_dmac_vap,
                oal_uint8                   *puc_beacon_buf,
                oal_uint16                  *pus_beacon_len)
{

    oal_uint8        uc_ie_len;
    oal_uint8       *puc_bf_origin = puc_beacon_buf;
    mac_device_stru *pst_mac_device;
    mac_vap_stru    *pst_mac_vap;
    oal_uint16       us_app_ie_len = 0;
    oal_uint8        uc_dsss_channel_num;

    if (puc_beacon_buf == OAL_PTR_NULL)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_mac_device = mac_res_get_dev(pst_dmac_vap->st_vap_base_info.uc_device_id);

    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_WIFI_BEACON, "{dmac_encap_beacon::pst_mac_device null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_vap = &(pst_dmac_vap->st_vap_base_info);

    /*************************************************************************/
    /*                        Management Frame Format                        */
    /* --------------------------------------------------------------------  */
    /* |Frame Control|Duration|DA|SA|BSSID|Sequence Control|Frame Body|FCS|  */
    /* --------------------------------------------------------------------  */
    /* | 2           |2       |6 |6 |6    |2               |0 - 2312  |4  |  */
    /* --------------------------------------------------------------------  */
    /*                                                                       */
    /*************************************************************************/
    /* ����Frame Control�ֶ� */
    mac_hdr_set_frame_control(puc_beacon_buf, WLAN_PROTOCOL_VERSION| WLAN_FC0_TYPE_MGT | WLAN_FC0_SUBTYPE_BEACON);

    /* ����durationΪ0����Ӳ���� */
    mac_hdr_set_duration(puc_beacon_buf, 0);

    /* ����address1, DA��һ���㲥��ַ */
    oal_memcopy(puc_beacon_buf + WLAN_HDR_ADDR1_OFFSET, BROADCAST_MACADDR, WLAN_MAC_ADDR_LEN);

    /* ����address2��SA��mac��ַ */
    oal_memcopy(puc_beacon_buf + WLAN_HDR_ADDR2_OFFSET,
                mac_mib_get_StationID(&pst_dmac_vap->st_vap_base_info),
                WLAN_MAC_ADDR_LEN);

    /* ����address3, BSSID */
    oal_memcopy(puc_beacon_buf + WLAN_HDR_ADDR3_OFFSET,
                pst_dmac_vap->st_vap_base_info.auc_bssid,
                WLAN_MAC_ADDR_LEN);

    /* ���÷�Ƭ���Ϊ0��beacon֡û��sequence number */
    mac_hdr_set_fragment_number(puc_beacon_buf, 0);

    /*************************************************************************/
    /*                Set the contents of the frame body                     */
    /*************************************************************************/

    /*************************************************************************/
    /*                       Beacon Frame - Frame Body                       */
    /* ----------------------------------------------------------------------*/
    /* |Timestamp|BcnInt|CapInfo|SSID|SupRates|DSParamSet|TIM  |CountryElem |*/
    /* ----------------------------------------------------------------------*/
    /* |8        |2     |2      |2-34|3-10    |3         |6-256|8-256       |*/
    /* ----------------------------------------------------------------------*/
    /* |PowerConstraint |Quiet|TPC Report|ERP |RSN  |WMM |Extended Sup Rates|*/
    /* ----------------------------------------------------------------------*/
    /* |3               |8    |4         |3   |4-255|26  | 3-257            |*/
    /* ----------------------------------------------------------------------*/
    /* |BSS Load |HT Capabilities |HT Operation |Overlapping BSS Scan       |*/
    /* ----------------------------------------------------------------------*/
    /* |7        |28              |24           |16                         |*/
    /* ----------------------------------------------------------------------*/
    /* |Extended Capabilities |                                              */
    /* ----------------------------------------------------------------------*/
    /* |3-8                   |                                              */
    /*************************************************************************/
    /* timestamp��Ӳ������ */
    puc_beacon_buf += MAC_80211_FRAME_LEN;
    OAL_MEMZERO(puc_beacon_buf, 8);

    /* Initialize index */
    puc_beacon_buf += MAC_TIME_STAMP_LEN;

    /* ����beacon interval */
    mac_set_beacon_interval_field(pst_mac_vap, puc_beacon_buf);
    puc_beacon_buf += MAC_BEACON_INTERVAL_LEN;

    /* ����capability information */
    mac_set_cap_info_ap(pst_mac_vap, puc_beacon_buf);
    puc_beacon_buf += MAC_CAP_INFO_LEN;

    /* ����ssid element,���������ssid���صĻ�������ssid */
    mac_set_ssid_ie(pst_mac_vap, puc_beacon_buf, &uc_ie_len, WLAN_FC0_SUBTYPE_BEACON);
    puc_beacon_buf += uc_ie_len;

    /* ����֧�ֵ����ʼ� */
    mac_set_supported_rates_ie(pst_mac_vap, puc_beacon_buf, &uc_ie_len);
    puc_beacon_buf += uc_ie_len;

    /* ��ȡdsss ie�ڵ�channel num */
    uc_dsss_channel_num = dmac_get_dsss_ie_channel_num(pst_mac_vap);

    /* ����dsss������ */
    mac_set_dsss_params(pst_mac_vap, puc_beacon_buf, &uc_ie_len, uc_dsss_channel_num);
    puc_beacon_buf += uc_ie_len;

    /* ���tim��Ϣ */
    dmac_set_tim_ie(pst_dmac_vap, puc_beacon_buf, &uc_ie_len);
    puc_beacon_buf += uc_ie_len;

#ifdef _PRE_WLAN_FEATURE_11D
    /* ���country��Ϣ */
    mac_set_country_ie(pst_mac_vap, puc_beacon_buf, &uc_ie_len);
    puc_beacon_buf += uc_ie_len;
#endif

    /* ���power constraint��Ϣ */
    mac_set_pwrconstraint_ie(pst_mac_vap, puc_beacon_buf, &uc_ie_len);
    puc_beacon_buf += uc_ie_len;

    /* ���quiet��Ϣ */
    mac_set_quiet_ie(pst_mac_vap, puc_beacon_buf, MAC_QUIET_COUNT, MAC_QUIET_PERIOD,
                      MAC_QUIET_DURATION, MAC_QUIET_OFFSET, &uc_ie_len);
    puc_beacon_buf += uc_ie_len;

    /* ���TPC Report��Ϣ */
    mac_set_tpc_report_ie(pst_mac_vap, puc_beacon_buf, &uc_ie_len);
    puc_beacon_buf += uc_ie_len;

#if defined(_PRE_WLAN_FEATURE_11K_EXTERN) || defined(_PRE_WLAN_FEATURE_11KV_INTERFACE)
    mac_set_rrm_enabled_cap_field((oal_void *)pst_mac_vap, puc_beacon_buf, &uc_ie_len);
    puc_beacon_buf += uc_ie_len;
#endif

    /* ���erp��Ϣ */
    mac_set_erp_ie(pst_mac_vap, puc_beacon_buf, &uc_ie_len);
    puc_beacon_buf += uc_ie_len;

    /* ���RSN ��ȫ�����Ϣ */
    mac_set_rsn_ie(pst_mac_vap, OAL_PTR_NULL, puc_beacon_buf, &uc_ie_len);
    puc_beacon_buf += uc_ie_len;

    /* ���WPA ��ȫ�����Ϣ */
    mac_set_wpa_ie(pst_mac_vap, puc_beacon_buf, &uc_ie_len);
    puc_beacon_buf += uc_ie_len;

    /* ���wmm��Ϣ */
    //mac_set_wmm_params_ie(pst_mac_vap, puc_beacon_buf, &uc_ie_len);
    mac_set_wmm_params_ie(pst_mac_vap, puc_beacon_buf, mac_mib_get_dot11QosOptionImplemented(pst_mac_vap), &uc_ie_len);
    puc_beacon_buf += uc_ie_len;

    /* ���extended supported rates��Ϣ */
    mac_set_exsup_rates_ie(pst_mac_vap, puc_beacon_buf, &uc_ie_len);
    puc_beacon_buf += uc_ie_len;

    /* ���bss load��Ϣ */
    mac_set_bssload_ie(pst_mac_vap, puc_beacon_buf, &uc_ie_len);
    puc_beacon_buf += uc_ie_len;

    /* ���HT Capabilities��Ϣ */
    mac_set_ht_capabilities_ie(pst_mac_vap, puc_beacon_buf, &uc_ie_len);
    puc_beacon_buf += uc_ie_len;

    /* ���HT Operation��Ϣ */
    mac_set_ht_opern_ie(pst_mac_vap, puc_beacon_buf, &uc_ie_len);
    puc_beacon_buf += uc_ie_len;

    /* ���Overlapping BSS Scan��Ϣ */
    mac_set_obss_scan_params(pst_mac_vap, puc_beacon_buf, &uc_ie_len);
    puc_beacon_buf += uc_ie_len;

    /* ���Extended Capabilities��Ϣ */
    mac_set_ext_capabilities_ie(pst_mac_vap, puc_beacon_buf, &uc_ie_len);
    puc_beacon_buf += uc_ie_len;

    /* ���vht cap��Ϣ */
    mac_set_vht_capabilities_ie(pst_mac_vap, puc_beacon_buf, &uc_ie_len);
    puc_beacon_buf += uc_ie_len;

    /* ���vht opern��Ϣ */
    mac_set_vht_opern_ie(pst_mac_vap, puc_beacon_buf, &uc_ie_len);
    puc_beacon_buf += uc_ie_len;

#ifdef _PRE_WLAN_FEATURE_OPMODE_NOTIFY
    mac_set_opmode_notify_ie((oal_void *)pst_mac_vap, puc_beacon_buf, &uc_ie_len);
    puc_beacon_buf += uc_ie_len;
#endif

    /* ���CSA IE */
    if (OAL_TRUE == pst_mac_vap->st_ch_switch_info.en_csa_present_in_bcn)
    {
        /*IE37*/
        mac_set_csa_ie(pst_mac_vap->st_ch_switch_info.en_csa_mode,pst_mac_vap->st_ch_switch_info.uc_announced_channel,
                   pst_mac_vap->st_ch_switch_info.uc_ch_switch_cnt,
                   puc_beacon_buf, &uc_ie_len);
        puc_beacon_buf += uc_ie_len;

        /*csa �������IE*/
        mac_set_csa_bw_ie((oal_void *)pst_mac_vap, puc_beacon_buf, &uc_ie_len);
        puc_beacon_buf += uc_ie_len;
    }

#ifdef _PRE_WLAN_CHIP_TEST
    if (1 == g_beacon_offload_test.uc_csa_in_beacon)
    {
        mac_set_csa_ie(g_beacon_offload_test.uc_opmode,g_beacon_offload_test.uc_announced_channel,
                   g_beacon_offload_test.uc_switch_cnt,
                   puc_beacon_buf, &uc_ie_len);
        puc_beacon_buf += uc_ie_len;
    }
#ifdef _PRE_WLAN_FEATURE_OPMODE_NOTIFY
    if (1 == g_beacon_offload_test.uc_opmode_in_beacon)
    {
        puc_beacon_buf[0] = MAC_EID_OPMODE_NOTIFY;
        puc_beacon_buf[1] = MAC_OPMODE_NOTIFY_LEN;
        puc_beacon_buf[2] = g_beacon_offload_test.uc_opmode;
        puc_beacon_buf += (MAC_IE_HDR_LEN + MAC_OPMODE_NOTIFY_LEN);
    }
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_TXBF
    mac_set_11ntxbf_vendor_ie(pst_mac_vap, puc_beacon_buf, &uc_ie_len);
    puc_beacon_buf += uc_ie_len;
#endif

    /* ���WPS��Ϣ */
    mac_add_app_ie((oal_void *)pst_mac_vap, puc_beacon_buf, &us_app_ie_len, OAL_APP_BEACON_IE);
    puc_beacon_buf += us_app_ie_len;

#ifdef _PRE_WLAN_FEATURE_HILINK
    /* ���OKC IE��Ϣ */
    mac_add_app_ie((oal_void *)pst_mac_vap, puc_beacon_buf, &us_app_ie_len, OAL_APP_OKC_BEACON_IE);
    puc_beacon_buf += us_app_ie_len;
#endif

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    mac_add_app_ie((oal_void *)pst_mac_vap, puc_beacon_buf, &us_app_ie_len, OAL_APP_VENDOR_IE);
    puc_beacon_buf += us_app_ie_len;
#endif

#ifdef _PRE_WLAN_FEATURE_P2P
    /* ���p2p noa Attribute*/
    if((IS_P2P_GO(&pst_dmac_vap->st_vap_base_info))&&
        (IS_P2P_NOA_ENABLED(pst_dmac_vap) || IS_P2P_OPPPS_ENABLED(pst_dmac_vap)))
    {
        mac_set_p2p_noa(pst_mac_vap, puc_beacon_buf, &uc_ie_len);
        puc_beacon_buf += uc_ie_len;
    }
#endif
#ifdef _PRE_WLAN_FEATURE_11R_AP
    mac_set_md_ie((oal_void *)pst_mac_vap, puc_beacon_buf, &uc_ie_len);
    puc_beacon_buf += uc_ie_len;
#endif

#ifdef _PRE_WLAN_NARROW_BAND
    if (pst_mac_vap->st_nb.en_open)
    {
        mac_set_nb_ie(puc_beacon_buf, &uc_ie_len);
        puc_beacon_buf += uc_ie_len;
    }
#endif
    /* multi-sta����������4��ַie */
#ifdef _PRE_WLAN_FEATURE_VIRTUAL_MULTI_STA
    mac_set_vender_4addr_ie((oal_void *)pst_mac_vap, puc_beacon_buf, &uc_ie_len);
    puc_beacon_buf += uc_ie_len;
#endif

    /*aput ���������������IE��Beacon֡β��*/
#if ((_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV)  || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV))
    if(OAL_TRUE == pst_mac_vap->st_ap_beacon_test_ie.en_include_test_ie)
    {
        if(pst_mac_vap->st_ap_beacon_test_ie.uc_test_ie_len > 0)
        {
            oal_memcopy(puc_beacon_buf,pst_mac_vap->st_ap_beacon_test_ie.uc_test_ie_info,pst_mac_vap->st_ap_beacon_test_ie.uc_test_ie_len);
            puc_beacon_buf += pst_mac_vap->st_ap_beacon_test_ie.uc_test_ie_len;
        }
    }
#endif
    *pus_beacon_len = (oal_uint16)(puc_beacon_buf - puc_bf_origin);

#ifdef _PRE_WLAN_CACHE_COHERENT_SUPPORT
    /* ȷ��beacon buf������ͬ�����µ�DDR */
    oal_dma_map_single(0, puc_bf_origin, *pus_beacon_len, OAL_TO_DEVICE);
#endif
    return OAL_SUCC;
}



OAL_STATIC oal_void  dmac_handle_tbtt_chan_mgmt_ap(dmac_vap_stru *pst_dmac_vap)
{
    mac_vap_stru              *pst_mac_vap = &(pst_dmac_vap->st_vap_base_info);
    mac_ch_switch_info_stru   *pst_csi = &(pst_mac_vap->st_ch_switch_info);

    /* AP׼�������ŵ��л� */
    if (WLAN_CH_SWITCH_STATUS_1 == pst_csi->en_ch_switch_status)
    {
        if (0 == pst_csi->uc_ch_switch_cnt)
        {
            oal_uint8   auc_null_addr[6] = {0};

            /* �ڵ�ǰ�ŵ�����beacon֡���� */
            hal_vap_beacon_suspend(pst_dmac_vap->pst_hal_vap);

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
            oal_mdelay(50);
#endif

            /* ��Beacon֡���Ƴ�Channel Switch Announcement IE */
            pst_csi->en_csa_present_in_bcn = OAL_FALSE;

            mac_vap_set_bssid(pst_mac_vap, auc_null_addr);
            dmac_chan_attempt_new_chan(pst_dmac_vap, pst_csi->uc_announced_channel, pst_csi->en_announced_bandwidth);
        }
        else
        {
            pst_csi->uc_ch_switch_cnt--;
        }
    }
    else if (0 == pst_dmac_vap->uc_dtim_count)
    {
        if (WLAN_BW_SWITCH_40_TO_20 == pst_csi->en_bw_switch_status || WLAN_BW_SWITCH_20_TO_40 == pst_csi->en_bw_switch_status)
        {
            if(MAC_VAP_STATE_UP == pst_mac_vap->en_vap_state)
            {
                pst_csi->en_bw_switch_status = WLAN_BW_SWITCH_DONE;

                /* ����VAP�µ�"����ģʽ"Ӧ��Ϊ WLAN_BAND_WIDTH_20MHz */
            #if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
                /* ����sta+ap���棬�����ŵ��仯������Ҫ�л���dbac����dbdc����Ҫʹ��multi�ӿ� */
                dmac_chan_multi_select_channel_mac(pst_mac_vap, pst_mac_vap->st_channel.uc_chan_number,  pst_mac_vap->st_channel.en_bandwidth);
                dmac_switch_complete_notify(pst_mac_vap, OAL_FALSE);
            #else
                dmac_chan_select_channel_mac(pst_mac_vap, pst_mac_vap->st_channel.uc_chan_number, pst_mac_vap->st_channel.en_bandwidth);
            #endif
            }
        }
    }
}
#ifndef HI110x_EDA

OAL_STATIC oal_void dmac_linkloss_update_mode(dmac_vap_stru *pst_dmac_vap, mac_device_stru *pst_mac_device)
{
    oal_bool_enum_uint8                en_bt_on = OAL_FALSE;
    oal_bool_enum_uint8                en_dbac_running = OAL_FALSE;
    wlan_linkloss_mode_enum_uint8      en_old_linkloss_mode = WLAN_LINKLOSS_MODE_BUTT;

    en_old_linkloss_mode = GET_CURRENT_LINKLOSS_MODE(pst_dmac_vap);

#ifdef _PRE_WLAN_FEATURE_BTCOEX
    en_bt_on = DMAC_VAP_GET_BTCOEX_STATUS(pst_dmac_vap)->un_bt_status.st_bt_status.bit_bt_on;
#endif
    en_dbac_running = mac_is_dbac_running(pst_mac_device);

    if (OAL_FALSE == en_bt_on)
    {
        /*��ͨ ����*/
        if (OAL_FALSE == en_dbac_running)
        {
            GET_CURRENT_LINKLOSS_MODE(pst_dmac_vap) = WLAN_LINKLOSS_MODE_NORMAL;
        }
        /*DBAC ����*/
        else
        {
            GET_CURRENT_LINKLOSS_MODE(pst_dmac_vap) = WLAN_LINKLOSS_MODE_DBAC;
        }
    }
     /*BT �������ȼ��ߣ�ֻҪBT���ˣ�����Ϊ��BT ����*/
    else
    {
        GET_CURRENT_LINKLOSS_MODE(pst_dmac_vap) = WLAN_LINKLOSS_MODE_BT;
    }

    if(en_old_linkloss_mode != GET_CURRENT_LINKLOSS_MODE(pst_dmac_vap))
    {
        OAM_WARNING_LOG4(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX,
                         "{dmac_linkloss_update_mode::mode changed!,old_mode[%d] --> new_mode[%d], linkloss_cnt[%d], linkloss_threshold[%d]}",
                         en_old_linkloss_mode,
                         GET_CURRENT_LINKLOSS_MODE(pst_dmac_vap),
                         GET_CURRENT_LINKLOSS_CNT(pst_dmac_vap),
                         GET_CURRENT_LINKLOSS_THRESHOLD(pst_dmac_vap));

    }

}


OAL_STATIC oal_void dmac_linkloss_send_probe_req(dmac_vap_stru *pst_dmac_vap, mac_device_stru *pst_mac_device)
{
    mac_vap_stru                      *pst_mac_vap;

    pst_mac_vap = &(pst_dmac_vap->st_vap_base_info);

    /* ɨ�衢�״2040���ŵ�ʱ��linkloss������������,���Բ����ⷢ�͵���̽��֡ */
    if ((OAL_TRUE == dmac_sta_csa_is_in_waiting(pst_mac_vap)) ||
        (MAC_SCAN_STATE_RUNNING == pst_mac_device->en_curr_scan_state))
    {
        return;
    }

    if((GET_CURRENT_LINKLOSS_CNT(pst_dmac_vap) >= (GET_CURRENT_LINKLOSS_INT_THRESHOLD(pst_dmac_vap) >> 1))
        &&(0 != pst_dmac_vap->st_linkloss_info.uc_allow_send_probe_req_cnt))
    {
        OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY,"dmac_linkloss_send_probe_req::now linkloss cnt is half of the threshold!");

        pst_dmac_vap->st_linkloss_info.uc_allow_send_probe_req_cnt--;
        /* ���������ţ�׼��probe req֡��ssid��Ϣ */
        dmac_scan_send_probe_req_frame(pst_dmac_vap, pst_mac_vap->auc_bssid, (oal_int8 *)(mac_mib_get_DesiredSSID(pst_mac_vap)));
    }

}
#endif


OAL_STATIC oal_void dmac_linkloss_update_threshold(dmac_vap_stru *pst_dmac_vap)
{
    wlan_linkloss_mode_enum en_mode = WLAN_LINKLOSS_MODE_BUTT;

    for (en_mode = WLAN_LINKLOSS_MODE_BT; en_mode < WLAN_LINKLOSS_MODE_BUTT ; en_mode++)
    {
        /*���޳�ʼֵ�����255*/
        GET_LINKLOSS_THRESHOLD(pst_dmac_vap, en_mode) =  MAKE_CURRENT_LINKLOSS_THRESHOLD(pst_dmac_vap, en_mode) < 0xFF ? (oal_uint8)MAKE_CURRENT_LINKLOSS_THRESHOLD(pst_dmac_vap, en_mode) : 0xFF;
        /*���޳�ʼֵ����С10*/
        if(GET_LINKLOSS_THRESHOLD(pst_dmac_vap, en_mode) < WLAN_LINKLOSS_MIN_THRESHOLD)
        {
            GET_LINKLOSS_THRESHOLD(pst_dmac_vap, en_mode) = WLAN_LINKLOSS_MIN_THRESHOLD;
        }

        /*���޳�ʼֵ��ֻ��beacon���ڸı�*/
        GET_LINKLOSS_INT_THRESHOLD(pst_dmac_vap, en_mode) = GET_LINKLOSS_THRESHOLD(pst_dmac_vap, en_mode);

        /* �˴������������ֵ 5/8�ĳ�ʼ���� */
        GET_LINKLOSS_MIN_THRESHOLD(pst_dmac_vap, en_mode) = (GET_LINKLOSS_THRESHOLD(pst_dmac_vap, en_mode)>>3) + (GET_LINKLOSS_THRESHOLD(pst_dmac_vap, en_mode)>>1);

        /* �˴����ý��ջ��߷���ʧ�����޼�ֵ 1/8�ĳ�ʼ���� */
        GET_LINKLOSS_THRESHOLD_DECR(pst_dmac_vap, en_mode) = (GET_LINKLOSS_THRESHOLD(pst_dmac_vap, en_mode)>>3);

        OAM_WARNING_LOG4(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX,
                         "{dmac_linkloss_update_threshold::linkloss_mode[%d], int_threshold[%d], min_threshold[%d], threshold_decr[%d]!}",
                         en_mode,
                         GET_LINKLOSS_THRESHOLD(pst_dmac_vap, en_mode),
                         GET_LINKLOSS_MIN_THRESHOLD(pst_dmac_vap, en_mode),
                         GET_LINKLOSS_THRESHOLD_DECR(pst_dmac_vap, en_mode));
    }
}


oal_void dmac_vap_linkloss_init(dmac_vap_stru *pst_dmac_vap)
{
    if (WLAN_VAP_MODE_BSS_STA != pst_dmac_vap->st_vap_base_info.en_vap_mode)
    {
        return;
    }

    dmac_vap_linkloss_clean(pst_dmac_vap);

    /* ��δ������Ԥ�����öԶ�ap��beacon����Ϊ100 */
    pst_dmac_vap->st_linkloss_info.ul_dot11BeaconPeriod = WLAN_BEACON_INTVAL_DEFAULT;
    GET_CURRENT_LINKLOSS_MODE(pst_dmac_vap) = WLAN_LINKLOSS_MODE_BUTT;
    pst_dmac_vap->st_linkloss_info.uc_linkloss_times = 0;

    dmac_linkloss_update_threshold(pst_dmac_vap);

#ifdef _PRE_WLAN_FEATURE_BTCOEX
    dmac_btcoex_linkloss_init(pst_dmac_vap);
#endif

}
#ifndef HI110x_EDA

OAL_STATIC oal_void dmac_vap_linkloss_incr(dmac_vap_stru *pst_dmac_vap, mac_device_stru *pst_mac_device)
{
    /* ɨ�衢�״2040���ŵ�ʱ��linkloss������������ */
    if ((OAL_TRUE == dmac_sta_csa_is_in_waiting(&(pst_dmac_vap->st_vap_base_info)))||
        (MAC_SCAN_STATE_RUNNING == pst_mac_device->en_curr_scan_state))
    {
        return;
    }

    INCR_CURRENT_LINKLOSS_CNT(pst_dmac_vap);
}
#endif

OAL_STATIC oal_void dmac_linkloss_threshold_incr(dmac_vap_stru *pst_dmac_vap)
{
    if(GET_CURRENT_LINKLOSS_THRESHOLD(pst_dmac_vap) >= GET_CURRENT_LINKLOSS_INT_THRESHOLD(pst_dmac_vap))
    {
        GET_CURRENT_LINKLOSS_THRESHOLD(pst_dmac_vap) = GET_CURRENT_LINKLOSS_INT_THRESHOLD(pst_dmac_vap);
    }
    else
    {
        GET_CURRENT_LINKLOSS_THRESHOLD(pst_dmac_vap) += LINKLOSS_THRESHOLD_INCR;
    }
}


oal_void dmac_vap_linkloss_threshold_incr(dmac_vap_stru *pst_dmac_vap)
{
    mac_device_stru                   *pst_mac_device;

    if (MAC_VAP_STATE_UP != pst_dmac_vap->st_vap_base_info.en_vap_state)
    {
        return;
    }

    pst_mac_device = mac_res_get_dev(pst_dmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {

        OAM_ERROR_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY,
                        "{dmac_vap_linkloss_threshold_incr::Cannot find mac_dev,by vap.device_id[%d]} ",
                        pst_dmac_vap->st_vap_base_info.uc_device_id);
        return ;
    }

    /* ɨ�衢�״2040���ŵ�ʱ��linkloss������������ */
    if ((OAL_TRUE == dmac_sta_csa_is_in_waiting(&(pst_dmac_vap->st_vap_base_info)))||
        (MAC_SCAN_STATE_RUNNING == pst_mac_device->en_curr_scan_state))
    {
        return;
    }

    dmac_linkloss_threshold_incr(pst_dmac_vap);

}


OAL_STATIC oal_void dmac_linkloss_threshold_decr(dmac_vap_stru *pst_dmac_vap)
{
    if(GET_CURRENT_LINKLOSS_THRESHOLD(pst_dmac_vap) < GET_CURRENT_LINKLOSS_MIN_THRESHOLD(pst_dmac_vap))
    {
        GET_CURRENT_LINKLOSS_THRESHOLD(pst_dmac_vap) = GET_CURRENT_LINKLOSS_MIN_THRESHOLD(pst_dmac_vap);
    }
    else
    {
        GET_CURRENT_LINKLOSS_THRESHOLD(pst_dmac_vap) -= GET_CURRENT_LINKLOSS_THRESHOLD_DECR(pst_dmac_vap);
    }
}


oal_void dmac_vap_linkloss_threshold_decr(dmac_vap_stru *pst_dmac_vap)
{
    mac_device_stru                   *pst_mac_device;

    if (MAC_VAP_STATE_UP != pst_dmac_vap->st_vap_base_info.en_vap_state)
    {
        return;
    }

    pst_mac_device = mac_res_get_dev(pst_dmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {

        OAM_ERROR_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY,
                        "{dmac_vap_linkloss_threshold_decr::Cannot find mac_dev,by vap.device_id[%d]} ",
                        pst_dmac_vap->st_vap_base_info.uc_device_id);
        return ;
    }

    /* ɨ�衢�״2040���ŵ�ʱ��linkloss������������ */
    if ((OAL_TRUE == dmac_sta_csa_is_in_waiting(&(pst_dmac_vap->st_vap_base_info))) ||
        (MAC_SCAN_STATE_RUNNING == pst_mac_device->en_curr_scan_state) ||
        (pst_dmac_vap->st_query_stats.s_signal < WLAN_FAR_DISTANCE_RSSI))
    {
        return;
    }

    dmac_linkloss_threshold_decr(pst_dmac_vap);
}
#ifndef HI110x_EDA

OAL_STATIC oal_void dmac_vap_linkloss_update_threshold(dmac_vap_stru *pst_dmac_vap, mac_device_stru *pst_mac_device)
{
    oal_uint32                         ul_dot11BeaconPeriod;
    dmac_vap_linkloss_stru            *pst_linkloss_info;

    ul_dot11BeaconPeriod = mac_mib_get_BeaconPeriod(&pst_dmac_vap->st_vap_base_info);

    pst_linkloss_info = &(pst_dmac_vap->st_linkloss_info);

    /*beacon���ڸı���Ҫ��������*/
    if(ul_dot11BeaconPeriod == pst_linkloss_info->ul_dot11BeaconPeriod)
    {
        return;
    }

    pst_linkloss_info->ul_dot11BeaconPeriod = ul_dot11BeaconPeriod;

    /* ��Чbeacon���(Ϊ0)��ǿ�а���beacon���Ϊ100ms����linkloss���� */
    if (0 == ul_dot11BeaconPeriod)
    {
        ul_dot11BeaconPeriod = WLAN_BEACON_INTVAL_DEFAULT;
    }

    /* beacon>400, ����beacon���Ϊ400ms����linkloss���� */
    if (ul_dot11BeaconPeriod > LINKLOSS_THRESHOLD_BEACON_MAX_INTVAL)
    {
        ul_dot11BeaconPeriod = LINKLOSS_THRESHOLD_BEACON_MAX_INTVAL;
    }

    pst_linkloss_info->uc_linkloss_times = (WLAN_BEACON_INTVAL_DEFAULT > ul_dot11BeaconPeriod) ? (WLAN_BEACON_INTVAL_DEFAULT / ul_dot11BeaconPeriod) : 1;

    OAM_WARNING_LOG2(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX,
            "{dmac_vap_linkloss_update_threshold::ul_dot11BeaconPeriod[%d] is changing to %d}",
              pst_linkloss_info->ul_dot11BeaconPeriod, ul_dot11BeaconPeriod);

    dmac_linkloss_update_threshold(pst_dmac_vap);

#ifdef _PRE_WLAN_FEATURE_BTCOEX
    dmac_btcoex_linkloss_update_threshold(pst_dmac_vap);
#endif
}
#endif

oal_void dmac_vap_linkloss_clean(dmac_vap_stru *pst_dmac_vap)
{
    mac_device_stru                   *pst_mac_device;
#ifdef _PRE_WLAN_FEATURE_BTCOEX
    dmac_vap_btcoex_occupied_stru     *pst_dmac_vap_btcoex_occupied;
#endif

    if (MAC_VAP_STATE_UP != pst_dmac_vap->st_vap_base_info.en_vap_state)
    {
        OAM_INFO_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY,
                        "{dmac_vap_linkloss_clean::en_vap_state[%d] is not MAC_VAP_STATE_UP, no need to clean.}",
                        pst_dmac_vap->st_vap_base_info.en_vap_state);
        return;
    }

    pst_mac_device = mac_res_get_dev(pst_dmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {

        OAM_ERROR_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY,
                        "{dmac_vap_linkloss_clean::Cannot find mac_dev,by vap.device_id[%d]} ",
                        pst_dmac_vap->st_vap_base_info.uc_device_id);
        return ;
    }

    GET_LINKLOSS_CNT(pst_dmac_vap, WLAN_LINKLOSS_MODE_BT) = 0;
    GET_LINKLOSS_CNT(pst_dmac_vap, WLAN_LINKLOSS_MODE_DBAC) = 0;
    GET_LINKLOSS_CNT(pst_dmac_vap, WLAN_LINKLOSS_MODE_NORMAL) = 0;
    pst_dmac_vap->st_linkloss_info.en_roam_scan_done = OAL_FALSE;
    pst_dmac_vap->st_linkloss_info.en_vowifi_report = OAL_FALSE;

    /*linkloss��������һ��֮�󣬷���prob req�Ĵ�����Ŀǰ����һ��*/
    pst_dmac_vap->st_linkloss_info.uc_allow_send_probe_req_cnt = 1;

#ifdef _PRE_WLAN_FEATURE_BTCOEX
    pst_dmac_vap_btcoex_occupied = &(pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_occupied);
    pst_dmac_vap_btcoex_occupied->uc_linkloss_index = 1;
    pst_dmac_vap_btcoex_occupied->uc_linkloss_occupied_times = 0;
#endif
}

#ifdef _PRE_WLAN_FEATURE_ROAM
#define ROAM_RSSI_LINKLOSS_TYPE           (-121)

oal_uint32 dmac_sta_roam_trigger_init(dmac_vap_stru *pst_dmac_vap)
{
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{dmac_sta_roam_trigger_init:: pst_dmac_vap NULL!}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_dmac_vap->st_roam_trigger.ul_cnt         = 0;
    pst_dmac_vap->st_roam_trigger.ul_time_stamp  = 0;
    pst_dmac_vap->st_roam_trigger.c_trigger_2G   = ROAM_TRIGGER_RSSI_NE70_DB;
    pst_dmac_vap->st_roam_trigger.c_trigger_5G   = ROAM_TRIGGER_RSSI_NE70_DB;

    return OAL_SUCC;
}


oal_uint32 dmac_sta_roam_trigger_check(dmac_vap_stru *pst_dmac_vap)
{
    mac_device_stru          *pst_mac_device;
    frw_event_mem_stru       *pst_event_mem     = OAL_PTR_NULL;
    frw_event_stru           *pst_event         = OAL_PTR_NULL;
    oal_uint32                ul_ret            = OAL_SUCC;
    oal_uint32                ul_delta_time     = 0;
    oal_bool_enum             en_trigger_enable = OAL_FALSE;
    oal_int8                  c_trigger_rssi    = 0;
    oal_int8                  c_current_rssi    = 0;
    oal_uint8                 uc_vap_id;
    oal_uint32                ul_cur_time       = 0;

    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_device  = (mac_device_stru *)mac_res_get_dev(pst_dmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(0, OAM_SF_WIFI_BEACON, "{dmac_sta_roam_trigger_check::pst_mac_device is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
     /* ���˷� WLAN0 �豸 */
    if (!(IS_LEGACY_VAP(&pst_dmac_vap->st_vap_base_info)))
    {
        return OAL_FAIL;
    }

     /* p2p������,ֱ��return
    if ((OAL_SUCC == mac_device_is_p2p_connected(pst_mac_device)))
    {
        return OAL_FAIL;
    }*/

    if (WLAN_BAND_5G == pst_dmac_vap->st_vap_base_info.st_channel.en_band)
    {
        c_trigger_rssi = pst_dmac_vap->st_roam_trigger.c_trigger_5G;
    }
    else
    {
        c_trigger_rssi = pst_dmac_vap->st_roam_trigger.c_trigger_2G;
    }

    c_current_rssi = (oal_int8)oal_get_real_rssi(pst_dmac_vap->st_query_stats.s_signal);

    if (c_current_rssi >= c_trigger_rssi)
    {
        pst_dmac_vap->st_roam_trigger.ul_cnt = 0;
    }
    else
    {
        /* �������ɴ�rssiС������ʱ����һ������  */
        if (pst_dmac_vap->st_roam_trigger.ul_cnt++ >= ROAM_TRIGGER_COUNT_THRESHOLD)
        {
            /* ���δ����¼���Ҫ�����ϱ���С��� */
            ul_cur_time    = (oal_uint32)OAL_TIME_GET_STAMP_MS();
            ul_delta_time  = (oal_uint32)OAL_TIME_GET_RUNTIME(pst_dmac_vap->st_roam_trigger.ul_time_stamp, ul_cur_time);

            if ((c_current_rssi <= ROAM_TRIGGER_RSSI_NE80_DB && ul_delta_time >= ROAM_TRIGGER_INTERVAL_10S) ||
                (c_current_rssi <= ROAM_TRIGGER_RSSI_NE75_DB && ul_delta_time >= ROAM_TRIGGER_INTERVAL_15S) ||
                (ul_delta_time >= ROAM_TRIGGER_INTERVAL_20S))
            {
                en_trigger_enable  = OAL_TRUE;
                pst_dmac_vap->st_roam_trigger.ul_cnt = 0;
            }
        }
    }

    /* WLAN0: LINK LOSS��������3/4������ʱ����һ�����Σ�rssiΪ0��ʾǿ������  */
    if ((GET_CURRENT_LINKLOSS_CNT(pst_dmac_vap) >= (GET_CURRENT_LINKLOSS_THRESHOLD(pst_dmac_vap) >> 1) + (GET_CURRENT_LINKLOSS_THRESHOLD(pst_dmac_vap) >> 2))
        &&(OAL_FALSE == pst_dmac_vap->st_linkloss_info.en_roam_scan_done))
    {
        OAM_WARNING_LOG2(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_WIFI_BEACON, "{dmac_sta_roam_trigger_check:: link cnt[%d] >= 3/4 threshold[%d],start roam scan.}",
                         GET_CURRENT_LINKLOSS_CNT(pst_dmac_vap),
                         GET_CURRENT_LINKLOSS_THRESHOLD(pst_dmac_vap));

        /*������������3/4,��ֹ�ظ���������ɨ��*/
        pst_dmac_vap->st_linkloss_info.en_roam_scan_done = OAL_TRUE;

        c_current_rssi     = ROAM_RSSI_LINKLOSS_TYPE;
        en_trigger_enable  = OAL_TRUE;
    }

    if (en_trigger_enable == OAL_FALSE)
    {
        return OAL_FAIL;
    }

    /* ���δ�����Ҫ��������������ʱ������WIFI+�л�Ƶ���������γ�ͻ���������������� IP ADDR �ѻ�ȡ */
    ul_delta_time  = (oal_uint32)OAL_TIME_GET_RUNTIME(pst_dmac_vap->st_roam_trigger.ul_ip_obtain_stamp, ul_cur_time);

    if(OAL_FALSE == pst_dmac_vap->st_roam_trigger.ul_ip_addr_obtained ||
       ul_delta_time < ROAM_WPA_CONNECT_INTERVAL_TIME)
    {
        OAM_WARNING_LOG2(0, OAM_SF_WIFI_BEACON, "{dmac_sta_roam_trigger_check:: ip_addr_check, ul_ip_addr_obtained = %d, ul_delta_time = %d}",
                        pst_dmac_vap->st_roam_trigger.ul_ip_addr_obtained,ul_delta_time);
        return OAL_FAIL;
    }

    uc_vap_id      = pst_dmac_vap->st_vap_base_info.uc_vap_id;

    /* �����¼��ڴ� */
    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(oal_uint8));
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(uc_vap_id, OAM_SF_WIFI_BEACON, "{dmac_sta_roam_trigger_check::pst_event_mem fail.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event = frw_get_event_stru(pst_event_mem);

    /* ��д�¼�ͷ */
    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                            FRW_EVENT_TYPE_DMAC_MISC,
                            DMAC_MISC_SUB_TYPE_ROAM_TRIGGER,
                            WLAN_MEM_EVENT_SIZE1,
                            FRW_EVENT_PIPELINE_STAGE_1,
                            pst_dmac_vap->st_vap_base_info.uc_chip_id,
                            pst_dmac_vap->st_vap_base_info.uc_device_id,
                            uc_vap_id);

    oal_memcopy(pst_event->auc_event_data, &c_current_rssi, OAL_SIZEOF(oal_uint8));

    /* �ַ��¼� */
    ul_ret = frw_event_dispatch_event(pst_event_mem);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(uc_vap_id, OAM_SF_WIFI_BEACON, "{dmac_sta_roam_trigger_check::frw_event_dispatch_event failed[%d].}", ul_ret);
        FRW_EVENT_FREE(pst_event_mem);
        return ul_ret;
    }

    /* �ͷ��¼� */
    FRW_EVENT_FREE(pst_event_mem);

    pst_dmac_vap->st_roam_trigger.ul_time_stamp = (oal_uint32)OAL_TIME_GET_STAMP_MS();

    return OAL_SUCC;
}
#endif //_PRE_WLAN_FEATURE_ROAM

#ifndef HI110x_EDA
#ifdef _PRE_WLAN_FEATURE_VOWIFI

OAL_STATIC void dmac_vap_update_vowifi_status(dmac_vap_stru *pst_dmac_vap)
{
    mac_vowifi_status_stru    *pst_vowifi_status;
    mac_vowifi_param_stru     *pst_vowifi_cfg_param;
    dmac_user_stru            *pst_dmac_user;
    oal_uint32                 ul_rx_succ_pkts;
    oal_uint32                 ul_pkt_drop_count;
    oal_uint64                 ull_timestamp_ms;

    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_VOWIFI, "{dmac_vap_update_vowifi_status::pst_dmac_vap is null} ");
        return ;
    }

    pst_vowifi_status    = pst_dmac_vap->pst_vowifi_status;
    pst_vowifi_cfg_param = pst_dmac_vap->st_vap_base_info.pst_vowifi_cfg_param;
    if ((OAL_PTR_NULL == pst_vowifi_status)||
        (OAL_PTR_NULL == pst_vowifi_cfg_param))
    {
        return ;
    }

    if (VOWIFI_DISABLE_REPORT == pst_vowifi_cfg_param->en_vowifi_mode)
    {
        /* ��ʹ��vowifi�����ֱ���˳� */
        return ;
    }

    pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user(pst_dmac_vap->st_vap_base_info.us_assoc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dmac_user))
    {
        OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_VOWIFI, "{dmac_vap_update_vowifi_status::pst_dmac_user[%d] NULL.}",
            pst_dmac_vap->st_vap_base_info.us_assoc_vap_id);
        return ;
    }

    /*
    vowifi���������������ĵ�:
    1.RSSI������������
    2.����TX/RX ʧ��
    3.arp req̽���5s �������ݽ���
    4.linkloss����������
    */

    ul_rx_succ_pkts = (pst_dmac_user->st_query_stats.ul_drv_rx_pkts >= pst_dmac_user->st_query_stats.ul_rx_dropped_misc)?
                      (pst_dmac_user->st_query_stats.ul_drv_rx_pkts - pst_dmac_user->st_query_stats.ul_rx_dropped_misc) : 0;

    /* step1, ����ͳ��ֵ */
    ull_timestamp_ms = OAL_TIME_GET_STAMP_MS();
    if (ull_timestamp_ms >= (pst_vowifi_status->ull_rssi_timestamp_ms + pst_vowifi_cfg_param->us_rssi_period_ms))
    {
        /* �����ź�vowifi rssiͳ��ֵ */
        pst_vowifi_status->uc_rssi_trigger_cnt = ((VOWIFI_LOW_THRES_REPORT == pst_vowifi_cfg_param->en_vowifi_mode) ?
                                               (oal_get_real_rssi(pst_dmac_vap->st_query_stats.s_signal) <= pst_vowifi_cfg_param->c_rssi_low_thres):
                                               (oal_get_real_rssi(pst_dmac_vap->st_query_stats.s_signal) >= pst_vowifi_cfg_param->c_rssi_high_thres)) ?
                                               (pst_vowifi_status->uc_rssi_trigger_cnt + 1):
                                               0;
        pst_vowifi_status->ull_rssi_timestamp_ms = ull_timestamp_ms;
    }

    if (ul_rx_succ_pkts != pst_vowifi_status->ul_arp_rx_succ_pkts)
    {
        /* ���µĽ���֡��˵��"��RX ֡"��̽�����Ч���������ʱ��� */
        pst_vowifi_status->ull_arp_timestamp_ms = ull_timestamp_ms;
    }



    /* step2�����ͳ��״̬ */
    /* vowifi RSSI��� */
    if (pst_vowifi_status->uc_rssi_trigger_cnt >= pst_vowifi_cfg_param->uc_trigger_count_thres)
    {
        OAM_WARNING_LOG2(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_VOWIFI, "{dmac_vap_update_vowifi_status::RSSI_monitor reporting,rssi[%d], count[%d].}",
                        oal_get_real_rssi(pst_dmac_vap->st_query_stats.s_signal),
                        pst_vowifi_status->uc_rssi_trigger_cnt);
        dmac_config_vowifi_report(pst_dmac_vap);
        return ;
    }

    /* ������ģʽ���ź�������⹤�����˽��� */
    if (VOWIFI_HIGH_THRES_REPORT == pst_vowifi_cfg_param->en_vowifi_mode)
    {
        return ;
    }

    /* Linkloss ����ֵ����һ�룬�л�vowifi״̬ */
    if (GET_CURRENT_LINKLOSS_CNT(pst_dmac_vap) >= GET_CURRENT_LINKLOSS_THRESHOLD(pst_dmac_vap) >> 1)
    {
        if(OAL_FALSE == pst_dmac_vap->st_linkloss_info.en_vowifi_report)
        {
            pst_dmac_vap->st_linkloss_info.en_vowifi_report = OAL_TRUE;
            dmac_config_vowifi_report(pst_dmac_vap);
            return ;
        }
    }

    /* tx arp req��5s���κγɹ����յ�����֡ */
    if (ul_rx_succ_pkts == pst_vowifi_status->ul_arp_rx_succ_pkts)
    {
        if (ull_timestamp_ms >= (pst_vowifi_status->ull_arp_timestamp_ms + 5000))
        {
            OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_VOWIFI, "{dmac_vap_update_vowifi_status::ARP_monitor reporting, the curr_rx_succ_pkts[%d].}",
                        pst_vowifi_status->ul_arp_rx_succ_pkts);
            dmac_config_vowifi_report(pst_dmac_vap);
            return ;
        }

    }

    /* ����10��������ʧ�� */
    ul_pkt_drop_count = pst_dmac_user->st_query_stats.ul_tx_failed - pst_vowifi_status->ul_tx_failed;
    if (ul_pkt_drop_count >= 10)
    {
        if (ul_pkt_drop_count == (pst_dmac_user->st_query_stats.ul_tx_total - pst_vowifi_status->ul_tx_total))
        {
            OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_VOWIFI, "{dmac_vap_update_vowifi_status::TX pkt_drop reporting, TX pkt_drop_count[%d] >= 10.}",
                        ul_pkt_drop_count);
            dmac_config_vowifi_report(pst_dmac_vap);
            return ;
        }
        else
        {
            pst_dmac_vap->pst_vowifi_status->ul_tx_total         = pst_dmac_user->st_query_stats.ul_tx_total;
            pst_dmac_vap->pst_vowifi_status->ul_tx_failed        = pst_dmac_user->st_query_stats.ul_tx_failed;
        }
    }

}
#endif /* _PRE_WLAN_FEATURE_VOWIFI */
OAL_STATIC void dmac_sta_tbtt_prior_linkloss_handler(dmac_vap_stru *pst_dmac_vap)
{
    mac_device_stru     *pst_mac_device;

    pst_mac_device = mac_res_get_dev(pst_dmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {

        OAM_ERROR_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY,
                        "{dmac_sta_up_misc::Cannot find mac_dev,by vap.device_id[%d]} ",
                        pst_dmac_vap->st_vap_base_info.uc_device_id);
        return ;
    }

    dmac_linkloss_update_mode(pst_dmac_vap, pst_mac_device);
    dmac_vap_linkloss_update_threshold(pst_dmac_vap, pst_mac_device);
    dmac_vap_linkloss_incr(pst_dmac_vap, pst_mac_device);

    /* linkloss��������һ��,׼�����͵���̽��֡ */
    dmac_linkloss_send_probe_req(pst_dmac_vap, pst_mac_device);

    /*linklossÿͳ�ƴﵽ10�Σ��ϱ�һ��ά����Ϣ��ɨ������ŵ�ʱ�̲���ӡ*/
    if ((0 == GET_CURRENT_LINKLOSS_CNT(pst_dmac_vap) % WLAN_LINKLOSS_REPORT)
        &&(0 != GET_CURRENT_LINKLOSS_CNT(pst_dmac_vap))
        && (OAL_FALSE == dmac_sta_csa_is_in_waiting(&(pst_dmac_vap->st_vap_base_info)))
        &&(MAC_SCAN_STATE_RUNNING != pst_mac_device->en_curr_scan_state))
    {
#ifdef _PRE_WLAN_DFT_STAT
        dmac_dft_report_all_para(pst_dmac_vap,OAL_FALSE);
#endif
    }
}


 OAL_STATIC void dmac_sta_up_misc(dmac_vap_stru *pst_dmac_vap)
 {

#ifdef _PRE_WLAN_FEATURE_TSF_SYNC
    pst_dmac_vap->uc_beacon_miss_cnt++;
#endif

#ifdef _PRE_WLAN_DOWNLOAD_PM
    if(g_us_download_rate_limit_pps)
    {
        return;
    }
#endif

    dmac_sta_tbtt_prior_linkloss_handler(pst_dmac_vap);

#ifdef _PRE_WLAN_FEATURE_VOWIFI
    /* ���vowifi�ļ��״̬ */
    dmac_vap_update_vowifi_status(pst_dmac_vap);
#endif /* _PRE_WLAN_FEATURE_VOWIFI */
#ifdef _PRE_WLAN_FEATURE_ROAM
    dmac_sta_roam_trigger_check(pst_dmac_vap);
#endif //_PRE_WLAN_FEATURE_ROAM

    /* sta��ap��ʧ���ӳ������޴��� */
    if (DMAC_IS_LINKLOSS(pst_dmac_vap))
    {
        OAM_WARNING_LOG4(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY,
                        "{dmac_sta_up_misc::Oh no,LinkLoss!! BSSID[%02X:XX:XX:%02X:%02X:%02X]} ",
                        pst_dmac_vap->st_vap_base_info.auc_bssid[0],
                        pst_dmac_vap->st_vap_base_info.auc_bssid[3],
                        pst_dmac_vap->st_vap_base_info.auc_bssid[4],
                        pst_dmac_vap->st_vap_base_info.auc_bssid[5]);
#ifdef _PRE_WLAN_DFT_STAT
        dmac_dft_report_all_para(pst_dmac_vap,OAL_TRUE);
#endif
        dmac_vap_linkloss_clean(pst_dmac_vap);
        dmac_send_disasoc_misc_event(&(pst_dmac_vap->st_vap_base_info),pst_dmac_vap->st_vap_base_info.us_assoc_vap_id, DMAC_DISASOC_MISC_LINKLOSS);
    }

    return ;
}


OAL_STATIC oal_void  dmac_tbtt_event_sta(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru  *pst_event;
    dmac_vap_stru   *pst_dmac_vap;

#ifdef _PRE_WLAN_CHIP_TEST
    if (1 == dmac_test_lpm_get_wow_en())
    {
         return;
    }
#endif

    pst_event = frw_get_event_stru(pst_event_mem);
    pst_dmac_vap = mac_res_get_dmac_vap(pst_event->st_event_hdr.uc_vap_id);

    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ACS, "dmac_tbtt_event_sta, pst_dmac_vap is null, vap id [%d].", pst_event->st_event_hdr.uc_vap_id);
        return;
    }

    if (MAC_VAP_STATE_UP == pst_dmac_vap->st_vap_base_info.en_vap_state)
    {
        dmac_sta_up_misc(pst_dmac_vap);
    }
    if(OAL_TRUE == dmac_sta_csa_is_in_waiting(&(pst_dmac_vap->st_vap_base_info)))
    {
        dmac_sta_csa_fsm_post_event(&(pst_dmac_vap->st_vap_base_info), WLAN_STA_CSA_EVENT_TBTT, 0, OAL_PTR_NULL);
    }
    return;
}
#endif
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)

OAL_STATIC oal_void  dmac_tbtt_event_ap_bottom_half(dmac_vap_stru *pst_dmac_vap)
{
    oal_uint32      ul_offset;
    oal_uint32      ul_led_tbtt_timer;
    dmac_user_stru *pst_dmac_multi_user;

    /* ��ǰDTIM���ڵ� */
    if (0 == pst_dmac_vap->uc_dtim_count)
    {

        ul_offset = pst_dmac_vap->us_in_tbtt_offset;
        hal_vap_read_tbtt_timer(pst_dmac_vap->pst_hal_vap, &ul_led_tbtt_timer);

        /* Ӳ������beacon֮ǰ��Ҫ��׼���ý��ܶ����еķ����鲥/�㲥�������´λ����ٷ��� */
        if (ul_led_tbtt_timer < ul_offset)
        {
            /* ����˴�tbtt�ж϶�Ӧ����DTIM���ڣ�����Ҫ�����鲥/�㲥���ܶ����еĻ���֡ */
            pst_dmac_multi_user = mac_res_get_dmac_user(pst_dmac_vap->st_vap_base_info.us_multi_user_idx);
            if (OAL_PTR_NULL == pst_dmac_multi_user)
            {
                OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_WIFI_BEACON, "{dmac_tbtt_event_ap_bottom_half::multi user[%d] fail.}",
                    pst_dmac_vap->st_vap_base_info.us_multi_user_idx);
                return;
            }
            /* �����ǰPSMδ����ʱ����Ľ��ܶ���ˢ������ */
            dmac_psm_queue_flush(pst_dmac_vap, pst_dmac_multi_user);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
            /*���û������������û����ҵ����ڽ��ܵ��û����������ܶ����Ƿ��а�������5��dtim�����а�������Ŀ����128������ж���*/
            if((1 <= pst_dmac_vap->st_vap_base_info.us_user_nums) && (0 != pst_dmac_vap->uc_ps_user_num))
            {
                dmac_user_ps_queue_overrun_notify(&pst_dmac_vap->st_vap_base_info);
            }
#endif
        }
        else
        {
            OAM_WARNING_LOG2(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_WIFI_BEACON,
                "{dmac_tbtt_event_ap_bottom_half::Delay to flush muti_frame, tbtt_timer(%d) > tbtt_offset(%d)}", ul_led_tbtt_timer, ul_offset);
        }
    }


    /* HI1103_DEV beacon encap�����ϰ벿���У��ŵ�������Ҫ�����°벿���� */
    /* �ŵ����� */
    dmac_handle_tbtt_chan_mgmt_ap(pst_dmac_vap);

}
#endif /* #if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV) */

OAL_STATIC oal_void  dmac_tbtt_event_ap(dmac_vap_stru *pst_dmac_vap)
{
    oal_uint8                    uc_bcn_idx_for_soft;   /* �������beacon֡��� */
    oal_uint8                   *puc_bcn_buf;
    hal_beacon_tx_params_stru    st_beacon_tx_params;
    hal_tx_txop_alg_stru        *pst_tx_param;
    dmac_user_stru              *pst_dmac_multi_user;
    oal_uint32                   ul_ret;
    mac_device_stru             *pst_macdev;
    wlan_channel_band_enum_uint8 en_band;
    hal_to_dmac_device_stru     *pst_hal_device;
#ifdef  _PRE_WLAN_FEATURE_AP_PM
    oal_uint32                   ul_bcn_interval_temp;
    hal_tx_txop_tx_power_stru    st_tx_power_temp;
    mac_pm_handler_stru          *pst_mac_pm_handle;
#endif
#ifdef _PRE_WLAN_FEATURE_PACKET_CAPTURE
    dmac_device_stru            *pst_dmac_dev;
#endif

    if ((MAC_VAP_STATE_UP != pst_dmac_vap->st_vap_base_info.en_vap_state) &&
        (MAC_VAP_STATE_PAUSE != pst_dmac_vap->st_vap_base_info.en_vap_state))
    {
        return;
    }

    pst_macdev = mac_res_get_dev(pst_dmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_macdev))
    {
        OAM_ERROR_LOG2(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_WIFI_BEACON,
                       "{dmac_tbtt_event_ap::dev is null! device id[%d] pst_dmac_vap[%p]}", pst_dmac_vap->st_vap_base_info.uc_device_id, pst_dmac_vap);
        return;
    }

    pst_hal_device = pst_dmac_vap->pst_hal_device;
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hal_device))
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_WIFI_BEACON,
                       "{dmac_tbtt_event_ap::pst_hal_device is null!}");
        return;
    }

    /* �����������beacon֡��� */
    uc_bcn_idx_for_soft = (DMAC_VAP_BEACON_BUFFER_BUTT == (pst_dmac_vap->uc_beacon_idx + 1)) ? DMAC_VAP_BEACON_BUFFER1 : DMAC_VAP_BEACON_BUFFER2;

    /* ���Ҫ���µ�beacon֡ */
    puc_bcn_buf = pst_dmac_vap->pauc_beacon_buffer[uc_bcn_idx_for_soft];
    if(puc_bcn_buf == OAL_PTR_NULL)
    {
        
        OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_WIFI_BEACON, "{dmac_tbtt_event_ap::beacon buf = NULL. uc_bcn_idx_for_soft=%d.}\r\n", uc_bcn_idx_for_soft);
        return;
    }

    /* ����1103��δ��beacon֡����Ҫtim cnt��ǰ��1 */
#ifndef _PRE_WLAN_MAC_BUGFIX_MCAST_HW_Q
    dmac_update_dtim_count_ap(pst_dmac_vap);
#endif


#if (_PRE_PRODUCT_ID != _PRE_PRODUCT_ID_HI1103_DEV)

    /* ��ǰDTIM���ڵ� */
    if (0 == pst_dmac_vap->uc_dtim_count)
    {
        /* ����˴�tbtt�ж϶�Ӧ����DTIM���ڣ�����Ҫ�����鲥/�㲥���ܶ����еĻ���֡ */
        pst_dmac_multi_user = mac_res_get_dmac_user(pst_dmac_vap->st_vap_base_info.us_multi_user_idx);
        if (OAL_PTR_NULL == pst_dmac_multi_user)
        {
            OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_WIFI_BEACON, "{dmac_tbtt_event_ap::multi user[%d] fail.}",
                pst_dmac_vap->st_vap_base_info.us_multi_user_idx);
            return;
        }
        /* �����ǰPSMδ����ʱ����Ľ��ܶ���ˢ������ */
        dmac_psm_queue_flush(pst_dmac_vap, pst_dmac_multi_user);

    #if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        /*���û������������û����ҵ����ڽ��ܵ��û����������ܶ����Ƿ��а�������5��dtim�����а�������Ŀ����128������ж���*/
        if((1 <= pst_dmac_vap->st_vap_base_info.us_user_nums) && (0 != pst_dmac_vap->uc_ps_user_num))
        {
            dmac_user_ps_queue_overrun_notify(&pst_dmac_vap->st_vap_base_info);
        }
    #endif
    }

#endif /* #if (_PRE_PRODUCT_ID != _PRE_PRODUCT_ID_HI1103_DEV) */

#ifdef _PRE_WLAN_MAC_BUGFIX_MCAST_HW_Q
    dmac_update_dtim_count_ap(pst_dmac_vap);
#endif

#if (_PRE_PRODUCT_ID != _PRE_PRODUCT_ID_HI1103_DEV)
    /* �ŵ����� */
    dmac_handle_tbtt_chan_mgmt_ap(pst_dmac_vap);
#endif /* #if (_PRE_PRODUCT_ID != _PRE_PRODUCT_ID_HI1103_DEV) */

    en_band = pst_dmac_vap->st_vap_base_info.st_channel.en_band;
    pst_tx_param =&(pst_dmac_vap->ast_tx_mgmt_bmcast[en_band]);
    pst_tx_param->ast_per_rate[0].rate_bit_stru.bit_reserve = 0;
#ifdef  _PRE_WLAN_FEATURE_AP_PM
    ul_bcn_interval_temp = mac_mib_get_BeaconPeriod(&pst_dmac_vap->st_vap_base_info);
#endif
#ifdef  _PRE_WLAN_FEATURE_AP_PM
    /*��������ֵ,������������ָ�*/
    st_tx_power_temp     = pst_tx_param->st_tx_power;
    pst_mac_pm_handle    = &pst_dmac_vap->st_pm_handler;

    if(PWR_SAVE_STATE_IDLE == GET_PM_STATE(pst_mac_pm_handle))
    {
        #ifdef _PRE_WLAN_FEATURE_DBAC
        if (!mac_is_dbac_enabled(mac_res_get_dev(pst_dmac_vap->st_vap_base_info.uc_device_id)))
        {
            // DBACʹ��ʱ,beacon��������
            mac_mib_set_BeaconPeriod(&pst_dmac_vap->st_vap_base_info, WLAN_BEACON_INTVAL_IDLE);
        }
        #endif

        oal_memcopy((oal_void*)&(pst_tx_param->st_tx_power),(oal_void*)&(pst_mac_pm_handle->ul_idle_beacon_txpower),4);

    }

#endif

    /* ����beacon֡ */
    dmac_encap_beacon(pst_dmac_vap, puc_bcn_buf, &(pst_dmac_vap->us_beacon_len));
#if ((_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV)  || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV))
    if(OAL_TRUE == pst_dmac_vap->st_vap_base_info.st_ap_beacon_test_ie.en_include_test_ie)
    {
        if(pst_dmac_vap->st_vap_base_info.st_ap_beacon_test_ie.uc_include_times > 0)
        {
            pst_dmac_vap->st_vap_base_info.st_ap_beacon_test_ie.uc_include_times --;
        }
        else
        {
            pst_dmac_vap->st_vap_base_info.st_ap_beacon_test_ie.en_include_test_ie = OAL_FALSE;
        }
    }
#endif
#ifndef _PRE_WLAN_MAC_BUGFIX_MCAST_HW_Q
    /*dtim countΪ0��bitmap�鲥��Ӧbit��0*/
    if (0 == pst_dmac_vap->uc_dtim_count)
    {
        pst_dmac_multi_user = mac_res_get_dmac_user(pst_dmac_vap->st_vap_base_info.us_multi_user_idx);
        if (OAL_PTR_NULL == pst_dmac_multi_user)
        {
            OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_WIFI_BEACON, "{dmac_tbtt_event_ap::multi user[%d] fail.}",
                pst_dmac_vap->st_vap_base_info.us_multi_user_idx);
            return;
        }
        dmac_psm_set_local_bitmap(pst_dmac_vap, pst_dmac_multi_user, 0);
    }
#endif

    /* �����µ�beacon֡д��Ӳ��beacon֡�Ĵ��� */
    /* ��д����Beacon֡���� */
    st_beacon_tx_params.pst_tx_param = pst_tx_param;
    st_beacon_tx_params.ul_pkt_ptr   = (oal_uint32)puc_bcn_buf;
    st_beacon_tx_params.us_pkt_len   = pst_dmac_vap->us_beacon_len;

    dmac_set_tx_chain_mask(pst_dmac_vap, &st_beacon_tx_params.ul_tx_chain_mask);

#ifdef _PRE_WLAN_FEATURE_BTCOEX
    dmac_btcoex_beacon_occupied_handler(pst_hal_device, pst_dmac_vap);
#endif

    /* HAL����Beacon֡�ӿ� */
    hal_vap_send_beacon_pkt(pst_dmac_vap->pst_hal_vap, &st_beacon_tx_params);

    ul_ret = oam_report_beacon(puc_bcn_buf, MAC_80211_FRAME_LEN,
                               puc_bcn_buf + MAC_80211_FRAME_LEN,
                               pst_dmac_vap->us_beacon_len,  OAM_OTA_FRAME_DIRECTION_TYPE_TX);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_WIFI_BEACON, "{dmac_tbtt_event_ap::oam_report_beacon return err: 0x%x.}\r\n", ul_ret);
    }

#ifdef _PRE_WLAN_FEATURE_PACKET_CAPTURE
    pst_dmac_dev = dmac_res_get_mac_dev(pst_macdev->uc_device_id);
    /* ֻ���ڻ��ץ��ģʽ�²Ż���ץ���ϱ�beacon�ĺ��� */
    if ((OAL_PTR_NULL != pst_dmac_dev) && (DMAC_PKT_CAP_MIX == pst_dmac_dev->st_pkt_capture_stat.uc_capture_switch))
    {
        /* �ϱ�beacon���ϲ� */
        ul_ret = dmac_pkt_cap_beacon(&(pst_dmac_dev->st_pkt_capture_stat), pst_dmac_vap);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(0, OAM_SF_PKT_CAP, "{dmac_tbtt_event_ap::capture_beacon return err: 0x%x.}\r\n", ul_ret);
        }
    }
#endif

#ifdef _PRE_WLAN_DFT_STAT
    /* ����beacon֡��Ŀͳ�� */
    DMAC_DFT_MGMT_STAT_INCR(pst_macdev->st_mgmt_stat.aul_tx_mgmt_soft[WLAN_BEACON]);
#endif
    /* ��������beacon֡ */
    pst_dmac_vap->uc_beacon_idx = uc_bcn_idx_for_soft;

#ifdef  _PRE_WLAN_FEATURE_AP_PM
/*�ָ�����ֵ*/
    mac_mib_set_BeaconPeriod(&pst_dmac_vap->st_vap_base_info, ul_bcn_interval_temp);
    pst_tx_param->st_tx_power = st_tx_power_temp;
#endif

}

#ifndef WIN32
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#if ((_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST))
    /* 03 MPW2 ð���ȹرչ��±��� */
#else

oal_void dmac_over_temp_handler(dmac_vap_stru *pst_dmac_vap, mac_device_stru *pst_mac_device, hal_to_dmac_device_stru *pst_hal_device)
{
    dmac_reset_para_stru        st_reset_param;
    oal_int16                   s_power_comp_val = 0;
#ifdef _PRE_WLAN_FEATURE_COMP_TEMP
    oal_uint8                            uc_cur_ch_num;
    wlan_channel_band_enum_uint8         en_freq_band;
    wlan_channel_bandwidth_enum_uint8    en_bandwidth;
#endif
    hal_to_dmac_device_rom_stru  *pst_hal_dev_rom = OAL_PTR_NULL;

    if(0 == pst_mac_device->ul_beacon_interval)
    {
        return;
    }

    pst_hal_dev_rom = (hal_to_dmac_device_rom_stru*)pst_hal_device->_rom;

    hal_get_pwr_comp_val(pst_hal_device, pst_hal_dev_rom->ul_duty_ratio, &s_power_comp_val);

#if (_PRE_WLAN_CHIP_ASIC == _PRE_WLAN_CHIP_VERSION)
    hal_over_temp_handler(pst_hal_device);
#endif

    if (OAL_TRUE == pst_hal_device->uc_over_temp)
    {
        pst_mac_device->en_reset_switch = OAL_TRUE;

        st_reset_param.uc_reset_type    = HAL_RESET_HW_TYPE_ALL;
        st_reset_param.uc_reset_mac_reg = OAL_TRUE;
        st_reset_param.uc_reset_phy_reg = OAL_TRUE;
        st_reset_param.uc_reset_mac_mod = HAL_RESET_MAC_ALL;
        st_reset_param.en_reason = DMAC_RETST_REASON_OVER_TEMP;

#if _PRE_WLAN_DFT_STAT
        hal_dft_report_all_reg_state(pst_hal_device);
#endif
        dmac_reset_hw(pst_mac_device, pst_hal_device, (oal_uint8 *)&st_reset_param);

        pst_mac_device->en_reset_switch = OAL_FALSE;
    }

    if(pst_hal_device->s_upc_amend != s_power_comp_val)
    {
        pst_hal_device->s_upc_amend = s_power_comp_val;

#ifdef _PRE_WLAN_FEATURE_COMP_TEMP
        uc_cur_ch_num = pst_dmac_vap->st_vap_base_info.st_channel.uc_chan_number;
        en_bandwidth  = pst_dmac_vap->st_vap_base_info.st_channel.en_bandwidth;
        en_freq_band  = pst_dmac_vap->st_vap_base_info.st_channel.en_band;

        /* ����UPC code */
        hal_device_amend_upc_code(pst_dmac_vap->pst_hal_device, en_freq_band, uc_cur_ch_num, en_bandwidth);
#endif
    }

}
#endif

#endif
#endif  /* WIN32 */


#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
OAL_STATIC oal_void dmac_update_txop_process(mac_vap_stru* pst_mac_vap)
{
    mac_device_stru*   pst_mac_dev;
    dmac_vap_stru*              pst_dmac_vap;
    oal_bool_enum_uint8         en_cur_bt_dbac_running = OAL_FALSE;
    oal_uint16                  aus_txop_limit[WLAN_BAND_WIDTH_BUTT] = {4000, 1300, 1300, 600, 600, 600, 600};

    pst_mac_dev = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_dev)
    {
        OAM_ERROR_LOG0(0, OAM_SF_WMM, "{dmac_update_txop_process::mac_device null, fail to set legacy txop val}");

        return;
    }
    pst_dmac_vap = mac_res_get_dmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_WMM, "{dmac_update_txop_process::mac_vap null pointer.}");
        return;
    }

#ifdef _PRE_WLAN_FEATURE_BTCOEX
    en_cur_bt_dbac_running = DMAC_VAP_GET_BTCOEX_STATUS(pst_dmac_vap)->un_bt_status.st_bt_status.bit_bt_on;
#endif
    en_cur_bt_dbac_running |= mac_is_dbac_running(pst_mac_dev);

    if(OAL_FALSE == pst_mac_dev->en_txop_enable)
    {
        if((OAL_FALSE == pst_dmac_vap->en_non_erp_exist)&&
           (OAL_FALSE == en_cur_bt_dbac_running)&&
           (0 == pst_mac_dev->uc_tx_ba_num))
        {
            pst_mac_dev->en_txop_enable = OAL_TRUE;
            hal_vap_set_machw_txop_limit_bkbe(pst_dmac_vap->pst_hal_vap,
                                      aus_txop_limit[pst_mac_vap->st_channel.en_bandwidth],
                                      (oal_uint16)mac_mib_get_QAPEDCATableTXOPLimit(&pst_dmac_vap->st_vap_base_info, WLAN_WME_AC_BK));
        }
    }
    else
    {
        if((OAL_TRUE == pst_dmac_vap->en_non_erp_exist)||
           (OAL_TRUE == en_cur_bt_dbac_running)||
           (0 != pst_mac_dev->uc_tx_ba_num))
        {
            pst_mac_dev->en_txop_enable = OAL_FALSE;
            hal_vap_set_machw_txop_limit_bkbe(pst_dmac_vap->pst_hal_vap,
                                      (oal_uint16)mac_mib_get_QAPEDCATableTXOPLimit(&pst_dmac_vap->st_vap_base_info, WLAN_WME_AC_BE),
                                      (oal_uint16)mac_mib_get_QAPEDCATableTXOPLimit(&pst_dmac_vap->st_vap_base_info, WLAN_WME_AC_BK));

        }
    }

    return;
}
#endif


oal_uint32  dmac_tbtt_event_handler(frw_event_mem_stru *pst_event_mem)
{
    dmac_vap_stru               *pst_dmac_vap;
    frw_event_stru              *pst_event;
    mac_device_stru             *pst_mac_device;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
    hal_error_state_stru         st_err_state;
#endif
    hal_to_dmac_device_stru     *pst_hal_device;
#ifndef WIN32
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    oal_int8                     c_current_rssi;
#endif
#endif  /* WIN32 */
#ifdef _PRE_PM_TIME_DEBUG
    oal_uint32              ul_tbtt_th_time;
#endif
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(0, OAM_SF_WIFI_BEACON, "{dmac_tbtt_event_handler::pst_event_mem null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_event = frw_get_event_stru(pst_event_mem);

    pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_event->st_event_hdr.uc_vap_id);
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG1(0, OAM_SF_WIFI_BEACON, "{dmac_tbtt_event_handler::pst_dmac_vap null.vap_id[%d]}",pst_event->st_event_hdr.uc_vap_id);

        return OAL_ERR_CODE_PTR_NULL;
    }

#if  defined(_PRE_PM_DYN_SET_TBTT_OFFSET) || defined(_PRE_PM_TIME_DEBUG)
    /* tbtt�°벿ʱ��� */
    if (IS_TBTT_TRAINING_IN_PROGRESS(pst_dmac_vap->pst_hal_vap) || g_pm_time_debug)
    {
        g_ul_tbtt_bh_time = glbcnt_read_low32();
    }
#endif

    
    pst_hal_device = DMAC_VAP_GET_HAL_DEVICE(pst_dmac_vap);
    if (OAL_PTR_NULL == pst_hal_device)
    {
        OAM_WARNING_LOG0(pst_event->st_event_hdr.uc_vap_id, OAM_SF_WIFI_BEACON, "{dmac_tbtt_event_handler::pst_hal_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    dmac_tbtt_exception_handler(pst_hal_device);

    pst_mac_device = mac_res_get_dev(pst_dmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_WIFI_BEACON, "{dmac_tbtt_event_handler::pst_mac_device null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

#ifndef WIN32
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#if ((_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST))
    /* 03 MPW2 ð���ȹرչ��±��� */
#else
    //OAM_WARNING_LOG2(0, OAM_SF_WIFI_BEACON, "{dmac_tbtt_event_handler::ul_duty_ratio=%d,beacon interval = %d!}",pst_mac_device->ul_duty_ratio,pst_mac_device->ul_beacon_interval);
    dmac_over_temp_handler(pst_dmac_vap, pst_mac_device, pst_hal_device);
#endif
#ifdef _PRE_WLAN_FEATURE_STA_PM
    dmac_psm_dec_null_frm(pst_dmac_vap);
#endif  /* _PRE_WLAN_FEATURE_STA_PM */

#endif
#endif  /* WIN32 */

#ifdef _PRE_WLAN_FEATURE_ALWAYS_TX
    if (OAL_SWITCH_ON == pst_dmac_vap->st_vap_base_info.bit_al_tx_flag)
    {
        return OAL_SUCC;
    }
#endif  /* _PRE_WLAN_FEATURE_ALWAYS_TX */

    if (WLAN_VAP_MODE_BSS_AP == pst_dmac_vap->st_vap_base_info.en_vap_mode)
    {
#ifdef _PRE_WLAN_FEATURE_P2P
        if ((IS_P2P_GO(&pst_dmac_vap->st_vap_base_info))&&
            (IS_P2P_OPPPS_ENABLED(pst_dmac_vap)))
        {
            dmac_p2p_oppps_ctwindow_start_event(pst_dmac_vap);
        }
#endif

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)
        dmac_tbtt_event_ap_bottom_half(pst_dmac_vap);
#else
        dmac_tbtt_event_ap(pst_dmac_vap);
#endif /* #if (_PRE_PRODUCT_ID != _PRE_PRODUCT_ID_HI1103_DEV)  */

    }
    else if (WLAN_VAP_MODE_BSS_STA == pst_dmac_vap->st_vap_base_info.en_vap_mode)
    {

#ifndef WIN32
#if (_PRE_MULTI_CORE_MODE ==_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC)

        c_current_rssi = (oal_int8)oal_get_real_rssi(pst_dmac_vap->st_query_stats.s_signal);

        hal_agc_threshold_handle(pst_hal_device, c_current_rssi);
#endif
#endif
        /* STA��pspoll�͹��Ĵ��� */
#ifdef _PRE_WLAN_FEATURE_STA_PM
        dmac_psm_process_tbtt_sta(pst_dmac_vap, pst_mac_device);
#endif

#ifdef _PRE_PM_TIME_DEBUG
        if (g_pm_time_debug)
        {
            ul_tbtt_th_time = (IS_LEGACY_VAP((&pst_dmac_vap->st_vap_base_info))) ? g_ul_wlan0_tbtt_th_time : g_ul_cl_tbtt_th_time;
            OAM_WARNING_LOG2(pst_event->st_event_hdr.uc_vap_id, OAM_SF_PWR, "{dmac_tbtt_event_handler::th2bh_tbtt[%d],rfpoweron[%d]}",
                                (g_ul_tbtt_bh_time-ul_tbtt_th_time), (g_ul_rf_on_time2-g_ul_rf_on_time1));
        }
#endif

#if (!defined(HI1102_EDA)) && (!defined(HI110x_EDA))
        dmac_tbtt_event_sta(pst_event_mem);
#endif
#ifdef _PRE_WLAN_FEATURE_BTCOEX
        /* ɨ�衢�״2040���ŵ�ʱ�򣬲�����bt����wifi linkloss���� */
        if ((OAL_FALSE == dmac_sta_csa_is_in_waiting(&(pst_dmac_vap->st_vap_base_info)))&&
            (MAC_SCAN_STATE_RUNNING != pst_mac_device->en_curr_scan_state))
        {
            dmac_btcoex_linkloss_occupied_process(pst_dmac_vap->pst_hal_chip, pst_dmac_vap->pst_hal_device, pst_dmac_vap);
        }
#endif
#ifdef _PRE_WLAN_FEATURE_11K
       if (OAL_TRUE == pst_dmac_vap->bit_11k_enable)
       {
           dmac_rrm_handle_quiet(pst_dmac_vap);
       }
#endif //_PRE_WLAN_FEATURE_11K
    }
    /* ���mac�������������֤ÿһ��tbtt��϶�У���ദ��Ĵ����� */
    /* ����vapʱ���������ݲ��������*/
    dmac_mac_error_cnt_clr(pst_mac_device);
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
    st_err_state.ul_error1_val = 0xffffffff;
    st_err_state.ul_error2_val = 0xffffffdf;
    hal_unmask_mac_error_init_status(pst_hal_device, &st_err_state);
#endif
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    dmac_update_txop_process(&pst_dmac_vap->st_vap_base_info);
#endif

    dmac_ant_tbtt_process(pst_hal_device);

#if 0
    /* 1102оƬ���ԣ���ӡ����tsf��״̬ */
    if (pst_dmac_vap->pst_hal_vap->uc_vap_id == 4)
    {
        oal_uint8             i;
        hal_to_dmac_vap_stru *pst_hal_vap;
        oal_uint32            ul_tsf;

        uc_cnt++;

        if (uc_cnt > 10)
        {
            uc_cnt = 0;

            for (i = 0; i < 2; i++)
            {
                hal_get_hal_vap(pst_hal_device, i, &pst_hal_vap);
                hal_vap_tsf_get_32bit(pst_hal_vap, &ul_tsf);

                OAM_INFO_LOG2(0, OAM_SF_DBAC, "hal_vap_id:%d, tsf:%d", i, ul_tsf);
            }

            for (i = 4; i < 7; i++)
            {
                hal_get_hal_vap(pst_hal_device, i, &pst_hal_vap);
                hal_vap_tsf_get_32bit(pst_hal_vap, &ul_tsf);

                OAM_INFO_LOG2(0, OAM_SF_DBAC, "hal_vap_id:%d, tsf:%d", i, ul_tsf);
            }
        }
    }
#endif

    return OAL_SUCC;
}

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)

oal_uint32  dmac_irq_tbtt_ap_isr(oal_uint8 uc_mac_vap_id)
{
    dmac_vap_stru *pst_dmac_vap;

    pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(uc_mac_vap_id);
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(uc_mac_vap_id, OAM_SF_WIFI_BEACON, "{dmac_irq_tbtt_ap_isr::pst_dmac_vap null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (!IS_AP(&(pst_dmac_vap->st_vap_base_info)))
    {
        OAM_ERROR_LOG0(uc_mac_vap_id, OAM_SF_WIFI_BEACON, "{dmac_irq_tbtt_ap_isr::dmac_vap is nor ap_mode!}");
        return OAL_FAIL;
    }

    dmac_tbtt_event_ap(pst_dmac_vap);

    return OAL_SUCC;

}

#endif /* #if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)  */
#ifdef _PRE_WLAN_FEATURE_P2P

oal_uint32  dmac_beacon_set_p2p_noa(
                mac_vap_stru           *pst_mac_vap,
                oal_uint32              ul_start_tsf,
                oal_uint32              ul_duration,
                oal_uint32              ul_interval,
                oal_uint8               uc_count)
{
    dmac_vap_stru              *pst_dmac_vap;
    /*������ܲ���������encap beacon,probe response*/
    pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG1(0, OAM_SF_WIFI_BEACON, "{dmac_beacon_set_p2p_noa::mac_res_get_dmac_vap fail.vap_id = %u}",pst_mac_vap->uc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_dmac_vap->st_p2p_noa_param.uc_count = uc_count;
    pst_dmac_vap->st_p2p_noa_param.ul_duration = ul_duration;
    pst_dmac_vap->st_p2p_noa_param.ul_interval = ul_interval;
    pst_dmac_vap->st_p2p_noa_param.ul_start_time = ul_start_tsf;
    return OAL_SUCC;
}

/*lint -e578*//*lint -e19*/
oal_module_symbol(dmac_beacon_set_p2p_noa);
/*lint +e578*//*lint +e19*/

#endif
#ifdef _PRE_WLAN_CHIP_TEST

void dmac_sta_test_info_dump(mac_vap_stru *pst_mac_vap)
{
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)

    mac_opmode_notify_stru *pst_opmode;

    OAL_IO_PRINT("uc_test_flag=%d\r\n", g_beacon_offload_test.uc_test_flag);
    OAL_IO_PRINT("uc_host_sleep=%d\r\n", g_beacon_offload_test.uc_host_sleep);
    OAL_IO_PRINT("uc_opmode_in_beacon=%d\r\n", g_beacon_offload_test.uc_opmode_in_beacon);
    OAL_IO_PRINT("uc_opmode=%d\r\n", g_beacon_offload_test.uc_opmode);

    pst_opmode = (mac_opmode_notify_stru*)&g_beacon_offload_test.uc_opmode;
    OAL_IO_PRINT("bit_channel_width=%d bit_rx_nss=%d bit_rx_nss_type=%d\r\n",
            pst_opmode->bit_channel_width, pst_opmode->bit_rx_nss, pst_opmode->bit_rx_nss_type);
    OAL_IO_PRINT("uc_csa_in_beacon=%d\r\n", g_beacon_offload_test.uc_csa_in_beacon);
    OAL_IO_PRINT("uc_announced_channel=%d\r\n", g_beacon_offload_test.uc_announced_channel);
    OAL_IO_PRINT("uc_switch_cnt=%d\r\n", g_beacon_offload_test.uc_switch_cnt);
    OAL_IO_PRINT("wmm in beacon=%d\r\n", mac_mib_get_dot11QosOptionImplemented(pst_mac_vap));
    OAL_IO_PRINT("wmm cnt=%d\r\n", pst_mac_vap->uc_wmm_params_update_count);
#endif
}



void dmac_sta_beacon_offload_test(mac_vap_stru *pst_mac_vap, oal_uint8 *uc_param)
{
    if (!uc_param)
    {
        OAM_ERROR_LOG0(0, OAM_SF_FRAME_FILTER, "dmac_sta_beacon_offload_test::uc_param null");
        return;
    }

    OAL_IO_PRINT("param[0]=%d param[1]=%d param[2]=%d param[3]=%d\r\n",
                uc_param[0], uc_param[1], uc_param[2], uc_param[3]);
    switch (uc_param[0])
    {
    case 0: //�򿪣��رղ���
        g_beacon_offload_test.uc_test_flag = uc_param[1];
        if (1 == uc_param[1])
        {
            oal_memset(&g_beacon_offload_test, 0, OAL_SIZEOF(g_beacon_offload_test));
            g_beacon_offload_test.uc_test_flag = uc_param[1];
            g_beacon_offload_test.uc_host_sleep = 0;
            g_beacon_offload_test.uc_opmode_in_beacon = 0;
            g_beacon_offload_test.uc_opmode = 0;
            g_beacon_offload_test.uc_csa_in_beacon = pst_mac_vap->st_ch_switch_info.en_csa_present_in_bcn;
            g_beacon_offload_test.uc_announced_channel = pst_mac_vap->st_ch_switch_info.uc_announced_channel;
            g_beacon_offload_test.uc_switch_cnt = pst_mac_vap->st_ch_switch_info.uc_ch_switch_cnt;
        }
        break;

    case 1: //�ֶ�ģ��host sleep
        g_beacon_offload_test.uc_host_sleep = uc_param[1];
        break;

    case 2: //����opmode ie
        g_beacon_offload_test.uc_opmode_in_beacon = uc_param[1];
        g_beacon_offload_test.uc_opmode = uc_param[2];
        break;

    case 3: //����csa ie
        g_beacon_offload_test.uc_csa_in_beacon = uc_param[1];
        g_beacon_offload_test.uc_announced_channel = uc_param[2];
        g_beacon_offload_test.uc_switch_cnt = uc_param[3];
        break;

    case 4:

        mac_mib_set_dot11QosOptionImplemented(pst_mac_vap, uc_param[1]);
        pst_mac_vap->uc_wmm_params_update_count = uc_param[2];
        break;

    default:
        OAL_IO_PRINT("wrong type\r\n");
        break;
    }

    dmac_sta_test_info_dump(pst_mac_vap);
    return;
}
#endif

oal_void  dmac_vap_linkloss_channel_clean(dmac_vap_stru *pst_dmac_vap, oal_netbuf_stru *pst_netbuf)
{
    oal_uint8                   uc_frame_channel;
    mac_vap_stru               *pst_mac_vap;
    dmac_rx_ctl_stru           *pst_rx_ctl;
    mac_rx_ctl_stru            *pst_rx_info;
    oal_uint8                  *puc_payload;
    oal_uint16                  us_msg_len;

    pst_mac_vap    = &pst_dmac_vap->st_vap_base_info;
    pst_rx_ctl     = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
    pst_rx_info    = (mac_rx_ctl_stru *)(&(pst_rx_ctl->st_rx_info));
    puc_payload    = MAC_GET_RX_PAYLOAD_ADDR(pst_rx_info, pst_netbuf);
    us_msg_len     = MAC_GET_RX_CB_PAYLOAD_LEN(pst_rx_info);  /*֡�峤��*/

    uc_frame_channel = mac_ie_get_chan_num(puc_payload,
                                            (us_msg_len - MAC_80211_FRAME_LEN),
                                            MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN + MAC_CAP_INFO_LEN,
                                            pst_rx_ctl->st_rx_info.uc_channel_number);
    if ((pst_mac_vap->st_channel.uc_chan_number == uc_frame_channel)
        && (0 != uc_frame_channel))
    {
        dmac_vap_linkloss_clean(pst_dmac_vap);
        dmac_vap_linkloss_threshold_incr(pst_dmac_vap);
    }
}

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST

oal_void  dmac_mgmt_obss_scan_notify(mac_vap_stru *pst_mac_vap)
{
    dmac_vap_stru *pst_dmac_vap;

    pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_PWR, "{dmac_ie_proc_obss_scan_ie::mac_res_get_dmac_vap fail,vap_id:%u.}",
                         pst_mac_vap->uc_vap_id);
        return ;
    }

    /* 20/40�����߼�����ʱ���ж��Ƿ�����obssɨ�趨ʱ�� */
    /* STAģʽ������Ҫobssɨ�趨ʱ��������ʱ�� */
    if ((OAL_TRUE == dmac_mgmt_need_obss_scan(pst_mac_vap)) &&
         (OAL_FALSE == pst_dmac_vap->uc_obss_scan_timer_started) )
    {
        OAM_WARNING_LOG0(0, OAM_SF_SCAN, "{dmac_ie_proc_obss_scan_ie:: start obss scan}");
        dmac_scan_start_obss_timer(pst_mac_vap);
    }
}
#endif


oal_uint32 dmac_sta_set_bandwith_handler(dmac_vap_stru  *pst_dmac_vap, wlan_channel_bandwidth_enum_uint8 en_sta_new_bandwidth)
{
    mac_vap_stru *pst_mac_vap;

    pst_mac_vap = &pst_dmac_vap->st_vap_base_info;

    if (MAC_BW_CHANGE == mac_vap_set_bw_check(pst_mac_vap, en_sta_new_bandwidth))
    {
        /* ֻ���������л���������ŵ��л���Ҫ����dmac_chan_multi_select_channel_mac�ӿ� */
        dmac_chan_select_channel_mac(pst_mac_vap, pst_mac_vap->st_channel.uc_chan_number, MAC_VAP_GET_CAP_BW(pst_mac_vap));
        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_ASSOC,
                       "{dmac_sta_set_bandwith_handler::change BW. uc_channel[%d], en_bandwidth[%d].}",
                       pst_mac_vap->st_channel.uc_chan_number,
                       en_sta_new_bandwidth);
    }

    if (OAL_TRUE == pst_mac_vap->st_ch_switch_info.bit_wait_bw_change)
    {
        pst_mac_vap->st_ch_switch_info.bit_wait_bw_change = OAL_FALSE;
    }

    return OAL_SUCC;
}


oal_void dmac_sta_post_bw_verify_switch(dmac_vap_stru  *pst_dmac_vap, dmac_sta_bw_switch_type_enum_enum_uint8 uc_verify_reslt)
{
    if (DMAC_STA_BW_VERIFY_20M_TO_40M == uc_verify_reslt)
    {
        /* �����л�״̬�� */
        dmac_sta_bw_switch_fsm_post_event(pst_dmac_vap, DMAC_STA_BW_SWITCH_EVENT_BEACON_40M, 0, OAL_PTR_NULL);
    }

    if (DMAC_STA_BW_VERIFY_40M_TO_20M == uc_verify_reslt)
    {
        dmac_sta_bw_switch_fsm_post_event(pst_dmac_vap, DMAC_STA_BW_SWITCH_EVENT_BEACON_20M, 0, OAL_PTR_NULL);
    }

    return;
}



oal_uint32  dmac_sta_up_rx_beacon(
                dmac_vap_stru   *pst_dmac_vap,
                oal_netbuf_stru *pst_netbuf,
                oal_uint8       *pen_go_on)
{
    mac_ieee80211_frame_stru   *pst_frame_hdr;
    mac_vap_stru               *pst_mac_vap;
    mac_user_stru              *pst_mac_user;
    dmac_rx_ctl_stru           *pst_rx_ctl;
    mac_rx_ctl_stru            *pst_rx_info;
    oal_uint8                  *puc_payload;
    oal_uint16                  us_ie_offset;
    oal_uint16                  us_msg_len;
    oal_uint32                  ul_change_flag = MAC_NO_CHANGE;
    oal_uint32                  ul_ret;
#ifdef _PRE_PRODUCT_ID_HI110X_DEV
#if (_PRE_OS_VERSION_RAW == _PRE_OS_VERSION)
    oal_uint16                  us_beacon_period;
#endif
#endif
    wlan_nss_enum_uint8         en_user_num_spatial_stream;             /* user����ռ������� */
    wlan_nss_enum_uint8         en_avail_bf_num_spatial_stream;         /* �û�֧�ֵ�Beamforming�ռ������� */
    wlan_nss_enum_uint8         en_avail_num_spatial_stream;            /* Tx��Rx֧��Nss�Ľ���,���㷨���� */
    oal_bool_enum_uint8         en_nss_change = OAL_FALSE;              /* ��ʶnss�����Ƿ��б仯��ˢ��user������ͬ����host�� */
    oal_uint8                            uc_verify_reslt;
    wlan_channel_bandwidth_enum_uint8    en_sta_new_bandwidth;
    wlan_bw_cap_enum_uint8               en_user_bw_cap;

    *pen_go_on = OAL_FALSE;

    pst_mac_vap    = &pst_dmac_vap->st_vap_base_info;
    pst_rx_ctl     = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
    pst_frame_hdr  = (mac_ieee80211_frame_stru *)mac_get_rx_cb_mac_hdr(&(pst_rx_ctl->st_rx_info));
    pst_rx_info    = (mac_rx_ctl_stru *)(&(pst_rx_ctl->st_rx_info));
    puc_payload    = MAC_GET_RX_PAYLOAD_ADDR(pst_rx_info, pst_netbuf);
    us_msg_len     = MAC_GET_RX_CB_PAYLOAD_LEN(pst_rx_info);  /*֡�峤��*/

    /* ��������bss��Beacon�������� */
    ul_ret = oal_compare_mac_addr(pst_mac_vap->auc_bssid, pst_frame_hdr->auc_address3);
    if(0 != ul_ret)
    {
        return OAL_SUCC;
    }

    /* ��ȡbeacon ֡�е��ŵ� */
    dmac_vap_linkloss_channel_clean(pst_dmac_vap, pst_netbuf);

#ifdef _PRE_PRODUCT_ID_HI110X_DEV
#if (_PRE_OS_VERSION_RAW == _PRE_OS_VERSION)
    /* ���beacon�����Ƿ�ı� */
    us_beacon_period = OAL_MAKE_WORD16(puc_payload[MAC_TIME_STAMP_LEN], puc_payload[MAC_TIME_STAMP_LEN + 1]);
    if (us_beacon_period != mac_mib_get_BeaconPeriod(pst_mac_vap) && (us_beacon_period != 0)) /* dbac������ʱ�����£���liuzhengqi���� */
    {
        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_FRAME_FILTER, "{dmac_sta_up_rx_beacon:beacon period changed old[%d]->new[%d].", mac_mib_get_BeaconPeriod(pst_mac_vap), us_beacon_period);
        mac_mib_set_BeaconPeriod((mac_vap_stru*)pst_dmac_vap, us_beacon_period);
        hal_vap_set_psm_beacon_period(pst_dmac_vap->pst_hal_vap, us_beacon_period);
        dmac_psm_update_dtime_period(pst_mac_vap, (oal_uint8)mac_mib_get_dot11dtimperiod(pst_mac_vap), us_beacon_period);
        dmac_psm_update_keepalive(pst_dmac_vap);
        dmac_vap_restart_dbac(pst_dmac_vap);
    }
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_TSF_SYNC
    pst_dmac_vap->uc_beacon_miss_cnt = 0;
#endif

#ifdef _PRE_WIFI_DMT
    *pen_go_on = OAL_TRUE;
    return OAL_SUCC;
#else

    us_ie_offset = MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN + MAC_CAP_INFO_LEN;

    if (us_msg_len > us_ie_offset)
    {
        puc_payload += us_ie_offset;
        us_msg_len  -= us_ie_offset;
    }
    else
    {
        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_FRAME_FILTER, "{dmac_sta_up_rx_beacon: rx beacon msg_len[%d] <= ie_offset[%d].", us_msg_len, us_ie_offset);
        return OAL_FALSE;
    }
#if 0
    if (OAL_TRUE == dmac_sta_channel_is_changed(pst_mac_vap, puc_payload, us_msg_len))
    {
        ul_ret = dmac_send_disasoc_misc_event(pst_mac_vap, pst_mac_vap->us_assoc_vap_id, DMAC_DISASOC_MISC_CHANNEL_MISMATCH);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG0(0, OAM_SF_WIFI_BEACON,
                       "{dmac_sta_up_rx_beacon::dmac_send_disasoc_misc_event fail.}");
        }
    }
#endif
    pst_mac_user = (mac_user_stru *)mac_res_get_mac_user(MAC_GET_RX_CB_TA_USER_IDX(&(pst_rx_ctl->st_rx_info)));
    if (OAL_PTR_NULL == pst_mac_user)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_FRAME_FILTER, "{dmac_sta_up_rx_beacon::pst_mac_user[%d] null}",
            MAC_GET_RX_CB_TA_USER_IDX(&(pst_rx_ctl->st_rx_info)));
        return OAL_FALSE;
    }
    en_user_num_spatial_stream     = pst_mac_user->en_user_num_spatial_stream;
    en_avail_bf_num_spatial_stream = pst_mac_user->en_avail_bf_num_spatial_stream;
    en_avail_num_spatial_stream    = pst_mac_user->en_avail_num_spatial_stream;

    /* ��non_erpվ��ʱʹ��long slottime */
    if (WLAN_BAND_2G == pst_mac_vap->st_channel.en_band)
    {
        dmac_sta_update_slottime(pst_mac_vap, pst_mac_user, puc_payload, us_msg_len);
    }

    //dmac_sta_update_txop_process(pst_mac_vap, pst_mac_user);

#endif

    /* �Ƿ���ҪOBSSɨ�� */
    dmac_mgmt_obss_scan_notify(pst_mac_vap);

    if(OAL_FALSE == dmac_sta_csa_is_in_waiting(pst_mac_vap))
    {
        /*����HT �����ϢԪ��*/
        ul_change_flag |= dmac_sta_up_update_ht_params(pst_mac_vap, puc_payload, us_msg_len, pst_mac_user);

        /* ���� VHT �����ϢԪ�� */
        ul_change_flag |= dmac_sta_up_update_vht_params(pst_mac_vap, puc_payload, us_msg_len, pst_mac_user);

    #ifdef _PRE_WLAN_FEATURE_11AX
        dmac_sta_up_update_spatial_reuse_params(pst_mac_vap, puc_payload, us_msg_len, pst_mac_user);
        ul_change_flag |=  dmac_sta_up_update_he_oper_params(pst_mac_vap, puc_payload, us_msg_len, pst_mac_user);
    #endif
    }
    /* ����ģʽ��� */
    dmac_sta_up_process_erp_ie(puc_payload, us_msg_len, pst_mac_user);

    dmac_sta_up_update_protection_mode(pst_dmac_vap, pst_mac_user);

#ifdef _PRE_WLAN_NARROW_BAND
    //mac_get_nb_ie(pst_mac_vap, puc_payload, us_msg_len);
#endif

#ifdef _PRE_WLAN_FEATURE_SMPS
    /* ����STA�յ�beacon֮֡��user��smps������ˢ��֮����Ҫ���¸��¿ռ���������֪ͨ�㷨 */
    if(!IS_VAP_SINGLE_NSS(pst_mac_vap) && !IS_USER_SINGLE_NSS(pst_mac_user))
    {
        dmac_check_smps_field(pst_mac_vap, puc_payload, us_msg_len, pst_mac_user);
    }
#endif

#ifdef _PRE_WLAN_FEATURE_OPMODE_NOTIFY
    /* ��vap��˫�ռ���ʱ������Operating Mode Notification ��ϢԪ�� */
    if(!IS_VAP_SINGLE_NSS(pst_mac_vap) && !IS_USER_SINGLE_NSS(pst_mac_user))
    {
        ul_change_flag |= dmac_check_opmode_notify(pst_mac_vap, puc_payload, us_msg_len, pst_mac_user);
    }
#endif

#ifdef _PRE_WLAN_FEATURE_M2S
    /*���ռ����������ͱ仯��������㷨���Ӻ���*/
    if ((pst_mac_user->en_avail_bf_num_spatial_stream != en_avail_bf_num_spatial_stream) ||
          (pst_mac_user->en_avail_num_spatial_stream != en_avail_num_spatial_stream))
    {
        en_nss_change = OAL_TRUE;
    }

    pst_mac_user->en_user_num_spatial_stream = dmac_m2s_get_bss_max_nss(pst_mac_vap, pst_netbuf, us_msg_len, OAL_TRUE);
    if(en_user_num_spatial_stream != pst_mac_user->en_user_num_spatial_stream)
    {
        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_M2S,
                  "{dmac_sta_up_rx_beacon::user nss[%d] need to refresh to [%d].}", en_user_num_spatial_stream, pst_mac_user->en_user_num_spatial_stream);

        en_nss_change = OAL_TRUE;
    }

    if(OAL_TRUE == en_nss_change)
    {
        /* ���ݴ������nss�仯��ˢ��Ӳ�����еİ�����֪ͨ�㷨��ͬ����host�� */
        dmac_m2s_nss_and_bw_alg_notify(pst_mac_vap, pst_mac_user, en_nss_change, OAL_FALSE);
    }
#else
    /*���ռ����������ͱ仯��������㷨���Ӻ���*/
    if ((pst_mac_user->en_avail_bf_num_spatial_stream != en_avail_bf_num_spatial_stream) ||
          (pst_mac_user->en_avail_num_spatial_stream != en_avail_num_spatial_stream))
    {
        dmac_alg_cfg_user_spatial_stream_notify(pst_mac_user);

        en_nss_change = OAL_TRUE;
    }

    if(OAL_TRUE == en_nss_change)
    {
        /* user����ͬ����hmac */
        if (OAL_SUCC != dmac_config_d2h_user_m2s_info_syn(pst_mac_vap, pst_mac_user))
        {
            OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ASSOC,
                      "{dmac_sta_up_rx_beacon::dmac_config_d2h_user_m2s_info_syn failed.}");
        }
    }
#endif

    /*csa������̲��������仯*/
    if ((OAL_FALSE == dmac_sta_csa_is_in_waiting(pst_mac_vap)) && (MAC_VAP_BW_FSM_BEACON_AVAIL(pst_mac_vap)))
    {
        ul_change_flag |= mac_vap_check_ap_usr_opern_bandwidth(pst_mac_vap, pst_mac_user);
        if ((MAC_VHT_CHANGE | MAC_HT_CHANGE | MAC_BW_DIFF_AP_USER | MAC_BW_OPMODE_CHANGE) & ul_change_flag)
        {
            en_sta_new_bandwidth = mac_vap_get_ap_usr_opern_bandwidth(pst_mac_vap, pst_mac_user);
            en_user_bw_cap  = mac_vap_bw_mode_to_bw(en_sta_new_bandwidth);
            uc_verify_reslt = dmac_sta_bw_switch_need_new_verify(pst_dmac_vap, en_user_bw_cap);

            /* 20M��40M����У�飬tx=20M&rx=40M */
            if (DMAC_STA_BW_VERIFY_20M_TO_40M == uc_verify_reslt)
            {
                en_user_bw_cap       = WLAN_BW_CAP_20M;
            }
            else if (DMAC_STA_BW_VERIFY_40M_TO_20M == uc_verify_reslt)
            {
                en_sta_new_bandwidth = MAC_VAP_GET_CAP_BW(pst_mac_vap);
                en_user_bw_cap       = WLAN_BW_CAP_20M;
            }

            OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_ASSOC,
                           "{dmac_sta_up_rx_beacon::change BW. ul_change[0x%x] uc_verify_reslt[%d] usr_new_bw_cap[%d] sta_new_bandwidth[%d].}",
                           ul_change_flag, uc_verify_reslt, en_user_bw_cap, en_sta_new_bandwidth);

            dmac_sta_set_bandwith_handler(pst_dmac_vap, en_sta_new_bandwidth);
            dmac_user_set_bandwith_handler(pst_mac_vap, (dmac_user_stru *)pst_mac_user, en_user_bw_cap);
            dmac_config_d2h_user_info_syn(pst_mac_vap, pst_mac_user);

            /* �����л� */
            dmac_sta_post_bw_verify_switch(pst_dmac_vap, uc_verify_reslt);
        }
        /* ����δ�ı�,���notify channel width��action֡�Ƿ�֪ͨ���㷨�ı���� */
        else
        {
            if (OAL_TRUE == pst_mac_vap->st_ch_switch_info.bit_wait_bw_change)
            {
                pst_mac_vap->st_ch_switch_info.bit_wait_bw_change = OAL_FALSE;
                /* Aciton notify֪ͨ�ı䣬����ʵ�ʴ���δ�ı䣬��Ҫ�л��� */
                en_sta_new_bandwidth = mac_vap_get_ap_usr_opern_bandwidth(pst_mac_vap, pst_mac_user);
                dmac_user_set_bandwith_handler(pst_mac_vap, (dmac_user_stru *)pst_mac_user, mac_vap_bw_mode_to_bw(en_sta_new_bandwidth));
            }
        }
    }

    /* �ŵ��л����� */
    dmac_chan_update_csw_info(pst_mac_vap, puc_payload, us_msg_len);

    /*�����������*/
    dmac_ie_proc_csa_bw_ie(pst_mac_vap, puc_payload, us_msg_len);

    if ((OAL_TRUE == dmac_sta_edca_is_changed(pst_mac_vap, puc_payload, us_msg_len)) ||
        (OAL_TRUE == dmac_sta_11ntxbf_is_changed(pst_mac_user, puc_payload, us_msg_len)))
    {
        *pen_go_on = OAL_TRUE;
    }

    return OAL_SUCC;
}


oal_uint32  dmac_beacon_timeout_event_hander(frw_event_mem_stru *pst_event_mem)
{
#ifdef _PRE_WLAN_FEATURE_STA_PM
    dmac_vap_stru               *pst_dmac_vap;
    frw_event_stru              *pst_event;
    mac_device_stru             *pst_mac_device;
    mac_sta_pm_handler_stru     *pst_mac_sta_pm_handle;
#ifndef HI110x_EDA
    oal_uint16                  us_beacon_timeout_adjust    =   0UL;
#ifdef _PRE_PRODUCT_ID_HI110X_DEV
#if (_PRE_OS_VERSION_RAW == _PRE_OS_VERSION)
    oal_uint16                  us_in_tbtt_adjust           =   0UL;
#endif
#endif
    hal_to_dmac_vap_stru        *pst_hal_vap;
#endif
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(0, OAM_SF_WIFI_BEACON, "{dmac_beacon_timeout_event_hander:event is null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event = frw_get_event_stru(pst_event_mem);
    pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_event->st_event_hdr.uc_vap_id);

    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_WIFI_BEACON, "{dmac_beacon_timeout_event_hander:vap is null }");

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_device = mac_res_get_dev(pst_dmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_WIFI_BEACON, "{dmac_beacon_timeout_event_hander:mac device is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_sta_pm_handle = &pst_dmac_vap->st_sta_pm_handler;
    if (OAL_FALSE == pst_mac_sta_pm_handle->en_is_fsm_attached)
    {
        OAM_WARNING_LOG0(0, OAM_SF_WIFI_BEACON, "{dmac_beacon_timeout_event_hander:pm fsm not attached.}");
        return OAL_FAIL;
    }

    if (MAC_SCAN_STATE_RUNNING == pst_mac_device->en_curr_scan_state)
    {
        pst_mac_sta_pm_handle->en_beacon_counting = OAL_FALSE;
        return OAL_SUCC;
    }

#ifndef HI110x_EDA
#ifdef _PRE_PRODUCT_ID_HI110X_DEV
#if (_PRE_OS_VERSION_RAW == _PRE_OS_VERSION)
    PM_Driver_Cbb_Mac_WlanRxTimeOutAdjust(&us_in_tbtt_adjust, &us_beacon_timeout_adjust);
#endif
#endif

    hal_set_beacon_timeout_val(pst_dmac_vap->pst_hal_device, pst_dmac_vap->us_beacon_timeout + us_beacon_timeout_adjust);

    pst_hal_vap = pst_dmac_vap->pst_hal_vap;

    /*ÿ�η���beacon timeout,���ú�����offset��ǰ,�յ�beacon�漴�ָ�*/
    hal_pm_set_tbtt_offset(pst_hal_vap, DMAC_BEACON_TIMEOUT_FIX_OFFSET);

    /* ����beacon��ʱ���� */
    pst_dmac_vap->bit_beacon_timeout_times++;

#ifdef _PRE_WLAN_FEATURE_SINGLE_RF_RX_BCN
    /* �����ղ���beacon֡����������beacon����ͨ��Ϊdev������� */
    if (pst_dmac_vap->bit_beacon_timeout_times > ((pst_dmac_vap->uc_bcn_tout_max_cnt + 1) >> 1)
    && pst_hal_vap->st_pm_info.uc_bcn_rf_chain != pst_dmac_vap->pst_hal_device->st_cfg_cap_info.uc_rf_chain)
    {
        hal_pm_set_bcn_rf_chain(pst_hal_vap, pst_dmac_vap->pst_hal_device->st_cfg_cap_info.uc_rf_chain);
        pst_mac_sta_pm_handle->aul_pmDebugCount[PM_MSG_BCN_TIMEOUT_SET_RX_CHAIN]++;
    }
#endif
#endif

    /* ����beacon��ʱ���� */
    if (pst_mac_sta_pm_handle->en_beacon_counting)
    {
        pst_mac_sta_pm_handle->aul_pmDebugCount[PM_MSG_BEACON_TIMEOUT_CNT]++;
        pst_mac_sta_pm_handle->en_beacon_counting = OAL_FALSE;
    }

    if (pst_mac_sta_pm_handle->ul_psm_pkt_cnt)
    {
        return OAL_SUCC;
    }

    /* p2p ������noa opppsʱbeacon��ʱ��Ͷ˯��Ʊ,����noa oppps�ڼ�ǳ˯ */
    if (OAL_TRUE == (oal_uint8)IS_P2P_PS_ENABLED(pst_dmac_vap))
    {
        return OAL_SUCC;
    }

    /* �͹���ģʽ�²Ż���beacon timeout ˯�� */
    if (WLAN_MIB_PWR_MGMT_MODE_PWRSAVE == (mac_mib_get_powermanagementmode(&(pst_dmac_vap->st_vap_base_info))))
    {
        /* ����beacon��ʱ������ͶƱ˯�ļ��� */
        pst_mac_sta_pm_handle->aul_pmDebugCount[PM_MSG_BCN_TOUT_SLEEP]++;
        if (OAL_TRUE == dmac_psm_is_tid_queues_empty(pst_dmac_vap))
        {
            dmac_pm_sta_post_event(pst_dmac_vap, STA_PWR_EVENT_BEACON_TIMEOUT, 0, OAL_PTR_NULL);
        }
    }
#endif

    return OAL_SUCC;
}

#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
#ifdef _PRE_WLAN_MAC_BUGFIX_TSF_SYNC
extern oal_uint64 g_ull_sta0_tsf_timestamp;
extern oal_uint64 g_ull_sta1_tsf_timestamp;

oal_void dmac_get_tsf_from_bcn(dmac_vap_stru *pst_dmac_vap, oal_netbuf_stru *pst_netbuf)
{
    oal_uint32                          ul_beacon_timestamp_low    = 0;
    oal_uint64                          ul_beacon_timestamp_high   = 0;
    oal_uint64                          ul_beacon_timestamp        = 0;
    oal_uint8                          *puc_payload                = OAL_PTR_NULL;

    puc_payload = OAL_NETBUF_PAYLOAD(pst_netbuf);

    ul_beacon_timestamp_low  =  puc_payload[0];
    ul_beacon_timestamp_low |= (puc_payload[1] << 8);
    ul_beacon_timestamp_low |= (puc_payload[2] << 16);
    ul_beacon_timestamp_low |= (puc_payload[3] << 24);

    ul_beacon_timestamp_high  =  puc_payload[4];
    ul_beacon_timestamp_high |= (puc_payload[5] << 8);
    ul_beacon_timestamp_high |= (puc_payload[6] << 16);
    ul_beacon_timestamp_high |= ((oal_uint32)puc_payload[7] << 24);

    ul_beacon_timestamp  = ul_beacon_timestamp_low;
    ul_beacon_timestamp |= (ul_beacon_timestamp_high << 32);

    if ((IS_LEGACY_VAP(&pst_dmac_vap->st_vap_base_info)))
    {
        g_ull_sta0_tsf_timestamp  = ul_beacon_timestamp;
    }
    else
    {
        g_ull_sta1_tsf_timestamp  = ul_beacon_timestamp;
    }
}
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_TSF_SYNC

oal_void dmac_sync_tsf_by_bcn(dmac_vap_stru *pst_dmac_vap, oal_netbuf_stru *pst_netbuf)
{
    oal_uint16                          us_beacon_interval         = 0;
    oal_uint32                          ul_beacon_timestamp_low    = 0;
    oal_uint64                          ul_beacon_timestamp_high   = 0;
    oal_uint64                          ul_beacon_timestamp        = 0;
    oal_uint8                           *puc_payload			   = OAL_PTR_NULL;

    if (pst_dmac_vap->uc_beacon_miss_cnt != 0)
    {
        pst_dmac_vap->uc_beacon_miss_cnt--;  /* dec after the beacon has been received */
    }

    puc_payload         = OAL_NETBUF_PAYLOAD(pst_netbuf);

    /*get the timestamp and beacon interval*/
    ul_beacon_timestamp_low  = puc_payload[0];
    ul_beacon_timestamp_low |= (puc_payload[1] << 8);
    ul_beacon_timestamp_low |= (puc_payload[2] << 16);
    ul_beacon_timestamp_low |= (puc_payload[3] << 24);

    ul_beacon_timestamp_high  =  puc_payload[4];
    ul_beacon_timestamp_high |= (puc_payload[5] << 8);
    ul_beacon_timestamp_high |= (puc_payload[6] << 16);
    /*lint -e647*/
    ul_beacon_timestamp_high |= (puc_payload[7] << 24);
    /*lint +e647*/

    /* ����keep alive��ʱ֮�� */
    //g_stKeepAliveTime.ulLow  = beacon_timestamp_low;
    //g_stKeepAliveTime.ulHigh = beacon_timestamp_high;

    ul_beacon_timestamp  = ul_beacon_timestamp_low;

    /*lint -e647*/
    ul_beacon_timestamp |= (ul_beacon_timestamp_high << 32);
    /*lint +e647*/

    us_beacon_interval   = puc_payload[MAC_TIME_STAMP_LEN];
    us_beacon_interval  |= (puc_payload[MAC_TIME_STAMP_LEN + 1] << 8);


    if (0 != pst_dmac_vap->uc_beacon_miss_cnt)
    {

        //OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR, "{dmac_sync_tsf_by_bcn::beacon miss cnt:[%d]",pst_dmac_vap->uc_beacon_miss_cnt);
        /*if beacon is missed,then clear saved*/
        pst_dmac_vap->ul_old_beacon_timestamp = 0;
        pst_dmac_vap->uc_beacon_miss_cnt = 0;
    }
    else if ((0 == pst_dmac_vap->uc_beacon_miss_cnt) && (0 == pst_dmac_vap->ul_old_beacon_timestamp))
    {
        /*saved the first timestamp*/
        pst_dmac_vap->ul_old_beacon_timestamp = ul_beacon_timestamp;
    }
    else if ((0 == pst_dmac_vap->uc_beacon_miss_cnt) && (0 != pst_dmac_vap->ul_old_beacon_timestamp))
    {
        /* cal the SyncTsfValue after receive the second beacon beacon֡���ж�������Ҫ����5ms���� */
        if (((oal_uint64)((ul_beacon_timestamp >> 10) - (pst_dmac_vap->ul_old_beacon_timestamp >> 10))) < (oal_uint64)(us_beacon_interval - 5))
        {
            pst_dmac_vap->us_sync_tsf_value = us_beacon_interval - (oal_uint16)((oal_uint64)((ul_beacon_timestamp >> 10) - ((pst_dmac_vap->ul_old_beacon_timestamp) >> 10))) + 70;
            OAM_INFO_LOG4(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR, "{dmac_sync_tsf_by_bcn::sync value:[%d],beacon interval:[%d],\
                                         new time:[%lu],old time:[%lu]}",pst_dmac_vap->us_sync_tsf_value,us_beacon_interval,(ul_beacon_timestamp),((pst_dmac_vap->ul_old_beacon_timestamp)));

        }
        else
        {
            pst_dmac_vap->us_sync_tsf_value = 0;
        }


        /*clear after fix once*/
        pst_dmac_vap->ul_old_beacon_timestamp  = ul_beacon_timestamp;
    }
}
#endif /* TSF_SYNC_OPT */


 oal_uint32 dmac_protection_set_autoprot(mac_vap_stru *pst_mac_vap, oal_switch_enum_uint8 en_mode)
{
    oal_uint32      ul_ret = OAL_SUCC;
    dmac_user_stru *pst_dmac_user;

    if (OAL_PTR_NULL == pst_mac_vap)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (OAL_SWITCH_OFF == en_mode)
    {
        pst_mac_vap->st_protection.bit_auto_protection = OAL_SWITCH_OFF;
        ul_ret = dmac_set_protection_mode(pst_mac_vap, WLAN_PROT_NO);
    }
    else
    {
        pst_mac_vap->st_protection.bit_auto_protection = OAL_SWITCH_ON;
        /*VAP Ϊ AP�����*/
        if (WLAN_VAP_MODE_BSS_AP == pst_mac_vap->en_vap_mode)
        {
            ul_ret = dmac_protection_update_mode_ap(pst_mac_vap);
        }
        /*VAP Ϊ STA�����*/
        else if (WLAN_VAP_MODE_BSS_STA == pst_mac_vap->en_vap_mode)
        {
            pst_dmac_user = mac_res_get_dmac_user(pst_mac_vap->us_assoc_vap_id); /*user�������AP����Ϣ*/
            if (OAL_PTR_NULL == pst_dmac_user)
            {
                return OAL_ERR_CODE_PTR_NULL;
            }

            ul_ret = dmac_protection_update_mode_sta(pst_mac_vap, pst_dmac_user);
        }
    }

    return ul_ret;
}


OAL_STATIC oal_uint32 dmac_protection_sync_data(mac_vap_stru *pst_mac_vap)
{
    mac_h2d_protection_stru           st_h2d_prot;
    oal_uint32                        ul_ret = OAL_SUCC;

    OAL_MEMZERO(&st_h2d_prot, OAL_SIZEOF(st_h2d_prot));

    oal_memcopy((oal_uint8*)&st_h2d_prot.st_protection, (oal_uint8*)&pst_mac_vap->st_protection,
              OAL_SIZEOF(mac_protection_stru));

    st_h2d_prot.en_dot11HTProtection         = mac_mib_get_HtProtection(pst_mac_vap);
    st_h2d_prot.en_dot11RIFSMode             = mac_mib_get_RifsMode(pst_mac_vap);

    ul_ret = dmac_protection_sync_event(pst_mac_vap, &st_h2d_prot);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{dmac_protection_sync_data::dmac_send_sys_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}


/*lint -save -e438 */
oal_uint32 dmac_set_protection_mode(mac_vap_stru *pst_mac_vap, wlan_prot_mode_enum_uint8 en_prot_mode)
{
    dmac_vap_stru                   *pst_dmac_vap;
    oal_uint32                       ul_ret = OAL_SUCC;

    pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_set_protection_mode::pst_dmac_vap null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    /*��ͬ�ı���ģʽ�Ѿ������ã�ֱ�ӷ���*/
    if (en_prot_mode == pst_mac_vap->st_protection.en_protection_mode)
    {
        return ul_ret;
    }

    /*�ر�֮ǰ�ı���ģʽ*/
    if (WLAN_PROT_ERP == pst_mac_vap->st_protection.en_protection_mode)
    {
        ul_ret = dmac_protection_set_erp_protection(pst_mac_vap, OAL_SWITCH_OFF);
        if(OAL_SUCC != ul_ret)
        {
            return ul_ret;
        }
    }
    else if (WLAN_PROT_HT == pst_mac_vap->st_protection.en_protection_mode)
    {
        ul_ret = dmac_protection_set_ht_protection(pst_mac_vap, OAL_SWITCH_OFF);
        if(OAL_SUCC != ul_ret)
        {
            return ul_ret;
        }
    }
    else
    {
        /*GF�������ޱ�������������*/
    }

    pst_mac_vap->st_protection.en_protection_mode = en_prot_mode;

    /*�����µı���ģʽ*/
    if (WLAN_PROT_ERP == en_prot_mode)
    {
        ul_ret = dmac_protection_set_erp_protection(pst_mac_vap, OAL_SWITCH_ON);
        if(OAL_SUCC != ul_ret)
        {
            return ul_ret;
        }
    }
    else if (WLAN_PROT_HT == en_prot_mode)
    {
        ul_ret = dmac_protection_set_ht_protection(pst_mac_vap, OAL_SWITCH_ON);
        if(OAL_SUCC != ul_ret)
        {
            return ul_ret;
        }
    }
    else
    {
        /*GF�������ޱ�������������*/
    }

    /*ֻ��AP��Ҫ�Ĳ���*/
    if(WLAN_VAP_MODE_BSS_AP == pst_mac_vap->en_vap_mode)
    {
        /*ͬ��host*/
        ul_ret = dmac_protection_sync_data(pst_mac_vap);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_set_protection_mode::dmac_protection_sync_data fail.ul_ret:%u}",ul_ret);
        }
        /*�����ϻ���ʱ���Ŀ����͹ر�*/
        if((WLAN_PROT_ERP == en_prot_mode)||(WLAN_PROT_HT == en_prot_mode))
        {
            /* ����OBSS�����ϻ���ʱ�� */
            dmac_protection_start_timer(pst_dmac_vap);
        }
        else
        {
            /* �ر�OBSS�����ϻ���ʱ�� */
            dmac_protection_stop_timer(pst_dmac_vap);
        }
    }

    /*��������֡�����֡�뱣��������صķ��Ͳ���*/

    return dmac_tx_update_protection_all_txop_alg(pst_dmac_vap);
}
/*lint -restore */

 
oal_void  dmac_sta_up_update_protection_mode(dmac_vap_stru *pst_dmac_vap,mac_user_stru *pst_mac_user)
{
    mac_vap_stru                        *pst_mac_vap = &(pst_dmac_vap->st_vap_base_info);
    wlan_prot_mode_enum_uint8            en_protection_mode;

    /*����������Ʋ�����,ֱ�ӷ���*/
    if (OAL_SWITCH_OFF == mac_vap_protection_autoprot_is_enabled(pst_mac_vap))
    {
        return;
    }

    en_protection_mode = mac_vap_get_user_protection_mode(pst_mac_vap, pst_mac_user);

    dmac_set_protection_mode(pst_mac_vap, en_protection_mode);
}


oal_uint32 dmac_protection_update_mib_ap(mac_vap_stru *pst_mac_vap)
{
    mac_protection_stru *pst_protection = OAL_PTR_NULL;
    oal_bool_enum_uint8  en_lsig_txop_full_protection_activated;
    oal_bool_enum_uint8  en_rifs_mode = OAL_FALSE;
    wlan_mib_ht_protection_enum_uint8  en_ht_protection = WLAN_MIB_HT_PROTECTION_BUTT;
    wlan_mib_ht_protection_enum_uint8  old_en_ht_protection;

    pst_protection = &(pst_mac_vap->st_protection);

    /*����vap��en_dot11LSIGTXOPFullProtectionActivated�ֶ�*/
    en_lsig_txop_full_protection_activated = ((0 != pst_mac_vap->us_user_nums) && (0 == pst_protection->uc_sta_no_lsig_txop_num)) ? OAL_TRUE: OAL_FALSE;

    mac_mib_set_LsigTxopFullProtectionActivated(pst_mac_vap, en_lsig_txop_full_protection_activated);

    /*����vap��en_dot11HTProtection��en_dot11RIFSMode�ֶ�*/

    /*The HT Protection field may be set to 20 MHz protection mode only if the following are true:
     * All STAs detected (by any means) in the primary channel and all the secondary channel are HT STAs
     * and all STAs that are memberThis BSS is a 20/40 MHz BSS,
     * and There is at least one 20 MHz HT STA associated with this BSS.
     */
    if ((OAL_FALSE == pst_protection->bit_obss_non_erp_present)
        &&(OAL_FALSE == pst_protection->bit_obss_non_ht_present)
        &&(0 == pst_protection->uc_sta_non_ht_num)
        &&(0 == pst_protection->uc_sta_non_erp_num)
        &&(WLAN_BAND_WIDTH_20M != pst_mac_vap->st_channel.en_bandwidth)
        &&(0 != pst_protection->uc_sta_20M_only_num))
    {
        en_ht_protection = WLAN_MIB_HT_20MHZ_PROTECTION;
        en_rifs_mode     = OAL_TRUE;
    }
    /*The HT Protection field may be set to nonmember protection mode only if the following are true:
     * A non-HT STA is detected (by any means) in either the primary or the secondary channel or in both
     * the primary and secondary channels, that is not known by the transmitting STA to be a member of
     * this BSS, and
     * All STAs that are known by the transmitting STA to be a member of this BSS are HT STAs.
     */
    else if ((OAL_TRUE == pst_protection->bit_obss_non_ht_present)
            &&(0 == pst_protection->uc_sta_non_ht_num)
            &&(0 == pst_protection->uc_sta_non_erp_num))
    {
        en_ht_protection = WLAN_MIB_HT_NONMEMBER_PROTECTION;
        en_rifs_mode     = OAL_FALSE;
    }

    /*The HT Protection field may be set to no protection mode only if the following are true:
     * All STAs detected (by any means) in the primary or the secondary channel are HT STAs, and
     * All STAs that are known by the transmitting STA to be a member of this BSS are either
     * 20/40 MHz HT STAs in a 20/40 MHz BSS, or
     * 20 MHz HT STAs in a 20 MHz BSS.
     */
    else if((OAL_FALSE == pst_protection->bit_obss_non_erp_present)
            &&(OAL_FALSE == pst_protection->bit_obss_non_ht_present)
            &&(0 == pst_protection->uc_sta_non_ht_num)
            &&(0 == pst_protection->uc_sta_non_erp_num))
    {
        if((WLAN_BAND_WIDTH_20M == pst_mac_vap->st_channel.en_bandwidth)
            ||((WLAN_BAND_WIDTH_20M != pst_mac_vap->st_channel.en_bandwidth)&&(0 == pst_protection->uc_sta_20M_only_num)))
        {
            en_ht_protection = WLAN_MIB_HT_NO_PROTECTION;
            en_rifs_mode     = OAL_TRUE;
        }
    }
    else
    {
        en_ht_protection = WLAN_MIB_HT_NON_HT_MIXED;
        en_rifs_mode     = OAL_FALSE;
    }

    old_en_ht_protection = mac_mib_get_HtProtection(pst_mac_vap);

    if(old_en_ht_protection != en_ht_protection)
    {
        mac_mib_set_HtProtection(pst_mac_vap, en_ht_protection);
        mac_mib_set_RifsMode(pst_mac_vap, en_rifs_mode);

        dmac_protection_sync_data(pst_mac_vap);
    }

    return dmac_protection_update_mode_ap(pst_mac_vap);
}


oal_uint32 dmac_protection_update_mode_ap(mac_vap_stru *pst_mac_vap)
{
    wlan_prot_mode_enum_uint8 en_protection_mode = WLAN_PROT_NO;
    mac_protection_stru      *pst_protection     = OAL_PTR_NULL;

    pst_protection = &(pst_mac_vap->st_protection);

    /*����������Ʋ����ã� ֱ�ӷ���*/
    if (OAL_SWITCH_OFF == mac_vap_protection_autoprot_is_enabled(pst_mac_vap))
    {
        return OAL_SUCC;
    }

    /*��2GƵ���£������non erpվ����AP������ ����OBSS�д���non erpվ�㣬 ����Ϊerp����*/
    if ((WLAN_BAND_2G == pst_mac_vap->st_channel.en_band)
         && ((0 != pst_protection->uc_sta_non_erp_num) || (OAL_TRUE == pst_protection->bit_obss_non_erp_present)))
    {
        en_protection_mode = WLAN_PROT_ERP;
    }
    /*�����non htվ����AP������ ����OBSS�д���non htվ�㣬 ����Ϊht����*/
    else if ((0 != pst_protection->uc_sta_non_ht_num) || (OAL_TRUE == pst_protection->bit_obss_non_ht_present))
    {
        en_protection_mode = WLAN_PROT_HT;
    }
    /*�����non gfվ����AP������ ����Ϊgf����*/
    else if (0 != pst_protection->uc_sta_non_gf_num)
    {
        en_protection_mode = WLAN_PROT_GF;
    }
    /*ʣ�µ������������*/
    else
    {
        en_protection_mode = WLAN_PROT_NO;
    }

    /*���þ��屣��ģʽ*/
    return dmac_set_protection_mode(pst_mac_vap, en_protection_mode);
}


oal_uint32 dmac_protection_update_mode_sta(mac_vap_stru *pst_mac_vap_sta, dmac_user_stru *pst_dmac_user)
{
    wlan_prot_mode_enum_uint8 en_protection_mode = WLAN_PROT_NO;

    if ((OAL_PTR_NULL == pst_mac_vap_sta) || (OAL_PTR_NULL == pst_dmac_user))
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    /*����������Ʋ����ã� ֱ�ӷ���*/
    if (OAL_SWITCH_OFF == mac_vap_protection_autoprot_is_enabled(pst_mac_vap_sta))
    {
        return OAL_SUCC;
    }

    en_protection_mode = mac_vap_get_user_protection_mode(pst_mac_vap_sta, &(pst_dmac_user->st_user_base_info));

    /*���þ��屣��ģʽ*/
    return dmac_set_protection_mode(pst_mac_vap_sta, en_protection_mode);
}


oal_void dmac_protection_obss_aging_ap(mac_vap_stru *pst_mac_vap)
{
    oal_bool_enum em_update_protection = OAL_FALSE; /*ָʾ�Ƿ���Ҫ����vap��protection*/

    /*����ERP�ϻ�����*/
    if (OAL_TRUE == pst_mac_vap->st_protection.bit_obss_non_erp_present)
    {
        pst_mac_vap->st_protection.uc_obss_non_erp_aging_cnt++;
        if (pst_mac_vap->st_protection.uc_obss_non_erp_aging_cnt >= WLAN_PROTECTION_NON_ERP_AGING_THRESHOLD)
        {
            pst_mac_vap->st_protection.bit_obss_non_erp_present = OAL_FALSE;
            em_update_protection = OAL_TRUE;
            pst_mac_vap->st_protection.uc_obss_non_erp_aging_cnt = 0;
        }
    }

    /*����HT�ϻ�����*/
    if (OAL_TRUE == pst_mac_vap->st_protection.bit_obss_non_ht_present)
    {
        pst_mac_vap->st_protection.uc_obss_non_ht_aging_cnt++;

        if (pst_mac_vap->st_protection.uc_obss_non_ht_aging_cnt >= WLAN_PROTECTION_NON_HT_AGING_THRESHOLD)
        {
            pst_mac_vap->st_protection.bit_obss_non_ht_present = OAL_FALSE;
            em_update_protection = OAL_TRUE;
            pst_mac_vap->st_protection.uc_obss_non_ht_aging_cnt = 0;
        }
    }

    /*��Ҫ���±���ģʽ*/
    if(OAL_TRUE == em_update_protection)
    {
        dmac_protection_update_mib_ap(pst_mac_vap);
    }

}


oal_uint32  dmac_protection_obss_update_timer(void *p_arg)
{
    dmac_vap_stru        *pst_dmac_vap;

    if (OAL_PTR_NULL == p_arg)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{dmac_protection_obss_update_timer::p_arg null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_dmac_vap = (dmac_vap_stru *)p_arg;

    dmac_protection_obss_aging_ap(&pst_dmac_vap->st_vap_base_info);

    return OAL_SUCC;
}


oal_void dmac_protection_start_timer(dmac_vap_stru  *pst_dmac_vap)
{
    /* ����OBSS�����ϻ���ʱ�� ��ʱ���ѿ����������ٿ��� */
    if (OAL_FALSE == pst_dmac_vap->st_obss_aging_timer.en_is_registerd)
    {
        FRW_TIMER_CREATE_TIMER(&(pst_dmac_vap->st_obss_aging_timer),
                           dmac_protection_obss_update_timer,
                           WLAN_USER_AGING_TRIGGER_TIME,                    /* 5000ms����һ�� */
                           (oal_void *)pst_dmac_vap,
                           OAL_TRUE,
                           OAM_MODULE_ID_DMAC,
                           pst_dmac_vap->st_vap_base_info.ul_core_id);

    }
}


oal_void dmac_protection_stop_timer(dmac_vap_stru  *pst_dmac_vap)
{
    /* �ر�OBSS�����ϻ���ʱ�� ��ʱ���ѹرգ������ٹر� */
    if(OAL_FALSE == pst_dmac_vap->st_obss_aging_timer.en_is_registerd)
    {
        return;
    }
    FRW_TIMER_DESTROY_TIMER(&(pst_dmac_vap->st_obss_aging_timer));
}


OAL_STATIC oal_bool_enum dmac_ap_is_olbc_present(oal_uint8 *puc_payload, oal_uint32 ul_payload_len)
{
    oal_uint8               uc_num_rates = 0;
    mac_erp_params_stru    *pst_erp_params;
    oal_uint8              *puc_ie       = OAL_PTR_NULL;


    if (ul_payload_len <= (MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN + MAC_CAP_INFO_LEN))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{dmac_ap_is_olbc_present::payload_len[%d]}", ul_payload_len);
        return OAL_FALSE;
    }

    ul_payload_len -= (MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN + MAC_CAP_INFO_LEN);
    puc_payload    += (MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN + MAC_CAP_INFO_LEN);

    puc_ie = mac_find_ie(MAC_EID_ERP, puc_payload, (oal_int32)ul_payload_len);
    if (OAL_PTR_NULL != puc_ie)
    {
        pst_erp_params = (mac_erp_params_stru *)(puc_ie + MAC_IE_HDR_LEN);
        /*���use protection����Ϊ1�� ����TRUE*/
        if (OAL_TRUE == pst_erp_params->bit_non_erp)
        {
            return OAL_TRUE;
        }
    }

    puc_ie = mac_find_ie(MAC_EID_RATES, puc_payload, (oal_int32)ul_payload_len);
    if (OAL_PTR_NULL != puc_ie)
    {
        uc_num_rates += puc_ie[1];
    }

    puc_ie = mac_find_ie(MAC_EID_XRATES, puc_payload, (oal_int32)ul_payload_len);
    if (OAL_PTR_NULL != puc_ie)
    {
        uc_num_rates += puc_ie[1];
    }

    /*����������ʼ���ĿС�ڻ����11bЭ��֧�ֵ�������ʼ������� ����TRUE*/
    if(uc_num_rates <= MAC_NUM_DR_802_11B)
    {
        OAM_INFO_LOG1(0, OAM_SF_ANY, "{dmac_ap_is_olbc_present::invalid uc_num_rates[%d].}", uc_num_rates);
        return OAL_TRUE;
    }

    return OAL_FALSE;
}


OAL_STATIC oal_void dmac_ap_process_obss_erp_ie(
                dmac_vap_stru                  *pst_dmac_vap,
                oal_uint8                      *puc_payload,
                oal_uint32                      ul_payload_len)
{
    /*����non erpվ��*/
    if (OAL_TRUE == dmac_ap_is_olbc_present(puc_payload, ul_payload_len))
    {
        pst_dmac_vap->st_vap_base_info.st_protection.bit_obss_non_erp_present = OAL_TRUE;
        pst_dmac_vap->st_vap_base_info.st_protection.uc_obss_non_ht_aging_cnt = 0;
    }
}


OAL_STATIC oal_uint32 dmac_ap_get_beacon_channel_num(oal_uint8 *puc_payload, oal_uint32 ul_payload_len,
    oal_uint32 *pul_primary_channel, oal_uint32 *pul_secondary_channel, oal_uint8 uc_curr_chan)
{
    oal_uint8              *puc_ie;
    mac_ht_opern_stru      *pst_ht_opern;
    mac_sec_ch_off_enum     uc_secondary_chan_offset;
    oal_uint16              us_offset =  MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN + MAC_CAP_INFO_LEN;

    //HT����£�ͨ��OPERATION�ֶεõ������ŵ���
    puc_ie = mac_find_ie(MAC_EID_HT_OPERATION, puc_payload + us_offset, (oal_int32)(ul_payload_len - us_offset));
    if ((OAL_PTR_NULL != puc_ie) && (puc_ie[1] >= 1))
    {
         pst_ht_opern = (mac_ht_opern_stru *)(puc_ie + MAC_IE_HDR_LEN);
        *pul_primary_channel         = (oal_uint32)pst_ht_opern->uc_primary_channel;
         uc_secondary_chan_offset    = (mac_sec_ch_off_enum)pst_ht_opern->bit_secondary_chan_offset;
         if (MAC_SCB == uc_secondary_chan_offset)
         {
            if(4 > *pul_primary_channel)
            {
                *pul_secondary_channel = *pul_primary_channel - 4;
            }
            else
            {
                *pul_secondary_channel = 0;
            }
         }
         else if (MAC_SCA == uc_secondary_chan_offset)
         {
             *pul_secondary_channel = *pul_primary_channel + 4;
         }
         else
         {
             *pul_secondary_channel = 0;
         }

         return OAL_SUCC;
    }
    //legacy����£�ͨ��DSSS Param set ie�еõ�
    *pul_primary_channel    = mac_ie_get_chan_num(puc_payload, (oal_uint16)ul_payload_len, us_offset, uc_curr_chan);
    *pul_secondary_channel  = 0;

    return OAL_SUCC;
}


OAL_STATIC oal_bool_enum dmac_ap_is_obss_non_ht_present(oal_uint8 *puc_payload, oal_uint32 ul_payload_len)
{
    mac_ht_opern_stru      *pst_ht_opern;
    oal_uint8              *puc_ie        = OAL_PTR_NULL;

    if (ul_payload_len <= (MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN + MAC_CAP_INFO_LEN))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{dmac_ap_is_olbc_present::payload_len[%d]}", ul_payload_len);
        return OAL_FALSE;
    }

    ul_payload_len -= (MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN + MAC_CAP_INFO_LEN);
    puc_payload    += (MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN + MAC_CAP_INFO_LEN);

    puc_ie = mac_find_ie(MAC_EID_HT_OPERATION, puc_payload, (oal_int32)ul_payload_len);
    if (OAL_PTR_NULL != puc_ie)
    {
        pst_ht_opern  = (mac_ht_opern_stru *)(puc_ie + MAC_IE_HDR_LEN);
        if (OAL_TRUE == pst_ht_opern->bit_obss_nonht_sta_present)
        {
            /*���OBSS Non-HT STAs Present����Ϊ1�� ����TRUE*/
            return OAL_TRUE;
        }
    }

    puc_ie = mac_find_ie(MAC_EID_HT_CAP, puc_payload, (oal_int32)ul_payload_len);
    if (OAL_PTR_NULL != puc_ie)
    {
        /*�����HT capability��ϢԪ�أ�����FALSE*/
        return OAL_FALSE;
    }

    /*���û��HT capability��ϢԪ�أ�����TRUE*/
    return OAL_TRUE;
}


OAL_STATIC oal_void dmac_ap_process_obss_ht_operation_ie(
                dmac_vap_stru                  *pst_dmac_vap,
                oal_uint8                      *puc_payload,
                oal_uint32                      ul_payload_len)
{
    /*�������non-htվ��*/
    if (OAL_TRUE == dmac_ap_is_obss_non_ht_present(puc_payload, ul_payload_len))
    {
        pst_dmac_vap->st_vap_base_info.st_protection.bit_obss_non_ht_present = OAL_TRUE;
        pst_dmac_vap->st_vap_base_info.st_protection.uc_obss_non_ht_aging_cnt = 0;
    }
}

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST

OAL_STATIC oal_void  dmac_ap_process_obss_bandwidth(dmac_vap_stru *pst_dmac_vap, oal_uint8 *puc_payload, oal_uint16 us_payload_len)
{
    mac_device_stru             *pst_mac_device;
    mac_vap_stru                *pst_mac_vap;
    oal_uint32                   ul_pri_freq;//��ǰap�����ŵ�����Ƶ��
    oal_uint32                   ul_sec_freq = 0;
    oal_uint32                   ul_sec_freq_chan = 0;//��ǰAP�Ĵ��ŵ���
    oal_uint32                   ul_affected_start;
    oal_uint32                   ul_affected_end;
    oal_uint32                   ul_pri;//�Է�AP�����ŵ�����Ƶ��
    oal_uint32                   ul_sec;
    oal_uint32                   ul_sec_chan, ul_pri_chan;//�Է�AP���ŵ���
    wlan_channel_bandwidth_enum_uint8 en_bandwidth;//��ǰ40M�������Ŀ��40M����


    pst_mac_device = mac_res_get_dev(pst_dmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_2040,
                                 "{dmac_ap_process_obss_ht_operation_ie::pst_mac_device null.}");
        return ;
    }

    pst_mac_vap   = &pst_dmac_vap->st_vap_base_info;

    if (OAL_FALSE == mac_mib_get_2040SwitchProhibited(pst_mac_vap))
    {

        if ((WLAN_BAND_WIDTH_40PLUS == pst_mac_vap->st_channel.en_bandwidth) ||
            (WLAN_BAND_WIDTH_40MINUS == pst_mac_vap->st_channel.en_bandwidth))
        {//��ǰΪ40M����Ҫ�ж��Ƿ��е�20M
            en_bandwidth = pst_mac_vap->st_channel.en_bandwidth;
        }
        else if((WLAN_BAND_WIDTH_20M == pst_mac_vap->st_channel.en_bandwidth)
                &&((WLAN_BAND_WIDTH_40PLUS == pst_dmac_vap->en_40M_bandwidth)
                    ||(WLAN_BAND_WIDTH_40MINUS == pst_dmac_vap->en_40M_bandwidth)))
        {//��ǰΪ20M,�ж��ܷ�ָ�40M
            en_bandwidth = pst_dmac_vap->en_40M_bandwidth;
        }
        else
        {//��������������жϸ�AP�ŵ�OBSS
            return;
        }

        if (WLAN_BAND_2G == pst_dmac_vap->st_vap_base_info.st_channel.en_band)
        {
            // ��ȡ���ŵ������ŵ�����Ƶ��
            ul_pri_freq = (oal_uint32)g_dmac_ast_freq_map_2g[pst_mac_vap->st_channel.uc_chan_number - 1].us_freq;//2412 + (pst_mac_vap->st_channel.uc_chan_number - 1) * 5;

            if (WLAN_BAND_WIDTH_40PLUS == en_bandwidth)
            {
                ul_sec_freq = ul_pri_freq + 20;
            }
            else if (WLAN_BAND_WIDTH_40MINUS == en_bandwidth)
            {
                ul_sec_freq = ul_pri_freq - 20;
            }

            // 2.4G������,��ⷶΧ��40MHz��������Ƶ��Ϊ����,���Ҹ�25MHZ
            ul_affected_start   = ((ul_pri_freq + ul_sec_freq) >> 1) - 25;
            ul_affected_end     = ((ul_pri_freq + ul_sec_freq) >> 1) + 25;

            //��ȡ�Է�AP�����ŵ���
            dmac_ap_get_beacon_channel_num(puc_payload, us_payload_len, &ul_pri_chan,
                &ul_sec_chan, pst_mac_vap->st_channel.uc_chan_number);

            //����Է�APƵ��
            ul_pri = (oal_int32)g_dmac_ast_freq_map_2g[ul_pri_chan - 1].us_freq;//2412 + (st_chan_result.uc_channel_number - 1) * 5;
            ul_sec = ul_pri;
            // ��BSSΪ40MHz����,������ŵ�Ƶ��
            if (ul_sec_chan)
            {
                if (ul_sec_chan < ul_pri_chan)
                    ul_sec = ul_pri - 20;
                else
                    ul_sec = ul_pri + 20;
            }

            //�Է�AP���ڸ��ŷ�Χ�����账��
            if ((ul_pri < ul_affected_start || ul_pri > ul_affected_end) &&
                (ul_sec < ul_affected_start || ul_sec > ul_affected_end))
                return ;

            if (!(ul_pri_freq == ul_pri))//�е�Ƶ���Ҳ���ͬһ���ŵ�����ǰAP����ʹ��40M
            {
                dmac_chan_start_40M_recovery_timer(pst_dmac_vap);
                if((WLAN_BAND_WIDTH_40PLUS == pst_mac_vap->st_channel.en_bandwidth)
                    ||(WLAN_BAND_WIDTH_40MINUS == pst_mac_vap->st_channel.en_bandwidth))
                {
                    dmac_chan_multi_switch_to_20MHz_ap(pst_dmac_vap);
                }
            }
        }
        else
        {//5G
            //��ȡ��AP�Ĵ��ŵ���
            if (WLAN_BAND_WIDTH_40PLUS == en_bandwidth)
            {
                ul_sec_freq_chan = pst_mac_vap->st_channel.uc_chan_number + 4;
            }
            else if (WLAN_BAND_WIDTH_40MINUS == en_bandwidth)
            {
                ul_sec_freq_chan = pst_mac_vap->st_channel.uc_chan_number - 4;
            }
            //��ȡ�Է�AP�����ŵ���
            dmac_ap_get_beacon_channel_num(puc_payload, us_payload_len, &ul_pri_chan,
                &ul_sec_chan, pst_mac_vap->st_channel.uc_chan_number);
            if ((0 == ul_sec_chan) && (ul_sec_freq_chan == ul_pri_chan))
            {//5G������Է�Ϊ20M�����ڴ��ŵ�����ǰAP����ʹ��40M
                dmac_chan_start_40M_recovery_timer(pst_dmac_vap);
                if((WLAN_BAND_WIDTH_40PLUS == pst_mac_vap->st_channel.en_bandwidth)
                    ||(WLAN_BAND_WIDTH_40MINUS == pst_mac_vap->st_channel.en_bandwidth))
                {
                    dmac_chan_multi_switch_to_20MHz_ap(pst_dmac_vap);
                }
            }
        }
    }

}


OAL_STATIC  oal_bool_enum_uint8  dmac_ap_is_40MHz_intol_bit_set(oal_uint8 *puc_payload, oal_uint16 us_payload_len)
{
    oal_uint16   us_ht_cap_info = 0;
    oal_uint8   *puc_ht_cap     = OAL_PTR_NULL;

    if (us_payload_len <= (MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN + MAC_CAP_INFO_LEN))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{dmac_ap_is_40MHz_intol_bit_set::payload_len[%d]}", us_payload_len);
        return OAL_FALSE;
    }

    us_payload_len -= (MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN + MAC_CAP_INFO_LEN);
    puc_payload    += (MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN + MAC_CAP_INFO_LEN);

    puc_ht_cap = mac_find_ie(MAC_EID_HT_CAP, puc_payload, us_payload_len);
    if (OAL_PTR_NULL != puc_ht_cap)
    {
        us_ht_cap_info = OAL_MAKE_WORD16(puc_ht_cap[MAC_IE_HDR_LEN], puc_ht_cap[MAC_IE_HDR_LEN + 1]);

        /* Forty MHz Intolerant BIT ?1 */
        if (us_ht_cap_info & BIT14)
        {
            return OAL_TRUE;
        }
    }

    return OAL_FALSE;
}


OAL_STATIC oal_bool_enum_uint8  dmac_ap_update_2040_chan_info(
                mac_device_stru             *pst_mac_device,
                mac_vap_stru                *pst_mac_vap,
                oal_uint8                   *puc_payload,
                oal_uint16                   us_payload_len,
                oal_uint8                    uc_pri_chan_idx,
                mac_sec_ch_off_enum_uint8    en_sec_ch_offset)
{
    wlan_channel_band_enum_uint8    en_band = pst_mac_vap->st_channel.en_band;
    mac_ap_ch_info_stru            *pst_ap_ch_list = pst_mac_device->st_ap_channel_list;
    oal_bool_enum_uint8             en_status_change = OAL_FALSE;
    oal_uint8                       uc_sec_ch_idx_offset = mac_get_sec_ch_idx_offset(en_band);
    oal_uint8                       uc_sec_chan_idx = 0;
    oal_uint32                      ul_ret;

    if (OAL_TRUE == dmac_ap_is_40MHz_intol_bit_set(puc_payload, us_payload_len))
    {
        pst_mac_device->en_40MHz_intol_bit_recd = OAL_TRUE;
    }

    if (MAC_CH_TYPE_PRIMARY != pst_ap_ch_list[uc_pri_chan_idx].en_ch_type)
    {
        pst_ap_ch_list[uc_pri_chan_idx].en_ch_type = MAC_CH_TYPE_PRIMARY;
        en_status_change = OAL_TRUE;
    }

    if (MAC_SCN != en_sec_ch_offset)
    {
        if (MAC_SCA == en_sec_ch_offset)
        {
            uc_sec_chan_idx = uc_pri_chan_idx + uc_sec_ch_idx_offset;
        }
        else if (MAC_SCB == en_sec_ch_offset)
        {
            uc_sec_chan_idx = uc_pri_chan_idx - uc_sec_ch_idx_offset;
        }

        ul_ret =  mac_is_channel_idx_valid(en_band, uc_sec_chan_idx);
        if (OAL_SUCC != ul_ret)
        {
            return en_status_change;
        }

        /*lint -save -e506 */
        if (uc_sec_chan_idx >= MAC_MAX_SUPP_CHANNEL)
        {
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040,
                             "{dmac_ap_update_2040_chan_info::invalid uc_sec_chan_idx[%d].}", uc_sec_chan_idx);
            return en_status_change;
        }
        /*lint -restore */

        if (MAC_CH_TYPE_PRIMARY != pst_ap_ch_list[uc_sec_chan_idx].en_ch_type)
        {
            if (MAC_CH_TYPE_SECONDARY != pst_ap_ch_list[uc_sec_chan_idx].en_ch_type)
            {
                pst_ap_ch_list[uc_sec_chan_idx].en_ch_type = MAC_CH_TYPE_SECONDARY;
                en_status_change = OAL_TRUE;
            }
        }
    }

    /*ͬ��device�����ŵ��б�*/
    dmac_ch_status_sync_event(pst_mac_vap, pst_ap_ch_list);

    return en_status_change;
}


OAL_STATIC oal_void  dmac_ap_handle_40_intol(dmac_vap_stru *pst_dmac_vap, oal_uint8 *puc_payload, oal_uint16 us_payload_len)
{
    mac_vap_stru             *pst_mac_vap;

    pst_mac_vap = mac_res_get_mac_vap(pst_dmac_vap->st_vap_base_info.uc_vap_id);
    if (OAL_PTR_NULL == pst_mac_vap)
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_2040,
                   "{dmac_ap_handle_40_intol::pst_mac_vap null.}");
        return;
    }

    if (WLAN_BAND_2G == pst_dmac_vap->st_vap_base_info.st_channel.en_band)
    {
        if (OAL_TRUE == dmac_ap_is_40MHz_intol_bit_set(puc_payload, us_payload_len))
        {
            dmac_chan_start_40M_recovery_timer(pst_dmac_vap);
            if((WLAN_BAND_WIDTH_40PLUS == pst_mac_vap->st_channel.en_bandwidth)
                    ||(WLAN_BAND_WIDTH_40MINUS == pst_mac_vap->st_channel.en_bandwidth))
            {
                dmac_chan_multi_switch_to_20MHz_ap(pst_dmac_vap);
                return;
            }
        }
    }
}

#endif


OAL_STATIC oal_uint32 dmac_ap_get_chan_idx_of_network(
                mac_vap_stru                *pst_mac_vap,
                oal_uint8                   *puc_payload,
                oal_uint16                   us_payload_len,
                mac_channel_stru            *pst_channel,
                mac_sec_ch_off_enum_uint8   *pen_sec_ch_offset)
{
    oal_uint32                     ul_ret;
    oal_uint8                     *puc_ie           = OAL_PTR_NULL;

    pst_channel->uc_chan_idx = 0xFF;

    if (us_payload_len <= (MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN + MAC_CAP_INFO_LEN))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{dmac_ap_get_chan_idx_of_network::payload_len[%d] is not correct!}", us_payload_len);
        return OAL_FAIL;
    }

    us_payload_len -= (MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN + MAC_CAP_INFO_LEN);
    puc_payload    += (MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN + MAC_CAP_INFO_LEN);

    /*���Ȳ���beacon�е�HT��Ϣ*/
    puc_ie = mac_find_ie(MAC_EID_HT_OPERATION, puc_payload, us_payload_len);
    if (OAL_PTR_NULL != puc_ie)
    {
        pst_channel->uc_chan_number = puc_ie[MAC_IE_HDR_LEN];

        if(pst_channel->uc_chan_number <= MAC_CHANNEL_FREQ_2_BUTT)
        {
            pst_channel->en_band = WLAN_BAND_2G;
        }
        else
        {
            pst_channel->en_band = WLAN_BAND_5G;
        }

        ul_ret = mac_get_channel_idx_from_num(pst_channel->en_band, pst_channel->uc_chan_number, &pst_channel->uc_chan_idx);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                             "{dmac_ap_get_chan_idx_of_network::ht operate get channel idx failed(band [%d], channel[%d])!}",
                             pst_channel->en_band, pst_channel->uc_chan_number);
            return ul_ret;
        }

        ul_ret =  mac_is_channel_idx_valid(pst_channel->en_band, pst_channel->uc_chan_idx);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                             "{dmac_ap_get_chan_idx_of_network::channel_idx_valid failed(band [%d], channel[%d], idx[%d])!}",
                             pst_channel->en_band, pst_channel->uc_chan_number, pst_channel->uc_chan_idx);
            return ul_ret;
        }

        *pen_sec_ch_offset = puc_ie[MAC_IE_HDR_LEN + 1] & 0x03;

        return OAL_SUCC;
    }

    /*û���ҵ�HT��Ϣ��Ȼ�����beacon �е�DS��Ϣ*/
    puc_ie = mac_find_ie(MAC_EID_DSPARMS, puc_payload, us_payload_len);
    if (OAL_PTR_NULL != puc_ie)
    {
        pst_channel->uc_chan_number = puc_ie[MAC_IE_HDR_LEN];
        if(pst_channel->uc_chan_number <= MAC_CHANNEL_FREQ_2_BUTT)
        {
            pst_channel->en_band = WLAN_BAND_2G;
        }
        else
        {
            pst_channel->en_band = WLAN_BAND_5G;
        }

        ul_ret = mac_get_channel_idx_from_num(pst_channel->en_band, pst_channel->uc_chan_number, &pst_channel->uc_chan_idx);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                             "{dmac_ap_get_chan_idx_of_network::ht operate get channel idx failed(band [%d], channel[%d])!}",
                             pst_channel->en_band, pst_channel->uc_chan_number);
            return ul_ret;
        }

        ul_ret =  mac_is_channel_idx_valid(pst_channel->en_band, pst_channel->uc_chan_idx);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                             "{dmac_ap_get_chan_idx_of_network::channel_idx_valid failed(2.4G, channel[%d], idx[%d])!}",
                             pst_channel->uc_chan_number, pst_channel->uc_chan_idx);
            return ul_ret;
        }

        return OAL_SUCC;
    }

    OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY,"{dmac_ap_get_chan_idx_of_network::beacon do not have the info of channel!}");

    return OAL_FAIL;
}


OAL_STATIC oal_void dmac_ap_store_network(mac_device_stru *pst_mac_device, oal_netbuf_stru *pst_netbuf)
{
    mac_bss_id_list_stru       *pst_bss_id_list = &pst_mac_device->st_bss_id_list;
    dmac_rx_ctl_stru           *pst_rx_ctrl = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
    oal_uint8                   auc_network_bssid[WLAN_MAC_ADDR_LEN] = {0};
    oal_bool_enum_uint8         en_already_present = OAL_FALSE;
    oal_uint8                   uc_loop;
    mac_ieee80211_frame_stru   *pst_frame_hdr;

    pst_frame_hdr = (mac_ieee80211_frame_stru *)mac_get_rx_cb_mac_hdr(&(pst_rx_ctrl->st_rx_info));
    /* ��ȡ֡���е�BSSID */
    mac_get_bssid((oal_uint8 *)pst_frame_hdr, auc_network_bssid);

    /* ���Թ㲥BSSID */
    if (0 == oal_compare_mac_addr(BROADCAST_MACADDR, auc_network_bssid))
    {
        return;
    }

    /* �ж��Ƿ��Ѿ������˸�BSSID */
    for (uc_loop = 0; (uc_loop < pst_bss_id_list->us_num_networks) && (uc_loop < WLAN_MAX_SCAN_BSS_PER_CH); uc_loop++)
    {
        if (0 == oal_compare_mac_addr(pst_bss_id_list->auc_bssid_array[uc_loop], auc_network_bssid))
        {
            en_already_present = OAL_TRUE;
            break;
        }
    }

    /* ����һ���µ�BSS��֡�������BSSID */
    if ((OAL_FALSE == en_already_present) && (pst_bss_id_list->us_num_networks < WLAN_MAX_SCAN_BSS_PER_CH))
    {
        oal_set_mac_addr((oal_uint8 *)pst_bss_id_list->auc_bssid_array[pst_bss_id_list->us_num_networks], (oal_uint8 *)auc_network_bssid);
        pst_bss_id_list->us_num_networks++;
    }
}


oal_void dmac_ap_wait_start_rx_obss_beacon(mac_device_stru *pst_mac_device, mac_vap_stru *pst_mac_vap, oal_netbuf_stru *pst_netbuf)
{
    oal_uint8                  *puc_payload;
    oal_uint16                  us_payload_len;
    mac_sec_ch_off_enum_uint8   en_sec_ch_offset = MAC_SCN;
    oal_uint32                  ul_ret;
    mac_channel_stru            st_channel = {0};
    mac_channel_stru           *pst_channel = &st_channel;
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    mac_rx_ctl_stru            *pst_rx_ctl;
#endif
    /* ��ȡ֡��ָ�� */
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    pst_rx_ctl    = (mac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
    puc_payload   = MAC_GET_RX_PAYLOAD_ADDR(pst_rx_ctl, pst_netbuf);
#else
    puc_payload   = OAL_NETBUF_PAYLOAD(pst_netbuf);
#endif
    /* ��ȡ֡�峤�� */
    us_payload_len   = (oal_uint16)oal_netbuf_get_len(pst_netbuf);

    /* ��֡���л�ȡ�ŵ������ʹ��ŵ�ƫ���� */
    ul_ret =dmac_ap_get_chan_idx_of_network(pst_mac_vap, puc_payload, us_payload_len,
                                            pst_channel, &en_sec_ch_offset);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_RX,
                                 "{dmac_ap_wait_start_rx_obss_beacon::hmac_ap_get_chan_idx_of_network failed[%d].}", ul_ret);
        return;
    }

   dmac_ap_store_network(pst_mac_device, pst_netbuf);

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
   dmac_ap_update_2040_chan_info(pst_mac_device, pst_mac_vap, puc_payload, us_payload_len, pst_channel->uc_chan_idx, en_sec_ch_offset);
#endif
}


oal_void  dmac_ap_up_rx_obss_beacon(dmac_vap_stru *pst_dmac_vap, oal_netbuf_stru *pst_netbuf)
{
    oal_uint8                *puc_payload;
    oal_uint16                us_payload_len;
    mac_vap_stru             *pst_mac_vap;
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    mac_rx_ctl_stru          *pst_rx_ctl;
#endif
    pst_mac_vap = mac_res_get_mac_vap(pst_dmac_vap->st_vap_base_info.uc_vap_id);
    if (OAL_PTR_NULL == pst_mac_vap)
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_2040,
                   "{dmac_ap_up_rx_obss_beacon::pst_mac_vap null.}");
        return;
    }

    /* ��ȡ֡��ָ�� */
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    pst_rx_ctl    = (mac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
    puc_payload   = MAC_GET_RX_PAYLOAD_ADDR(pst_rx_ctl, pst_netbuf);
#else
    puc_payload   = OAL_NETBUF_PAYLOAD(pst_netbuf);
#endif
    /* ��ȡ֡�峤�� */
    us_payload_len   = (oal_uint16)oal_netbuf_get_len(pst_netbuf);

    /*����ERP���*/
    if (WLAN_BAND_2G == pst_dmac_vap->st_vap_base_info.st_channel.en_band)
    {
        dmac_ap_process_obss_erp_ie(pst_dmac_vap, puc_payload, us_payload_len);
    }

    /*����HT operation���*/
    dmac_ap_process_obss_ht_operation_ie(pst_dmac_vap, puc_payload, us_payload_len);

    /*����AP�б������mib��*/
    dmac_protection_update_mib_ap(&(pst_dmac_vap->st_vap_base_info));

    /*�ж��Ƿ���Ҫ��20M*/
#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
    if((OAL_FALSE == mac_mib_get_2040SwitchProhibited(pst_mac_vap)))
    {
        dmac_ap_handle_40_intol(pst_dmac_vap, puc_payload, us_payload_len);
        dmac_ap_process_obss_bandwidth(pst_dmac_vap, puc_payload, us_payload_len);
    }
#endif
}
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

