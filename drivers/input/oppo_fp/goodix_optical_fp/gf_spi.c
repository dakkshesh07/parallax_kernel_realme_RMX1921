/************************************************************************************
 ** File: - SDM660.LA.1.0\android\vendor\oppo_app\fingerprints_hal\drivers\goodix_fp\gf_spi.c
 ** VENDOR_EDIT
 ** Copyright (C), 2008-2017, OPPO Mobile Comm Corp., Ltd
 **
 ** Description:
 **      goodix fingerprint kernel device driver
 **
 ** Version: 1.0
 ** Date created: 16:20:11,12/07/2017
 ** Author: Ziqing.guo@Prd.BaseDrv
 ** TAG: BSP.Fingerprint.Basic
 **
 ** --------------------------- Revision History: --------------------------------
 **  <author>        <data>          <desc>
 **  Ziqing.guo      2017/07/12      create the file for goodix 5288
 **  Ziqing.guo      2017/08/29      fix the problem of failure after restarting fingerprintd
 **  Ziqing.guo      2017/08/31      add goodix 3268,5288
 **  Ziqing.guo      2017/09/11      add gf_cmd_wakelock
 **  Ran.Chen        2017/11/30      add vreg_step for goodix_fp
 **  Ran.Chen        2017/12/07      remove power_off in release for Power supply timing
 **  Ran.Chen        2018/01/29      modify for fp_id, Code refactoring
 **  Ran.Chen        2018/11/27      remove define MSM_DRM_ONSCREENFINGERPRINT_EVENT
 **  Ran.Chen        2018/12/15      modify for power off in ftm mode (for SDM855)
 **  Ran.Chen        2019/03/17      remove power off in ftm mode (for SDM855)
 **  Bangxiong.Wu    2019/05/10      add for SM7150 (MSM_19031 MSM_19331)
 **  Ran.Chen        2019/05/09      add for GF_IOC_CLEAN_TOUCH_FLAG
 ************************************************************************************/
#define pr_fmt(fmt)    KBUILD_MODNAME ": " fmt

#include <linux/input.h>
#include <net/sock.h>
#include <linux/of_gpio.h>
#include <linux/msm_drm_notify.h>
#include "gf_spi.h"
#include <soc/oppo/oppo_project.h>
#if defined(CONFIG_OPPO_FINGERPRINT_GOODIX_SPI)
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#elif defined(CONFIG_OPPO_FINGERPRINT_GOODIX_PLATFORM)
#include <linux/platform_device.h>
#endif
#include <soc/oppo/boot_mode.h>

#define VER_MAJOR   1
#define VER_MINOR   2
#define PATCH_LEVEL 9

#define WAKELOCK_HOLD_TIME 500 /* in ms */

#define GF_SPIDEV_NAME      "goodix,goodix_fp"
#define GF_DEV_NAME         "goodix_fp"
#define GF_CHRDEV_NAME      "goodix_fp_spi"
#define GF_CLASS_NAME       "goodix_fp"
#define MAX_MSGSIZE         32
#define GF_MAX_DEVS         32	/* ... up to 256 */

#if (!defined USED_GPIO_PWR) || (defined CONFIG_19081_PWR)
struct vreg_config {
    char *name;
    unsigned long vmin;
    unsigned long vmax;
    int ua_load;
};

#ifdef CONFIG_19081_PWR
static const struct vreg_config vreg_conf[] = {
    { "ldo7", 3300000UL, 3300000UL, 150000, },
};
#else
static const struct vreg_config vreg_conf[] = {
    { "ldo5", 2960000UL, 2960000UL, 150000, },
};
#endif

static struct fp_underscreen_info fp_tpinfo;
static unsigned int lasttouchmode = 0;

static int gf_dev_major;

static DECLARE_BITMAP(minors, GF_MAX_DEVS);
static LIST_HEAD(device_list);
static DEFINE_MUTEX(device_list_lock);
static struct wakeup_source fp_wakelock;
static struct gf_dev gf;
static int pid = -1;
static struct sock *nl_sk;

static inline int gf_sendnlmsg(const char *message)
{
    struct nlmsghdr *nlh;
    struct sk_buff *skb;
    int rc;

    if (!message)
        return -EINVAL;

    if (pid < 1) {
        pr_info("cannot send msg... no receiver\n");
        return 0;
    }

    skb = nlmsg_new(MAX_MSGSIZE, GFP_KERNEL);
    if (!skb)
        return -ENOMEM;

    nlh = nlmsg_put(skb, 0, 0, 0, MAX_MSGSIZE, 0);
    NETLINK_CB(skb).portid = 0;
    NETLINK_CB(skb).dst_group = 0;
    strlcpy(nlmsg_data(nlh), message, MAX_MSGSIZE);

    rc = netlink_unicast(nl_sk, skb, pid, MSG_DONTWAIT);
    if (rc < 0)
        pr_err("failed to send msg to userspace. rc = %d\n", rc);

    return rc;
}

static inline void gf_netlink_rcv(struct sk_buff *skb)
{
    struct nlmsghdr *nlh;
    skb = skb_get(skb);

    if (skb->len >= NLMSG_HDRLEN) {
        nlh = nlmsg_hdr(skb);
        pid = nlh->nlmsg_pid;
        if (nlh->nlmsg_flags & NLM_F_ACK)
            netlink_ack(skb, nlh, 0);
        kfree_skb(skb);
    }

}


static inline int gf_netlink_init(void)
{
#ifdef CONFIG_OPPO_FINGERPRINT_GOODIX_NETLINK
    struct netlink_kernel_cfg cfg = {
        .input = gf_netlink_rcv,
    };

    nl_sk = netlink_kernel_create(&init_net, NETLINK_TEST, &cfg);

    if(!nl_sk){
        pr_err("goodix_fp: cannot create netlink socket\n");
        return -EIO;
    }
#endif
    return 0;
}

static inline void gf_netlink_exit(void)
{
#ifdef CONFIG_OPPO_FINGERPRINT_GOODIX_NETLINK
    if(nl_sk != NULL){
        netlink_kernel_release(nl_sk);
        nl_sk = NULL;
    }
#endif
}

static inline int vreg_setup(struct gf_dev *goodix_fp, const char *name,
    bool enable)
{
    size_t i;
    int rc;
    struct regulator *vreg;
    struct device *dev = goodix_fp->dev;
    if (NULL == name) {
        pr_err("name is NULL\n");
        return -EINVAL;
    }
    pr_debug("Regulator %s vreg_setup,enable=%d \n", name, enable);
    for (i = 0; i < ARRAY_SIZE(goodix_fp->vreg); i++) {
        const char *n = vreg_conf[i].name;
        if (!strncmp(n, name, strlen(n)))
            goto found;
    }
    pr_err("Regulator %s not found\n", name);
    return -EINVAL;
found:
    vreg = goodix_fp->vreg[i];
    if (enable) {
        if (!vreg) {
            vreg = regulator_get(dev, name);
            if (IS_ERR(vreg)) {
                pr_err("Unable to get  %s\n", name);
                return PTR_ERR(vreg);
            }
        }
        if (regulator_count_voltages(vreg) > 0) {
            rc = regulator_set_voltage(vreg, vreg_conf[i].vmin,
                    vreg_conf[i].vmax);
            if (rc)
                pr_err("Unable to set voltage on %s, %d\n",
                    name, rc);
        }
        rc = regulator_set_load(vreg, vreg_conf[i].ua_load);
        if (rc < 0)
            pr_err("Unable to set current on %s, %d\n",
                    name, rc);
        rc = regulator_enable(vreg);
        if (rc) {
            pr_err("error enabling %s: %d\n", name, rc);
            regulator_put(vreg);
            vreg = NULL;
        }
        goodix_fp->vreg[i] = vreg;
    } else {
        if (vreg) {
            if (regulator_is_enabled(vreg)) {
                regulator_disable(vreg);
                pr_debug("disabled %s\n", name);
            }
            regulator_put(vreg);
            goodix_fp->vreg[i] = NULL;
        }
        pr_err("disable vreg is null \n");
        rc = 0;
    }
    return rc;
}
#endif

static inline int gf_parse_dts(struct gf_dev* gf_dev)
{
    int rc = 0;
    struct device *dev = gf_dev->dev;
    struct device_node *np = dev->of_node;

    gf_dev->reset_gpio = of_get_named_gpio(np, "goodix,gpio_reset", 0);
    if (gf_dev->reset_gpio < 0) {
        pr_err("falied to get reset gpio!\n");
        return gf_dev->reset_gpio;
    }

    rc = devm_gpio_request(dev, gf_dev->reset_gpio, "goodix_reset");
    if (rc) {
        pr_err("failed to request reset gpio, rc = %d\n", rc);
        goto err_reset;
    }
    gpio_direction_output(gf_dev->reset_gpio, 0);

    gf_dev->irq_gpio = of_get_named_gpio(np, "goodix,gpio_irq", 0);
    if (gf_dev->irq_gpio < 0) {
        pr_err("falied to get irq gpio!\n");
        return gf_dev->irq_gpio;
    }

    rc = devm_gpio_request(dev, gf_dev->irq_gpio, "goodix_irq");
    if (rc) {
        pr_err("failed to request irq gpio, rc = %d\n", rc);
        goto err_irq;
    }
    gpio_direction_input(gf_dev->irq_gpio);

#if defined(USED_GPIO_PWR)

    gf_dev->pwr_gpio = of_get_named_gpio(np, "goodix,goodix_pwr", 0);
        pr_debug("end of_get_named_gpio  goodix_pwr!\n");
    if (gf_dev->pwr_gpio < 0) {
        pr_err("falied to get goodix_pwr gpio!\n");
        return gf_dev->pwr_gpio;
    }

    rc = devm_gpio_request(dev, gf_dev->pwr_gpio, "goodix_pwr");
    if (rc) {
        pr_err("failed to request goodix_pwr gpio, rc = %d\n", rc);
        goto err_pwr;
    }
    gpio_direction_output(gf_dev->pwr_gpio, 0);
    pr_debug("set goodix_pwr output 0 \n");

#elif defined(PROJECT_19651)
    pr_debug("begin of_get_named_gpio  goodix_vdd for 19651!\n");
    gf_dev->vdd_gpio = of_get_named_gpio(np, "goodix,goodix_vdd", 0);
        pr_debug("end of_get_named_gpio  goodix_vdd for 19651!\n");
    if (gf_dev->vdd_gpio < 0) {
        pr_err("falied to get goodix_vdd gpio!\n");
        return gf_dev->vdd_gpio;
    }

    rc = devm_gpio_request(dev, gf_dev->vdd_gpio, "goodix_vdd");
    if (rc) {
        pr_err("failed to request goodix_vdd gpio, rc = %d\n", rc);
        devm_gpio_free(dev, gf_dev->vdd_gpio);
    }
    gpio_direction_output(gf_dev->vdd_gpio, 0);
    pr_debug("set goodix_vdd output 0 \n");

    gf_dev->pwr_gpio = of_get_named_gpio(np, "goodix,goodix_pwr", 0);
        pr_debug("end of_get_named_gpio  goodix_pwr for 19651!\n");
    if (gf_dev->pwr_gpio < 0) {
        pr_err("falied to get goodix_pwr gpio!\n");
        return gf_dev->pwr_gpio;
    }

    rc = devm_gpio_request(dev, gf_dev->pwr_gpio, "goodix_pwr");
    if (rc) {
        pr_err("failed to request goodix_pwr gpio, rc = %d\n", rc);
        devm_gpio_free(dev, gf_dev->pwr_gpio);
    }
    gpio_direction_output(gf_dev->pwr_gpio, 0);
    pr_debug("set goodix_pwr output 0 \n");
#endif

pr_debug("end gf_parse_dts !\n");

#if defined(USED_GPIO_PWR)
err_pwr:
    devm_gpio_free(dev, gf_dev->pwr_gpio);
#endif

err_irq:
    devm_gpio_free(dev, gf_dev->reset_gpio);
err_reset:
    return rc;
}

static inline void gf_cleanup(struct gf_dev *gf_dev)
{
    pr_debug("[info] %s\n",__func__);
    if (gpio_is_valid(gf_dev->irq_gpio))
    {
        gpio_free(gf_dev->irq_gpio);
        pr_debug("remove irq_gpio success\n");
    }
    if (gpio_is_valid(gf_dev->reset_gpio))
    {
        gpio_free(gf_dev->reset_gpio);
        pr_debug("remove reset_gpio success\n");
    }
#if defined(USED_GPIO_PWR)
    if (gpio_is_valid(gf_dev->pwr_gpio))
    {
        gpio_free(gf_dev->pwr_gpio);
        pr_debug("remove pwr_gpio success\n");
    }

#elif defined(PROJECT_19651)
    if (gpio_is_valid(gf_dev->vdd_gpio))
    {
        gpio_free(gf_dev->vdd_gpio);
        pr_debug("remove vdd_gpio success\n");
    }
    if (gpio_is_valid(gf_dev->pwr_gpio))
    {
        gpio_free(gf_dev->pwr_gpio);
        pr_debug("remove pwr_gpio success\n");
    }
#endif
}

static inline int gf_set_power(struct gf_dev *gf_dev, bool enabled)
{
    int rc = 0;

/*power on auto during boot, no need fp driver power on*/
#if defined(AUTO_PWR)
    pr_debug("[%s] power on auto, no need power on again\n", __func__);
    return rc;
#endif
    pr_debug("---- power on ok ----\n");
#if defined(USED_GPIO_PWR)
    gpio_set_value(gf_dev->pwr_gpio, enabled ? 1 : 0);
    pr_debug("set pwe_gpio 1\n");
#elif defined(PROJECT_19651)
    gpio_set_value(gf_dev->pwr_gpio, enabled ? 1 : 0);
    msleep(5);
    gpio_set_value(gf_dev->vdd_gpio, enabled ? 1 : 0);
    pr_debug("set pwe_gpio %s for 19651 \n",
        enabled ? "1" : "0");
#else 
    rc = vreg_setup(gf_dev, "ldo5", enabled);
#endif
#ifdef CONFIG_19081_PWR
    rc = vreg_setup(gf_dev, "ldo7", enabled);
#endif
    msleep(30);
    return rc;
}

static inline int gf_hw_reset(struct gf_dev *gf_dev, unsigned int delay_ms)
{
    if(gf_dev == NULL) {
        pr_info("Input buff is NULL.\n");
        return -1;
    }

    if (gf_dev->vreg[0]) {
        int voltage = regulator_get_voltage(gf_dev->vreg[0]);
        int enable = regulator_is_enabled(gf_dev->vreg[0]);
        if (enable) {
            pr_debug("goodix fingerprint power LDO: %d mV, enable=%d\n", voltage/1000, enable);
        } else {
            pr_debug("goodix fingerprint power is disable.\n");
            gf_set_power(gf_dev, true);
        }
    } else {
        pr_debug("goodix fingerprint power is NULL.\n");
        gf_set_power(gf_dev, true);
    }

    //gpio_direction_output(gf_dev->reset_gpio, 1);
    gpio_set_value(gf_dev->reset_gpio, 0);
    mdelay(5);
    gpio_set_value(gf_dev->reset_gpio, 1);
    mdelay(delay_ms);
    return 0;
}


static inline void gf_enable_irq(struct gf_dev *gf_dev)
{
    if (!gf_dev->irq_enabled) {
        enable_irq(gf_dev->irq);
        gf_dev->irq_enabled = 1;
    } else {
        pr_warn("IRQ has been enabled.\n");
    }
}

static inline void gf_disable_irq(struct gf_dev *gf_dev)
{
    if (gf_dev->irq_enabled) {
        gf_dev->irq_enabled = 0;
        disable_irq(gf_dev->irq);
    } else {
        pr_warn("IRQ has been disabled.\n");
    }
}

#ifdef CONFIG_OPPO_FINGERPRINT_GOODIX_CLK_CTRL
static long spi_clk_max_rate(struct clk *clk, unsigned long rate)
{
    long lowest_available, nearest_low, step_size, cur;
    long step_direction = -1;
    long guess = rate;
    int max_steps = 10;

    cur = clk_round_rate(clk, rate);
    if (cur == rate)
        return rate;

    /* if we got here then: cur > rate */
    lowest_available = clk_round_rate(clk, 0);
    if (lowest_available > rate)
        return -EINVAL;

    step_size = (rate - lowest_available) >> 1;
    nearest_low = lowest_available;

    while (max_steps-- && step_size) {
        guess += step_size * step_direction;
        cur = clk_round_rate(clk, guess);

        if ((cur < rate) && (cur > nearest_low))
            nearest_low = cur;

        /*
         * if we stepped too far, then start stepping in the other
         * direction with half the step size
         */
        if (((cur > rate) && (step_direction > 0))
                || ((cur < rate) && (step_direction < 0))) {
            step_direction = -step_direction;
            step_size >>= 1;
        }
    }
    return nearest_low;
}

static void spi_clock_set(struct gf_dev *gf_dev, int speed)
{
    long rate;
    int rc;

    rate = spi_clk_max_rate(gf_dev->core_clk, speed);
    if (rate < 0) {
        pr_info("%s: no match found for requested clock frequency:%d",
                __func__, speed);
        return;
    }

    rc = clk_set_rate(gf_dev->core_clk, rate);
}

static int gfspi_ioctl_clk_init(struct gf_dev *data)
{
    data->clk_enabled = 0;
    data->core_clk = clk_get(data->dev, "core_clk");
    if (IS_ERR_OR_NULL(data->core_clk)) {
        pr_err("%s: fail to get core_clk\n", __func__);
        return -EPERM;
    }
    data->iface_clk = clk_get(data->dev, "iface_clk");
    if (IS_ERR_OR_NULL(data->iface_clk)) {
        pr_err("%s: fail to get iface_clk\n", __func__);
        clk_put(data->core_clk);
        data->core_clk = NULL;
        return -ENOENT;
    }
    return 0;
}

static int gfspi_ioctl_clk_enable(struct gf_dev *data)
{
    int err;

    if (data->clk_enabled)
        return 0;

    err = clk_prepare_enable(data->core_clk);
    if (err) {
        pr_debug("%s: fail to enable core_clk\n", __func__);
        return -EPERM;
    }

    err = clk_prepare_enable(data->iface_clk);
    if (err) {
        pr_debug("%s: fail to enable iface_clk\n", __func__);
        clk_disable_unprepare(data->core_clk);
        return -ENOENT;
    }

    data->clk_enabled = 1;

    return 0;
}

static int gfspi_ioctl_clk_disable(struct gf_dev *data)
{
    if (!data->clk_enabled)
        return 0;

    clk_disable_unprepare(data->core_clk);
    clk_disable_unprepare(data->iface_clk);
    data->clk_enabled = 0;

    return 0;
}

static int gfspi_ioctl_clk_uninit(struct gf_dev *data)
{
    if (data->clk_enabled)
        gfspi_ioctl_clk_disable(data);

    if (!IS_ERR_OR_NULL(data->core_clk)) {
        clk_put(data->core_clk);
        data->core_clk = NULL;
    }

    if (!IS_ERR_OR_NULL(data->iface_clk)) {
        clk_put(data->iface_clk);
        data->iface_clk = NULL;
    }

    return 0;
}
#endif

static inline irqreturn_t gf_irq(int irq, void *handle)
{
    struct gf_dev *gf_dev = &gf;
    char msg = GF_NET_EVENT_IRQ;
    __pm_wakeup_event(&fp_wakelock, msecs_to_jiffies(WAKELOCK_HOLD_TIME));
    gf_sendnlmsg(&msg);
    kill_fasync(&gf_dev->async, SIGIO, POLL_IN);

    return IRQ_HANDLED;
}

static inline int irq_setup(struct gf_dev *gf_dev)
{
    int status;

    gf_dev->irq = gpio_to_irq(gf_dev->irq_gpio);
    status = request_threaded_irq(gf_dev->irq, NULL, gf_irq,
            IRQF_TRIGGER_RISING | IRQF_ONESHOT, "gf", gf_dev);

    if (status) {
        pr_err("failed to request IRQ:%d\n", gf_dev->irq);
        return status;
    }
    enable_irq_wake(gf_dev->irq);
    gf_dev->irq_enabled = 1;

    return status;
}

static inline void irq_cleanup(struct gf_dev *gf_dev)
{
    gf_dev->irq_enabled = 0;
    disable_irq(gf_dev->irq);
    disable_irq_wake(gf_dev->irq);
    free_irq(gf_dev->irq, gf_dev);//need modify
}

static inline long gf_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct gf_dev *gf_dev = &gf;
    int retval = 0;
    u8 netlink_route = NETLINK_TEST;
    struct gf_ioc_chip_info info;

    if (_IOC_TYPE(cmd) != GF_IOC_MAGIC) {
        return -ENODEV;
    }

    if (_IOC_DIR(cmd) & _IOC_READ) {
        retval = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
    } else if (_IOC_DIR(cmd) & _IOC_WRITE) {
        retval = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
    }
    if (retval) {
        return -EFAULT;
    }

    if (gf_dev->device_available == 0) {
        switch (cmd) {
            case GF_IOC_ENABLE_POWER:
                pr_debug("power cmd\n");
                break;
            case GF_IOC_DISABLE_POWER:
                pr_debug("power cmd\n");
                break;
            default:
                pr_debug("Sensor is power off currently. \n");
                return -ENODEV;
        }
    }

    switch (cmd) {
        case GF_IOC_INIT:
            if (copy_to_user((void __user *)arg, (void *)&netlink_route, sizeof(u8)))
                retval = -EFAULT;
            break;
        case GF_IOC_DISABLE_IRQ:
            gf_disable_irq(gf_dev);
            break;
        case GF_IOC_ENABLE_IRQ:
            gf_enable_irq(gf_dev);
            break;
        case GF_IOC_RESET:
            gf_hw_reset(gf_dev, 3);
            break;
        case GF_IOC_ENABLE_SPI_CLK:
#ifdef CONFIG_OPPO_FINGERPRINT_GOODIX_CLK_CTRL
            gfspi_ioctl_clk_enable(gf_dev);
#endif
            break;
        case GF_IOC_DISABLE_SPI_CLK:
#ifdef CONFIG_OPPO_FINGERPRINT_GOODIX_CLK_CTRL
            gfspi_ioctl_clk_disable(gf_dev);
#endif
            break;
        case GF_IOC_ENABLE_POWER:
            if (gf_dev->device_available == 1)
                break;
            gf_set_power(gf_dev, true);
            gf_dev->device_available = 1;
            break;
        case GF_IOC_DISABLE_POWER:
            if (gf_dev->device_available == 0)
                break;
            gf_set_power(gf_dev, false);
            gf_dev->device_available = 0;
            break;
        case GF_IOC_REMOVE:
            irq_cleanup(gf_dev);
            gf_cleanup(gf_dev);
            break;
        case GF_IOC_CHIP_INFO:
            if (copy_from_user(&info, (struct gf_ioc_chip_info *)arg, sizeof(struct gf_ioc_chip_info))) {
                retval = -EFAULT;
            } else {
                pr_info("vendor_id : 0x%x\n", info.vendor_id);
                pr_info("mode : 0x%x\n", info.mode);
                pr_info("operation: 0x%x\n", info.operation);
            }
            break;
        case GF_IOC_CLEAN_TOUCH_FLAG:
            lasttouchmode = 0;
            break;
        default:
            pr_warn("unsupport cmd:0x%x\n", cmd);
            break;
    }

    return retval;
}

#ifdef CONFIG_COMPAT
static inline long gf_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    return gf_ioctl(filp, cmd, (unsigned long)compat_ptr(arg));
}
#endif /*CONFIG_COMPAT*/


static inline int gf_open(struct inode *inode, struct file *filp)
{
    struct gf_dev *gf_dev = &gf;
    int status = -ENXIO;

    mutex_lock(&device_list_lock);

    list_for_each_entry(gf_dev, &device_list, device_entry) {
        if (gf_dev->devt == inode->i_rdev) {
            status = 0;
            break;
        }
    }

    if (status == 0) {
        gf_dev->users++;
        filp->private_data = gf_dev;
        nonseekable_open(inode, filp);
        if (gf_dev->users == 1) {
            status = gf_parse_dts(gf_dev);
            if (status)
                return status;

            status = irq_setup(gf_dev);
            if (status)
                gf_cleanup(gf_dev);
        }
        
    } else {
        pr_info("No device for minor %d\n", iminor(inode));
    }

    mutex_unlock(&device_list_lock);
    /* perform HW reset */
    gf_hw_reset(gf_dev, 3);
    return status;
}

static inline int gf_fasync(int fd, struct file *filp, int mode)
{
    struct gf_dev *gf_dev = filp->private_data;
    return fasync_helper(fd, filp, mode, &gf_dev->async);
}

static inline int gf_release(struct inode *inode, struct file *filp)
{
    struct gf_dev *gf_dev = &gf;
    int status = 0;

    mutex_lock(&device_list_lock);
    gf_dev = filp->private_data;
    filp->private_data = NULL;

    /*last close?? */
    gf_dev->users--;
    if (!gf_dev->users) {
        irq_cleanup(gf_dev);
        gf_cleanup(gf_dev);
        /*power off the sensor*/
        gf_dev->device_available = 0;
    }
    mutex_unlock(&device_list_lock);
    return status;
}

static const struct file_operations gf_fops = {
    .owner = THIS_MODULE,
    /* REVISIT switch to aio primitives, so that userspace
     * gets more complete API coverage.  It'll simplify things
     * too, except for the locking.
     */
    .unlocked_ioctl = gf_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl = gf_compat_ioctl,
#endif /*CONFIG_COMPAT*/
    .open = gf_open,
    .release = gf_release,
    .fasync = gf_fasync,
};

static int goodix_fb_state_chg_callback(struct notifier_block *nb,
        unsigned long val, void *data)
{
    struct gf_dev *gf_dev;
    struct msm_drm_notifier *evdata = data;
    unsigned int blank;
    char msg = 0;
    int retval = 0;

    gf_dev = container_of(nb, struct gf_dev, notifier);

    if (val == MSM_DRM_ONSCREENFINGERPRINT_EVENT) {
        uint8_t op_mode = 0x0;
        op_mode = *(uint8_t *)evdata->data;

        switch (op_mode) {
            case 1:
                msg = GF_NET_EVENT_UI_READY;
                gf_sendnlmsg(&msg);
                break;
        }
        return retval;
    }

    if (evdata && evdata->data && val == MSM_DRM_EARLY_EVENT_BLANK && gf_dev) {
        blank = *(int *)(evdata->data);
        switch (blank) {
            case MSM_DRM_BLANK_POWERDOWN:
                if (gf_dev->device_available == 1) {
                    gf_dev->fb_black = 1;
                    msg = GF_NET_EVENT_FB_BLACK;
                    gf_sendnlmsg(&msg);
                    kill_fasync(&gf_dev->async, SIGIO, POLL_IN);
                }
                break;
            case MSM_DRM_BLANK_UNBLANK:
                if (gf_dev->device_available == 1) {
                    gf_dev->fb_black = 0;
                    msg = GF_NET_EVENT_FB_UNBLACK;
                    gf_sendnlmsg(&msg);
                    kill_fasync(&gf_dev->async, SIGIO, POLL_IN);
                }
                break;
            default:
                pr_debug("%s defalut\n", __func__);
                break;
        }
    }
    return NOTIFY_OK;
}

static struct notifier_block goodix_noti_block = {
    .notifier_call = goodix_fb_state_chg_callback,
};

int gf_opticalfp_irq_handler(struct fp_underscreen_info* tp_info)
{
    char msg = 0;
    fp_tpinfo = *tp_info;
    if(tp_info->touch_state== lasttouchmode){
        return IRQ_HANDLED;
    }
    __pm_wakeup_event(&fp_wakelock, msecs_to_jiffies(WAKELOCK_HOLD_TIME));
    if (1 == tp_info->touch_state) {
        msg = GF_NET_EVENT_TP_TOUCHDOWN;
        gf_sendnlmsg(&msg);
        lasttouchmode = tp_info->touch_state;
    } else {
        msg = GF_NET_EVENT_TP_TOUCHUP;
        gf_sendnlmsg(&msg);
        lasttouchmode = tp_info->touch_state;
    }

    return IRQ_HANDLED;
}


static struct class *gf_class;
#if defined(CONFIG_OPPO_FINGERPRINT_GOODIX_SPI)
static int gf_probe(struct spi_device *dev)
#elif defined(CONFIG_OPPO_FINGERPRINT_GOODIX_PLATFORM)
static int gf_probe(struct platform_device *dev)
#endif
{
    struct gf_dev *gf_dev = &gf;
    int status = -EINVAL;
    unsigned long minor;
    int boot_mode = 0;
    /* Initialize the driver data */
    INIT_LIST_HEAD(&gf_dev->device_entry);
    gf_dev->dev = &dev->dev;
    gf_dev->irq_gpio = -EINVAL;
    gf_dev->reset_gpio = -EINVAL;
    gf_dev->pwr_gpio = -EINVAL;
    gf_dev->device_available = 0;
    gf_dev->fb_black = 0;

    if((FP_GOODIX_3268 != get_fpsensor_type())
            && (FP_GOODIX_5288 != get_fpsensor_type())
            && (FP_GOODIX_5228 != get_fpsensor_type())
            && (FP_GOODIX_OPTICAL_95 != get_fpsensor_type())){
        pr_err("%s, found not goodix sensor\n", __func__);
        status = -EINVAL;
        goto error_hw;
    }

    /* If we can allocate a minor number, hook up this device.
     * Reusing minors is fine so long as udev or mdev is working.
     */
    mutex_lock(&device_list_lock);
    minor = find_first_zero_bit(minors, GF_MAX_DEVS);
    if (minor < GF_MAX_DEVS) {
        struct device *dev;

        gf_dev->devt = MKDEV(gf_dev_major, minor);
        dev = device_create(gf_class, gf_dev->dev, gf_dev->devt,
                gf_dev, GF_DEV_NAME);
        status = IS_ERR(dev) ? PTR_ERR(dev) : 0;
    } else {
        dev_dbg(gf_dev->dev, "no minor number available!\n");
        status = -ENODEV;
        mutex_unlock(&device_list_lock);
        goto error_hw;
    }

    if (status == 0) {
        set_bit(minor, minors);
        list_add(&gf_dev->device_entry, &device_list);
    } else {
        gf_dev->devt = 0;
    }
    mutex_unlock(&device_list_lock);

#ifdef CONFIG_OPPO_FINGERPRINT_GOODIX_CLK_CTRL
    /* Enable spi clock */
    if (gfspi_ioctl_clk_init(gf_dev))
        goto gfspi_probe_clk_init_failed;

    if (gfspi_ioctl_clk_enable(gf_dev))
        goto gfspi_probe_clk_enable_failed;

    spi_clock_set(gf_dev, 1000000);
#endif

    gf_dev->notifier = goodix_noti_block;
#if defined(CONFIG_DRM_MSM)
    status = msm_drm_register_client(&gf_dev->notifier);
    if (status == -1) {
        return status;
    }
#endif
    wakeup_source_init(&fp_wakelock, "fp_wakelock");
    pr_debug("version V%d.%d.%02d\n", VER_MAJOR, VER_MINOR, PATCH_LEVEL);

    return status;

#ifdef CONFIG_OPPO_FINGERPRINT_GOODIX_CLK_CTRL
gfspi_probe_clk_enable_failed:
    gfspi_ioctl_clk_uninit(gf_dev);
gfspi_probe_clk_init_failed:
#endif

error_hw:
    gf_dev->device_available = 0;
    boot_mode = get_boot_mode();
    if (MSM_BOOT_MODE__FACTORY == boot_mode)
        gf_set_power(gf_dev, false);

    return status;
}

#if defined(CONFIG_OPPO_FINGERPRINT_GOODIX_SPI)
static inline int gf_remove(struct spi_device *dev)
#elif defined(CONFIG_OPPO_FINGERPRINT_GOODIX_PLATFORM)
static int gf_remove(struct platform_device *dev)
#endif
{
    struct gf_dev *gf_dev = dev_get_drvdata(&dev->dev);
    wakeup_source_trash(&fp_wakelock);

    msm_drm_unregister_client(&gf_dev->notifier);
    if (gf_dev->input)
        input_unregister_device(gf_dev->input);
    input_free_device(gf_dev->input);

    /* prevent new opens */
    mutex_lock(&device_list_lock);
    list_del(&gf_dev->device_entry);
    device_destroy(gf_class, gf_dev->devt);
    clear_bit(MINOR(gf_dev->devt), minors);
    mutex_unlock(&device_list_lock);

    return 0;
}

static struct of_device_id gx_match_table[] = {
    { .compatible = GF_SPIDEV_NAME },
    {},
};

#if defined(CONFIG_OPPO_FINGERPRINT_GOODIX_SPI)
static struct spi_driver gf_driver = {
#elif defined(CONFIG_OPPO_FINGERPRINT_GOODIX_PLATFORM)
static struct platform_driver gf_driver = {
#endif
    .driver = {
        .name = GF_DEV_NAME,
        .owner = THIS_MODULE,
        .of_match_table = gx_match_table,
        .probe_type = PROBE_PREFER_ASYNCHRONOUS,
    },
    .probe = gf_probe,
    .remove = gf_remove,
};

static inline int __init gf_init(void)
{
    int rc;

    /* Allocate chardev region and assign major number */
    BUILD_BUG_ON(GF_MAX_DEVS > 256);
    rc = __register_chrdev(gf_dev_major, 0, GF_MAX_DEVS, GF_CHRDEV_NAME,
                   &gf_fops);
    if (rc < 0) {
        pr_warn("failed to register char device\n");
        return rc;
    }
    gf_dev_major = rc;

    /* Create class */
    gf_class = class_create(THIS_MODULE, GF_CLASS_NAME);
    if (IS_ERR(gf_class)) {
        pr_warn("failed to create class.\n");
        rc = PTR_ERR(gf_class);
        goto error_class;
    }
#if defined(CONFIG_OPPO_FINGERPRINT_GOODIX_PLATFORM)
    rc = platform_driver_register(&gf_driver);
#elif defined(CONFIG_OPPO_FINGERPRINT_GOODIX_SPI)
    rc = spi_register_driver(&gf_driver);
#endif
    if (rc < 0) {
        pr_warn("failed to register driver\n");
        goto error_register;
    }

    rc = gf_netlink_init();
    if (rc < 0) {
        pr_warn("failed to initialize netlink\n");
        goto error_netlink;
    }
    pr_debug("initialization successfully done\n");
    return 0;

error_netlink:
#if defined(CONFIG_OPPO_FINGERPRINT_GOODIX_PLATFORM)
        platform_driver_unregister(&gf_driver);
#elif defined(CONFIG_OPPO_FINGERPRINT_GOODIX_SPI)
        spi_unregister_driver(&gf_driver);
#endif
error_register:
    class_destroy(gf_class);
error_class:
    unregister_chrdev(gf_dev_major, GF_CHRDEV_NAME);
    return rc;
}

static inline void __exit gf_exit(void)
{
    gf_netlink_exit();
#if defined(CONFIG_OPPO_FINGERPRINT_GOODIX_PLATFORM)
    platform_driver_unregister(&gf_driver);
#elif defined(CONFIG_OPPO_FINGERPRINT_GOODIX_SPI)
    spi_unregister_driver(&gf_driver);
#endif
    class_destroy(gf_class);
    unregister_chrdev(gf_dev_major, gf_driver.driver.name);
}

EXPORT_SYMBOL(gf_opticalfp_irq_handler);
late_initcall(gf_init);
module_exit(gf_exit);

MODULE_AUTHOR("Jiangtao Yi, <yijiangtao@goodix.com>");
MODULE_AUTHOR("Jandy Gou, <gouqingsong@goodix.com>");
MODULE_DESCRIPTION("goodix fingerprint sensor device driver");
MODULE_LICENSE("GPL");
