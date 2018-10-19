
#ifdef __cplusplus
    #if __cplusplus
        extern "C" {
    #endif
#endif
/*****************************************************************************
  1 Header File Including
*****************************************************************************/
#define HI11XX_LOG_MODULE_NAME "[HI1103_BOARD]"
#define HI11XX_LOG_MODULE_NAME_VAR hi1103_board_loglevel
#ifdef _PRE_CONFIG_USE_DTS
#include <linux/of.h>
#include <linux/of_gpio.h>
#endif
    /*lint -e322*//*lint -e7*/
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/timer.h>
#include <linux/delay.h>
#ifdef CONFIG_PINCTRL
#include <linux/pinctrl/consumer.h>
#endif
    /*lint +e322*//*lint +e7*/

#include "board.h"
#include "plat_debug.h"
#include "plat_pm.h"
#include "oal_hcc_host_if.h"
#include "plat_uart.h"
#include "plat_firmware.h"
#include "oal_pcie_linux.h"

/*****************************************************************************
  2 Global Variable Definition
*****************************************************************************/

#ifdef BFGX_UART_DOWNLOAD_SUPPORT
extern int32 ps_recv_patch(void *disc_data, const uint8 *data, int32 count);
#endif

#ifdef _PRE_CONFIG_GPIO_TO_SSI_DEBUG
extern int32 ssi_download_test(ssi_trans_test_st* pst_ssi_test);
extern ssi_trans_test_st ssi_test_st;
#endif

#ifdef PLATFORM_DEBUG_ENABLE
int32 g_device_monitor_enable = 0;
#endif
/*
*
***************************************************************************
  3
 Function Definition
***
**************************************************************************/

//function hi110x_power_tcxo() just for p10+
void hi110x_power_tcxo(void)
{
#ifdef _PRE_CONFIG_USE_DTS
    struct device_node * np = NULL;
    int32 ret = BOARD_FAIL;
    int32 physical_gpio = 0;

    ret = get_board_dts_node(&np, DTS_NODE_HISI_HI110X);
    if(BOARD_SUCC != ret)
    {
        PS_PRINT_ERR("DTS read node %s fail!!!\n", DTS_NODE_HISI_HI110X);
        return;
    }

    ret = of_property_read_bool(np, DTS_PROP_TCXO_CTRL_ENABLE);
    if (ret)
    {
        ret = get_board_gpio(DTS_NODE_HISI_HI110X, DTS_PROP_GPIO_TCXO_1P95V, &physical_gpio);
        if(BOARD_SUCC != ret)
        {
            PS_PRINT_ERR("get dts prop %s failed\n", DTS_PROP_GPIO_TCXO_1P95V);
            return;
        }
        g_board_info.tcxo_1p95_enable = physical_gpio;
#ifdef GPIOF_OUT_INIT_LOW
        ret = gpio_request_one(g_board_info.tcxo_1p95_enable, GPIOF_OUT_INIT_LOW, PROC_NAME_GPIO_TCXO_1P95V);
        if (ret)
        {
            PS_PRINT_ERR("%s gpio_request failed\n", PROC_NAME_GPIO_TCXO_1P95V);
            return;
        }
#else
        ret = gpio_request(g_board_info.tcxo_1p95_enable, PROC_NAME_GPIO_TCXO_1P95V);
        if (ret)
        {
            PS_PRINT_ERR("%s gpio_request failed\n", PROC_NAME_GPIO_TCXO_1P95V);
            return;
        }
#endif

        gpio_direction_output(g_board_info.tcxo_1p95_enable, GPIO_LOWLEVEL);
        PS_PRINT_INFO("hi110X tcxo 1p95v enable gpio:%d\n", g_board_info.tcxo_1p95_enable);
    }
#endif

    return;
}

int32 hi1103_get_board_power_gpio(void)
{
    int32 ret = BOARD_FAIL;
    int32 physical_gpio = 0;

    /*just for p10+ start*/
    hi110x_power_tcxo();
    /*just for p10+ end*/

    ret = get_board_gpio(DTS_NODE_HISI_HI110X, DTS_PROP_GPIO_HI110X_GPIO_SSI_CLK, &physical_gpio);
    if(BOARD_SUCC != ret)
    {
        PS_PRINT_INFO("get dts prop %s failed, gpio-ssi don't support\n", DTS_PROP_GPIO_HI110X_GPIO_SSI_CLK);
    }
    else
    {
        g_board_info.ssi_gpio_clk = physical_gpio;
        ret = get_board_gpio(DTS_NODE_HISI_HI110X, DTS_PROP_GPIO_HI110X_GPIO_SSI_DATA, &physical_gpio);
        if(BOARD_SUCC != ret)
        {
            PS_PRINT_INFO("get dts prop %s failed, gpio-ssi don't support\n", DTS_PROP_GPIO_HI110X_GPIO_SSI_DATA);
            g_board_info.ssi_gpio_clk = 0;
        }
        else
        {
            g_board_info.ssi_gpio_data = physical_gpio;
            PS_PRINT_INFO("gpio-ssi support, ssi clk gpio:%d, ssi data gpio:%d\n", g_board_info.ssi_gpio_clk, g_board_info.ssi_gpio_data);
        }
    }

    ret = get_board_gpio(DTS_NODE_HISI_HI110X, DTS_PROP_GPIO_HI110X_POWEN_ON, &physical_gpio);
    if(BOARD_SUCC != ret)
    {
        PS_PRINT_ERR("get dts prop %s failed\n", DTS_PROP_GPIO_HI110X_POWEN_ON);
        return BOARD_FAIL;
    }
    g_board_info.power_on_enable = physical_gpio;
#ifdef GPIOF_OUT_INIT_LOW
    ret = gpio_request_one(physical_gpio, GPIOF_OUT_INIT_LOW, PROC_NAME_GPIO_POWEN_ON);
    if (ret)
    {
        PS_PRINT_ERR("%s gpio_request failed\n", PROC_NAME_GPIO_POWEN_ON);
        return BOARD_FAIL;
    }
#else
    ret = gpio_request(physical_gpio, PROC_NAME_GPIO_POWEN_ON);
    if (ret)
    {
        PS_PRINT_ERR("%s gpio_request failed\n", PROC_NAME_GPIO_POWEN_ON);
        return BOARD_FAIL;
    }
    gpio_direction_output(physical_gpio, 0);
#endif

    /*bfgn enable*/
    ret = get_board_gpio(DTS_NODE_HISI_HI110X, DTS_PROP_GPIO_BFGX_POWEN_ON_ENABLE, &physical_gpio);
    if(BOARD_SUCC != ret)
    {
        PS_PRINT_ERR("get dts prop %s failed\n", DTS_PROP_GPIO_BFGX_POWEN_ON_ENABLE);
        goto err_get_bfgx_power_gpio;
    }

    g_board_info.bfgn_power_on_enable = physical_gpio;
#ifdef GPIOF_OUT_INIT_LOW
    ret = gpio_request_one(physical_gpio, GPIOF_OUT_INIT_LOW, PROC_NAME_GPIO_BFGX_POWEN_ON);
    if (ret)
    {
        PS_PRINT_ERR("%s gpio_request failed\n", PROC_NAME_GPIO_BFGX_POWEN_ON);
        goto err_get_bfgx_power_gpio;
    }
#else
    ret = gpio_request(physical_gpio, PROC_NAME_GPIO_BFGX_POWEN_ON);
    if (ret)
    {
        PS_PRINT_ERR("%s gpio_request failed\n", PROC_NAME_GPIO_BFGX_POWEN_ON);
        goto err_get_bfgx_power_gpio;
    }
    gpio_direction_output(physical_gpio, 0);
#endif

#ifndef BFGX_UART_DOWNLOAD_SUPPORT
    /*wlan enable*/
    ret = get_board_gpio(DTS_NODE_HISI_HI110X, DTS_PROP_GPIO_WLAN_POWEN_ON_ENABLE, &physical_gpio);
    if(BOARD_SUCC != ret)
    {
        PS_PRINT_ERR("get dts prop %s failed\n", DTS_PROP_GPIO_WLAN_POWEN_ON_ENABLE);
        goto err_get_wlan_power_gpio;
    }
    g_board_info.wlan_power_on_enable = physical_gpio;
#ifdef GPIOF_OUT_INIT_LOW
    ret = gpio_request_one(physical_gpio, GPIOF_OUT_INIT_LOW, PROC_NAME_GPIO_WLAN_POWEN_ON);
    if (ret)
    {
        PS_PRINT_ERR("%s gpio_request failed\n", PROC_NAME_GPIO_WLAN_POWEN_ON);
        goto err_get_wlan_power_gpio;
    }
#else
    ret = gpio_request(physical_gpio, PROC_NAME_GPIO_WLAN_POWEN_ON);
    if (ret)
    {
        PS_PRINT_ERR("%s gpio_request failed\n", PROC_NAME_GPIO_WLAN_POWEN_ON);
        goto err_get_wlan_power_gpio;
    }
    gpio_direction_output(physical_gpio, 0);
#endif
#endif
    return BOARD_SUCC;
#ifndef BFGX_UART_DOWNLOAD_SUPPORT
err_get_wlan_power_gpio:
    gpio_free(g_board_info.bfgn_power_on_enable);
#endif
err_get_bfgx_power_gpio:
    gpio_free(g_board_info.power_on_enable);
    return BOARD_FAIL;
}

void hi1103_free_board_power_gpio(void)
{
    gpio_free(g_board_info.power_on_enable);
    gpio_free(g_board_info.bfgn_power_on_enable);
#ifndef BFGX_UART_DOWNLOAD_SUPPORT
    gpio_free(g_board_info.wlan_power_on_enable);
#endif
}

int32 hi1103_board_wakeup_gpio_init(void)
{
    int32 ret = BOARD_FAIL;
    int32 physical_gpio = 0;

    /*bfgx wake host gpio request*/
    ret = get_board_gpio(DTS_NODE_HI110X_BFGX, DTS_PROP_HI110X_GPIO_BFGX_WAKEUP_HOST, &physical_gpio);
    if(BOARD_SUCC != ret)
    {
        PS_PRINT_ERR("get dts prop %s failed\n", DTS_PROP_HI110X_GPIO_BFGX_WAKEUP_HOST);
        goto err_get_bfgx_wkup_host_gpio;
    }

    g_board_info.bfgn_wakeup_host = physical_gpio;
    PS_PRINT_INFO("hi110x bfgx wkup host gpio is %d\n",g_board_info.bfgn_wakeup_host);
#ifdef GPIOF_IN
    ret = gpio_request_one(physical_gpio, GPIOF_IN, PROC_NAME_GPIO_BFGX_WAKEUP_HOST);
    if (ret)
    {
        PS_PRINT_ERR("%s gpio_request failed\n", PROC_NAME_GPIO_BFGX_WAKEUP_HOST);
        goto err_get_bfgx_wkup_host_gpio;
    }
#else
    ret = gpio_request(physical_gpio, PROC_NAME_GPIO_BFGX_WAKEUP_HOST);
    if (ret)
    {
        PS_PRINT_ERR("%s gpio_request failed\n", PROC_NAME_GPIO_BFGX_WAKEUP_HOST);
        goto err_get_bfgx_wkup_host_gpio;
    }
    gpio_direction_input(physical_gpio);
#endif

#ifndef  BFGX_UART_DOWNLOAD_SUPPORT
    /*wifi wake host gpio request*/
    ret = get_board_gpio(DTS_NODE_HI110X_WIFI, DTS_PROP_HI110X_GPIO_WLAN_WAKEUP_HOST, &physical_gpio);
    if(BOARD_SUCC != ret)
    {
        PS_PRINT_ERR("get dts prop %s failed\n", DTS_PROP_HI110X_GPIO_WLAN_WAKEUP_HOST);
        goto err_get_wlan_wkup_host_gpio;
    }

    g_board_info.wlan_wakeup_host = physical_gpio;
    PS_PRINT_INFO("hi110x wifi wkup host gpio is %d\n",g_board_info.wlan_wakeup_host);
#ifdef GPIOF_IN
    ret = gpio_request_one(physical_gpio, GPIOF_IN, PROC_NAME_GPIO_WLAN_WAKEUP_HOST);
    if (ret)
    {
        PS_PRINT_ERR("%s gpio_request failed\n", PROC_NAME_GPIO_WLAN_WAKEUP_HOST);
        goto err_get_wlan_wkup_host_gpio;
    }
#else
    ret = gpio_request(physical_gpio, PROC_NAME_GPIO_WLAN_WAKEUP_HOST);
    if (ret)
    {
        PS_PRINT_ERR("%s gpio_request failed\n", PROC_NAME_GPIO_WLAN_WAKEUP_HOST);
        goto err_get_wlan_wkup_host_gpio;
    }
    gpio_direction_input(physical_gpio);
#endif

    /*host wake wlan gpio request*/
    ret = get_board_gpio(DTS_NODE_HI110X_WIFI, DTS_PROP_GPIO_HOST_WAKEUP_WLAN, &physical_gpio);
    if(BOARD_SUCC != ret)
    {
        PS_PRINT_ERR("get dts prop %s failed\n", DTS_PROP_GPIO_HOST_WAKEUP_WLAN);
        goto err_get_host_wake_up_wlan_fail;
    }

    g_board_info.host_wakeup_wlan = physical_gpio;
    PS_PRINT_INFO("hi110x host wkup wifi gpio is %d\n",g_board_info.host_wakeup_wlan);
#ifdef GPIOF_OUT_INIT_LOW
    ret = gpio_request_one(physical_gpio, GPIOF_OUT_INIT_LOW, PROC_NAME_GPIO_HOST_WAKEUP_WLAN);
    if (ret)
    {
        PS_PRINT_ERR("%s gpio_request failed\n", PROC_NAME_GPIO_HOST_WAKEUP_WLAN);
        goto err_get_host_wake_up_wlan_fail;
    }
#else
    ret = gpio_request(physical_gpio, PROC_NAME_GPIO_HOST_WAKEUP_WLAN);
    if (ret)
    {
        PS_PRINT_ERR("%s gpio_request failed\n", PROC_NAME_GPIO_HOST_WAKEUP_WLAN);
        goto err_get_host_wake_up_wlan_fail;
    }else{
        gpio_direction_input(physical_gpio);
    }
#endif
#endif
    return BOARD_SUCC;
#ifndef  BFGX_UART_DOWNLOAD_SUPPORT
err_get_host_wake_up_wlan_fail:
    gpio_free(g_board_info.wlan_wakeup_host);
err_get_wlan_wkup_host_gpio:
    gpio_free(g_board_info.bfgn_wakeup_host);
#endif
err_get_bfgx_wkup_host_gpio:
    return BOARD_FAIL;
}

void hi1103_free_board_wakeup_gpio(void)
{
    gpio_free(g_board_info.bfgn_wakeup_host);
#ifndef BFGX_UART_DOWNLOAD_SUPPORT
    gpio_free(g_board_info.wlan_wakeup_host);
    gpio_free(g_board_info.host_wakeup_wlan);
#endif
}

void hi1103_tcxo_enable(void)
{
    PS_PRINT_INFO("hi110x enable tcxo\n");
    power_state_change(g_board_info.tcxo_1p95_enable, BOARD_POWER_ON);
}

void hi1103_tcxo_disable(void)
{
    PS_PRINT_INFO("hi110x disable tcxo\n");
    power_state_change(g_board_info.tcxo_1p95_enable, BOARD_POWER_OFF);
}

void hi1103_chip_power_on(void)
{
    hi1103_tcxo_enable();
    board_host_wakeup_dev_set(0);
    power_state_change(g_board_info.power_on_enable, BOARD_POWER_ON);
#if defined(PLATFORM_DEBUG_ENABLE) && defined(_PRE_CONFIG_GPIO_TO_SSI_DEBUG)
#if 0
    if(0 == ssi_force_reset_aon())
    {
        mdelay(5);
    }
#endif
#endif
}

void hi1103_chip_power_off(void)
{
    power_state_change(g_board_info.power_on_enable, BOARD_POWER_OFF);
    hi1103_tcxo_disable();
    board_host_wakeup_dev_set(0);
}

void hi1103_wifi_enable(void)
{
    int32 ret = -EFAIL;
    int32 gpio = g_board_info.wlan_power_on_enable;

    //if(hcc_get_current_110x_bus())
    {
        /*��һ��ö��ʱBUS ��δ��ʼ��*/
        ret = hcc_bus_power_ctrl_register(hcc_get_current_110x_bus(), HCC_BUS_CTRL_POWER_UP, board_wlan_gpio_power_on, (void*)(long)gpio);
        hcc_bus_power_action(hcc_get_current_110x_bus(), HCC_BUS_POWER_UP);
    }
}

void hi1103_wifi_disable(void)
{
    int32 ret = -EFAIL;
    int32 gpio = g_board_info.wlan_power_on_enable;

    //if(hcc_get_current_110x_bus())
    {
        ret = hcc_bus_power_ctrl_register(hcc_get_current_110x_bus(), HCC_BUS_CTRL_POWER_DOWN, board_wlan_gpio_power_off, (void*)(long)gpio);
        hcc_bus_power_action(hcc_get_current_110x_bus(), HCC_BUS_POWER_DOWN);
    }
}

void hi1103_bfgx_enable(void)
{
    power_state_change(g_board_info.bfgn_power_on_enable, BOARD_POWER_ON);
}

void hi1103_bfgx_disable(void)
{
    power_state_change(g_board_info.bfgn_power_on_enable, BOARD_POWER_OFF);
}

void hi1103_board_power_on(uint32 ul_subsystem)
{
    if (WLAN_POWER == ul_subsystem)
    {
        if (bfgx_is_shutdown())
        {
            PS_PRINT_INFO("wifi pull up power_on_enable gpio!\n");
            hi1103_chip_power_on();
            hi1103_bfgx_enable();
        }
        hi1103_wifi_enable();
    }
    else if (BFGX_POWER == ul_subsystem)
    {
#ifndef BFGX_UART_DOWNLOAD_SUPPORT
        if (wlan_is_shutdown())
        {
            PS_PRINT_INFO("bfgx pull up power_on_enable gpio!\n");
            hi1103_chip_power_on();
            hi1103_wifi_enable();
        }
#else
        hi1103_chip_power_on();
#endif
        hi1103_bfgx_enable();
    }
    else
    {
        PS_PRINT_ERR("power input system:%d error\n", ul_subsystem);
    }
}

void hi1103_board_power_off(uint32 ul_subsystem)
{
    if (WLAN_POWER == ul_subsystem)
    {
        hi1103_wifi_disable();
        if (bfgx_is_shutdown())
        {
            PS_PRINT_INFO("wifi pull down power_on_enable!\n");
            hi1103_bfgx_disable();
            hi1103_chip_power_off();
        }
    }
    else if (BFGX_POWER == ul_subsystem)
    {
        hi1103_bfgx_disable();
#ifndef BFGX_UART_DOWNLOAD_SUPPORT
        if (wlan_is_shutdown())
        {
            PS_PRINT_INFO("bfgx pull down power_on_enable!\n");
            hi1103_wifi_disable();
            hi1103_chip_power_off();
        }
#else
        hi1103_chip_power_off();
#endif
    }
    else
    {
        PS_PRINT_ERR("power input system:%d error\n", ul_subsystem);
        return;
    }
}

void hi1103_board_power_reset(uint32 ul_subsystem)
{
    hi1103_bfgx_disable();
#ifndef BFGX_UART_DOWNLOAD_SUPPORT
    hi1103_wifi_disable();
#endif
    hi1103_chip_power_off();
    hi1103_chip_power_on();
#ifndef BFGX_UART_DOWNLOAD_SUPPORT
    hi1103_wifi_enable();
#endif
    hi1103_bfgx_enable();
}

void hi1103_bfgx_subsys_reset(void)
{
    hi1103_bfgx_disable();
    hi1103_bfgx_enable();
}

void hi1103_wifi_subsys_reset(void)
{
    hi1103_wifi_disable();
    hi1103_wifi_enable();
}

#ifndef BFGX_UART_DOWNLOAD_SUPPORT
int32 hi1103_bfgx_dev_power_on(void)
{
    int32 error = BFGX_POWER_SUCCESS;
    struct ps_core_s *ps_core_d = NULL;
    struct pm_drv_data *pm_data = pm_get_drvdata();

    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return BFGX_POWER_FAILED;
    }

    ps_get_core_reference(&ps_core_d);
    if (unlikely(NULL == ps_core_d))
    {
        PS_PRINT_ERR("ps_core_d is err\n");
        return BFGX_POWER_FAILED;
    }

    hi1103_board_power_on(BFGX_POWER);

    if (BFGX_POWER_SUCCESS != open_tty_drv(ps_core_d->pm_data))
    {
        PS_PRINT_ERR("open tty fail!\n");
        error = BFGX_POWER_TTY_OPEN_FAIL;
        goto bfgx_power_on_fail;
    }

    if (wlan_is_shutdown())
    {
        if (BFGX_POWER_SUCCESS != firmware_download_function(BFGX_CFG))
        {
            hcc_bus_disable_state(pm_data->pst_wlan_pm_info->pst_bus, OAL_BUS_STATE_ALL);
            PS_PRINT_ERR("bfgx download firmware fail!\n");
            error = BFGX_POWER_DOWNLOAD_FIRMWARE_FAIL;
            goto bfgx_power_on_fail;
        }
        hcc_bus_disable_state(pm_data->pst_wlan_pm_info->pst_bus, OAL_BUS_STATE_ALL);

/*eng support monitor*/
#ifdef PLATFORM_DEBUG_ENABLE
    if (!g_device_monitor_enable)
    {
#endif
        hi1103_wifi_disable();
#ifdef PLATFORM_DEBUG_ENABLE
    }
#endif
    }
    else
    {
        /*��ʱBFGX ��Ҫ�⸴λBCPU*/
        PS_PRINT_ERR("wifi dereset bcpu\n");
        if(BFGX_POWER_SUCCESS != wlan_pm_open_bcpu())
        {
            PS_PRINT_ERR("wifi dereset bcpu fail!\n");
            error = BFGX_POWER_WIFI_DERESET_BCPU_FAIL;
            CHR_EXCEPTION(CHR_GNSS_DRV(CHR_GNSS_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_OPEN_BCPU));
            goto bfgx_power_on_fail;
        }
    }

    return BFGX_POWER_SUCCESS;

bfgx_power_on_fail:
    hi1103_board_power_off(BFGX_POWER);
    return error;
}
#else
int32 hi1103_bfgx_dev_power_on(void)
{
    int32 error = BFGX_POWER_SUCCESS;
    struct ps_core_s *ps_core_d = NULL;
    struct pm_drv_data *pm_data = pm_get_drvdata();
    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return BFGX_POWER_FAILED;
    }

    ps_get_core_reference(&ps_core_d);
    if (unlikely(NULL == ps_core_d))
    {
        PS_PRINT_ERR("ps_core_d is err\n");
        return BFGX_POWER_FAILED;
    }

    PS_PRINT_INFO("bfgx pull up power_on_enable gpio!\n");
    hi1103_board_power_on(BFGX_POWER);

    if (BFGX_POWER_SUCCESS != open_tty_drv(ps_core_d->pm_data))
    {
        PS_PRINT_ERR("open tty fail!\n");
        error = BFGX_POWER_TTY_OPEN_FAIL;
        goto bfgx_power_on_fail;
    }

/*ssi write mem test*/
#ifdef SSI_DOWNLOAD_BFGX_FIRMWARE
#ifdef _PRE_CONFIG_GPIO_TO_SSI_DEBUG
    ssi_test_st.test_type = SSI_FILE_TEST;
    if (0 != ssi_download_test(&ssi_test_st))
    {
        PS_PRINT_ERR("ssi_download_test fail!\n");
        goto bfgx_power_on_fail;
    }
#else
    PS_PRINT_ERR("ssi_download_test no support\n");
    goto bfgx_power_on_fail;
#endif
/*gnss only download test*/
#else
    tty_recv = ps_recv_patch;
    error = ps_core_d->ps_pm->download_patch();
    if (error)
    {   /* if download patch err,and close uart */
        //ps_core_d->in_download_patch_state = false;
        error = release_tty_drv(ps_core_d->pm_data);
        PS_PRINT_ERR(" download_patch is failed!\n");
        goto bfgx_power_on_fail;
    }
#endif

    PS_PRINT_SUC(" download_patch is successfully!\n");

    return BFGX_POWER_SUCCESS;

bfgx_power_on_fail:
    hi1103_board_power_off(BFGX_POWER);
    return error;
}
#endif

int32 hi1103_bfgx_dev_power_off(void)
{

    int32 error = BFGX_POWER_SUCCESS;
    struct ps_core_s *ps_core_d = NULL;
    struct pm_drv_data *pm_data = pm_get_drvdata();

    PS_PRINT_INFO("%s\n", __func__);

    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return BFGX_POWER_FAILED;
    }

    ps_get_core_reference(&ps_core_d);
    if (unlikely(NULL == ps_core_d))
    {
        PS_PRINT_ERR("ps_core_d is err\n");
        return BFGX_POWER_FAILED;
    }

    if(SUCCESS != uart_bfgx_close_cmd())
    {
       /*bfgx self close fail �ˣ�����ҲҪͨ��wifi shutdown bcpu*/
       PS_PRINT_ERR("bfgx self close fail\n");
       CHR_EXCEPTION(CHR_GNSS_DRV(CHR_GNSS_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_CLOSE_BCPU));
    }

    if (SUCCESS != release_tty_drv(ps_core_d->pm_data))
    {
       /*����ִ�е��˴���˵������һ����ҵ���Ѿ��رգ�����tty�Ƿ�رճɹ���device��Ҫ�µ�*/
       PS_PRINT_ERR("wifi off, close tty is err!");
    }

    if (!wlan_is_shutdown())
    {
        PS_PRINT_ERR("wifi shutdown bcpu\n");
        if(SUCCESS != wlan_pm_shutdown_bcpu_cmd())
        {
           PS_PRINT_ERR("wifi shutdown bcpu fail\n");
           CHR_EXCEPTION(CHR_GNSS_DRV(CHR_GNSS_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_CLOSE_BCPU));
           error = BFGX_POWER_FAILED;
        }
    }

    PS_PRINT_INFO("%s,set BFGX_SLEEP\n", __func__);
    pm_data->bfgx_dev_state = BFGX_SLEEP;
    pm_data->uart_state     = UART_NOT_READY;
    pm_data->uart_baud_switch_to = g_default_baud_rate;

    hi1103_board_power_off(BFGX_POWER);

    return error;
}

int32 hi1103_wlan_power_off(void)
{
    struct pm_drv_data *pm_data = pm_get_drvdata();
    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -FAILURE;
    }

    /*�ȹر�SDIO TXͨ��*/
    hcc_bus_disable_state(hcc_get_current_110x_bus(), OAL_BUS_STATE_TX);

    /*wakeup dev,send poweroff cmd to wifi*/
    if(OAL_SUCC != wlan_pm_poweroff_cmd())
    {
        /*wifi self close ʧ����Ҳ��������ִ�У�uart�ر�WCPU���쳣�ָ��Ƴٵ�wifi�´�open��ʱ��ִ��*/
        DECLARE_DFT_TRACE_KEY_INFO("wlan_poweroff_by_sdio_fail",OAL_DFT_TRACE_FAIL);
        CHR_EXCEPTION(CHR_WIFI_DRV(CHR_WIFI_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_CLOSE_WCPU));
#ifdef PLATFORM_DEBUG_ENABLE
        return -FAILURE;
#endif
    }

    hcc_bus_disable_state(hcc_get_current_110x_bus(), OAL_BUS_STATE_ALL);

    hi1103_board_power_off(WLAN_POWER);

    pm_data->pst_wlan_pm_info->ul_wlan_power_state = POWER_STATE_SHUTDOWN;

    return SUCCESS;
}
int32 hi1103_wlan_power_on(void)
{
    int32 ret;
    int32  error = WIFI_POWER_SUCCESS;
    struct pm_drv_data *pm_data = pm_get_drvdata();
    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -FAILURE;
    }

    hi1103_board_power_on(WLAN_POWER);

    hcc_bus_power_action(hcc_get_current_110x_bus(), HCC_BUS_POWER_PATCH_LOAD_PREPARE);

    if (bfgx_is_shutdown())
    {
        error = firmware_download_function(BFGX_AND_WIFI_CFG);
        hi1103_bfgx_disable();
    }
    else
    {
        error = firmware_download_function(WIFI_CFG);
    }

    if (WIFI_POWER_SUCCESS != error)
    {
        PS_PRINT_ERR("firmware download fail\n");
        error = WIFI_POWER_BFGX_OFF_FIRMWARE_DOWNLOAD_FAIL;
        goto wifi_power_fail;
    }
    else
    {
        pm_data->pst_wlan_pm_info->ul_wlan_power_state = POWER_STATE_OPEN;
    }

    ret = hcc_bus_power_action(hcc_get_current_110x_bus(), HCC_BUS_POWER_PATCH_LAUCH);
    if (0 != ret)
    {
        DECLARE_DFT_TRACE_KEY_INFO("wlan_poweron HCC_BUS_POWER_PATCH_LAUCH by gpio_fail", OAL_DFT_TRACE_FAIL);
        PS_PRINT_ERR("wlan_poweron HCC_BUS_POWER_PATCH_LAUCH by gpio fail ret=%d", ret);
        error = WIFI_POWER_BFGX_OFF_BOOT_UP_FAIL;
        CHR_EXCEPTION(CHR_WIFI_DRV(CHR_WIFI_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_WCPU_BOOTUP));
        goto wifi_power_fail;
    }

    return WIFI_POWER_SUCCESS;
wifi_power_fail:
    //hi1103_board_power_off(WLAN_POWER);
    return error;

}

int32 hi1103_get_board_pmu_clk32k(void)
{
    int32 ret= BOARD_FAIL;
    const char * clk_name = NULL;

    ret = get_board_custmize(DTS_NODE_HISI_HI110X, DTS_PROP_CLK_32K, &clk_name);
    if (BOARD_SUCC != ret)
    {
        return BOARD_FAIL;
    }
    g_board_info.clk_32k_name = clk_name;

    PS_PRINT_INFO("hi110x 32k clk name is %s\n", g_board_info.clk_32k_name);

    return BOARD_SUCC;
}
int32 hi1103_get_board_uart_port(void)
{
#ifdef _PRE_CONFIG_USE_DTS
    int32 ret= BOARD_FAIL;
    struct device_node * np = NULL;
    const char *uart_port = NULL;

    ret = get_board_dts_node(&np, DTS_NODE_HI110X_BFGX);
    if(BOARD_SUCC != ret)
    {
        PS_PRINT_ERR("DTS read node %s fail!!!\n", DTS_NODE_HI110X_BFGX);
        return BOARD_FAIL;
    }

    /*ʹ��uart4����Ҫ��dts������DTS_PROP_UART_PCLK�ָ��uart4������sensorhub*/
    ret = of_property_read_bool(np, DTS_PROP_HI110X_UART_PCLK);
    if (ret)
    {
        PS_PRINT_INFO("uart pclk normal\n");
        g_board_info.uart_pclk = UART_PCLK_NORMAL;
    }
    else
    {
        PS_PRINT_INFO("uart pclk from sensorhub\n");
        g_board_info.uart_pclk = UART_PCLK_FROM_SENSORHUB;
    }

    ret = get_board_custmize(DTS_NODE_HI110X_BFGX, DTS_PROP_HI110X_UART_POART, &uart_port);
    if(BOARD_SUCC != ret)
    {
        return BOARD_FAIL;
    }
    g_board_info.uart_port = uart_port;

    return BOARD_SUCC;
#else
    return BOARD_SUCC;
#endif
}

#ifdef HAVE_HISI_IR
int32 hi1103_board_ir_ctrl_gpio_init(void)
{
    int32 ret = BOARD_FAIL;
    int32 physical_gpio = 0;

    ret = get_board_gpio(DTS_NODE_HI110X_BFGX, DTS_PROP_GPIO_BFGX_IR_CTRL, &physical_gpio);
    if(BOARD_SUCC != ret)
    {
        PS_PRINT_INFO("dts prop %s not exist\n", DTS_PROP_GPIO_BFGX_IR_CTRL);
        g_board_info.bfgx_ir_ctrl_gpio= -1;
    }
    else
    {
        g_board_info.bfgx_ir_ctrl_gpio= physical_gpio;

#ifdef GPIOF_OUT_INIT_LOW
        ret = gpio_request_one(physical_gpio, GPIOF_OUT_INIT_LOW, PROC_NAME_GPIO_BFGX_IR_CTRL);
        if (ret)
        {
            PS_PRINT_ERR("%s gpio_request failed\n", PROC_NAME_GPIO_BFGX_IR_CTRL);
        }
#else
        ret = gpio_request(physical_gpio, PROC_NAME_GPIO_BFGX_IR_CTRL);
        if (ret)
        {
            PS_PRINT_ERR("%s gpio_request failed\n", PROC_NAME_GPIO_BFGX_IR_CTRL);
        }
        else
        {
            gpio_direction_output(physical_gpio, 0);
        }
#endif
    }

    return BOARD_SUCC;
}

int32 hi1103_board_ir_ctrl_pmic_init(struct platform_device *pdev)
{
#ifdef _PRE_CONFIG_USE_DTS
    int32 ret = BOARD_FAIL;
    struct device_node * np = NULL;
    int32 irled_voltage = 0;
    if (NULL == pdev){
        PS_PRINT_ERR("board pmu pdev is NULL!\n");
        return ret;
    }

    ret = get_board_dts_node(&np, DTS_NODE_HI110X_BFGX);
    if (BOARD_SUCC != ret)
    {
        PS_PRINT_ERR("DTS read node %s fail!!!\n", DTS_NODE_HI110X_BFGX);
        return ret;
    }

    g_board_info.bfgn_ir_ctrl_ldo = regulator_get(&pdev->dev, DTS_PROP_HI110x_IRLED_LDO_POWER);

    if (IS_ERR(g_board_info.bfgn_ir_ctrl_ldo))
    {
        PS_PRINT_ERR("board_ir_ctrl_pmic_init get ird ldo failed\n");
        return ret;
    }

    ret = of_property_read_u32(np, DTS_PROP_HI110x_IRLED_VOLTAGE, &irled_voltage);
    if (BOARD_SUCC == ret)
    {
        PS_PRINT_INFO("set irled voltage %d mv\n", irled_voltage/1000);
        ret = regulator_set_voltage(g_board_info.bfgn_ir_ctrl_ldo,(int)irled_voltage,(int)irled_voltage);
        if (ret)
        {
            PS_PRINT_ERR("board_ir_ctrl_pmic_init set voltage ldo failed\n");
            return ret;
        }
    }
    else
    {
        PS_PRINT_ERR("get irled voltage failed ,use default\n");
    }

    ret = regulator_set_mode(g_board_info.bfgn_ir_ctrl_ldo,REGULATOR_MODE_NORMAL);
    if (ret)
    {
        PS_PRINT_ERR("board_ir_ctrl_pmic_init set ldo mode failed\n");
        return ret;
    }
    PS_PRINT_INFO("board_ir_ctrl_pmic_init success\n");
    return BOARD_SUCC;
#else
    return BOARD_SUCC;
#endif
}
#endif

int32 hi1103_board_ir_ctrl_init(struct platform_device *pdev)
{
#if (defined HAVE_HISI_IR) && (defined _PRE_CONFIG_USE_DTS)
    int ret = BOARD_SUCC;
    struct device_node * np = NULL;

    ret = get_board_dts_node(&np, DTS_NODE_HI110X_BFGX);
    if (BOARD_SUCC != ret)
    {
        PS_PRINT_ERR("DTS read node %s fail!!!\n", DTS_NODE_HI110X_BFGX);
        goto err_get_ir_ctrl_gpio;
    }
    g_board_info.have_ir = of_property_read_bool(np, "have_ir");
    if (!g_board_info.have_ir)
    {
        PS_PRINT_ERR("board has no Ir");
    }
    else
    {
        ret = of_property_read_u32(np, DTS_PROP_HI110x_IR_LDO_TYPE, &g_board_info.irled_power_type);
        PS_PRINT_INFO("read property ret is %d, irled_power_type is %d\n", ret, g_board_info.irled_power_type);
        if (BOARD_SUCC != ret)
        {
            PS_PRINT_ERR("get dts prop %s failed\n", DTS_PROP_HI110x_IR_LDO_TYPE);
            goto err_get_ir_ctrl_gpio;
        }

        if (IR_GPIO_CTRL == g_board_info.irled_power_type)
        {
            ret = hi1103_board_ir_ctrl_gpio_init();
            if (BOARD_SUCC != ret)
            {
                PS_PRINT_ERR("ir_ctrl_gpio init failed\n");
                goto err_get_ir_ctrl_gpio;
            }
        }
        else if (IR_LDO_CTRL == g_board_info.irled_power_type)
        {
            ret = hi1103_board_ir_ctrl_pmic_init(pdev);
            if (BOARD_SUCC != ret)
            {
                PS_PRINT_ERR("ir_ctrl_pmic init failed\n");
                goto err_get_ir_ctrl_gpio;
            }
        }
        else
        {
            PS_PRINT_ERR("get ir_ldo_type failed!err num is %d\n", g_board_info.irled_power_type);
            goto err_get_ir_ctrl_gpio;
        }
    }
    return BOARD_SUCC;

err_get_ir_ctrl_gpio:
    return BOARD_FAIL;
#else
    return BOARD_SUCC;
#endif
}

int32 hi1103_check_evb_or_fpga(void)
{
#ifdef _PRE_CONFIG_USE_DTS
    int32 ret= BOARD_FAIL;
    struct device_node * np = NULL;

    ret = get_board_dts_node(&np, DTS_NODE_HISI_HI110X);
    if(BOARD_SUCC != ret)
    {
        PS_PRINT_ERR("DTS read node %s fail!!!\n", DTS_NODE_HISI_HI110X);
        return BOARD_FAIL;
    }

    ret = of_property_read_bool(np, DTS_PROP_HI110X_VERSION);
    if (ret)
    {
        PS_PRINT_INFO("HI1103 ASIC VERSION\n");
        g_board_info.is_asic = VERSION_ASIC;
    }
    else
    {
        PS_PRINT_INFO("HI1103 FPGA VERSION\n");
        g_board_info.is_asic = VERSION_FPGA;
    }

    return BOARD_SUCC;
#else
    return BOARD_SUCC;
#endif
}
int32 hi1103_board_get_power_pinctrl(struct platform_device *pdev)
{
#ifdef _PRE_CONFIG_USE_DTS
    int32  ret = BOARD_FAIL;
    int32  physical_gpio = 0;
    struct device_node * np = NULL;
    struct pinctrl *pinctrl;
    struct pinctrl_state *pinctrl_def;
    struct pinctrl_state *pinctrl_idle;

    /* ����Ƿ���Ҫprepare before board power on */
    /* JTAG SELECT ���ͣ�XLDO MODEѡ��2.8v */
    ret = get_board_dts_node(&np, DTS_NODE_HISI_HI110X);
    if(BOARD_SUCC != ret)
    {
        PS_PRINT_ERR("DTS read node %s fail!!!\n", DTS_NODE_HISI_HI110X);
        goto err_read_dts_node;
    }

    ret = of_property_read_bool(np, DTS_PROP_HI110X_POWER_PREPARE);
    if (ret)
    {
        PS_PRINT_INFO("need prepare before board power on\n");
        g_board_info.need_power_prepare = NEED_POWER_PREPARE;
    }
    else
    {
        PS_PRINT_INFO("no need prepare before board power on\n");
        g_board_info.need_power_prepare = NO_NEED_POWER_PREPARE;
    }

    if (NO_NEED_POWER_PREPARE == g_board_info.need_power_prepare)
    {
        return BOARD_SUCC;
    }

    pinctrl = devm_pinctrl_get(&pdev->dev);
    if (OAL_IS_ERR_OR_NULL(pinctrl))
    {
        PS_PRINT_ERR("iomux_lookup_block failed, and the value of pinctrl is %p\n", pinctrl);
        CHR_EXCEPTION(CHR_WIFI_DEV(CHR_WIFI_DEV_EVENT_CHIP, CHR_WIFI_DEV_ERROR_IOMUX));
        goto err_pinctrl_get;
    }
    g_board_info.pctrl = pinctrl;

    pinctrl_def = pinctrl_lookup_state(pinctrl, "default");
    if (OAL_IS_ERR_OR_NULL(pinctrl_def))
    {
        PS_PRINT_ERR("pinctrl_lookup_state default failed, and the value of pinctrl_def is %p\n", pinctrl_def);
        goto err_lookup_default;
    }
    g_board_info.pins_normal = pinctrl_def;

    pinctrl_idle = pinctrl_lookup_state(pinctrl, "idle");
    if (OAL_IS_ERR_OR_NULL(pinctrl_idle))
    {
        PS_PRINT_ERR("pinctrl_lookup_state idel failed, and the value of pinctrl_idle is %p\n", pinctrl_idle);
        goto err_lookup_idle;
    }
    g_board_info.pins_idle = pinctrl_idle;

    ret = pinctrl_select_state(g_board_info.pctrl, g_board_info.pins_normal);
    if (ret < 0)
    {
        PS_PRINT_ERR("pinctrl_select_state default failed.\n");
        goto err_select_state;
    }

    /* LTE_TX_ACTIVT GPIO */
    ret = get_board_gpio(DTS_NODE_HISI_HI110X, DTS_PROP_HI110X_GPIO_XLDO_PINMUX, &physical_gpio);

    if(BOARD_SUCC != ret)
    {
        PS_PRINT_ERR("get dts prop %s failed\n", DTS_PROP_GPIO_HI110X_POWEN_ON);
        CHR_EXCEPTION(CHR_WIFI_DEV(CHR_WIFI_DEV_EVENT_CHIP, CHR_WIFI_DEV_ERROR_GPIO));
        goto err_get_xldo_pinmux;
    }

    g_board_info.xldo_pinmux = physical_gpio;

    ret = of_property_read_u32(np, DTS_PROP_GPIO_XLDO_LEVEL, &physical_gpio);
    if(BOARD_SUCC != ret)
    {
        PS_PRINT_ERR("get dts prop %s failed\n", DTS_PROP_GPIO_XLDO_LEVEL);
        CHR_EXCEPTION(CHR_WIFI_DEV(CHR_WIFI_DEV_EVENT_CHIP, CHR_WIFI_DEV_ERROR_GPIO));
        goto err_read_xldo_level;
    }

    g_board_info.gpio_xldo_level = physical_gpio;

    return BOARD_SUCC;

    err_read_xldo_level:
    err_get_xldo_pinmux:
    err_select_state:
    err_lookup_idle:
    err_lookup_default:
        devm_pinctrl_put(g_board_info.pctrl);
    err_pinctrl_get:
    err_read_dts_node:

    return BOARD_FAIL;
#else
    return BOARD_SUCC;
#endif
}
int32 hi1103_get_ini_file_name_from_dts(int8 *dts_prop, int8 *prop_value, uint32 size)
{
#ifdef _PRE_CONFIG_USE_DTS
    int32  ret = 0;
    struct device_node *np = NULL;
    int32  len;
    int8   out_str[HISI_CUST_NVRAM_LEN] = {0};

    np = of_find_compatible_node(NULL, NULL, COST_HI110X_COMP_NODE);

    if (NULL == np)
    {
        INI_ERROR("dts node %s not found", COST_HI110X_COMP_NODE);
        return INI_FAILED;
    }

    len = of_property_count_u8_elems(np, dts_prop);
    if (len < 0)
    {
        INI_ERROR("can't get len of dts prop(%s)", dts_prop);
        return INI_FAILED;
    }

    len = INI_MIN(len, (int32)sizeof(out_str));
    INI_DEBUG("read len of dts prop %s is:%d", dts_prop, len);
    ret = of_property_read_u8_array(np, dts_prop, out_str, len);
    if (0 > ret)
    {
        INI_ERROR("read dts prop (%s) fail", dts_prop);
        return INI_FAILED;
    }

    len = INI_MIN(len, (int32)size);
    memcpy(prop_value, out_str, (size_t)len);
    INI_DEBUG("dts prop %s value is:%s", dts_prop, prop_value);
#endif
    return INI_SUCC;
}

/*factory test, wifi power on,
  do some test under bootloader mode*/
void hi1103_dump_gpio_regs(void)
{
    int16 value;
    int32 ret;
    value = 0;
    ret = read_device_reg16(GPIO_BASE_ADDR + GPIO_INOUT_CONFIG_REGADDR, &value);
    if(ret)
    {
        return;
    }

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "gpio reg 0x%x = 0x%x", GPIO_BASE_ADDR + GPIO_INOUT_CONFIG_REGADDR, value);

    value = 0;
    ret = read_device_reg16(GPIO_BASE_ADDR + GPIO_LEVEL_GET_REGADDR, &value);
    if(ret)
    {
        return;
    }

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "gpio reg 0x%x = 0x%x", GPIO_BASE_ADDR + GPIO_LEVEL_GET_REGADDR, value);
}

int32 hi1103_check_device_ready(void)
{
    int16 value;
    int32 ret;

    value = 0;
    ret = read_device_reg16(0x50000000, &value);
    if(ret)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "read 0x%x reg failed, ret=%d", (0x50000000),ret);
        return -OAL_EFAIL;
    }

    if(value != 0x101)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "device sysctrl reg error, value=0x%x", value);
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}

int32 hi1103_check_wlan_wakeup_host(void)
{
    int32 i;
    int16 value;
    int32 ret;
    if(0 == g_board_info.wlan_wakeup_host)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "wlan_wakeup_host gpio is zero!");
        return -OAL_EIO;
    }

    value = 0;
    ret = read_device_reg16(GPIO_BASE_ADDR + GPIO_INOUT_CONFIG_REGADDR, &value);
    if(ret)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "read 0x%x reg failed, ret=%d", (GPIO_BASE_ADDR + GPIO_INOUT_CONFIG_REGADDR),ret);
        return -1;
    }

    /*���*/
    value |= (WLAN_DEV2HOST_GPIO);

    ret = write_device_reg16(GPIO_BASE_ADDR + GPIO_INOUT_CONFIG_REGADDR, value);
    if(ret)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "write 0x%x reg failed,value=0x%x, ret=%d",
                            (GPIO_BASE_ADDR + GPIO_INOUT_CONFIG_REGADDR),value, ret);
        return -1;
    }

    for(i = 0 ; i < 2; i++)
    {
        ret = write_device_reg16(GPIO_BASE_ADDR + GPIO_LEVEL_CONFIG_REGADDR, WLAN_DEV2HOST_GPIO);
        if(ret)
        {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "write 0x%x reg failed,value=0x%x, ret=%d",
                                (GPIO_BASE_ADDR + GPIO_LEVEL_CONFIG_REGADDR), WLAN_DEV2HOST_GPIO, ret);
            return -1;
        }

        oal_msleep(1);

        if(0 == gpio_get_value(g_board_info.wlan_wakeup_host))
        {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "pull gpio high failed!");
            hi1103_dump_gpio_regs();
            return -OAL_EFAIL;
        }

        ret = write_device_reg16(GPIO_BASE_ADDR + GPIO_LEVEL_CONFIG_REGADDR, 0x0);
        if(ret)
        {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "write 0x%x reg failed,value=0x%x, ret=%d",
                                (GPIO_BASE_ADDR + GPIO_LEVEL_CONFIG_REGADDR), WLAN_DEV2HOST_GPIO, ret);
            return -1;
        }

        oal_msleep(1);

        if(0 != gpio_get_value(g_board_info.wlan_wakeup_host))
        {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "pull gpio low failed!");
            hi1103_dump_gpio_regs();
            return -OAL_EFAIL;
        }

        oal_print_hi11xx_log(HI11XX_LOG_INFO, "check d2h wakeup io %d times ok", i + 1);

    }

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "check d2h wakeup io done");
    return OAL_SUCC;
}

int32 hi1103_check_host_wakeup_wlan(void)
{
    int32 i;
    int16 value;
    int32 ret;
    if(0 == g_board_info.host_wakeup_wlan)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "host_wakeup_wlan gpio is zero!");
        return -OAL_EIO;
    }

    value = 0;
    ret = read_device_reg16(GPIO_BASE_ADDR + GPIO_INOUT_CONFIG_REGADDR, &value);
    if(ret)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "read 0x%x reg failed, ret=%d", (GPIO_BASE_ADDR + GPIO_INOUT_CONFIG_REGADDR),ret);
        return -1;
    }

    /*����*/
    value &= (~WLAN_HOST2DEV_GPIO);

    ret = write_device_reg16(GPIO_BASE_ADDR + GPIO_INOUT_CONFIG_REGADDR, value);
    if(ret)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "write 0x%x reg failed,value=0x%x, ret=%d",
                            (GPIO_BASE_ADDR + GPIO_INOUT_CONFIG_REGADDR),value, ret);
        return -1;
    }

    for(i = 0 ; i < 2; i++)
    {

        gpio_direction_output(g_board_info.host_wakeup_wlan, GPIO_HIGHLEVEL);
        oal_msleep(1);

        value = 0;
        ret = read_device_reg16(GPIO_BASE_ADDR + GPIO_LEVEL_GET_REGADDR, &value);
        if(ret)
        {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "read 0x%x reg failed, ret=%d", (GPIO_BASE_ADDR + GPIO_LEVEL_GET_REGADDR),ret);
            return -1;
        }

        oal_print_hi11xx_log(HI11XX_LOG_DBG, "read 0x%x reg=0x%x", (GPIO_BASE_ADDR + GPIO_LEVEL_GET_REGADDR), value);

        value &= WLAN_HOST2DEV_GPIO;

        if(0 == value)
        {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "pull gpio high failed!");
            hi1103_dump_gpio_regs();
            return -OAL_EFAIL;
        }

        gpio_direction_output(g_board_info.host_wakeup_wlan, GPIO_LOWLEVEL);
        oal_msleep(1);

        value = 0;
        ret = read_device_reg16(GPIO_BASE_ADDR + GPIO_LEVEL_GET_REGADDR, &value);
        if(ret)
        {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "read 0x%x reg failed, ret=%d", (GPIO_BASE_ADDR + GPIO_LEVEL_GET_REGADDR),ret);
            return -1;
        }

        oal_print_hi11xx_log(HI11XX_LOG_DBG, "read 0x%x reg=0x%x", (GPIO_BASE_ADDR + GPIO_LEVEL_GET_REGADDR), value);

        value &= WLAN_HOST2DEV_GPIO;

        if(0 != value)
        {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "pull gpio low failed!");
            hi1103_dump_gpio_regs();
            return -OAL_EFAIL;
        }

        oal_print_hi11xx_log(HI11XX_LOG_INFO, "check h2d wakeup io %d times ok", i + 1);

    }

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "check h2d wakeup io done");
    return 0;
}

int32 hi1103_dev_io_test(void)
{
    int32 ret = 0;
    declare_time_cost_stru(cost);
    struct pm_drv_data *pm_data = pm_get_drvdata();
    if (NULL == pm_data)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "pm_data is NULL!");
        return -FAILURE;
    }

    if(!bfgx_is_shutdown())
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "bfgx is open, test abort!");
        return -OAL_ENODEV;
    }

    if(!wlan_is_shutdown())
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "wlan is open, test abort!");
        return -OAL_ENODEV;
    }

    oal_get_time_cost_start(cost);

    hcc_bus_wake_lock(pm_data->pst_wlan_pm_info->pst_bus);
    hcc_bus_lock(pm_data->pst_wlan_pm_info->pst_bus);
    /*power on wifi, need't download firmware*/
    hi1103_board_power_on(WLAN_POWER);

    hcc_bus_power_action(hcc_get_current_110x_bus(), HCC_BUS_POWER_PATCH_LOAD_PREPARE);
    ret = hcc_bus_reinit(pm_data->pst_wlan_pm_info->pst_bus);
    if (OAL_SUCC != ret)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "reinit bus %d failed, ret=%d",
                                               pm_data->pst_wlan_pm_info->pst_bus->bus_type, ret);
        hi1103_board_power_off(WLAN_POWER);
        hcc_bus_unlock(pm_data->pst_wlan_pm_info->pst_bus);
        hcc_bus_wake_unlock(pm_data->pst_wlan_pm_info->pst_bus);
        return -OAL_EFAIL;
    }

    wlan_pm_init_dev();/*PM,TBD*/

    ret = hi1103_check_device_ready();
    if(ret)
    {
        hi1103_board_power_off(WLAN_POWER);
        hcc_bus_unlock(pm_data->pst_wlan_pm_info->pst_bus);
        hcc_bus_wake_unlock(pm_data->pst_wlan_pm_info->pst_bus);
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "check_device_ready failed, ret=%d",
                                               ret);
        return ret;
    }

    /*check io*/
    ret = hi1103_check_host_wakeup_wlan();
    if(ret)
    {
        hi1103_board_power_off(WLAN_POWER);
        hcc_bus_unlock(pm_data->pst_wlan_pm_info->pst_bus);
        hcc_bus_wake_unlock(pm_data->pst_wlan_pm_info->pst_bus);
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "check_host_wakeup_wlan failed, ret=%d",
                                               ret);
        return ret;
    }

    ret = hi1103_check_wlan_wakeup_host();
    if(ret)
    {
        hi1103_board_power_off(WLAN_POWER);
        hcc_bus_unlock(pm_data->pst_wlan_pm_info->pst_bus);
        hcc_bus_wake_unlock(pm_data->pst_wlan_pm_info->pst_bus);
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "check_wlan_wakeup_host failed, ret=%d",
                                               ret);
        return ret;
    }

    hi1103_board_power_off(WLAN_POWER);

    hcc_bus_unlock(pm_data->pst_wlan_pm_info->pst_bus);
    hcc_bus_wake_unlock(pm_data->pst_wlan_pm_info->pst_bus);

    oal_get_time_cost_end(cost);
    oal_calc_time_cost_sub(cost);
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "hi1103 device io test cost %llu us", time_cost_var_sub(cost));
    return OAL_SUCC;
}

int32 hi1103_pcie_ip_test(oal_int32 test_count)
{
    int32 ret = 0;
    hcc_bus   *pst_bus, *old_bus;
    declare_time_cost_stru(cost);
    struct pm_drv_data *pm_data = pm_get_drvdata();
    if (NULL == pm_data)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "pm_data is NULL!");
        return -FAILURE;
    }

    if(OAL_TRUE != oal_pcie_110x_working_check())
    {
        /*��֧��PCIe,ֱ�ӷ��سɹ�*/
        oal_print_hi11xx_log(HI11XX_LOG_WARN, "do not support PCIe!");
        return OAL_SUCC;
    }

    if(!bfgx_is_shutdown())
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "bfgx is open, test abort!");
        return -OAL_ENODEV;
    }

    if(!wlan_is_shutdown())
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "wlan is open, test abort!");
        return -OAL_ENODEV;
    }

    old_bus = hcc_get_current_110x_bus();
    if(HCC_BUS_PCIE != old_bus->bus_type)
    {
        /*�����л���PCIE*/
        ret = hcc_switch_bus(HCC_CHIP_110X_DEV, HCC_BUS_PCIE);
        if(ret)
        {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "switch to PCIe failed, ret=%d", ret);
            return -OAL_ENODEV;
        }
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "switch to PCIe ok.");
    }
    else
    {
        old_bus = NULL;
    }

    oal_get_time_cost_start(cost);

    pst_bus = hcc_get_current_110x_bus();

    hcc_bus_wake_lock(pst_bus);
    hcc_bus_lock(pst_bus);
    /*power on wifi, need't download firmware*/
    hi1103_board_power_on(WLAN_POWER);

    hcc_bus_power_action(pst_bus, HCC_BUS_POWER_PATCH_LOAD_PREPARE);
    ret = hcc_bus_reinit(pst_bus);
    if (OAL_SUCC != ret)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "reinit bus %d failed, ret=%d",
                                               pst_bus->bus_type, ret);
        hi1103_board_power_off(WLAN_POWER);
        hcc_bus_unlock(pst_bus);
        hcc_bus_wake_unlock(pst_bus);
        return -OAL_EFAIL;
    }

    wlan_pm_init_dev();/*PM,TBD*/

    ret = hi1103_check_device_ready();
    if(ret)
    {
        hi1103_board_power_off(WLAN_POWER);
        hcc_bus_unlock(pst_bus);
        hcc_bus_wake_unlock(pst_bus);
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "check_device_ready failed, ret=%d",
                                               ret);
        return ret;
    }

    ret = oal_pcie_ip_factory_test(pst_bus, test_count);
    if(ret)
    {
        hi1103_board_power_off(WLAN_POWER);
        hcc_bus_unlock(pst_bus);
        hcc_bus_wake_unlock(pst_bus);
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "pcie_ip_factory_test failed, ret=%d",
                                               ret);
        return ret;
    }

    hi1103_board_power_off(WLAN_POWER);

    hcc_bus_unlock(pst_bus);
    hcc_bus_wake_unlock(pst_bus);

    if(NULL != old_bus)
    {
        ret = hcc_switch_bus(HCC_CHIP_110X_DEV, old_bus->bus_type);
        if(ret)
        {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "restore to bus %d failed, ret=%d", old_bus->bus_type, ret);
            return -OAL_ENODEV;
        }
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "resotre to bus %d ok.", old_bus->bus_type);
    }

    oal_get_time_cost_end(cost);
    oal_calc_time_cost_sub(cost);
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "hi1103 pcie ip test %llu us", time_cost_var_sub(cost));

    return OAL_SUCC;

}

EXPORT_SYMBOL(hi1103_pcie_ip_test);

EXPORT_SYMBOL(hi1103_dev_io_test);



#ifdef __cplusplus
    #if __cplusplus
            }
    #endif
#endif

