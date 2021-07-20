/* ------------------------------------------------------------------------*/
/*
Utilities for the network, user, transport, and subnet modules.
These are functional utilities and the error determination functions.       */
/*--------------------------------------------------------------------------*/


char UtilSid[] = "@(#)util.c	4.5 06/30/98";
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <signal.h>
#include <string.h>
#include "defs.h"
#include "types.h"
#include "globals.h"

#define PARAM   2

/*--------------------------------------------------------------------------*/
/* Returns next value in distribution according to the distribution type.   */
/* Modificada para incluir los nuevos tipos de trafico.                     */

double get_distrib_val(struct distrib *p, double ejecucion_anterior,
					  int longitud)
{
                        /*NO TE OLVIDES DE ANHADIR AQUI LAS FUNCIONES*/
                        /*DE DISTRIBUCION QUE VAYAS CREANDO          */
    double res,res2, resultado;     

#ifdef INDEP_SEEDS
    extern int seedOne, seedTwo;
    int seed1, seed2;
    seed1 = seedOne;
    seed2 = seedTwo;
    seedOne = p->seed1;
    seedTwo = p->seed2;
#endif

    switch (p->type)
      {
      case CONSTANT:
	if (p->indice)
	  {
	    resultado = p->value1;
	    break;
	  }
	p->indice = TRUE;
	resultado = uniform(0.0, p->value1);
	break;
      case UNIFORM:
	resultado = uniform(p->value1, p->value2);
	break;
      case NORMAL:
	resultado = normal(p->value1, p->value2);
	break;
      case POISSON:
	resultado = poisson(p->value1);
	break;
      case EXPONENTIAL:
	resultado = expont(p->value1);
	break;
      case GAMMA:
	resultado = Gamma(p->value1, p->value2, &(p->value3), &(p->value4));
	break;
      case TRIANGULAR:
	if (longitud == 0)
	    resultado = triangular(p->value1, p->value2, p->value3);
	else
	    resultado = tiempo_triangular(p, ejecucion_anterior, longitud);
	break;
      case TABULAR:
	resultado = tabular(p, ejecucion_anterior);
	break;
      case FILE_SMPLS:
	resultado = file_smpls(p);
	break;
      case BINARY_FILE:
	resultado = binary_file(p);
	break;
      case VELOCIDAD_UNIFORME:
	resultado = velocidad_uniforme(p, ejecucion_anterior, longitud);
	break;
      case SINCRONO:
	resultado = sincrono(p, ejecucion_anterior);
	break;
      case SRIRAM86:
	resultado = sriram86(p->value1,p->value2,p->value3);
	break;
      case BRADY69:
	resultado = brady69( p->value1, p->value2, p->value3,
			     &(p->value4),&(p->value14),&(p->value15) );
	break;
      case GRUBER82:
	resultado = gruber82(p->value1, p->value2, p->value3,
			     p->value4, p->value5, p->value6,
			     &(p->value15) );
	break;
      case RIYAZ95_A:
	res = riyaz95(p->value1,p->value2,p->value3,p->value4,
		      p->value5,p->value6,
		      p->value7, &(p->value8), &(p->value9),
		      &(p->value13), &(p->value14), &(p->value15));
	prioridades_riyaz95(12,res,&(res), &(res2));
	p->value10 = res2;
	resultado = res;
	break;
      case RIYAZ95_B:
	resultado = p->otro->value10;
	break;
      case KRUNZ95:
	resultado = krunz95(p->value1, p->value2,
			    p->value3, p->value4,
			    p->value5, p->value6,
			    &(p->value14),&(p->value15) );
	break;
      case YEGENOGLU92:
	resultado = yegenoglu92(&(p->value15),&(p->value1),&(p->value2),&(p->value3),
				p->value4, p->value5, p->value6,
				p->value7, p->value8, p->value9, 
				p->value10, p->value11, p->value12);
	break;
      case TORBEY91:
	resultado = torbey91(p->value1, p->value2,p->value3,p->value4,
			     p->value13,&(p->value5),&(p->value14),
			     &(p->value15));
	break;
#ifdef TCPLIB
      case TAM_PKT_SMTP:
	resultado = tam_pkt_smtp();
	break;
      case TAM_PKT_NNTP:
	resultado = tam_pkt_nntp();
	break;
#endif
      case LOGNORMAL:
	resultado = lognormal(p->value1, p->value2);
	break;
      case PARETO:
	resultado = Pareto(p->value1, p->value2);
	break;
      case WEILLBULL:
	resultado = Weillbull(p->value1, p->value2);
	break;
#ifdef TCPLIB
      case TIEMPO_TELNET_1:
	resultado = tiempo_telnet_1(p->value1, &(p->value2), &(p->value3),
				    &(p->value13));
	break;
      case TIEMPO_TELNET_2:
	resultado = tiempo_telnet_2(p->value1, p->value2,p->value3,
				    &(p->value4),&(p->value13),&(p->value14));
	break;
      case TIEMPO_FTP:
	resultado = tiempo_ftp ( p->value1, p->value2, p->value3, 
				 p->value4, &(p->otro->value5), &(p->value6));
	break;
      case PKT_FTP:
	resultado = pkt_ftp( p->value1, p->value2, &(p->value3), 
			     &(p->value4), &(p->value5), &(p->value13), 
			     &(p->value14), &(p->value15));
	break;
#endif
      case DOBLE_DET:
	resultado = doble_determinista(p->value1, p->value2, p->value3);
	break;

      default:
	p_error("distribution type unknown");
	exit (1);
	/* No llega aqui; simplemente para evitar el "warning" */
    }
#ifdef INDEP_SEEDS
    p->seed1 = seedOne;
    p->seed2 = seedTwo;
    seedOne = seed1;
    seedTwo = seed2;
#endif
    return resultado;
}

/*---------------------------------------------------------------------------*/
/*	Frees a data struct by marking it as not in use by the requested
	party (TX or RX).  If nobody is using the data struct, then it
	will be put on the used heap or freed, as required by the user.      */
void free_data(struct data *dp)
{
    int type;
    /* Record type of data structure */

    type = dp->flags & MASK;

    if (!oqueues)
      free(dp);
    else
      {
	switch (type)
	  {
	  case AK:
	  case XAK:  
	    dp->next_data = ak_heap;
	    ak_heap = dp;
	    break;
	  case DT:
	  case XDT:
	    dp->next_data = dt_heap;
	    dt_heap = dp;
	    break;
	  }
      }
  }

/*---------------------------------------------------------------------------
   Calculate the probability of an error in a block of size.  Return
   the probability as a bound dividing a 32-bit integer space.              */
static long block_err(int size, int media_no)
{
    double prob;
    /* COMPUTE PROBABILITY OF BLOCK SUCCESS */

    prob = pow(m[media_no].byte_ok_prob, (double) size);

    /* RETURN PROBABILITY (INTEGER) OF BLOCK ERROR */

    return ((long) (((1.0 - prob) * MAX_INT) + 0.5));
}

/*-------------------------------------------------------------------------
	Find The Cut point for the probability of error in a message block.*/
static long find_cut(int blk_size, int media_no)
{
    struct enode *tp;
    struct enode *np;
    for (tp = m[media_no].head; tp != NULL; tp = tp->next)
      {
	if (tp->size == blk_size)
	  return (tp->cutoff);
	if (tp->size > blk_size)
	  break;
      }
    
    if((np = (struct enode *) malloc(sizeof(struct enode)))==NULL)
      p_error("insuficiente memoria HEAP para la estructura de un enode\n");
    
    np->size = blk_size;
    np->cutoff = block_err(blk_size, media_no);
    
    /* INSERT NEW MSG SIZE IN MIDDLE OF LIST */

    if (tp != NULL)
      {
	np->prev = tp->prev;
	np->next = tp;
	if (tp->prev)
	  {
	    tp->prev->next = np;
	    tp->prev = np;
	  }
	else
	  m[media_no].head = np;
      }
    
    /* START NEW LIST */
    
    else if (m[media_no].head == NULL)
      {
	np->prev = NULL;
	np->next = NULL;
	m[media_no].head = np;
	m[media_no].tail = np;
      }

    /* PUT AT END OF LIST */
    
    else
      {
	np->prev = m[media_no].tail;
	np->next = NULL;
	np->prev->next = np;
	m[media_no].tail = np;
      }
    
    return (np->cutoff);
  }

/*---------------------------------------------------------------------------
  Returns true if an error is found for the block size stated.              */
int found_error(int blk_size, int media_no)
{
  if (m[media_no].error_rate == 0.0)
    return (FALSE);
  if (aleatorio() <= find_cut(blk_size, media_no))
    return (TRUE);
  return (FALSE);
}

/*-------------------------------------------------------------------------*/
/* Returns TRUE if there is subnet to network interface space available    */
int sn_to_net_int_space_avail(int n_no)
{
  if (n[n_no].net_from_sn_intf >= SN_TO_NET_INT_LIMIT)
    return (FALSE);
  return (TRUE);
}

/*-------------------------------------------------------------------------*/
/*Returns TRUE if the number of msgs in the net->sn interface is less than
   the defined LIMIT                                                       */
int net_to_sn_int_space_avail(int sn_no)
{
  if (s[sn_no].sn_from_net_intf >= NET_TO_SN_INT_LIMIT)
    return (FALSE);
  return (TRUE);
}

/*---------------------------------------------------------------------------*/
/* Funcion drenadora de mensajes a subnet */
double Q_msg(int net_no, int conn, struct traffic *trafico)
{
  struct data *msg;
  int size;
  
  if((msg = ((struct data *) malloc(sizeof(struct data))))==NULL)
    p_error("insuficiente memoria HEAP para la estructura de data\n");
  
  msg->flags = trafico->t_flag;
  msg->seq_num = c[conn].seq_num++;
  msg->nh_size = msg->th_size = 0;
  
  if (c[conn].left_size)
    {
      msg->frame_num = --c[conn].s_frame_num;
      msg->first = FALSE;
      if (c[conn].left_size > s[net_no].max_size)
	{
	  msg->dsize = s[net_no].max_size;
	  c[conn].left_size -= s[net_no].max_size;
	}
      else
	{
	  msg->dsize = c[conn].left_size;
	  c[conn].left_size = 0;
	}
    }
  else
    {
      c[conn].s_pkt_num++;
      msg->first = TRUE;

      size = get_distrib_val(trafico->msg, trafico->cur_tarrive, 0);
      trafico->cur_tarrive += get_distrib_val(trafico->arrive, 
					      trafico->cur_tarrive,size);
      if (size > s[net_no].max_size)
	{
	  msg->dsize = s[net_no].max_size;
	  c[conn].left_size = size - s[net_no].max_size;
	  c[conn].s_frame_num = msg->frame_num =
	    (size - 1) / s[net_no].max_size;
	}
      else
	{
	  msg->dsize = size;
	  msg->frame_num = 0;
	}
    }
  msg->pkt_num = c[conn].s_pkt_num;
  msg->gen_time = msg->stime = trafico->cur_tarrive;

  schedule(NTW, EVENT1, msg->stime, msg, net_no);
  return (msg->stime);
}
