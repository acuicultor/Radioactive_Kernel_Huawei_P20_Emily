


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_AP_PM
/*****************************************************************************
  1 ͷ�ļ�����
*****************************************************************************/
#include "oal_ext_if.h"
#include "mac_device.h"
#include "mac_resource.h"
#include "mac_pm.h"
#include "dmac_ap_pm.h"
#include "dmac_vap.h"
#include "frw_timer.h"
#include "dmac_green_ap.h"
#include "hal_ext_if.h"


#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_AP_PM_C

OAL_STATIC oal_void ap_power_state_work_entry(oal_void *p_ctx);
OAL_STATIC oal_void ap_power_state_work_exit(oal_void *p_ctx);
OAL_STATIC oal_uint32 ap_power_state_work_event(oal_void *p_ctx,
                                                        oal_uint16    us_event,
                                                        oal_uint16    us_event_data_len,
                                                        oal_void      *p_event_data);

OAL_STATIC oal_void ap_power_state_deep_sleep_entry(oal_void *p_ctx);
OAL_STATIC oal_void ap_power_state_deep_sleep_exit(oal_void *p_ctx);
OAL_STATIC oal_uint32 ap_power_state_deep_sleep_event(oal_void      *p_ctx,
                                                        oal_uint16    us_event,
                                                        oal_uint16    us_event_data_len,
                                                        oal_void      *p_event_data);

OAL_STATIC oal_void ap_power_state_wow_entry(oal_void *p_ctx);
OAL_STATIC oal_void ap_power_state_wow_exit(oal_void *p_ctx);
OAL_STATIC oal_uint32 ap_power_state_wow_event(oal_void      *p_ctx,
                                                        oal_uint16    us_event,
                                                        oal_uint16    us_event_data_len,
                                                        oal_void      *p_event_data);

OAL_STATIC oal_void ap_power_state_idle_entry(oal_void *p_ctx);
OAL_STATIC oal_void ap_power_state_idle_exit(oal_void *p_ctx);
OAL_STATIC oal_uint32 ap_power_state_idle_event(oal_void      *p_ctx,
                                                        oal_uint16    us_event,
                                                        oal_uint16    us_event_data_len,
                                                        oal_void      *p_event_data);


OAL_STATIC oal_void ap_power_state_off_entry(oal_void *p_ctx);
OAL_STATIC oal_void ap_power_state_off_exit(oal_void *p_ctx);
OAL_STATIC oal_uint32 ap_power_state_off_event(oal_void      *p_ctx,
                                                        oal_uint16    us_event,
                                                        oal_uint16    us_event_data_len,
                                                        oal_void      *p_event_data);

OAL_STATIC oal_uint32 dmac_pm_ap_inactive_timer(oal_void* pst_arg);
OAL_STATIC oal_void dmac_pm_state_trans(mac_pm_handler_stru* pst_handler,oal_uint8 uc_state);

OAL_STATIC oal_uint8 dmac_pm_ap_sta_wow_available(mac_device_stru *pst_device);

#ifdef _PRE_E5_722_PLATFORM
extern oal_void wlan_set_driver_lock(oal_int32 locked);
#endif

/*****************************************************************************
  2 ȫ�ֱ�������
*****************************************************************************/
/* ȫ��״̬�������� */
oal_fsm_state_info  g_ap_power_fsm_info[] = {
    {
          PWR_SAVE_STATE_WORK,
          "WORK",
          ap_power_state_work_entry,
          ap_power_state_work_exit,
          ap_power_state_work_event,
    },
    {
          PWR_SAVE_STATE_DEEP_SLEEP,
          "DEEP_SLEEP",
          ap_power_state_deep_sleep_entry,
          ap_power_state_deep_sleep_exit,
          ap_power_state_deep_sleep_event,
    },
    {
          PWR_SAVE_STATE_WOW,
          "WOW",
          ap_power_state_wow_entry,
          ap_power_state_wow_exit,
          ap_power_state_wow_event,
    },
    {
          PWR_SAVE_STATE_IDLE,
          "IDLE",
          ap_power_state_idle_entry,
          ap_power_state_idle_exit,
          ap_power_state_idle_event,
    },
    {
          PWR_SAVE_STATE_OFF,
          "OFF",
          ap_power_state_off_entry,
          ap_power_state_off_exit,
          ap_power_state_off_event,
    }
};

/*VAP״̬��device״̬��ӳ���*/
oal_uint8 g_pm_state_map[PWR_SAVE_STATE_BUTT] =
{
    DEV_PWR_STATE_WORK,          //->PWR_SAVE_STATE_WORK = 0,         /*����״̬*/
    DEV_PWR_STATE_DEEP_SLEEP,    //->PWR_SAVE_STATE_DEEP_SLEEP,      /*��˯״̬*/
    DEV_PWR_STATE_WOW,           //->PWR_SAVE_STATE_WOW,             /*WOW״̬*/
    DEV_PWR_STATE_IDLE,         //->PWR_SAVE_STATE_IDLE,            /*idle״̬�����û�����*/
    DEV_PWR_STATE_OFF            //->PWR_SAVE_STATE_OFF,
};

#define DMAC_VAP2DEV_PM_STATE(_uc_state) (g_pm_state_map[_uc_state])

/*****************************************************************************
  3 ����ʵ��
*****************************************************************************/

OAL_STATIC oal_void ap_power_state_work_entry(oal_void *p_ctx)
{
    mac_pm_handler_stru*    pst_pm_handler = (mac_pm_handler_stru*)p_ctx;
    oal_uint32              ul_core_id;

    ul_core_id = OAL_GET_CORE_ID();
    //dmac_vap_stru *pst_dmac_vap = (dmac_vap_stru *)(pst_pm_handler->p_mac_fsm->p_oshandler);


    if(OAL_PTR_NULL == pst_pm_handler)
    {
        return ;
    }

    pst_pm_handler->ul_user_check_count = 0;
    pst_pm_handler->ul_inactive_time    = 0;
    if((pst_pm_handler->bit_pwr_siso_en)||(pst_pm_handler->bit_pwr_wow_en))
    {
        if (OAL_FALSE == pst_pm_handler->st_inactive_timer.en_is_registerd)
        {
            FRW_TIMER_CREATE_TIMER(&pst_pm_handler->st_inactive_timer,
                                   dmac_pm_ap_inactive_timer,
                                   AP_PWR_DEFAULT_USR_CHECK_TIME,      /* 5000ms����һ�� */
                                   (oal_void*)pst_pm_handler,
                                   OAL_TRUE,
                                   OAM_MODULE_ID_DMAC,
                                   ul_core_id);
        }
    }

    return;
}


OAL_STATIC oal_void ap_power_state_work_exit(oal_void *p_ctx)
{
    mac_pm_handler_stru *pst_pm_handler = (mac_pm_handler_stru*)p_ctx;

    if(OAL_PTR_NULL == pst_pm_handler)
    {
        return;
    }
    else if (OAL_TRUE == pst_pm_handler->st_inactive_timer.en_is_registerd)
    {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&pst_pm_handler->st_inactive_timer);
    }

    return;
}


OAL_STATIC oal_uint32 ap_power_state_work_event(oal_void   *p_ctx,
                                                        oal_uint16    us_event,
                                                        oal_uint16    us_event_data_len,
                                                        oal_void      *p_event_data)
{
    mac_pm_handler_stru*    pst_pm_handler = (mac_pm_handler_stru*)p_ctx;

    oal_fsm_stru        *pst_oal_fsm;
    mac_device_stru     *pst_device;
    dmac_vap_stru       *pst_dmac_vap;

    if(OAL_PTR_NULL == pst_pm_handler)
    {
        return OAL_FAIL;
    }

    pst_oal_fsm  = &pst_pm_handler->st_oal_fsm;
    pst_dmac_vap = GET_PM_FSM_OWNER(pst_pm_handler);
    pst_device   =  mac_res_get_dev(pst_dmac_vap->st_vap_base_info.uc_device_id);
    if(OAL_PTR_NULL == pst_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_PWR, "ap_power_state_work_event: pst_device NULL.");
        return OAL_ERR_CODE_PTR_NULL;
    }

    switch(us_event)
    {
        case AP_PWR_EVENT_INACTIVE:
        case AP_PWR_EVENT_NO_USR:
            if(pst_pm_handler->bit_pwr_wow_en)
            {
                if ((MAC_VAP_STATE_UP != pst_dmac_vap->st_vap_base_info.en_vap_state)
                    || (MAC_SCAN_STATE_RUNNING == pst_device->en_curr_scan_state)
            #ifdef _PRE_WLAN_FEATURE_DBAC
                    || (mac_is_dbac_enabled(pst_device))
            #endif
                    )
                {
                    OAM_INFO_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR,
                            "{ap_power_state_work_event:dbac is enable or vap not up[%d]!}", pst_dmac_vap->st_vap_base_info.en_vap_state);
                    pst_pm_handler->ul_user_check_count = 0;
                    return OAL_SUCC;
                }

                dmac_pm_state_trans(pst_pm_handler, PWR_SAVE_STATE_WOW);
            }
            else
            {
                dmac_pm_state_trans(pst_pm_handler, PWR_SAVE_STATE_IDLE);
            }
        break;
        case AP_PWR_EVENT_USR_ASSOC:
        case AP_PWR_EVENT_STA_SCAN_CONNECT:
            pst_pm_handler->ul_user_check_count = 0;
        break;
        case AP_PWR_EVENT_WIFI_DISABLE:
            dmac_pm_state_trans(pst_pm_handler, PWR_SAVE_STATE_DEEP_SLEEP);
            break;
        default:

        /* OAM��־�в���ʹ��%s*/
        OAM_WARNING_LOG2(0, OAM_SF_PWR, "{fsm in state %d do not process event %d }",
                     pst_oal_fsm->uc_cur_state, us_event);
        break;

    }
    return OAL_SUCC;
}



OAL_STATIC oal_void ap_power_state_deep_sleep_entry(oal_void *p_ctx)
{
    return;
}


OAL_STATIC oal_void ap_power_state_deep_sleep_exit(oal_void *p_ctx)
{
    return;
}


OAL_STATIC oal_uint32 ap_power_state_deep_sleep_event(oal_void      *p_ctx,
                                                        oal_uint16    us_event,
                                                        oal_uint16    us_event_data_len,
                                                        oal_void      *p_event_data)
{
    mac_pm_handler_stru*    pst_pm_handler = (mac_pm_handler_stru*)p_ctx;

    if(OAL_PTR_NULL == pst_pm_handler)
    {
        return OAL_FAIL;
    }

    switch(us_event)
    {
        case AP_PWR_EVENT_WIFI_ENABLE:
            dmac_pm_state_trans(pst_pm_handler, PWR_SAVE_STATE_WORK);
        break;
        default:
        OAM_WARNING_LOG2(0, OAM_SF_PWR, "{fsm in state %d do not process event %d }",
                    GET_PM_STATE(pst_pm_handler), us_event);
        break;
    }
    return OAL_SUCC;
}


OAL_STATIC oal_void ap_power_state_wow_entry(oal_void *p_ctx)
{
    return;
}


OAL_STATIC oal_void ap_power_state_wow_exit(oal_void *p_ctx)
{
    return;
}


OAL_STATIC oal_uint32 ap_power_state_wow_event(oal_void      *p_ctx,
                                                        oal_uint16    us_event,
                                                        oal_uint16    us_event_data_len,
                                                        oal_void      *p_event_data)
{
    mac_pm_handler_stru*    pst_pm_handler = (mac_pm_handler_stru*)p_ctx;

    if(OAL_PTR_NULL == pst_pm_handler)
    {
        OAM_INFO_LOG0(0, OAM_SF_PWR, "ap_power_state_wow_event: pst_pm_handler NULL.");
        return OAL_FAIL;
    }

    switch(us_event)
    {
        /*
            ����һ:�����Ƿ���������¼�ʹvap��wow����
            AP_PWR_EVENT_USR_ASSOC:�û�����ʱ����vap��wow�л���work�����¼�����Ӳ��û�н��뵽wow�������ĳһ��VAP����wow�ĳ�����
                                   �Ż���:��ǰ�ǽ������е�vap����Ҫ�Ż�ֻ����һ��VAP
            AP_PWR_EVENT_WOW_WAKE:CPU�жϽ��ѣ��������е�VAP
            ��Ҫ�����¼�:CPUû�н���˯�ߣ�wifi˯�ߣ������ϲ��������·�ʱ������WIFI���¼�
        */

        /* �����: PCIE��λRECONFIG�ľ������� */

        case AP_PWR_EVENT_USR_ASSOC:
        case AP_PWR_EVENT_WOW_WAKE:
        case AP_PWR_EVENT_VAP_DOWN:
        case AP_PWR_EVENT_STA_SCAN_CONNECT:
            if(pst_pm_handler->bit_pwr_wow_en)
            {
                dmac_pm_state_trans(pst_pm_handler, PWR_SAVE_STATE_WORK);
            }
            break;

        default:

        /* OAM��־�в���ʹ��%s*/
        OAM_WARNING_LOG2(0, OAM_SF_PWR, "{fsm in state %d do not process event %d }",
                    GET_PM_STATE(pst_pm_handler), us_event);
        break;
    }
    return OAL_SUCC;
}


OAL_STATIC oal_void ap_power_state_idle_entry(oal_void *p_ctx)
{
    /*IDLE״̬��:
    VAP�������beacon���ڵ�1s,���͹��ʵ���������ʽ�12db��
    device������hal��ر�1·���շ���ͨ·(��mac_pm_set_hal_state)������PCIE L1-S�͹��ģ�mem-prechargre��soc�����Զ��ſ�*/

    /* ����beacon period, ԭ��ÿVAP���裬��ΪԼ��device��VAP��beaconֵһ��������mac_pm_set_hal_state��ʵ��*/

    /*���书�ʵ�����tbtt�жϴ�����*/

    /*device�͹���������mac_pm_set_hal_state�����*/

    return;
}


OAL_STATIC oal_void ap_power_state_idle_exit(oal_void *p_ctx)
{

    return;

}


OAL_STATIC oal_uint32 ap_power_state_idle_event(oal_void      *p_ctx,
                                                        oal_uint16    us_event,
                                                        oal_uint16    us_event_data_len,
                                                        oal_void      *p_event_data)
{
    mac_pm_handler_stru*    pst_pm_handler = (mac_pm_handler_stru*)p_ctx;

    if(OAL_PTR_NULL == pst_pm_handler)
    {
        return OAL_FAIL;
    }

    switch(us_event)
    {
        case AP_PWR_EVENT_USR_ASSOC:
            dmac_pm_state_trans(pst_pm_handler, PWR_SAVE_STATE_WORK);
        break;
        case AP_PWR_EVENT_WIFI_DISABLE:
            dmac_pm_state_trans(pst_pm_handler, PWR_SAVE_STATE_DEEP_SLEEP);
            break;
        default:
            /* OAM��־�в���ʹ��%s*/
        OAM_WARNING_LOG2(0, OAM_SF_PWR, "{fsm in state %d do not process event %d }",
                    GET_PM_STATE(pst_pm_handler), us_event);
        break;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_void ap_power_state_off_entry(oal_void *p_ctx)
{
    return;
}


OAL_STATIC oal_void ap_power_state_off_exit(oal_void *p_ctx)
{
    return;
}


OAL_STATIC oal_uint32 ap_power_state_off_event(oal_void      *p_ctx,
                                                        oal_uint16    us_event,
                                                        oal_uint16    us_event_data_len,
                                                        oal_void      *p_event_data)
{
    return OAL_SUCC;
}


oal_uint32  ap_pm_wow_host_wake_event(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru         *pst_event;
    mac_pm_handler_stru    *pst_pm_handler;
    mac_device_stru        *pst_mac_device;
    dmac_vap_stru          *pst_dmac_vap;
    oal_uint8               uc_vap_idx;

    pst_event               = frw_get_event_stru(pst_event_mem);

    pst_mac_device = mac_res_get_dev(pst_event->st_event_hdr.uc_device_id);
    if(OAL_PTR_NULL == pst_mac_device)
    {
        return OAL_FAIL;
    }

    /* ѭ��divice�µ�����vap��������VAP��ΪWORK״̬ */
    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {

        pst_dmac_vap =  mac_res_get_dmac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_dmac_vap)
        {
            OAM_WARNING_LOG0(pst_mac_device->auc_vap_id[uc_vap_idx], OAM_SF_CFG, "{ap_pm_wow_host_wake_event::pst_dmac_vap null.");
            return OAL_ERR_CODE_PTR_NULL;
        }

        /* ap/sta��wow������������ */
        if (WLAN_VAP_MODE_BSS_AP != pst_dmac_vap->st_vap_base_info.en_vap_mode)
        {
            continue;
        }

        pst_pm_handler = &pst_dmac_vap->st_pm_handler;
        if (OAL_FALSE == pst_pm_handler->en_is_fsm_attached)
        {
            return OAL_FAIL;
        }
        oal_fsm_event_dispatch(&pst_pm_handler->st_oal_fsm,
                                AP_PWR_EVENT_WOW_WAKE,
                                0, OAL_PTR_NULL);
    }

    return OAL_SUCC;
}


oal_uint32 dmac_pm_post_event(dmac_vap_stru* pst_dmac_vap, oal_uint16 us_type, oal_uint16 us_datalen, oal_uint8* pst_data)
{
    mac_pm_handler_stru  *pst_handler = OAL_PTR_NULL;
    oal_uint32            ul_ret;
    mac_device_stru      *pst_device;
    oal_uint8             uc_vap_idx;
    dmac_vap_stru        *pst_vap;

    if(pst_dmac_vap == OAL_PTR_NULL)
    {
        OAM_INFO_LOG0(0, OAM_SF_PWR, "dmac_pm_post_event:: pst_dmac_vap NULL!");
        return OAL_FAIL;
    }
    pst_device = mac_res_get_dev(pst_dmac_vap->st_vap_base_info.uc_device_id);
    if(OAL_PTR_NULL == pst_device)
    {
        OAM_INFO_LOG0(0, OAM_SF_PWR, "dmac_pm_post_event:: pst_device NULL!");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (OAL_FALSE == pst_device->en_pm_enable)
    {
        OAM_INFO_LOG0(0, OAM_SF_PWR, "{dmac_pm_post_event:: wow not enable.}");
        return OAL_SUCC;
    }
    /* ���ap+staģʽ,����device,�ҵ�ap mode��vap */
    if (WLAN_VAP_MODE_BSS_STA == pst_dmac_vap->st_vap_base_info.en_vap_mode)
    {
        /* ֻ��staut,����true */
        if (pst_device->uc_sta_num >= pst_device->uc_vap_num)
        {
            OAM_WARNING_LOG0(0, OAM_SF_PWR, "dmac_pm_post_event:: only staut in device!");
            return OAL_TRUE;
        }

        for (uc_vap_idx = 0; uc_vap_idx < pst_device->uc_vap_num; uc_vap_idx++)
        {
            pst_vap = mac_res_get_dmac_vap(pst_device->auc_vap_id[uc_vap_idx]);

            if ((OAL_PTR_NULL != pst_vap) && (WLAN_VAP_MODE_BSS_AP == pst_vap->st_vap_base_info.en_vap_mode))
            {
                pst_handler = &pst_vap->st_pm_handler;
                break;
            }
        }
    }
    else
    {
        pst_handler = &pst_dmac_vap->st_pm_handler;
    }

    if (OAL_PTR_NULL == pst_handler || OAL_FALSE == pst_handler->en_is_fsm_attached)
    {
        return OAL_FAIL;
    }

    ul_ret = oal_fsm_event_dispatch(&pst_handler->st_oal_fsm, us_type, us_datalen, pst_data);

    return ul_ret;
}



OAL_STATIC oal_void dmac_pm_state_trans(mac_pm_handler_stru* pst_handler,oal_uint8 uc_state)
{
    oal_fsm_stru    *pst_oal_fsm = &pst_handler->st_oal_fsm;
    mac_device_stru *pst_mac_dev;
    dmac_vap_stru   *pst_dmac_vap;

    pst_dmac_vap = (dmac_vap_stru*)(pst_oal_fsm->p_oshandler);
    pst_mac_dev  =  mac_res_get_dev(pst_dmac_vap->st_vap_base_info.uc_device_id);
    if(OAL_PTR_NULL == pst_mac_dev)
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR, "dmac_pm_state_trans:: pst_mac_dev null");
        return;
    }

    if(uc_state>=PWR_SAVE_STATE_BUTT)
    {
        /* OAM��־�в���ʹ��%s*/
        OAM_ERROR_LOG1(pst_mac_dev->uc_cfg_vap_id, OAM_SF_PWR, "hmac_pm_state_trans:invalid state %d",uc_state);
        return;
    }

    oal_fsm_trans_to_state(pst_oal_fsm, uc_state);

    /*��fsm��״̬�Ѿ��л��ˣ���arbiterͶƱ*/
    mac_pm_arbiter_to_state(pst_mac_dev, &(pst_dmac_vap->st_vap_base_info), pst_handler->ul_pwr_arbiter_id,
                            DMAC_VAP2DEV_PM_STATE(pst_oal_fsm->uc_prev_state),
                            DMAC_VAP2DEV_PM_STATE(pst_oal_fsm->uc_cur_state));

    return;

}



OAL_STATIC oal_uint32 dmac_pm_ap_inactive_timer(oal_void* pst_arg)
{
    mac_pm_handler_stru *pst_pm_handler = (mac_pm_handler_stru*)pst_arg;
    dmac_vap_stru       *pst_dmac_vap;
    mac_device_stru     *pst_device;

    pst_dmac_vap = GET_PM_FSM_OWNER(pst_pm_handler);
    pst_device = mac_res_get_dev(pst_dmac_vap->st_vap_base_info.uc_device_id);
    if(OAL_PTR_NULL == pst_device)
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR, "dmac_pm_ap_inactive_timer:: pst_device null");
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAM_INFO_LOG3(0, OAM_SF_PWR, "{dmac_pm_ap_inactive_timer:: us_user_nums[%d], vap_num[%d], check count[%d].}",
            pst_dmac_vap->st_vap_base_info.us_user_nums, pst_device->uc_vap_num, pst_pm_handler->ul_user_check_count);

    if ((0 == pst_dmac_vap->st_vap_base_info.us_user_nums) && (OAL_TRUE == dmac_pm_ap_sta_wow_available(pst_device)))
    {
        if (pst_pm_handler->ul_user_check_count >= AP_PWR_MAX_USR_CHECK_COUNT)
        {
            oal_fsm_event_dispatch(&pst_pm_handler->st_oal_fsm,
                                    AP_PWR_EVENT_NO_USR,
                                    0, OAL_PTR_NULL);
        }
        else
        {
            pst_pm_handler->ul_user_check_count++;
        }
    }
    else
    {
        /*TBD:VAP������⣬�Ƿ��Ծ*/
    }

    return OAL_SUCC;
}



oal_void dmac_pm_ap_attach(dmac_vap_stru *pst_dmac_vap)
{
    mac_pm_handler_stru *pst_handler = OAL_PTR_NULL;
    mac_device_stru     *pst_mac_device;
    oal_uint8            auc_fsm_name[6] = {0};
    oal_uint32           ul_ret;

    if (OAL_TRUE == pst_dmac_vap->st_pm_handler.en_is_fsm_attached)
    {
        OAM_WARNING_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CFG, "dmac_pm_ap_attach::vap id[%d],fsm already attached.",
                         pst_dmac_vap->st_vap_base_info.uc_vap_id);
        return;
    }

    pst_mac_device = mac_res_get_dev(pst_dmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR, "{dmac_ap_pm_attach: vap id[%d], mac device is null.",
                       pst_dmac_vap->st_vap_base_info.uc_vap_id);
        return;
    }

    /* ���ж��Ƶ�����wowǰ. ����dbacʹ��ʱ,û��״̬��,���ܽ���˯, */
    pst_handler = &pst_dmac_vap->st_pm_handler;
    pst_handler->bit_pwr_sleep_en     = 0;
    pst_handler->bit_pwr_siso_en      = OAL_TRUE;
#if (_PRE_CONFIG_TARGET_PRODUCT == _PRE_TARGET_PRODUCT_TYPE_E5)
    pst_handler->bit_pwr_wow_en       = OAL_TRUE;
#else
    pst_handler->bit_pwr_wow_en       = OAL_FALSE;
#endif
    pst_handler->ul_inactive_time     = 0;
    pst_handler->ul_user_check_count  = 0;
    pst_handler->ul_max_inactive_time = AP_PWR_DEFAULT_INACTIVE_TIME;
    pst_handler->ul_idle_beacon_txpower   = 0xf2;

    /* ׼��һ��Ψһ��fsmname */
    auc_fsm_name[0] = (oal_uint8)pst_dmac_vap->st_vap_base_info.ul_core_id;
    auc_fsm_name[1] = pst_dmac_vap->st_vap_base_info.uc_chip_id;
    auc_fsm_name[2] = pst_dmac_vap->st_vap_base_info.uc_device_id;
    auc_fsm_name[3] = pst_dmac_vap->st_vap_base_info.uc_vap_id;
    auc_fsm_name[4] = pst_dmac_vap->st_vap_base_info.en_vap_mode;
    auc_fsm_name[5] = 0;

    ul_ret = oal_fsm_create(pst_dmac_vap,
                            auc_fsm_name,
                            pst_handler,
                            &pst_handler->st_oal_fsm,
                            PWR_SAVE_STATE_WORK,
                            g_ap_power_fsm_info,
                            OAL_SIZEOF(g_ap_power_fsm_info)/OAL_SIZEOF(oal_fsm_state_info));
    if (OAL_SUCC == ul_ret)
    {
        pst_handler->en_is_fsm_attached = OAL_TRUE;
    }

    pst_handler->ul_pwr_arbiter_id = mac_pm_arbiter_alloc_id(pst_mac_device, pst_handler->st_oal_fsm.uc_name, MAC_PWR_ARBITER_TYPE_AP);
}


oal_void dmac_pm_ap_deattach(dmac_vap_stru* pst_dmac_vap)
{
    mac_pm_handler_stru *pst_handler = OAL_PTR_NULL;
    mac_device_stru     *pst_mac_device;

    pst_handler = &pst_dmac_vap->st_pm_handler;
    if(OAL_FALSE == pst_handler->en_is_fsm_attached)
    {
        return;
    }

    /*�����л���work״̬*/
    dmac_pm_state_trans(pst_handler, PWR_SAVE_STATE_WORK);

    if (OAL_TRUE == pst_handler->st_inactive_timer.en_is_registerd)
    {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&pst_handler->st_inactive_timer);
    }

    pst_mac_device = mac_res_get_dev(pst_dmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR, "{dmac_pm_ap_deattach: mac device is null.");
        return;
    }
    mac_pm_arbiter_free_id(pst_mac_device, pst_handler->ul_pwr_arbiter_id);

    pst_handler->en_is_fsm_attached = OAL_FALSE;

    return;
}


OAL_STATIC oal_uint8 dmac_pm_ap_sta_wow_available(mac_device_stru *pst_device)
{
#if (_PRE_TARGET_PRODUCT_TYPE_E5 == _PRE_CONFIG_TARGET_PRODUCT)
    mac_vap_stru    *pst_vap;
    oal_uint8        uc_vap_idx;
#endif

    if (OAL_PTR_NULL == pst_device)
    {
        return OAL_FALSE;
    }

    if (0 == pst_device->uc_sta_num)
    {
        OAM_INFO_LOG0(0, OAM_SF_PWR, "{dmac_pm_ap_sta_wow_available:staut num is 0.}");
        return OAL_TRUE;
    }

    if (pst_device->uc_sta_num > 1)
    {
        OAM_INFO_LOG1(0, OAM_SF_PWR, "{dmac_pm_ap_sta_wow_available:staut num[%d] > 1.}", pst_device->uc_sta_num);
        return OAL_FALSE;
    }

#if (_PRE_TARGET_PRODUCT_TYPE_E5 == _PRE_CONFIG_TARGET_PRODUCT)
    /* ����device,����staut */
    for (uc_vap_idx = 0; uc_vap_idx < pst_device->uc_vap_num; uc_vap_idx++)
    {
        pst_vap = mac_res_get_mac_vap(pst_device->auc_vap_id[uc_vap_idx]);

        if (OAL_PTR_NULL == pst_vap)
        {
            continue;
        }
        if ((WLAN_VAP_MODE_BSS_STA == pst_vap->en_vap_mode) && (0 == pst_vap->us_user_nums))
        {
            OAM_INFO_LOG1(pst_vap->uc_vap_id, OAM_SF_PWR, "{dmac_pm_ap_sta_wow_available:: sta state[%d].}", pst_vap->en_vap_state);
            return OAL_TRUE;
        }
    }
#endif

    return OAL_FALSE;
}



oal_uint32  dmac_pm_ap_sta_scan_connect_event(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    dmac_vap_stru   *pst_dmac_vap;
    oal_uint32       ul_status = 0;

    pst_dmac_vap = mac_res_get_dmac_vap(pst_mac_vap->uc_vap_id);
    if(OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_PWR, "dmac_pm_ap_sta_scan_connect_event:: pst_dmac_vap is null");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ɨ�败���˳�wow,������ռ����,����host���߲��·�vap down */

    hal_get_wow_enable_status(pst_dmac_vap->pst_hal_device, &ul_status);
    if (ul_status)
    {
     #ifdef _PRE_E5_722_PLATFORM
        wlan_set_driver_lock(true);
     #endif
    }

    return dmac_pm_post_event(pst_dmac_vap, AP_PWR_EVENT_STA_SCAN_CONNECT, 0, OAL_PTR_NULL);
}


oal_void dmac_pcie_ps_switch(mac_vap_stru *pst_mac_vap, oal_uint8 uc_idle_state)
{
    mac_chip_stru           *pst_mac_chip;
    hal_to_dmac_device_stru *pst_hal_device;

    pst_mac_chip = mac_res_get_mac_chip(pst_mac_vap->uc_chip_id);
    pst_hal_device = DMAC_VAP_GET_HAL_DEVICE(pst_mac_vap);

    if ((OAL_PTR_NULL == pst_mac_chip) || (OAL_PTR_NULL == pst_hal_device))
    {
        return;
    }

    OAM_INFO_LOG3(0, OAM_SF_PWR, "dmac_pcie_ps_switch:: chip_id = %d, user_cnt = %d, idle state = %d.",
            pst_mac_chip->uc_chip_id, pst_mac_chip->uc_assoc_user_cnt, uc_idle_state);

    if (OAL_TRUE == uc_idle_state)
    {
        /* ���û���������l0s��l1pm */
        if (pst_mac_chip->uc_assoc_user_cnt <= 1)
        {
            if (OAL_TRUE == pst_hal_device->bit_l0s_on)
            {
                hal_set_soc_lpm(pst_hal_device, HAL_LPM_SOC_PCIE_L0, 1, WLAN_PCIE_L0_DEFAULT);
            }
            if (OAL_TRUE == pst_hal_device->bit_l1pm_on)
            {
                hal_set_soc_lpm(pst_hal_device, HAL_LPM_SOC_PCIE_L1_PM, 1, WLAN_PCIE_L1_DEFAULT);
            }
        }
    }
    else
    {
        /* ��һ���û������˳�l0s��l1pm */
        if (0 == pst_mac_chip->uc_assoc_user_cnt)
        {
            if (OAL_TRUE == pst_hal_device->bit_l0s_on)
            {
                hal_set_soc_lpm(pst_hal_device, HAL_LPM_SOC_PCIE_L0, 0, WLAN_PCIE_L0_DEFAULT);
            }
            if (OAL_TRUE == pst_hal_device->bit_l1pm_on)
            {
                hal_set_soc_lpm(pst_hal_device, HAL_LPM_SOC_PCIE_L1_PM, 0, WLAN_PCIE_L1_DEFAULT);
            }
        }
    }
}

#endif
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

