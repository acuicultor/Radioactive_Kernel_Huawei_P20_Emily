

#ifndef    __DMAC_PKT_CAPTURE_H__
#define    __DMAC_PKT_CAPTURE_H__

#ifdef  __cplusplus
#if     __cplusplus
extern  "C" {
#endif
#endif

#ifdef    _PRE_WLAN_FEATURE_PACKET_CAPTURE
/*****************************************************************************
  1 ����ͷ�ļ�����
*****************************************************************************/
#include "oal_ext_if.h"
#include "oam_ext_if.h"
#include "frw_ext_if.h"
#include "hal_ext_if.h"
#include "dmac_vap.h"
#include "dmac_ext_if.h"
#include "wlan_types.h"
#include "mac_vap.h"
#include "mac_frame.h"
#include "hal_commom_ops.h"
#include "oal_net.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_PKT_CAPTURE_H

/*****************************************************************************
  2 �궨��
*****************************************************************************/
/* 2G��5G��Ĭ������ */
#define DMAC_PKT_CAP_2G_RATE    1000
#define DMAC_PKT_CAP_5G_RATE    6000

/* ��Ӧ֡��Ӧ������ѡ�� */
#define DMAC_PKT_CAP_SIFS_RATE1     1000
#define DMAC_PKT_CAP_SIFS_RATE2     6000
#define DMAC_PKT_CAP_SIFS_RATE3     24000

/* Radiotap�汾 */
#define PKTHDR_RADIOTAP_VERSION     0

/* ����ץ����غ� */
#define DMAC_PKT_CAP_TX_NOISE       -95
#define DMAC_PKT_CAP_SIGNAL_OFFSET  -94
#define DMAC_PKT_CAP_NOISE_MAX      0
#define DMAC_PKT_CAP_NOISE_MIN      -100
#define DMAC_PKT_CAP_RATE_UNIT      500

/* CTS\ACK֡��ʱ�� */
#define DMAC_PKT_CAP_CTS_ACK_TIME1      132
#define DMAC_PKT_CAP_CTS_ACK_TIME2      40
#define DMAC_PKT_CAP_CTS_ACK_TIME3      28

/* RTS֡��ʱ�� */
#define DMAC_PKT_CAP_RTS_TIME1      48
#define DMAC_PKT_CAP_RTS_TIME2      28

/* BA֡��ʱ�� */
#define DMAC_PKT_CAP_BA_TIME1       276
#define DMAC_PKT_CAP_BA_TIME2       64
#define DMAC_PKT_CAP_BA_TIME3       32

/*****************************************************************************
  3 ö�ٶ���
*****************************************************************************/
/* ץ��ģʽ���������� */
typedef enum
{
    DMAC_PKT_CAP_NORMAL     = 0,
    DMAC_PKT_CAP_MIX        = 1,
    DMAC_PKT_CAP_MONITOR    = 2,

    DMAC_PKT_CAP_BUTT
}dmac_pkt_cap_type;
typedef oal_uint8 dmac_pkt_cap_type_uint8;

/* Radiotapͷ��present */
typedef enum
{
    DMAC_IEEE80211_RADIOTAP_TSFT = 0x00000001,
    DMAC_IEEE80211_RADIOTAP_FLAGS = 0x00000002,
    DMAC_IEEE80211_RADIOTAP_RATE = 0x00000004,
    DMAC_IEEE80211_RADIOTAP_CHANNEL = 0x00000008,

    DMAC_IEEE80211_RADIOTAP_DBM_ANTSIGNAL = 0x00000020,
    DMAC_IEEE80211_RADIOTAP_DBM_ANTNOISE = 0x00000040,
    DMAC_IEEE80211_RADIOTAP_LOCK_QUALITY = 0x00000080,

    DMAC_IEEE80211_RADIOTAP_MCS = 0x00080000,
    DMAC_IEEE80211_RADIOTAP_VHT = 0x00200000,

    DMAC_IEEE80211_RADIOTAP_BUTT
}ieee80211_radiotap_it_present;
typedef oal_uint32 ieee80211_radiotap_it_present_uint32;

/* Radiotap��չ���ֵ�flags */
typedef enum
{
    DMAC_IEEE80211_RADIOTAP_F_CFP = 0x01,
    DMAC_IEEE80211_RADIOTAP_F_SHORTPRE = 0x02,
    DMAC_IEEE80211_RADIOTAP_F_WEP = 0x04,
    DMAC_IEEE80211_RADIOTAP_F_FRAG = 0x08,
    DMAC_IEEE80211_RADIOTAP_F_FCS = 0x10,
    DMAC_IEEE80211_RADIOTAP_F_DATAPAD = 0x20,
    DMAC_IEEE80211_RADIOTAP_F_BADFCS = 0x40,
    DMAC_IEEE80211_RADIOTAP_F_SHORTGI = 0x80,

    DMAC_IEEE80211_RADIOTAP_F_BUTT
}ieee80211_radiotap_flags;
typedef oal_uint8 ieee80211_radiotap_flags_uint8;

/* Radiotap��չ���ֵ�channel types */
typedef enum
{
    DMAC_IEEE80211_CHAN_CCK = 0x0020,
    DMAC_IEEE80211_CHAN_OFDM = 0x0040,
    DMAC_IEEE80211_CHAN_2GHZ = 0x0080,
    DMAC_IEEE80211_CHAN_5GHZ = 0x0100,
    DMAC_IEEE80211_CHAN_DYN = 0x0400,
    DMAC_IEEE80211_CHAN_HALF = 0x4000,
    DMAC_IEEE80211_CHAN_QUARTER = 0x8000,

    DMAC_IEEE80211_CHAN_BUTT
}ieee80211_radiotap_channel_types;
typedef oal_uint16 ieee80211_radiotap_channel_types_uint16;

/* Radiotap��չ���ֵ�mcs info���ӳ�Աknown */
typedef enum
{
    DMAC_IEEE80211_RADIOTAP_MCS_HAVE_BW = 0x01,
    DMAC_IEEE80211_RADIOTAP_MCS_HAVE_MCS = 0x02,
    DMAC_IEEE80211_RADIOTAP_MCS_HAVE_GI = 0x04,
    DMAC_IEEE80211_RADIOTAP_MCS_HAVE_FMT = 0x08,
    DMAC_IEEE80211_RADIOTAP_MCS_HAVE_FEC = 0x10,
    DMAC_IEEE80211_RADIOTAP_MCS_HAVE_STBC = 0x20,

    DMAC_IEEE80211_RADIOTAP_MCS_HAVE_BUTT
}ieee80211_radiotap_mcs_have;
typedef oal_uint8 ieee80211_radiotap_mcs_have_uint8;

/* Radiotap��չ���ֵ�mcs info���ӳ�Աflags */
typedef enum
{
    DMAC_IEEE80211_RADIOTAP_MCS_BW_MASK = 0x03,
    DMAC_IEEE80211_RADIOTAP_MCS_BW_20 = 0,
    DMAC_IEEE80211_RADIOTAP_MCS_BW_40 = 1,
    DMAC_IEEE80211_RADIOTAP_MCS_BW_20L = 2,
    DMAC_IEEE80211_RADIOTAP_MCS_BW_20U = 3,

    DMAC_IEEE80211_RADIOTAP_MCS_SGI = 0x04,
    DMAC_IEEE80211_RADIOTAP_MCS_FMT_GF = 0x08,
    DMAC_IEEE80211_RADIOTAP_MCS_FEC_LDPC = 0x10,
    DMAC_IEEE80211_RADIOTAP_MCS_STBC_MASK = 0x60,
    DMAC_IEEE80211_RADIOTAP_MCS_STBC_SHIFT = 5,

    DMAC_IEEE80211_RADIOTAP_MCS_BUTT
}ieee80211_radiotap_mcs_flags;
typedef oal_uint8 ieee80211_radiotap_mcs_flags_uint8;

/* Radiotap��չ���ֵ�vht info���ӳ�Աknown */
typedef enum
{
    DMAC_IEEE80211_RADIOTAP_VHT_KNOWN_STBC = 0x0001,
    DMAC_IEEE80211_RADIOTAP_VHT_KNOWN_TXOP_PS_NA = 0x0002,
    DMAC_IEEE80211_RADIOTAP_VHT_KNOWN_GI = 0x0004,
    DMAC_IEEE80211_RADIOTAP_VHT_KNOWN_SGI_NSYM_DIS = 0x0008,
    DMAC_IEEE80211_RADIOTAP_VHT_KNOWN_LDPC_EXTRA_OFDM_SYM = 0x0010,
    DMAC_IEEE80211_RADIOTAP_VHT_KNOWN_BEAMFORMED = 0x0020,
    DMAC_IEEE80211_RADIOTAP_VHT_KNOWN_BANDWIDTH = 0x0040,
    DMAC_IEEE80211_RADIOTAP_VHT_KNOWN_GROUP_ID = 0x0080,
    DMAC_IEEE80211_RADIOTAP_VHT_KNOWN_PARTIAL_AID = 0x0100,

    DMAC_IEEE80211_RADIOTAP_VHT_KNOWN_BUTT
}ieee80211_radiotap_vht_known;
typedef oal_uint16 ieee80211_radiotap_vht_known_uint16;

/* Radiotap��չ���ֵ�vht info���ӳ�Աflags */
typedef enum
{
    DMAC_IEEE80211_RADIOTAP_VHT_FLAG_STBC = 0x01,
    DMAC_IEEE80211_RADIOTAP_VHT_FLAG_TXOP_PS_NA = 0x02,
    DMAC_IEEE80211_RADIOTAP_VHT_FLAG_SGI = 0x04,
    DMAC_IEEE80211_RADIOTAP_VHT_FLAG_SGI_NSYM_M10_9 = 0x08,
    DMAC_IEEE80211_RADIOTAP_VHT_FLAG_LDPC_EXTRA_OFDM_SYM = 0x10,
    DMAC_IEEE80211_RADIOTAP_VHT_FLAG_BEAMFORMED = 0x20,

    DMAC_IEEE80211_RADIOTAP_VHT_FLAG_BUTT
}ieee80211_radiotap_vht_flags;
typedef oal_uint8 ieee80211_radiotap_vht_flags_uint8;

/* Radiotap��չ���ֵ�vht info���ӳ�Աbandwidth */
typedef enum
{
    DMAC_IEEE80211_RADIOTAP_VHT_BW_20   = 0,
    DMAC_IEEE80211_RADIOTAP_VHT_BW_40   = 1,
    DMAC_IEEE80211_RADIOTAP_VHT_BW_80   = 4,
    DMAC_IEEE80211_RADIOTAP_VHT_BW_160  = 11,

    DMAC_IEEE80211_RADIOTAP_VHT_BW_BUTT
}ieee80211_radiotap_vht_bandwidth;
typedef oal_uint8 ieee80211_radiotap_vht_bandwidth_uint8;

/*  */
typedef enum
{
    DMAC_IEEE80211_RADIOTAP_CODING_LDPC_USER0 = 0x01,
    DMAC_IEEE80211_RADIOTAP_CODING_LDPC_USER1 = 0x02,
    DMAC_IEEE80211_RADIOTAP_CODING_LDPC_USER2 = 0x04,
    DMAC_IEEE80211_RADIOTAP_CODING_LDPC_USER3 = 0x08,

    DMAC_IEEE80211_RADIOTAP_CODING_LDPC_BUTT
}ieee80211_radiotap_vht_coding;
typedef oal_uint8 ieee80211_radiotap_vht_coding_uint8;


/*****************************************************************************
  4 ȫ�ֱ�������
*****************************************************************************/

/*****************************************************************************
  5 ��Ϣͷ����
*****************************************************************************/


/*****************************************************************************
  6 ��Ϣ����
*****************************************************************************/


/*****************************************************************************
  7 STRUCT����
*****************************************************************************/
typedef struct
{
    oal_uint8     uc_it_version;            /* ʹ��radiotap header����Ҫ�汾, Ŀǰ����Ϊ0 */
    oal_uint8     uc_it_pad;                /* Ŀǰ��δʹ��, ֻ��Ϊ��4�ֽڶ��� */
    oal_uint16    us_it_len;                /* radiotap�ĳ���, ������header+fields */
    oal_uint32    ul_it_present;            /* ͨ��bitλ����fields����Щ��Ա */
}ieee80211_radiotap_header_stru;

typedef struct
{
    oal_uint64    ull_timestamp;            /* ��ǰ֡��ʱ���, ��λΪus */
    oal_uint8     uc_flags;                 /* ��־λ */
    oal_uint8     uc_data_rate;             /* TX/RX��������, ��λΪ500Kbps */
    oal_uint16    us_channel_freq;          /* AP�����ŵ�������Ƶ��, ��λMHz */
    oal_uint16    us_channel_type;          /* �ŵ�����, ��ʶ5G����2G */
    oal_int8      c_ssi_signal;             /* �ź�ǿ��, ��λΪdBm */
    oal_int8      c_ssi_noise;              /* ����ǿ��, ��λΪdBm */
    oal_int16     s_signal_quality;         /* �������岻��, ��Ʒ���ļ��㷽����RSSI + 94 */
    oal_uint8     uc_mcs_info_known;        /* mcs��Ϣ, 11nЭ��ʱ���ֶ���Ч */
    oal_uint8     uc_mcs_info_flags;
    oal_uint8     uc_mcs_info_rate;
    oal_uint16    us_vht_known;             /* vht��Ϣ, 11acЭ��ʱ���ֶ���Ч */
    oal_uint8     uc_vht_flags;
    oal_uint8     uc_vht_bandwidth;
    oal_uint8     uc_vht_mcs_nss[4];
    oal_uint8     uc_vht_coding;
    oal_uint8     uc_vht_group_id;
    oal_uint16    us_vht_partial_aid;
}ieee80211_radiotap_fields_stru;

typedef struct
{
    ieee80211_radiotap_header_stru  st_radiotap_header;     /* radiotapͷ�ṹ�� */
    ieee80211_radiotap_fields_stru  st_radiotap_fields;     /* radiotap����ṹ�� */
}ieee80211_radiotap_stru;

typedef struct
{
    oal_uint32                 *pul_circle_buf_start;        /* ���͹���ץ����ѭ��buffer��ʼ��ַ */
    oal_uint16                  us_circle_buf_depth;         /* ���͹���ץ����ѭ��buffer��� */
    oal_uint16                  us_circle_buf_index;         /* device�ṹ�µ�ץ������ѭ��buffer��index */
    oal_uint8                   uc_capture_switch;           /* �����ץ�����Զ�Ӧ��ģʽ���� */
    oal_bool_enum_uint8         en_report_sdt_switch;        /* �����ץ����Ϣ�Ƿ��ϱ���sdt�Ŀ��� */
    oal_uint16                  us_reserved;                 /* �����ֶ� */
    oal_uint32                  ul_total_report_pkt_num;     /* ץ��ģʽ���ܹ��ϱ���ץ������ */
}dmac_packet_stru;

/* ץ��ģʽ�µ�ѭ��buffer�ṹ�� */
typedef struct
{
    oal_uint32     *pul_link_addr;                  /* ���������������ַ */
    oal_uint8       uc_peer_index;                  /* �û�������, ����ʹ�� */
    oal_uint8       bit_tx_cnt      :   4,          /* ��ǰ֡�ڼ��η��� */
                    bit_q_type      :   3,          /* ���Ͷ��к�, ����ʹ�� */
                    bit_frm_type    :   1;          /* ֡����, 0ΪRTS, 1Ϊ��RTS */
    oal_uint16      bit_reserved    :   15,         /* �����ֶ� */
                    bit_status      :   1;          /* buffer��Ϣ��Ч��ʶ */
    oal_uint8       uc_timestamp[8];                /* ���͵�ʱ���, mac������δ����ʱ�� */
}dmac_mem_circle_buf_stru;


/*****************************************************************************
  8 UNION����
*****************************************************************************/


/*****************************************************************************
  9 OTHERS����
*****************************************************************************/

/*****************************************************************************
  10 ��������
*****************************************************************************/
extern oal_uint32  dmac_pkt_cap_tx(dmac_packet_stru *pst_dmac_packet, hal_to_dmac_device_stru *pst_hal_device);
extern oal_uint32  dmac_pkt_cap_rx(dmac_rx_ctl_stru *pst_cb_ctrl, oal_netbuf_stru *pst_netbuf, dmac_packet_stru *pst_dmac_packet, hal_to_dmac_device_stru *pst_hal_device, oal_int8 c_rssi_ampdu);
extern oal_uint32  dmac_pkt_cap_ba(dmac_packet_stru *pst_dmac_packet, dmac_rx_ctl_stru *pst_cb_ctrl, hal_to_dmac_device_stru *pst_hal_device);
extern oal_uint32  dmac_pkt_cap_beacon(dmac_packet_stru *pst_dmac_packet, dmac_vap_stru *pst_dmac_vap);
extern oal_void  dmac_tx_get_vap_id(hal_to_dmac_device_stru *pst_hal_device, hal_tx_dscr_stru *pst_tx_dscr, oal_uint8 *puc_vap_id);


#endif /* _PRE_WLAN_FEATURE_PACKET_CAPTURE */

#ifdef  __cplusplus
#if     __cplusplus
    }
#endif
#endif

#endif

