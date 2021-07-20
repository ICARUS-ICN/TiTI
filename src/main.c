/*  VERSION 1.0 - NBS

main routine for transport simulation model - 8/84 (original)

errors are written to file simerror.log
metrics are written to file simmetric.log
if oqueues is ON, queue listings are written to file simout.log

if the model is run with the graphics option, info for graphics output for
DATAPLOT is written to files MODEL1-6

main execution loop:

	events in tp, user, network, and subnet are executed until all events
	that can be executed at this wall clock time have been executed
	or are blocked

	then the wall clock is advanced

	the above two steps are repeated until there are no more events to
	execute because all queues are empty

HISTORY:	Added user interface (KLM)	 11/84
		Added metric_clock_adjust (KLM)  12/84
		Added limits model control (KLM) 11/84
		Moved user interface to COMMANDS.C (KLM)  4/85
		Added control of multiple networks and subnets (MW) 4/85
		Added option CSMA/CD media module control  (KLM) 7/85
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/times.h>
#include "defs.h"
#include "types.h"
#include "globals.h"
#define ERROR_FILE	"simerror.log"

char MainSid[]="@(#)main.c	4.8 01/19/99";

#if #system(amigaos) || #system(NetBSD)
static void sig_TERM()
#else
static void sig_TERM(int i)
#endif
{
    sigue_programa = OFF;
    signal(SIGTERM, sig_TERM);
    return;
}

#if #system(amigaos) || #system(NetBSD)
static void sig_USR1()
#else
static void sig_USR1(int i)
#endif
{
    volcado_parcial = ON;
    signal(SIGUSR1, sig_USR1);
    return;
}

static int adjust_clock()
{

/*
the wall clock is adjusted by this module to be min(t_prime, t_2prime)
where t_prime is the minimum of all tstars for all disabled mods, and
t_2prime is the minimum of all mod clocks to all enabled mods

the t_star time for each mod is the execute time of the first event which
has an execute time greater than the wall clock

the mod's clock is the time that the module is free to do something else

a module is disabled if it has events to execute (execute time for the event
is <= the wall clock) but they are blocked, so the mod can't do anything

*/

    int i;
    int j;
    struct event *e;
    double t_prime;
    double t_2prime;
    double t_star;
    struct subnet_info *sn;
    struct net_info *net;
    struct e_queues *ppr;

    static int deadlock = 0;
    t_prime = t_2prime = INFINITY;

    
    for (i = 0, net=&n[0], sn=&s[0]; i < no_of_nets; i++, net++, sn++)
    {
	if (net->enable == FALSE)
	{
	    t_star = INFINITY;
	    for (j = 0, ppr=&net->n_queue[0]; j < MAX_PRIORITY; j++, ppr++)
	    {
		for (e = ppr->head; ((e != NULL) && (e->xtime <= sim_clock)); e = e->next_aevent);
		if ((e != NULL) && (e->xtime < t_star))
		    t_star = e->xtime;
	    }
	    if (t_star < t_prime)
		t_prime = t_star;
	}
	else if (*net->clock_ptr < t_2prime)
	    t_2prime = *net->clock_ptr;

	if (sn->enable == FALSE)
	{
	    t_star = INFINITY;
	    for (j = 0, ppr=&sn->sn_queue[0]; j < MAX_PRIORITY; j++,ppr++)
	    {
		for (e = ppr->head; ((e != NULL) && (e->xtime <= sim_clock)); e = e->next_aevent);
		if ((e != NULL) && (e->xtime < t_star))
		    t_star = e->xtime;
	    }
	    if (t_star < t_prime)
		t_prime = t_star;
	}
	else if (*sn->clock_ptr < t_2prime)
	    t_2prime = *sn->clock_ptr;
    }

    /* advance the clock accordingly */
    if (t_prime < t_2prime)
	sim_clock = t_prime;
    else
	sim_clock = t_2prime;
    /*
       check for possible deadlock in the bulk data case, i.e., no events
       happen within some minimum time
    */

    if ((sim_clock - dead_clock) >= MIN_DEAD_TIME)
	deadlock++;

    /* comprueba si se esta en el transitorio para el analisis estadistico */

    if (anal_est && transitorio && sim_clock >= clk_limit)
      transitorio = FALSE;

    /* enable all mods so blocked events can be rechecked */
    enable_mods();

    return (deadlock);
}

static int all_queues_empty()
{

/*
this function returns true if all mods have nothing to do

all priority event queues are checked for each mod

tp's event queues are considered empty if the only thing for it to do is send
an acknowledgement due to the window timer or acknowledgement-interval timer
expiration.
*/

    int i;
    int j;
    struct event *e;
    /* all priority event queues for each network are checked */
    for (i = 0; i < no_of_nets; i++)
	for (j = 0; j < MAX_PRIORITY; j++)
	{
	    if (n[i].n_queue[j].head != NULL)
		return (FALSE);

	    if (s[i].prio_tab[EVENT3] != j)
	    {
		if (s[i].sn_queue[j].head != NULL)
		    return (FALSE);
	    }
	    else
	    {
		for (e = s[i].sn_queue[j].head; e != NULL; e = e->next_aevent)
		    if (e->id != EVENT3)
			return (FALSE);
	    }
	}
    /* all queues are empty */
    return (TRUE);
}

static void final_clock_adjust()
{

/*
a final clock adjustment is needed because when all queues are empty, the clock
has been adjusted just prior to that to be equal to either the window timer or
acknowledgement-interval timer values (tp is disabled, and the timer value will
be its t_star)

this function corrects the wall clock by setting it back to the maximum of all
mod clocks

*/

    int i;
    sim_clock = *n[0].clock_ptr;
    for (i = 1; i < no_of_nets; i++)
    {

	if (*n[i].clock_ptr > sim_clock)
	    sim_clock = *n[i].clock_ptr;
	if (*s[i].clock_ptr > sim_clock)
	    sim_clock = *s[i].clock_ptr;
    }

}
/*
	The metric calculations depend upon using the largest of the
	clocks (i.e. individual CPU clocks or system wall clock).  Note,
	however, that at run termination the system clock is the largest
	but it is too large.  Therefore, final_clock_adjust should be
	called when the simulation terminates with an empty Q.
*/
void metric_clock_adjust()
{
    int i;
    metric_clock = 0.0;

    for (i = 0; i < no_of_nets; i++)
    {
	if (*n[i].clock_ptr >= VERY_LG_NO)
	    continue;
	if (*n[i].clock_ptr > metric_clock)
	    metric_clock = *n[i].clock_ptr;
    }

    for (i = 0; i < no_of_nets; i++)
    {
	if (*s[i].clock_ptr >= VERY_LG_NO)
	    continue;
	if (*s[i].clock_ptr > metric_clock)
	    metric_clock = *s[i].clock_ptr;
    }

    if (sim_clock > metric_clock)
	metric_clock = sim_clock;
}
/*
	Function to test for execution limits.
	Returns 0 if limits not reached.
	Returns pointer to a message string if a limit is reached.
*/

static char *limits()
{

    static char *cpu_msg = "\nCPU limit reached";
    static char *clk_msg = "\nCLOCK limit reached";
    static char *oct_msg = "\nOCTET limit reached";
    static char *evt_msg = "\nEVENT limit reached";
    static char *cntl_msg = "\nInterrupted from the terminal";
    static char *err_msg = "\nDamaged Messages limit reached";
    static char *anal_est_msg = "\nPrecision and confidence inteval required reached";
    if (pause_point == ON)
    {
	pause_point = OFF;
	return (cntl_msg);
    }

    /* Get system CPU times */

    times(&sys_time);

    if (anal_est)
    {
	if (!num_promedios)
	    return (anal_est_msg);
    }
    else if (sim_clock >= clk_limit)
	return (clk_msg);
    else
    {
	if (((sys_time.tms_utime + sys_time.tms_stime) -
	     (start_cpu.tms_utime + start_cpu.tms_stime))
	    >= cpu_limit)

	    return (cpu_msg);


	if (evts_exec >= evt_limit)
	    return (evt_msg);

	if (octs_rcvd >= oct_limit)
	    return (oct_msg);

	if (err_cnt >= err_limit)
	    return (err_msg);

    }
    return (0);
}

int main(int argc, char *argv[])
{
    int i, paso_final_prev;
    char *cp, *mptr;
    struct subnet_info *sn;
    struct net_info *net;

    if (argc < 2)
    {
	printf("usage:  unixfddi  param_file [graphics] [modes...] [limits...]\n");
	exit(1);
    }

    efp = fopen(ERROR_FILE, "w");

    /* Set up parameters for signal catching */

    m1 = argv[1];

    /* Assume No graphics output & batch run */

    omode = NONE;
    xmode = BATCH;
    debug = OFF;
    verbose = OFF;
    oqueues = OFF;
    stop_print = OFF;
    pause_point = OFF;
    mem_dist = FALSE;
    event_stat = FALSE;
    anal_est = FALSE;
    files = FALSE;
    inter_est = FALSE;
    chd = FALSE;
    chdopts = NULL;
    gg1 = FALSE;
    fast = FALSE;
    fddi_ii = FALSE;
    TTRT = 0.0;
#ifdef INDEP_SEEDS
    one_minus_u = FALSE;
#endif
    paso_final_prev = 0;

    /* Set initial execution control limits */

    clk_limit = MAX_INT;
    cpu_limit = MAX_INT;
    oct_limit = MAX_INT;
    evt_limit = MAX_INT;
    err_limit = MAX_INT;

    file_limit = NUM_MAX_MUESTRAS;

    /* parse the run time arguments */

    if (argc > 2)
    {
	for (i = 2; i < argc; i++)
	{
#ifdef PURIFY
	  AP_Report(0);
#endif
	    sucase(argv[i]);
	    cp = sfind(argv[i], "=");
	    if (*cp != '\0')
		*cp++ = '\0';
#ifdef PURIFY
	    AP_Report(1);
#endif
	    if (sequal(argv[i], "ANAL_EST"))
		anal_est = TRUE;
	    else if (sequal(argv[i], "CV") || sequal(argv[i], "CV_ASINC"))
	      CV_ASINC = TRUE;
	    else if (sequal(argv[i], "CV_S"))
	      CV_S = TRUE;
	    else if (sequal(argv[i], "CV_U"))
	      CV_U= TRUE;

#ifdef INDEP_SEEDS
	    else if (sequal(argv[i], "1_U"))
		one_minus_u = TRUE;
#endif
	    else if (sequal(argv[i], "FILES"))
		files = TRUE;
	    else if (sequal(argv[i], "INTER_EST"))
		inter_est = TRUE;
	    else if (sequal(argv[i], "SIZE"))
		paso_final_prev = atol(cp);
	    else if (sequal(argv[i], "CHD"))
		chd = TRUE;
	    else if (sequal(argv[i], "CHDOPTS"))
		chdopts = strdup(cp);
	    else if (sequal(argv[i], "FAST"))
		fast = TRUE;
	    else if (sequal(argv[i], "GG1"))
		gg1 = fast = TRUE;
	    else if (sequal(argv[i], "FILE_LIMIT"))
	        file_limit = atol(cp);
	    else if (sequal(argv[i], "GRAPHICS"))
		omode = GRAPHICS;
	    else if (sequal(argv[i], "BATCH"))
		xmode = BATCH;
	    else if (sequal(argv[i], "TERM"))
		xmode = TERM;
	    else if (sequal(argv[i], "CLOCK"))
		clk_limit = atol(cp) * MILLISEC;
	    else if (sequal(argv[i], "CPU"))
		cpu_limit = atol(cp) * (MILLISEC / 10);
	    else if (sequal(argv[i], "OCTETS"))
		oct_limit = atol(cp);
	    else if (sequal(argv[i], "EVENTS"))
		evt_limit = atol(cp);
	    else if (sequal(argv[i], "ERRORS"))
		err_limit = atol(cp);
	    else if (sequal(argv[i], "SEED"))
		seed = atol(cp);
	    else if (sequal(argv[i], "TTRT"))
		TTRT = atof(cp);
	    else if (sequal(argv[i], "VERBOSE"))
		verbose = ON;
	    else if (sequal(argv[i], "FDDI-II"))
		fddi_ii = TRUE;
	    else if (sequal(argv[i], "DEBUG"))
		debug = ON;
	    else if (sequal(argv[i], "QUEUES"))
		oqueues = ON;
	    else if (sequal(argv[i], "WAIT"))
		xmode = WAIT;
	    else if (sequal(argv[i], "ESTATS"))
		event_stat = TRUE;
	    else
	      {
		fprintf(efp, "Unknown option %s\n", argv[i]);
		exit(1);
	      }
	}
    }
       
    /* Seed random number generation */

    if (!seed)
	seed=time(0);

    semilla_aleatorio(seed);
                           /*--------------------------------------*/
    srand48(seed);         /*NECESARIO PARA USAR LA LIBRERIA Tcplib*/
                           /*--------------------------------------*/

    if (inter_est)
      {
	inter_est = getpid();
	printf("%d\n", inter_est);
      }

    init(argv[1]);

    if (paso_final_prev)
      {
	paso_final_prev = 1 << paso_final_prev;
	if (!paso_final || paso_final_prev < paso_final)
	  paso_final = paso_final_prev;
      }

    /* inicializa el tiempo de duracion del transitorio */

    if (anal_est && clk_limit != MAX_INT)
      transitorio = TRUE;
    else
      transitorio = FALSE;

    /* Save starting cpu time used and starting wall time */

    times(&start_cpu);

    time(&wtime);

    /* If user requested immediate control, give it to him */

    if (xmode == WAIT)
    {
	tpause("User requested immediate control.");
	xmode = TERM;
    }

    /* setup to catch signal if execution mode is terminal */
    if (xmode == TERM)
	signal(SIGINT, sig);

    sigue_programa = ON;
    signal(SIGTERM, sig_TERM);
    
    volcado_parcial = OFF;
    signal(SIGUSR1, sig_USR1);

    if (gg1)
      gg1_node(m1);
    if (fast)
      fast_fddi(m1);

    /* loop until there is nothing for transport, user or network to do */
    while (all_queues_empty() == FALSE && sigue_programa)
    {

	if (sim_clock > VERY_LG_NO)
	{
	    /*
	       there is still something to do but the clock is too far advanced
	    */

	    metrics(m1);
	    p_error("deadlock - all event queues are NOT empty and wall clock = INFINITY");
	}
	/*
	   do this section of code while there has been activity in at least 1
	   mod
	*/
	do
	{
	    modactivity = FALSE;
	    /*
	       modactivity becomes true if an event in tp, user, network or
	       subnet is executed
	    */


	    for (i = 0, sn=&s[0]; i < no_of_nets; i++,sn++)
		if (sn->enable && *sn->clock_ptr <= sim_clock)
		  if (sn->event_q_head == NULL)
		    sn->enable = FALSE;
		  else
		    subnet(i);

	    for (i = 0, net=&n[0]; i < no_of_nets; i++,net++)
		if (net->enable && *net->clock_ptr <= sim_clock)
		  if (net->event_q_head == NULL)
		    net->enable = FALSE;
		  else
		    network(i);

	} while (modactivity);

	/* Check for deadlock & quit if found */

	if (adjust_clock() >= DEADLOCK)
	    if (xmode == BATCH)
	    {
		metrics(argv[1]);
		p_error("deadlock");
	    }
	    else
		tpause("Deadlock");

	/* Check for execution limits and take appropriate action */

	if ((mptr = limits()))
	{

	    if (xmode == BATCH)
	    {
		metrics(argv[1]);
		p_error(mptr);
	    }
	    else
		tpause(mptr);
	}

	/* If debug mode is ON, print new clock value */

	if (debug)
	    printf("clock is %f\n", sim_clock);

	if(volcado_parcial)
	  {
	    volcado_parcial = OFF;
	    imprime("actual.log", m1);
	  }

      }

    /* Model run termination processing */

    final_clock_adjust();

    metrics(m1);

    /* if graphics data files are requested, produce them */

/*
	if (omode == GRAPHICS)
		post_prt();
*/

    /* if the event queues are being saved for output, produce them */

    if (oqueues)
	p_error("finished");

    fclose(efp);
    exit (0);
}



