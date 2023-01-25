#ifndef MYCELIUM_H
#define MYCELIUM_H

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

#define NLMSG_HMI_SYNC 0xA0

#define NLMSG_SIG_COMMAND 0xC0
#define NLMSG_CANSIG_FLOAT 0xC1
#define NLMSG_CANSIG_INT32 0xC2
#define NLMSG_CANSIG_UINT32 0xC3
#define NLMSG_CANSIG_BOOL 0xC4


typedef enum {
    NLHMI_REGPID = 0,
    NLHMI_RESETPID,
    NLHMI_REQUEST
} netlinkhmisync_n;

typedef enum {
    NLCMD_LINK_EXIT = 0,
    NLCMD_NAVUP,
    NLCMD_NAVDOWN,
    NLCMD_NAVLEFT,
    NLCMD_NAVRIGHT,
    NLCMD_NAVBACK,
    NLCMD_NAVENTER
} netlinkcmddid_n;

typedef enum {
    NLSIG_SPEED = 0,
    NLSIG_CHARGE,
    NLSIG_RANGE,
    NLSIG_INREVERSE
} netlinksigid_n;

typedef union canSignalData {
    float f;
    int32_t i;
    uint32_t u;
    bool b;
} canSignalData_u;

#pragma pack(push, 4)

typedef struct nlHmiSyncPayload {
    netlinkhmisync_n transferid;
    uint32_t stamp;
} nlHmiSyncPayload_t;

typedef struct nlCommandPayload {
     netlinkcmddid_n cmdid;
} nlCmdPayload_t;

typedef struct nlSignalPayload {
    netlinksigid_n sigid;
    canSignalData_u value;
} nlSignalPayload_t;

#pragma pack(pop)


#endif /* MYCELIUM_H */
