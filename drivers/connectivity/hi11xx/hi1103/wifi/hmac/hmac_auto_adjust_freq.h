

#ifndef __MAC_AUTO_ADJUST_FREQ_H__
#define __MAC_AUTO_ADJUST_FREQ_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/*****************************************************************************
  1 ����ͷ�ļ�����
*****************************************************************************/

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
/*****************************************************************************
  2 �궨��
*****************************************************************************/
#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
/*
 * set cpu freq A53-0
 * this function used on K3V3+ platform
 */
#define CPU_MAX_FREQ "/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq"
#define CPU_MIN_FREQ "/sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq"

/*
 * set ddr freq
 * this function used on K3V3+ platform
 */
#define DDR_MAX_FREQ "/sys/class/devfreq/ddrfreq/max_freq"
#define DDR_MIN_FREQ "/sys/class/devfreq/ddrfreq/min_freq"

#define MAX_DEGRADE_FREQ_COUNT_THRESHOLD_SUCCESSIVE_3 (3)         /*����3�����ڶ���Ҫ��Ƶ�Ž�Ƶ*/
#define MAX_DEGRADE_FREQ_COUNT_THRESHOLD_SUCCESSIVE_10 (100)    /*�а�ʱ����100�����ڶ���Ҫ��Ƶ�Ž�Ƶ*/

/* WIFI���������ϴ�ʱ���շ��жϰ��ڴ�� */
#define WLAN_IRQ_AFFINITY_IDLE_CPU   0
#define WLAN_IRQ_AFFINITY_BUSY_CPU   4

#define WLAN_IRQ_THROUGHPUT_THRESHOLD_HIGH    32000  /* 250*1024/8 = K bytes/s */
#define WLAN_IRQ_THROUGHPUT_THRESHOLD_LOW     19200  /* 150*1024/8 = K bytes/s */

#endif

#define WLAN_FREQ_TIMER_PERIOD    (100)                         /*��ʱ��100ms��ʱ*/
#define WLAN_THROUGHPUT_STA_PERIOD   20

/*****************************************************************************
  3 ö�ٶ���
*****************************************************************************/
#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
enum
{
    /* indicate to set cpu/ddr max frequencies */
    SCALING_MAX_FREQ                        = 0,

    /* indicate to set cpu/ddr min frequencies */
    SCALING_MIN_FREQ                        = 1,
};

enum
{
    /* frequency lock disable mode */
    FREQ_LOCK_DISABLE                       = 0,

    /* frequency lock enable mode */
    FREQ_LOCK_ENABLE                        = 1,
};
typedef oal_uint8 oal_freq_lock_enum_uint8;

typedef enum
{
    CMD_SET_AUTO_FREQ_ENDABLE,
    CMD_SET_DEVICE_FREQ_VALUE,
    CMD_SET_CPU_FREQ_VALUE,
    CMD_SET_DDR_FREQ_VALUE,
    CMD_SET_AUTO_BYPASS_DEVICE_AUTO_FREQ,
    CMD_GET_DEVICE_AUTO_FREQ,
    CMD_SET_DEVICE_FREQ_TC,
    CMD_AUTO_FREQ_BUTT,
}oal_auto_freq_cmd_enum;
typedef oal_uint8 oal_auto_freq_cmd_enum_uint8;
#endif

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
typedef struct {
#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
    oal_uint32  ul_tx_pkts;  /* WIFI ҵ����֡ͳ�� */
    oal_uint32  ul_rx_pkts;   /* WIFI ҵ�����֡ͳ�� */
#endif
    oal_uint32  ul_tx_bytes;  /* WIFI ҵ����֡ͳ�� */
    oal_uint32  ul_rx_bytes;   /* WIFI ҵ�����֡ͳ�� */
}wifi_txrx_pkt_stat;

#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
typedef struct {
    oal_uint8  uc_device_type;   /*device��Ƶ����*/
    oal_uint8  uc_reserve[3];   /*�����ֶ�*/
} device_speed_freq_level_stru;
typedef struct {
    oal_uint32  ul_speed_level;    /*����������*/
    oal_uint32  ul_min_cpu_freq;  /*CPU��Ƶ����*/
    oal_uint32  ul_min_ddr_freq;   /*DDR��Ƶ����*/
} host_speed_freq_level_stru;
extern host_speed_freq_level_stru g_host_speed_freq_level[];
extern device_speed_freq_level_stru g_device_speed_freq_level[];
#endif

typedef struct {
#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
    oal_bool_enum_uint8  en_is_inited;
    oal_uint8            uc_lock_mod;  /*ʹ�ܿ���*/
    oal_uint8            uc_curr_lock_level;  /*��ǰ��Ƶ�ȼ�*/
    oal_uint8            uc_req_lock_level;   /*Ŀ����Ƶ�ȼ�*/
    oal_uint32           ul_pre_jiffies;
    oal_uint32           ul_adjust_count;
    oal_mutex_stru       st_lock_freq_mtx;
	//oal_work_stru        st_work;
#endif
    oal_bool_enum_uint8  en_irq_affinity;  /*�жϿ���*/
    oal_uint8            uc_timer_cycles;  /* ��ʱ�������� */
    oal_uint8            uc_cur_irq_cpu;  /* ��ǰ�ж�����CPU */
    oal_uint8            uc_req_irq_cpu;  /* �����ж�����CPU */
    oal_uint32           ul_pre_time;        /* ��һ��ͳ��ʱ�� */
    oal_uint32           ul_total_sdio_pps;  /*��������*/
    oal_uint16           us_throughput_irq_high;
    oal_uint16           us_throughput_irq_low;
    frw_timeout_stru     hmac_freq_timer;     /*auto freq timer*/
} freq_lock_control_stru;
extern freq_lock_control_stru g_freq_lock_control;

#ifdef _PRE_WLAN_FEATURE_NEGTIVE_DET
extern oal_bool_enum_uint8 g_en_pk_mode_swtich;
extern oal_uint32 g_st_pk_mode_high_th_table[WLAN_PROTOCOL_CAP_BUTT][WLAN_BW_CAP_BUTT];
extern oal_uint32 g_st_pk_mode_low_th_table[WLAN_PROTOCOL_CAP_BUTT][WLAN_BW_CAP_BUTT];
#endif
/*****************************************************************************
  8 UNION����
*****************************************************************************/


/*****************************************************************************
  9 OTHERS����
*****************************************************************************/


/*****************************************************************************
  10 ��������
*****************************************************************************/
extern oal_bool_enum_uint8 hmac_set_auto_freq_mod(oal_freq_lock_enum_uint8 uc_freq_enable);
extern oal_bool_enum_uint8 hmac_set_cpu_freq_raw(oal_uint8 uc_freq_type, oal_uint32 ul_freq_value);
extern oal_bool_enum_uint8 hmac_set_ddr_freq_raw(oal_uint8 uc_freq_type, oal_uint32 ul_freq_value);
extern oal_void hmac_auto_freq_wifi_rx_stat(oal_uint32 ul_pkt_count);
extern oal_void hmac_auto_freq_wifi_tx_stat(oal_uint32 ul_pkt_count);
extern oal_void hmac_adjust_freq(oal_void);
extern oal_void hmac_adjust_set_freq(oal_void);
extern oal_void hmac_wifi_auto_freq_ctrl_init(void);
extern oal_void hmac_wifi_auto_freq_ctrl_deinit(void);
#endif /* end of mac_auto_adjust_freq.h */

extern oal_void  hmac_freq_timer_init(oal_void);
extern oal_void  hmac_freq_timer_deinit(oal_void);
extern oal_void hmac_auto_freq_wifi_rx_bytes_stat(oal_uint32 ul_pkt_bytes);
extern oal_void hmac_auto_freq_wifi_tx_bytes_stat(oal_uint32 ul_pkt_bytes);
extern oal_void  hmac_wifi_pm_state_notify(oal_bool_enum_uint8 en_wake_up);
extern oal_void  hmac_wifi_state_notify(oal_bool_enum_uint8 en_wifi_on);
#endif

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif


