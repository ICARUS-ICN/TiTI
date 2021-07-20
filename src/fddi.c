/*  VERSION 2.0 - COLUMBUS DMS

file - fddi.c

	fddi subnet and media simulation

*/
char FddiSid[] = "@(#)fddi.c	4.2 07/30/98";

#include "defs.h"
#include "globals.h"
#include "types.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/*
	CHECK FOR A BUFFER TO HOLD OUTGOING MESSAGES
*/
static int sn_tx_buf_avail(struct subnet_info *sn)
{
	if (sn->send_buf_size < sn->send_buf_bound)
		return (TRUE);
	return (FALSE);
}

/*
	CHECK FOR A BUFFER TO HOLD INCOMING MESSAGES
*/
static int sn_rx_buf_avail(struct subnet_info *sn)
{
	if (sn->rec_buf_size < sn->rec_buf_bound)
		return (TRUE);
	return (FALSE);
}

/*
	SELECT A PRIORITY FOR FDDI TRANSMISSION

	This could be an interesting control to experiment with.  For
	now, three simple methods of selecting priority have been allowed
	for.  The method to use is driven by conditional compilation.
*/
static int sn_select_tx_q(struct data *dp, int cid)
{
#ifdef SIZE_SELECT /* DEPENDS ON MESSAGE SIZE */
	int pri;

	for (pri = NO_LEVELS_PRI; pri >= 0; pri--)
	{

		if (dp->dsize <= limit_size[pri])

			return (pri);
	}

#endif

#ifdef DIRECTED /* ASSUMES A NO_LEVELS_PRI+1 VALUED QOS \
		   (0-NO_LEVELS_PRI) */

	return (c[cid].qos);

#endif /* IF PRIORITIES SCHEME NOT IMPLEMENTED  */
	/* -1 IMPLIES ASYNCHRONOUS DATA */
	/* OTHERWISE SYNCHRONOUS DATA */

	if (c[cid].qos == -1)
		return (NO_LEVELS_PRI - 1);
	return (NO_LEVELS_PRI);
}

/*
	REMOVE THE FIRST BUFFER FROM A BUFFER LIST
*/
static void remove_from_head(struct b_queues *blist)
{
	/* IF THERE IS ONLY ONE ITEM ON THE LIST, UPDATE LIST HEADER */

	if (blist->head == blist->end)
	{
		blist->head = NULL;
		blist->end = NULL;
	}

	/* POINT HEAD OF LIST TO THE SECOND ITEM */

	else
	{
		blist->head = blist->head->next_dt;
	}
}

/*
	SCHEDULE THE MESSAGE AT THE HEAD OF RECEIVE QUEUE FOR SENDING TO
	THE NETWORK MODULE.  DELETE THE ASSOCIATED BUFFER.
*/
static void sn_send_net_msg(struct subnet_info *sn, int sn_no)
{
	struct b_queues *blist;
	struct buf *old_buf;
	/* GET FIRST BUFFER IN RECEIVE LIST */

	blist = &(sn->Rx_Q);

	/* IF THE RECEIVE LIST IS EMPTY, RETURN */

	if ((old_buf = blist->head) == NULL)
		return;

	/* IF A MESSAGE IS ALREADY PENDING, RETURN */

	if (sn->no_net_msg_sched == FALSE)
		return;

	/* SCHEDULE SENDING THE MESSAGE TO THE NETWORK MODULE */

	schedule(SNT, EVENT4, old_buf->dt->stime, old_buf->dt, sn_no);

	/* RECORD THE PENDING MESSAGE */

	sn->no_net_msg_sched = FALSE;

	/* UNLINK THE BUFFER */

	remove_from_head(blist);

	/* FREE THE BUFFER */

	free(old_buf);
}

/*
 * Calcula el retardo de propagacion para un mensaje  
 * entre las estaciones source_net y dest_net	      
 */
static float tx_delay(int source_net, int dest_net, int media_no)
{
	float delay;
	int hops;
	int subnet, flag;

	/* # of hops between source & destination */
	hops = dest_net - source_net;
	if (hops < 0.0)
		hops += no_of_nets;

	flag = FALSE;
	hops = 0;
	subnet = source_net;
	while (subnet != dest_net)
	{
		/* Message cross master station */
		if (subnet == m[media_no].master)
			flag = TRUE;
		subnet = s[subnet].logical_next_station;
		hops++;
	}

	delay = m[media_no].prop_delay * hops +
			m[media_no].fddi_head_end_latency * hops;

	/* Add LAB buffer delay */
	if (flag)
		delay += m[media_no].master_delay;

	return (delay);
}

/* 
  BUSCA EL TIEMPO DE EJECUCION MAS PROXIMO DE LA CAPA DE RED Y
  SUBRED PARA UN NODO DADO
*/
static double siguiente_evento(int nodo, struct event *actual)
{
	int i;
	double minimo;
	struct e_queues *ppr;

	minimo = INFINITY;
	if (s[nodo].event_q_head != NULL)
	{
		ppr = &s[nodo].sn_queue[0];
		for (i = 0; i < MAX_PRIORITY; i++, ppr++)
			if (ppr->head != NULL && ppr->head != actual && ppr->head->xtime < minimo)
				minimo = ppr->head->xtime;
	}
	if (n[nodo].event_q_head != NULL)
	{
		ppr = &n[nodo].n_queue[0];
		for (i = 0; i < MAX_PRIORITY; i++, ppr++)
			if (ppr->head != NULL && ppr->head->xtime < minimo)
				minimo = ppr->head->xtime;
	}
	return minimo;
}

/*
	ADD A BUFFER TO THE END OF A BUFFER LIST
*/
static void add_to_end(struct b_queues *blist, struct buf *buffer)
{

	/* IF THE LIST IS EMPTY, A THE FIRST BUFFER */

	if (blist->end == NULL)
	{
		blist->end = buffer;
		blist->head = buffer;
	}

	/* PUT THE BUFFER LAST */
	else
	{
		blist->end->next_dt = buffer;
		blist->end = buffer;
	}
}

/*
	STORE A MESSAGE AT THE END OF THE RECEIVE BUFFER LIST
*/
static void sn_store_rx_msg(struct subnet_info *sn, struct data *msg)
{
	struct b_queues *blist;
	struct buf *new_buf;
	/* POINT TO THE RECEIVE BUFFER LIST */

	blist = &(sn->Rx_Q);

	/* ALLOCATE AND INITIALIZE A NEW BUFFER */

	if ((new_buf = ((struct buf *)malloc(sizeof(struct buf)))) == NULL)
		p_error("insuficiente memoria HEAP para la estructura de un bufer\n");

	new_buf->dt = msg;
	new_buf->next_dt = NULL;

	/* PUT THE BUFFER ON THE LIST */

	add_to_end(blist, new_buf);

	/* TAKE AWAY A RECEIVE BUFFER */

	sn->rec_buf_size++;
}

/*
	STORE A TRANSMIT MESSAGE AT THE END OF A TRANSMIT QUEUE
*/
static void sn_store_tx_msg(struct subnet_info *sn, struct data *msg, int pri)
{
	struct b_queues *blist;
	struct buf *new_buf;
	/* POINT TO TRANSMIT BUFFER LIST */

	blist = &(sn->Tx_Q[pri]);

	/* ALLOCATE AND INITIALIZE A NEW BUFFER */

	if ((new_buf = ((struct buf *)malloc(sizeof(struct buf)))) == NULL)
		p_error("insuficiente memoria HEAP para la estructura de un bufer\n");

	new_buf->dt = msg;
	new_buf->next_dt = NULL;

	/* ADD THE NEW BUFFER TO THE END OF THE TRANSMIT LIST */

	add_to_end(blist, new_buf);

	/* TAKE ONE AVAILABLE BUFFER AWAY */

	sn->send_buf_size++;
}

/*
	GET THE MESSAGE AT THE HEAD OF A TRANSMIT QUEUE AND DELETE
	THE BUFFER.
*/
static struct data *sn_get_tx_msg(struct subnet_info *sn, int pri)
{
	struct b_queues *blist;
	struct buf *old_buf;
	struct data *msg;

	/* POINT TO THE HEADER OF THE PROPER TRANSMIT LIST */

	blist = &(sn->Tx_Q[pri]);

	/* IF THE LIST IS EMPTY, RETURN */

	if ((old_buf = blist->head) == NULL)
		return (NULL);

	/* SAVE THE MESSAGE FROM THE BUFFER */

	msg = blist->head->dt;

	/* REMOVE THE BUFFER FROM THE TRANSMIT LIST */

	remove_from_head(blist);

	/* FREE THE BUFFER */

	free(old_buf);

	return (msg);
}

void subnet(int sn_no)
{

	struct event *saved, *ep;
	struct data *dp;
	struct subnet_info *sn;

	float duration;	  /* time it takes to due something that */
					  /* keeps a CPU busy */
	double time;	  /* time to schedule an event */
	float amt1, amt2; /* amount of clocking time */
	float target;	  /* token target time */
	double hold_time; /* field to save the time */
	float TRT, THT;
	float ventana;	   /* tiempo de transmision */
	int msg_size;	   /* size of message as placed onto medium */
	int msg_header;	   /* size of message header */
	int pri;		   /* transmit priority */
	int cid;		   /* connection identifier */
	int other_sn_no;   /* the other subnet number */
	int source_net_no; /* source network module number */
	int dest_net_no;   /* destination network module number */
	int media_no;	   /* media number for the current subnet */
	int q_id;		   /* transmission queue number */
	int blocked;	   /* event blocked flag */
	int print_buf;
	int burst_count; /* number of frames in a tx burst */
	int late;
	promedio st_ttrt;

	double time_aux, minimo, t_ex_min, vueltas, t_vuelta, t_vueltas;
	int sn_no_1, sn_no_2, n_vueltas, fin, i;
	struct subnet_info *sn_1, *sn_2;
	struct media_info *media;

	/* para acelerar el paso del testigo */

	sn = &s[sn_no];
	saved = NULL;

	do /* DO UNTIL AN EXECUTEABLE EVENT IS FOUND */
	{
		blocked = FALSE;

		/* SELECT THE NEXT EVENT FOR POSSIBLE EXECUTION */

		if ((saved = getevent(SNT, sn_no, saved)) == ALLBLOCKED)
		{
			sn->enable = FALSE;
			return;
		}

		/* EXTRACT THE KEYS FOR EVENT PROCESSING */

		/* Punteros a estructuras de analisis estadistico */

		st_ttrt = sn->ttrt;

		/* These fields are meaningless for the token (EVENT3) */

		cid = ((saved->data_p->flags >> CSHIFT) & XDTMASK); /* SES */

		dest_net_no = ((saved->data_p->flags >> DSHIFT) & XDTMASK);
		source_net_no = ((saved->data_p->flags >> SSHIFT) & XDTMASK);

		/* Figure out who the peer subnet is */

		if (sn_no == source_net_no)
			other_sn_no = dest_net_no;
		else
			other_sn_no = source_net_no;

		media_no = sn->media_interface;

		dp = saved->data_p;

		/* INITIALIZE TIME FIELDS */

		duration = 0.0;
		time = 0.0;

		/* TRY TO EXECUTE THE SELECTED EVENT */

		switch (saved->id)
		{

		case EVENT1: /* RECEIVE MSG FROM NETWORK */

			/* CHECK FOR SUBNET TX BUFFER ROOM */
			if (!sn_tx_buf_avail(sn))
			{
				blocked = TRUE;
				break;
			}

			/* decrement number of msgs in the net->sn interface */
			sn->sn_from_net_intf--;

			/* SELECT A TRANSMIT PRIORITY QUEUE */
			q_id = sn_select_tx_q(dp, cid);

			/* STORE MESSAGE INTO SELECTED TRANSMIT QUEUE */

			sn_store_tx_msg(sn, dp, q_id);

			/* INCREMENT SIMULATED SEND PROCESSING TIME */

			msg_size = dp->dsize + dp->th_size + dp->nh_size;

			duration = (sn->read_buf_time * msg_size);

			/* ENABLE THE SENDING NETWORK MODULE */

			enable_net(source_net_no);

			break;

		case EVENT2: /* RECEIVE MESSAGE FROM MEDIA */

			/* CHECK FOR SUBNET RCV BUFFER ROOM */

			if (!sn_rx_buf_avail(sn))
			{
				/* RECORD THE DROP */

				upd_sn_drops(dp, cid);
				sn->mem_drops++;
				saved->atime = LOST;

				/* TELL ABOUT THE DROP */

				if (verbose)
				{
					printf("%f:SUBNET %d msg %ld for cid %d from SUBNET %d DROPPED\n",
						   sim_clock, sn_no, dp->seq_num, cid, other_sn_no);
				}

				/* DISCARD THE MESSAGE */

				dp = remove_event(saved, sn_no, SNT);
				free_data(dp);

				/* PUSH AHEAD TO NEXT EVENT */

				blocked = TRUE;
				break;
			}

			/* DISCARD ERRONEOUS MESSAGE IF CRC CHECKING ENABLED */

			if (m[media_no].crc_on && (dp->flags & ERROR_BIT))
			{
				/* RECORD THE DROP */

				sn->crc_drops++;
				saved->atime = LOST;

				/* TELL ABOUT THE DROP */

				if (verbose)
				{
					printf("%f:SUBNET %d ERROR in msg %ld for cid %d from SUBNET %d\n",
						   sim_clock, sn_no, dp->seq_num, cid, other_sn_no);
				}

				/* DISCARD THE MESSAGE */

				dp = remove_event(saved, sn_no, SNT);
				free_data(dp);

				/* PUSH AHEAD TO NEXT EVENT */

				blocked = TRUE;
				break;
			}

			/* UPDATE SUBNET RECEIVE STATISTICS */

			msg_size = dp->dsize + dp->th_size + dp->nh_size;
			upd_snr_stats(sn_no, msg_size, dp->snh_size);

			/* RECEIVE AND BUFFER THE MESSAGE FROM THE MEDIA */

			sn_store_rx_msg(sn, dp);

			/* SCHEDULE SENDING MESSAGE TO NETWORK MODULE */

			duration = sn->proc_recv_time;
			time = sim_clock + duration;
			dp->stime = time;
			sn_send_net_msg(sn, sn_no);

			/* TELL ABOUT THE ARRIVAL */

			if (verbose)
			{
				printf("%f:SUBNET %d received msg %ld for cid %d from SUBNET %d\n",
					   sim_clock, sn_no, dp->seq_num, cid, other_sn_no);
			}
			break;

		case EVENT3: /* RECEIVE TOKEN */

			/* RECORD TOKEN ROTATION TIME */

			TRT = sim_clock - sn->last_visit;
			sn->tokens++;
			THT = TRT;
			if (THT > TTRT)
			{
				late = TRUE;
				sn->last_visit = sim_clock + TTRT - THT;
			}
			else
			{
				late = FALSE;
				sn->last_visit = sim_clock;
			}

			if (st_ttrt != NULL)
				if (!transitorio)
					if (sim_clock != 0.0)
						promedia(st_ttrt, TRT, sim_clock);

			/* INITIALIZE THE REQUIRED FIELDS */

			time = sim_clock;
			burst_count = 0;

			ventana = 0;

			hold_time = saved->xtime; /* This is to accomodate the queuing
					   time calculations */

			if (verbose)
				printf("%f:SUBNET %d received TOKEN\n",
					   sim_clock, sn_no);

			/* FOR EACH PRIORITY CLASS */

			for (pri = NO_LEVELS_PRI; pri >= 0; pri--)
			{
				/* DETERMINE TARGET TIME */

				switch (pri)
				{
				case NO_LEVELS_PRI:
					/*
		       SYNCHRONOUS TRAFFIC ALWAYS HAS TIME TO SEND
		    */

					target = sn->target_time_syn;
					sn->last_token_time_syn = time;
					break;

				case NO_LEVELS_PRI - 1: /* ASYNCHROUS TRAFFIC CLASS 7 */

					target = (late) ? 0.0 : TTRT - THT;
					sn->last_token_time[7] = time;
					break;

				default: /* ASYNCHRONOUS TRAFFIC CLASSES 0-6 */
					/*
		       COMPUTE ELAPSED TIME SINCE THE CLASS LAST HELD TOKEN
		    */

					target = (late) ? 0.0 : sn->target_time[pri] - THT;

					/* RECORD THE CURRENT TIME */

					sn->last_token_time[pri] = time;
					break;
				}

				/* WHILE TIME && MESSAGES ARE LEFT */

				print_buf = TRUE;

				while ((target > 0) && (dp = sn_get_tx_msg(sn, pri)))
				{
					/* EXTRACT DEST. SUBNET */

					other_sn_no = (dp->flags >> DSHIFT) & XDTMASK; /* SES */
					cid = ((dp->flags >> CSHIFT) & XDTMASK);	   /* SES */

					/* COMPUTE MESSAGE TRANSMIT SIZE */

					msg_size = dp->dsize + dp->th_size +
							   dp->nh_size;

					msg_header = sn->subnet_head;
					dp->snh_size = msg_header;
					dp->q_wait = time - dp->stime;

					/* COUNT BYTES ON CHANNEL */

					m[media_no].byte_count += msg_size + msg_header;

					/* ADD PREAMBLE */

					msg_header += m[media_no].preamble;

					/* RECORD STATISTICS FOR SUBNET SENDING */

					upd_sns_stats(sn_no, msg_size, msg_header, cid);
					msg_size += msg_header;

					/* COMPUTE TRANSMIT TIME */
					if (fddi_ii)
						amt1 = (msg_size * 8) /
							   m[media_no].packet_bandwidth;
					else
						amt1 = (msg_size * 8) /
							   m[media_no].link_speed;

					/* inicializa la cuenta de ventana de transmision */
					if (files)
						ventana += amt1;

					/* MANAGE COUNTDOWN CLOCK */

					target -= amt1;
					if (pri != NO_LEVELS_PRI)
						THT += amt1;

					/* DETERMINE NEW TIME */

					if (verbose)
						printf("%f:SUBNET %d send msg %ld for cid %d to SUBNET %d\n",
							   time, sn_no, dp->seq_num, cid, other_sn_no);

					time += amt1;

					/* SCHEDULE BUFFER RELEASE */

					schedule(SNT, EVENT5, time, &tkn, sn_no);

					/* SCHEDULE RECEIPT OF MESSAGE */

					if (fddi_ii)
						amt2 = tx_delay(sn_no, other_sn_no, media_no);
					else
						amt2 = (m[media_no].prop_delay + m[media_no].fddi_head_end_latency) * c[cid].between;

					schedule(SNT, EVENT2, time + amt2, dp, other_sn_no);

					/* GENERATE MESSAGE DAMAGE */

					dp->flags &= ERRORMASK;

					if (found_error(msg_size, media_no))
					{
						/* MARK THE ERROR */

						dp->flags |= ERROR_BIT;

						/* UPDATE DAMAGE COUNTS */

						upd_errs(dp, cid);
					}

					/* UPDATE THE BURST COUNT */

					burst_count++;

					dp->serv_time = amt1;
					dp->delay = amt2;

					/* ENABLE THE RECEIVER */

					enable_subnet(other_sn_no);
				}
			}

			/* PASS THE TOKEN */

			time_aux = time;
			sn_no_1 = sn_no;
			sn_1 = &s[sn_no_1];
			minimo = INFINITY;
			fin = FALSE;
			do /* recorre la red como mucho una vez buscando el tiempo de un evento
		   de ejecucion anterior a la posesion del testigo */
			{
				sn_no_2 = sn_1->logical_next_station;
				sn_2 = &s[sn_no_2];
				media = &m[sn_2->media_interface];
				time += sn_1->token_proc_time + media->prop_delay + media->fddi_head_end_latency;

				if (fddi_ii)
					if (sn_no_1 == media->master)
						time += media->master_delay;

				if (sn_2->send_buf_size || (t_ex_min = siguiente_evento(sn_no_2, saved)) < time)
				/* comprueba si hay un evento de ejecucion anterior a la posesion
		     del testigo, incluyendo tramas listas para enviar */
				{
					fin = TRUE;
					break;
				}
				sn_2->min_ex_event = t_ex_min;
				if (t_ex_min < minimo)
					minimo = t_ex_min;
				sn_2->tokens++;
				if (sn_2->ttrt != NULL && !transitorio)
					promedia(sn_2->ttrt, time - sn_2->last_visit, sim_clock);
				sn_2->last_visit = time;
				sn_1 = sn_2;
				sn_no_1 = sn_no_2;
			} while (sn_no_1 != sn_no);

			if (!fin)
			{
				t_vuelta = time - time_aux;
				vueltas = floor((minimo - time) / t_vuelta);
				if (vueltas > 0.0)
				/* actualiza todos los nodos con el numero de vueltas del testigo */
				{
					t_vueltas = vueltas * t_vuelta;
					n_vueltas = (int)vueltas;
					do
					{
						sn_2 = &s[sn_1->logical_next_station];
						sn_2->tokens += n_vueltas;
						if (sn_2->ttrt != NULL && !transitorio)
							for (i = 0; i < n_vueltas; i++)
								promedia(sn_2->ttrt, t_vuelta, sim_clock);
						sn_2->last_visit += t_vueltas;
						sn_1 = sn_2;
					} while (sn_1 != sn);
					time += t_vueltas;
				}
				while (TRUE)
				/* da parte de una vuelta actualizando nodos hasta dejar el testigo */
				{
					sn_no_2 = sn_1->logical_next_station;
					sn_2 = &s[sn_no_2];
					media = &m[sn_2->media_interface];
					time += sn_1->token_proc_time + media->prop_delay + media->fddi_head_end_latency;

					if (fddi_ii)
						if (sn_no_1 == media->master)
							time += media->master_delay;

					if (sn_2->min_ex_event < time)
						/* comprueba si hay un evento de ejecucion anterior a la posesion
			 del testigo */
						break;
					sn_2->tokens++;
					if (sn_2->ttrt != NULL && !transitorio)
						promedia(sn_2->ttrt, time - sn_2->last_visit, sim_clock);
					sn_2->last_visit = time;
					sn_1 = sn_2;
					sn_no_1 = sn_no_2;
				}
			}

			if (verbose)
				printf("%f:SUBNET %d pass TOKEN to SUBNET %d\n",
					   time, sn_no, sn_no_2);

			/* SCHEDULE TOKEN ARRIVAL */

			schedule(SNT, EVENT3, time, &tkn, sn_no_2);

			/* ENABLE THE TOKEN RECEIVING SUBNET */

			enable_subnet(sn_no_2);

			/* COMPUTE THE BUSY TIME FOR THE SUBNET MODULE */

			duration = (sn->proc_send_time * burst_count);

			saved->id = EVENT3;
			saved->xtime = hold_time;

			break;

		case EVENT4: /* SEND MESSAGE TO THE NETWORK MODULE */

			/* CHECK THE INTERFACE FLOW CONTROL STATUS */

			if (!sn_to_net_int_space_avail(dest_net_no))
			{
				blocked = TRUE;
				break;
			}

			/* DETERMINE MESSAGE SIZE */

			msg_size = dp->dsize + dp->th_size + dp->nh_size;

			/* SEND THE MESSAGE */

			/* DETERMINE SEND START TIME */

			time = (sn->interface_free > sim_clock ? sn->interface_free : sim_clock);

			duration = n[dest_net_no].sn_int_notif +
					   (sn->write_time * msg_size);

			time += duration + n[dest_net_no].msg_pass_time_oh +
					(n[dest_net_no].msg_pass_time_po * msg_size);

			/* RECORD THE NEXT TIME THE INTERFACE WILL BE FREE */

			sn->interface_free = time;

			/* SCHEDULE RECEIPT BY THE NETWORK MODULE */

			ep = schedule(NTW, EVENT2, time, dp, dest_net_no);

			/* tag the event with msg order number */
			ep->msg_order = sn->order++;
			/*
	       DENOTE THAT A MESSAGE IS PENDING DELIVERY TO THE NETWORK
	    */

			sn->no_net_msg_sched = TRUE;

			/* increment count of msgs in sn->net interface */
			n[dest_net_no].net_from_sn_intf++;

			/* SCHEDULE A NEW MESSAGE FOR SENDING, IF AVAILABLE */

			sn_send_net_msg(sn, sn_no);

			/* enable receiving network */
			enable_net(dest_net_no);

			break;

		case EVENT5: /* Free a buffer slot */

			/* UPDATE TRANSMIT BUFFER AVAILABILITY */

			sn->send_buf_size--;
		}
	} while (blocked == TRUE);

	/* FALL THROUGH FOR EXECUTED EVENTS  */

	/* WRAP UP THE EXECUTION OF THE EVENT */

	saved->atime = sim_clock;

	if (debug)
		printf("%f:SUBNET %d Cid %d Event %d\n",
			   sim_clock, sn_no, cid, saved->id + 1);

	/* UPDATE QUEUE STATISTICS */

	exec_event(SNT, sn_no, cid, saved, duration);

	/* UPDATE CPU UTILIZATION */

	cpu_use(sn->clk_index, duration);

	/* DELETE THE EXECUTED EVENT */

	remove_event(saved, sn_no, SNT);

	/* UPDATE THE MODULE CLOCK BUSY */

	*sn->clock_ptr = sim_clock + duration;

	modactivity = TRUE;
}

/*#define CYCLE_LENGTH 125e-6
#define HEADER_SPEED (HEADER_SIZE / (CYCLE_LENGTH * MILLISEC))
#define DPG_SPEED (768000.0 / MILLISEC)
#define WBC_SPEED (6144000.0 / MILLISEC)
#define HEADER_SIZE (52 + wbcs * 4) */
#define ISO_CHANNEL_BW (64000.0 / MILLISEC)
#define C_MAX 13

/*
 * Reserva ancho de banda para trafico de paquetes y para trafico 
 * isocrono.
 */
int channel_allocator(struct media_info *media_struct, int nets)
{
	/*  int i;*/
	int no_of_channels, cycles, wbcs;
	float delay, bandwidth, rem;

	/* Allocate DPG bandwidth */
	bandwidth = (media_struct->link_speed - DPG_SPEED);

	/* Max. # of WBC's & remaining bandwidth */
	wbcs = (int)(bandwidth / WBC_SPEED);
	bandwidth -= (wbcs * WBC_SPEED + HEADER_SPEED);

	/* Not enough bandwidth */
	if (bandwidth < 0.0)
		wbcs--;

	if (wbcs == 0)
		p_error("El medio no soporta canales WBC.");

	/* Usable bandwidth */
	bandwidth = (HEADER_SPEED + DPG_SPEED + (WBC_SPEED * wbcs));

	/* New fields in media_info struct */
	media_struct->current_link_speed = bandwidth;
	media_struct->channels = wbcs;
	media_struct->header_bandwidth = (HEADER_SPEED);

	/* Isochronous bandwidth requirement */
	bandwidth = media_struct->no_of_calls * ISO_CHANNEL_BW;
	rem = (bandwidth / WBC_SPEED);

	if ((rem - (int)rem) == 0.0)
		no_of_channels = (int)rem;
	else
		no_of_channels = (int)(bandwidth / WBC_SPEED + 1);

	if (no_of_channels > media_struct->channels)
		p_error("No hay ancho de banda suficiente para el trafico isocrono\n");

	/* Allocatable isochronous traffic */
	media_struct->iso_bandwidth = no_of_channels * WBC_SPEED;

	/* Remaining bandwidth is packet bandwdith */
	media_struct->packet_bandwidth = media_struct->current_link_speed -
									 media_struct->header_bandwidth -
									 media_struct->iso_bandwidth;

	/* Update token length */
	media_struct->token_length *= ((media_struct->link_speed) / (media_struct->packet_bandwidth));

	/* Compute delay in LAB buffer */

	/* Time to round the ring (miliseconds) */
	delay = (media_struct->prop_delay + media_struct->fddi_head_end_latency) * nets;

	/* Ring length in cycles */
	rem = (delay / (CYCLE_LENGTH * MILLISEC));
	if ((rem - (int)rem) == 0)
		cycles = (int)rem;
	else
		cycles = (int)(delay / (CYCLE_LENGTH * MILLISEC) + 1);

	if (cycles > C_MAX)
		p_error("El anillo es demasiado grande.");

	/* LAB buffer delay */
	media_struct->master_delay = cycles * CYCLE_LENGTH * MILLISEC - delay;

	/*
   * Assign master station randomly 
   * Note that master station first receive the token
   */

	/*
    for (i = 0; i < nets - 1; i++)
    
    if (bernoulli ((float) (1.0 / (float) nets)))
    break;
	
    media_struct->master = i;
  */

	m_delay = delay;
	m_cycles = cycles;
	m_wbcs = wbcs;
	m_no_of_channels = no_of_channels;

	/* Checks */
	if (verbose)
	{
		printf("\nEstacion Master: %d\n", media_struct->master);
		printf("Latencia del anillo: %f ms -> %d ciclo(s)\n", delay, cycles);
		printf("Retardo LAB buffer: %f ms\n", media_struct->master_delay);
		printf("BW Trafico isocrono: %f Kbps", media_struct->iso_bandwidth);
		printf(" (%2.2f%c)\n", media_struct->iso_bandwidth / media_struct->current_link_speed * 100, 37);
		printf("# WBCs ocupados: %d de %d (%2.2f%c)\n", no_of_channels, wbcs, no_of_channels * 100.0 / wbcs, 37);
		printf("BW Trafico de Paquetes: %f Kbps ", media_struct->packet_bandwidth);
		printf("(%2.2f%c)\n", media_struct->packet_bandwidth / media_struct->current_link_speed * 100, 37);
		printf("Longitud del testigo: %f ms\n", media_struct->token_length);
	}
	return (TRUE);
}
