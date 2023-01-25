//Taken from https://stackoverflow.com/questions/15215865/netlink-sockets-in-c-using-the-3-x-linux-kernel?lq=1

#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>

#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/kernel.h>

#include "mycelium.h"

static int counter = 0;
static struct task_struct *thread;

/* to be paired with the hrib.c example*/
#define NETLINK_USER 29
#define FAIL_TO_SEND_TRH 4

struct sock *nl_sk = NULL;

static unsigned int totaltx_counter = 0;

int app_pid = -1;


static int thread_fn(void *data)
{
    struct nlmsghdr *nlh;
	struct sk_buff *skb_out;
	int msg_size, pid;
    int fi, res;
    int failed_to_send_counter = 0;
    int fake_speed = 0;

	printk(KERN_INFO "Entering: %s\n", __FUNCTION__);

	nlSignalPayload_t signal_payload;

	while (!kthread_should_stop()) {
		printk(KERN_INFO "thread_fn loop %d\n", counter++);

		if (app_pid != -1) {
            msg_size = sizeof(nlSignalPayload_t);

            skb_out = nlmsg_new(msg_size, 0);
			nlh = nlmsg_put(skb_out, 0, counter, NLMSG_CANSIG_INT32, msg_size, 0);
			NETLINK_CB(skb_out).dst_group = 0;
			
			signal_payload.sigid = NLSIG_SPEED;
            signal_payload.value.i = fake_speed;
            fake_speed = (fake_speed>400) ? 0 : fake_speed + 10;

            memcpy((void*)NLMSG_DATA(nlh), (void*)&signal_payload, msg_size);
        
			res = nlmsg_unicast(nl_sk, skb_out, app_pid);
			if (res < 0) {
                printk(KERN_INFO "Error while sending msg to user - %d\n", ++failed_to_send_counter);
                if (failed_to_send_counter >= FAIL_TO_SEND_TRH) {
                    app_pid = -1; // unregister existing pid
                    failed_to_send_counter = 0; // reset counter
                }
            }
		}
		else {
			printk(KERN_INFO "User app PID not set\n");
		}

		msleep(1026); /* sleep for 1026ms */
	}

	return 0;
}

static void mycelium_recv_msg(struct sk_buff *skb)
{
	struct nlmsghdr *nlh;
    uint32_t mtype;

    /*custom protocol 29 related*/
    nlHmiSyncPayload_t *nlHmiSyncPayld;
	
    nlh = (struct nlmsghdr *)skb->data;

    nlHmiSyncPayld = (nlHmiSyncPayload_t *)nlmsg_data(nlh);

    mtype = nlh->nlmsg_type;
    printk(KERN_INFO "\nMycelium %s; Type: 0x%02X\n", __FUNCTION__, mtype);
    
    /* process the message */
    switch (mtype) {
        case NLMSG_CANSIG_FLOAT:
        case NLMSG_CANSIG_INT32:
        case NLMSG_CANSIG_UINT32:
        case NLMSG_CANSIG_BOOL:        
        case NLMSG_SIG_COMMAND:
            printk(KERN_INFO "-- unsupported message RX, type: 0x%02X \n", mtype);
        break;
    
        case NLMSG_HMI_SYNC:
            printk(KERN_INFO "-- mtype NLMSG_HMI_SYNC / ID: 0x%02X  stamp: %d --\n", nlHmiSyncPayld->transferid, nlHmiSyncPayld->stamp);
            if (nlHmiSyncPayld->transferid == NLHMI_REGPID) {
                app_pid = nlh->nlmsg_pid;
                printk(KERN_INFO "App pid registered: %d\n", app_pid);
            }
            if (nlHmiSyncPayld->transferid == NLHMI_RESETPID) {
                app_pid = -1;
                printk(KERN_INFO "App pid reset.\n");
            }
        break;
        
        default:
            /*to cover the example messages sending text over*/
            printk(KERN_INFO "-- unknown message type 0x%02X-- payload: %s\n", mtype, (char *)NLMSG_DATA(nlh));
        break;
    }
    
    return;
}

static int __init hello_init(void)
{
	printk("Entering: %s\n", __FUNCTION__);
	//This is for 3.6 kernels and above.
	struct netlink_kernel_cfg cfg = {
		.input = mycelium_recv_msg,
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