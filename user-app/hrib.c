#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <unistd.h>

#include <sys/time.h>

#define NETLINK_USER 29
#define MAX_PAYLOAD 1024

typedef enum
{
    false,
    true
} bool;

int sock_fd;

/* thread function to receive messages */
void *recv_thread(void *arg)
{
    struct sockaddr_nl src_addr;
    struct nlmsghdr *nlh;
    struct iovec iov;
    struct msghdr msg;
    int len;
    bool exitflag = false;

    nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
    memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));

    iov.iov_base = (void *)nlh;
    iov.iov_len = NLMSG_SPACE(MAX_PAYLOAD);

    msg.msg_name = (void *)&src_addr;
    msg.msg_namelen = sizeof(src_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    while (1)
    {
        /* receive the message */
        len = recvmsg(sock_fd, &msg, 0);
        if (len < 0)
        {
            perror("recvmsg");
            exit(EXIT_FAILURE);
        }

        /* process the message */
        printf("Received message: %s\n", (char *)NLMSG_DATA(nlh));

        /* should we exit ? */
        exitflag = strcmp((char *)NLMSG_DATA(nlh), "EXITFLAG");
        if (exitflag)
        {
            printf("EXIT msg received\n");
            break;
        }
    }

    free(nlh);
}

int main()
{
    pthread_t recv_thread_id;
    struct sockaddr_nl src_addr, dest_addr;
    int ptret;
    struct nlmsghdr *nlh;
	struct sk_buff *skb_out;
    struct iovec iov;
    struct msghdr msg;

    /* create socket */
    sock_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_USER);
    if (sock_fd < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    /* fill in the source address */
    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid(); /* self pid */

    /* bind the socket to the source address */
    if (bind(sock_fd, (struct sockaddr *)&src_addr, sizeof(src_addr)) < 0)
    {
        perror("bind");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    /* create the receive thread */
    if (pthread_create(&recv_thread_id, NULL, recv_thread, NULL) != 0)
    {
        perror("pthread_create");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
    memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
    nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
    nlh->nlmsg_pid = getpid();
    nlh->nlmsg_flags = 0;

    strcpy((char*)NLMSG_DATA(nlh), "Hello-from-APP");

    iov.iov_base = (void *)nlh;
    iov.iov_len = nlh->nlmsg_len;
    msg.msg_name = (void *)&dest_addr;
    msg.msg_namelen = sizeof(dest_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;


    printf("Send a binding msg to kernel NETLINk_USER ID %d\n", NETLINK_USER);
    sendmsg(sock_fd,&msg,0);

    /* wait for the receive thread to exit */
    printf("|> wait for the receive thread to exit\n");
    ptret = pthread_join(recv_thread_id, NULL);

    printf("|> pthread joined, exit code: %d\n", ptret);

    /* close the socket */
    close(sock_fd);
    return 0;
}