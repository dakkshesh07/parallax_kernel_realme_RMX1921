/************************************************************************************
 ** File: - vendor/fingerprint/goodix/gf_hal/gf_queue.c
 ** VENDOR_EDIT
 ** Copyright (C), 2008-2018, OPPO Mobile Comm Corp., Ltd
 **
 ** Description:
 **      Fingerprint SENSORTEST FOR GOODIX (SW23)
 **
 ** Version: 1.0
 ** Date created: 19:23:15,13/03/2018
 ** Author: oujinrong@Prd.BaseDrv
 ** TAG: BSP.Fingerprint.Basic
 ** --------------------------- Revision History: --------------------------------
 **    <author>      <data>           <desc>
 **    oujinrong     2018/03/13       create the file, add fix for problem for coverity 17731
 ************************************************************************************/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/timer.h>
#include <linux/time.h>
#include <linux/types.h>
#include <net/sock.h>
#include <net/netlink.h>
#include "gf_spi.h"

#define MAX_MSGSIZE 32

static int pid = -1;
static struct sock *nl_sk;

int gf_sendnlmsg(const char *message)
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

static void gf_netlink_rcv(struct sk_buff *skb)
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


int gf_netlink_init(void)
{
#ifdef GF_NETLINK_ENABLE
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

void gf_netlink_exit(void)
{
#ifdef GF_NETLINK_ENABLE
	if(nl_sk != NULL){
		netlink_kernel_release(nl_sk);
		nl_sk = NULL;
	}
#endif
}

