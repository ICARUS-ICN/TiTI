/*  VERSION 2.0 - COLUMBUS DMS
file - network.c
*/
char NetworkSid[]="@(#)network.c	4.1 10/02/97";

#include <stdio.h>
#include "defs.h"
#include "types.h"
#include "globals.h"

void network(int net_no)
{
  struct event *saved;
  struct data *dp;
  double time;
  float duration;
  int cid, i;
  int blocked;
  long size;
  struct net_info *net;

  saved = NULL;
  net = &n[net_no];

  do
    {
      /* getevent returns next event in priority order to execute */
      if ((saved = getevent(NTW, net_no, saved)) == ALLBLOCKED)
	{

	  /* all executable events are blocked */
	  net->enable = FALSE;
	  return;
	}
      cid = ((saved->data_p->flags >> CSHIFT) & XDTMASK); /* connection id -SES */

      if (debug)
	printf("network %d event %d\n", net_no, saved->id + 1);

      blocked = FALSE;
      dp = saved->data_p;

      switch (saved->id)
	{

	case EVENT1:		/* send msg to subnet */
	  Q_msg(net_no, cid, &c[cid].tgen);
	  if (!net_to_sn_int_space_avail(net_no))
	    {
	      /* CUENTA LA PERDIDA */

	      net->perdidas++;

	      saved->atime = LOST;

	      /* AVISA DE LA PERDIDA */

	      if (verbose)
		{
		  printf("%f:NETWORK %d msg %ld for cid %d PERDIDO\n",
			 sim_clock, net_no, dp->seq_num, cid);
		}

	      /* ELIMINA LA TRAMA */

	      dp = remove_event(saved, net_no, NTW);
	      free_data(dp);

	      /* SALTA AL SIGUIENTE EVENTO */

	      blocked = TRUE;
	      break;

	    }
	  size = dp->dsize;
	  upd_ns_stats(net_no, size, dp->nh_size, cid);
	  duration = 0;
	  time = sim_clock;
	  schedule(SNT, EVENT1, time, dp, net_no);
	  s[net_no].sn_from_net_intf++;
	  net->s_interface_free = time;
	  enable_subnet(net_no);
	  break;

	case EVENT2:
	  net->net_from_sn_intf--;
	  s[net_no].rec_buf_size--;

	  if (!dp->frame_num && (dp->first ||
				 (!c[cid].lost &&
				  c[cid].r_pkt_num == dp->pkt_num &&
				  c[cid].r_frame_num == dp->frame_num + 1
				 ))
	      )
	    /* ha recibido un paquete bien */
	    {
	      if (c[cid].perdidas)
		{
		  for (i = c[cid].r_well_num + 1; i < dp->pkt_num; i++)
		    promedia(c[cid].perdidas, 1, sim_clock);
		  c[cid].r_well_num = dp->pkt_num;
		  promedia(c[cid].perdidas, 0, sim_clock);
		}

	      if (c[cid].retardo)
		{
		  if (dp->first)
		    promedia(c[cid].retardo,
			     sim_clock - dp->gen_time - dp->serv_time - dp->delay,
			     sim_clock);
		  else
		    promedia(c[cid].retardo,
			     sim_clock - dp->gen_time - c[cid].acum_serv_time -
			     dp->serv_time - dp->delay,
			     sim_clock);
		}
	    }
	  else
	    {
	      if (dp->first)
		{
		  c[cid].lost = FALSE;
		  c[cid].r_pkt_num = dp->pkt_num;
		  c[cid].r_frame_num = dp->frame_num;
		  c[cid].acum_serv_time = dp->serv_time;
		}
	      else
		if (!(c[cid].r_pkt_num == dp->pkt_num &&
		      dp->frame_num == --c[cid].r_frame_num
		      ))
		  c[cid].lost = TRUE;
	      else
		c[cid].acum_serv_time += dp->serv_time;
	    }

	  size = dp->dsize;
	  upd_nr_stats(net_no, size, dp->nh_size);
	  octs_rcvd += size;
	  c[cid].tot_oct_recvd += size;
	  duration = 0;
	  enable_subnet(net_no);
	  break;
	}

    } while (blocked == TRUE);

  /* record the actual time the event was executed */
  saved->atime = sim_clock;

  /* Network events are always active */

  dead_clock = sim_clock;

  if (debug)
    printf("%f:NET %d Cid %d Event %d Dead Clock is %f\n", sim_clock, net_no, cid, saved->id + 1, dead_clock);

  /* update event statistics */

  exec_event(NTW, net_no, cid, saved, duration);

  /* remove the event that was just executed */
  if (saved->id == EVENT2)
    {
      dp = remove_event(saved, net_no, NTW);
      free_data(dp);
    }
  else
    remove_event(saved, net_no, NTW);

  /* update CPU use */
  cpu_use(net->clk_index, duration);

  /* network's clock is advanced */
  *net->clock_ptr = sim_clock + duration;

  modactivity = TRUE;
}
