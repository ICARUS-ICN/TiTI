/*  VERSION 1.0 - NBS

	file - commands.c

	This file contains functions that comprise the user interface to the
	model.  This includes the processing of software interrupts from
	the terminal and voluntary pauses within the model execution.  Several
	functions for general string processing are included.  Several
	functions are also included that print information to a file (i.e.
	the terminal) but many other printing functions are included in
	the file print.c.  The printing functions in the present file
	were completed later and those in print.c were adapted for general
	printing to any kind of file.

	The main line of the user interface is tpause().  There are also
	functions for each command List(), Set(), Cverbose(), Cdebug(),
	and Help().
*/
char CommandsSid[] = "@(#)commands.c	4.1 10/02/97";
#include <stdio.h>
#include <math.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef linux
#include <time.h>
#else
#include <sys/times.h>
#endif
#include "defs.h"
#include "types.h"
#include "globals.h"
#define  ERROR_FILE	"simerror.log"

static int finaliza=0; /* esta variable se pone a 1 cuando se pasa de interactiva a background */

/*
	Catch a signal from the terminal and pass control to the
	pause point processor. sig handles ^C when model is running. sig2
	handles ^C when a pause point is being processed (i.e. sig2 is active
	only when you are already in tpause).
*/

#if #system(amigaos) || #system(NetBSD)
void sig()
#else
     void sig(int i)
#endif
{
  pause_point = ON;
  signal(SIGINT, sig);
  return;
}

#if #system(amigaos) || #system(NetBSD)
static void sig2()
#else
     static void sig2(int i)
#endif
{
  stop_print = ON;
  signal(SIGINT, sig2);
  return;
}
/*
string processing functions used in the model by main and the user interface.

	sucase
	sequal
	sfind
	sskip
	stcmp
	prompt
	slen
*/

void sucase(char *s)
{
  while (*s)
    {
      if (*s >= 'a' && *s <= 'z')
	*s = *s - 'a' + 'A';
      s++;
    }
}


int sequal(char *s1, char *s2)
{
  for (; *s1 == *s2; s1++, s2++)
    if (!(*s1))
      return (1);
  return (0);
}


char *sfind(char *ts, char *ps)
{
  char *ss;
  while (*ts)
    {
      ss = ps;
      while (*ss)
	{
	  if (*ts == *ss)
	    return (ts);
	  ss++;
	}
      ts++;
    }
  return (ts);
}

static char *sskip(char *ts, char *ps)
{
  char *ss;
  ss = ps;
  while (*ss)
    {
      if (*ts == *ss)
	{
	  ts++;
	  ss = ps;
	}
      ss++;
    }
  return (ts);
}

static int stcmp(char *s1, char *s2)
{
  for (; *s1 == *s2; s1++, s2++)
    if (!(*s1))
      return (0);
  return (*s1 - *s2);
}

static void sout(char *s)
{
  while (*s)
    putc(*s++, stdout);
}

static int slen(char *s)
{
  int count = 0;
  while (*s++)
    count++;
  return (count);
}

static int prompt(char *out, char *in)
{
  char *buf;
  buf = in;
  sout(out);
  while ((*buf = getc(stdin)) != '\n')
    buf++;
  *buf = '\0';
  return (slen(in));
}

/*
	Data for terminal command processor.
*/

#define MIN_UNQ		3
#define	NOT_FOUND	-1
#define CESTATS		3
#define CLOCK		4
#define CONTINUE	5
#define CPU		6
#define CRASH		7
#define DEBUG		8
#define EVENTS		12
#define HELP		13
#define LIMITS		15
#define LIST		16
#define NINFO		20
#define OCTETS		21
#define RETRANS		22
#define SET		23
#define STATUS		24
#define STOP		25
#define TESTATS		26
#define TINFO		28
#define UESTATS		29
#define UINFO		30
#define VERBOSE		31
#define YES		32
#define ABORT		33
#define PARAM		34
#define RETURN		22
#define MEASURES	35
#define XPDS		36
#define SINFO		37
#define SESTATS		42
#define MINFO		43
#define CINFO		44
#define ERRORS		45
#define MESTATS		46
#define IBA             47 /* indica cambio de interactiva a background */

#define MAX_BUF	132

char *white = " \t";		/* white space */

/*

	COMMAND KEYWORD TABLE

	THIS TABLE IS USED TO LOOKUP THE REQUESTED KEYWORD AND RETURN THE
	CODE.  A BINARY SEARCH IS PERFORMED ON THIS TABLE; THEREFORE, THE
	ENTRIES MUST BE IN ALPHABETICAL ORDER BY KEYWORD STRING.
*/

static struct ktab
{
  char *object;
  int code;
} keys[] = {
  {"!", VERBOSE},
  {"$", DEBUG},
  {"-", LIST},
  {".", CONTINUE},
  {"=", SET},
  {"?", HELP},
  /*   {"AB", ABORT},
   {"ABO", ABORT},*/
  {"B", STOP},
  {"BY", STOP},
  {"BYE", STOP},
  {"CE", CESTATS},
  {"CES", CESTATS},
  {"CH", SET},
  {"CHA", SET},
  {"CI", CINFO},
  {"CIN", CINFO},
  {"CL", CLOCK},
  {"CLK", CLOCK},
  {"CLO", CLOCK},
  {"CO", CONTINUE},
  {"CON", CONTINUE},
  {"CP", CPU},
  {"CPU", CPU},
  {"DEB", DEBUG},
  {"EL", CLOCK},
  {"ELA", CLOCK},
  {"EN", STOP},
  {"END", STOP},
  {"ER", ERRORS},
  {"ERR", ERRORS},
  {"EV", EVENTS},
  {"EVE", EVENTS},
  {"EX", STOP},
  {"EXI", STOP},
  {"H", HELP},
  {"HE", HELP},
  {"HEL", HELP},
  {"IBA", IBA},
  {"LIM", LIMITS},
  {"LIS", LIST},
  {"MEA", MEASURES},
  {"MI", MINFO},
  {"MIN", MINFO},
  {"NI", NINFO},
  {"NIN", NINFO},
  {"OC", OCTETS},
  {"OCT", OCTETS},
  {"OF", OFF},
  {"OFF", OFF},
  {"ON", ON},
  {"PA", PARAM},
  {"PAR", PARAM},
  {"PR", LIST},
  {"PRI", LIST},
  /*  {"Q", ABORT},
   {"QU", ABORT},
   {"QUI", ABORT},*/
  {"R", RETRANS},
  {"RE", RETRANS},
  {"RET", RETRANS},
  {"SES", SESTATS},
  {"SET", SET},
  {"SH", LIST},
  {"SHO", LIST},
  {"SI", SINFO},
  {"SIN", SINFO},
  {"STA", STATUS},
  {"STO", STOP},
  {"TES", TESTATS},
  {"TI", TINFO},
  {"TIN", TINFO},
  {"TY", LIST},
  {"TYP", LIST},
  {"UES", UESTATS},
  {"UI", UINFO},
  {"UIN", UINFO},
  {"UP", SET},
  {"UPD", SET},
  {"VE", VERBOSE},
  {"VER", VERBOSE},
  {"VI", LIST},
  {"VIE", LIST},
  {"W", CLOCK},
  {"WA", CLOCK},
  {"WAL", CLOCK},
  {"X", XPDS},
  {"XP", XPDS},
  {"XPD", XPDS}
};

#define	NKEYS	(sizeof(keys) / sizeof(struct ktab))

/*
	BINARY SEARCH OF KEYWORD TABLE
*/

static int Lookup(char *s)
{
  int cond;
  struct ktab *low = &keys[0];
  struct ktab *high = &keys[NKEYS - 1];
  struct ktab *mid;
  while (low <= high)
    {
      mid = low + (high - low) / 2;
      if ((cond = stcmp(s, mid->object)) < 0)
	high = mid - 1;
      else if (cond > 0)
	low = mid + 1;
      else
	return (mid->code);
    }
  return (NOT_FOUND);
}

/*
	Print routines for limits & status of model execution.
*/

static void print_limits(FILE *fp)
{

  if (anal_est)
    fprintf(fp, "La Simulacion finalizara cuando\nse obtengan los intervalos de confianza\ncon los margenes de error pedidos\npara toda las variables.\n");

  else
    {
      fprintf(fp, "\n\nLimite de reloj = %d %s\n",
	      clk_limit / MILLISEC,(clk_limit/MILLISEC)>1 ? "segundos":"segundo");

      /*	fprintf(fp, "CPU limit = %d secs.\n",
		cpu_limit / (MILLISEC / 10));

		fprintf(fp, "Octet receipt limit = %d.\n", oct_limit);

		fprintf(fp, "Damaged message limit = %d.\n", err_limit);

		fprintf(fp, "Event execution limit = %d.\n", evt_limit); */
    }
}

static void print_status(FILE *fp)
{
  long now;
  long elapsed;
  long days = 0;
  long hours = 0;
  long mins = 0;
  long secs = 0;

  times(&sys_time);

  fprintf(fp, "\n\nTiempo de simulacion = %.3f secs.\n",
	  sim_clock / MILLISEC + .0005);

  fprintf(fp, "Tiempo de CPU usado= %.3f.\n",
	  ((float) ((sys_time.tms_utime + sys_time.tms_stime)
		    - (start_cpu.tms_utime + start_cpu.tms_stime)))
	  / HZ + .0005);

  /*    fprintf(fp, "Octets received = %d.\n", octs_rcvd);

	fprintf(fp, "Damaged messages = %d.\n", err_cnt);

	fprintf(fp, "Events executed = %d.\n", evts_exec);

	fprintf(fp, "Total retransmissions = %d.\n", retrans);

	fprintf(fp, "Expedited data count = %d.\n", xpd_cnt);
	*/
  fprintf(fp, "Modo Verbose es %s.\n",
	  (verbose ? "ON" : "OFF"));

  fprintf(fp, "Modo Debug es %s.\n",
	  (debug ? "ON" : "OFF"));

  /*    fprintf(fp, "Output mode is %s.\n",
	(omode ? "GRAPHICS" : "NONE"));

	fprintf(fp, "Queues %s being saved.\n",
	(oqueues ? "ARE" : "ARE NOT"));   */
  if (anal_est )
    fprintf(fp,"Transitorio = %d.\n",clk_limit);
  else
    fprintf(fp,"Transitorio = 0.\n");
    
  time(&now);
  elapsed = now - wtime;

  days = elapsed / (24 * 60 * 60);

  elapsed = elapsed % (24 * 60 * 60);

  hours = elapsed / (60 * 60);

  elapsed = elapsed % (60 * 60);

  mins = elapsed / 60;

  secs = elapsed % 60;

  fprintf(fp, "Duracion de la ejecucion:\t");

  if (days)
    fprintf(fp, "%ld dias ", days);

  if (hours)
    fprintf(fp, "%ld horas ", hours);

  if (mins)
    fprintf(fp, "%ld minutos ", mins);

  fprintf(fp, "%ld segundos.\n\n", secs);
}

void print_meas(FILE *fp)
{
  int i;
  /*    float Tnet_messages, Tnhdr_bits, Tndata_bits, Tsubnet_mess, Tsnhdr_bits;
	double Tsndata_bits;
	long Tdt_drops, Tak_drops, Txdt_drops, Txak_drops;
	int trials;


	trials = 0;



	Tnet_messages = 0;
	Tnhdr_bits = Tndata_bits = Tsubnet_mess = Tsnhdr_bits = 0;
	Tsndata_bits = Tdt_drops = Tak_drops = Txdt_drops = Txak_drops = 0;

	for (i = 0; i < no_conns; i++)
	{
	k = start_cid + i;

	Tnet_messages += stat[k].net_messages;
	Tnhdr_bits += stat[k].nhdr_bits;
	Tndata_bits += stat[k].ndata_bits;
	Tsubnet_mess += stat[k].subnet_messages;
	Tsnhdr_bits += stat[k].snhdr_bits;
	Tsndata_bits += stat[k].sndata_bits;
	Tdt_drops += stat[k].dt_drops;
	Tak_drops += stat[k].ak_drops;
	Txdt_drops += stat[k].xdt_drops;
	Txak_drops += stat[k].xak_drops;
	}

	fprintf(fp, "\nNETWORK MEASURES\n");
	fprintf(fp, "NET_MSGs %.0f\tNHDR_bits %.0f\tNDATA_bits %.0f\n",
	Tnet_messages, Tnhdr_bits, Tndata_bits);

	fprintf(fp, "\nSUBNET MEASURES\n");
	fprintf(fp, "SUBNET_MSGs %.0f\tSNHDR_bits %.0f\tSNDATA_bits %.0f\n",
	Tsubnet_mess, Tsnhdr_bits, Tsndata_bits);
	fprintf(fp,"DT_DROPs %d\tAK_DROPs %d\tXDT_DROPs %d \tXAK_DROPs %d\n",
	Tdt_drops,Tak_drops,Txdt_drops,Txak_drops);*/


  for (i = 0; i < no_of_media; i++)
    {
      
      if (fast)
	fprintf(fp, "\nThroughput Medio: %.3f bps.\n",
		((m[i].byte_count * 8) / (sim_clock))
		* MILLISEC);
      else
	{
	  metric_clock_adjust(); 

	  if (metric_clock)
	    {
	      fprintf(fp, "\nThroughput Medio: %.3f bps.\n",
		      ((m[i].byte_count * 8) / (metric_clock))
		      * MILLISEC);
	    }
	}
    }
  fprintf(fp, "\n");
}

static void print_meas2(FILE *fp)
{
  int cid, nodo_trt, nodo, pri;

  if(fast==0)
    {
      for (cid=0; cid<no_conns; cid++)
	{
	  if ( c[cid].retardo )
	    {
	      fprintf(fp,"\nCONEXION %d\n", cid);
	      estado_promedio(c[cid].retardo);
	    }
	}
      for(nodo_trt=0; nodo_trt<no_of_nets; nodo_trt++)
	if ( s[nodo_trt].ttrt )
	  {
	    fprintf(fp,"\nTRT medio, NODO %d:\n",nodo_trt);
	    estado_promedio(s[nodo_trt].ttrt);
	  }
    }
  else
    {    /* fast = True */
      for(nodo=0; nodo<no_of_nets; nodo++)
	{  
	  for( pri=0; pri<5; pri++)
	    {
	      if( s[nodo].clases[pri] )
		if( (s[nodo].clases[pri])->retardo)
		  {
		    if (pri==0)
		      printf("\n\nNODO %d", nodo);
		    fprintf(fp,"\nPRIORIDAD %d", pri);
		    estado_promedio(s[nodo].clases[pri]->retardo);
		  }
	    }
	}
    } 
}

/*
	LIST VERB PROCESSOR
*/

static void List(char *buf)
{
  char *token, *next;
  int object;
  int value;
  /* isolate token */

  token = sskip(buf, white);
  next = sfind(token, white);
  if (*next)
    {
      *next = '\0';
      next++;
    }

  /* shorten token to 3 characters */

  if (slen(token) > MIN_UNQ)
    *(token + 3) = '\0';

  /* Lookup object */

  object = Lookup(token);

  /* list object */

  switch (object)
    {
      /*      case CLOCK:
	      MUESTRA EL RELOJ EN PANTALLA
	      printf("\n\n");
	      print_elapsed(stdout);
	      break;
	      */
    case CINFO:

      value = atoi(next);
      print_ci(stdout, value);
      break;

    case CPU:

      cpu_util(stdout);
      break;

    case LIMITS:

      print_limits(stdout);
      break;

    case MEASURES:

      /* print raw accumulators */

      print_meas(stdout);
      print_meas2( stdout );/*----*/
      break;

    case MINFO:

      value = atoi(next);
      print_mi(stdout, value);
      break;

    case NINFO:

      value = atoi(next);
      print_ni(stdout, value);
      break;

    case PARAM:

      printf("\n\nParameter file is %s.\n", m1);
      break;

    case SINFO:

      value = atoi(next);
      print_si(stdout, value);
      break;

    case STATUS:

      print_status(stdout);
      break;

    default:

      sout("\nDesconocido objecto!\n");
      break;
    }
}

/*
	SET VERB PROCESSOR
*/

static void Set(char *s)
{
  char *token, *next;
  int object;
  token = sskip(s, white);
  next = sfind(token, white);
  if (*next)
    {
      *next = '\0';
      next++;
    }

  if (slen(token) > MIN_UNQ)
    *(token + 3) = '\0';

  object = Lookup(token);

  switch (object)
    {
    case CLOCK:

      clk_limit = (atoi(next) * MILLISEC);
      break;

    case CPU:

      cpu_limit = (atoi(next) * (MILLISEC / 10));
      break;

    case ERRORS:

      err_limit = atoi(next);
      break;

    case OCTETS:

      oct_limit = atoi(next);
      break;

    case EVENTS:

      evt_limit = atoi(next);
      break;

    default:
       
      sout("\ncomando desconocido!\n");
      /*sout("\nunknown object!\n");*/
      break;
    }
}

/*
	VERBOSE VERB PROCESSOR
*/

static void Cverbose(char *s)
{
  char *token, *next;
  int object;
  token = sskip(s, white);
  next = sfind(token, white);
  if (*next)
    {
      *next = '\0';
      next++;
    }

  if (slen(token) > MIN_UNQ)
    *(token + 3) = '\0';

  object = Lookup(token);

  switch (object)
    {
    case ON:

      verbose = ON;
      break;

    case OFF:

      verbose = OFF;
      break;

    default:

      printf("\nModo Verbose esta %s\n",
	     (verbose ? "ON" : "OFF"));
      break;
    }
}

/*
	DEBUG VERB PROCESSOR
*/

static void Cdebug(char *s)
{
  char *token, *next;
  int object;
  token = sskip(s, white);
  next = sfind(token, white);
  if (*next)
    {
      *next = '\0';
      next++;
    }

  if (slen(token) > MIN_UNQ)
    *(token + 3) = '\0';

  object = Lookup(token);

  switch (object)
    {
    case ON:

      debug = ON;
      break;

    case OFF:

      debug = OFF;
      break;

    default:

      printf("\nModo Debug esta %s\n",
	     (debug ? "ON" : "OFF"));
      break;
    }
}

static void cont_help()
{
  static char *more =
    "Vuelve a la ejecucion de la Simulacion.  Cualquier cambio hecho tendra efecto.";
  static char *syn =
    "Sinonimos son: RETURN	. (punto)";
  printf("\n%s\n\n%s\n", more, syn);
}

static void debug_help()
{
  static char *more =
    "Cuando debug esta activado, se muestran por salida standard eventos importantes.";
  static char *also =
    "Se escribe tambien algo extra al fichero de error.";
  static char *syn =
    "Sinonimos son: $ (simbolo dollar)";
  printf("\n%s\n%s\n\n%s\n", more, also, syn);
}

static void list_help()
{
  static char *more1 =
    "<OBJECT> puede ser uno de los siguientes:";
  static char *more3 =
    "CES[TATS] <VALUE>   Estadisticas de lo eventos por conexion.";
  static char *more29 =
    "CIN[FO]   <VALUE>   Informacion especifica de conexion.";
  static char *more4 =
    "CLO[CK]             Tiempo transcurrido desde el inicio del programa.";
  static char *more5 =
    "CPU                 Medidas de utilizacion de CPU.";
  /*  static char *more6 =
    "DEL[AY]             Medidas de retardo.";*/
  static char *more10 =
    "LIM[ITS]            Limites del modelo de ejecucion.";
  static char *more22 =
    "MEA[SURES]          Raw accumulators.";
  static char *more11 =
    "MEM[ORY]            Medidas de utilizacion de memoria.";

  static char *more30 =
    "MIN[FO]   <VALUE>   Media information.";
  static char *more12 =
    "NES[TATS] <VALUE>   Estadisticas de los eventos para el modulo de red.";
  static char *more13 =
    "NIN[FO]   <VALUE>   Informacion del modulo de red.";
  static char *more21 =
    "PAR[AM]             Nombre del fichero de parametros.";
  static char *more25 =
    "SES[TATS] <VALUE>   Estadisticos de eventos de la subred.";
  static char *more26 =
    "SIN[FO]   <VALUE>   Informacion del modulo de la subred.";
  static char *more15 =
    "STA[TUS]            Estado de los modos y limites del modelo de ejecucion.";
  static char *more16 =
    "TES[TATS] <VALUE>   Estadisticas de eventos para el modulo de transporte.";
  static char *more17 =
    "THR[OUGHPUT]        Throughput, Utilizacion de Canal, & Efficiencies.";
  static char *more18 =
    "TIN[FO]   <VALUE>   Informacion del modulo de transporte.";
  static char *more19 =
    "UES[TATS] <VALUE>   Estadisticas de eventos para el modulo de usuario.";
  static char *more20 =
    "UIN[FO]   <VALUE>   Informacion del modulo de usuario.";
  static char *syn =
    "Synonyms are: PRINT SHOW TYPE VIEW - (dash)";

  char line[10];
  /* PRINT PAGE ONE */

  printf("\n%s\n\n", more1);
  printf("%s\n", more3);
  printf("%s\n", more29);
  printf("%s\n", more4);
  printf("%s\n", more5);
  printf("%s\n", more10);
  printf("%s\n", more22);
  printf("%s\n", more11);

  printf("%s\n", more30);
  printf("%s\n", more12);
  printf("%s\n", more13);

  /* WAIT */

  prompt("\nMORE <PULSA RETURN>\n", &line[0]);

  /* PRINT PAGE TWO */

  printf("%s\n", more21);
  printf("%s\n", more25);
  printf("%s\n", more26);
  printf("%s\n", more15);
  printf("%s\n", more16);
  printf("%s\n", more17);
  printf("%s\n", more18);
  printf("%s\n", more19);
  printf("%s\n", more20);
  printf("\n%s\n", syn);
}

static void set_help()
{
  static char *more =
    "El modelo de ejecucion parara y te dara el control cuando un limite sea alcanzado.";
  static char *more1 =
    "Limites estan disponibles para <OBJECT> el cual puede ser uno de los siguientes:";
  static char *more2 =
    "CLO[CK]      Pone el limite de tiempo simulado en segundos.";
  static char *more3 =
    "CPU          Pone el limite de tiempo real de CPU en segundos.";
  /*
  static char *more4 =
    "OCT[ETS]     Set simulated octets received limit.";
  static char *more9 =
    "ERR[ORS]     Set simulated damaged message limit.";
  static char *more5 =
    "EVE[NTS]     Set simulation event execution limit.";
  static char *more7 =
    "MAX[RETRAN]  Set limit on simulated retransmissions for an individual message.";
    */
  static char *syn =
    "Sinonimos son: CHANGE UPDATE = (igual)";
  printf("\n%s\n", more);
  printf("\n%s\n", more1);
  printf("\n%s\n", more2);
  printf("%s\n", more3);
  /*    printf("%s\n", more4);
	printf("%s\n", more5);
	printf("%s\n", more9);
	printf("%s\n", more7);*/
  printf("\n%s\n", syn);
}

static void stop_help()
{
  static char *more1 =
    "Finaliza correctamente el programa y escribe cualquier informacion disponible";
  static char *more2 =    "al fichero de medidas de la simulacion, al fichero de error, y, si es adecuado";
  static char *more3 =
    "al fichero de datos graficos.";
  static char *syn =
    "Sinonimos son: BYE END EXIT ";
  printf("\n%s\n", more1);
  printf("%s\n", more2);
  printf("%s\n", more3);
  printf("\n%s\n", syn);
}

static void verbose_help()
{
  static char *more =
    "Cuando el modo verbose esta ON, XDT, DT & IDU arrivals son escritos a salida standard.";
  static char *also =
    "DTs & XDTs en error son tambien visualizados.";
  static char *syn =
    "Sinonimos son: ! (signo de exclamacion)";
  printf("\n%s\n%s\n\n%s\n", more, also, syn);
}

/*
	HELP VERB PROCESSOR
*/

static void Help(char *s)
{
  /*  static char *a =
    "ABO[RT]                      Abrupt exit deleting all files.";*/
  static char *c =
    "CON[TINUE]                   Continua la ejecucion de la simulacion.";
  static char *d =
    "DEB[UG]   ON | OFF           Pone el modo debug  ON u OFF.";
  static char *h =
    "HEL[P]   <VERB>              Obtiene informacion sobre lo indicado <VERB>.";
  static char *l =
    "LIS[T]   <OBJECT> [<VALUE>]  Muestra la informacion de <OBJECT> al pulsar CRT.";
  static char *se =
    "SET      <OBJECT> <VALUE>    Actualiza lo indicado <OBJECT> al numero <VALUE>.";
  static char *st =
    "STO[P]                       Fin correcto escribiendo medidas a un fichero.";
  static char *v =
    "VER[BOSE] ON | OFF           Pone el modo verbose ON u OFF.";
  static char *cc =
    "^C                           Vuelve rapido al nivel de comandos.";


  char *token, *next;
  int object;
  token = sskip(s, white);
  next = sfind(token, white);
  if (*next)
    {
      *next = '\0';
      next++;
    }

  if (slen(token) > MIN_UNQ)
    *(token + 3) = '\0';

  object = Lookup(token);

  switch (object)
    {
      /*      case ABORT:

	      printf("\n\n%s\n", a);
	      abort_help();
	      break;
	      */
    case CONTINUE:

      printf("\n\n%s\n", c);
      cont_help();
      break;

    case DEBUG:

      printf("\n\n%s\n", d);
      debug_help();
      break;

    case LIST:

      printf("\n\n%s\n", l);
      list_help();
      break;

    case SET:

      printf("\n\n%s\n", se);
      set_help();
      break;

    case STOP:

      printf("\n\n%s\n", st);
      stop_help();
      break;

    case VERBOSE:

      printf("\n\n%s\n", v);
      verbose_help();
      break;

    case NOT_FOUND:

      /*	printf("\n\n%s\n", a); ABORT */
      printf("%s\n", c);
      printf("%s\n", d);
      printf("%s\n", h);
      printf("%s\n", l);
      printf("%s\n", se);
      printf("%s\n", st);
      printf("%s\n", v);
      printf("%s\n", cc);
      break;

    default:

      sout("\ncomando desconocido!\n");
      /*	sout("\nunknown object!\n");*/
      break;
    }
}

/*
	This function is the command processor for terminal control of the
	model when it is executed in interactive mode.
*/

void tpause(char *msg)
{
  static char line[MAX_BUF];
  char *token, *next;
  int verb;

  if ( finaliza )
    { 
      if (fast)
	{
	  fast_metrics(m1);
	  p_error("Stopped from terminal");
	}
      else
	{
	  metric_clock_adjust();
	  metrics(m1);
	  p_error("Stopped from terminal");
	}
      /*         finaliza = 0; */
      signal(SIGINT, sig2);
      return;
    }   

  if( strstr ( msg,"Interrupted from the terminal"))
    printf("\n\nInterrumpido desde el terminal\n\n");
  else
    printf("\n\n%s\n\n", msg);

  printf("Simulador de protocolo MAC FDDI - AIT - ETSIT VIGO\n\n");
  /*
    SETUP to process a ^C as command interrupt & return to command level
    */

  signal(SIGINT, sig2);

  /* adjust the metric clock */
    
  if (!fast)
    metric_clock_adjust();

  for (;;)
    {
      /* enable listing at terminal */

      stop_print = OFF;

      /* prompt for user input */

      while (!prompt("\nyes?", &line[0]));

      /* map to upper case */

      sucase(&line[0]);

      /* isolate the first token */

      token = sskip(&line[0], white);
      next = sfind(token, white);
      if (*next)
	{
	  *next = '\0';
	  next++;
	}

      /* shorten token to 3 characters */

      if (slen(token) > MIN_UNQ)
	*(token + 3) = '\0';

      /* Lookup the verb */

      verb = Lookup(token);

      /* process the verb */

      switch (verb)
	{
	  /*	  case ABORT:

		  fclose(efp);
		  #ifdef VMS
		  delete(ERROR_FILE);
		  #endif
		  #ifdef UNIX
		  unlink(ERROR_FILE);
		  #endif
		  exit();
		  */
	case RETURN:
	case CONTINUE:

	  printf("\ncontinue\n");/*--------NUEVO-----------*/
	  signal(SIGINT, sig);
	  pause_point = OFF;
	  /*--------------- CON ESTO SE CONSIGUE CAMBIAR stdout , Y ASI FUCIONA TODO BACKGROUND */
	  if ( finaliza == 1)
	    dup2( 2,1); /* es fundamental cuando se pasa de interactiva a background con */
	  /* las opciones VERBOSE y/o DEBUG activadas                      */ 
	  /*--------------------------*/
	  return;
	  break;

	case DEBUG:

	  Cdebug(next);
	  break;

	case HELP:

	  Help(next);
	  break;

	case IBA:

	  finaliza=1; /* la proxima vez que entre en t_pause() la simulacion finalizara */
	  break;      /* ademas esta variable se mira en CASE CONTINUE para hacer dup2(2,1) */
	  /* que es fundamental cuando se pasa de interactiva a background con */
	  /* las opciones VERBOSE y/o DEBUG activadas                          */ 
	case LIST:

	  List(next);
	  break;

	case SET:

	  Set(next);
	  break;

	case STOP:

	  if (fast)
	    {
	      printf("\nstop\n");/*-------NUEVO--------*/ 
	      fast_metrics(m1);
	      p_error("Stopped from terminal");
	    }
	  else
	    {
	      printf("\nstop\n");/*-------NUEVO--------*/ 
	      metrics(m1);
	      p_error("Stopped from terminal");
	    }
	  break;

	case VERBOSE:

	  Cverbose(next);
	  break;
  
	default:

	  sout("\ncomando desconocido!\n");  
	  /*sout("\nunknown verb!\n");*/
	  break;
	}
    }
}

void print_elapsed(FILE *fp)
{
  long now;
  long elapsed;
  long days = 0;
  long hours = 0;
  long mins = 0;
  long secs = 0;

  time(&now);
  elapsed = now - wtime;

  days = elapsed / (24 * 60 * 60);

  elapsed = elapsed % (24 * 60 * 60);

  hours = elapsed / (60 * 60);

  elapsed = elapsed % (60 * 60);

  mins = elapsed / 60;

  secs = elapsed % 60;

  fprintf(fp, "Duracion de la ejecucion:\t");

  if (days)
    fprintf(fp, "%ld dias ", days);

  if (hours)
    fprintf(fp, "%ld horas ", hours);

  if (mins)
    fprintf(fp, "%ld minutos ", mins);

  fprintf(fp, "%ld segundos.\n\n", secs);
}

/* static void abort_help() */
/* { */
/*   static char *more = */
/*     "Aborts the program after closing and deleting the error file.  All is lost!"; */
/*   static char *syn = "Synonyms are: QUIT"; */
/*   printf("\n%s\n\n%s\n", more, syn); */
/* } */
