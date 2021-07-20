/*  VERSION 2.0 - COLUMBUS DMS
file - print.c

Utilities for the simulation model

*/
char PrintSid[]="@(#)print.c	4.3 06/30/98";

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <signal.h>
#include "defs.h"
#include "types.h"
#include "globals.h"

/*
Print the specific data queue (IDU, DT, AK) that is passed to this function.
*/
static void doprint(struct data *d, /* queue to print */
					FILE *fp)
{
    struct data *p;
    fprintf(fp,
	 "type seq#  flag     use  dsize th  nh  sh  err eot trs cdt  xmark");

#ifdef RANGE_ACK
    fprintf(fp, " low   high");
#endif
#ifdef SELECT_ACK
    fprintf(fp, "  one_dt");
#endif
    fprintf(fp, "\n");

    for (p = d; p != NULL; p = p->next_data)
    {
	CHK_CNTLC
	    switch (p->flags & MASK)
	{

	  case DT:
	    fprintf(fp, "DT  ");
	    break;

	  case XDT:
	    fprintf(fp, "XDT ");
	    break;

	  case AK:
	    fprintf(fp, "AK  ");
	    break;

	  case XAK:
	    fprintf(fp, "XAK ");
	    break;

	  case IDU:
	    fprintf(fp, "IDU ");
	    break;

	  case XIDU:
	    fprintf(fp, "XIDU");
	    break;
	}

	fprintf(fp, " %5ld %8x %3d %5ld %3ld %3ld %3ld ",
		p->seq_num, p->flags, p->uses, p->dsize, p->th_size,
		p->nh_size, p->snh_size);

	if ((p->flags & ERROR_BIT) == ERROR_BIT)
	    fprintf(fp, "T   ");
	else
	    fprintf(fp, "F   ");

	if ((p->flags & EOT) == EOT)
	    fprintf(fp, "T   ");
	else
	    fprintf(fp, "F   ");

	fprintf(fp, "%3d %4ld ", p->trans, p->cdt);

	if (((p->flags & MASK) == AK) || ((p->flags & MASK) == XAK))
	{

#ifdef RANGE_ACK
	    fprintf(fp, "%5d %5d %5d", p->x_mark, p->low, p->high);
#endif
#ifdef SELECT_ACK
	    fprintf(fp, "%5d %5d", p->x_mark, p->one_dt);
#endif
	}

	else
	    fprintf(fp, "%5ld", p->x_mark);

	fprintf(fp, "\n");
    }
}

static void print_dts(FILE *fp)
{
    fprintf(fp, "\n\nUsed DT and XDT data heap common to all TPs and NET\n\n");
    doprint(dt_heap, fp);
}

static void print_aks(FILE *fp)
{
    fprintf(fp, "\n\nUsed AK and XAK data heap common to all TPs and NET\n\n");
    doprint(ak_heap, fp);
}

/*
Print the data queues (IDUs, DT, AK)
*/
static void print_dqueue(FILE *fp)
{
    print_dts(fp);

    print_aks(fp);
}

/*
	Print a data structure as AK, DT, XAK, IDU or XIDU.
*/
static void print_data(FILE *fp, struct data *dp)
{
    int cid, source, dest;
    int type;
    cid = (dp->flags >> CSHIFT) & XDTMASK;	/* SES */
    source = (dp->flags >> SSHIFT) & XDTMASK;	/* SES */
    dest = (dp->flags >> DSHIFT) & XDTMASK;	/* SES */
    type = dp->flags & MASK;

    switch (type)
    {
      case IDU:

	fprintf(fp, "IDU# %ld", dp->seq_num);
	if (dp->flags & START_TSDU)
	    fprintf(fp, ":ST");
	if (dp->flags & EOT)
	    fprintf(fp, ":ET");
	break;

      case XIDU:

	fprintf(fp, "XIDU# %ld:EOT", dp->seq_num);
	break;

      case XDT:

	fprintf(fp, "XDT# %ld:TR %d", dp->seq_num, dp->trans);
	if (dp->flags & ERROR_BIT)
	    fprintf(fp, ":ER");
	break;

      case XAK:

	fprintf(fp, "XAK# %ld", dp->seq_num);
	if (dp->flags & ERROR_BIT)
	    fprintf(fp, ":ER");
	break;

      case DT:

	fprintf(fp, "DT# %ld:TR %d", dp->seq_num, dp->trans);
	if (dp->flags & START_TSDU)
	    fprintf(fp, ":ST");
	if (dp->flags & EOT)
	    fprintf(fp, ":ET:H %ld", dp->hold);
	if (dp->flags & ERROR_BIT)
	    fprintf(fp, ":ER");
	if (dp->x_mark > -1)
	    fprintf(fp, ":M %ld", dp->x_mark);
	break;

      case AK:

	fprintf(fp, "AK# %ld", dp->seq_num);
	if (dp->flags & ERROR_BIT)
	    fprintf(fp, ":ER");
#ifdef SELECT_ACK
	if (dp->one_dt > -1)
	    fprintf(fp, ":SA# %d", dp->one_dt);
#endif
#ifdef RANGE_ACK
	fprintf(fp, "L# %d:H# %d", dp->low, dp->high);
#endif
	break;

      default:

	fprintf(fp, "UNK# %ld", dp->seq_num);
	break;
    }
    fprintf(fp, ":DS %ld:TH %ld:NH %ld:SH %ld:U %d",
	    dp->dsize, dp->th_size, dp->nh_size, dp->snh_size, dp->uses);
    fprintf(fp, ":C %d:S %d:D %d\n", cid, source, dest);
}

/*
Print event queue that is passed to this function.
*/
static void equeue(struct event *eq, FILE *fp)
/* struct event *eq: event queue to print */
{
    struct event *e;
    fprintf(fp, "id\txtime\tatime\tstatus\n");

    for (e = eq; e != NULL; e = e->next_event)
    {
	CHK_CNTLC
	    fprintf(fp, "%d\t%.3f\t%.3f\t", (e->id + 1), e->xtime, e->atime);
	if (e->data_p == NULL)
	    fprintf(fp, "DEAD\n");
	else
	{
	    fprintf(fp, "ACTIVE\n");
	    print_data(fp, e->data_p);
	}
    }
}

/*
Print event queues (user(s), tp(s), network).
*/
static void print_equeue(FILE *fp)
{
    int i;
    for (i = 0; i < no_of_nets; i++)
    {
	CHK_CNTLC
	    fprintf(fp, "\n\nTP%d Event Queue\n\n", i);
	equeue(n[i].event_q_head, fp);
    }

    for (i = 0; i < no_of_nets; i++)
    {
	CHK_CNTLC
	    fprintf(fp, "\n\nTP%d Event Queue\n\n", i);
	equeue(s[i].event_q_head, fp);
    }
}

/*
	Print information about network(s) - their structure.
*/
void print_ni(FILE *fp, int inst)
{
    int i;
    int start, end;
    if (inst < 0 || inst >= no_of_nets)
    {
	start = 0;
	end = no_of_nets;
    }
    else
    {
	start = inst;
	end = start + 1;
    }

    for (i = start; i < end; i++)
    {
	fprintf(fp, "\n\nNet# %d Info\n\n", i);

	fprintf(fp, "clock\tenable\n");
	fprintf(fp, "%.3f\t%d\n\n",
		*n[i].clock_ptr, n[i].enable);

	fprintf(fp, "Simulated CPU# %d\n\n", n[i].clk_index);

	fprintf(fp, "s_int\n");
	fprintf(fp, "%d\n", n[i].net_from_sn_intf);

	fprintf(fp, "sn_notif\n");
	fprintf(fp, "%.3f\n\n",
		n[i].sn_int_notif);

	fprintf(fp, "s_data\ts_head\ts_msgs\n");
	fprintf(fp, "%ld\t%ld\t%ld\n\n",
		n[i].sent_net_data_bytes,
		n[i].sent_net_header_bytes,
		n[i].sent_net_messages);

	fprintf(fp, "r_data\tr_head\tr_msgs\n");
	fprintf(fp, "%ld\t%ld\t%ld\n\n",
		n[i].recv_net_data_bytes,
		n[i].recv_net_header_bytes,
		n[i].recv_net_messages);
    }
}

/*

	Print information about subnet(s) - their structure.
*/
void print_si(FILE *fp, int inst)
{
    int i;
    int start, end;
    int pri;
    struct buf *b;
    if (inst < 0 || inst >= no_of_nets)
    {
	start = 0;
	end = no_of_nets;
    }
    else
    {
	start = inst;
	end = start + 1;
    }

    for (i = start; i < end; i++)
    {
	fprintf(fp, "\n\nSubnet# %d Info\n\n", i);

	fprintf(fp, "clock\tenable\tsubnet_head\n");
	fprintf(fp, "%.3f\t%d\t%d\n\n",
		*s[i].clock_ptr, s[i].enable, s[i].subnet_head);

	fprintf(fp, "Simulated CPU# %d MEDIA# %d\n\n",
		s[i].clk_index, s[i].media_interface);

	fprintf(fp, "n_int\tnmsg\tm_drop\tc_drop\n");
	fprintf(fp, "%d\t%d\t%ld\t%ld\n\n",
		s[i].sn_from_net_intf, s[i].no_net_msg_sched,
		s[i].mem_drops, s[i].crc_drops);
	fprintf(fp, "order\tsproc\trproc\n");
	fprintf(fp, "%d\t%.3f\t%.3f\n\n",
		s[i].order, s[i].proc_send_time, s[i].proc_recv_time);

	fprintf(fp, "write\tread\n");
	fprintf(fp, "%.3f\t%.3f\n\n",
		s[i].write_time, s[i].read_buf_time);

	fprintf(fp, "sq_lim\tsq_siz\trq_lim\trq_siz\n");
	fprintf(fp, "%d\t%d\t%d\t%d\n\n",
		s[i].send_buf_bound, s[i].send_buf_size,
		s[i].rec_buf_bound, s[i].rec_buf_size);

	fprintf(fp, "s_data\ts_head\ts_msgs\n");
	fprintf(fp, "%ld\t%ld\t%ld\n\n",
		s[i].sent_subnet_data_bytes,
		s[i].sent_subnet_header_bytes,
		s[i].sent_subnet_messages);

	fprintf(fp, "r_data\tr_head\tr_msgs\n");
	fprintf(fp, "%ld\t%ld\t%ld\n\n",
		s[i].recv_subnet_data_bytes,
		s[i].recv_subnet_header_bytes,
		s[i].recv_subnet_messages);
	if (m[s[i].media_interface].medium_type == TOKEN_HI_SPEED)
	{
	    fprintf(fp, "tkproc\tnxt_sta\n");
	    fprintf(fp, "%.3f\t%d\n\n",
		    s[i].token_proc_time,
		    s[i].logical_next_station);

	    fprintf(fp, "trials\tvisit\n");
	    fprintf(fp, "%ld\t%.3f\n",
		    s[i].tokens,
		    s[i].last_visit);

	    fprintf(fp, "class\ttarget\tlast_token_time\n");
	    fprintf(fp, "%s\t%.3f\t%.3f\n",
		    "Sync.",
		    s[i].target_time_syn,
		    s[i].last_token_time_syn);

	    for (pri = NO_LEVELS_PRI - 1; pri >= 0; pri--)
	    {
		fprintf(fp, "P.%d\t%.3f\t%.3f\n",
			pri,
			s[i].target_time[pri],
			s[i].last_token_time[pri]);
	    }

	    fprintf(fp, "\nReceive Buffer\n\n");
	    fprintf(fp, "seq#  flags   use dsize th  nh  sh  \n");
	    for (b = s[i].Rx_Q.head; b != NULL; b = b->next_dt)
	    {
		fprintf(fp, "%5ld %8x %3d %5ld %3ld %3ld %3ld\n",
			b->dt->seq_num, b->dt->flags, b->dt->uses,
			b->dt->dsize, b->dt->th_size, b->dt->nh_size,
			b->dt->snh_size);
	    }

	    fprintf(fp, "\n\n Synch. Traffic Transmit Buffer\n\n");
	    fprintf(fp, "seq#  flags   use dsize th  nh  sh  \n");
	    for (b = s[i].Tx_Q[NO_LEVELS_PRI].head; b != NULL; b = b->next_dt)
	    {
		fprintf(fp, "%5ld %8x %3d %5ld %3ld %3ld %3ld\n",
			b->dt->seq_num, b->dt->flags, b->dt->uses,
			b->dt->dsize, b->dt->th_size, b->dt->nh_size,
			b->dt->snh_size);
	    }

	    for (pri = NO_LEVELS_PRI - 1; pri >= 0; pri--)
	    {

		fprintf(fp, "\n\n Asynch. Priority %d Transmit Buffer\n\n", pri);
		fprintf(fp, "seq#  flags   use dsize th  nh  sh  \n");
		for (b = s[i].Tx_Q[pri].head; b != NULL; b = b->next_dt)
		{
		    fprintf(fp, "%5ld %8x %3d %5ld %3ld %3ld %3ld\n",
			    b->dt->seq_num, b->dt->flags, b->dt->uses,
			    b->dt->dsize, b->dt->th_size, b->dt->nh_size,
			    b->dt->snh_size);
		}

	    }

	}

    }
}

/*
	Print information about media - their structure.
*/
void print_mi(FILE *fp, int inst)
{
    int start, end;
    int i;
    struct enode *ep;
    if (inst < 0 || inst >= no_of_media)
    {
	start = 0;
	end = no_of_media;
    }
    else
    {
	start = inst;
	end = start + 1;
    }

    for (i = start; i < end; i++)
    {
	fprintf(fp, "MEDIA# %d INFO TYPE IS %d\n\n", i,
		m[i].medium_type);

	fprintf(fp, "erate\ttx_spd\tprop\tcrc_on\tbyte_ok\n");
	fprintf(fp, "%.3E\t%.3f\t%.3f\t%d\t%.8f\n\n",
		m[i].error_rate, m[i].link_speed, m[i].prop_delay,
		m[i].crc_on, m[i].byte_ok_prob);
	fprintf(fp, "byte_count\tno_of_stations\n");
	fprintf(fp, "%15.0f\t%d\n\n", m[i].byte_count, m[i].no_of_stations);

	if (m[s[i].media_interface].medium_type == TOKEN_HI_SPEED)
	{
	    fprintf(fp, "TLENGTH\tLATENCY\tPRE\n");
	    fprintf(fp, "%.3f\t%.3f\t%d\n\n",
		    m[i].token_length,
		    m[i].fddi_head_end_latency,
		    m[i].preamble);
	}

	for (ep = m[i].head; ep != NULL; ep = ep->next)
	    fprintf(fp, "size : %d\tcutoff : %ld\n", ep->size,
		    ep->cutoff);
	fprintf(fp, "\n");

    }
}

/*
	Print information about connections - their structure.
*/
void print_ci(FILE *fp, int inst)
{
    int start, end;
    int i;
    if (inst < start_cid || inst >= no_conns + start_cid)	/* SESA-SWAR #3 */
    {
	start = start_cid;
	end = start_cid + no_conns;
    }
    else
    {
	start = inst;
	end = start + 1;
    }

    for (i = start; i < end; i++)
    {
	fprintf(fp, "CONNECTION# %d INFO\n\n", i);

	fprintf(fp, "BETWEEN USER %d AND USER %d\n\n",
		c[i].c_end1, c[i].c_end2);

	fprintf(fp, "tpdu_sz\tcsum_on\tseq_sz\n");
	fprintf(fp, "%d\t%d\t%d\n\n",
		c[i].tpdu_size, c[i].tp_cksum_on,
		c[i].norm_seq_size);

	fprintf(fp, "ak_hd\txak_hd\tdt_hd\txdt_hd\n");
	fprintf(fp, "%d\t%d\t%d\t%d\n\n",
		c[i].ak_head,
		c[i].xak_head,
		c[i].dt_head,
		c[i].xdt_head);
#ifndef NBS_EXP
#ifndef OSI_EXP
	fprintf(fp, "mark_sz = %d  ", c[i].mark_size);
#endif
#endif
#ifdef SELECT_ACK
	fprintf(fp, "sel_ack_sz = %d ", c[i].sel_ack_size);
#endif
	fprintf(fp, " qos = %d\n", c[i].qos);
    }
}

/*
All information about state of model is printed out to file simout.log.
User, tp and network structures, data lists and event queues are printed.
*/
static void print_info()
{
    FILE *ofp;
    ofp = fopen("simout.log", "w");

    print_ni(ofp, ALL);
    print_si(ofp, ALL);
    print_mi(ofp, ALL);
    print_ci(ofp, ALL);
    print_dqueue(ofp);
    print_equeue(ofp);

    fclose(ofp);
}

/*
Print Error is called when an error is found.  It will dump all the
event queues and module structures to file simout.log if
oqueues is ON.
*/
void p_error(char *s)
{
  
    if (s) fprintf(efp, "%s\n", s);
    fprintf(efp, "clock is %f\n", sim_clock);
    fclose(efp);

    if (oqueues)
	print_info();
    if (strstr(s, "finalizada"))
      exit(0);
    exit(1);
}
