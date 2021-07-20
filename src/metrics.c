/*  VERSION 2.0 - COLUMBUS DMS
file - metrics.c

metrics calculated after simulation run are:

     user throughput = octets received / total sec.

     channel efficiency = (user throughput * 8) / link_speed

     channel utilization = (total bits sent / total sec.) /
link speed

     channel use = user throughput per connection * 8) / link
speed

     DT/AK ratio = total # DTs / total # AKs

     Transport protocol efficiency (per connection) = user
data/network data

     Network protocol efficiency = network data / subnet data

     Subnet protocol efficiency = subnet data / subnet data +
header

     total retransmissions = total # of DT retransmissions

     maximum retransmissions = highest # of times a single DT
was transmitted

     rate of retransmissions = total retransmissions / # of
DTs

     distribution of retransmissions = # of DTs retransmitted
1 . . . 20 or > 20 times

     One-way and Two-way Delays ( per octet and per message)
          avg delay, min, max, variance, std-dev, & coeff of
var.

     ODT/TPDU ratio = # of original DTs / (total #DTs + total
#AKs +
                            fixed overhead messages)

     Send memory utilization per Transport per connection

     Receive memory utilization per Transport per connection

     Total memory utilization per Transport per connection

     CPU utilization
          (Transport CPUs are broken out per connection use)

     Event statistics

metric report written to file simmetric.log

metrics also written to metric structure used for graphic
display output
*/
char MetricsSid[]="@(#)metrics.c	4.1 10/02/97";

#include <stdio.h>
#include <math.h>
#include "defs.h"
#ifdef UNIX
#include <sys/time.h>
#else
#include <time.h>
#endif
#include "types.h"
#include "globals.h"

static void summary(const char *fname, FILE *mfp)
{
    struct tms csecs;
    float cpu_secs;

    fprintf(mfp, "Comienzo de la ejecucion:\t%s", ctime(&wtime));
    times(&csecs);
    print_elapsed(mfp);

    cpu_secs = ((float) ((csecs.tms_utime + csecs.tms_stime) - (start_cpu.tms_utime + start_cpu.tms_stime))) / HZ;

    fprintf(mfp, "%10.3f segundos de CPU usados\n", cpu_secs +.0005);
    fprintf(mfp, "%10.3f segundos simulados\n", sim_clock / MILLISEC +.0005);
    fprintf(mfp, "%10d eventos ejecutados a %.3f eventos por segundo real de CPU\n", evts_exec, (evts_exec / cpu_secs) +.0005);
    fprintf(mfp, "%10.0f octetos recibidos\n\n", octs_rcvd);

    fprintf(mfp, "\tNodos: %4d\n", no_of_nets);
    fprintf(mfp, "\tSemilla: %2ld\n", seed);
	 fprintf(mfp, "\tTTRT = %.1f\n", TTRT);
    if (anal_est)
      fprintf(mfp, "\tTransitorio: %d ms\n", clk_limit);
    fprintf(mfp, "\tFichero de Entrada: %s\n", fname);
	 fprintf(mfp,"\n\t Version acelerada del simulador\n");

    print_meas(mfp);

	 if(fddi_ii)
		{
		  fprintf(mfp, "\n ***  PARAMETROS DE INTERES PARA FDDI-II  ***\n");
		  fprintf(mfp, "\nEstacion Master: %d\n", m[0].master);
		  fprintf(mfp, "Latencia del anillo: %f ms -> %d ciclo(s)\n", m_delay, m_cycles);
		  fprintf(mfp, "Retardo LAB buffer: %f ms\n", m[0].master_delay);
		  fprintf(mfp, "BW Trafico isocrono: %f Kbps", m[0].iso_bandwidth);
		  fprintf(mfp, " (%2.2f%c)\n", m[0].iso_bandwidth/m[0].current_link_speed * 100, 37);
		  fprintf(mfp, "# WBCs ocupados: %d de %d (%2.2f%c)\n", m_no_of_channels, m_wbcs, m_no_of_channels * 100.0 / m_wbcs, 37);
		  fprintf(mfp, "BW Trafico de Paquetes: %f Kbps ", m[0].packet_bandwidth);
		  fprintf(mfp, "(%2.2f%c)\n", m[0].packet_bandwidth/m[0].current_link_speed*100, 37);
		  fprintf(mfp, "Longitud del testigo: %f ms\n", m[0].token_length);
		}
}

/*

*/
static void msg_stats(FILE *fp)
{
    int i;

    fprintf(fp, "\n\n ***  ESTADISTICAS DE RETARDO DE MENSAJES (ms)  ***\n");

    for (i = start_cid; i < start_cid + no_conns; i++)
      {
	if (c[i].retardo != NULL)
	  {
	    fprintf(fp, "\n\nCONEXION %d\n",i);
	    promedios_stats(c[i].retardo, fp);
	  }
      }
}

static void buffer_stats(FILE *fp)
{
    int i;

    fprintf(fp, "\n\n ***  ESTADISTICAS DE PROBABILIDAD DE PERDIDA DE PAQUETES (tramas)  ***");

    for (i = start_cid; i < start_cid + no_conns; i++)
      {
	if (c[i].perdidas != NULL)
	  {
	    fprintf(fp, "\n\nCONEXION %d\n",i);
	    promedios_stats(c[i].perdidas, fp);
	  }
      }
}

static void TRT_stats(FILE *fp)
{
  int i;

  for (i=0; i<no_of_nets && s[i].ttrt == NULL; i++);
  if (i<no_of_nets)
    {
      fprintf(fp, "\n\n ***  ESTADISTICAS DEL TRT (ms)  ***\n\n");
      fprintf(fp, "NODO %d\n", i);
      promedios_stats(s[i].ttrt, fp);
    }
}

void imprime(const char *nombre_fichero, const char *fn)
{
    FILE *mfp;

    mfp = fopen(nombre_fichero, "w");

#ifdef NIP
    /* Compute TPE for MODEL VALIDATION */

    for (i = 0; i < tsdus_rcvd; i++)
	fprintf(mfp, "TPE for TSDU #%d is %.3f\n\n", i, tpe[i]);
#endif
    /* print a summary of the run */
    summary(fn, mfp);

    /* imprime estadisticas del TRT */
    TRT_stats(mfp);

    /* imprime estadisticas de retardos de mensajes */
    msg_stats(mfp);

    /* imprime estadisticas de llenado de buferes */
    buffer_stats(mfp);

    /* print channel use, efficiency and utilization metrics */
     /* channel(mfp) */ ;

    /* print protocol efficiency metrics */
/*    protocol_eff(mfp);*/

    /* print CPU utilization metrics */
/*     cpu_util(mfp); */

    fclose(mfp);
}

void metrics(char *fn)
{
  int i;      
  
  for (i=0;i < no_of_nets;i++)
    {
      if (files && s[i].ttrt != NULL)
	cierra_ficheros(s[i].ttrt);
      /*	if (files && s[i].llenado != NULL)
		cierra_ficheros(s[i].llenado);*/
    }
  for (i=start_cid;i < start_cid + no_conns; i++)
    if (files && c[i].retardo != NULL)
      cierra_ficheros(c[i].retardo);
  
  /* adjust metric clock */
  
  metric_clock_adjust();
  
  imprime("simmetric.log",fn);
}


/*

*/
void cpu_util(FILE *fp)
{
    int i, j;
    if (sim_clock == 0.0)
    {
	fprintf(fp, "\nNo simulation has yet occurred!\n");
	return;
    }

    fprintf(fp, "\n\nCPU\tSECS.\tUTIL.\tPROCESSES\n\n");

    for (i = 0; i < no_clocks; i++)
    {

	fprintf(fp, "%d\t%.3f\t%.3f\t", i, cpus[i] / MILLISEC +.0005, cpus[i] / metric_clock +.0005);

	for (j = 0; j < no_of_nets; j++)
	    if (n[j].clk_index == i)
		fprintf(fp, "NTW#%d ", j);
	for (j = 0; j < no_of_nets; j++)
	    if (s[j].clk_index == i)
		fprintf(fp, "SNT#%d ", j);
	fprintf(fp, "\n");
    }

}

/*  */
/*      Channel use per connection is computed, and channel */
/* efficiency */
/*      and channel utilization per media. */
/* */
/* static void channel(FILE *mfp) */
/* { */
/*     int i, j, k, net_no1, net_no2; */
/*     long total, Ttotal, Tsndata, Tsnhdr; */
/*     float chan_th, time_sec; */
/*     time_sec = metric_clock / MILLISEC; */

/*     for (i = 0; i < no_of_media; i++) */
/*     { */
/* 	Ttotal = 0; */
/* 	Tsndata = 0; */
/* 	Tsnhdr = 0; */

/* 	fprintf(mfp, "\nMEDIA #%d\n", i); */

/* 	for (j = 0; j < no_conns; j++) */
/* 	{ */
/* 	    total = 0; */
/* 	    k = start_cid + j; */

/* 	    net_no1 = c[k].c_end1; */
/* 	    net_no2 = c[k].c_end2; */

 	    /* skip connections not involving this media */
/* 	    if ((s[net_no1].media_interface != i) && (s[net_no2].media_interface != i)) */
/* 		continue; */
 	    /*
 	       data received by peer TP is data successfully sent over this
 	       connection
 	    */ 
/* 	    if (s[net_no1].media_interface == i) */
/* 		total += c[k].tot_oct_recvd; */
/* 	    if (s[net_no2].media_interface == i) */
/* 		total += c[k].tot_oct_recvd; */
/* 	    if (time_sec != 0) */
/* 		chan_th = ((total * 8) / time_sec) / (m[i].link_speed * MILLISEC); */

/* 	    fprintf(mfp, "\tCHANNEL USE is %.3f for", chan_th +.0005); */

/* 	    Ttotal += total; */
/* 	    Tsndata += stat[k].sndata_bits; */
/* 	    Tsnhdr += stat[k].snhdr_bits; */
/* 	} */

/* 	if (time_sec != 0) */
/* 	{ */
/* 	    chan_th = ((Ttotal * 8) / time_sec) / (m[i].link_speed * MILLISEC); */

/* 	    fprintf(mfp, "\tCHANNEL EFFICIENCY is %.3f\n", chan_th +.0005); */

/* 	    chan_th = ((Tsndata + Tsnhdr) / time_sec) / (m[i].link_speed * MILLISEC); */

/* 	    fprintf(mfp, "\tCHANNEL UTILIZATION is %.3f\n\n", chan_th +.0005); */
/* 	} */
/* 	else */
/* 	{ */
/* 	    fprintf(mfp, "\tCHANNEL EFFICIENCY is UNDEFINED\n"); */
/* 	    fprintf(mfp, "\tCHANNEL UTILIZATION is UNDEFINED\n\n"); */
/* 	} */
/*     } */
/* } */

/* static void protocol_eff(FILE *fp) */
/* { */
/*     double prot_eff; */
/*     int i, k; */
/*     double Tinfo, Tndata, Tsndata, Tsnhdr; */
/*     Tinfo = 0; */
/*     Tndata = 0; */
/*     Tsndata = 0; */
/*     Tsnhdr = 0; */

/*     for (i = 0; i < no_conns; i++) */
/*     { */
/* 	k = start_cid + i; */
 	/*
 	   skip connections which had no network data recorded
 	*/
/* 	if (stat[k].ndata_bits <= 0) */
/* 	    continue; */
/* 	Tsndata += stat[k].sndata_bits; */
/* 	Tsnhdr += stat[k].snhdr_bits; */
/*     } */

/*     if (Tsndata) */
/*     { */
/* 	prot_eff = Tsndata / (Tsndata + Tsnhdr); */
/* 	fprintf(fp, "\nSUBNET PROTOCOL EFFICIENCY is %.3f\n\n", prot_eff +.0005); */
/*     } */
/*     else */
/*     { */

/* 	fprintf(fp, "SUBNET PROTOCOL EFFICIENCY is UNDEFINED\n\n"); */
/*     } */
/* } */

/* #define   MAX_SUBTOTALS  2 */
/* #define SUB0        0 */
/* #define SUB1        1 */

/* static struct event_stats totals[NUM_EVENT_TYPES][MAX_SUBTOTALS]; */

/* static char *t_events[] = { */

/*     "IDU->T", */
/*     "ODT->N", */
/*     "XIDU->T", */
/*     "OXDT->N", */
/*     "AK->N", */
/*     "XAK->N", */
/*     "IDU->U", */
/*     "XIDU->U", */
/*     "P - AK", */
/*     "P - XAK", */
/*     "P - DT", */
/*     "P - XDT", */
/*     "RDT->N", */
/*     "RXDT->N", */
/*     "WAK->N", */
/*     "MAK->N", */
/*     "N->T" */
/* }; */

/* static char *u_events[] = { */

/*     "IDU->T", */
/*     "IDU->U", */
/*     "XIDU->T", */
/*     "XIDU->U", */
/*     "C - IDU", */
/*     "NOTIFY" */
/* }; */

/* static char *n_events[] = { */

/*     "T->N", */
/*     "N->S", */
/*     "S->N", */
/*     "N->T" */
/* }; */

/* static char *s_events[] = */
/* { */
/*     "N->S", */
/*     "M->S", */
/*     "TOKEN", */
/*     "S->N", */
/*     "S->M", */
/*     "SYNC->M", */
/*     "P.7 ->M", */
/*     "P.6 ->M", */
/*     "P.5 ->M", */
/*     "P.4 ->M", */
/*     "P.3 ->M", */
/*     "P.2 ->M", */
/*     "P.1 ->M", */
/*     "P.0 ->M" */
/* }; */
