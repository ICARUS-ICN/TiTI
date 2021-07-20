/*  VERSION 1.0 - NBS

file - stats.c

This file contains various statistics and measurement routines for the
simulation model.

*/
char StatsSid[]="@(#)stats.c	4.2 06/30/98";

#include <stdio.h>
#include <math.h>
#include <signal.h>
#include "defs.h"
#include "types.h"
#include "globals.h"

/*
	Update Network Send Accumulators
*/
void upd_ns_stats(int net_no, int data, int header, int conn)
{
    stats[conn].net_messages++;
    stats[conn].nhdr_bits += (header * 8);
    stats[conn].ndata_bits += (data * 8);

    n[net_no].sent_net_data_bytes += data;
    n[net_no].sent_net_header_bytes += header;
    n[net_no].sent_net_messages++;
}

/*
	Update Network Receive Accumulators
*/
void upd_nr_stats(int net_no, int data, int header)
{
    n[net_no].recv_net_data_bytes += data;
    n[net_no].recv_net_header_bytes += header;
    n[net_no].recv_net_messages++;
}

/*
	Update Subnet Send Accumulators
*/
void upd_sns_stats(int sn_no, int data, int header, int conn)
{
    stats[conn].subnet_messages++;
    stats[conn].snhdr_bits += (header * 8);
    stats[conn].sndata_bits += (data * 8);

    s[sn_no].sent_subnet_data_bytes += data;
    s[sn_no].sent_subnet_header_bytes += header;
    s[sn_no].sent_subnet_messages++;
}

/*
	Update Subnet Receive Accumulators
*/
void upd_snr_stats(int sn_no, int data, int header)
{
    s[sn_no].recv_subnet_data_bytes += data;
    s[sn_no].recv_subnet_header_bytes += header;
    s[sn_no].recv_subnet_messages++;

}

void upd_sn_drops(struct data *msg, int conn)
{
    switch (msg->flags & MASK)
    {
      case DT:
	stats[conn].dt_drops++;
	break;

      case XDT:
	stats[conn].xdt_drops++;
	break;

      case AK:
	stats[conn].ak_drops++;
	break;

      case XAK:
	stats[conn].xak_drops++;
	break;
    }
}

/*
	Update Error Counters
*/
void upd_errs(struct data *dp, int conn)
{
    switch (dp->flags & MASK)
    {
      case DT:

	stats[conn].dt_errors++;
	break;

      case XDT:

	stats[conn].xdt_errors++;
	break;

      case AK:

	stats[conn].ak_errors++;
	break;

      case XAK:

	stats[conn].xak_errors++;
	break;

      default:

	return;
    }

    err_cnt++;
}

/*

*/
void cpu_use(int processor, float amount)
{
    cpus[processor] += amount;
}
