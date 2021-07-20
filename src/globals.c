#include "globals.h"

/************************************************************/
/* special globals needed by the signal catching routine */
/* these are passed from main and correspond to argv[1] */
/**************************************************************/
char *m1;

FILE *efp;

/* GLOBAL SIMULATION CLOCK, DEADLOCK DETECTION SHADOW CLOCK AND METRIC */
/* COMPUTATION CLOCK */

double sim_clock;	 /* wall clock */
double dead_clock;	 /* clock for detecting window ack deadlock */
double metric_clock; /* adjusted clock for computing metrics */

long seed; /* valor de la semilla */

double primer_msg; /* tiempo del primer envio de mensage por cualquier network */
int transitorio;
double sig_event; /* instante de llegada de la siguiente trama de */
				  /* cualquier nodo y de cualquier prioridad a la red */
				  /* en la version reducida */

int nodo_inicial; /* Nodo que recibe el testigo inicialmente */
int fast;		  /* booleana que indica si se ejcuta la reducida o no */
int gg1;		  /* ídem para simular una simple G/G/1

/* Variables anhadidas para FDDI-II */

int fddi_ii; /* booleana que indica si se desea incluir FDDI-II o no */
int m_no_of_channels, m_cycles, m_wbcs;
float m_delay;

float TTRT; /* anhadida en la v. reducida s'olo para imprimirla */
			/* Sera usada posteriormente como target de la P7 */

/* MODE FLAGS FOR EXECUTION CONTROL */

int xmode;		 /* execution mode flag - TERMINAL OR BATCH */
int omode;		 /* output mode flag - GRAPHICS OR NONE */
int oqueues;	 /* controls the saving of event queues for output */
int verbose;	 /* execute in verbose mode */
int debug;		 /* Turn on debugging */
int stop_print;	 /* Turn off listing in progress */
int pause_point; /* Pause from model execution */
int mem_dist;	 /* Controls printing of memory distribution */
int event_stat;	 /* Controls printing of event statistics */

/* FLAGS PARA CONTROL DEL VOLCADO Y TERMINACION */

int sigue_programa;	 /* controla la salida suave del programa - TERM */
int volcado_parcial; /* controla el volcado durante la simulaci'on de
			    resultados parciales */

/* MODEL EXECUTION CONTROL LIMITS */

int anal_est; /* boolean variable that indicates if statistic analysis is done */

#ifdef INDEP_SEEDS
int one_minus_u; /* boolean variable that indicates if this is an anthitetic simulation */
#endif

int CV_ASINC;				   /* variable de control asincrona */
int CV_S;					   /* variable de control sincrona: S */
int CV_U;					   /* variable de control sincrona: Upsilon */
int files;					   /* variable booleana que indica si se desea imprimir en ficheros las ventanas 
                           de transmision, los llenados de buffer, y los retardos */
int inter_est;				   /* booleana: impresion de promedios intermedios */
int chd;					   /* booleana: cambio de distribucion de ficheros */
char *chdopts;				   /* parametros de cambio de distribucion de ficheros */
int file_limit;				   /* numero maximo de muestras imprimibles en fichero (salvo bloques) */
int clk_limit;				   /* limit of simulated milliseconds to run */
int cpu_limit;				   /* limit of real CPU 10 millisecond increments to run */
int oct_limit;				   /* limit of count of octets to be received */
int evt_limit;				   /* limit of count of simulated events to execute */
int retran_limit;			   /* limit of count of simulated retransmissions */
int max_retran;				   /* limit of retransmissions for a single message */
int xpd_limit;				   /* limit of expedited data messages sent */
int err_limit;				   /* limit of damaged messages */
int limit_size[NO_LEVELS_PRI]; /* limits of sizes to select FDDI priorities */

/* MODEL EXECUTION CONTROL STATUS */

double octs_rcvd; /* count of octets received by users */
int evts_exec;	  /* count of events executed by users */
int retrans;	  /* count of retransmissions */
int xpd_cnt;	  /* count of expedited data messages sent */
int err_cnt;	  /* count of damaged messages */
int tsdus_rcvd;	  /* count of TSDUs received */
int modactivity;  /* indication of event execution in some module */

/* MODEL EXECUTION KEYS */

int no_of_nets;
int no_of_media;
int no_conns;
int no_clocks;
int start_cid;

/* NETWORK STRUCTURES */

struct net_info *n;

/* MEDIA STRUCTURES	*/

struct subnet_info *s;
struct media_info m[MAX_NO_MEDIA];

/* CONNECTION RELATIONSHIP INFORMATION (Source to Destination User) */

struct Inter_CV_Sample *first_waitingCV;
int Inter_CV_file;
int faltan;		/* Numero de v.a.s con muestras aun por alcanzar la potencia de 2
	       inmediatamente inferior al minimo tama~no de fichero */
int paso_final; /* Dicho numero potencia de 2 */
struct conn_info *c;

/* GLOBAL MODEL SIMULATION PARAMETERS */

int Nth; /* after N DTs are received an AK is sent */

/* HEAPS FOR SAVING USED AKS & DTS IF REQUIRED BY USER */

struct data *dt_heap;
struct data *ak_heap;

/* MODULE CLOCKS, CPU CLOCKS, & BUFFER SPACE */

/* SES #5 double clocks[MAX_NO_CLOCKS]; */ /* array of mod clocks */
double (*clocks)[2];					   /* array of mod clocks the second coordinate
					is the MIPS power of the clock */
/* more than one mod can point to the same clock */
double *cpus; /* array for CPU UTILIZATION */

/* METRIC ACCUMULATORS */
struct measures *stats;

/* TIME STRUCTURES */

struct tms sys_time; /* CPU time used */

struct tms start_cpu; /* CPU time used already when program starts */

long wtime; /* Model start time - secs. since 1970 */

struct data tkn;

#ifdef NIP

float tpe[MAX_NO_TSDUS];

#endif
