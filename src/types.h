#ifndef TYPES_H
#define TYPES_H

/*--------------------------------------------------------------------------*/
/*  
Version @(#)types.h	4.8 08/31/98

              Almost all structures are defined here
                   event queues are linked lists, 
              one list for each user, tp, and network.
                                                                             */
/*---------------------------------------------------------------------------*/
#include "promedios.h"

/* EVENT TEMPLATE */

struct event
{
  double xtime;              /* time when event can be executed */
  int id;                    /* identification of event */
  int priority;              /* event priority */
  struct data *data_p;       /* pointer to data */
  struct event *next_event;  /* next event in list active or not */
  struct event *prev_event;  /* previous event in big list */
  double atime;              /* actual time when event was executed */
  int msg_order;             /* msg tag number to prevent misordering */
  struct event *next_aevent; /* next active event */
  struct event *prev_aevent; /* previous active event */
};
/* DATA TEMPLATE */

struct data
{
  int flags; /* bits 29-1 are used                   SES */
  /* 1,2,3 data type (DT,XDT,AK,XAK,IDU,XIDU) */
  /* 4 error bit                              */
  /* 5 eot bit                                */
  /* 6 mark as received correctly             */
  /* 7 io wait bit                        SES */
  /* 8 mark as start of TSDU              SES */
  /* 9,10,11 ?????????                    SES */
  /* 12/17 conn-id                        SES */
  /* 18/23 source                         SES */
  /* 24/29 destination                    SES */
  float q_wait;     /* tiempo de espera en cola para el mensaje */
  double serv_time; /* tiempo que tarda el mensaje en llegar al
				   nodo destino */
  double delay;     /* tiempo de propagaci'on */

  double gen_time; /* time when this packet was generated */

  int first;          /* indica la primera trama del paquete */
  long first_seq_num; /* indica el num. de la primera trama */
  long pkt_num;       /* numero de paquete */
  long frame_num;     /* numero de trama */

  int uses;       /* count of # of pointers to this data */
  long int dsize; /* count of transport user data in this package */
  long th_size;   /* count of transport header data in this
				   package */
  long nh_size;   /* count of network header data in this package */
  long snh_size;  /* count of subnet header data in this package */
  long int seq_num;
  int trans;       /* number of times transmitted */
  long int cdt;    /* credit */
  long int hold;   /* pass info between tp and user */
  long int x_mark; /* XDT seq_num - meaning depends on exp. mech. */
#ifdef NBS_EXP
  long wait; /* XAK awaited blocking this DT */
#endif
#ifdef RANGE_ACK
  long int low;
  long int high;
#endif
#ifdef SELECT_ACK
  long int one_dt;
#endif
  double stime; /* time when this data may be sent */
  struct data *next_data;
};

/* TRAFFIC GENERATION TEMPLATE */
struct traffic
{
  /* static fields */
  int t_flag;             /* basic flag bits to assign to IDUs */
  int tsdu_cnt;           /* count of TSDUs to be generated */
  int command_cnt;        /* count of msgs in each command for
				   VIRTUAL_TERM */
  struct distrib *msg;    /* pointer to TSDU description */
  struct distrib *arrive; /* pointer to TSDU arrival rate distribution */
  float prob_of_msg;      /* probability of this msg */

  /* dynamic fields */
  double cur_tarrive;         /* arrival time of the current TSDU */
  int cur_tlen;               /* number of bytes sent for the current TSDU */
  int cur_tcnt;               /* current number of TSDUs sent */
  long msg_len;               /* current TSDU msg length */
  struct tsdu_info *tsdu_ptr; /* pointer to TSDU */
};

/* DISTRIBUTIONS TEMPLATE */

struct distrib
{
  int seed1, seed2;
  int type;      /* CONSTANT, NORMAL, EXPONENTIAL, etc. */
  double value1; /* parameter values to pass to distribution
				   function. */
  double value2;
  double value3;
  double value4;
  double value5;
  double value6;
  double value7;
  double value8;
  double value9;
  double value10;
  double value11;
  double value12;
  int value13;
  int value14; /* idem 14, indica si ya se ha usado o no */
  int value15; /* en el caso de distribuci'on por
			       fichero, indica el punto donde empieza
			       el intervalo de la siguiente v.a
			       */
  struct tipo_tabla *tabla;
  int indice;                            /* indica para CONSTANT la primera llamada */
  struct distrib *otro;                  /* apunta a la otra distribucion de este trafico */
  double (*F)(double, double, double);   /* Punteros a funciones cuando hay */
  double (*inv)(double, double, double); /* que realizar chd local */
};

/* Tablas para distribuciones de tipo tabular. */

struct tipo_tabla /* Tiempos de llegada entre tramas. */
{
  char *name; /* Nombre del fichero */
#ifdef FLOAT
  float
#else
  double
#endif
      *valor; /* Matriz de valores */
  int limite; /* Si binario, numero de muestras del bloque
				valor*/

  int uses; /* Numero de v.a.s sobre el mismo fichero.
			      Si binario, descriptor de fichero */

  int order; /* Orden de inicializaci'on para cada una.
			      Si binario, muestra actual en valor */

  float intervalo;
  double media; /* Valor medio del fichero */

  struct tipo_tabla *sig;
};
/* BUFFER TEMPLATE */

struct buf
{
  struct data *dt; /* pointer to DT in list */
  struct buf *next_dt;
};
/* BUFFER QUEUE HEAD TEMPLATE */

struct b_queues
{
  struct buf *head;     /* pointer to head of buffer */
  struct buf *end;      /* pointer to end of buffer */
  struct buf *dt_start; /* place to start looking in buffer */
};
/* EVENT QUEUE HEAD TEMPLATE */

struct e_queues
{ /* each event queue has pointer to head and end */
  struct event *head;
  struct event *end;
};
/* NETWORK STRUCTURE */

struct net_info
{ /* info kept for network */

  /* NETWORK EVENT SCHEDULING INFORMATION */

  double *clock_ptr;                     /* pointer to network's clock */
  int clk_index;                         /* simulated cpu # of this process */
  int enable;                            /* true or false */
  int prio_tab[NUM_NET_EVENTS];          /* network event priority table */
  int net_from_sn_intf;                  /* count of number of msgs in sn->net interface */
  struct event *event_q_head;            /* head of total list of events */
  struct e_queues n_queue[MAX_PRIORITY]; /* an event queue for each
						   priority */
  float s_interface_free;                /* time when net-sn interface is next free */
  float sn_int_notif;                    /* time to notify net-sn IPC */
  float msg_pass_time_oh;                /* net-media IPC time - overhead */
  float msg_pass_time_po;                /* net-media IPC time - per octet */
  int net_head;                          /* net headers */
  /* NETWORK BUFFERS */
  int send_q_size; /* current send queue size */
  long perdidas;   /* mensajes perdidos en network fuente por cola
				   llena */
  /* NETWORK STATISTICS */
  long sent_net_data_bytes;
  long sent_net_header_bytes;
  long sent_net_messages;

  long recv_net_data_bytes;
  long recv_net_header_bytes;
  long recv_net_messages;
};

/* MEDIA STRUCTURES */

struct media_info
{
  int medium_type; /* Type of medium - Link, Contention Bus, */
  /* Token Bus */
  float error_rate;   /* bit error rate on this medium */
  float link_speed;   /* transmission speed on this medium */
  float prop_delay;   /* network propagation delay */
  int crc_on;         /* 1=TRUE if doing crc checking */
  struct enode *head; /* block error probability list head & tail */
  struct enode *tail;
  float byte_ok_prob; /* probability of an error free byte */
  double byte_count;  /* count of bytes sent on the medium */
  int no_of_stations; /* number of background stations */
  /* zero is infinite number of stations */
  int preamble; /* preamble of the subnet frames */
  /* The token */
  float token_length;          /* length of token in milleseconds */
  float fddi_head_end_latency; /* Delay in the remodulator */
  float token_time;            /* Tiempo que tarda el testigo en ir de una */
                               /* estacion a la siguiente */
                               /* anhadido para la v. reducida */

  /* Los siguientes campos ha sido anhadidos para FDDI-II */

  float packet_bandwidth;   /* BW for packet traffic */
  float iso_bandwidth;      /* BW for circuit-switched traffic */
  float header_bandwidth;   /* BW for cycle header */
  float current_link_speed; /* Usable bandwidth */
  float cs_traffic;         /* Percent. of cs traffic */
  int no_of_calls;          /* # of isochronous calls in course */
  int master;               /* # of master station */
  int channels;             /* WBC asigned to cs traffic */
  float master_delay;       /* LAB buffer delay */
};
/* SUBNET STRUCTURES */

struct Inter_CV_Sample
{
  unsigned long next_results; /* n = 2^k, punto de muestreo */
  double reloj;               /* instante de muestreo (llegada de la tarea n al nodo) */
  double sumW;                /* W incluyendo tarea n */
  double sumTTRT;             /* llegada del testigo al nodo cuando transmito tarea n */
  unsigned long long num_rot; /* numero de vueltas del testigo */
  unsigned long long sumSis;  /* S de todas las tareas anteriores a la n */
  unsigned long num_samples;  /*   numero   de  tareas      "     "  " " */
  unsigned cs_remaining;      /* num. conexiones por actualizar esto */
  unsigned *cs;               /* qu'e     "       "       "       "  */
  unsigned c;                 /* connection number in the array */
  struct Inter_CV_Sample *next;
};

struct trafico
{
  struct distrib *msg;
  struct distrib *arrive;
  double llegada, sig_llegada, intertime, waiting, S; /* usa S en vez de L en GG1 */
  long int sig_size, left_size, ant_size;

  /* For CVs total arrivals and total demanded time */
  struct Inter_CV_Sample inter_cv;
  double sumW;
  unsigned long long sumSis;
  unsigned long num_samples;

  double ant_tx;
  unsigned long num_vuelta;

  promedio retardo;
};

struct subnet_info
{
  int subnet_number; /* en fast no se lleva cuenta del numero de subred
			   actual */
  promedio ttrt;
  int media_interface;          /* array # into media_info array */
  double *clock_ptr;            /* pointer to subnet clock */
  int clk_index;                /* simulated cpu# of this process */
  int enable;                   /* module enabled (T or F) */
  int prio_tab[NUM_SNT_EVENTS]; /* subnet event priority table */
  int sn_from_net_intf;         /* count of msgs int the net->sn interface */
  int no_net_msg_sched;         /* flags pending message going to network
			   module */
  int order;                    /* ordering of msgs passed to network */

  double min_ex_event; /*guarda el tiempo de ejecucion del proximo evento de*/
  /* red o subre; para la aceleracion del paso del testigo */
  struct event *event_q_head;             /* head of total list of events */
  struct e_queues sn_queue[MAX_PRIORITY]; /* an event q for each priority */
  /*	MEASURES */
  double tmp_ant_trans; /* tiempo anterior transmision */
  long mem_drops;       /* count of messages dropped due to no buffers */
  long crc_drops;       /* count of messages dropped due to crc errors */
  long tokens;          /* Number of Token Visits to station */
  double last_visit;    /* Time of previous token visit */
  /*	SUBNET QUEUES 	*/
  int send_buf_bound; /* bound on send buffer */
  int send_buf_size;  /* current size of send buffer */
  int rec_buf_bound;  /* bound on receive buffer */
  int rec_buf_size;   /* current size of receive buffer */
  /* 	SIMULATION TIMES */
  double interface_free; /* time when interface is next free */
  double link_free;      /* time when link is next free */

  float write_time;     /* time to write 1 octet to interface */
  float read_buf_time;  /* time to read and buffer 1 octet from
			   interface */
  float proc_send_time; /* time to process a message from the net
			   module */
  float proc_recv_time; /* time to process a message from the media */

  int subnet_head;   /* subnet headers */
  long int max_size; /* maximum data size with this header */

  float token_proc_time;                   /* time to get and release the token */
  int logical_next_station;                /* Next station in the logical ring */
  struct b_queues Rx_Q;                    /* station receiving queue */
  struct b_queues Tx_Q[NO_LEVELS_PRI + 1]; /* station transmit queues */
  float target_time_syn;                   /* target rotation time for synchronous traffic */
  float target_time[NO_LEVELS_PRI - 1];    /*asynchronous target rotation times*/
  /*  El target_time[7] = TTRT para todod los nodos */
  double last_token_time_syn;                /*last time synchronous traffic had the token */
  double last_token_time[NO_LEVELS_PRI];     /* last time the class had the
						   token */
  struct trafico *clases[NO_LEVELS_PRI + 1]; /* para v. reducida */

#ifdef SATELLITE
  struct b_queues Rx_Q; /* subnet receive queue */
#endif

  /* SUBNET STATISTICS */

  long sent_subnet_data_bytes;
  long sent_subnet_header_bytes;
  long sent_subnet_messages;

  long recv_subnet_data_bytes;
  long recv_subnet_header_bytes;
  long recv_subnet_messages;
};
/* CONNECTION RELATION INFORMATION */

struct conn_info
{              /* connection info */
  int c_end1;  /* user # of end1 of connection */
  int c_end2;  /* user # of end2 of connection */
  int between; /* nodos entre end1 y end2 */

  /*if only subnet then:*/
  struct traffic tgen;
  long tot_oct_recvd;
  /**/

  struct Inter_CV_Sample inter_cv;
  double sumW;
  unsigned long long sumSis;
  unsigned long num_samples;

  /* NETWORK SIMULATION TIMES */

  long seq_num;

  long r_pkt_num;
  long r_frame_num;
  double acum_serv_time; /* tiempo de servicio acumulado por las tramas
				 del paquete actualmente en recepci'on */

  long s_pkt_num;   /* numero del paquete en envio */
  long s_frame_num; /* numero (cuenta atr'as) de la trama en envio
			       */
  long r_well_num;  /* numero del ultimo bien recibido */
  int lost;
  long left_size; /* tama~no del paquete que queda por enviar */

  promedio retardo;  /* calculo del retardo medio por conexion */
  promedio perdidas; /* calculo de la probabilidad de intervalo de perdida
			de paquetes */

  float calidad, tolerancia;
  char nombre_fich[LINELENGTH]; /* variables para inicializar el promedio en
				    fast.c, en caso de estar utilizando el
				    metodo CV con v. de ctrl. t. rotacion del
				    testigo */

  int tpdu_size;     /* maximum tpdu size */
  int tp_cksum_on;   /* tp checksumming on 1=YES,0=NO */
  int norm_seq_size; /* normal sequence number size 1=YES,0=EXTENDED */
  int ak_head;       /* acknowledgement header size */
  int xak_head;      /* expedited acknowledgement header size */
  int dt_head;       /* normal data header size */
  int xdt_head;      /* expedited data header size */
#ifndef NBS_EXP
#ifndef OSI_EXP
  int mark_size; /* size of mark in normal data stream */
#endif
#endif
#ifdef SELECT_ACK
  int sel_ack_size; /* size of selective acknowledgement */
#endif
  int qos;                      /* Quality of service for this connection */
  int command_cnt;              /* VIRTUAL_TERM, count of type2 msgs per type1
				   msg */
  struct distrib *command_dist; /* VIRTUAL_TERM, command size distrib */

  struct msg_delay_stats *one_way_del_msg;  /* one-way delay per msg */
  struct oct_delay_stats *one_way_del_oct;  /* one-way delay per octet */
  struct msg_delay_stats *two_way_del_msg1; /* two-way delay stats
						   query-response */
  struct msg_delay_stats *two_way_del_msg2; /* type 2 query-response pair */
  struct oct_delay_stats *two_way_del_oct1; /* two-way delay per octet */
  struct oct_delay_stats *two_way_del_oct2;
};
/* TSDU INFORMATON */

struct tsdu_info
{ /* tsdu info kept for metrics */
  int uses;
  long int size;
  long int qr_size; /* size of query-response combo. */
  float s_time;     /* time 1st idu of tsdu is sent from user to tp */
  float qs_time;    /* query send time - for query-response appl */
  float b_time;     /* begin time for receive delay stats */
  int type;         /* type of TSDU - for query-response appl */
};
/* EVENT STATISTICS TEMPLATE */

struct event_stats
{
  long int sched;
  long int exec;
  long int can;
  float q_delay;
  float ser_time;
};
/* METRICS */

struct meas
{ /* measures kept for graphics postprocessing */
  float user_thrpt;
  float thrpt_eff;
  float chan_eff;
  float chan_util;
  float dt_ak_ratio;
  float prot_eff;
  int total_retrans;
  int maxi_retrans;
  float rate_retrans;
  float avg_delay_oct;
  float min_delay_oct;
  float max_delay_oct;
  float power;
  float coeff_var;
  float dt_tpdu_ratio;
};
/* ACCUMULATORS FOR LATER CALCULATION OF METRICS */

struct measures
{
  long rxdts;                      /* retransmitted XDTs */
  long oxdts;                      /* original XDTs */
  long aks;                        /* total AKs */
  long xaks;                       /* total XAKs */
  long rdts;                       /* retransmitted DTs */
  long odts;                       /* original DTs */
  long ak_bits;                    /* total AK bits */
  long xak_bits;                   /* total XAK bits */
  long dt_bits;                    /* total DT bits */
  long xdt_bits;                   /* total XDT bits */
  long info_bits;                  /* total user information bits */
  double nhdr_bits;                /* total network header bits */
  double net_messages;             /* total network messages sent */
  double ndata_bits;               /* total network data bits sent */
  double snhdr_bits;               /* total subnet header bits sent */
  double subnet_messages;          /* total subnet messages sent */
  double sndata_bits;              /* total subnet data bits sent */
  long histo_retrans[BUCKETS + 1]; /* histogram of DT retransmssions */
  long retrans_max;                /* maximum times a individual DT was resent */
  long dt_errors;                  /* total number of damaged DTs */
  long xdt_errors;                 /* total number of damaged XDTs */
  long ak_errors;                  /* total number of damaged AKs */
  long xak_errors;                 /* total number of damaged XAKs */
  long dt_drops;                   /* total number of dropped DTs */
  long xdt_drops;                  /* total number of dropped XDTs */
  long ak_drops;                   /* total number of dropped AKs */
  long xak_drops;                  /* total number of dropped XAKs */
  long release_waits;              /* number of times TP suspended during AK
				   processing */
  long tx_waits;                   /* number of times TP suspended awaiting a
				   network TX Queue slot */
  long normal_waits;               /* number of times TP suspended normally */
};
/* DELAY STATISTICS */

struct oct_delay_stats
{

  float min_oct_delay;      /* minimun octet delay */
  float max_oct_delay;      /* maximum octet delay */
  float sum_of_squares_oct; /* sum for variance computation */
  float total_delays;       /* sum of total delays */
  float total_sizes;        /* sum of total sizes */
  int tcount;               /* count of TSDUs */
};

struct msg_delay_stats
{

  float min_msg_delay;      /* minimum message delay */
  float max_msg_delay;      /* maximum message delay */
  float sum_of_squares_msg; /* sum for variance computation */
  float tot_delays;         /* total delays */
  int count;                /* count of TSDUs */
  long tot_sizes;           /* total messages sizes */
  long min_msg_size;        /* minimum message size */
  long max_msg_size;        /* maximum message size */
};
/**   STRUCTURE FOR ERROR PROBABILITY LOOKUPS  **/

struct enode
{
  int size;
  long cutoff;
  struct enode *prev;
  struct enode *next;
};
/* MEMORY UTILIZATION */

struct mem_util_info
{

  float *tsbuf;     /* pointer to send buffer utilization map */
  float *trbuf;     /* pointer to receive buffer utilization map */
  double tsbclock;  /* last time send buffer use changed */
  double trbclock;  /* last time receive buffer use changed */
  long int tsbsize; /* current send buffer space in use */
  long int trbsize; /* current receive buffer space in use */
  long int tsbmax;  /* maximum send buffer space used */
  long int trbmax;  /* maximum receive buffer space used */
  int tsbnum;       /* number of buckets in send buffer map */
  int trbnum;       /* number of buckets in receive buffer map */
};

/* PROTOTIPOS DE FUNCIONES */

/* main.c */

void metric_clock_adjust();

/* commands.c */

char *sfind(char *, char *);
void sucase(char *s);
int sequal(char *, char *);
#if #system(amigaos) || #system(NetBSD)
void sig();
#else
void sig(int);
#endif
void tpause(char *msg);
void print_elapsed(FILE *fp);
void print_meas(FILE *fp);

/* distrib.c */

double uniform(float a, float b);
void semilla_aleatorio(long);
long aleatorio();
double expont(float mean);
int bernoulli(float prob1);
double normal(float mean, float std_dev);
int poisson(float mean);
double Gamma(float shape, float scale, double *pa, double *pb);
double triangular(float a, float b, float c);
double tiempo_triangular(struct distrib *dist, double tiempo_anterior,
                         int longitud);
double tabular(struct distrib *dist, double tiempo_anterior);
double file_smpls(struct distrib *p);

void binary_control(void);
double binary_file(struct distrib *p);

double velocidad_uniforme(struct distrib *dist, double tiempo_anterior,
                          int longitud);
double sincrono(struct distrib *dist, double ejecucion_anterior);
double sriram86(float p, float T, float media_silencio);
double brady69(float media_silencio, float media_spurt, float T,
               double *paquetes, int *silencio, int *primer_paquete);
double gruber82(float r, float r1, float c1, float r2, float c2,
                float T, int *silencio);
double riyaz95(float I1, float I2, float P1, float P2, float B1,
               float B2, float p, double *cambio_escena, double *GOPde10,
               int *tramas, int *tramasB, int *fin);
void prioridades_riyaz95(int PBP, double muestra, double *alta,
                         double *baja);
double krunz95(float mI, float dI, float mP, float dP, float mB,
               float dB, int *tramas, int *tramasB);
double yegenoglu92(int *p_estado, double *muestra1, double *muestra2,
                   double *muestra3, double a1, double a2, double a3,
                   double m1, double m2, double m3, double d1,
                   double d2, double d3);
double torbey91(float T, float media_off, float p1, float p2,
                int uniforme, double *numero, int *on, int *primera_vez);
//double tam_pkt_smtp();
//double tam_pkt_nntp();
double lognormal(float media, float desv);
double Pareto(float k, float m);
double Weillbull(float a, float b);
//double tiempo_telnet_1 (float media_inicio, double *tiempo_siguiente,
//		       double *duracion, int *primer_byte);
//double tiempo_telnet_2 (float media_inicio, float m_tam, float d_tam,
//		       double *tiempo_siguiente, int *primer_byte,
//		       int *tamano);
//double tiempo_ftp (float media_inicio, float T, float m_off2,
//		  float d_off2, double *flag, double *inicio_siguiente);
//double pkt_ftp(float MTU, float cab, double *inicio_tx_item,
//	      double *inicio_ses_ftp, double *flag, int *pkt_final,
//	      int *n_pkts, int *items);
double doble_determinista(float valor1, float valor2, float p);

/* distrib.c: parte heredada de la herramienta chd */

double uni_inv(double y, double a, double b);
double uni_inv_diff(double y, double a, double b);
double F_uni(double x, double a, double b);
double exp_inv(double y, double m);
double exp_inv2(double y, double m, double nada);
double F_exp(double x, double m);
double F_exp2(double x, double m, double nada);
double G_inv(double p);
double nor_inv(double y, double m, double desv);
double F_nor_0_1(double x, double nada, double nada2);
double F_nor(double x, double m, double desv);
double lognor_inv(double y, double m, double desv);
double lognor_nor_0_1(double y, double m, double desv);
double F_log_nor(double x, double m, double desv);
double wei_inv(double y, double alfa, double beta);
double F_wei(double x, double alfa, double beta);
double gamln(double z);
double beta(double z, double w);
double gauinv(double p);
double gamain(double x, double a);
double gaminv(double p, double V);
double gam_inv(double y, double alfa, double beta);
double F_gam(double x, double alfa, double beta);
double betacf(double x, double a, double b);
double betai(double x, double a, double b);
double betinv(double ALPHA, double P, double Q);
double F_bet(double x, double alfa, double beta);
double bet_inv(double y, double alfa, double beta);
double F_pareto(double x, double alfa, double k);
double pareto_inv(double y, double alfa, double k);
double F_poisson(unsigned x, double tasa);
double F_poisson2(double x, double tasa, double nada);

/* init.c */

void init(char *file);

/* event.c */

void enable_mods();
struct event *schedule(int type, int event_num, double ex_time,
                       struct data *data_ptr, int num);
struct event *getevent(int type, int num, struct event *cur_event);
struct data *remove_event(struct event *ep, int num, int type);
void enable_subnet(int sn);
void exec_event(int mod, int num, int cid, struct event *eptr,
                float duration);
void enable_net(int nn);

/* fast.c */

void fast_fddi(char *fn);
void fast_metrics(char *fn);

/* gg1.c */

void gg1_node(char *fn);
void gg1_metrics(char *fn);

/* metrics.c */

void metrics(char *fn);
void imprime(const char *nombre_fichero, const char *fn);
void cpu_util(FILE *fp);

/* print.c */

void p_error(char *s);
void print_ci(FILE *fp, int inst);
void print_mi(FILE *fp, int inst);
void print_ni(FILE *fp, int inst);
void print_si(FILE *fp, int inst);

/* fddi.c */

void subnet(int sn_no);
int channel_allocator(struct media_info *media_struct, int nets);

/* stats.c */

void upd_ns_stats(int net_no, int data, int header, int conn);
void upd_nr_stats(int net_no, int data, int header);
void upd_sns_stats(int sn_no, int data, int header, int conn);
void upd_snr_stats(int sn_no, int data, int header);
void upd_sn_drops(struct data *msg, int conn);
void upd_errs(struct data *dp, int conn);
void cpu_use(int processor, float amount);

/* util.c */

double Q_msg(int net_no, int conn, struct traffic *trafico);
int net_to_sn_int_space_avail(int sn_no);
void free_data(struct data *dp);
int found_error(int blk_size, int media_no);
int sn_to_net_int_space_avail(int n_no);
double get_distrib_val(struct distrib *p, double ejecucion_anterior,
                       int longitud);
void network(int net_no);

#endif