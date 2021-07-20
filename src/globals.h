#ifndef GLOBALS_H
#define GLOBALS_H

/* VERSION 2.0 - COLUMBUS DMS
NewVersion @(#)globals.h	3.17 07/24/98 ETSIT-IT

file - globals.h

*/

#include <stdio.h>
#include <sys/times.h>

#include "comdefs.h"
#include "types.h"

#define MAX_OCC 17 /* the maximum occurence of any module type */
				   /* used to allocate storage for event statistics */
				   /* must be updated when the number of users, tps, */
				   /* or networks is increased.			  */
#define BATCH 0
#define TERM 1
#define WAIT 2
#define GRAPHICS 1
#define ON 1
#define OFF 0
#define VERSION "2.0"
#define CHK_CNTLC   \
	if (stop_print) \
		return;

/************************************************************/
/* special globals needed by the signal catching routine */
/* these are passed from main and correspond to argv[1] */
/**************************************************************/
extern char *m1;

extern FILE *efp;

/* GLOBAL SIMULATION CLOCK, DEADLOCK DETECTION SHADOW CLOCK AND METRIC */
/* COMPUTATION CLOCK */

extern double sim_clock;	/* wall clock */
extern double dead_clock;	/* clock for detecting window ack deadlock */
extern double metric_clock; /* adjusted clock for computing metrics */

extern long seed; /* valor de la semilla */

extern double primer_msg; /* tiempo del primer envio de mensage por cualquier network */
extern int transitorio;
extern double sig_event; /* instante de llegada de la siguiente trama de */
						 /* cualquier nodo y de cualquier prioridad a la red */
						 /* en la version reducida */

extern int nodo_inicial; /* Nodo que recibe el testigo inicialmente */
extern int fast;		 /* booleana que indica si se ejcuta la reducida o no */
extern int gg1;			 /* ídem para simular una simple G/G/1

/* Variables anhadidas para FDDI-II */

extern int fddi_ii; /* booleana que indica si se desea incluir FDDI-II o no */
extern int m_no_of_channels, m_cycles, m_wbcs;
extern float m_delay;

extern float TTRT; /* anhadida en la v. reducida s'olo para imprimirla */
				   /* Sera usada posteriormente como target de la P7 */

/* MODE FLAGS FOR EXECUTION CONTROL */

extern int xmode;		/* execution mode flag - TERMINAL OR BATCH */
extern int omode;		/* output mode flag - GRAPHICS OR NONE */
extern int oqueues;		/* controls the saving of event queues for output */
extern int verbose;		/* execute in verbose mode */
extern int debug;		/* Turn on debugging */
extern int stop_print;	/* Turn off listing in progress */
extern int pause_point; /* Pause from model execution */
extern int mem_dist;	/* Controls printing of memory distribution */
extern int event_stat;	/* Controls printing of event statistics */

/* FLAGS PARA CONTROL DEL VOLCADO Y TERMINACION */

extern int sigue_programa;	/* controla la salida suave del programa - TERM */
extern int volcado_parcial; /* controla el volcado durante la simulaci'on de
			    resultados parciales */

/* MODEL EXECUTION CONTROL LIMITS */

extern int anal_est; /* boolean variable that indicates if statistic analysis is done */

#ifdef INDEP_SEEDS
extern int one_minus_u; /* boolean variable that indicates if this is an anthitetic simulation */
#endif

extern int CV_ASINC;				  /* variable de control asincrona */
extern int CV_S;					  /* variable de control sincrona: S */
extern int CV_U;					  /* variable de control sincrona: Upsilon */
extern int files;					  /* variable booleana que indica si se desea imprimir en ficheros las ventanas 
                           de transmision, los llenados de buffer, y los retardos */
extern int inter_est;				  /* booleana: impresion de promedios intermedios */
extern int chd;						  /* booleana: cambio de distribucion de ficheros */
extern char *chdopts;				  /* parametros de cambio de distribucion de ficheros */
extern int file_limit;				  /* numero maximo de muestras imprimibles en fichero (salvo bloques) */
extern int clk_limit;				  /* limit of simulated milliseconds to run */
extern int cpu_limit;				  /* limit of real CPU 10 millisecond increments to run */
extern int oct_limit;				  /* limit of count of octets to be received */
extern int evt_limit;				  /* limit of count of simulated events to execute */
extern int retran_limit;			  /* limit of count of simulated retransmissions */
extern int max_retran;				  /* limit of retransmissions for a single message */
extern int xpd_limit;				  /* limit of expedited data messages sent */
extern int err_limit;				  /* limit of damaged messages */
extern int limit_size[NO_LEVELS_PRI]; /* limits of sizes to select FDDI priorities */

/* MODEL EXECUTION CONTROL STATUS */

extern double octs_rcvd; /* count of octets received by users */
extern int evts_exec;	 /* count of events executed by users */
extern int retrans;		 /* count of retransmissions */
extern int xpd_cnt;		 /* count of expedited data messages sent */
extern int err_cnt;		 /* count of damaged messages */
extern int tsdus_rcvd;	 /* count of TSDUs received */
extern int modactivity;	 /* indication of event execution in some module */

/* MODEL EXECUTION KEYS */

extern int no_of_nets;
extern int no_of_media;
extern int no_conns;
extern int no_clocks;
extern int start_cid;

/* NETWORK STRUCTURES */

extern struct net_info *n;

/* MEDIA STRUCTURES	*/

extern struct subnet_info *s;
extern struct media_info m[MAX_NO_MEDIA];

/* CONNECTION RELATIONSHIP INFORMATION (Source to Destination User) */

extern struct Inter_CV_Sample *first_waitingCV;
extern int Inter_CV_file;
extern int faltan;	   /* Numero de v.a.s con muestras aun por alcanzar la potencia de 2
	       inmediatamente inferior al minimo tama~no de fichero */
extern int paso_final; /* Dicho numero potencia de 2 */
extern struct conn_info *c;

/* GLOBAL MODEL SIMULATION PARAMETERS */

extern int Nth; /* after N DTs are received an AK is sent */

/* HEAPS FOR SAVING USED AKS & DTS IF REQUIRED BY USER */

extern struct data *dt_heap;
extern struct data *ak_heap;

/* MODULE CLOCKS, CPU CLOCKS, & BUFFER SPACE */

/* SES #5 double clocks[MAX_NO_CLOCKS]; */ /* array of mod clocks */
extern double (*clocks)[2];				   /* array of mod clocks the second coordinate
					is the MIPS power of the clock */
/* more than one mod can point to the same clock */
extern double *cpus; /* array for CPU UTILIZATION */

/* METRIC ACCUMULATORS */
extern struct measures *stats;

/* TIME STRUCTURES */

extern struct tms sys_time; /* CPU time used */

extern struct tms start_cpu; /* CPU time used already when program starts */

extern long wtime; /* Model start time - secs. since 1970 */

extern struct data tkn;

#ifdef NIP

extern float tpe[MAX_NO_TSDUS];

#endif

#endif /* GLOBALS_H */