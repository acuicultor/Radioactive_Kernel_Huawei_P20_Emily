
#ifdef __cplusplus
#if    __cplusplus
extern "C" {
#endif
#endif

#ifdef    _PRE_WLAN_FEATURE_PACKET_CAPTURE
/*****************************************************************************
  1 ͷ�ļ�����
*****************************************************************************/
#include "dmac_pkt_capture.h"
#include "dmac_alg.h"
//#include "dmac_main.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_PKT_CAPTURE_C

/*****************************************************************************
  2 ȫ�ֱ�������
*****************************************************************************/



/*****************************************************************************
  3 ����ʵ��
*****************************************************************************/


OAL_STATIC  oal_uint16  dmac_encap_ctl_rts(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_buffer, oal_uint8 *puc_mac,
                               oal_uint32 ul_data_rate, oal_uint32 ul_cts_rate, oal_uint32 ul_ack_rate)
{
    oal_uint8       *puc_origin = puc_buffer;


    /*************************************************************************/
    /*                        control Frame Format                           */
    /* --------------------------------------------------------------------  */
    /* |Frame Control|Duration  |RA    |TA      |FCS|                          */
    /* --------------------------------------------------------------------  */
    /* | 2           |2         |6     |6       |4    |                        */
    /* --------------------------------------------------------------------  */
    /*                                                                       */
    /*************************************************************************/

    /*************************************************************************/
    /*                        ����֡ͷ                                      */
    /*************************************************************************/
    /* ֡�����ֶ�ȫΪ0������type��subtype */
    mac_hdr_set_frame_control(puc_buffer, WLAN_PROTOCOL_VERSION| WLAN_FC0_TYPE_CTL| WLAN_FC0_SUBTYPE_RTS);

    /* ����duration=ack��ʱ��+cts��ʱ��+����֡��ʱ��+3*SIFSʱ�� */
    *(oal_uint16 *)(puc_buffer + 2) = (oal_uint16)(((100<<3) * 1000)/ul_data_rate + DMAC_PKT_CAP_CTS_ACK_TIME3 + DMAC_PKT_CAP_CTS_ACK_TIME3 + 3 * SIFSTIME);

    /* ���õ�ַ1 */
    oal_set_mac_addr(puc_buffer + WLAN_HDR_ADDR1_OFFSET, puc_mac);

    /* ���õ�ַ2Ϊ�Լ���MAC��ַ */
    oal_set_mac_addr(puc_buffer + WLAN_HDR_ADDR2_OFFSET, pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID);

    puc_buffer += MAC_80211_CTL_HEADER_LEN;

    return (oal_uint16)(puc_buffer - puc_origin);
}


OAL_STATIC  oal_uint16  dmac_encap_ctl_cts(oal_uint8 *puc_buffer, oal_uint8 *puc_mac,
                                           oal_uint16 us_rts_duration, oal_uint32 ul_cts_rate)
{
    oal_uint8       *puc_origin = puc_buffer;
    oal_uint8        uc_cts_time;

    /*************************************************************************/
    /*                        control Frame Format                           */
    /* --------------------------------------------------------------------  */
    /* |Frame Control|Duration  |RA    |FCS|                          */
    /* --------------------------------------------------------------------  */
    /* | 2           |2         |6     |4  |                        */
    /* --------------------------------------------------------------------  */
    /*                                                                       */
    /*************************************************************************/

    /*************************************************************************/
    /*                        ����֡ͷ                                      */
    /*************************************************************************/

    /* ֡�����ֶ�ȫΪ0������type��subtype */
    mac_hdr_set_frame_control(puc_buffer, WLAN_PROTOCOL_VERSION| WLAN_FC0_TYPE_CTL| WLAN_FC0_SUBTYPE_CTS);

    /* ����cts������, ����duration */
    if (ul_cts_rate >= DMAC_PKT_CAP_SIFS_RATE3)
    {
        uc_cts_time = DMAC_PKT_CAP_CTS_ACK_TIME3;
    }
    else if (ul_cts_rate >= DMAC_PKT_CAP_SIFS_RATE2 && ul_cts_rate < DMAC_PKT_CAP_SIFS_RATE3)
    {
        uc_cts_time = DMAC_PKT_CAP_CTS_ACK_TIME2;
    }
    else
    {
        uc_cts_time = DMAC_PKT_CAP_CTS_ACK_TIME1;
    }
    *(oal_uint16 *)(puc_buffer + 2) = (oal_uint16)(us_rts_duration - SIFSTIME - uc_cts_time);

    /* ���õ�ַ1 */
    oal_set_mac_addr(puc_buffer + WLAN_HDR_ADDR1_OFFSET, puc_mac);

    puc_buffer += (ACK_CTS_FRAME_LEN - WLAN_HDR_FCS_LENGTH);

    return (oal_uint16)(puc_buffer - puc_origin);
}


OAL_STATIC  oal_uint16  dmac_encap_ctl_ack(oal_uint8 *puc_buffer, oal_uint8 *puc_mac)
{
    oal_uint8       *puc_origin = puc_buffer;

    /*************************************************************************/
    /*                        control Frame Format                           */
    /* --------------------------------------------------------------------  */
    /* |Frame Control|Duration  |RA    |FCS|                          */
    /* --------------------------------------------------------------------  */
    /* | 2           |2         |6     |4  |                        */
    /* --------------------------------------------------------------------  */
    /*                                                                       */
    /*************************************************************************/

    /*************************************************************************/
    /*                        ����֡ͷ                                      */
    /*************************************************************************/

    /* ֡�����ֶ�ȫΪ0������type��subtype */
    mac_hdr_set_frame_control(puc_buffer, WLAN_PROTOCOL_VERSION| WLAN_FC0_TYPE_CTL| WLAN_FC0_SUBTYPE_ACK);

    /* ����durationΪ0 */
    *(oal_uint16 *)(puc_buffer + 2) = (oal_uint16)0x00;

    /* ���õ�ַ1 */
    oal_set_mac_addr(puc_buffer + WLAN_HDR_ADDR1_OFFSET, puc_mac);

    puc_buffer += (ACK_CTS_FRAME_LEN - WLAN_HDR_FCS_LENGTH);

    return (oal_uint16)(puc_buffer - puc_origin);
}



OAL_STATIC  oal_uint16  dmac_encap_ctl_ba(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_buff, oal_uint8 *puc_mac, oal_uint8 uc_lut_index)
{
    oal_uint16               us_ba_ctl = 0;
    oal_uint32               ul_addr_h;
    oal_uint32               ul_addr_l;
    oal_uint32               ul_ba_para;
    oal_uint32               ul_bitmap_h;
    oal_uint32               ul_bitmap_l;
    oal_uint32               ul_winstart ;
    hal_to_dmac_device_stru *pst_hal_device;
    dmac_device_stru        *pst_dmac_dev;

    pst_dmac_dev = dmac_res_get_mac_dev(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_dmac_dev)
    {
        OAM_ERROR_LOG0(0, OAM_SF_FRAME_FILTER, "{dmac_encap_ba::pst_dmac_dev is null}\r\n");
        return 0;
    }
    pst_hal_device = pst_dmac_dev->past_hal_device[0];
    if (OAL_PTR_NULL == pst_hal_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_FRAME_FILTER, "{dmac_encap_ba::pst_hal_device is null}\r\n");
        return 0;
    }

    /*************************************************************************/
    /*                     BlockAck  Frame Format                             */
    /* --------------------------------------------------------------------  */
    /* |Frame Control|Duration|RA|TA|BA Control|BA Info              |FCS|  */
    /* |             |        |  |  |          |                     |   |  */
    /* --------------------------------------------------------------------  */
    /* | 2           |2       |6 |6 |2         |var                  |4  |  */
    /* --------------------------------------------------------------------  */
    /*                                                                       */
    /*************************************************************************/

    /* ����subtypeΪBA */
    mac_hdr_set_frame_control(puc_buff, (oal_uint16)WLAN_PROTOCOL_VERSION | WLAN_FC0_TYPE_CTL | WLAN_FC0_SUBTYPE_BA);

    /* ����durationΪ0 */
    *(oal_uint16 *)(puc_buff + 2) = (oal_uint16)0x00;

    /* ����RA */
    oal_set_mac_addr(puc_buff + WLAN_HDR_ADDR1_OFFSET, puc_mac);

    /* ����TA */
    oal_set_mac_addr(puc_buff + WLAN_HDR_ADDR2_OFFSET, pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID);

    /* BA Control field */
    us_ba_ctl = (oal_uint16)(0 << 12);
    /* BA Policy is set to Normal Ack */
    us_ba_ctl &= ~BIT0;
    /* ��multi tid */
    us_ba_ctl &= ~BIT1;
    /* compressed bitmap */
    us_ba_ctl |= BIT2;
    /* ����BA Control */
    puc_buff[16] = us_ba_ctl & 0xFF;
    puc_buff[17] = (us_ba_ctl >> 8) & 0xFF;

    /* ��ȡӲ�����ba���� */
    hal_get_machw_ba_params(pst_hal_device,
                            uc_lut_index,
                            &ul_addr_h,
                            &ul_addr_l,
                            &ul_bitmap_h,
                            &ul_bitmap_l,
                            &ul_ba_para);

    /* ��ȡ������ssn */
    ul_winstart  = (ul_ba_para & 0x0000FFF0);
    *(oal_uint16 *)(puc_buff + 18) = (oal_uint16)ul_winstart;

    /* ��ȡ������bitmap */
    puc_buff[20] = ul_bitmap_l & 0xFF;
    puc_buff[21] = (ul_bitmap_l >> 8) & 0xFF;
    puc_buff[22] = (ul_bitmap_l >> 16) & 0xFF;
    puc_buff[23] = (ul_bitmap_l >> 24) & 0xFF;
    puc_buff[24] = ul_bitmap_h & 0xFF;
    puc_buff[25] = (ul_bitmap_h >> 8) & 0xFF;
    puc_buff[26] = (ul_bitmap_h >> 16) & 0xFF;
    puc_buff[27] = (ul_bitmap_h >> 24) & 0xFF;

    /* BA controlռ2���ֽ�, BA infoռ2+8�ܹ�10���ֽ� */
    return MAC_80211_CTL_HEADER_LEN + 2 + 10;
}


OAL_STATIC  oal_void  dmac_next_circle_buff(oal_uint16 *pus_circle_buf_index)
{
    /* ����������һ��ѭ��buffer */
    if ((WLAN_PACKET_CAPTURE_CIRCLE_BUF_DEPTH - 1) == *(pus_circle_buf_index))
    {
        (*pus_circle_buf_index) = 0;
    }
    else
    {
        (*pus_circle_buf_index)++;
    }
}


OAL_STATIC  oal_void  dmac_fill_radiotap(ieee80211_radiotap_stru *pst_radiotap, oal_uint32 ul_data_rate, hal_statistic_stru *pst_per_rate,
                                         mac_vap_stru *pst_mac_vap, dmac_rx_ctl_stru *pst_cb_ctrl, oam_ota_frame_direction_type_enum_uint8 uc_direct, oal_int8 c_rssi_ampdu)
{
    oal_uint8                        uc_flags           = DMAC_IEEE80211_RADIOTAP_F_FCS;         /* FCS at endλ��Ϊ1 */
    oal_uint8                        uc_data_rate       = 0;        /* data rate��Ϣ, 11ag��11bЭ��ʱ���ֶ���Ч */
    oal_uint16                       us_ch_freq;
    oal_uint16                       us_ch_type;
    oal_int8                         c_ssi_signal       = 0;
    oal_int8                         c_ssi_noise;
    oal_int16                        s_signal_quality;
    oal_uint8                        uc_mcs_info_known  = 0;        /* mcs��Ϣ, 11nЭ��ʱ���ֶ���Ч */
    oal_uint8                        uc_mcs_info_flags  = 0;
    oal_uint8                        uc_mcs_info_rate   = 0;
    oal_uint16                       us_vht_known       = 0;        /* vht��Ϣ, 11acЭ��ʱ���ֶ���Ч */
    oal_uint8                        uc_vht_flags       = 0;
    oal_uint8                        uc_vht_bandwidth   = 0;
    oal_uint8                        uc_vht_mcs_nss[4]  = {0};
    oal_uint8                        uc_vht_coding = 0;
    oal_uint8                        uc_vht_group_id = 0;
    oal_uint16                       us_vht_partial_aid = 0;

    mac_ieee80211_frame_stru        *pst_mac_head       = OAL_PTR_NULL;
    mac_regclass_info_stru          *pst_regdom_info    = OAL_PTR_NULL;

    /* ��дfields�ֶ����flags��Ա */
    pst_mac_head = (mac_ieee80211_frame_stru *)((oal_uint8 *)pst_radiotap + OAL_SIZEOF(ieee80211_radiotap_stru));
    if (1 == pst_per_rate->bit_preamble)
    {
        uc_flags = uc_flags | DMAC_IEEE80211_RADIOTAP_F_SHORTPRE;
    }
    if (1 == pst_mac_head->st_frame_control.bit_more_frag)
    {
        uc_flags = uc_flags | DMAC_IEEE80211_RADIOTAP_F_FRAG;
    }
    if (1 == pst_per_rate->uc_short_gi)
    {
        uc_flags = uc_flags | DMAC_IEEE80211_RADIOTAP_F_SHORTGI;
    }

    /* ��дfields�ֶ��е�������Աch_freq��ch_type��ssi_signal��ssi_noise��signal_quality */
    if (OAM_OTA_FRAME_DIRECTION_TYPE_TX == uc_direct)
    {
        pst_regdom_info = mac_get_channel_num_rc_info(pst_mac_vap->st_channel.en_band, pst_mac_vap->st_channel.uc_chan_number);
        /* ���͵�SSI���������ŵ�������涨������͹���, noiseͳһΪ-95dBm */
        if (OAL_PTR_NULL != pst_regdom_info)
        {
            c_ssi_signal = (oal_int8)pst_regdom_info->uc_max_reg_tx_pwr;
        }
        c_ssi_noise  = DMAC_PKT_CAP_TX_NOISE;
        s_signal_quality = c_ssi_signal - DMAC_PKT_CAP_SIGNAL_OFFSET;

        /* ���͵��ŵ�ֱ��ͨ��mac vap��st_channel��Ա��� */
        if (WLAN_BAND_2G == pst_mac_vap->st_channel.en_band)
        {
            us_ch_freq = 5 * pst_mac_vap->st_channel.uc_chan_number + WLAN_2G_CENTER_FREQ_BASE;
            us_ch_type = (oal_uint16)DMAC_IEEE80211_CHAN_2GHZ | (oal_uint16)DMAC_IEEE80211_CHAN_DYN;
        }
        else
        {
            us_ch_freq = 5 * pst_mac_vap->st_channel.uc_chan_number + WLAN_5G_CENTER_FREQ_BASE;
            us_ch_type = (oal_uint16)DMAC_IEEE80211_CHAN_5GHZ | (oal_uint16)DMAC_IEEE80211_CHAN_OFDM;
        }
    }
    else
    {
        if (HAL_RX_FCS_ERROR == pst_cb_ctrl->st_rx_status.bit_dscr_status)
        {
            /* CRCУ��������, BAD_FCSλ��1 */
            uc_flags = uc_flags | DMAC_IEEE80211_RADIOTAP_F_BADFCS;
            /* ����һ����FCSУ�����֡������ͳ�� */
        }
        if (0 != pst_cb_ctrl->st_rx_statistic.c_rssi_dbm)
        {
            c_ssi_signal = pst_cb_ctrl->st_rx_statistic.c_rssi_dbm;
        }
        else
        {
            c_ssi_signal = c_rssi_ampdu;
        }
        /* snr_ant����0.5dBΪ��λ��ʵ��ʹ��ǰ��Ҫ�ȳ���2���ҳ�������snr��ʾ��Χ������Сsnr��ʾ */
        /*lint -e702*/
        c_ssi_noise = c_ssi_signal - (pst_cb_ctrl->st_rx_statistic.c_snr_ant0 >> 2) - (pst_cb_ctrl->st_rx_statistic.c_snr_ant1 >> 2);
        /*lint +e702*/
        if ((c_ssi_noise >= DMAC_PKT_CAP_NOISE_MAX) || (c_ssi_noise < DMAC_PKT_CAP_NOISE_MIN))
        {
            c_ssi_noise = DMAC_PKT_CAP_NOISE_MIN;
        }
        s_signal_quality = c_ssi_signal - DMAC_PKT_CAP_SIGNAL_OFFSET;

        /* ���յ��ŵ���ͨ�����ҽ��������������֡���ŵ���� */
        if (pst_cb_ctrl->st_rx_info.uc_channel_number < 36)
        {
            us_ch_freq = 5 * pst_cb_ctrl->st_rx_info.uc_channel_number + WLAN_2G_CENTER_FREQ_BASE;
            us_ch_type = (oal_uint16)DMAC_IEEE80211_CHAN_2GHZ | (oal_uint16)DMAC_IEEE80211_CHAN_DYN;
        }
        else
        {
            us_ch_freq = 5 * pst_cb_ctrl->st_rx_info.uc_channel_number + WLAN_5G_CENTER_FREQ_BASE;
            us_ch_type = (oal_uint16)DMAC_IEEE80211_CHAN_5GHZ | (oal_uint16)DMAC_IEEE80211_CHAN_OFDM;
        }
    }

    /* ��дfields�ֶ��е�������Ϣ, ����radiotap��Ҫ��, 11nʱmcs_info��Ч��11acʱvht_info��Ч��11ag��11bʱdata_rate��Ч */
    if (WLAN_HT_PHY_PROTOCOL_MODE == pst_per_rate->un_nss_rate.st_ht_rate.bit_protocol_mode)
    {
        uc_mcs_info_known = (oal_uint8)DMAC_IEEE80211_RADIOTAP_MCS_HAVE_BW | (oal_uint8)DMAC_IEEE80211_RADIOTAP_MCS_HAVE_MCS | (oal_uint8)DMAC_IEEE80211_RADIOTAP_MCS_HAVE_GI |
                            (oal_uint8)DMAC_IEEE80211_RADIOTAP_MCS_HAVE_FMT | (oal_uint8)DMAC_IEEE80211_RADIOTAP_MCS_HAVE_FEC;
        /* ��������BWֻ��20, 20L��20U, ��û��40M��ѡ�� */
        if (WLAN_BAND_ASSEMBLE_20M != pst_per_rate->uc_bandwidth)
        {
            uc_mcs_info_flags = uc_mcs_info_flags | DMAC_IEEE80211_RADIOTAP_MCS_BW_40;
        }
        if (1 == pst_per_rate->uc_short_gi)
        {
            uc_mcs_info_flags = uc_mcs_info_flags | DMAC_IEEE80211_RADIOTAP_MCS_SGI;
        }
        if (1 == pst_per_rate->bit_preamble)
        {
            uc_mcs_info_flags = uc_mcs_info_flags | DMAC_IEEE80211_RADIOTAP_MCS_FMT_GF;
        }
        if (1 == pst_per_rate->bit_channel_code)
        {
            uc_mcs_info_flags = uc_mcs_info_flags | DMAC_IEEE80211_RADIOTAP_MCS_FEC_LDPC;
        }
        uc_mcs_info_flags = uc_mcs_info_flags | (pst_per_rate->bit_stbc << DMAC_IEEE80211_RADIOTAP_MCS_STBC_SHIFT);
        /* ��д��Ӧ��mcs������Ϣ */
        uc_mcs_info_rate = pst_per_rate->un_nss_rate.st_ht_rate.bit_ht_mcs;
    }
    else if (WLAN_VHT_PHY_PROTOCOL_MODE == pst_per_rate->un_nss_rate.st_ht_rate.bit_protocol_mode)
    {
        us_vht_known = (oal_uint16)DMAC_IEEE80211_RADIOTAP_VHT_KNOWN_STBC | (oal_uint16)DMAC_IEEE80211_RADIOTAP_VHT_KNOWN_TXOP_PS_NA | (oal_uint16)DMAC_IEEE80211_RADIOTAP_VHT_KNOWN_GI |
                       (oal_uint16)DMAC_IEEE80211_RADIOTAP_VHT_KNOWN_BEAMFORMED | (oal_uint16)DMAC_IEEE80211_RADIOTAP_VHT_KNOWN_BANDWIDTH | (oal_uint16)DMAC_IEEE80211_RADIOTAP_VHT_KNOWN_GROUP_ID | (oal_uint16)DMAC_IEEE80211_RADIOTAP_VHT_KNOWN_PARTIAL_AID;
        /* vht��Ӧ��flags��Ϣ, ����STBC��Short GI�� */
        if (0 != pst_per_rate->bit_stbc)
        {
            uc_vht_flags = uc_vht_flags | DMAC_IEEE80211_RADIOTAP_VHT_FLAG_STBC;
        }
        if (1 == pst_per_rate->uc_short_gi)
        {
            uc_vht_flags = uc_vht_flags | DMAC_IEEE80211_RADIOTAP_VHT_FLAG_SGI;
        }
        /* ��д��Ӧ��vht������Ϣ */
        if (WLAN_BAND_ASSEMBLE_20M == pst_per_rate->uc_bandwidth)
        {
            uc_vht_bandwidth = uc_vht_bandwidth | DMAC_IEEE80211_RADIOTAP_VHT_BW_20;
        }
        else if (WLAN_BAND_ASSEMBLE_40M == pst_per_rate->uc_bandwidth || WLAN_BAND_ASSEMBLE_40M_DUP == pst_per_rate->uc_bandwidth)
        {
            uc_vht_bandwidth = uc_vht_bandwidth | DMAC_IEEE80211_RADIOTAP_VHT_BW_40;
        }
        else if (WLAN_BAND_ASSEMBLE_80M == pst_per_rate->uc_bandwidth || WLAN_BAND_ASSEMBLE_80M_DUP == pst_per_rate->uc_bandwidth)
        {
            uc_vht_bandwidth = uc_vht_bandwidth | DMAC_IEEE80211_RADIOTAP_VHT_BW_80;
        }
        else
        {
            uc_vht_bandwidth = uc_vht_bandwidth | DMAC_IEEE80211_RADIOTAP_VHT_BW_160;
        }
        /* ��д��Ӧ��vht������Ϣ�����뷽ʽ */
        uc_vht_mcs_nss[0] = (pst_per_rate->un_nss_rate.st_vht_nss_mcs.bit_vht_mcs << 4) + (pst_per_rate->un_nss_rate.st_vht_nss_mcs.bit_nss_mode + 1);
        if (1 == pst_per_rate->bit_channel_code)
        {
            uc_vht_coding = uc_vht_coding | DMAC_IEEE80211_RADIOTAP_CODING_LDPC_USER0;
        }
    }
    else
    {
        uc_data_rate = (oal_uint8)(ul_data_rate / DMAC_PKT_CAP_RATE_UNIT);
    }

    pst_radiotap->st_radiotap_header.uc_it_version = PKTHDR_RADIOTAP_VERSION;
    pst_radiotap->st_radiotap_header.us_it_len     = OAL_SIZEOF(ieee80211_radiotap_stru);
    pst_radiotap->st_radiotap_header.uc_it_pad     = 0;
    pst_radiotap->st_radiotap_header.ul_it_present = (oal_uint32)DMAC_IEEE80211_RADIOTAP_TSFT | (oal_uint32)DMAC_IEEE80211_RADIOTAP_FLAGS | (oal_uint32)DMAC_IEEE80211_RADIOTAP_RATE | (oal_uint32)DMAC_IEEE80211_RADIOTAP_CHANNEL |
                    (oal_uint32)DMAC_IEEE80211_RADIOTAP_DBM_ANTSIGNAL | (oal_uint32)DMAC_IEEE80211_RADIOTAP_DBM_ANTNOISE | (oal_uint32)DMAC_IEEE80211_RADIOTAP_LOCK_QUALITY | (oal_uint32)DMAC_IEEE80211_RADIOTAP_MCS | (oal_uint32)DMAC_IEEE80211_RADIOTAP_VHT;

    pst_radiotap->st_radiotap_fields.uc_flags           = uc_flags;
    pst_radiotap->st_radiotap_fields.uc_data_rate       = uc_data_rate;
    pst_radiotap->st_radiotap_fields.us_channel_freq    = us_ch_freq;
    pst_radiotap->st_radiotap_fields.us_channel_type    = us_ch_type;
    pst_radiotap->st_radiotap_fields.c_ssi_signal       = c_ssi_signal;
    pst_radiotap->st_radiotap_fields.c_ssi_noise        = c_ssi_noise;
    pst_radiotap->st_radiotap_fields.s_signal_quality   = s_signal_quality;

    pst_radiotap->st_radiotap_fields.uc_mcs_info_known  = uc_mcs_info_known;
    pst_radiotap->st_radiotap_fields.uc_mcs_info_flags  = uc_mcs_info_flags;
    pst_radiotap->st_radiotap_fields.uc_mcs_info_rate   = uc_mcs_info_rate;

    pst_radiotap->st_radiotap_fields.us_vht_known       = us_vht_known;
    pst_radiotap->st_radiotap_fields.uc_vht_flags       = uc_vht_flags;
    pst_radiotap->st_radiotap_fields.uc_vht_bandwidth   = uc_vht_bandwidth;
    pst_radiotap->st_radiotap_fields.uc_vht_mcs_nss[0]  = uc_vht_mcs_nss[0];
    pst_radiotap->st_radiotap_fields.uc_vht_mcs_nss[1]  = uc_vht_mcs_nss[1];
    pst_radiotap->st_radiotap_fields.uc_vht_mcs_nss[2]  = uc_vht_mcs_nss[2];
    pst_radiotap->st_radiotap_fields.uc_vht_mcs_nss[3]  = uc_vht_mcs_nss[3];
    pst_radiotap->st_radiotap_fields.uc_vht_coding      = uc_vht_coding;
    pst_radiotap->st_radiotap_fields.uc_vht_group_id    = uc_vht_group_id;
    pst_radiotap->st_radiotap_fields.us_vht_partial_aid = us_vht_partial_aid;
}


OAL_STATIC  oal_void  dmac_report_packet(oal_netbuf_stru *pst_netbuf, oal_uint16 us_rt_len, oal_uint16 us_mac_header_len,
                                         oal_uint16 us_mac_body_len, oal_bool_enum_uint8 en_report_sdt_switch,
                                         oam_ota_frame_direction_type_enum_uint8 uc_direct)
{
    hw_ker_wifi_sniffer_packet_s    st_packet;

    if (OAL_TRUE == en_report_sdt_switch)
    {
        oam_report_80211_frame(BROADCAST_MACADDR, (oal_uint8 *)OAL_NETBUF_DATA(pst_netbuf) + us_rt_len, (oal_uint8)us_mac_header_len,
                               (oal_uint8 *)OAL_NETBUF_DATA(pst_netbuf) + us_rt_len + us_mac_header_len,(oal_uint16)(us_mac_header_len + us_mac_body_len),uc_direct);
        return;
    }
    st_packet.ul_manufacturerid  = 0;
    st_packet.puc_radiotapheader = (oal_uint8 *)OAL_NETBUF_DATA(pst_netbuf);
    st_packet.ul_rhlen           = (oal_uint16)us_rt_len;
    st_packet.puc_macheader      = (oal_uint8 *)(st_packet.puc_radiotapheader + st_packet.ul_rhlen);

    /* Frame */
    st_packet.ul_macheaderlen = us_mac_header_len;
    st_packet.puc_databuff    = (oal_uint8 *)(st_packet.puc_macheader + st_packet.ul_macheaderlen);
    st_packet.ul_datalen      = us_mac_body_len;

    oal_wifi_mirror_pkt(&st_packet);
}


oal_uint32 dmac_pkt_cap_tx(dmac_packet_stru *pst_dmac_packet, hal_to_dmac_device_stru *pst_hal_device)
{
    dmac_mem_circle_buf_stru        *pst_buf_current    = OAL_PTR_NULL;
    hal_tx_dscr_stru                *pst_current_dscr   = OAL_PTR_NULL;
    oal_netbuf_stru                 *pst_netbuf         = OAL_PTR_NULL;
    mac_tx_ctl_stru                 *pst_tx_ctl_cb      = OAL_PTR_NULL;
    mac_ieee80211_frame_stru        *pst_mac_frame_head = OAL_PTR_NULL;
    oal_netbuf_stru                 *pst_new_netbuf     = OAL_PTR_NULL;
    oal_uint16                       us_mac_header_len;
    oal_uint16                       us_mac_body_len;
    oal_uint32                       ul_mac_total_len;
    oal_uint8                        uc_vap_id;
    dmac_vap_stru                   *pst_dmac_vap       = OAL_PTR_NULL;
    ieee80211_radiotap_stru         *pst_radiotap       = OAL_PTR_NULL;
    hal_statistic_stru               st_per_rate;
    oal_uint32                       ul_data_rate;
    hal_tx_dscr_ctrl_one_param       st_rate_param;
    hal_tx_txop_rate_params_stru     st_phy_tx_mode;

    oal_uint8                        uc_tx_rate_rank;
    oal_uint8                        uc_tx_count;
    oal_uint8                        uc_tsf_offset;
    oal_uint32                       ul_seq;
    oal_uint8                        uc_ack_time;
    oal_uint16                       us_ba_time;
    oal_uint32                       ul_ret;

    hal_tx_dscr_stru                *pst_mpdu_dscr      = OAL_PTR_NULL;         /* ��AMPDU�зǵ�һ��MPDUʹ�õķ��������� */
    oal_netbuf_stru                 *pst_mpdu_netbuf    = OAL_PTR_NULL;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hal_device))
    {
        OAM_ERROR_LOG0(0, OAM_SF_PKT_CAP, "{dmac_pkt_cap_tx::pst_hal_device is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��dmac_device�л�ȡ��ǰҪ�����ѭ��buffer��ַ */
    pst_buf_current = (dmac_mem_circle_buf_stru *)((oal_uint8 *)pst_dmac_packet->pul_circle_buf_start + pst_dmac_packet->us_circle_buf_index*OAL_SIZEOF(dmac_mem_circle_buf_stru));

    while (OAL_PTR_NULL != pst_buf_current)
    {
        /* ��ȡѭ��buffer��ǰλ���Լ���ǰλ�õ����������ַpst_current_dscr */
        pst_buf_current = (dmac_mem_circle_buf_stru *)((oal_uint8 *)pst_dmac_packet->pul_circle_buf_start + pst_dmac_packet->us_circle_buf_index*OAL_SIZEOF(dmac_mem_circle_buf_stru));

        /* �����ǰѭ��buffer��״̬λ��Ч, �˳�ѭ�� */
        if (0 == pst_buf_current->bit_status)
        {
            break;
        }
        if (0 == pst_buf_current->uc_timestamp)
        {
            /* ѭ��buffer״̬λ��Ч, ������ʱ���Ϊ0���쳣, �ϱ�����sdt */
            OAM_ERROR_LOG0(0, OAM_SF_PKT_CAP, "{dmac_pkt_cap_tx::tx_timestamp equals zero.}");
        }
        if (OAL_PTR_NULL == pst_buf_current->pul_link_addr)
        {
            OAM_ERROR_LOG1(0, OAM_SF_PKT_CAP, "{pst_buf_current::status is valid, but the pul_link_addr is NULL. buffer idx[%d]}", pst_dmac_packet->us_circle_buf_index);
            /* ����������һ��ѭ��buffer */
            dmac_next_circle_buff(&(pst_dmac_packet->us_circle_buf_index));
            pst_buf_current->bit_status = 0;
            continue;
        }

        /* ��ȡѭ��buffer��Ӧ�ĵ�ǰ����������������Ӧ��netbuf��netbuf��Ӧ��cb�ֶ�, ��cb��Ӧ��mac֡ͷ */
        pst_current_dscr = (hal_tx_dscr_stru *)OAL_DSCR_PHY_TO_VIRT((oal_uint32)pst_buf_current->pul_link_addr);
        if (OAL_PTR_NULL == pst_current_dscr || OAL_UNLIKELY(OAL_FALSE == pst_current_dscr->bit_is_first))
        {
            OAM_WARNING_LOG0(0, OAM_SF_PKT_CAP, "{dmac_pkt_cap_tx::pst_current_dscr is NULL, or is not the first mpdu of ppdu.}");
            /* ����������һ��ѭ��buffer */
            dmac_next_circle_buff(&(pst_dmac_packet->us_circle_buf_index));
            pst_buf_current->bit_status = 0;
            continue;
        }
        pst_netbuf = pst_current_dscr->pst_skb_start_addr;
        if (OAL_PTR_NULL == pst_netbuf)
        {
            OAM_WARNING_LOG0(0, OAM_SF_PKT_CAP, "{dmac_pkt_cap_tx::pst_netbuf is NULL.}");
            /* ����������һ��ѭ��buffer */
            dmac_next_circle_buff(&(pst_dmac_packet->us_circle_buf_index));
            pst_buf_current->bit_status = 0;
            continue;
        }
        pst_tx_ctl_cb = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
        if (OAL_PTR_NULL == pst_tx_ctl_cb)
        {
            OAM_WARNING_LOG0(0, OAM_SF_PKT_CAP, "{dmac_pkt_cap_tx::pst_tx_ctl_cb is null.}");
            /* ����������һ��ѭ��buffer */
            dmac_next_circle_buff(&(pst_dmac_packet->us_circle_buf_index));
            pst_buf_current->bit_status = 0;
            continue;
        }
        pst_mac_frame_head = pst_tx_ctl_cb->pst_frame_header;
        if (OAL_PTR_NULL == pst_mac_frame_head)
        {
            OAM_WARNING_LOG0(0, OAM_SF_PKT_CAP, "{dmac_pkt_cap_tx::pst_mac_frame_head is null.}");
            /* ����������һ��ѭ��buffer */
            dmac_next_circle_buff(&(pst_dmac_packet->us_circle_buf_index));
            pst_buf_current->bit_status = 0;
            continue;
        }

        /* ͨ��hal���Ӧ����������ȡmac vap id, �ٻ��mac vap */
        dmac_tx_get_vap_id(pst_hal_device, pst_current_dscr, &uc_vap_id);
        //pst_mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(uc_vap_id);
        pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(uc_vap_id);
        if (OAL_PTR_NULL == pst_dmac_vap)
        {
            /* MAC VAP IDΪ0, ��������VAP, ��MIB�� */
            OAM_WARNING_LOG1(0, OAM_SF_PKT_CAP, "{dmac_pkt_cap_tx::dmac vap[id %d] is null.}", uc_vap_id);
            /* ����������һ��ѭ��buffer */
            dmac_next_circle_buff(&(pst_dmac_packet->us_circle_buf_index));
            continue;
        }

        /* ��ȡ����֡������, ����ݵڼ��η����ҵ���Ӧ���ʵȼ� */
        hal_tx_get_dscr_phy_tx_mode_param(pst_current_dscr, &st_phy_tx_mode);
        hal_tx_get_dscr_ctrl_one_param(pst_hal_device, pst_current_dscr, &st_rate_param);

        uc_tx_count = pst_buf_current->bit_tx_cnt;
        /* ����ѭ��buffer��¼��ǰ֡�ķ��ʹ������֡ͷ��retryλΪ1 */
        if (uc_tx_count > 0)
        {
            pst_mac_frame_head->st_frame_control.bit_retry = 1;
        }
        for (uc_tx_rate_rank = 0; uc_tx_rate_rank < HAL_TX_RATE_MAX_NUM; uc_tx_rate_rank++)
        {
            if (uc_tx_count <= st_rate_param.ast_per_rate[uc_tx_rate_rank].rate_bit_stru.bit_tx_count)
            {
                break;
            }
            uc_tx_count -= st_rate_param.ast_per_rate[uc_tx_rate_rank].rate_bit_stru.bit_tx_count;
        }
        if (HAL_TX_RATE_MAX_NUM == uc_tx_rate_rank)
        {
            uc_tx_rate_rank = HAL_TX_RATE_MAX_NUM - 1;
        }
        st_per_rate.un_nss_rate.st_ht_rate.bit_ht_mcs        = st_rate_param.ast_per_rate[uc_tx_rate_rank].rate_bit_stru.un_nss_rate.st_ht_rate.bit_ht_mcs;
        st_per_rate.un_nss_rate.st_ht_rate.bit_protocol_mode = st_rate_param.ast_per_rate[uc_tx_rate_rank].rate_bit_stru.un_nss_rate.st_ht_rate.bit_protocol_mode;
        st_per_rate.uc_short_gi      = st_rate_param.ast_per_rate[uc_tx_rate_rank].rate_bit_stru.bit_short_gi_enable;
        st_per_rate.bit_preamble     = st_rate_param.ast_per_rate[uc_tx_rate_rank].rate_bit_stru.bit_preamble_mode;
        st_per_rate.bit_stbc         = st_rate_param.ast_per_rate[uc_tx_rate_rank].rate_bit_stru.bit_stbc_mode;
        st_per_rate.bit_channel_code = st_phy_tx_mode.en_channel_code;
        st_per_rate.uc_bandwidth     = st_phy_tx_mode.en_channel_bandwidth;
        ul_ret = dmac_alg_get_rate_kbps(pst_hal_device, &st_per_rate, &ul_data_rate);
        if (OAL_ERR_CODE_PTR_NULL == ul_ret)
        {
            OAM_ERROR_LOG0(0, OAM_SF_PKT_CAP, "{dmac_pkt_cap_tx::get rate failed.}");
            /* ����ȡ����ʧ��, 2GĬ������Ϊ1Mbps, 5GĬ������Ϊ6Mbps */
            if (WLAN_BAND_2G == pst_dmac_vap->st_vap_base_info.st_channel.en_band)
            {
                ul_data_rate = DMAC_PKT_CAP_2G_RATE;
            }
            else
            {
                ul_data_rate = DMAC_PKT_CAP_5G_RATE;
            }
        }

        /* �ж��Ƿ���RTS, 0��ʾRTS, pst_new_netbuf�������ϱ���netbuf */
        if (0 == pst_buf_current->bit_frm_type)
        {
            /* ��RTS֡, ��֡˳����radiotap��֡ͷ��payload */
            pst_new_netbuf = OAL_MEM_NETBUF_ALLOC(OAL_MGMT_NETBUF, OAL_SIZEOF(ieee80211_radiotap_stru) + MAC_80211_CTL_HEADER_LEN, OAL_NETBUF_PRIORITY_MID);
            if (OAL_PTR_NULL == pst_new_netbuf)
            {
                OAM_ERROR_LOG0(0, OAM_SF_PKT_CAP, "{dmac_pkt_cap_tx::pst_new_netbuf to request memory failed.}");
                /* ����������һ��ѭ��buffer */
                dmac_next_circle_buff(&(pst_dmac_packet->us_circle_buf_index));
                pst_buf_current->bit_status = 0;
                continue;
            }
            OAL_MEMZERO((oal_uint8 *)OAL_NETBUF_DATA(pst_new_netbuf), OAL_SIZEOF(ieee80211_radiotap_stru));
            /* ��mac vapΪERP����ģʽ, ��ǰ������������Ϊ5.5Mbps, ���һ��Ϊ1Mbps, ����ǰ����Ϊ24Mbps, ���һ��Ϊ6Mbps */
            if ((HAL_TX_RATE_MAX_NUM - 1) == uc_tx_rate_rank)
            {
                if(WLAN_PROT_ERP == pst_dmac_vap->st_vap_base_info.st_protection.en_protection_mode)
                {
                    ul_data_rate = DMAC_PKT_CAP_2G_RATE;
                }
                else
                {
                    ul_data_rate = DMAC_PKT_CAP_5G_RATE;
                }
            }
            else
            {
                if(WLAN_PROT_ERP == pst_dmac_vap->st_vap_base_info.st_protection.en_protection_mode)
                {
                    ul_data_rate = (oal_uint32)(5.5 * DMAC_PKT_CAP_2G_RATE);
                }
                else
                {
                    ul_data_rate = 4 * DMAC_PKT_CAP_5G_RATE;
                }
            }
            /* ͳ��MAC֡ͷ��֡��ĳ��� */
            us_mac_header_len = dmac_encap_ctl_rts(&pst_dmac_vap->st_vap_base_info, (oal_uint8 *)OAL_NETBUF_DATA(pst_new_netbuf) + OAL_SIZEOF(ieee80211_radiotap_stru), pst_mac_frame_head->auc_address1, ul_data_rate, ul_data_rate, ul_data_rate);
            us_mac_body_len   = 0;
            ul_mac_total_len  = us_mac_header_len + us_mac_body_len;
            /* ������������RTS֡, �޸�Э��ģʽΪ11ag */
            st_per_rate.un_nss_rate.st_legacy_rate.bit_protocol_mode = WLAN_LEGACY_OFDM_PHY_PROTOCOL_MODE;
        }
        else
        {
            /* ͳ��MAC֡ͷ��֡��ĳ��� */
            us_mac_header_len = pst_tx_ctl_cb->uc_frame_header_length;
            us_mac_body_len   = pst_tx_ctl_cb->us_mpdu_payload_len;
            ul_mac_total_len  = us_mac_header_len + us_mac_body_len;
            pst_new_netbuf    = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, OAL_SIZEOF(ieee80211_radiotap_stru) + ul_mac_total_len, OAL_NETBUF_PRIORITY_MID);
            if (OAL_PTR_NULL == pst_new_netbuf)
            {
                OAM_ERROR_LOG0(0, OAM_SF_PKT_CAP, "{dmac_pkt_cap_tx::pst_new_netbuf to request memory failed.}");
                /* ����������һ��ѭ��buffer */
                dmac_next_circle_buff(&(pst_dmac_packet->us_circle_buf_index));
                pst_buf_current->bit_status = 0;
                continue;
            }
            OAL_MEMZERO((oal_uint8 *)OAL_NETBUF_DATA(pst_new_netbuf), OAL_SIZEOF(ieee80211_radiotap_stru));
            /* �жϸ�֡�Ƿ�Ϊ����֡ */
            if (MAC_GET_CB_IS_DATA_FRAME(pst_tx_ctl_cb))
            {
                /* ������֡������netbufʱ, ��Ҫ��֡ͷλ�ÿ�����, ������netbuf�е�dataָ�� */
                oal_memcopy((oal_uint8 *)OAL_NETBUF_DATA(pst_new_netbuf) + OAL_SIZEOF(ieee80211_radiotap_stru), (oal_uint8 *)pst_mac_frame_head, us_mac_header_len);
                oal_memcopy((oal_uint8 *)OAL_NETBUF_DATA(pst_new_netbuf) + OAL_SIZEOF(ieee80211_radiotap_stru) + us_mac_header_len, (oal_uint8 *)OAL_NETBUF_DATA(pst_netbuf), us_mac_body_len);
                pst_mac_frame_head = (mac_ieee80211_frame_stru *)((oal_uint8 *)OAL_NETBUF_DATA(pst_new_netbuf) + OAL_SIZEOF(ieee80211_radiotap_stru));
                /* ��QOS����֡seq numҲ����Ӳ����д, ����Ĵ���ȥ����seq num */
                if (OAL_TRUE != (oal_bool_enum_uint8)MAC_GET_CB_IS_QOS_DATA(pst_tx_ctl_cb))
                {
                    hal_get_tx_sequence_num(pst_dmac_vap->pst_hal_device, 0, 0, 0, 0,&ul_seq);
                    if (0 == ul_seq)
                    {
                        pst_mac_frame_head->bit_seq_num = 4095;
                    }
                    pst_mac_frame_head->bit_seq_num = ul_seq - 1;
                }
            }
            else
            {
                /* ������֡��ָ���netbuf�ĸ��Ƶ��������netbuf��, ������seq num */
                oal_memcopy((oal_uint8 *)OAL_NETBUF_DATA(pst_new_netbuf) + OAL_SIZEOF(ieee80211_radiotap_stru), OAL_NETBUF_DATA(pst_netbuf), ul_mac_total_len);
                hal_get_tx_sequence_num(pst_dmac_vap->pst_hal_device, 0, 0, 0, 0,&ul_seq);
                pst_mac_frame_head = (mac_ieee80211_frame_stru *)((oal_uint8 *)OAL_NETBUF_DATA(pst_new_netbuf) + OAL_SIZEOF(ieee80211_radiotap_stru));
                if (0 == ul_seq)
                {
                    pst_mac_frame_head->bit_seq_num = 4095;
                }
                pst_mac_frame_head->bit_seq_num = ul_seq - 1;
            }
            /* ����tx֡��duration�ֶ� */
            if (ul_data_rate >= DMAC_PKT_CAP_SIFS_RATE3)
            {
                uc_ack_time = DMAC_PKT_CAP_CTS_ACK_TIME3;
            }
            else if (ul_data_rate >= DMAC_PKT_CAP_SIFS_RATE2 && ul_data_rate < DMAC_PKT_CAP_SIFS_RATE3)
            {
                uc_ack_time = DMAC_PKT_CAP_CTS_ACK_TIME2;
            }
            else
            {
                uc_ack_time = DMAC_PKT_CAP_CTS_ACK_TIME1;
            }
            *(oal_uint16 *)((oal_uint8 *)OAL_NETBUF_DATA(pst_new_netbuf) + OAL_SIZEOF(ieee80211_radiotap_stru) + 2) = SIFSTIME + uc_ack_time;
        }
        /* ��������Ϣд��ieee80211_radiotap_stru */
        pst_radiotap = (ieee80211_radiotap_stru *)OAL_NETBUF_DATA(pst_new_netbuf);
        dmac_fill_radiotap(pst_radiotap, ul_data_rate, &st_per_rate, &pst_dmac_vap->st_vap_base_info, OAL_PTR_NULL, OAM_OTA_FRAME_DIRECTION_TYPE_TX, 0);
        pst_radiotap->st_radiotap_fields.ull_timestamp = (oal_uint64)((oal_uint64)pst_buf_current->uc_timestamp[7] << 56) +
                                             (oal_uint64)((oal_uint64)pst_buf_current->uc_timestamp[6] << 48) +
                                             (oal_uint64)((oal_uint64)pst_buf_current->uc_timestamp[5] << 40) +
                                             (oal_uint64)((oal_uint64)pst_buf_current->uc_timestamp[4] << 32) +
                                             (oal_uint64)((oal_uint64)pst_buf_current->uc_timestamp[3] << 24) +
                                             (oal_uint64)((oal_uint64)pst_buf_current->uc_timestamp[2] << 16) +
                                             (oal_uint64)((oal_uint64)pst_buf_current->uc_timestamp[1] << 8) +
                                              (oal_uint64)pst_buf_current->uc_timestamp[0];
        dmac_report_packet(pst_new_netbuf, (oal_uint16)OAL_SIZEOF(ieee80211_radiotap_stru), (oal_uint16)us_mac_header_len, (oal_uint16)us_mac_body_len,
                           pst_dmac_packet->en_report_sdt_switch, OAM_OTA_FRAME_DIRECTION_TYPE_TX);
        /* ͳ���ϱ���֡���� */
        pst_dmac_packet->ul_total_report_pkt_num++;

        /* �ж��Ƿ�ΪAMPDU, ������Ҫ��������֮���MPDU֡, ����Ҫ��֤��ʱ���Ǵ�RTS��tx������ȡ�� */
        if (1 == pst_current_dscr->bit_is_ampdu && 0 != pst_buf_current->bit_frm_type)
        {
            hal_get_tx_dscr_next(pst_hal_device, pst_current_dscr, &pst_mpdu_dscr);
            uc_tsf_offset = 1;
            while (OAL_PTR_NULL != pst_mpdu_dscr)
            {
                pst_netbuf = pst_mpdu_dscr->pst_skb_start_addr;
                if (OAL_PTR_NULL == pst_netbuf)
                {
                    OAM_WARNING_LOG0(0, OAM_SF_PKT_CAP, "{dmac_pkt_cap_tx::pst_netbuf_ampdu is null.}");
                    oam_report_dscr(BROADCAST_MACADDR, (oal_uint8 *)pst_mpdu_dscr, (oal_uint16)WLAN_MEM_SHARED_TX_DSCR_SIZE1, OAM_OTA_TX_DSCR_TYPE);
                    break;
                }
                pst_tx_ctl_cb = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
                pst_mac_frame_head = pst_tx_ctl_cb->pst_frame_header;

                us_mac_header_len = pst_tx_ctl_cb->uc_frame_header_length;
                us_mac_body_len   = pst_tx_ctl_cb->us_mpdu_payload_len;
                ul_mac_total_len  = us_mac_header_len + us_mac_body_len;
                pst_mpdu_netbuf    = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, OAL_SIZEOF(ieee80211_radiotap_stru) + ul_mac_total_len, OAL_NETBUF_PRIORITY_MID);
                if (OAL_PTR_NULL == pst_mpdu_netbuf)
                {
                    OAM_ERROR_LOG0(0, OAM_SF_PKT_CAP, "{dmac_pkt_cap_tx::pst_new_netbuf to request memory failed.}");
                    break;
                }
                OAL_MEMZERO((oal_uint8 *)OAL_NETBUF_DATA(pst_mpdu_netbuf), OAL_SIZEOF(ieee80211_radiotap_stru));
                /* ���AMPDU���һ��MPDUΪ�ش�, ��֮���MPDU��Ϊ�ش�֡ */
                if (pst_buf_current->bit_tx_cnt > 0)
                {
                    pst_mac_frame_head->st_frame_control.bit_retry = 1;
                }
                /* ������֡������netbufʱ, ��Ҫ��֡ͷλ�ÿ�����, ������netbuf�е�dataָ�� */
                oal_memcopy((oal_uint8 *)OAL_NETBUF_DATA(pst_mpdu_netbuf) + OAL_SIZEOF(ieee80211_radiotap_stru), (oal_uint8 *)pst_mac_frame_head, us_mac_header_len);
                oal_memcopy((oal_uint8 *)OAL_NETBUF_DATA(pst_mpdu_netbuf) + OAL_SIZEOF(ieee80211_radiotap_stru) + us_mac_header_len, (oal_uint8 *)OAL_NETBUF_DATA(pst_netbuf), us_mac_body_len);
                /* ����tx֡��duration�ֶ� */
                if (ul_data_rate >= DMAC_PKT_CAP_SIFS_RATE3)
                {
                    us_ba_time = DMAC_PKT_CAP_BA_TIME3;
                }
                else if (ul_data_rate >= DMAC_PKT_CAP_SIFS_RATE2 && ul_data_rate < DMAC_PKT_CAP_SIFS_RATE3)
                {
                    us_ba_time = DMAC_PKT_CAP_BA_TIME2;
                }
                else
                {
                    us_ba_time = DMAC_PKT_CAP_BA_TIME1;
                }
                pst_mac_frame_head->bit_duration_value = SIFSTIME + us_ba_time;
                /* ��������Ϣд��ieee80211_radiotap_stru */
                pst_radiotap = (ieee80211_radiotap_stru *)OAL_NETBUF_DATA(pst_mpdu_netbuf);
                dmac_fill_radiotap(pst_radiotap, ul_data_rate, &st_per_rate, &pst_dmac_vap->st_vap_base_info, OAL_PTR_NULL, OAM_OTA_FRAME_DIRECTION_TYPE_TX, 0);
                pst_radiotap->st_radiotap_fields.ull_timestamp = (oal_uint64)((oal_uint64)pst_buf_current->uc_timestamp[7] << 56) +
                                             (oal_uint64)((oal_uint64)pst_buf_current->uc_timestamp[6] << 48) +
                                             (oal_uint64)((oal_uint64)pst_buf_current->uc_timestamp[5] << 40) +
                                             (oal_uint64)((oal_uint64)pst_buf_current->uc_timestamp[4] << 32) +
                                             (oal_uint64)((oal_uint64)pst_buf_current->uc_timestamp[3] << 24) +
                                             (oal_uint64)((oal_uint64)pst_buf_current->uc_timestamp[2] << 16) +
                                             (oal_uint64)((oal_uint64)pst_buf_current->uc_timestamp[1] << 8) +
                                              (oal_uint64)pst_buf_current->uc_timestamp[0] + (oal_uint64)uc_tsf_offset;
                dmac_report_packet(pst_mpdu_netbuf, (oal_uint16)OAL_SIZEOF(ieee80211_radiotap_stru), (oal_uint16)us_mac_header_len, (oal_uint16)us_mac_body_len,
                           pst_dmac_packet->en_report_sdt_switch, OAM_OTA_FRAME_DIRECTION_TYPE_TX);
                /* ͳ���ϱ���֡���� */
                pst_dmac_packet->ul_total_report_pkt_num++;

                oal_netbuf_free(pst_mpdu_netbuf);
                hal_get_tx_dscr_next(pst_hal_device, pst_mpdu_dscr, &pst_mpdu_dscr);
                uc_tsf_offset++;
            }
        }

        /* �����굱ǰ��Чbuffer֮����յ�ǰbuffer״̬λ */
        pst_buf_current->bit_status = 0;
        oal_netbuf_free(pst_new_netbuf);

        /* ����������һ��ѭ��buffer */
        dmac_next_circle_buff(&(pst_dmac_packet->us_circle_buf_index));
    }
    return OAL_SUCC;
}


oal_uint32  dmac_pkt_cap_rx(dmac_rx_ctl_stru *pst_cb_ctrl, oal_netbuf_stru *pst_netbuf, dmac_packet_stru *pst_dmac_packet, hal_to_dmac_device_stru *pst_hal_device, oal_int8 c_rssi_ampdu)
{
    oal_netbuf_stru               *pst_new_netbuf       = OAL_PTR_NULL;     /* �������֡��netbuf */
    ieee80211_radiotap_stru       *pst_radiotap         = OAL_PTR_NULL;
    oal_netbuf_stru               *pst_make_netbuf      = OAL_PTR_NULL;     /* ���湹��֡��netbuf */
    ieee80211_radiotap_stru       *pst_make_radiotap    = OAL_PTR_NULL;
    mac_ieee80211_frame_stru      *pst_mac_frame_head   = OAL_PTR_NULL;
    oal_uint8                      uc_ack_len;
    oal_uint8                      uc_cts_len;
    oal_uint16                     us_frame_len;
    oal_uint8                     *puc_vap_mac_addr     = OAL_PTR_NULL;
    mac_vap_stru                  *pst_mac_vap          = OAL_PTR_NULL;
    hal_statistic_stru             st_per_rate;
    oal_uint32                     ul_rate_kbps;                            /* ����֡������ */
    oal_uint32                     ul_ack_rate;                             /* �����ACK������ */
    hal_to_dmac_vap_stru          *pst_hal_vap          = OAL_PTR_NULL;
    oal_uint32                     ul_ret;
    oal_uint8                      uc_mac_vap_id;
    oal_uint16                     us_rts_duration;

    if ((OAL_PTR_NULL == pst_cb_ctrl) || (OAL_PTR_NULL == pst_netbuf) || (OAL_PTR_NULL == pst_dmac_packet) || (OAL_PTR_NULL == pst_hal_device))
    {
        OAM_ERROR_LOG0(0, OAM_SF_PKT_CAP, "{dmac_pkt_cap_rx::input argument detect failed.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    /* ��ȡ��ǰ��������Ӧ��֡��֡�� */
    us_frame_len = pst_cb_ctrl->st_rx_info.us_frame_len;
    /* ����pst_new_netbuf�洢����֡��radiotap��֡���� */
    pst_new_netbuf = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, OAL_SIZEOF(ieee80211_radiotap_stru) + us_frame_len, OAL_NETBUF_PRIORITY_MID);
    if (OAL_PTR_NULL == pst_new_netbuf)
    {
        OAM_ERROR_LOG0(0, OAM_SF_PKT_CAP, "{dmac_pkt_cap_rx::pst_new_netbuf request memory failed.}");
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }
    OAL_MEMZERO((oal_uint8 *)OAL_NETBUF_DATA(pst_new_netbuf), OAL_SIZEOF(ieee80211_radiotap_stru));
    oal_memcopy(OAL_NETBUF_DATA(pst_new_netbuf) + OAL_SIZEOF(ieee80211_radiotap_stru), OAL_NETBUF_DATA(pst_netbuf), us_frame_len);

    /* ���ж�hal vap id�Ƿ���Ч, ��ͨ��hal vap id�ҵ�mac vap id, Ȼ���ҵ�mac vap�ṹ, Ȼ����mib�����ҵ���ǰvap��mac��ַ */
    if (HAL_VAP_ID_IS_VALID(pst_cb_ctrl->st_rx_info.bit_vap_id))
    {
        hal_get_hal_vap(pst_hal_device, pst_cb_ctrl->st_rx_info.bit_vap_id, &pst_hal_vap);
        if (OAL_PTR_NULL == pst_hal_vap)
        {
            OAM_WARNING_LOG1(0, OAM_SF_PKT_CAP, "{dmac_pkt_cap_rx:get vap faild, hal vap id=%u}", pst_cb_ctrl->st_rx_info.bit_vap_id);
            /* ��ȡhal vapʧ��, ֱ��ʹ������vap id */
            uc_mac_vap_id = 0;
        }
        else
        {
            uc_mac_vap_id = pst_hal_vap->uc_mac_vap_id;
        }
    }
    else
    {
        /* hal vap id��Ч, ��ʹ������vap id, ���˿����ж�������BSS����֡, ���蹹��SIFS��Ӧ֡ */
        uc_mac_vap_id = 0;
    }
    pst_mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(uc_mac_vap_id);
    if (OAL_PTR_NULL == pst_mac_vap)
    {
        /* MAC VAP IDΪ0, ��������VAP, ��MIB��, ������������ʹ��MIB�����ֿ�ָ����� */
        OAM_WARNING_LOG0(0, OAM_SF_PKT_CAP, "{dmac_pkt_cap_rx::pst_mac_vap is null.}");
        /* �ͷ�pst_new_netbuf */
        oal_netbuf_free(pst_new_netbuf);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ��ȡ����֡������ */
    st_per_rate.un_nss_rate.st_ht_rate.bit_ht_mcs        = pst_cb_ctrl->st_rx_statistic.un_nss_rate.st_ht_rate.bit_ht_mcs;
    st_per_rate.un_nss_rate.st_ht_rate.bit_protocol_mode = pst_cb_ctrl->st_rx_statistic.un_nss_rate.st_ht_rate.bit_protocol_mode;
    st_per_rate.uc_short_gi      = pst_cb_ctrl->st_rx_status.bit_GI;
    st_per_rate.bit_preamble     = pst_cb_ctrl->st_rx_status.bit_preabmle;
    st_per_rate.bit_stbc         = pst_cb_ctrl->st_rx_status.bit_STBC;
    st_per_rate.bit_channel_code = pst_cb_ctrl->st_rx_status.bit_channel_code;
    st_per_rate.uc_bandwidth     = pst_cb_ctrl->st_rx_status.bit_freq_bandwidth_mode;
    ul_ret = dmac_alg_get_rate_kbps(pst_hal_device, &st_per_rate, &ul_rate_kbps);
    if (OAL_ERR_CODE_PTR_NULL == ul_ret)
    {
        OAM_ERROR_LOG0(0, OAM_SF_PKT_CAP, "{dmac_pkt_cap_rx::get rate failed.}");
        /* ����ȡ����ʧ��, Ĭ������Ϊ6Mbps */
        ul_rate_kbps = DMAC_PKT_CAP_5G_RATE;
    }

    /* ���radiotap */
    pst_radiotap = (ieee80211_radiotap_stru *)OAL_NETBUF_DATA(pst_new_netbuf);
    dmac_fill_radiotap(pst_radiotap, ul_rate_kbps, &st_per_rate, pst_mac_vap, pst_cb_ctrl, OAM_OTA_FRAME_DIRECTION_TYPE_RX, c_rssi_ampdu);
    pst_radiotap->st_radiotap_fields.ull_timestamp = (oal_uint64)pst_cb_ctrl->st_rx_status.ul_tsf_timestamp;

    /* �ϱ���ǰ��������Ӧ��֡ */
    dmac_report_packet(pst_new_netbuf, (oal_uint16)OAL_SIZEOF(ieee80211_radiotap_stru), (oal_uint16)pst_cb_ctrl->st_rx_info.uc_mac_header_len,
                       (oal_uint16)(pst_cb_ctrl->st_rx_info.us_frame_len - pst_cb_ctrl->st_rx_info.uc_mac_header_len),
                        pst_dmac_packet->en_report_sdt_switch, OAM_OTA_FRAME_DIRECTION_TYPE_RX);

    /* ͳ��ץ������ */
    pst_dmac_packet->ul_total_report_pkt_num++;

    pst_mac_frame_head = (mac_ieee80211_frame_stru *)mac_get_rx_cb_mac_hdr(&(pst_cb_ctrl->st_rx_info));
    if  (OAL_PTR_NULL == pst_mac_frame_head)
    {
        OAM_ERROR_LOG0(0, OAM_SF_PKT_CAP, "dmac_pkt_cap_rx::mac frame head is null.");
        /* �ͷ�pst_new_netbuf */
        oal_netbuf_free(pst_new_netbuf);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* MAC VAP IDΪ0, ��������VAP, ��MIB��, ������������ʹ��MIB�����ֿ�ָ�����, ��������VAP ID��ֵ�ж� */
    if (0 == uc_mac_vap_id)
    {
        /* �ͷ�pst_new_netbuf */
        oal_netbuf_free(pst_new_netbuf);
        return OAL_ERR_CODE_PTR_NULL;
    }
    puc_vap_mac_addr = mac_mib_get_StationID(pst_mac_vap);

    /* �ж�֡�Ƿ��Ƿ�����BSS�� */
    if (0 == oal_compare_mac_addr(pst_mac_frame_head->auc_address1, puc_vap_mac_addr))
    {
        /* �жϽ��յ��ǿ���֡��������֡/����֡ */
        if (WLAN_FC0_TYPE_CTL == mac_get_frame_type(OAL_NETBUF_DATA(pst_netbuf)))
        {
            /* �����RTS�������Ҫ����һ��CTS, ������ */
            if(WLAN_RTS == mac_frame_get_subtype_value(OAL_NETBUF_DATA(pst_netbuf)))
            {
                /* ����pst_make_netbuf�洢�����SIFS��Ӧ֡ */
                pst_make_netbuf = OAL_MEM_NETBUF_ALLOC(OAL_MGMT_NETBUF, OAL_SIZEOF(ieee80211_radiotap_stru) + ACK_CTS_FRAME_LEN - WLAN_HDR_FCS_LENGTH, OAL_NETBUF_PRIORITY_MID);
                if (OAL_PTR_NULL == pst_make_netbuf)
                {
                    OAM_ERROR_LOG0(0, OAM_SF_PKT_CAP, "{dmac_pkt_cap_rx::pst_make_netbuf request memory failed.}");
                    oal_netbuf_free(pst_new_netbuf);
                    return OAL_ERR_CODE_ALLOC_MEM_FAIL;
                }
                OAL_MEMZERO((oal_uint8 *)OAL_NETBUF_DATA(pst_make_netbuf), OAL_SIZEOF(ieee80211_radiotap_stru));
                /* ����CTS�Ĺ��캯��, ����pst_make_radiotapָ��pst_make_netbuf��dataͷ */
                us_rts_duration     = pst_mac_frame_head->bit_duration_value;
                uc_cts_len          = (oal_uint8)dmac_encap_ctl_cts((oal_uint8 *)OAL_NETBUF_DATA(pst_make_netbuf) + OAL_SIZEOF(ieee80211_radiotap_stru), pst_mac_frame_head->auc_address2, us_rts_duration, ul_rate_kbps);
                pst_make_radiotap   = (ieee80211_radiotap_stru *)(OAL_NETBUF_DATA(pst_make_netbuf));
                /* ��乹��֡��radiotap */
                dmac_fill_radiotap(pst_make_radiotap, ul_rate_kbps, &st_per_rate, pst_mac_vap, OAL_PTR_NULL, OAM_OTA_FRAME_DIRECTION_TYPE_TX, 0);
                pst_make_radiotap->st_radiotap_fields.ull_timestamp = (oal_uint64)pst_cb_ctrl->st_rx_status.ul_tsf_timestamp + SIFSTIME;

                /* �����CTS֡�ϱ� */
                dmac_report_packet(pst_make_netbuf, (oal_uint16)OAL_SIZEOF(ieee80211_radiotap_stru), (oal_uint16)uc_cts_len,
                                    0, pst_dmac_packet->en_report_sdt_switch, OAM_OTA_FRAME_DIRECTION_TYPE_TX);

                /* ͳ���ϱ���֡���� */
                pst_dmac_packet->ul_total_report_pkt_num++;
                /* �ͷ�pst_make_netbuf */
                oal_netbuf_free(pst_make_netbuf);
            }
        }
        else
        {
            /* �ж��Ƿ�ΪAMPDU, ������Ҫ��BA, �����ACK */
            if (1 != pst_cb_ctrl->st_rx_status.bit_AMPDU)
            {
                pst_make_netbuf = OAL_MEM_NETBUF_ALLOC(OAL_MGMT_NETBUF, OAL_SIZEOF(ieee80211_radiotap_stru) + ACK_CTS_FRAME_LEN - WLAN_HDR_FCS_LENGTH, OAL_NETBUF_PRIORITY_MID);
                if (OAL_PTR_NULL == pst_make_netbuf)
                {
                    OAM_ERROR_LOG0(0, OAM_SF_TX, "{dmac_pkt_cap_rx::pst_make_netbuf request memory failed.}");
                    oal_netbuf_free(pst_new_netbuf);
                    return OAL_ERR_CODE_ALLOC_MEM_FAIL;
                }
                OAL_MEMZERO((oal_uint8 *)OAL_NETBUF_DATA(pst_make_netbuf), OAL_SIZEOF(ieee80211_radiotap_stru));
                /* ����ACK�Ĺ��캯��, ����pst_make_radiotapָ��pst_make_netbuf��dataͷ */
                uc_ack_len          = (oal_uint8)dmac_encap_ctl_ack(OAL_NETBUF_DATA(pst_make_netbuf) + OAL_SIZEOF(ieee80211_radiotap_stru), pst_mac_frame_head->auc_address2);
                pst_make_radiotap   = (ieee80211_radiotap_stru *)(OAL_NETBUF_DATA(pst_make_netbuf));
                /* ����ACK�����ʴ���24����24Mbps, С��24�Ҵ���6����6Mbps, С��6����1Mbps, ͬʱע�⽵Э�� */
                st_per_rate.un_nss_rate.st_ht_rate.bit_protocol_mode = WLAN_LEGACY_OFDM_PHY_PROTOCOL_MODE;
                if (ul_rate_kbps >= DMAC_PKT_CAP_SIFS_RATE3)
                {
                    ul_ack_rate = DMAC_PKT_CAP_SIFS_RATE3;
                }
                else if (ul_rate_kbps >= DMAC_PKT_CAP_SIFS_RATE2 && ul_rate_kbps < DMAC_PKT_CAP_SIFS_RATE3)
                {
                    ul_ack_rate = DMAC_PKT_CAP_SIFS_RATE2;
                }
                else
                {
                    ul_ack_rate = DMAC_PKT_CAP_SIFS_RATE1;
                    st_per_rate.un_nss_rate.st_ht_rate.bit_protocol_mode = WLAN_11B_PHY_PROTOCOL_MODE;
                }
                /* ��乹��ACK��radiotap */
                dmac_fill_radiotap(pst_make_radiotap, ul_ack_rate, &st_per_rate, pst_mac_vap, OAL_PTR_NULL, OAM_OTA_FRAME_DIRECTION_TYPE_TX, 0);
                pst_make_radiotap->st_radiotap_fields.ull_timestamp = (oal_uint64)pst_cb_ctrl->st_rx_status.ul_tsf_timestamp +
                       (oal_uint64)(((oal_uint32)(pst_cb_ctrl->st_rx_info.us_frame_len - pst_cb_ctrl->st_rx_info.uc_mac_header_len) << 3) * 1000 / ul_rate_kbps) + SIFSTIME;

                /* �����ACK֡�ϱ� */
                dmac_report_packet(pst_make_netbuf, (oal_uint16)OAL_SIZEOF(ieee80211_radiotap_stru), (oal_uint16)uc_ack_len,
                                    0, pst_dmac_packet->en_report_sdt_switch, OAM_OTA_FRAME_DIRECTION_TYPE_TX);

                /* ͳ���ϱ���֡���� */
                pst_dmac_packet->ul_total_report_pkt_num++;
                /* �ͷ�pst_make_netbuf */
                oal_netbuf_free(pst_make_netbuf);
            }
        }
    }
    /* �ͷŽ���֡�����pst_new_netbuf */
    oal_netbuf_free(pst_new_netbuf);
    return OAL_SUCC;
}


oal_uint32  dmac_pkt_cap_ba(dmac_packet_stru *pst_dmac_packet, dmac_rx_ctl_stru *pst_cb_ctrl, hal_to_dmac_device_stru *pst_hal_device)
{
    oal_uint32                      ul_ba_len;
    oal_netbuf_stru                *pst_make_netbuf = OAL_PTR_NULL;
    mac_vap_stru                   *pst_mac_vap;
    mac_ieee80211_qos_frame_stru   *pst_mac_head;
    ieee80211_radiotap_stru        *pst_radiotap;
    hal_statistic_stru              st_per_rate;
    oal_uint32                      ul_rate_kbps;
    oal_uint32                      ul_ba_kbps;
    oal_uint8                      *puc_vap_mac_addr;
    oal_uint32                      ul_ret;
    hal_to_dmac_vap_stru           *pst_hal_vap;
    oal_uint16                      us_user_idx = 0xFFFF;
    dmac_user_stru                 *pst_ta_dmac_user;
    oal_uint8                       uc_lut_index;

    /* ���ж�hal vap id�Ƿ���Ч, ��ͨ��hal vap id�ҵ�mac vap id, Ȼ���ҵ�mac vap�ṹ, Ȼ����mib�����ҵ���ǰvap��mac��ַ */
    if (HAL_VAP_ID_IS_VALID(pst_cb_ctrl->st_rx_info.bit_vap_id))
    {
        hal_get_hal_vap(pst_hal_device, pst_cb_ctrl->st_rx_info.bit_vap_id, &pst_hal_vap);
        if (OAL_PTR_NULL == pst_hal_vap)
        {
            OAM_WARNING_LOG1(0, OAM_SF_PKT_CAP, "{dmac_pkt_cap_ba:get vap faild, hal vap id=%d}", pst_cb_ctrl->st_rx_info.bit_vap_id);
            return OAL_ERR_CODE_PTR_NULL;
        }
    }
    else
    {
        /* hal vap id���Ϸ�, ���Ƿ�����BSS�ľۺ�֡, ����BA */
        OAM_WARNING_LOG0(0, OAM_SF_PKT_CAP, "{dmac_pkt_cap_ba::hal vap id is invalid.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_hal_vap->uc_mac_vap_id);
    if ((OAL_PTR_NULL == pst_mac_vap) ||(0 == pst_hal_vap->uc_mac_vap_id))
    {
        /* MAC VAP IDΪ0, ��������VAP, ��MIB��, ������������ʹ��MIB�����ֿ�ָ�����, ��������VAP ID��ֵ�ж� */
        OAM_WARNING_LOG0(0, OAM_SF_PKT_CAP, "{dmac_pkt_cap_ba::pst_mac_vap is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    /* ֱ��ʹ��QOS֡��֡�ṹ */
    pst_mac_head = (mac_ieee80211_qos_frame_stru *)mac_get_rx_cb_mac_hdr(&(pst_cb_ctrl->st_rx_info));
    if (OAL_PTR_NULL == pst_mac_head)
    {
        OAM_ERROR_LOG0(0, OAM_SF_PKT_CAP, "dmac_pkt_cap_ba::mac frame head is null.");
        return OAL_ERR_CODE_PTR_NULL;
    }
    puc_vap_mac_addr = mac_mib_get_StationID(pst_mac_vap);

    /* �ж��Ƿ��Ƿ�����bss��֡ */
    if(0 == oal_compare_mac_addr(pst_mac_head->auc_address1, puc_vap_mac_addr))
    {
        /* ͨ��user��mac��ַ���Ҷ�Ӧuser��index, ���ջ��dmac user�ṹ�� */
        ul_ret = mac_vap_find_user_by_macaddr(pst_mac_vap, pst_mac_head->auc_address2, &us_user_idx);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(0, OAM_SF_TX, "{dmac_pkt_cap_ba::us_user_idx is invalid [%u].}", us_user_idx);
            return OAL_ERR_CODE_PTR_NULL;
        }
        pst_ta_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user(us_user_idx);
        if (OAL_PTR_NULL == pst_ta_dmac_user)
        {
            OAM_WARNING_LOG0(0, OAM_SF_TX, "{dmac_pkt_cap_ba::pst_ta_dmac_user is NULL.}");
            return OAL_ERR_CODE_PTR_NULL;
        }
        /* ͨ��user�����ķ���tid�������, �ҵ�lut index */
        uc_lut_index    = pst_ta_dmac_user->ast_tx_tid_queue[pst_mac_head->bit_qc_tid].st_ba_rx_hdl.uc_lut_index;
        pst_make_netbuf = OAL_MEM_NETBUF_ALLOC(OAL_MGMT_NETBUF, OAL_SIZEOF(ieee80211_radiotap_stru) + MAC_80211_CTL_HEADER_LEN + 12, OAL_NETBUF_PRIORITY_MID);
        if (OAL_PTR_NULL == pst_make_netbuf)
        {
            OAM_WARNING_LOG0(0, OAM_SF_TX, "{dmac_pkt_cap_ba::alloc netbuff failed.}");
            return OAL_ERR_CODE_ALLOC_MEM_FAIL;
        }
        OAL_MEMZERO((oal_uint8 *)OAL_NETBUF_DATA(pst_make_netbuf), OAL_SIZEOF(ieee80211_radiotap_stru));
        /* ����ba֡ */
        ul_ba_len = dmac_encap_ctl_ba(pst_mac_vap, OAL_NETBUF_DATA(pst_make_netbuf) + OAL_SIZEOF(ieee80211_radiotap_stru), pst_mac_head->auc_address2, uc_lut_index);

        /* ��ȡ����֡������ */
        st_per_rate.un_nss_rate.st_ht_rate.bit_ht_mcs        = pst_cb_ctrl->st_rx_statistic.un_nss_rate.st_ht_rate.bit_ht_mcs;
        st_per_rate.un_nss_rate.st_ht_rate.bit_protocol_mode = pst_cb_ctrl->st_rx_statistic.un_nss_rate.st_ht_rate.bit_protocol_mode;
        st_per_rate.uc_short_gi         = pst_cb_ctrl->st_rx_status.bit_GI;
        st_per_rate.bit_preamble        = pst_cb_ctrl->st_rx_status.bit_preabmle;
        st_per_rate.bit_stbc            = pst_cb_ctrl->st_rx_status.bit_STBC;
        st_per_rate.bit_channel_code    = pst_cb_ctrl->st_rx_status.bit_channel_code;
        st_per_rate.uc_bandwidth        = pst_cb_ctrl->st_rx_status.bit_freq_bandwidth_mode;
        ul_ret = dmac_alg_get_rate_kbps(pst_hal_device, &st_per_rate, &ul_rate_kbps);
        if (OAL_ERR_CODE_PTR_NULL == ul_ret)
        {
            OAM_ERROR_LOG0(0, OAM_SF_PKT_CAP, "{dmac_pkt_cap_ba::get rate failed.}");
            /* ����ȡ����ʧ��, Ĭ������Ϊ6Mbps */
            ul_rate_kbps = DMAC_PKT_CAP_5G_RATE;
        }

        /* ����BA�����ʴ���24����24Mbps, С��24�Ҵ���6����6Mbps, С��6����1Mbps, ͬʱע�⽵Э�� */
        st_per_rate.un_nss_rate.st_ht_rate.bit_protocol_mode = WLAN_LEGACY_OFDM_PHY_PROTOCOL_MODE;
        if (ul_rate_kbps >= DMAC_PKT_CAP_SIFS_RATE3)
        {
            ul_ba_kbps = DMAC_PKT_CAP_SIFS_RATE3;
        }
        else if (ul_rate_kbps >= DMAC_PKT_CAP_SIFS_RATE2 && ul_rate_kbps < DMAC_PKT_CAP_SIFS_RATE3)
        {
            ul_ba_kbps = DMAC_PKT_CAP_SIFS_RATE2;
        }
        else
        {
            ul_ba_kbps = DMAC_PKT_CAP_SIFS_RATE1;
            st_per_rate.un_nss_rate.st_ht_rate.bit_protocol_mode = WLAN_11B_PHY_PROTOCOL_MODE;
        }
        /* ���radiotap */
        pst_radiotap = (ieee80211_radiotap_stru *)OAL_NETBUF_DATA(pst_make_netbuf);
        dmac_fill_radiotap(pst_radiotap, ul_ba_kbps, &st_per_rate, pst_mac_vap, pst_cb_ctrl, OAM_OTA_FRAME_DIRECTION_TYPE_TX, 0);
        pst_radiotap->st_radiotap_fields.ull_timestamp = (oal_uint64)pst_cb_ctrl->st_rx_status.ul_tsf_timestamp +
                   (oal_uint64)(((oal_uint32)(pst_cb_ctrl->st_rx_info.us_frame_len - pst_cb_ctrl->st_rx_info.uc_mac_header_len) << 3) * 1000 / ul_rate_kbps) + SIFSTIME;

        /* �ϱ���ǰ��������Ӧ��ba֡ */
        dmac_report_packet(pst_make_netbuf, (oal_uint16)OAL_SIZEOF(ieee80211_radiotap_stru), (oal_uint16)MAC_80211_CTL_HEADER_LEN,
                            (oal_uint16)(ul_ba_len - MAC_80211_CTL_HEADER_LEN), pst_dmac_packet->en_report_sdt_switch, OAM_OTA_FRAME_DIRECTION_TYPE_TX);

        /* ͳ���ϱ���֡���� */
        pst_dmac_packet->ul_total_report_pkt_num++;

        oal_netbuf_free(pst_make_netbuf);
    }
    return OAL_SUCC;
}


oal_uint32  dmac_pkt_cap_beacon(dmac_packet_stru *pst_dmac_packet, dmac_vap_stru *pst_dmac_vap)
{
    oal_uint32                   ul_cur_vap_timestamp[2];
    oal_uint32                   ul_ext_ap_eleven_timestamp;
    oal_uint8                   *puc_cur_bcn_buf;
    hal_to_dmac_vap_stru         st_ext_ap_eleven_vap;
    oal_netbuf_stru             *pst_make_netbuf;
    ieee80211_radiotap_stru     *pst_radiotap;
    oal_uint32                   ul_cur_bcn_rate;
    oal_uint32                   ul_cur_bcn_tx_mode;
    hal_statistic_stru           st_per_rate;
    oal_uint32                   ul_rate_kbps;
    oal_uint32                   ul_seq;
    mac_ieee80211_frame_stru    *pst_mac_header;
    oal_uint32                   ul_ret;

    /* 1����ȡ��ӦVAP��tsfֵ, �Լ���Ӧbeacon��seq num */
    hal_vap_tsf_get_64bit(pst_dmac_vap->pst_hal_vap, &ul_cur_vap_timestamp[1], &ul_cur_vap_timestamp[0]);
    hal_get_tx_sequence_num(pst_dmac_vap->pst_hal_device, 0, 0, 0, 0,&ul_seq);

    /* 2����ȡEXT AP11��tsfֵ, ��д��Ӧ����ʱ��radiotap */
    st_ext_ap_eleven_vap.uc_vap_id      = WLAN_HAL_EXT_AP11_VAP_ID;
    st_ext_ap_eleven_vap.uc_chip_id     = pst_dmac_vap->pst_hal_vap->uc_chip_id;
    st_ext_ap_eleven_vap.uc_device_id   = pst_dmac_vap->pst_hal_vap->uc_device_id;
    hal_vap_tsf_get_32bit(&st_ext_ap_eleven_vap, &ul_ext_ap_eleven_timestamp);

    /* 3����ȡ��Ҫ�ϱ���beacon֡ pst_dmac_vap->pauc_beacon_buffer[pst_dmac_vap->uc_beacon_idx], ����beacon֡��beacon timestamp */
    pst_make_netbuf = OAL_MEM_NETBUF_ALLOC(OAL_MGMT_NETBUF, OAL_SIZEOF(ieee80211_radiotap_stru) + pst_dmac_vap->us_beacon_len, OAL_NETBUF_PRIORITY_MID);
    if (OAL_PTR_NULL == pst_make_netbuf)
    {
        OAM_ERROR_LOG0(0, OAM_SF_TX, "{dmac_tbtt_event_ap::pst_make_netbuf to request memory failed.}");
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }
    OAL_MEMZERO((oal_uint8 *)OAL_NETBUF_DATA(pst_make_netbuf), OAL_SIZEOF(ieee80211_radiotap_stru));
    puc_cur_bcn_buf = pst_dmac_vap->pauc_beacon_buffer[pst_dmac_vap->uc_beacon_idx];
    oal_memcopy((oal_uint8 *)OAL_NETBUF_DATA(pst_make_netbuf) + OAL_SIZEOF(ieee80211_radiotap_stru), puc_cur_bcn_buf, pst_dmac_vap->us_beacon_len);
    *(oal_uint64 *)((oal_uint8 *)OAL_NETBUF_DATA(pst_make_netbuf) + OAL_SIZEOF(ieee80211_radiotap_stru) + MAC_80211_FRAME_LEN) = ((oal_uint64)ul_cur_vap_timestamp[1] << 32) + (oal_uint64)ul_cur_vap_timestamp[0];

    pst_mac_header = (mac_ieee80211_frame_stru *)((oal_uint8 *)OAL_NETBUF_DATA(pst_make_netbuf) + OAL_SIZEOF(ieee80211_radiotap_stru));
    if (0 == ul_seq)
    {
        pst_mac_header->bit_seq_num = 4095;
    }
    pst_mac_header->bit_seq_num = ul_seq - 1;

    /* 4����ȡ��ӦVAP��beacon֡��������-��Ӧ�Ĵ��� */
    hal_get_bcn_info(pst_dmac_vap->pst_hal_vap, &ul_cur_bcn_rate, &ul_cur_bcn_tx_mode);
    st_per_rate.uc_bandwidth     = WLAN_BAND_WIDTH_20M;
    st_per_rate.uc_short_gi      = (ul_cur_bcn_rate & BIT28) >> 28;
    st_per_rate.bit_preamble     = (ul_cur_bcn_rate & BIT27) >> 27;
    st_per_rate.bit_channel_code = (ul_cur_bcn_tx_mode & BIT2) >> 2;
    st_per_rate.un_nss_rate.st_legacy_rate.bit_protocol_mode = (ul_cur_bcn_rate & (BIT22 | BIT23)) >> 22;
    st_per_rate.un_nss_rate.st_legacy_rate.bit_legacy_rate   = (ul_cur_bcn_rate & (BIT16 | BIT17 | BIT18 | BIT19)) >> 16;
    ul_ret = dmac_alg_get_rate_kbps(pst_dmac_vap->pst_hal_device, &st_per_rate, &ul_rate_kbps);
    if (OAL_ERR_CODE_PTR_NULL == ul_ret)
    {
        OAM_ERROR_LOG0(0, OAM_SF_PKT_CAP, "{dmac_pkt_cap_beacon::get rate failed.}");
        /* ����ȡ����ʧ��, 2GĬ������Ϊ1Mbps, 5GĬ������Ϊ6Mbps */
        if (WLAN_BAND_2G == pst_dmac_vap->st_vap_base_info.st_channel.en_band)
        {
            ul_rate_kbps = DMAC_PKT_CAP_2G_RATE;
        }
        else
        {
            ul_rate_kbps = DMAC_PKT_CAP_5G_RATE;
        }
    }

    /* 5����дradiotapͷ */
    pst_radiotap = (ieee80211_radiotap_stru *)OAL_NETBUF_DATA(pst_make_netbuf);
    dmac_fill_radiotap(pst_radiotap, ul_rate_kbps, &st_per_rate, &(pst_dmac_vap->st_vap_base_info), OAL_PTR_NULL, OAM_OTA_FRAME_DIRECTION_TYPE_TX, 0);
    pst_radiotap->st_radiotap_fields.ull_timestamp = (oal_uint64)ul_ext_ap_eleven_timestamp;

    /* �ϱ�Beacon֡ */
    dmac_report_packet(pst_make_netbuf, (oal_uint16)OAL_SIZEOF(ieee80211_radiotap_stru), (oal_uint16)MAC_80211_FRAME_LEN,
                       (oal_uint16)(pst_dmac_vap->us_beacon_len - MAC_80211_FRAME_LEN), pst_dmac_packet->en_report_sdt_switch, OAM_OTA_FRAME_DIRECTION_TYPE_TX);
    /* ͳ���ϱ���֡���� */
    pst_dmac_packet->ul_total_report_pkt_num++;
    oal_netbuf_free(pst_make_netbuf);

    return OAL_SUCC;
}


/*lint -e578*//*lint -e19*/
oal_module_symbol(dmac_pkt_cap_tx);
oal_module_symbol(dmac_pkt_cap_rx);
oal_module_symbol(dmac_pkt_cap_ba);
oal_module_symbol(dmac_pkt_cap_beacon);
/*lint +e578*//*lint +e19*/


#endif /* _PRE_WLAN_FEATURE_PACKET_CAPTURE */

#ifdef  __cplusplus
#if     __cplusplus
    }
#endif
#endif

