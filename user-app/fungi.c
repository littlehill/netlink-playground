//Taken from https://stackoverflow.com/questions/15215865/netlink-sockets-in-c-using-the-3-x-linux-kernel?lq=1

#include <pthread.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/time.h>

#define REPEAT_COUNT 10000

#define NETLINK_USER 29

#define MAX_PAYLOAD 1024 /* maximum payload size*/
struct sockaddr_nl src_addr, dest_addr;
struct nlmsghdr *nlh = NULL;
struct iovec iov;
int sock_fd;
struct msghdr msg;

long long meas_start, meas_end;

long long current_timestamp() {
    struct timeval te; 
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // calculate milliseconds
    // printf("milliseconds: %lld\n", milliseconds);
    return milliseconds;
}

/* thread function to receive messages */
void *recv_thread(void *arg)
{
    struct sockaddr_nl src_addr;
    struct nlmsghdr *nlh;
    struct iovec iov;
    struct msghdr msg;
    int len;
    int exitflag = 0;

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
        if (!exitflag)
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
int ptret;

sock_fd=socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);
if(sock_fd<0)
return -1;

memset(&src_addr, 0, sizeof(src_addr));
src_addr.nl_family = AF_NETLINK;
src_addr.nl_pid = getpid(); /* self pid */

bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr));

memset(&dest_addr, 0, sizeof(dest_addr));
memset(&dest_addr, 0, sizeof(dest_addr));
dest_addr.nl_family = AF_NETLINK;
dest_addr.nl_pid = 0; /* For Linux Kernel */
dest_addr.nl_groups = 0; /* unicast */

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


sendmsg(sock_fd,&msg,0);

    /* create the receive thread */
    if (pthread_create(&recv_thread_id, NULL, recv_thread, NULL) != 0)
    {
        perror("pthread_create");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }


/*  printf("Sending message to kernel %d-X times\n", REPEAT_COUNT);
    meas_start = current_timestamp();
    for (int i=0; i<REPEAT_COUNT; i+=1) {
        sendmsg(sock_fd,&msg,0);
    }
    meas_end = current_timestamp();
    long long result = meas_end-meas_start;
    printf("Sending speed:  %lld[ms] to send %d messages. -> %.4f [ms/message]\n", result, REPEAT_COUNT,  result/((float)REPEAT_COUNT));
*/

    /* wait for the receive thread to exit */
    printf("|> wait for the receive thread to exit\n");
    ptret = pthread_join(recv_thread_id, NULL);

    printf("|> pthread joined, exit code: %d\n", ptret);

close(sock_fd);
}
