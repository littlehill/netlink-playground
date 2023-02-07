#ifndef MYCELIUM_H
#define MYCELIUM_H

#include <stdbool.h>

#define MYCELIUM_VERSION_STR "1.2.0"

// define the data types of messages
// HAS to be greater than NLMSG_MIN_TYPE (defined in the include/uapi/linux/netlink.h ), for 5.10.72  NLMSG_MIN_TYPE is 0x10

#define NLMSG_HMI_SYNC 0xA0

#define NLMSG_SIG_COMMAND 0xC0
#define NLMSG_CANSIG_FLOAT 0xC1
#define NLMSG_CANSIG_INT32 0xC2
#define NLMSG_CANSIG_UINT32 0xC3
#define NLMSG_CANSIG_BOOL 0xC4

// define the enums for special identifiers

typedef enum
{
    NLHMI_REGPID = 0,
    NLHMI_RESETPID,
    NLHMI_REQUEST
} netlinkhmisync_n;

typedef enum
{
    NLCMD_LINK_EXIT = 0,
    NLCMD_NAVUP,
    NLCMD_NAVDOWN,
    NLCMD_NAVLEFT,
    NLCMD_NAVRIGHT,
    NLCMD_NAVBACK,
    NLCMD_NAVENTER
} netlinkcmddid_n;

typedef enum
{
    NLSIG_SPEED = 0,
    NLSIG_CHARGE,
    NLSIG_RANGE,
    NLSIG_INREVERSE
} netlinksigid_n;

typedef union canSignalData
{
    float f;
    int32_t i;
    uint32_t u;
    bool b;
} canSignalData_u;

// define the payload structure(s)
// - padded to 4 bytes so we do not confuse the memcpy when doing a sizeof on said struct

#pragma pack(push, 4)

typedef struct nlHmiSyncPayload
{
    netlinkhmisync_n transferid;
    uint32_t stamp;
} nlHmiSyncPayload_t;

typedef struct nlCommandPayload
{
    netlinkcmddid_n cmdid;
} nlCmdPayload_t;

typedef struct nlSignalPayload
{
    netlinksigid_n sigid;
    canSignalData_u value;
} nlSignalPayload_t;

typedef union myceliumData
{
    nlHmiSyncPayload_t  hmiPay;
    nlCmdPayload_t  cmdPay;
    nlSignalPayload_t  sigPay;
} myceliumData_u;

typedef struct rpmsgMyceliumWrap
{
    int32_t type;
    int32_t size;
    uint32_t seq;
    myceliumData_u data;
} rpmsgMyceliumWrap_t;

typedef struct rpmsgPayload
{
    rpmsgMyceliumWrap_t payload;
    uint32_t crc;
} rpmsgPayload_t;

#pragma pack(pop)

#endif /* MYCELIUM_H */
