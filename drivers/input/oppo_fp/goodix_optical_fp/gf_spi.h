/*
 * driver definition for sensor driver
 *
 * Coypright (c) 2017 Goodix
 */
#ifndef __GF_SPI_H
#define __GF_SPI_H

#include <linux/types.h>
#include <linux/notifier.h>
#include "../oppo_fp_common/oppo_fp_common.h"

/**********************************************************/
enum FP_MODE{
	GF_IMAGE_MODE = 0,
	GF_KEY_MODE,
	GF_SLEEP_MODE,
	GF_FF_MODE,
	GF_DEBUG_MODE = 0x56
};

#if defined(CONFIG_OPPO_FINGERPRINT_GOODIX_NAV_EVENT)
#define GF_NAV_INPUT_UP			KEY_UP
#define GF_NAV_INPUT_DOWN		KEY_DOWN
#define GF_NAV_INPUT_LEFT		KEY_LEFT
#define GF_NAV_INPUT_RIGHT		KEY_RIGHT
#define GF_NAV_INPUT_CLICK		KEY_VOLUMEDOWN
#define GF_NAV_INPUT_DOUBLE_CLICK	KEY_VOLUMEUP
#define GF_NAV_INPUT_LONG_PRESS		KEY_SEARCH
#define GF_NAV_INPUT_HEAVY		KEY_CHAT
#endif

#define GF_KEY_INPUT_HOME		KEY_HOME
#define GF_KEY_INPUT_MENU		KEY_MENU
#define GF_KEY_INPUT_BACK		KEY_BACK
#define GF_KEY_INPUT_POWER		KEY_POWER
#define GF_KEY_INPUT_CAMERA		KEY_CAMERA

#if defined(CONFIG_OPPO_FINGERPRINT_GOODIX_NAV_EVENT)
typedef enum gf_nav_event {
	GF_NAV_NONE = 0,
	GF_NAV_FINGER_UP,
	GF_NAV_FINGER_DOWN,
	GF_NAV_UP,
	GF_NAV_DOWN,
	GF_NAV_LEFT,
	GF_NAV_RIGHT,
	GF_NAV_CLICK,
	GF_NAV_HEAVY,
	GF_NAV_LONG_PRESS,
	GF_NAV_DOUBLE_CLICK,
} gf_nav_event_t;
#endif

typedef enum gf_key_event {
	GF_KEY_NONE = 0,
	GF_KEY_HOME,
	GF_KEY_POWER,
	GF_KEY_MENU,
	GF_KEY_BACK,
	GF_KEY_CAMERA,
} gf_key_event_t;

struct gf_key {
	enum gf_key_event key;
	uint32_t value;   /* key down = 1, key up = 0 */
};

struct gf_key_map {
	unsigned int type;
	unsigned int code;
};

struct gf_ioc_chip_info {
	unsigned char vendor_id;
	unsigned char mode;
	unsigned char operation;
	unsigned char reserved[5];
};

#define GF_IOC_MAGIC    'g'     //define magic number
#define GF_IOC_INIT             _IOR(GF_IOC_MAGIC, 0, uint8_t)
#define GF_IOC_EXIT             _IO(GF_IOC_MAGIC, 1)
#define GF_IOC_RESET            _IO(GF_IOC_MAGIC, 2)
#define GF_IOC_ENABLE_IRQ       _IO(GF_IOC_MAGIC, 3)
#define GF_IOC_DISABLE_IRQ      _IO(GF_IOC_MAGIC, 4)
#define GF_IOC_ENABLE_SPI_CLK   _IOW(GF_IOC_MAGIC, 5, uint32_t)
#define GF_IOC_DISABLE_SPI_CLK  _IO(GF_IOC_MAGIC, 6)
#define GF_IOC_ENABLE_POWER     _IO(GF_IOC_MAGIC, 7)
#define GF_IOC_DISABLE_POWER    _IO(GF_IOC_MAGIC, 8)
#define GF_IOC_INPUT_KEY_EVENT  _IOW(GF_IOC_MAGIC, 9, struct gf_key)
#define GF_IOC_ENTER_SLEEP_MODE _IO(GF_IOC_MAGIC, 10)
#define GF_IOC_GET_FW_INFO      _IOR(GF_IOC_MAGIC, 11, uint8_t)
#define GF_IOC_REMOVE           _IO(GF_IOC_MAGIC, 12)
#define GF_IOC_CHIP_INFO        _IOW(GF_IOC_MAGIC, 13, struct gf_ioc_chip_info)

#define GF_IOC_WAKELOCK_TIMEOUT_ENABLE        _IO(GF_IOC_MAGIC, 18 )
#define GF_IOC_WAKELOCK_TIMEOUT_DISABLE        _IO(GF_IOC_MAGIC, 19 )
#define GF_IOC_CLEAN_TOUCH_FLAG        _IO(GF_IOC_MAGIC, 20 )

#if defined(CONFIG_OPPO_FINGERPRINT_GOODIX_NAV_EVENT)
#define GF_IOC_NAV_EVENT	_IOW(GF_IOC_MAGIC, 14, gf_nav_event_t)
#define  GF_IOC_MAXNR    15  /* THIS MACRO IS NOT USED NOW... */
#else
#define  GF_IOC_MAXNR    14  /* THIS MACRO IS NOT USED NOW... */
#endif

#define GF_NET_EVENT_FB_BLACK 2
#define GF_NET_EVENT_FB_UNBLACK 3
#define NETLINK_TEST 25
#define MAX_MSGSIZE         32

enum NETLINK_CMD {
    GF_NET_EVENT_TEST = 0,
    GF_NET_EVENT_IRQ = 1,
    GF_NET_EVENT_SCR_OFF,
    GF_NET_EVENT_SCR_ON,
    GF_NET_EVENT_TP_TOUCHDOWN,
    GF_NET_EVENT_TP_TOUCHUP,
    GF_NET_EVENT_UI_READY,
    GF_NET_EVENT_MAX,
};


struct gf_dev {
	dev_t devt;
	struct list_head device_entry;
	struct device *dev;
	struct clk *core_clk;
	struct clk *iface_clk;

	struct input_dev *input;
	/* buffer is NULL unless this device is open (users > 0) */
	unsigned users;
	signed irq_gpio;
	signed reset_gpio;
	signed pwr_gpio;
	signed vdd_gpio;
	int irq;
	int irq_enabled;
	int clk_enabled;
	struct fasync_struct *async;
	struct notifier_block notifier;
	char device_available;
	char fb_black;
	struct regulator *vreg[1];
};

static int pid = -1;
static struct sock *nl_sk;

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

#if defined(PROJECT_19651)
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

#if defined(PROJECT_19651)
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
#if defined(PROJECT_19651)
    gpio_set_value(gf_dev->pwr_gpio, enabled ? 1 : 0);
    msleep(5);
    gpio_set_value(gf_dev->vdd_gpio, enabled ? 1 : 0);
    pr_debug("set pwe_gpio %s for 19651 \n",
        enabled ? "1" : "0");
#else 
    rc = vreg_setup(gf_dev, "ldo5", enabled);
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

int gf_opticalfp_irq_handler(struct fp_underscreen_info* tp_info);

#endif /*__GF_SPI_H*/
