

/*****************************************************************************
  1 Include Head file
*****************************************************************************/
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include "plat_debug.h"
#include "bfgx_low_power.h"
#include "plat_pm.h"
#include "bfgx_exception_rst.h"
#include "oneimage.h"
#include "oal_kernel_file.h"
#include "oal_sdio_host_if.h"
#include "plat_pm_wlan.h"
#include "hisi_ini.h"
#include "plat_uart.h"
#include "board.h"
#include "plat_firmware.h"

/*****************************************************************************
  2 Define global variable
*****************************************************************************/
struct kobject *g_sysfs_hi110x_bfgx    = NULL;
struct kobject *g_sysfs_hisi_pmdbg     = NULL;

#ifdef PLATFORM_DEBUG_ENABLE
struct kobject *g_sysfs_hi110x_debug   = NULL;
int32 g_uart_rx_dump  = UART_DUMP_CLOSE;
#endif

#ifdef HAVE_HISI_NFC
struct kobject *g_sysfs_hisi_nfc = NULL;
#endif

int32 g_plat_loglevel = PLAT_LOG_INFO;
int32 g_bug_on_enable = BUG_ON_DISABLE;

extern FIRMWARE_GLOBALS_STRUCT  g_st_cfg_info;

#ifdef CONFIG_HISI_PMU_RTC_READCOUNT
extern unsigned long hisi_pmu_rtc_readcount(void);
#endif

/*****************************************************************************
  3 Function implement
*****************************************************************************/

STATIC ssize_t show_wifi_pmdbg(struct device *dev, struct kobj_attribute *attr, int8 *buf)
{
   struct wlan_pm_s *pst_wlan_pm = wlan_pm_get_drv();
   oal_int32         ret = 0;

   if (NULL == buf)
   {
      PS_PRINT_ERR("buf is NULL\n");
      return -FAILURE;
   }

#ifdef PLATFORM_DEBUG_ENABLE
   ret += snprintf(buf + ret , PAGE_SIZE - ret, "wifi_pm_debug usage: \n"
                       " 1:dump host info 2:dump device info\n"
                       " 3:open wifi      4:close wifi \n"
                       " 5:enable pm      6:disable pm \n");
#else
  ret += snprintf(buf + ret , PAGE_SIZE - ret, "wifi_pm_debug usage: \n"
                       " 1:dump host info 2:dump device info\n");
#endif

   ret += wlan_pm_host_info_print(pst_wlan_pm, buf + ret, PAGE_SIZE - ret);

   return ret;
}

STATIC ssize_t store_wifi_pmdbg(struct device *dev, struct kobj_attribute *attr, const char *buf, size_t count)
{
    struct wlan_pm_s *pst_wlan_pm = wlan_pm_get_drv();
    int input = 0;

    if (NULL == buf)
    {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    input = oal_atoi(buf);

    if (NULL == pst_wlan_pm)
    {
        OAL_IO_PRINT("pm_data is NULL!\n");
        return -FAILURE;
    }

    switch(input)
    {
       case 1:
        wlan_pm_dump_host_info();
       break;
       case 2:
        wlan_pm_dump_device_info();
       break;
#ifdef PLATFORM_DEBUG_ENABLE
       case 3:
        wlan_pm_open();
       break;
       case 4:
        wlan_pm_close();
       break;
       case 5:
        wlan_pm_enable();
       break;
       case 6:
        wlan_pm_disable();
       break;
#endif
       default:
       break;
    }

    return count;
}

STATIC ssize_t show_bfgx_pmdbg(struct device *dev, struct kobj_attribute *attr, int8 *buf)
{
    PS_PRINT_INFO("%s\n", __func__);

    if (NULL == buf)
    {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    return snprintf(buf, SNPRINT_LIMIT_TO_KERNEL, "cmd       func     \n"
                        "  1  plat pm enable\n  2  plat pm disable\n"
                        "  3   bt  pm enable\n  4   bt  pm disable\n"
                        "  5  gnss pm enable\n  6  gnss pm disable\n"
                        "  7   nfc pm enable\n  8   nfc pm disable\n"
                        "  9  pm ctrl enable\n  10 pm ctrl disable\n"
                        "  11 pm debug switch\n ");
}

extern  void plat_pm_debug_switch(void);
STATIC ssize_t store_bfgx_pmdbg(struct device *dev, struct kobj_attribute *attr, const char *buf, size_t count)
{
    struct pm_drv_data *pm_data = NULL;
    int32  cmd = 0;

    if (NULL == buf)
    {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    cmd = simple_strtol(buf, NULL, 10);
    PS_PRINT_INFO("cmd:%d\n", cmd);

    pm_data = pm_get_drvdata();
    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -FAILURE;
    }

    switch (cmd)
    {
        case 1: /* disable plat lowpower function */
            pm_data->bfgx_lowpower_enable = BFGX_PM_ENABLE;
            break;
        case 2: /* enable plat lowpower function */
            pm_data->bfgx_lowpower_enable = BFGX_PM_DISABLE;
            break;
        case 3: /* enable bt lowpower function */
            pm_data->bfgx_bt_lowpower_enable = BFGX_PM_ENABLE;
            break;
        case 4: /* disable bt lowpower function */
            pm_data->bfgx_bt_lowpower_enable = BFGX_PM_DISABLE;
            break;
        case 5: /* enable gnss lowpower function */
            pm_data->bfgx_gnss_lowpower_enable = BFGX_PM_ENABLE;
            break;
        case 6: /* disable gnss lowpower function */
            pm_data->bfgx_gnss_lowpower_enable = BFGX_PM_DISABLE;
            break;
        case 7: /* enable nfc lowpower function */
            pm_data->bfgx_nfc_lowpower_enable = BFGX_PM_ENABLE;
            break;
        case 8: /* disable nfc lowpower function */
            pm_data->bfgx_nfc_lowpower_enable = BFGX_PM_DISABLE;
            break;
        case 9:
            pm_data->bfgx_pm_ctrl_enable = BFGX_PM_ENABLE;
            break;
        case 10:
            pm_data->bfgx_pm_ctrl_enable = BFGX_PM_DISABLE;
            break;
        case 11:
            plat_pm_debug_switch();
            break;

        default:
            PS_PRINT_ERR("unknown cmd %d\n", cmd);
            break;
    }

    return count;
}

STATIC struct kobj_attribute wifi_pmdbg =
__ATTR(wifi_pm, 0644, (void *)show_wifi_pmdbg, (void *)store_wifi_pmdbg);

STATIC struct kobj_attribute bfgx_pmdbg =
__ATTR(bfgx_pm, 0644, (void *)show_bfgx_pmdbg, (void *)store_bfgx_pmdbg);

STATIC struct attribute *pmdbg_attrs[] = {
        &wifi_pmdbg.attr,
        &bfgx_pmdbg.attr,
        NULL,
};

STATIC struct attribute_group pmdbg_attr_grp = {
    .attrs = pmdbg_attrs,
};

STATIC ssize_t show_download_mode(struct device *dev, struct kobj_attribute *attr, int8 *buf)
{
    BOARD_INFO* board_info = NULL;
    if (NULL == buf)
    {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }
    board_info = get_hi110x_board_info();
    if(NULL == board_info)
    {
        PS_PRINT_ERR("board_info is null");
        return -FAILURE;
    }
    if (((board_info->wlan_download_channel) < MODE_DOWNLOAD_BUTT)
        && (board_info->bfgn_download_channel < MODE_DOWNLOAD_BUTT))
    {
        return snprintf(buf, SNPRINT_LIMIT_TO_KERNEL, "wlan:%s,bfgn:%s\n",
                device_download_mode_list[board_info->wlan_download_channel].name, device_download_mode_list[board_info->bfgn_download_channel].name);
    }
    else
    {
        PS_PRINT_ERR("download_firmware_mode:%d error", board_info->wlan_download_channel);
        return -FAILURE;
    }
}

/* called by octty from hal to decide open or close uart */
STATIC ssize_t show_install(struct device *dev, struct kobj_attribute *attr, int8 *buf)
{
    struct ps_plat_s *pm_data = NULL;

    PS_PRINT_FUNCTION_NAME;

    if (NULL == buf)
    {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    pm_data = dev_get_drvdata(&hw_ps_device->dev);
    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -FAILURE;
    }

    return snprintf(buf, SNPRINT_LIMIT_TO_KERNEL, "%d\n", pm_data->ldisc_install);
}

/* read by octty from hal to decide open which uart */
STATIC ssize_t show_dev_name(struct device *dev, struct kobj_attribute *attr, int8 *buf)
{
    struct ps_plat_s *pm_data = NULL;

    PS_PRINT_FUNCTION_NAME;

    if (NULL == buf)
    {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    pm_data = dev_get_drvdata(&hw_ps_device->dev);
    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -FAILURE;
    }

    return snprintf(buf, SNPRINT_LIMIT_TO_KERNEL, "%s\n", pm_data->dev_name);
}

/* read by octty from hal to decide what baud rate to use */
STATIC ssize_t show_baud_rate(struct device *dev, struct kobj_attribute *attr, int8 *buf)
{
    struct ps_plat_s *pm_data = NULL;

    PS_PRINT_FUNCTION_NAME;

    if (NULL == buf)
    {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    pm_data = dev_get_drvdata(&hw_ps_device->dev);
    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -FAILURE;
    }

    return snprintf(buf, SNPRINT_LIMIT_TO_KERNEL, "%ld\n", pm_data->baud_rate);
}

/* read by octty from hal to decide whether or not use flow cntrl */
STATIC ssize_t show_flow_cntrl(struct device *dev, struct kobj_attribute *attr, int8 *buf)
{
    struct ps_plat_s *pm_data = NULL;

    PS_PRINT_FUNCTION_NAME;

    if (NULL == buf)
    {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    pm_data = dev_get_drvdata(&hw_ps_device->dev);
    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -FAILURE;
    }

    return snprintf(buf, SNPRINT_LIMIT_TO_KERNEL, "%d\n", pm_data->flow_cntrl);
}

/* show curr bfgx proto yes or not opened state */
STATIC ssize_t show_bfgx_active_state(struct device *dev, struct kobj_attribute *attr, int8 *buf)
{
    struct ps_plat_s *pm_data = NULL;
    uint8 bt_state   = POWER_STATE_SHUTDOWN;
    uint8 fm_state   = POWER_STATE_SHUTDOWN;
    uint8 gnss_state = POWER_STATE_SHUTDOWN;
#ifdef HAVE_HISI_IR
    uint8 ir_state   = POWER_STATE_SHUTDOWN;
#endif
#ifdef HAVE_HISI_NFC
    uint8 nfc_state  = POWER_STATE_SHUTDOWN;
#endif

    PS_PRINT_DBG("%s\n", __func__);

    if (NULL == buf)
    {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    pm_data = dev_get_drvdata(&hw_ps_device->dev);
    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -EFAULT;
    }

    if (true == atomic_read(&pm_data->core_data->bfgx_info[BFGX_BT].subsys_state))
    {
        bt_state = POWER_STATE_OPEN;
    }

    if (true == atomic_read(&pm_data->core_data->bfgx_info[BFGX_FM].subsys_state))
    {
        fm_state = POWER_STATE_OPEN;
    }

    if (true == atomic_read(&pm_data->core_data->bfgx_info[BFGX_GNSS].subsys_state))
    {
        gnss_state = POWER_STATE_OPEN;
    }

#ifdef HAVE_HISI_IR
    if (true == atomic_read(&pm_data->core_data->bfgx_info[BFGX_IR].subsys_state))
    {
        ir_state = POWER_STATE_OPEN;
    }
#endif

#ifdef HAVE_HISI_NFC
    if (true == atomic_read(&pm_data->core_data->bfgx_info[BFGX_NFC].subsys_state))
    {
        nfc_state = POWER_STATE_OPEN;
    }
#endif

#if ((defined HAVE_HISI_IR) && (defined HAVE_HISI_NFC))
    return snprintf(buf, SNPRINT_LIMIT_TO_KERNEL, "bt:%d; fm:%d; gnss:%d; ir:%d; nfc:%d;\n", bt_state, fm_state, gnss_state, ir_state, nfc_state);
#elif (!(defined HAVE_HISI_IR) && !(defined HAVE_HISI_NFC))
    return snprintf(buf, SNPRINT_LIMIT_TO_KERNEL, "bt:%d; fm:%d; gnss:%d;\n", bt_state, fm_state, gnss_state);
#elif ((defined HAVE_HISI_IR))
    return snprintf(buf, SNPRINT_LIMIT_TO_KERNEL, "bt:%d; fm:%d; gnss:%d; ir:%d;\n", bt_state, fm_state, gnss_state, ir_state);
#else
    return snprintf(buf, SNPRINT_LIMIT_TO_KERNEL, "bt:%d; fm:%d; gnss:%d; nfc:%d;\n", bt_state, fm_state, gnss_state, nfc_state);
#endif
}

STATIC ssize_t show_ini_file_name(struct device *dev, struct kobj_attribute *attr, char *buf)
{
    if (NULL == buf)
    {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    return snprintf(buf, INI_FILE_PATH_LEN, "%s", g_ini_file_name);
}

STATIC ssize_t show_country_code(struct device *dev, struct kobj_attribute *attr, char *buf)
{
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
    int8 *country_code = NULL;
    int ret;
    int8 ibuf[COUNTRY_CODE_LEN]={0};

    if (NULL == buf)
    {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    country_code = hwifi_get_country_code();
    if (strncmp(country_code, "99", strlen("99")) == 0)
    {
        ret = get_cust_conf_string(INI_MODU_WIFI, STR_COUNTRY_CODE, ibuf, sizeof(ibuf)-1);
        if (ret == INI_SUCC)
        {
            strncpy(buf, ibuf, COUNTRY_CODE_LEN);
            buf[COUNTRY_CODE_LEN-1] = '\0';
            return strlen(buf);
        }
        else
        {
            PS_PRINT_ERR("get dts country_code error\n");
            return 0;
        }
    }
    else
    {
        return snprintf(buf, COUNTRY_CODE_LEN, "%s", country_code);
    }
#else
    return -FAILURE;
#endif
}

STATIC ssize_t show_bfgx_exception_count(struct device *dev, struct kobj_attribute *attr, int8 *buf)
{
    PS_PRINT_DBG("exception debug: bfgx rst count is %d\n", plat_get_excp_total_cnt(SUBSYS_BFGX));
    return snprintf(buf, SNPRINT_LIMIT_TO_KERNEL, "%d\n", plat_get_excp_total_cnt(SUBSYS_BFGX));
}

STATIC ssize_t show_wifi_exception_count(struct device *dev, struct kobj_attribute *attr, int8 *buf)
{
    PS_PRINT_DBG("exception debug: wifi rst count is %d\n", plat_get_excp_total_cnt(SUBSYS_WIFI));
    return snprintf(buf, SNPRINT_LIMIT_TO_KERNEL, "%d\n",  plat_get_excp_total_cnt(SUBSYS_WIFI));
}
#ifdef HAVE_HISI_GNSS
STATIC ssize_t show_gnss_lowpower_state(struct device *dev, struct kobj_attribute *attr, char *buf, size_t count)
{
    struct pm_drv_data *pm_data = pm_get_drvdata();

    PS_PRINT_DBG("show_gnss_lowpower_state!\n");

    if (NULL == buf)
    {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -FAILURE;
    }

    return snprintf(buf, SNPRINT_LIMIT_TO_KERNEL, "%d\n", atomic_read(&pm_data->gnss_sleep_flag));
}

STATIC ssize_t gnss_lowpower_state_store(struct device *dev, struct kobj_attribute *attr, const char *buf, size_t count)
{
    int flag = 0;
    struct pm_drv_data *pm_data = NULL;

    PS_PRINT_INFO("gnss_lowpower_state_store!\n");

    if (NULL == buf)
    {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    pm_data = pm_get_drvdata();
    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -FAILURE;
    }

    flag = simple_strtol(buf, NULL, 10);
    /*gnss write the flag to request sleep*/
    if (1 == flag)
    {
        if (BFGX_PM_DISABLE == pm_data->bfgx_lowpower_enable)
        {
            PS_PRINT_WARNING("gnss low power disabled!\n");
            return -FAILURE;
        }
        if (BFGX_SLEEP == pm_data->ps_pm_interface->bfgx_dev_state_get())
        {
            PS_PRINT_WARNING("gnss proc: dev has been sleep, not allow dev slp\n");
            return -FAILURE;
        }
        /*if bt and fm are both shutdown ,we will pull down gpio directly*/
        PS_PRINT_DBG("flag = 1!\n");

        if (!timer_pending(&pm_data->bfg_timer))
        {
            PS_PRINT_SUC("gnss low power request sleep!\n");
            //host_allow_bfg_sleep(pm_data->ps_pm_interface->ps_core_data);
            queue_work(pm_data->wkup_dev_workqueue, &pm_data->send_allow_sleep_work);
        }

        /*set the flag to 1 means gnss request sleep*/
        atomic_set(&pm_data->gnss_sleep_flag, GNSS_AGREE_SLEEP);
    }
    else
    {
        PS_PRINT_ERR("invalid gnss lowpower data!\n");
        return -FAILURE;
    }

    return count;
}
#endif

STATIC ssize_t show_loglevel(struct device *dev, struct kobj_attribute *attr, int8 *buf)
{
    PS_PRINT_INFO("%s\n", __func__);

    if (NULL == buf)
    {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    return snprintf(buf, SNPRINT_LIMIT_TO_KERNEL, "curr loglevel=%d, curr bug_on=%d\nalert:0\nerr:1\nwarning:2\nfunc|succ|info:3\ndebug:4\nbug_on enable:b\nbug_on disable:B\n", g_plat_loglevel, g_bug_on_enable);
}

STATIC ssize_t store_loglevel(struct device *dev, struct kobj_attribute *attr, const char *buf, size_t count)
{
    int32 loglevel = PLAT_LOG_INFO;

    PS_PRINT_INFO("%s\n", __func__);

    if (NULL == buf)
    {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    /*bug on set*/
    if ('b' == *buf)
    {
        g_bug_on_enable = BUG_ON_ENABLE;
        PS_PRINT_INFO("BUG_ON enable sucess, g_bug_on_enable = %d\n", g_bug_on_enable);
        return count;
    }
    else if ('B' == *buf)
    {
        g_bug_on_enable = BUG_ON_DISABLE;
        PS_PRINT_INFO("BUG_ON disable sucess, g_bug_on_enable = %d\n", g_bug_on_enable);
        return count;
    }

    loglevel = simple_strtol(buf, NULL, 10);
    if(PLAT_LOG_ALERT > loglevel)
    {
        g_plat_loglevel = PLAT_LOG_ALERT;
    }
    else if(PLAT_LOG_DEBUG < loglevel)
    {
        g_plat_loglevel = PLAT_LOG_DEBUG;
    }
    else
    {
        g_plat_loglevel = loglevel;
    }

    return count;
}

#ifdef HAVE_HISI_IR
STATIC ssize_t show_ir_mode(struct device *dev, struct kobj_attribute *attr, int8 *buf)
{
    struct pm_drv_data *pm_data = pm_get_drvdata();

    PS_PRINT_INFO("%s\n", __func__);

    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -FAILURE;
    }

    if (!g_board_info.have_ir) {
        PS_PRINT_ERR("board have no ir module");
        return -FAILURE;
    }
    if (NULL == buf)
    {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    if (!isAsic())
    {
        PS_PRINT_ERR("HI1102 FPGA VERSION, ir contral gpio not exist\n");
        return -FAILURE;
    }

    if (IR_GPIO_CTRL == g_board_info.irled_power_type)
    {
        return snprintf(buf, SNPRINT_LIMIT_TO_KERNEL, "%d\n", gpio_get_value(pm_data->board->bfgx_ir_ctrl_gpio));
    }
    else if (IR_LDO_CTRL == g_board_info.irled_power_type)
    {
        return snprintf(buf, SNPRINT_LIMIT_TO_KERNEL, "%d\n",(regulator_is_enabled(pm_data->board->bfgn_ir_ctrl_ldo)) > 0 ? 1:0);
    }

    return -FAILURE;
}

STATIC ssize_t store_ir_mode(struct device *dev, struct kobj_attribute *attr, const char *buf, size_t count)
{
    int32 ir_ctrl_level;
    int ret;

    struct pm_drv_data *pm_data = pm_get_drvdata();

    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -FAILURE;
    }

    PS_PRINT_INFO("into %s,irled_power_type is %d\n", __func__, pm_data->board->irled_power_type);

    if (NULL == buf)
    {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    if (!g_board_info.have_ir)
    {
        PS_PRINT_ERR("board have no ir module");
        return -FAILURE;
    }

    if (IR_GPIO_CTRL == pm_data->board->irled_power_type)
    {
        if (!isAsic())
        {
            PS_PRINT_ERR("HI1102 FPGA VERSION, ignore ir contral gpio\n");
            return count;
        }

        ir_ctrl_level = simple_strtol(buf, NULL, 10);
        if (GPIO_LOWLEVEL == ir_ctrl_level)
        {
            gpio_direction_output(pm_data->board->bfgx_ir_ctrl_gpio, GPIO_LOWLEVEL);
        }
        else if (GPIO_HIGHLEVEL == ir_ctrl_level)
        {
            gpio_direction_output(pm_data->board->bfgx_ir_ctrl_gpio, GPIO_HIGHLEVEL);
        }
        else
        {
            PS_PRINT_ERR("gpio level should be 0 or 1, cur value is [%d]\n", ir_ctrl_level);
            return -FAILURE;
        }
    }
    else if (IR_LDO_CTRL == pm_data->board->irled_power_type)
    {
        if (IS_ERR(pm_data->board->bfgn_ir_ctrl_ldo))
        {
            PS_PRINT_ERR("ir_ctrl get ird ldo failed\n");
            return -FAILURE;
        }

        ir_ctrl_level = simple_strtol(buf, NULL, 10);
        if (GPIO_LOWLEVEL == ir_ctrl_level)
        {
            ret = regulator_disable(pm_data->board->bfgn_ir_ctrl_ldo);
            if (ret)
            {
                PS_PRINT_ERR("ir_ctrl disable ldo failed\n");
            }
        }
        else if (GPIO_HIGHLEVEL == ir_ctrl_level)
        {
            ret = regulator_enable(pm_data->board->bfgn_ir_ctrl_ldo);
            if (ret)
            {
                PS_PRINT_ERR("ir_ctrl enable ldo failed\n");
            }
        }
        else
        {
            PS_PRINT_ERR("ir_ctrl level should be 0 or 1, cur value is [%d]\n", ir_ctrl_level);
            return -FAILURE;
        }
    }
    else
    {
        PS_PRINT_ERR("get ir_ldo_type error! ir_ldo_type is %d!\n", pm_data->board->irled_power_type);
        return -FAILURE;
    }
    PS_PRINT_INFO("set ir ctrl mode as %d!\n", (int)ir_ctrl_level);
    return count;
}
#endif

STATIC ssize_t bfgx_sleep_state_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    struct pm_drv_data *pm_data = pm_get_drvdata();

    if (NULL == buf)
    {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -FAILURE;
    }

    return snprintf(buf, SNPRINT_LIMIT_TO_KERNEL, "%d\n", pm_data->bfgx_dev_state);
}

STATIC ssize_t bfgx_wkup_host_count_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    struct pm_drv_data *pm_data = pm_get_drvdata();

    if (NULL == buf)
    {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -FAILURE;
    }

    return snprintf(buf, SNPRINT_LIMIT_TO_KERNEL, "%d\n", pm_data->bfg_wakeup_host);
}

STATIC int8 **get_bin_file_path(int32 *bin_file_num)
{
    int32 loop, index = 0, l_len;
    int8 **path;
    int8 *begin;
    if (NULL == bin_file_num)
    {
        PS_PRINT_ERR("bin file count is NULL!\n");
        return NULL;
    }
    *bin_file_num = 0;
    /* �ҵ�ȫ�ֱ����д�����ļ��ĸ��� */
    for (loop = 0; loop < g_st_cfg_info.al_count[BFGX_AND_WIFI_CFG]; loop++)
    {
        if (FILE_TYPE_CMD == g_st_cfg_info.apst_cmd[BFGX_AND_WIFI_CFG][loop].cmd_type)
        {
            (*bin_file_num)++;
        }
    }
    /* Ϊ���bin�ļ�·����ָ����������ռ� */
    path = (int8 **)OS_KMALLOC_GFP((*bin_file_num) * OAL_SIZEOF(int8 *));
    if (unlikely(NULL == path))
    {
        PS_PRINT_ERR("malloc path space fail!\n");
        return NULL;
    }
    /* ��֤û��ʹ�õ�����Ԫ��ȫ��ָ��NULL,��ֹ�����ͷ� */
    OS_MEM_SET((void *)path, 0, (*bin_file_num) * OAL_SIZEOF(int8 *));
    /* ��bin�ļ���·��ȫ��������һ��ָ�������� */
    for (loop = 0; loop < g_st_cfg_info.al_count[BFGX_AND_WIFI_CFG]; loop++)
    {
        if (FILE_TYPE_CMD == g_st_cfg_info.apst_cmd[BFGX_AND_WIFI_CFG][loop].cmd_type && index < *bin_file_num)
        {
            begin = OS_STR_CHR(g_st_cfg_info.apst_cmd[BFGX_AND_WIFI_CFG][loop].cmd_para, '/');
            if (NULL == begin)
            {
                continue ;
            }
            l_len = (int32)OAL_STRLEN(begin);
            path[index] = (int8 *)OS_KMALLOC_GFP(l_len + 1);
            if (NULL == path[index])
            {
                PS_PRINT_ERR("malloc file path mem fail! index is %d\n", index);
                for (loop = 0; loop < index; loop++)
                {
                    USERCTL_KFREE(path[loop]);
                }
                USERCTL_KFREE(path);
                return NULL;
            }
            OS_MEM_CPY(path[index], begin, l_len + 1);
            index++;
        }
    }
    return path;
}

STATIC ssize_t dev_version_show(struct device *dev, struct kobj_attribute *attr, int8 *buf)
{
    BOARD_INFO *hi110x_board_info = NULL;
    int8 *dev_version_bfgx = NULL;
    int8 *dev_version_wifi = NULL;
    int8 **pca_bin_file_path;
    int32 bin_file_num, loop, ret;
    PS_PRINT_INFO("%s\n", __func__);

    if (unlikely(NULL == buf))
    {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }
    hi110x_board_info = get_hi110x_board_info();
    if (unlikely(NULL == hi110x_board_info))
    {
        PS_PRINT_ERR("get board info fail\n");
        return -FAILURE;
    }

    /* ��1103ϵͳ��ʱ��֧��device����汾�� */
    if (hi110x_board_info->chip_nr != BOARD_VERSION_HI1103)
    {
        return snprintf(buf, SNPRINT_LIMIT_TO_KERNEL, "%s.\n%s.\n""NOTE:\n"
                        "     device software version only support on hi1103 now!!!\n",
                        hi110x_board_info->chip_type, g_param_version.param_version);
    }
    /* ���device�Ƿ�򿪹���cfg�����ļ��Ƿ������, ����ȫ�ֱ�����û������, ��ʾ�û�����һ��frimware */
    if (0 == g_st_cfg_info.al_count[BFGX_AND_WIFI_CFG])
    {
        return snprintf(buf, SNPRINT_LIMIT_TO_KERNEL, "%s.\n%s.\n""NOTE:\n"
                        "You need open bt or wifi once to download frimware to get device bin file path to parse!\n",
                        hi110x_board_info->chip_type, g_param_version.param_version);
    }
    /* ��ȫ�ֱ����л�ȡbin�ļ���·������ָ��bin�ļ�����·����ָ������ */
    pca_bin_file_path = get_bin_file_path(&bin_file_num);
    if (unlikely(NULL == pca_bin_file_path))
    {
        PS_PRINT_ERR("get bin file path from g_st_cfg_info fail\n");
        return -FAILURE;
    }
    /* �����ҵ������е�bin�ļ�����device����汾�� */
    for (loop = 0; loop < bin_file_num; loop++)
    {
        dev_version_bfgx = get_str_from_file(pca_bin_file_path[loop], DEV_SW_MARK_STR_BFGX, DEV_SW_VERSION_HEAD5BYTE);
        if (NULL != dev_version_bfgx)
        {
            break;
        }
    }
    for (loop = 0; loop < bin_file_num; loop++)
    {
        dev_version_wifi = get_str_from_file(pca_bin_file_path[loop], DEV_SW_MARK_STR_WIFI, DEV_SW_VERSION_HEAD5BYTE);
        if (NULL != dev_version_wifi)
        {
            break;
        }
    }
    ret = snprintf(buf, SNPRINT_LIMIT_TO_KERNEL,
                        "%s.\n%s.\nBFGX DEVICE VERSION:%s.\nWIFI DEVICE VERSION:%s.\n",
                        hi110x_board_info->chip_type, g_param_version.param_version,
                        dev_version_bfgx,dev_version_wifi);
    /* �ͷ�����������ڴ�ռ� */
    for (loop = 0; loop < bin_file_num; loop++)
    {
        USERCTL_KFREE(pca_bin_file_path[loop]);
    }
    USERCTL_KFREE(pca_bin_file_path);
    USERCTL_KFREE(dev_version_bfgx);
    USERCTL_KFREE(dev_version_wifi);
    return ret;
}

#ifdef CONFIG_HISI_PMU_RTC_READCOUNT
STATIC ssize_t show_hisi_gnss_ext_rtc_count(struct device *dev, struct kobj_attribute *attr, char *buf)
{
    unsigned long rtc_count;

    if ((NULL == dev)
         || (NULL == attr)
         || (NULL == buf))
    {
        PS_PRINT_ERR("paramater is NULL\n");
        return 0;
    }

    rtc_count = hisi_pmu_rtc_readcount();
    PS_PRINT_INFO("show_hisi_gnss_ext_rtc_count: %ld\n", rtc_count);
    memcpy(buf, (char*)&rtc_count, sizeof(rtc_count));
    return sizeof(rtc_count);
}
#endif

STATIC struct kobj_attribute download_mode =
__ATTR(device_download_mode, 0444, (void *)show_download_mode, NULL);

STATIC struct kobj_attribute ldisc_install =
__ATTR(install, 0444, (void *)show_install, NULL);

STATIC struct kobj_attribute uart_dev_name =
__ATTR(dev_name, 0444, (void *)show_dev_name, NULL);

STATIC struct kobj_attribute uart_baud_rate =
__ATTR(baud_rate, 0444, (void *)show_baud_rate, NULL);

STATIC struct kobj_attribute uart_flow_cntrl =
__ATTR(flow_cntrl, 0444, (void *)show_flow_cntrl, NULL);

STATIC struct kobj_attribute bfgx_active_state =
__ATTR(bfgx_state, 0444, (void *)show_bfgx_active_state, NULL);

STATIC struct kobj_attribute ini_file_name =
__ATTR(ini_file_name, 0444, (void *)show_ini_file_name, NULL);

STATIC struct kobj_attribute country_code =
__ATTR(country_code, 0444, (void *)show_country_code, NULL);

STATIC struct kobj_attribute bfgx_rst_count =
__ATTR(bfgx_rst_count, 0444, (void *)show_bfgx_exception_count, NULL);

STATIC struct kobj_attribute wifi_rst_count =
__ATTR(wifi_rst_count, 0444, (void *)show_wifi_exception_count, NULL);

#ifdef HAVE_HISI_GNSS
STATIC struct kobj_attribute gnss_lowpower_cntrl =
__ATTR(gnss_lowpower_state, 0644, (void *)show_gnss_lowpower_state, (void *)gnss_lowpower_state_store);
#endif

STATIC struct kobj_attribute bfgx_loglevel =
__ATTR(loglevel, 0664, (void *)show_loglevel, (void *)store_loglevel);

#ifdef HAVE_HISI_IR
STATIC struct kobj_attribute bfgx_ir_ctrl =
__ATTR(ir_ctrl, 0664, (void *)show_ir_mode, (void *)store_ir_mode);
#endif

#ifdef CONFIG_HISI_PMU_RTC_READCOUNT
STATIC struct kobj_attribute hisi_gnss_ext_rtc =
__ATTR(gnss_ext_rtc, 0444, (void *)show_hisi_gnss_ext_rtc_count, NULL);
#endif

STATIC struct kobj_attribute bfgx_sleep_attr =
__ATTR(bfgx_sleep_state, 0444, (void *)bfgx_sleep_state_show, NULL);

STATIC struct kobj_attribute bfgx_wkup_host_count_attr =
__ATTR(bfgx_wkup_host_count, 0444, (void *)bfgx_wkup_host_count_show, NULL);

STATIC struct kobj_attribute device_version =
__ATTR(version, 0444, (void *)dev_version_show, NULL);

STATIC struct attribute *bfgx_attrs[] = {
        &download_mode.attr,
        &ldisc_install.attr,
        &uart_dev_name.attr,
        &uart_baud_rate.attr,
        &uart_flow_cntrl.attr,
        &bfgx_active_state.attr,
        &ini_file_name.attr,
        &country_code.attr,
        &bfgx_rst_count.attr,
        &wifi_rst_count.attr,
#ifdef HAVE_HISI_GNSS
        &gnss_lowpower_cntrl.attr,
#endif
        &bfgx_loglevel.attr,
#ifdef HAVE_HISI_IR
        &bfgx_ir_ctrl.attr,
#endif
        &bfgx_sleep_attr.attr,
        &bfgx_wkup_host_count_attr.attr,
        &device_version.attr,
#ifdef CONFIG_HISI_PMU_RTC_READCOUNT
        &hisi_gnss_ext_rtc.attr,
#endif
        NULL,
};

STATIC struct attribute_group bfgx_attr_grp = {
    .attrs = bfgx_attrs,
};

#ifdef PLATFORM_DEBUG_ENABLE
STATIC ssize_t show_exception_dbg(struct device *dev, struct kobj_attribute *attr, int8 *buf)
{
    int32 index = 0;
    PS_PRINT_INFO("%s\n", __func__);

    if (NULL == buf)
    {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }


    index = snprintf(buf, PAGE_SIZE,
                        "==========cmd  func             \n"
                        "   1  close bt          \n"
                        "   2  set beat flat to 0\n"
                        "   3  enable dfr subsystem rst\n"
                        "   4  disble dfr subsystem rst\n");
    return plat_get_dfr_sinfo(buf,index);
}

STATIC ssize_t store_exception_dbg(struct device *dev, struct kobj_attribute *attr, const char *buf, size_t count)
{
    int32  cmd = 0;
    int32  ret = 0;
    struct ps_core_s *ps_core_d = NULL;
    struct st_exception_info *pst_exception_data = NULL;

    if (NULL == buf)
    {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    get_exception_info_reference(&pst_exception_data);
    if (NULL == pst_exception_data)
    {
        PS_PRINT_ERR("get exception info reference is error\n");
        return 0;
    }

    ps_get_core_reference(&ps_core_d);
    if (NULL == ps_core_d)
    {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return 0;
    }

    cmd = simple_strtol(buf, NULL, 10);
    PS_PRINT_INFO("cmd:%d\n", cmd);

    ret = prepare_to_visit_node(ps_core_d);
    if (ret < 0)
    {
        PS_PRINT_ERR("prepare work FAIL\n");
        return ret;
    }

    switch (cmd)
    {
        case 1:
            PS_PRINT_INFO("exception debug test: close BT\n");
            ps_tx_sys_cmd(ps_core_d, SYS_MSG, SYS_CFG_CLOSE_BT);
            break;
        case 2:
            PS_PRINT_INFO("exception: set debug beat flag to 0\n");
            pst_exception_data->debug_beat_flag = 0;
            break;
        case 3:
            PS_PRINT_INFO("enable dfr subsystem rst\n");
            pst_exception_data->subsystem_rst_en = DFR_SUSUBS_ENABLE;
            break;
        case 4:
            PS_PRINT_INFO("disable dfr subsystem rst\n");
            pst_exception_data->subsystem_rst_en = DFR_SUSUBS_DISABLE;
            break;
        default:
            PS_PRINT_ERR("unknown cmd %d\n", cmd);
            break;
    }

    post_to_visit_node(ps_core_d);

    return count;
}

STATIC ssize_t show_uart_rx_dump(struct device *dev, struct kobj_attribute *attr, int8 *buf)
{
    PS_PRINT_INFO("%s\n", __func__);

    if (NULL == buf)
    {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    return snprintf(buf, SNPRINT_LIMIT_TO_KERNEL, "curr uart dump status =%d\n no:0\n yes:1\n", g_uart_rx_dump);
}

STATIC ssize_t store_uart_rx_dump(struct device *dev, struct kobj_attribute *attr, const char *buf, size_t count)
{
    if (NULL == buf)
    {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    g_uart_rx_dump = simple_strtol(buf, NULL, 10);
    PS_PRINT_INFO("g_uart_rx_dump aft %d\n", g_uart_rx_dump);
    return count;
}

STATIC ssize_t show_dev_test(struct device *dev, struct kobj_attribute *attr, int8 *buf)
{
    PS_PRINT_INFO("%s\n", __func__);

    if (NULL == buf)
    {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    return snprintf(buf, SNPRINT_LIMIT_TO_KERNEL, "cmd  func\n  1  cause bfgx panic\n  2  enable exception recovery\n  3  enable wifi open bcpu\n"
                                   "  4  pull up power gpio\n  5  pull down power gpio\n  6  uart loop test\n"
                                   "  7  wifi download test\n  8  open uart\n  9  close uart\n"
                                   "  10 open bt\n  11 bfg download test\n  12 bfg wifi download test\n"
                                   "  13 wlan_pm_open_bcpu\n  14 wlan_power_on \n  15 wlan_power_off\n"
                                   "  16 wlan_pm_open\n  17 wlan_pm_close\n  18 wlan gpio power on\n  19 bfg gpio power on\n  20 gnss monitor enable\n");
}
oal_uint  wlan_pm_open_bcpu(oal_void);
extern int32 uart_loop_test(void);
extern int firmware_download_function(uint32 which_cfg);
extern  int32 hi1103_wlan_power_on(void);
extern  int32 hi1103_wlan_power_off(void);
extern oal_uint32 wlan_pm_close(oal_void);
extern int32 g_device_monitor_enable;
STATIC ssize_t store_dev_test(struct device *dev, struct kobj_attribute *attr, const char *buf, size_t count)
{
    int32 cmd;
    int32 ret;
    uint8 send_data[6] = {0x7e, 0x0, 0x06, 0x0, 0x0, 0x7e};
    struct ps_core_s *ps_core_d = NULL;
    struct st_exception_info *pst_exception_data = NULL;
    BOARD_INFO * bd_info = NULL;

    PS_PRINT_INFO("%s\n", __func__);

    bd_info = get_hi110x_board_info();
    if (unlikely(NULL == bd_info))
    {
        PS_PRINT_ERR("board info is err\n");
        return BFGX_POWER_FAILED;
    }

    if (NULL == buf)
    {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    get_exception_info_reference(&pst_exception_data);
    if (NULL == pst_exception_data)
    {
        PS_PRINT_ERR("get exception info reference is error\n");
        return 0;
    }

    ps_get_core_reference(&ps_core_d);
    if (unlikely(NULL == ps_core_d))
    {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return -EINVAL;
    }

    cmd = simple_strtol(buf, NULL, 10);
    switch (cmd)
    {
        case 1:
            ret = prepare_to_visit_node(ps_core_d);
            if (ret < 0)
            {
                PS_PRINT_ERR("prepare work FAIL\n");
                return ret;
            }

            PS_PRINT_INFO("bfgx test cmd %d, cause device panic\n", cmd);
            ps_tx_sys_cmd(ps_core_d, SYS_MSG, SYS_CFG_DEV_PANIC);

            post_to_visit_node(ps_core_d);
            break;
        case 2:
            PS_PRINT_INFO("cmd %d,enable platform dfr\n", cmd);
            pst_exception_data->exception_reset_enable = PLAT_EXCEPTION_ENABLE;
            break;
        case 3:
            PS_PRINT_INFO("cmd %d,enable wifi open bcpu\n", cmd);
            wifi_open_bcpu_set(1);
            break;
        case 4:
            PS_PRINT_INFO("cmd %d,test pull up power gpio\n", cmd);
            bd_info->bd_ops.board_power_on(WLAN_POWER);
        case 5:
            PS_PRINT_INFO("cmd %d,test pull down power gpio\n", cmd);
            bd_info->bd_ops.board_power_off(WLAN_POWER);
        case 6:
            PS_PRINT_INFO("cmd %d,start uart loop test\n", cmd);
            uart_loop_test();
            break;
        case 7:
            PS_PRINT_INFO("cmd %d,firmware download wifi_cfg\n", cmd);
            firmware_download_function(1); //wifi_cfg
            break;
        case 8:
            PS_PRINT_INFO("cmd %d,open uart\n", cmd);
            open_tty_drv(ps_core_d->pm_data);
            break;
        case 9:
            PS_PRINT_INFO("cmd %d,close uart\n", cmd);
            release_tty_drv(ps_core_d->pm_data);
            break;
        case 10:
            PS_PRINT_INFO("cmd %d,uart cmd test\n", cmd);
            ps_write_tty(ps_core_d, send_data, sizeof(send_data));
            break;
        case 11:
            PS_PRINT_INFO("cmd %d,firmware download bfgx_cfg\n", cmd);
            firmware_download_function(2);
            break;
        case 12:
            PS_PRINT_INFO("cmd %d,firmware download bfgx_and_wifi cfg\n", cmd);
            firmware_download_function(0);
            break;
        case 13:
            PS_PRINT_INFO("cmd %d,wlan_pm_open_bcpu\n", cmd);
            wlan_pm_open_bcpu();
            break;
        case 14:
            PS_PRINT_INFO("cmd %d,hi1103_wlan_power_on\n", cmd);
            hi1103_wlan_power_on();
            break;
        case 15:
            PS_PRINT_INFO("cmd %d,hi1103_wlan_power_off\n", cmd);
            hi1103_wlan_power_off();
            break;
        case 16:
            PS_PRINT_INFO("cmd %d,wlan_pm_open\n", cmd);
            wlan_pm_open();
            break;
        case 17:
            PS_PRINT_INFO("cmd %d,wlan_pm_close\n", cmd);
            wlan_pm_close();
            break;
        case 18:
            PS_PRINT_INFO("cmd %d,wlan gpio power on\n", cmd);
            bd_info->bd_ops.board_power_on(WLAN_POWER);
            break;
        case 19:
            PS_PRINT_INFO("cmd %d,bfgx gpio power on\n", cmd);
            bd_info->bd_ops.board_power_on(BFGX_POWER);
            break;
        case 20:
            PS_PRINT_INFO("cmd %d,bfgx gpio power on\n", cmd);
            g_device_monitor_enable = 1;
            break;
        default:
            PS_PRINT_ERR("unknown cmd %d\n", cmd);
            break;
    }

    return count;
}

STATIC ssize_t show_wifi_mem_dump(struct device *dev, struct kobj_attribute *attr, int8 *buf)
{
    PS_PRINT_INFO("%s\n", __func__);

    if (NULL == buf)
    {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    return snprintf(buf, SNPRINT_LIMIT_TO_KERNEL, "cmd         func             \n"
                        " 1    uart halt wcpu         \n"
                        " 2    uart read wifi pub reg \n"
                        " 3    uart read wifi priv reg\n"
                        " 4    uart read wifi mem     \n"
                        " 5    equal cmd 1+2+3+4      \n");
}

STATIC ssize_t store_wifi_mem_dump(struct device *dev, struct kobj_attribute *attr, const char *buf, size_t count)
{
    int32 cmd;
    int32 ret;
    struct ps_core_s *ps_core_d = NULL;
    struct st_exception_info *pst_exception_data = NULL;

    PS_PRINT_INFO("%s\n", __func__);

    if (NULL == buf)
    {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    get_exception_info_reference(&pst_exception_data);
    if (NULL == pst_exception_data)
    {
        PS_PRINT_ERR("get exception info reference is error\n");
        return 0;
    }

    ps_get_core_reference(&ps_core_d);
    if (unlikely(NULL == ps_core_d))
    {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return -EINVAL;
    }

    ret = prepare_to_visit_node(ps_core_d);
    if (ret < 0)
    {
        PS_PRINT_ERR("prepare work FAIL\n");
        return ret;
    }

    cmd = simple_strtol(buf, NULL, 10);
    switch (cmd)
    {
        case 1:
            PS_PRINT_INFO("wifi mem dump cmd %d, halt wcpu\n", cmd);
            uart_halt_wcpu();
            break;
        case 2:
            PS_PRINT_INFO("wifi mem dump cmd %d, read wifi public register\n", cmd);
            plat_wait_last_rotate_finish();
            if (EXCEPTION_SUCCESS == uart_read_wifi_mem(WIFI_PUB_REG))
            {
                /*send cmd to oam_hisi to rotate file*/
                plat_send_rotate_cmd_2_app(CMD_READM_WIFI_UART);
            }
            else
            {
                plat_rotate_finish_set();
            };
            break;
        case 3:
            PS_PRINT_INFO("wifi mem dump cmd %d, read wifi priv register\n", cmd);
            plat_wait_last_rotate_finish();
            if (EXCEPTION_SUCCESS == uart_read_wifi_mem(WIFI_PRIV_REG))
            {
                /*send cmd to oam_hisi to rotate file*/
                plat_send_rotate_cmd_2_app(CMD_READM_WIFI_UART);
            }
            else
            {
                plat_rotate_finish_set();
            };
            break;
        case 4:
            PS_PRINT_INFO("wifi mem dump cmd %d, read wifi mem\n", cmd);
            plat_wait_last_rotate_finish();
            if (EXCEPTION_SUCCESS == uart_read_wifi_mem(WIFI_MEM))
            {
                /*send cmd to oam_hisi to rotate file*/
                plat_send_rotate_cmd_2_app(CMD_READM_WIFI_UART);
            }
            else
            {
                plat_rotate_finish_set();
            };
            break;
        case 5:
            PS_PRINT_INFO("wifi mem dump cmd %d\n", cmd);
            debug_uart_read_wifi_mem(1);
            break;
        default:
            PS_PRINT_ERR("error cmd:[%d]\n", cmd);
            break;
    }

    post_to_visit_node(ps_core_d);

    return count;
}


STATIC ssize_t show_bfgx_dump(struct device *dev, struct kobj_attribute *attr, int8 *buf)
{
    PS_PRINT_INFO("%s\n", __func__);

    if (NULL == buf)
    {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    return snprintf(buf,SNPRINT_LIMIT_TO_KERNEL, "cmd           func            \n"
                        " 1    sdio read bcpu pub reg  \n"
                        " 2    sdio read bcpu priv reg \n"
                        " 3    sdio read bcpu mem      \n"
                        " 4    equal cmd 1+2+3         \n");
}

STATIC ssize_t store_bfgx_reg_and_reg_dump(struct device *dev, struct kobj_attribute *attr, const char *buf, size_t count)
{
    int32 cmd;
    int32 ret;
    struct ps_core_s *ps_core_d = NULL;
    struct st_exception_info *pst_exception_data = NULL;

    PS_PRINT_INFO("%s\n", __func__);

    if (NULL == buf)
    {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    get_exception_info_reference(&pst_exception_data);
    if (NULL == pst_exception_data)
    {
        PS_PRINT_ERR("get exception info reference is error\n");
        return 0;
    }

    ps_get_core_reference(&ps_core_d);
    if (unlikely(NULL == ps_core_d))
    {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return -EINVAL;
    }

    ret = prepare_to_visit_node(ps_core_d);
    if (ret < 0)
    {
        PS_PRINT_ERR("prepare work FAIL\n");
        return ret;
    }

    cmd = simple_strtol(buf, NULL, 10);
    switch (cmd)
    {
        case 1:
            PS_PRINT_INFO("bfgx mem dump cmd %d,sdio read bcpu pub reg\n", cmd);
            debug_sdio_read_bfgx_reg_and_mem(BFGX_PUB_REG);
            break;
        case 2:
            PS_PRINT_INFO("bfgx mem dump cmd %d, sdio read bcpu priv reg\n", cmd);
            debug_sdio_read_bfgx_reg_and_mem(BFGX_PRIV_REG);
            break;
        case 3:
            PS_PRINT_INFO("bfgx mem dump cmd %d, sdio read bcpu mem\n", cmd);
            debug_sdio_read_bfgx_reg_and_mem(BFGX_MEM);
            break;
        case 4:
            PS_PRINT_INFO("bfgx mem dump cmd %d, sdio read bcpu reg and mem\n", cmd);
            debug_sdio_read_bfgx_reg_and_mem(SDIO_BFGX_MEM_DUMP_BOTTOM);
            break;
        default:
            PS_PRINT_ERR("error cmd:[%d]\n", cmd);
            break;
    }

    post_to_visit_node(ps_core_d);

    return count;
}

#ifdef BFGX_UART_DOWNLOAD_SUPPORT
extern uart_download_test_st g_st_uart_download_test;
STATIC ssize_t show_bfgx_uart_download(struct device *dev, struct kobj_attribute *attr, int8 *buf)
{
    PS_PRINT_INFO("%s\n", __func__);

    if (NULL == buf)
    {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    return snprintf(buf,SNPRINT_LIMIT_TO_KERNEL, "baud:%-14s file_len:%-10d send_len:%-10d status:%-5d speed(byte/ms):%-6d usedtime(ms):%-6d \n",
                    g_st_uart_download_test.baud, g_st_uart_download_test.file_len, g_st_uart_download_test.xmodern_len,
                    g_st_uart_download_test.send_status,g_st_uart_download_test.file_len/g_st_uart_download_test.used_time,
                    g_st_uart_download_test.used_time);
    return -FAILURE;
}
extern int32 uart_download_test(uint8* baud, uint32 file_len);

STATIC ssize_t store_bfgx_uart_download(struct device *dev, struct kobj_attribute *attr, const char *buf, size_t count)
{
#define BFGX_UART_TEST_CMD_LEN (100)
    uint8 baud[BFGX_UART_TEST_CMD_LEN]={0};
    uint8 buf_data[BFGX_UART_TEST_CMD_LEN];
    int32 i = 0;
    int32 buf_len = strlen(buf);
    const char * tmp_buf = buf;
    uint32 file_len=0;
    uint32 max_index;
    PS_PRINT_INFO("%s\n", __func__);
    PS_PRINT_DBG("buf:%s,len:%d,count:%d\n",buf,buf_len,(int32)count);
    while(*tmp_buf == ' ')
    {
        tmp_buf++;
        buf_len--;
        if (!buf_len)
        {
            return count;
        }
    };
    i = 0;
    while(*tmp_buf != ' ')
    {
        if (i < BFGX_UART_TEST_CMD_LEN)
            baud[i++]=*tmp_buf;
        tmp_buf++;
        buf_len--;
        if (!buf_len)
        {
            return count;
        }
    };
    max_index = i < BFGX_UART_TEST_CMD_LEN ? i : (BFGX_UART_TEST_CMD_LEN - 1);
    baud[max_index]='\0';
    i = 0;
    PS_PRINT_DBG("@#baud:%s,tmp_buf:%s\n",baud, tmp_buf);
    while(*tmp_buf == ' ')
    {
        tmp_buf++;
        buf_len--;
        if (!buf_len)
        {
            return count;
        }
    };
    while(((*tmp_buf>='0') && (*tmp_buf<='9')) || ((*tmp_buf>='a') && (*tmp_buf<='z')))
    {
        if (!buf_len)
        {
            return count;
        }
        if (i < BFGX_UART_TEST_CMD_LEN)
            buf_data[i++]=*tmp_buf;
        tmp_buf++;
        buf_len--;
    };
    max_index = i < BFGX_UART_TEST_CMD_LEN ? i : (BFGX_UART_TEST_CMD_LEN - 1);
    buf_data[max_index]='\0';
    file_len = simple_strtol(buf_data, NULL, 0);

    PS_PRINT_INFO("baud:[%s],file_len[%d]", baud, file_len);
    uart_download_test(baud, file_len);
    return count;
}
#endif /* BFGX_UART_DOWNLOAD_SUPPORT */

#define WIFI_BOOT_TEST_DATA_BUFF_LEN  (1024)
static uint32 g_ul_wifi_boot_file_len = 0;
static uint32 g_ul_wifi_boot_total = 0;
static uint32 g_ul_wifi_power_on_succ = 0;
static uint32 g_ul_wifi_power_on_fail = 0;
static uint32 g_ul_wifi_power_off_succ = 0;
static uint32 g_ul_wifi_power_off_fail = 0;

STATIC ssize_t show_wifi_download(struct device *dev, struct kobj_attribute *attr, int8 *buf)
{
    PS_PRINT_INFO("%s\n", __func__);
    if (NULL == buf)
    {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    return snprintf(buf, SNPRINT_LIMIT_TO_KERNEL,"total:%-10d file_len:%-10d on_succ:%-10d on_fail:%-10d off_succ:%-10d off_fail:%-10d\n",
                            g_ul_wifi_boot_total, g_ul_wifi_boot_file_len, g_ul_wifi_power_on_succ,g_ul_wifi_power_on_fail,
                            g_ul_wifi_power_off_succ, g_ul_wifi_power_off_fail);;
}
void wifi_boot_download_test(uint32  ul_test_len)
{
    mm_segment_t fs = {0};
    OS_KERNEL_FILE_STRU *fp = {0};
    int8 write_data[WIFI_BOOT_TEST_DATA_BUFF_LEN];
    int32 write_total_circle=ul_test_len/WIFI_BOOT_TEST_DATA_BUFF_LEN;
    int32 write_data_extra_len = ul_test_len%WIFI_BOOT_TEST_DATA_BUFF_LEN;
    int32 write_count = 0;
    int8 filename[100] = "/system/vendor/firmware/test_wifi_boot";
    //init
    for (write_count=0;write_count < WIFI_BOOT_TEST_DATA_BUFF_LEN;write_count++)
    {
        write_data[write_count] = write_count;
    }
    //create file
    fp = filp_open(filename, O_RDWR | O_CREAT, 0664);
    if (OAL_IS_ERR_OR_NULL(fp))
    {
        PS_PRINT_ERR("create file error,fp = 0x%p\n", fp);
    }
    fs = get_fs();
    set_fs(KERNEL_DS);
    if (write_total_circle != 0)
    {
        for (write_count = 0; write_count < write_total_circle; write_count++)
        {
                vfs_write(fp, write_data, WIFI_BOOT_TEST_DATA_BUFF_LEN, &fp->f_pos);
        }
    }
    if (write_data_extra_len != 0)
    {
         vfs_write(fp, write_data, write_data_extra_len, &fp->f_pos);
    }
    set_fs(fs);
    filp_close(fp, NULL);
    PS_PRINT_INFO("#@file:%s prepare succ", filename);
}
STATIC ssize_t store_wifi_download(struct device *dev, struct kobj_attribute *attr, const char *buf, size_t count)
{
#define WIFI_DOWNLOAD_TEST_CMD_LEN   (50)
    uint8 buf_data[WIFI_DOWNLOAD_TEST_CMD_LEN];
    int32 i = 0;
    int32 buf_len = strlen(buf);
    const char * tmp_buf = buf;
    uint32 file_len=0;
    uint32 max_index;
    PS_PRINT_INFO("%s\n", __func__);
    PS_PRINT_DBG("buf:%s,len:%d,count:%d\n",buf,buf_len,(int32)count);

    //get params
    while(*tmp_buf == ' ')
    {
        tmp_buf++;
        buf_len--;
        if (!buf_len)
        {
            return count;
        }
    };
    i = 0;
    while(((*tmp_buf>='0') && (*tmp_buf<='9')) || ((*tmp_buf>='a') && (*tmp_buf<='z')))
    {
        if (!buf_len)
        {
            return count;
        }
        if (i < WIFI_DOWNLOAD_TEST_CMD_LEN)
        {
            buf_data[i++]=*tmp_buf;
        }
        tmp_buf++;
        buf_len--;
    };
    max_index = i < WIFI_DOWNLOAD_TEST_CMD_LEN ? i : (WIFI_DOWNLOAD_TEST_CMD_LEN - 1);
    buf_data[max_index]='\0';
    file_len = simple_strtol(buf_data, NULL, 0);
    PS_PRINT_INFO("#@get file len:%d prepare succ", file_len);

    //do download test and set flag
    g_ul_wifi_boot_file_len = file_len;
    wifi_boot_download_test(file_len);
    g_ul_wifi_boot_total++;
    if ( 0 == hi1103_wlan_power_on())
    {
        g_ul_wifi_power_on_succ++;
        PS_PRINT_INFO("#@power on succ\n");
    }
    else
    {
        g_ul_wifi_power_on_fail++;
        PS_PRINT_INFO("#@power on fail\n");
    };
    if ( 0 == hi1103_wlan_power_off())
    {
        g_ul_wifi_power_off_succ++;
        PS_PRINT_INFO("#@power off succ\n");
    }
    else
    {
        g_ul_wifi_power_off_fail++;
        PS_PRINT_INFO("#@power on fail\n");
    };
    return count;
}


#ifdef _PRE_CONFIG_GPIO_TO_SSI_DEBUG
extern int32 ssi_download_test(ssi_trans_test_st* pst_ssi_test);
extern ssi_trans_test_st ssi_test_st;
STATIC ssize_t show_ssi_test(struct device *dev, struct kobj_attribute *attr, int8 *buf)
{
    PS_PRINT_INFO("%s\n", __func__);

    if (NULL == buf)
    {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }
    return snprintf(buf,SNPRINT_LIMIT_TO_KERNEL, "len:%-14d time:%-14d status:%-5d speed(byte/ms):%-8d\n", ssi_test_st.trans_len, ssi_test_st.used_time, ssi_test_st.send_status, ssi_test_st.trans_len/ssi_test_st.used_time);
}
extern int32 test_hd_ssi_write(void);
STATIC ssize_t store_ssi_test(struct device *dev, struct kobj_attribute *attr, const char *buf, size_t count)
{
#define SSI_TEST_CMD_MAX_LEN (50)
    int32 cmd;
    struct ps_core_s *ps_core_d = NULL;
    char s_addr[SSI_TEST_CMD_MAX_LEN];
    int32 dsc_addr;
    char s_data[SSI_TEST_CMD_MAX_LEN];
    int32 set_data;
    const char * tmp_buf = buf;
    size_t buf_len=count;
    int32 i = 0;
    int32 max_index;
    PS_PRINT_INFO("%s\n", __func__);

    if (NULL == buf)
    {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    ps_get_core_reference(&ps_core_d);
    if (unlikely(NULL == ps_core_d))
    {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return -EINVAL;
    }

    //ret = prepare_to_visit_node(ps_core_d);
    //if (ret < 0)
    //{
    //    PS_PRINT_ERR("prepare work FAIL\n");
    //    return ret;
    //}

    cmd = simple_strtol(buf, NULL, 10);
    memset(&ssi_test_st,0,sizeof(ssi_test_st));
    switch (cmd)
    {
        case 1:
            PS_PRINT_INFO("ssi download test cmd %d\n", cmd);
            ssi_test_st.test_type = SSI_MEM_TEST;
            ssi_download_test(&ssi_test_st);
            break;
        case 2:
            PS_PRINT_INFO("ssi download test cmd %d\n", cmd);
            ssi_test_st.test_type = SSI_FILE_TEST;
            ssi_download_test(&ssi_test_st);
            break;
        case 3:
            PS_PRINT_INFO("power on enable cmd %d\n", cmd);
            hi1103_chip_power_on();
            hi1103_bfgx_enable();
            hi1103_wifi_enable();
            break;
        case 4:
            PS_PRINT_INFO("power off enable cmd %d\n", cmd);
            hi1103_bfgx_disable();
            hi1103_wifi_disable();
            hi1103_chip_power_off();
            break;
        case 5 :
            PS_PRINT_INFO("hard ware cmd %d\n", cmd);
            test_hd_ssi_write();
            break;
        default:
            PS_PRINT_DBG("default cmd %s\n", buf);
            tmp_buf++;
            while(*tmp_buf == ' ')
            {
                tmp_buf++;
                buf_len--;
                if (!buf_len)
                {
                    return count;
                }
            };
            i = 0;
            while(((*tmp_buf>='0') && (*tmp_buf<='9')) || ((*tmp_buf>='a') && (*tmp_buf<='z')))
            {
                if (i<SSI_TEST_CMD_MAX_LEN)
                {
                    s_addr[i++]=*tmp_buf;
                }
                tmp_buf++;
                buf_len--;
                if (!buf_len)
                {
                    return count;
                }
            };
            max_index = i < SSI_TEST_CMD_MAX_LEN ? i : (SSI_TEST_CMD_MAX_LEN - 1);
            s_addr[max_index]='\0';
            i = 0;
            dsc_addr = simple_strtol(s_addr, NULL, 0);
            switch (buf[0])
            {
                case 'r':
                    PS_PRINT_INFO("ssi read: 0x%x=0x%x\n", dsc_addr, ssi_single_read(dsc_addr));
                    break;
                case 'w':
                    while(*tmp_buf == ' ')
                    {
                        tmp_buf++;
                        buf_len--;
                        if (!buf_len)
                        {
                            return count;
                        }
                    };
                    while(((*tmp_buf>='0') && (*tmp_buf<='9')) || ((*tmp_buf>='a') && (*tmp_buf<='z')))
                    {
                        if (!buf_len)
                        {
                            return count;
                        }
                        if (i < SSI_TEST_CMD_MAX_LEN)
                        {
                            s_data[i++]=*tmp_buf;
                        }
                        tmp_buf++;
                        buf_len--;
                    };
                    max_index = i < SSI_TEST_CMD_MAX_LEN ? i : (SSI_TEST_CMD_MAX_LEN - 1);
                    s_data[max_index]='\0';
                    set_data = simple_strtol(s_data, NULL, 0);
                    PS_PRINT_INFO("ssi_write s_addr:0x%x,s_data:0x%x\n",dsc_addr,set_data);
                    if (0 != ssi_single_write(dsc_addr,set_data))
                    {
                        PS_PRINT_ERR("ssi write fail s_addr:0x%x s_data:0x%x\n", dsc_addr,set_data);
                    }
                    break;
               default:
                    PS_PRINT_INFO("no suit cmd:%c\n",buf[0]);
                    break;
            }
            return count;
            PS_PRINT_ERR("error cmd:[%d]\n", cmd);
            break;
    }

    //post_to_visit_node(ps_core_d);

    return count;
}
#endif
STATIC struct kobj_attribute plat_exception_dbg =
__ATTR(exception_dbg, 0644, (void *)show_exception_dbg, (void *)store_exception_dbg);

STATIC struct kobj_attribute uart_dumpctrl =
__ATTR(uart_rx_dump, 0664, (void *)show_uart_rx_dump, (void *)store_uart_rx_dump);

STATIC struct kobj_attribute bfgx_dev_test =
__ATTR(bfgx_test, 0664, (void *)show_dev_test, (void *)store_dev_test);

STATIC struct kobj_attribute wifi_mem_dump =
__ATTR(wifi_mem, 0664, (void *)show_wifi_mem_dump, (void *)store_wifi_mem_dump);

STATIC struct kobj_attribute bfgx_mem_and_reg_dump=
__ATTR(bfgx_dump, 0664, (void *)show_bfgx_dump, (void *)store_bfgx_reg_and_reg_dump);


STATIC struct kobj_attribute wifi_boot_test=
__ATTR(wifi_boot, 0664, (void *)show_wifi_download, (void *)store_wifi_download);

#ifdef BFGX_UART_DOWNLOAD_SUPPORT
STATIC struct kobj_attribute bfgx_uart_download=
__ATTR(bfgx_boot, 0664, (void *)show_bfgx_uart_download, (void *)store_bfgx_uart_download);
#endif /* BFGX_UART_DOWNLOAD_SUPPORT */

#ifdef _PRE_CONFIG_GPIO_TO_SSI_DEBUG
STATIC struct kobj_attribute ssi_test_trans=
__ATTR(ssi_test, 0664, (void *)show_ssi_test, (void *)store_ssi_test);
#endif

STATIC struct attribute *hi110x_debug_attrs[] = {
        &plat_exception_dbg.attr,
        &uart_dumpctrl.attr,
        &bfgx_dev_test.attr,
        &wifi_mem_dump.attr,
        &bfgx_mem_and_reg_dump.attr,
        &wifi_boot_test.attr,
#ifdef BFGX_UART_DOWNLOAD_SUPPORT
        &bfgx_uart_download.attr,
#endif
#ifdef _PRE_CONFIG_GPIO_TO_SSI_DEBUG
        &ssi_test_trans.attr,
#endif
        NULL,
};

STATIC struct attribute_group hi110x_debug_attr_grp = {
    .attrs = hi110x_debug_attrs,
};
#endif

#ifdef HAVE_HISI_NFC
STATIC ssize_t show_hisi_nfc_conf_name(struct device *dev, struct kobj_attribute *attr, int8 *buf)
{
    char hisi_nfc_conf_name[BUFF_LEN] = {0};
    int32 ret = 0;

    if (NULL == buf)
    {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    ret = read_nfc_conf_name_from_dts(hisi_nfc_conf_name, sizeof(hisi_nfc_conf_name),
                                       DTS_COMP_HISI_NFC_NAME, DTS_COMP_HW_HISI_NFC_CONFIG_NAME);
    if (ret < 0)
    {
        PS_PRINT_ERR("read_nfc_conf_name_from_dts %s,ret = %d\n", DTS_COMP_HW_HISI_NFC_CONFIG_NAME, ret);
        return ret;
    }

    return snprintf(buf, sizeof(hisi_nfc_conf_name), "%s", hisi_nfc_conf_name);
}

STATIC ssize_t show_brcm_nfc_conf_name(struct device *dev, struct kobj_attribute *attr, int8 *buf)
{
    char brcm_nfc_conf_name[BUFF_LEN] = {0};
    int32 ret = 0;

    if (NULL == buf)
    {
        PS_PRINT_ERR("buf is NULL\n");
        return -FAILURE;
    }

    ret = read_nfc_conf_name_from_dts(brcm_nfc_conf_name, sizeof(brcm_nfc_conf_name),
                                       DTS_COMP_HISI_NFC_NAME, DTS_COMP_HW_BRCM_NFC_CONFIG_NAME);
    if (ret < 0)
    {
        PS_PRINT_ERR("read_nfc_conf_name_from_dts %s,ret = %d\n", DTS_COMP_HW_BRCM_NFC_CONFIG_NAME, ret);
        return ret;
    }

    return snprintf(buf, sizeof(brcm_nfc_conf_name), "%s", brcm_nfc_conf_name);
}

STATIC struct kobj_attribute hisi_nfc_conf =
__ATTR(nxp_config_name, 0444, (void *)show_hisi_nfc_conf_name, NULL);

STATIC struct kobj_attribute brcm_nfc_conf =
__ATTR(nfc_brcm_conf_name, 0444, (void *)show_brcm_nfc_conf_name, NULL);

STATIC struct attribute *hisi_nfc_attrs[] = {
        &hisi_nfc_conf.attr,
        &brcm_nfc_conf.attr,
        NULL,
};

STATIC struct attribute_group hisi_nfc_attr_grp = {
    .attrs = hisi_nfc_attrs,
};
#endif

int32 bfgx_user_ctrl_init(void)
{
    int status;
    struct kobject *pst_root_object = NULL;

    pst_root_object = oal_get_sysfs_root_object();
    if(NULL == pst_root_object)
    {
        PS_PRINT_ERR("[E]get root sysfs object failed!\n");
        return -EFAULT;
    }

    g_sysfs_hisi_pmdbg = kobject_create_and_add("pmdbg", pst_root_object);
    if (NULL == g_sysfs_hisi_pmdbg)
    {
        PS_PRINT_ERR("Failed to creat g_sysfs_hisi_pmdbg !!!\n ");
		goto fail_g_sysfs_hisi_pmdbg;
    }

    status = sysfs_create_group(g_sysfs_hisi_pmdbg, &pmdbg_attr_grp);
    if (status)
    {
		PS_PRINT_ERR("failed to create g_sysfs_hisi_pmdbg sysfs entries\n");
		goto fail_create_pmdbg_group;
    }

    g_sysfs_hi110x_bfgx = kobject_create_and_add("hi110x_ps", NULL);
    if (NULL == g_sysfs_hi110x_bfgx)
    {
        PS_PRINT_ERR("Failed to creat g_sysfs_hi110x_ps !!!\n ");
		goto fail_g_sysfs_hi110x_bfgx;
    }

    status = sysfs_create_group(g_sysfs_hi110x_bfgx, &bfgx_attr_grp);
    if (status)
    {
		PS_PRINT_ERR("failed to create g_sysfs_hi110x_bfgx sysfs entries\n");
		goto fail_create_bfgx_group;
    }

#ifdef PLATFORM_DEBUG_ENABLE
    g_sysfs_hi110x_debug = kobject_create_and_add("hi110x_debug", NULL);
    if (NULL == g_sysfs_hi110x_debug)
    {
        PS_PRINT_ERR("Failed to creat g_sysfs_hi110x_debug !!!\n ");
        goto fail_g_sysfs_hi110x_debug;
    }

    status = sysfs_create_group(g_sysfs_hi110x_debug, &hi110x_debug_attr_grp);
    if (status)
    {
        PS_PRINT_ERR("failed to create g_sysfs_hi110x_debug sysfs entries\n");
        goto fail_create_hi110x_debug_group;
    }
#endif

#ifdef HAVE_HISI_NFC
    if (!is_my_nfc_chip())
    {
        PS_PRINT_ERR("cfg dev board nfc chip type is not match, skip driver init\n");
    }
    else
    {
        PS_PRINT_INFO("cfg dev board nfc type is matched with hisi_nfc, continue\n");
        g_sysfs_hisi_nfc = kobject_create_and_add("nfc", NULL);
        if (NULL == g_sysfs_hisi_nfc)
        {
            PS_PRINT_ERR("Failed to creat g_sysfs_hisi_nfc !!!\n ");
            goto fail_g_sysfs_hisi_nfc;
        }

        status = sysfs_create_group(g_sysfs_hisi_nfc, &hisi_nfc_attr_grp);
        if (status)
        {
            PS_PRINT_ERR("failed to create g_sysfs_hisi_nfc sysfs entries\n");
            goto fail_create_hisi_nfc_group;
        }
    }
#endif

    return 0;

#ifdef HAVE_HISI_NFC
fail_create_hisi_nfc_group:
    kobject_put(g_sysfs_hisi_nfc);
fail_g_sysfs_hisi_nfc:
#endif
#ifdef PLATFORM_DEBUG_ENABLE
    sysfs_remove_group(g_sysfs_hi110x_debug, &hi110x_debug_attr_grp);
fail_create_hi110x_debug_group:
    kobject_put(g_sysfs_hi110x_debug);
fail_g_sysfs_hi110x_debug:
#endif
    sysfs_remove_group(g_sysfs_hi110x_bfgx, &bfgx_attr_grp);
fail_create_bfgx_group:
    kobject_put(g_sysfs_hi110x_bfgx);
fail_g_sysfs_hi110x_bfgx:
    sysfs_remove_group(g_sysfs_hisi_pmdbg, &pmdbg_attr_grp);
fail_create_pmdbg_group:
    kobject_put(g_sysfs_hisi_pmdbg);
fail_g_sysfs_hisi_pmdbg:
    return -EFAULT;
}

void bfgx_user_ctrl_exit(void)
{
#ifdef HAVE_HISI_NFC
    if (is_my_nfc_chip())
    {
        sysfs_remove_group(g_sysfs_hisi_nfc, &hisi_nfc_attr_grp);
        kobject_put(g_sysfs_hisi_nfc);
    }
#endif

#ifdef PLATFORM_DEBUG_ENABLE
	sysfs_remove_group(g_sysfs_hi110x_debug, &hi110x_debug_attr_grp);
	kobject_put(g_sysfs_hi110x_debug);
#endif

	sysfs_remove_group(g_sysfs_hi110x_bfgx, &bfgx_attr_grp);
	kobject_put(g_sysfs_hi110x_bfgx);

	sysfs_remove_group(g_sysfs_hisi_pmdbg, &pmdbg_attr_grp);
	kobject_put(g_sysfs_hisi_pmdbg);
}

