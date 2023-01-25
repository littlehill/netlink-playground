//Taken from https://stackoverflow.com/questions/15215865/netlink-sockets-in-c-using-the-3-x-linux-kernel?lq=1

#include <pthread.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/time.h>

#include <stdint.h>
#include "../kernel-modules/mycelium.h"

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
    /*netlink related*/
    struct sockaddr_nl src_addr;
    struct nlmsghdr *nlh;
    struct iovec iov;
    struct msghdr msg;
    int len;
    int exitflag = 0;

    /*custom protocol 29 related*/
    nlSignalPayload_t *nlSigPayld;
    nlCmdPayload_t *nlCmdPayld;
    nlHmiSyncPayload_t *nlHmiSyncPayld;

    nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
    memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));

    iov.iov_base = (void *)nlh;
    iov.iov_len = NLMSG_SPACE(MAX_PAYLOAD);

    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    nlSigPayld = (nlSignalPayload_t *)NLMSG_DATA(nlh);
    nlCmdPayld = (nlCmdPayload_t *)NLMSG_DATA(nlh);
    nlHmiSyncPayld = (nlHmiSyncPayload_t *)NLMSG_DATA(nlh);

    while (1)
    {
        u_int32_t mtype;
        /* receive the message */
        len = recvmsg(sock_fd, &msg, 0);
        if (len < 0)
        {
            perror("recvmsg");
            free(nlh); //just to be sure, exit() should mem. release anyway
            exit(EXIT_FAILURE);
        }

        /*should be same as reading nlh->nlmsg_type */
        mtype = ((struct nlmsghdr *)msg.msg_iov->iov_base)->nlmsg_type;
        
        printf("\nRX msg; Type: 0x%02X\n", mtype);
        
        /* process the message */
        switch (mtype) {
          case NLMSG_CANSIG_FLOAT:
            printf("-- SigID: 0x%02X / type float / value: %.3f\n", nlSigPayld->sigid, nlSigPayld->value.f );
            break;
           
          case NLMSG_CANSIG_INT32:
            printf("-- SigID: 0x%02X / type int32 / value: %d\n", nlSigPayld->sigid, nlSigPayld->value.i );
            break;
          
          case NLMSG_CANSIG_UINT32:
            printf("-- SigID: 0x%02X / type uint32 / value: %d\n", nlSigPayld->sigid, nlSigPayld->value.u );
            break;
        
          case NLMSG_CANSIG_BOOL:
            printf("-- SigID: 0x%02X / type bool / value: %s\n", nlSigPayld->sigid, (nlSigPayld->value.b == true) ? "true" : "false" );
            break;
          
          case NLMSG_SIG_COMMAND:
            printf("-- CmdID: 0x%02X / type command (generic)\n", nlCmdPayld->cmdid);
            if (nlCmdPayld->cmdid == NLCMD_LINK_EXIT) {
                exitflag = true;
            }
            break;
        
          case NLMSG_HMI_SYNC:
            printf("-- mtype NLMSG_HMI_SYNC / ID: 0x%02X  stamp: %d --\n", nlHmiSyncPayld->transferid, nlHmiSyncPayld->stamp);
            break;
          
          default:
            printf("-- unknown message type 0x%02X-- payload: %s\n", mtype, (char *)NLMSG_DATA(nlh));
            break;
        }
        
        /* should we exit ? */
        if (exitflag)
        {
            printf("EXIT command received\n");
            free(nlh); //just to be sure, exit() should mem. release anyway
            exit(EXIT_SUCCESS);
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
