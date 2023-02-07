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

#define REPEAT_COUNT 2

#define NETLINK_USER 29
#define SIG_RPMSG_MASK 0x000000FF

#define MAX_PAYLOAD 128 /* maximum payload size*/
struct sockaddr_nl src_addr, dest_addr;
struct nlmsghdr *nlh = NULL;
struct iovec iov;
int sock_fd;
struct msghdr msg;

long long meas_start, meas_end;

void translateSignalName(netlinksigid_n id ,char* name) {
    switch (id) {
        case NLSIG_SPEED:
            strcpy(name, "SPEED");
        break;
        case NLSIG_CHARGE:
            strcpy(name, "CHARGE");
        break;
        case NLSIG_RANGE:
            strcpy(name, "RANGE");
        break;
        case NLSIG_INREVERSE:
            strcpy(name, "INREVERSE");
        break;
        default :
            strcpy(name, "-unknown-");
        break;
    }
}


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
        uint32_t mtype;
        netlinksigid_n local_SigId;

        char namebuf[24];
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
        local_SigId = (netlinksigid_n)(nlSigPayld->sigid & SIG_RPMSG_MASK);

        //printf("\nRX msg; Type: 0x%02X \n", mtype);
        
        if (nlSigPayld->value.i >= REPEAT_COUNT) {
            meas_end = current_timestamp();
            long long result = meas_end-meas_start;
            printf("val: %d\n", nlSigPayld->value.i);
            printf("Sending speed:  %lld[ms] to send %d messages. -> %.4f [ms/message]\n", result, REPEAT_COUNT,  result/((float)REPEAT_COUNT));
            }
        else {
            //printf ("count %d\n",nlSigPayld->value.i);
        }

        // /* process the message */
        // switch (mtype) {
        //   case NLMSG_CANSIG_FLOAT:
        //     translateSignalName(local_SigId, namebuf);
        //     printf("-- SigID: 0x%02X / %s -- float / value: %.3f\n", local_SigId, namebuf, nlSigPayld->value.f );
        //     break;
           
        //   case NLMSG_CANSIG_INT32:
        //     translateSignalName(local_SigId, namebuf);
        //     printf("-- SigID: 0x%02X / %s -- int32 / value: %d\n", local_SigId, namebuf, nlSigPayld->value.i );
        //     break;
          
        //   case NLMSG_CANSIG_UINT32:
        //     translateSignalName(local_SigId, namebuf);
        //     printf("-- SigID: 0x%02X / %s -- uint32 / value: %d\n", local_SigId, namebuf, nlSigPayld->value.u );
        //     break;
        
        //   case NLMSG_CANSIG_BOOL:
        //     translateSignalName(local_SigId, namebuf);
        //     printf("-- SigID: 0x%02X / %s -- bool / value: %s\n", local_SigId, namebuf, (nlSigPayld->value.b == true) ? "true" : "false" );
        //     break;
          
        //   case NLMSG_SIG_COMMAND:
        //     printf("-- CmdID: 0x%02X / type command (generic)\n", nlCmdPayld->cmdid);
        //     if (nlCmdPayld->cmdid == NLCMD_LINK_EXIT) {
        //         exitflag = true;
        //     }
        //     break;
        
        //   case NLMSG_HMI_SYNC:
        //     printf("-- mtype NLMSG_HMI_SYNC / ID: 0x%02X  stamp: %d --\n", nlHmiSyncPayld->transferid, nlHmiSyncPayld->stamp);
        //     break;
          
        //   default:
        //     printf("-- unknown message type 0x%02X-- payload: %s\n", mtype, (char *)NLMSG_DATA(nlh));
        //     break;
        // }
        
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

// create the function to be executed as a thread
void *transmit_emulator(void *ptr)
{
    int type = (int) ptr;
    fprintf(stderr,"Thread - %d\n",type);
    return  ptr;
}


int main()
{
    pthread_t recv_thread_id, tx_thread_id;
    int ptret;
    nlHmiSyncPayload_t sync_payload;
    nlSignalPayload_t signal_payload;
    int mypid = getpid(); /* self pid */

    sock_fd=socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);
    if(sock_fd<0)
    return -1;

    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = mypid;

    bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr));

    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0; /* For Linux Kernel */
    dest_addr.nl_groups = 0; /* unicast */

    nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
    memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
    nlh->nlmsg_type = NLMSG_HMI_SYNC; /*from mycelium.h, request/response type*/
    nlh->nlmsg_pid = mypid;
    nlh->nlmsg_flags = 0;
    nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);

    /*insert the REGPID payload*/
    sync_payload.transferid = NLHMI_REGPID;
    sync_payload.stamp = 5; //'random' number - chosen by fair dice roll
    memcpy((void*)NLMSG_DATA(nlh), (void*)&sync_payload, sizeof(nlHmiSyncPayload_t));

    /*unsure why exactly the example had this in it*/
    iov.iov_base = (void *)nlh;
    iov.iov_len = nlh->nlmsg_len;
    msg.msg_name = (void *)&dest_addr;
    msg.msg_namelen = sizeof(dest_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    /*transmit*/
    sendmsg(sock_fd,&msg,0); //sync the pid

    /* create the receive thread */
    if (pthread_create(&recv_thread_id, NULL, recv_thread, NULL) != 0)
    {
        perror("pthread_create");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    nlh->nlmsg_type = NLMSG_CANSIG_INT32; /*from mycelium.h, request/response type*/
    signal_payload.sigid = NLSIG_SPEED;
    signal_payload.value.i = 0;
    memcpy((void*)NLMSG_DATA(nlh), (void*)&signal_payload, sizeof(nlSignalPayload_t));

  printf("Sending message to kernel %d-X times\n", REPEAT_COUNT);
    meas_start = current_timestamp();
    for (int i=0; i<REPEAT_COUNT; i+=1) {
        //signal_payload.value.i = (signal_payload.value.i+1)%400;
        //memcpy((void*)NLMSG_DATA(nlh), (void*)&signal_payload, sizeof(nlSignalPayload_t));
        sendmsg(sock_fd,&msg,0);
    }
    //meas_end = current_timestamp();
    long long result = meas_end-meas_start;
    // moved to RX on 10.000th message
    //printf("Sending speed:  %lld[ms] to send %d messages. -> %.4f [ms/message]\n", result, REPEAT_COUNT,  result/((float)REPEAT_COUNT));


    /* wait for the receive thread to exit */
    printf("|> wait for the receive thread to exit\n");
    ptret = pthread_join(recv_thread_id, NULL);
    ptret = pthread_join(recv_thread_id, NULL);

    printf("|> pthread joined, exit code: %d\n", ptret);

    close(sock_fd);
}
