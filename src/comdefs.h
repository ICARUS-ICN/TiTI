/*--------------------------------------------------------------------------*/
/*
NewVersion @(#)comdefs.h	3.9 07/20/98

 This file contains the common defines that are included in every model
                          configuration.
   
	                                                                    */
/*--------------------------------------------------------------------------*/

#include <limits.h>

/* #define FILES_MMAPPED */
#define INDEP_SEEDS /* Generador individual por v.a. */
#ifdef INDEP_SEEDS
#define ALEATORIO3 /* Fuerza generador LMCG con una semilla */
				   /* Mas lento pero necesario en batch de simulaciones */
#endif

#define DIRECTED 1 /* To activate timers P6 P4 P2 P0 SES */
#define MILLISEC 1000

#ifndef HZ
#ifdef CLK_TCK /* definido en limit.h en System V */
#define HZ (CLK_TCK)
#else
#ifdef VMS
#define HZ (MILLISEC / 10) /*10-millisec. time units returned in times() */
#else
#ifdef UNIX
#define HZ 60 /* 1/60 sec units returned in times() */
#else
#define HZ 60 /* 1/60 sec units returned in times() */
#endif		  //UNIX
#endif		  //VMS
#endif		  // CLK_TCK
#endif		  // HZ

#define LINK 0 /* Codes for media type */
#define CONTENTION 1
#define TOKEN 2
#define TOKEN_HI_SPEED 3
#define HYBRID_RING 4

#define CONSTANT 0 /* Codes for distributions */
#define UNIFORM 1
#define NORMAL 2
#define POISSON 3
#define EXPONENTIAL 4
#define GAMMA 5
#define TRIANGULAR 6
#define TABULAR 7
#define VELOCIDAD_UNIFORME 8
#define SINCRONO 9
#define GRUBER82 10 /* ANHADIDO */
#define SRIRAM86 11
#define YEGENOGLU92 12
#define BRADY69 13
#define KRUNZ95 14
#define TORBEY91 15
#define RIYAZ95_A 16
#define RIYAZ95_B 17
#define WEILLBULL 18
#define PARETO 19
#define LOGNORMAL 20
#define TIEMPO_TELNET_1 21
#define TIEMPO_TELNET_2 22
#define TAM_PKT_SMTP 23
#define PKT_FTP 24
#define TIEMPO_FTP 25
#define TAM_PKT_NNTP 26
#define DOBLE_DET 27
#define FILE_SMPLS 28
#define BINARY_FILE 29

#define UNI_FILE_XFER 1 /* Codes for application types */
#define BI_FILE_XFER 2
#define PERIODIC_STAT_RPT 3
#define REQUEST_STAT 4
#define DATA_ENTRY 5
#define VIRTUAL_TERM 6
#define DB_QUERY 7

#define LONG 1	 /* Type of TSDU response for VIRTUAL TERM appl */
#define CR 2	 /* Type of TSDU request/response for VIRTUAL TERM appl */
#define FORM 3	 /* Type of TSDU response for DATA ENTRY appl */
#define FIELDS 4 /* Type of TSDU request for DATA ENTRY appl */

#define NO_LEVELS_PRI 8 /* Maximun number of priority levels for \
						 asynchronous traffic in FDDI    */

#define MTYPES 2 /* Number of backgound message types */

#define SN_TO_NET_INT_LIMIT 1 /* limit on number of msgs in intf */
#define NET_TO_SN_INT_LIMIT 1

#define MAX_BLOCK_CNT 6		 /* count of common message sizes */
#define ARRIVAL_INTERVAL 0.0 /* default arrival interval for continuously queued data */
#define BUCKETS 20			 /* count of bins in the retransmission histogram */
#define XDT_DATA 16			 /* Maximum size of data in an XDT */
#define CHKSUM_SIZE 4		 /* size of TP checksum field */
#define NORMAL_HEAD_SIZE 5	 /* normal seq# size, no chksum	*/
#define NOR_SEQ_NO_SIZE 1	 /* 1 octet for seq#, normal */
#define EXT_SEQ_NO_SIZE 4	 /* 4 octets for seq#, extended */
#define EXT_AK_HEAD_SIZE 10	 /* ak head size for extended seq# */

#ifdef NIP
#define TIMER_TIME 2.0	 /* # of ms on avg. needed to fire a */
						 /* retransmit timer	*/
#define LONG_SUSPEND .75 /* # of ms on avg. a TP must wait to */
						 /*  regain the processor */
#define SHORT_SUSPEND .3 /* # of ms on avg. a TP must wait to */
						 /*  regain the processor */
#endif

#ifdef RECEIVE_LIMIT
#define MAX_RECEIVES_IN_A_ROW 16 /* # of msgs tp can receive before blocking */
#endif

#define CHUNK 128 /* memory map chunk size */
#define NONE 0
#define ERROR -1
#define ALL -1
#define BUSY 1
#define FREE 0
#define TRUE 1
#define FALSE 0
#ifndef INFINITY
#define INFINITY 1.7E38
#endif
#define VERY_LG_NO 1.6E38
#define MAX_INT 2147483647
#define INV_MAX_INT ((double)(1.0 / MAX_INT))

#define MAX_NO_MEDIA 2

#ifdef SATELLITE
#define MAX_NO_MEDIA 2
#endif

#define NUM_MAX_MUESTRAS 1000

#define MAX_PRIORITY 3
#define MAX_NO_TSDUS 200
#define MAX_NO_BUFS 40
#define MIN_DEAD_TIME 10000
#define DEADLOCK 1
#define OH_MSGS 5
#define PLUS '+'
#define MINUS '-'
#define DT 1
#define XDT 2
#define AK 3
#define XAK 4
#define IDU 5
#define XIDU 6
#define ERROR_BIT 8
#define EOT 16
#define MARK 32		   /* mark DT as received */
#define IO_WAIT 64	   /* IO WAIT flag - SES */
#define START_TSDU 128 /* mark as start of TSDU - SES */
#define RETRIEVE 1
#define MERGE 2
#define MARK_EOT 4
#define FIRST_EM 8
#define RELEASE 16
#define LOST -1	  /* event is lost */
#define CANCEL -2 /* event is canceled */
#define ALLBLOCKED 0
#define LINELENGTH 128
#define TYPESIZE 5
#define CSHIFT 8  /* shift data flags to get connection # - SES */
#define DSHIFT 16 /* shift data flags to get destination user - SES */
#define SSHIFT 24 /* shift data flags to get source user - SES */
#define ERRORMASK 037777777767
#define MASK 07
#define XDTMASK 0377		 /* SES */
#define ADDMASK 037777777400 /* address is connection, source & destination - SES */

#define NTW 0 /* third row in priority table */
#define SNT 1 /* fourth row in priority table */

#ifdef SATELLITE
#define NUM_MOD_TYPES /* number of types of modules */
#endif

#define NUM_MOD_TYPES 2

#define NUM_NET_EVENTS 2 /* maximum number of network event types */

#ifdef SATELLITE
#define NUM_SNT_EVENTS 4 /* maximum number of subnet event types */
#endif

#define NUM_SNT_EVENTS 5
#define NUM_EVENT_TYPES NUM_SNT_EVENTS /* maximum number of types of events */

#define XMIT 0 /* transmit buffer */
#define RCV 1  /* receive buffer */

#define EVENT1 0
#define EVENT2 1
#define EVENT3 2
#define EVENT4 3
#define EVENT5 4
#define EVENT6 5
#define EVENT7 6
#define EVENT8 7
#define EVENT9 8
#define EVENT10 9
#define EVENT11 10
#define EVENT12 11
#define EVENT13 12
#define EVENT14 13
#define EVENT15 14
#define EVENT16 15
#define EVENT17 16

/****
Constantes y tipos utilizados en la simulacion de HRC - FDDI-II
*****/

/* Longitud del ciclo en segundos */

#define CYCLE_LENGTH 125e-6

/* Ancho de banda de la cabecera del ciclo */
#define HEADER_SPEED (HEADER_SIZE / (CYCLE_LENGTH * MILLISEC))

/* Ancho de banda del DPG */
#define DPG_SPEED (768000.0 / MILLISEC)

/* Ancho de banda del WBC */
#define WBC_SPEED (6144000.0 / MILLISEC)

/* Tamanio de la cabecera del ciclo en bits */
#define HEADER_SIZE (52 + wbcs * 4)

/* Ancho de banda de un canal de voz */
#define CHANNEL_BW (64000.0 / MILLISEC)

/* ancho de banda de senalizacion */
#define SIGNALLING_BW (16000.0 / MILLISEC)
