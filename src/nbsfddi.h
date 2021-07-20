/* VERSION 2.0 - COLUMBUS DMS
NewVersion @(#)nbsfddi.h	2.3 11/24/94 ETSIT-IT
file - nbsdefs.h

this defines the nbs expedited mechanism

the nbs acknowledgement-retransmission mechanism is defined by setting N=1
in the input parameter file, since nbs ack-retrans is implemented as Nth data
ack-retrans with N=1

Nth data acknowledgement-retransmission mechanism is not explicitly defined
it is the default

*/

#define OSI_EXP		1
// #define UNIX            1
#define DIRECTED        1        /* SES */
#define SYS_VERSION_SV

#define VA_CTRL_TLLEGADAS
/* si no esta definida, la variable de control ser'a el tamanho de mensaje */

/*
#define UNIX
#define VMS

#define SYS_VERSION_SV
#define SYS_VERSION_BSD

#define CSMA_CD
#define TOKEN_BUS
#define SATELLITE
#define NIP
#define INTEL
#define NBS_EXP
#define NON_BLOCK_EXP
#define TSDU_SENS_EXP
#define OSI_EXP
#define SELECT_ACK
#define RANGE_ACK
#define NO_REORDERING
#define ONE_AK_QUEUED
#define RECEIVE_LIMIT
#define SIZE_SELECT
#define QOS_SELECT
#define DIRECTED
*/

#include "comdefs.h"
