#ifndef MYCELIUM_H
#define MYCELIUM_H

#include <stdint.h>
#include <stdbool.h>

// struct nlmsghdr* nlmsg_put	(	struct nl_msg * 	n,
// uint32_t 	pid,
// uint32_t 	seq,
// int 	type,
// int 	payload,
// int 	flags 
// )		
// read
// Add a netlink message header to a netlink message.

// Parameters
// n	netlink message
// pid	netlink process id or NL_AUTO_PID
// seq	sequence number of message or NL_AUTO_SEQ
// type	message type
// payload	length of message payload
// flags	message flags
// Adds or overwrites the netlink message header in an existing message object. If payload is greater-than zero additional room will be reserved, f.e. for family specific headers. It can be accesed via nlmsg_data().


// define the data types of messages
// HAS to be greater than NLMSG_MIN_TYPE (defined in the include/uapi/linux/netlink.h ), for 5.10.72  NLMSG_MIN_TYPE is 0x10

#define NLMSG_HMI_REGPID 0xA0
#define NLMSG_HMI_RESETPID 0xA1
#define NLMSG_HMI_REQUEST 0xA2

#define NLMSG_SIG_COMMAND 0xB0
#define NLMSG_CANSIG_FLOAT 0xB1
#define NLMSG_CANSIG_INT32 0xB2
#define NLMSG_CANSIG_UINT32 0xB3
#define NLMSG_CANSIG_BOOL 0xB4

typedef enum {
    NLSIG_SPEED = 0,
    NLSIG_CHARGE,
    NLSIG_RANGE,
    NLSIG_INREVERSE
} netlinksigid_t;

typedef enum {
    NLCMD_LINK_EXIT,
    NLCMD_NAVUP,
    NLCMD_NAVDOWN,
    NLCMD_NAVLEFT,
    NLCMD_NAVRIGHT,
    NLCMD_NAVBACK,
    NLCMD_NAVENTER
} netlinkcmddid_t;

typedef union canSignalData {
    float f;
    int32_t i;
    uint32_t u;
    bool b;
} canSignalData_t;

#pragma pack(push, 4)

typedef struct nlSignalPayload {
    netlinksigid_t sigid;
    canSignalData_t value;
} nlSignalPayload_t;

typedef struct nlCommandPayload {
     netlinkcmddid_t cmdid;
} nlCmdPayload_t;

#pragma pack(pop)


#endif /* MYCELIUM_H */
