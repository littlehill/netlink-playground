//Taken from https://stackoverflow.com/questions/15215865/netlink-sockets-in-c-using-the-3-x-linux-kernel?lq=1

#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>

#include <linux/kthread.h>
#include <linux/delay.h>
//#include <linux/kernel.h>

#include "mycelium.h"

static int counter = 0;
static struct task_struct *thread;

/* to be paired with the hrib.c example*/
#define NETLINK_USER 29

struct sock *nl_sk = NULL;

static unsigned int totaltx_counter = 0;

int app_pid = -1;


static int thread_fn(void *data)
{
    struct nlmsghdr *nlh;
	struct sk_buff *skb_out;
	int msg_size, pid;
    int fi, res;

	printk(KERN_INFO "Entering: %s\n", __FUNCTION__);

	char *canmsg = "CANSIM: 0x01 0x02 0x03 0x04 0x05 0x06 0x07 0x08 in string";
    msg_size = strlen(canmsg);

	while (!kthread_should_stop()) {
		printk(KERN_INFO "thread_fn loop %d\n", counter++);


		if (app_pid != -1) {
			skb_out = nlmsg_new(msg_size, 0);
			nlh = nlmsg_put(skb_out, 0, counter, NLMSG_CANSIG_INT32, msg_size, 0);
			NETLINK_CB(skb_out).dst_group = 0;
			
			strcpy((char*)NLMSG_DATA(nlh), "Kernel CAN-msg: 0x0A 0x0B 0x0C 0x0D 0x10 0x20 0x30 0x40");

			pid = app_pid; /*pid of sending process */

			res = nlmsg_unicast(nl_sk, skb_out, pid);
			if (res < 0)
				printk(KERN_INFO "Error while sending msg to user\n");
		}
		else {
			printk(KERN_INFO "User app PID not set\n");
		}

		msleep(1026); /* sleep for 1026ms */
	}

	return 0;
}

static void hello_nl_recv_msg(struct sk_buff *skb)
{
	struct nlmsghdr *nlh;

	printk(KERN_INFO "Entering: %s\n", __FUNCTION__);
	nlh = (struct nlmsghdr *)skb->data;

	if (totaltx_counter == 0 ) {
		app_pid = nlh->nlmsg_pid;
		printk(KERN_INFO "First call, pid: %d\n", app_pid);
	}
	
	printk(KERN_INFO "Netlink received msg payload:'%s' // rxcnt: %d\n",
	       (char *)nlmsg_data(nlh), ++totaltx_counter);
}

static int __init hello_init(void)
{
	printk("Entering: %s\n", __FUNCTION__);
	//This is for 3.6 kernels and above.
	struct netlink_kernel_cfg cfg = {
		.input = hello_nl_recv_msg,
	};

	nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);
	//nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, 0, hello_nl_recv_msg,NULL,THIS_MODULE);
	if (!nl_sk) {
		printk(KERN_ALERT "Error creating socket.\n");
		return -10;
	}

	thread = kthread_run(thread_fn, NULL, "hello_world");
	if (IS_ERR(thread)) {
		printk(KERN_ERR "Failed to create thread\n");
		return PTR_ERR(thread);
	}

	return 0;
}

static void __exit hello_exit(void)
{
	printk(KERN_INFO "exiting hello module\n");
	netlink_kernel_release(nl_sk);
	kthread_stop(thread);
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");