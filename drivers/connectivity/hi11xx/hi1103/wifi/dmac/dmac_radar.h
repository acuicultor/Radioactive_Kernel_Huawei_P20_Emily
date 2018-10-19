

#ifndef __DMAC_RADAR_H__
#define __DMAC_RADAR_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_DFS_OPTIMIZE

/*****************************************************************************
  1 ����ͷ�ļ�����
*****************************************************************************/
#include "hal_ext_if.h"
#include "mac_device.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_RADAR_H

/*****************************************************************************
  2 �궨��
*****************************************************************************/
#define STAGGER_MAX_DURATION                100         /* STAGGER�����״������ */
#define STAGGER_PULSE_MARGIN                4           /* STAGGER�����״���ͬ��������� */
#define RADAR_PULSE_MARGIN_ETSI             4           /* ETSI��������С��� */
#define RADAR_PULSE_MARGIN_FCC              3           /* FCC��������С��� */
#define RADAR_PULSE_MARGIN_FCC_LOW_TYPE     3           /* FCC TYPE0~TYPE2��������С��� */

#define RADAR_PULSE_DURATION_MARGIN            20          /* ��������С��� */
#define RADAR_PULSE_POWER_MARGIN               20          /* ���幦����С��� */
#define RADAR_PULSE_DURATION_MARGIN_ERROR     100          /* ������������ */
#define RADAR_FCC_CHIRP_PULSE_DURATION_MARGIN   5          /* ��������С��� */

#define MAX_RADAR_NORMAL_PULSE_ANA_CNT                  5   /* ��chirp�״�������Ϣ���������� */
#define MAX_RADAR_NORMAL_PULSE_ANA_CNT_ETSI_TYPE3       8   /* ETSI type3�״�������Ϣ���������� */
#define EXTRA_PULSE_CNT                                 3   /* buf�ж����ѯ��������� */

#define MAX_RADAR_STAGGER_NUM               3           /* STAGGER�������������� */
#define MIN_RADAR_STAGGER_DURATION          9           /* STAGGER����������С��� */
#define MEAN_RADAR_STAGGER_DURATION         80          /* STAGGER������������� */

#define MIN_RADAR_NORMAL_DURATION           9           /* normal����������С��� */
#define MIN_RADAR_NORMAL_DURATION_MKK       8           /* normal����������С��� */

#define MIN_RADAR_NORMAL_DURATION_ETSI_TYPE3    180     /* ETSI TYPE3 ��С���*/
#define MAX_RADAR_NORMAL_DURATION_FCC_TYPE2 110         /* FCC TYPE2  ����� */
#define MIN_RADAR_NORMAL_DURATION_FCC_TYPE4 40          /* FCC TYPE4  ��С��� */

#define MIN_RADAR_PULSE_PRI                 148         /* �״�������С���(us) */
#define MIN_ETSI_PULSE_PRI                  248         /* ETSI�״�������С���(us) */

#define MIN_ETSI_CHIRP_PRI                  245         /* ETSI chirp�״���С������(us) */
#define MIN_FCC_CHIRP_PRI                   990         /* FCC chirp�״���С������(us) */
#define MIN_MKK_CHIRP_PRI                   990         /* MKK chirp�״���С������(us) */
#define MIN_ETSI_CHIRP_PRI_NUM              4           /* ETSI chirp�״���С���������� */
#define MIN_FCC_CHIRP_PRI_NUM               1           /* FCC chirp�״���С���������� */
#define MIN_MKK_CHIRP_PRI_NUM               1           /* MKK chirp�״���С���������� */

#define MIN_RADAR_PULSE_POWER               394         /* ����power��Сֵ */
#define MIN_RADAR_PULSE_POWER_FCC_TYPE0     390         /* FCC type0����power��Сֵ */
#define MIN_RADAR_PULSE_POWER_ETSI_STAGGER  394         /* ETSI Stagger����power��Сֵ */

#define RADAR_NORMAL_PULSE_TYPE             0           /* ��chirp�״��������� */
#define RADAR_CHIRP_PULSE_TYPE              1           /* chirp�״��������� */

#define MAX_PULSE_TIMES                     4           /* ������֮������� */

#define MAX_STAGGER_PULSE_TIMES             4           /* stagger�����������������ϵ */
#define MIN_FCC_TOW_TIMES_INT_PRI           100//200    /* FCC chirp�״������жϵ���Сʱ����(ms) */
#define MAX_FCC_TOW_TIMES_INT_PRI           8000        /* FCC chirp�״������жϵ����ʱ����(ms) */
#define MAX_FCC_CHIRP_PULSE_CNT_IN_600US    5           /* FCC chirp�״�һ���ж�600ms������������*/
#define MAX_FCC_CHIRP_EQ_DURATION_NUM       3           /* FCC chirp������ͬduration��������*/

#define RADAR_PULSE_NO_PERIOD               0           /* ���岻�߱������� */
#define RADAR_PULSE_NORMAL_PERIOD           1           /* ����߱������� */
#define RADAR_PULSE_BIG_PERIOD              2           /* ���������ϴ� */
#define RADAR_PULSE_ONE_DIFF_PRI            3           /* �ȼ�������г���һ�����ȼ�� */

#define RADAR_ETSI_PPS_MARGIN               2
#define RADAR_ETSI_TYPE5_MIN_PPS_DIFF       (20 - RADAR_ETSI_PPS_MARGIN)          /* ETSI type5��ͬPRI֮����СPPSƫ��� */
#define RADAR_ETSI_TYPE5_MAX_PPS_DIFF       (50 + RADAR_ETSI_PPS_MARGIN)          /* ETSI type5��ͬPRI֮�����PPSƫ��� */
#define RADAR_ETSI_TYPE6_MIN_PPS_DIFF       (80 - RADAR_ETSI_PPS_MARGIN)          /* ETSI type6��ͬPRI֮����СPPSƫ��� */
#define RADAR_ETSI_TYPE6_MAX_PSS_DIFF       (400 + RADAR_ETSI_PPS_MARGIN)         /* ETSI type6��ͬPRI֮�����PPSƫ��� */

#define RADAR_ONE_SEC_IN_US                 1000000

#define CHECK_RADAR_ETSI_TYPE1_PRI(_a)      (((_a) >= 998 && (_a) <= 5002) ? OAL_TRUE : OAL_FALSE)
#define CHECK_RADAR_ETSI_TYPE2_PRI(_a)      (((_a) >= 623  && (_a) <= 5002) ? OAL_TRUE : OAL_FALSE)
#define CHECK_RADAR_ETSI_TYPE3_PRI(_a)      (((_a) >= 248  && (_a) <= 437) ? OAL_TRUE : OAL_FALSE)
#define CHECK_RADAR_ETSI_TYPE5_PRI(_a)      (((_a) >= 2500 && (_a) <= 3333) ? OAL_TRUE : OAL_FALSE)
#define CHECK_RADAR_ETSI_TYPE6_PRI(_a)      (((_a) >= 833 && (_a) <= 2500) ? OAL_TRUE : OAL_FALSE)
#define CHECK_RADAR_ETSI_STAGGER_PRI(_a)    (((_a) >= 833 && (_a) <= 3333) ? OAL_TRUE : OAL_FALSE)
#define CHECK_RADAR_ETSI_TYPE3_FILTER_DURATION(_a)      (((_a) >= 120  && (_a) <= 180) ? OAL_TRUE : OAL_FALSE)
#define CHECK_RADAR_ETSI_TYPE3_FILTER_POWER(_a)         (((_a) <= 7) ? OAL_TRUE : OAL_FALSE)
#define CHECK_RADAR_ETSI_TYPE6_DURATION_DIFF(_a)   (((_a) <= 47) ? OAL_TRUE : OAL_FALSE)
#define CHECK_RADAR_ETSI_SHORT_PULSE(_a)    (((_a) >= 10 && (_a) <= 30) ? OAL_TRUE : OAL_FALSE)

#define CHECK_RADAR_FCC_TYPE0_PRI(_a)       (((_a) >= 1426  && (_a) <= 1430) ? OAL_TRUE : OAL_FALSE)
#define CHECK_RADAR_FCC_TYPE1_PRI(_a)       (((_a) >= 516  && (_a) <= 3068) ? OAL_TRUE : OAL_FALSE)
#define CHECK_RADAR_FCC_TYPE2_PRI(_a)       (((_a) >= 148  && (_a) <= 232) ? OAL_TRUE : OAL_FALSE)
#define CHECK_RADAR_FCC_TYPE3_PRI(_a)       (((_a) >= 198  && (_a) <= 502) ? OAL_TRUE : OAL_FALSE)
#define CHECK_RADAR_FCC_TYPE4_PRI(_a)       (((_a) >= 198  && (_a) <= 502) ? OAL_TRUE : OAL_FALSE)
#define CHECK_RADAR_FCC_TYPE4_PRI_SMALL(_a) ((((_a) >= 233  && (_a) <= 330) || ((_a) >= 336  && (_a) <= 502)) ? OAL_TRUE : OAL_FALSE)
#define CHECK_RADAR_FCC_TYPE5_PRI(_a)       (((_a) >= 998  && (_a) <= 2002*2) ? OAL_TRUE : OAL_FALSE)
#define CHECK_RADAR_FCC_TYPE6_PRI(_a)       (((_a) >= 331  && (_a) <= 335) ? OAL_TRUE : OAL_FALSE)

#define CHECK_RADAR_FCC_TYPE4_DURATION(_a)  (((_a) >= 40  && (_a) <= 220) ? OAL_TRUE : OAL_FALSE)

#define CHECK_RADAR_FCC_TYPE2_PRI_SMALL(_a) (((_a) >= 148  && (_a) <= 198) ? OAL_TRUE : OAL_FALSE)
#define CHECK_RADAR_FCC_TYPE2_FILTER_CASE_DURATION(_a) (((_a) >= 160  && (_a) <= 180) ? OAL_TRUE : OAL_FALSE)
#define CHECK_RADAR_FCC_TYPE2_FILTER_CASE_PRI(_a)       (((_a) >= 238  && (_a) <= 244) ? OAL_TRUE : OAL_FALSE)
#define CHECK_RADAR_FCC_TYPE2_FILTER_CASE_MIN_DURATION  60
#define CHECK_RADAR_FCC_TYPE2_FILTER_CASE_DURATION_DIFF 15
#define CHECK_RADAR_FCC_CHIRP_TOTAL_CNT(_a)         (((_a) >= 3  && (_a) <= 15) ? OAL_TRUE : OAL_FALSE)
#define CHECK_RADAR_FCC_TYPE4_DURATION_DIFF(_a)     (((_a) <= 30) ? OAL_TRUE : OAL_FALSE)
#define CHECK_RADAR_FCC_TYPE4_POW_DIFF(_a)          (((_a) <= 2) ? OAL_TRUE : OAL_FALSE)
#define CHECK_RADAR_FCC_CHIRP_PRI(_a)               (((_a) >= 7000  && (_a) <= 80000) ? OAL_TRUE : OAL_FALSE)
#define CHECK_RADAR_FCC_CHIRP_POW_DIFF(_a)          (((_a) <= 8) ? OAL_TRUE : OAL_FALSE)

#define TWO_TIMES(_a)                       ((_a) << 1)
#define THREE_TIMES(_a)                     ((_a)*3)

#define TWO_TIMES_STAGGER_PULSE_MARGIN      (2 * STAGGER_PULSE_MARGIN)
#define THREE_TIMES_STAGGER_PULSE_MARGIN    (3 * STAGGER_PULSE_MARGIN)
#define FOUR_TIMES_STAGGER_PULSE_MARGIN     (4 * STAGGER_PULSE_MARGIN)

#define CHECK_STAGGER_PRI_ABSUB(_a)       (((_a) <= RADAR_PULSE_MARGIN_ETSI) || ((_a)>50))

#define CHECK_RADAR_ETSI_TYPE2_HW(_country, _num)   ((2 == (_num)) && HAL_DFS_RADAR_TYPE_ETSI == (_country))
#define CHECK_RADAR_ETSI_TYPE3_HW(_country, _num)   ((3 == (_num)) && HAL_DFS_RADAR_TYPE_ETSI == (_country))
#define CHECK_RADAR_FCC_TYPE4_HW(_country, _num)    ((4 == (_num)) && HAL_DFS_RADAR_TYPE_FCC == (_country))
#define CHECK_RADAR_FCC_TYPE3_HW(_country, _num)    ((3 == (_num)) && HAL_DFS_RADAR_TYPE_FCC == (_country))

#define CHECK_RADAR_ETSI_TYPE23_OR_FCC_TYPE34_HW(_country, _num)    \
        ((HAL_DFS_RADAR_TYPE_ETSI == (_country) && (2 == (_num) || 3 == (_num))) \
        || (HAL_DFS_RADAR_TYPE_FCC == (_country) && (3 == (_num) || 4 == (_num))))

#define CHECK_RADAR_ETSI_TYPE2_IRQ_NUM(_a, _b)  (((_a) >= 1  && (_a) <= 3 && (0 == (_b) ||((_b) >= 1  && (_b) <= 8))) ? OAL_TRUE : OAL_FALSE)
#define CHECK_RADAR_ETSI_TYPE3_IRQ_NUM(_a, _b)  (((_a) >= 1  && (_a) <= 5 && (0 == (_b) ||((_b) >= 1  && (_b) <= 8))) ? OAL_TRUE : OAL_FALSE)

#define CHECK_RADAR_FCC_TYPE4_IRQ_NUM(_a, _b)   (((_a) >= 1  && (_a) <= 3 && (0 == (_b) ||((_b) >= 1  && (_b) <= 4))) ? OAL_TRUE : OAL_FALSE)
#define CHECK_RADAR_FCC_TYPE3_IRQ_NUM(_a, _b)   (((_a) >= 1  && (_a) <= 4 && (0 == (_b) ||((_b) >= 1  && (_b) <= 4))) ? OAL_TRUE : OAL_FALSE)

/*chirp crazy repot*/
#define MAX_IRQ_CNT_IN_CHIRP_CRAZY_REPORT_DET_FCC     100
#define MAX_IRQ_CNT_IN_CHIRP_CRAZY_REPORT_DET_ETSI    40

/*****************************************************************************
  3 ö�ٶ���
*****************************************************************************/
/* staggerģʽ���ڼ������ */
typedef enum
{
    DMAC_RADAR_STAGGER_PERIOD_PRI_EQUAL,            /* ������ͬ��PRI����PRI���Ϸ�ΧҪ�� */
    DMAC_RADAR_STAGGER_PERIOD_PRI_ERR,              /* ������ͬ��PRI����PRI�����Ϸ�ΧҪ�� */
    DMAC_RADAR_STAGGER_PERIOD_NOT_DEC,              /* ��������ͬ��PRI���޷�ȷ�������ԣ�������һ����� */

    DMAC_RADAR_STAGGER_PERIOD_BUTT
} dmac_radar_stagger_period_enum;

typedef oal_uint8 dmac_radar_stagger_period_enum_uint8;

/* staggerģʽradar���� */
typedef enum
{
    DMAC_RADAR_STAGGER_TYPE_INVALID,
    DMAC_RADAR_STAGGER_TYPE5,
    DMAC_RADAR_STAGGER_TYPE6,

    DMAC_RADAR_STAGGER_TYPE_BUTT
} dmac_radar_stagger_type_enum;

typedef oal_uint8 dmac_radar_stagger_type_enum_uint8;

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
/* ������Ϣ������� */
typedef struct
{
    oal_uint32      ul_min_pri;                         /* ��С������ */
    oal_uint16      us_min_duration;                    /* ��С������ */
    oal_uint16      us_max_power;                       /* ����power���ֵ */
    oal_uint32      aul_pri[MAX_RADAR_PULSE_NUM];       /* ���������� */

    oal_uint8       uc_pri_cnt;                         /* ���������� */
    oal_uint8       uc_stagger_cnt;                     /* stagger������Ŀ */
    oal_uint8       uc_begin;                           /* ����������Ϣ����ʼindex */
    oal_uint8       auc_resv1[1];                       /* ����λ */

    oal_uint16      us_avrg_power;                      /* ƽ�����幦�� */
    oal_uint16      us_duration_diff;                   /* ��С������ */
    oal_uint32      ul_extra_pri;                       /* stagger type6�����ȡһ��pri */
    oal_uint32      ul_pri_diff;                        /* ��С������ */

    oal_uint16      us_power_diff;                      /* ����power���ֵ */
    oal_uint16      us_avrg_duration;                   /* ƽ�������� */
} dmac_radar_pulse_analysis_result_stru;


/*****************************************************************************
  8 UNION����
*****************************************************************************/


/*****************************************************************************
  9 OTHERS����
*****************************************************************************/

extern oal_bool_enum_uint8 dmac_radar_filter(mac_device_stru *pst_mac_device, hal_radar_det_event_stru *pst_radar_det_info);
extern oal_bool_enum_uint8 dmac_radar_crazy_report_det_timer(hal_to_dmac_device_stru *pst_hal_device);

#endif /* _PRE_WLAN_FEATURE_DFS_OPTIMIZE */


#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of dmac_vap.h */
