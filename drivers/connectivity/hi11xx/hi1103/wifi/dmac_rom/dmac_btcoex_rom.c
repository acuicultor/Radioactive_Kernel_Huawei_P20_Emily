


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
#include "dmac_btcoex.h"
#include "dmac_device.h"
#include "dmac_resource.h"
#include "dmac_config.h"
#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_BTCOEX_ROM_C

/*****************************************************************************
  2 ȫ�ֱ�������
*****************************************************************************/

/*****************************************************************************
  3 ����ʵ��
*****************************************************************************/


oal_void  dmac_btcoex_set_occupied_period(dmac_vap_stru *pst_dmac_vap, oal_uint16 us_occupied_period)
{
    hal_to_dmac_device_stru *pst_hal_device;

    pst_hal_device = DMAC_VAP_GET_HAL_DEVICE((mac_vap_stru *)pst_dmac_vap);
    if(OAL_PTR_NULL == pst_hal_device)
    {
        OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_set_occupied_period:pst_hal_device is NULL}\n");
        return;
    }

#if(_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)
    if(WLAN_BAND_5G == pst_dmac_vap->st_vap_base_info.st_channel.en_band)
    {
        /* 5G����Ҫ��occupy���� */
        OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_COEX,
            "{dmac_btcoex_set_occupied_period:: 5G donot need to set occupied.}");
    }
    else
#endif
    {
        hal_set_btcoex_occupied_period(pst_hal_device, us_occupied_period);
    }
}




oal_void dmac_btcoex_set_mgmt_priority(mac_vap_stru *pst_mac_vap, oal_uint16 us_timeout_ms)
{
    hal_to_dmac_device_stru       *pst_hal_device;

    pst_hal_device  = DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap);
    if (OAL_PTR_NULL == pst_hal_device)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_set_mgmt_priority:: DMAC_VAP_GET_HAL_DEVICE null}");
        return;
    }

#if(_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)
    if(WLAN_BAND_5G == pst_mac_vap->st_channel.en_band)
    {
        /* 5G����Ҫ��priority���� */
        return;
    }
#endif

    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_set_mgmt_priority:: set us_timeout_us[%d]}",us_timeout_ms);

    /* 16'hffff����������priority    16'h0������priority   others�������õ�ʱ������priority */
    hal_set_btcoex_priority_period(pst_hal_device, us_timeout_ms);

}




oal_void dmac_btcoex_encap_preempt_frame(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user)
{
    hal_to_dmac_device_stru          *pst_hal_device;
    dmac_vap_stru                    *pst_dmac_vap       = OAL_PTR_NULL;
    oal_uint32                        ul_qosnull_seq_num = 0;

    pst_dmac_vap   = MAC_GET_DMAC_VAP(pst_mac_vap);
    pst_hal_device = DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap);
    if (OAL_PTR_NULL == pst_hal_device)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_encap_preempt_frame:: DMAC_VAP_GET_HAL_DEVICE null}");
        return;
    }

    /* ��ʼ��dmac vap��Ӧ��preempt�ֶΣ�����д��Ӳ����premmpt֡��ַ��premmpt֡�Ĳ��� */
    OAL_MEMZERO(&(pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_null_preempt_param), OAL_SIZEOF(dmac_vap_btcoex_null_preempt_stru));

    switch (pst_dmac_vap->st_dmac_vap_btcoex.en_all_abort_preempt_type)
    {
        case HAL_BTCOEX_HW_POWSAVE_NOFRAME:
        case HAL_BTCOEX_HW_POWSAVE_SELFCTS:
            break;

        case HAL_BTCOEX_HW_POWSAVE_NULLDATA:
        {
            /* ��д֡ͷ,����from dsΪ1��to dsΪ0��ps=1 ���frame control�ĵڶ����ֽ�Ϊ12 */
            mac_ieee80211_frame_stru *pst_mac_header =
                (mac_ieee80211_frame_stru *)pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_null_preempt_param.auc_null_qosnull_frame;
            mac_null_data_encap(pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_null_preempt_param.auc_null_qosnull_frame,
                                WLAN_PROTOCOL_VERSION | WLAN_FC0_TYPE_DATA | WLAN_FC0_SUBTYPE_NODATA | 0x1100,
                                pst_mac_user->auc_user_mac_addr,
                                mac_mib_get_StationID(pst_mac_vap));

            /* �趨seq num��frag */
            pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_null_preempt_param.auc_null_qosnull_frame[22] = 0;
            pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_null_preempt_param.auc_null_qosnull_frame[23] = 0;
            pst_mac_header->st_frame_control.bit_power_mgmt = 1;
            break;
        }

        case HAL_BTCOEX_HW_POWSAVE_QOSNULL:
        {
            dmac_btcoex_qosnull_frame_stru *pst_mac_header =
                (dmac_btcoex_qosnull_frame_stru *)pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_null_preempt_param.auc_null_qosnull_frame;
            mac_null_data_encap(pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_null_preempt_param.auc_null_qosnull_frame,
                                WLAN_PROTOCOL_VERSION | WLAN_FC0_TYPE_DATA | WLAN_FC0_SUBTYPE_QOS_NULL | 0x1100,
                                pst_mac_user->auc_user_mac_addr,
                                mac_mib_get_StationID(pst_mac_vap));

            pst_mac_header->st_frame_control.bit_power_mgmt = 1;
            pst_mac_header->bit_qc_tid = WLAN_TIDNO_COEX_QOSNULL;
            pst_mac_header->bit_qc_eosp = 0;

            /* ����seq�����к� */
            hal_get_btcoex_abort_qos_null_seq_num(pst_hal_device, &ul_qosnull_seq_num);
            pst_mac_header->bit_sc_seq_num = (ul_qosnull_seq_num + 1);
            hal_set_btcoex_abort_qos_null_seq_num(pst_hal_device, pst_mac_header->bit_sc_seq_num);

            /*Э��涨������QOS NULL DATAֻ����normal ack ��������Ҫ����0�ǶԷ����ack */
            pst_mac_header->bit_qc_ack_polocy = WLAN_TX_NORMAL_ACK;
            break;
        }

        default:
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_encap_preempt_frame::en_sw_preempt_type[%d] error!}",
                pst_dmac_vap->st_dmac_vap_btcoex.en_all_abort_preempt_type);
            return;
    }

    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_encap_preempt_frame::en_sw_preempt_type[%d]!}",
                pst_dmac_vap->st_dmac_vap_btcoex.en_all_abort_preempt_type);
}




oal_void dmac_btcoex_set_preempt_frame_param(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user)
{
    hal_to_dmac_device_stru          *pst_hal_device;
    dmac_vap_stru                    *pst_dmac_vap         = OAL_PTR_NULL;
    dmac_user_stru                   *pst_dmac_user        = OAL_PTR_NULL;
    oal_uint16                        us_preempt_param_val = 0;

    pst_dmac_vap   = MAC_GET_DMAC_VAP(pst_mac_vap);
    pst_dmac_user  = MAC_GET_DMAC_USER(pst_mac_user);
    pst_hal_device = DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap);
    if (OAL_PTR_NULL == pst_hal_device)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_set_preempt_frame_param:: DMAC_VAP_GET_HAL_DEVICE null}");
        return;
    }

    switch (pst_dmac_vap->st_dmac_vap_btcoex.en_all_abort_preempt_type)
    {
        case HAL_BTCOEX_HW_POWSAVE_NOFRAME:
            /* ���ӽӿ�ͨ���ԣ�NOFRAMEֱ�ӷ��أ�д��Ӧ�Ĵ�������Ϊ0���� */
            return;

        case HAL_BTCOEX_HW_POWSAVE_SELFCTS:
            break;

        case HAL_BTCOEX_HW_POWSAVE_NULLDATA:
            pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_null_preempt_param.bit_cfg_coex_tx_peer_index   = pst_dmac_user->uc_lut_index;

            /* null����peer idx */
            us_preempt_param_val = us_preempt_param_val |
                pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_null_preempt_param.bit_cfg_coex_tx_peer_index;
            break;

        case HAL_BTCOEX_HW_POWSAVE_QOSNULL:
            pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_null_preempt_param.bit_cfg_coex_tx_peer_index =  pst_dmac_user->uc_lut_index;
            pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_null_preempt_param.bit_cfg_coex_tx_qos_null_tid = WLAN_TIDNO_COEX_QOSNULL;

            /* qos null��Ҫ����tid */
            us_preempt_param_val = us_preempt_param_val |
                pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_null_preempt_param.bit_cfg_coex_tx_qos_null_tid << 8;
            /* qos null����peer idx */
            us_preempt_param_val = us_preempt_param_val |
                pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_null_preempt_param.bit_cfg_coex_tx_peer_index;
            break;

        default:
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_set_preempt_frame_param::en_sw_preempt_type[%d] error!}",
                pst_dmac_vap->st_dmac_vap_btcoex.en_all_abort_preempt_type);
           return;
    }

    /* 03��self cts��ʱ��Ҳ��Ҫvap index; 02���������Щvap���0~4�ң������ҵ���vap��mac��ַ��Ϊself cts�ĵ�ַ */
    pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_null_preempt_param.bit_cfg_coex_tx_vap_index = pst_dmac_vap->pst_hal_vap->uc_vap_id;

    /* ����֡����Ҫ����vap index�� self cts��ʱ��Ҳ��Ҫvap index(pilot�Ż����������д�ã�����ע����֤) */
    us_preempt_param_val = us_preempt_param_val |
        (pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_null_preempt_param.bit_cfg_coex_tx_vap_index << 12);

    hal_set_btcoex_abort_preempt_frame_param(pst_hal_device, us_preempt_param_val);

    OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_set_preempt_frame_param::SUCC en_sw_preempt_type[%d] us_preempt_param_val[%d]!}",
                pst_dmac_vap->st_dmac_vap_btcoex.en_all_abort_preempt_type, us_preempt_param_val);
}




oal_void dmac_btcoex_init_preempt(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user)
{
    dmac_vap_btcoex_stru    *pst_dmac_vap_btcoex;
#if 0 //���оƬ��֤��򿪴˴�������
    oal_uint8                auc_mac_vap_id[WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE] = {0};
#endif

    pst_dmac_vap_btcoex = &(MAC_GET_DMAC_VAP(pst_mac_vap)->st_dmac_vap_btcoex);

    /* 1. �������ͣ�ֱ�ӷ��� */
    if (HAL_BTCOEX_HW_POWSAVE_BUTT <= pst_dmac_vap_btcoex->en_all_abort_preempt_type)
    {
        return;
    }

#if 0 //���оƬ��֤��򿪴˴�������
    /* 2. ��vapʱ�򣬷���preempt֡��ʽ����Ҫǿ���е�HAL_BTCOEX_SELFCTSģʽ */
    if(2 <= hal_device_find_all_up_vap(pst_hal_device, auc_mac_vap_id)
       &&(HAL_BTCOEX_HW_POWSAVE_SELFCTS != pst_dmac_vap_btcoex->en_all_abort_preempt_type))
    {
        pst_dmac_vap_btcoex->en_all_abort_preempt_type = HAL_BTCOEX_HW_POWSAVE_SELFCTS;
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_init_preempt:: multi vap switch to en_sw_preempt_type[%d]!}",
                pst_dmac_vap_btcoex->en_all_abort_preempt_type);
    }
#endif

    /* 3. encap preempt֡ */
    dmac_btcoex_encap_preempt_frame(pst_mac_vap, pst_mac_user);

    /* 4. ����premmpt֡�Ĳ��� */
    dmac_btcoex_set_preempt_frame_param(pst_mac_vap, pst_mac_user);

    /* 5. д��null qos null֡��ַ */
    if ((HAL_BTCOEX_HW_POWSAVE_NULLDATA == pst_dmac_vap_btcoex->en_all_abort_preempt_type)
        ||(HAL_BTCOEX_HW_POWSAVE_QOSNULL == pst_dmac_vap_btcoex->en_all_abort_preempt_type))
    {
        hal_set_btcoex_abort_null_buff_addr(DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap), (oal_uint32)(pst_dmac_vap_btcoex->st_dmac_vap_btcoex_null_preempt_param.auc_null_qosnull_frame));
    }

    /* 6. ����оƬpremmpt���� */
    hal_set_btcoex_tx_abort_preempt_type(DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap), pst_dmac_vap_btcoex->en_all_abort_preempt_type);
}

#endif /* end of _PRE_WLAN_FEATURE_BTCOEX */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif


