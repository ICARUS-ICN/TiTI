/* VERSION 2.0 - COLUMBUS DMS

file - event.c

This file contains event management routines for the
simulation model.  The
functions include event scheduling, removing, cancelling, and
finding.
Also included are the functions for maintaining event queueing
statistics
and controlling the event execution status of simulated
modules.

*/
char EventSid[]="@(#)event.c	4.1 10/02/97";

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <signal.h>
#include "defs.h"
#include "types.h"
#include "globals.h"
/*

The following functions enable specific modules.  One enables
all modules
(i.e., tp, user, network, subnet) and a separate function
exists to enable exactly
one module -- one for subnet, one for network, one for
transport, one for user.
*/
void enable_mods()
{
    int i;
    struct subnet_info *sn;
    struct net_info *net;

    for (i = 0, net=&n[0]; i < no_of_nets; i++, net++)
      if (net->event_q_head != NULL)
	net->enable = TRUE;

    for (i = 0, sn=&s[0]; i < no_of_nets; i++, sn++)
      if (sn->event_q_head != NULL)
	sn->enable = TRUE;

}

void enable_net(int nn)
{
    n[nn].enable = TRUE;
}

void enable_subnet(int sn)
{
    s[sn].enable = TRUE;
}

/*
Schedule an event given event id, execution time and modules to
schedule it for.  The event is put on the priority event queue in
time order
*/
struct event *schedule(int type, int event_num, double ex_time,
		       struct data *data_ptr, int num)
/* int type: module type (user, tp, network, subnet) */
/* int event_num: which event to schedule - the event id */
/* double ex_time: execution time for the event */
/* struct data *data_ptr: pointer to data associated with event */
/* int num: module # */
{
    struct event *e, *new_event;
    struct event *saved, *bottom_ptr, *head_ptr;
    int pr;
    /* every event must have data associated with it */

    if (data_ptr == NULL)
    {
	metrics(m1);
	p_error("NULL data ptr passed to schedule");
    }
#ifdef DEBUG
    /* If the data pointer is wrong, no use continuing */

    if (((data_ptr->flags & MASK) < DT) || ((data_ptr->flags & MASK) > XIDU))
    {
	if (xmode == BATCH)
	{
	    metrics(m1);
	    p_error(bptrmsg);
	}
	else
	{
	    fprintf(termid, "TYPE %d NUM %d EID %d XTIME %f DP %x FLAGS %x\n",
		type, num, event_num + 1, ex_time, data_ptr, data_ptr->flags);
	    tpause(bptrmsg);
	}
    }
#endif
    /* make a new event */

    if((new_event = ((struct event *) malloc(sizeof(struct event))))==NULL)
      p_error("insuficiente memoria HEAP para la estructura de un evento\n");


    /* switch according to module type */

    switch (type)
    {
      case NTW:

	pr = n[num].prio_tab[event_num];
	/*
	   if this is the first event being scheduled for network, reset its
	   clock
	*/

	if (*n[num].clock_ptr > VERY_LG_NO)
	    *n[num].clock_ptr = 0;

	/* keep a sequential thread through the queue */

	new_event->next_event = n[num].event_q_head;
	n[num].event_q_head = new_event;

	bottom_ptr = n[num].n_queue[pr].end;
	head_ptr = n[num].n_queue[pr].head;
	break;

      case SNT:

	pr = s[num].prio_tab[event_num];
	/*
	   if this is the first event being scheduled for subnet, reset its
	   clock
	*/

	if (*s[num].clock_ptr > VERY_LG_NO)
	    *s[num].clock_ptr = 0;

	/* keep a sequential thread through the queue */

	new_event->next_event = s[num].event_q_head;
	s[num].event_q_head = new_event;

	bottom_ptr = s[num].sn_queue[pr].end;
	head_ptr = s[num].sn_queue[pr].head;
	break;
    }

    /* Doublely link the big event list */

    new_event->prev_event = NULL;
    if (new_event->next_event != NULL)
	new_event->next_event->prev_event = new_event;
    /*
       the new event is put in time order on appropriate priority event queue
    */
    /* queue searched bottom up */

    for (e = bottom_ptr; ((e != NULL) && (e->xtime > ex_time)); e = e->prev_aevent);

    if (e != NULL)
    {
	saved = e->next_aevent;
	e->next_aevent = new_event;
    }
    else
	saved = head_ptr;

    new_event->prev_aevent = e;
    new_event->next_aevent = saved;

    if (saved != NULL)
	saved->prev_aevent = new_event;

    switch (type)
    {
      case NTW:

	if (new_event->prev_aevent == NULL)	/* new event is head of queue */
	    n[num].n_queue[pr].head = new_event;

	if (new_event->next_aevent == NULL)	/* new event is end of queue */
	    n[num].n_queue[pr].end = new_event;

	break;

      case SNT:

	if (new_event->prev_aevent == NULL)	/* new event is head of queue */
	    s[num].sn_queue[pr].head = new_event;

	if (new_event->next_aevent == NULL)	/* new event is end of queue */
	    s[num].sn_queue[pr].end = new_event;

	break;

    }

    new_event->xtime = ex_time;
    new_event->id = event_num;
    new_event->priority = pr;
    new_event->data_p = data_ptr;

    new_event->atime = 0;

    /* check for scheduling error */

    if (new_event->next_aevent == new_event)
    {
	fprintf(efp, "%f: MOD %d #%d EVENT %d->%x DATA->%x ETIME %f\n",
		sim_clock, type, num, event_num, (unsigned int) new_event,
		(unsigned int) new_event->data_p, new_event->xtime);
	metrics(m1);
	p_error("event scheduled pointing to itself");
    }
    return (new_event);
}

/*
Get the next event to execute. Highest priority events go
first, in
execution time order.
Returns pointer to event.
*/
struct event *getevent(int type, int num, struct event* cur_event)
/* int type: module type (tp, user, network) */
/* int num: user# or tp# */
/* struct event *cur_event: current event is NULL if this is first pass */
{
    struct event *e;
    int i;
    int pr;
    struct subnet_info *sn;
    struct net_info *net;
    struct e_queues *ppr;

    sn = &s[num];
    net = &n[num];

    if (cur_event != NULL)
    {

	/* the previous selected event was blocked, therefore get */
	/* the next one on the queue */

	e = cur_event->next_aevent;

	if ((e != NULL) && (e->xtime <= sim_clock))
	    return (e);

	/* the current event was the end of the queue, so get */
	/* event on next lowest priority queue */
	/* find what priority queue the current event is on */

	switch (type)
	{

	  case NTW:
	    pr = net->prio_tab[cur_event->id];
	    break;

	  case SNT:
	    pr = sn->prio_tab[cur_event->id];
	    break;

	}
    }
    else
	/* first time through, start at highest priority queue */

	pr = MAX_PRIORITY;

    /* starting at appropriate priority queue, check the first event */
    /* in each queue for execution */

    pr--;
    switch (type)
      {
	
      case NTW:
	ppr = &net->n_queue[pr];
	break;
	
      case SNT:
	ppr = &sn->sn_queue[pr];
	break;
      }

    for (i = pr; i >= 0; i--, ppr--)
    {
      e = ppr->head;

	/* if the queue is empty, check the next one */

	if (e == NULL)
	    continue;
	/*
	   if the first event in the queue is eligible for execution, then
	   return it
	*/

	if (e->xtime <= sim_clock)
	    return (e);
    }

    /* all event in all queues can not be executed */

    return (ALLBLOCKED);
}

/*
Remove an event from appropriate priority event queue for the module
(user, tp, network)
If oqueues is ON, event space is not freed, it is no longer
in active event list, but it remains in the big double thread list.
*/
struct data *remove_event(struct event *ep, int num, int type)
/* struct event *ep: event to be removed */
/* int num: module# */
/* int type: user, tp, network, or subnet */
{
    int pr;
    struct data *dp;
    /* save the pointer to the event data */

    dp = ep->data_p;

    /* if the first event in the queue is being removed, */
    /* change the head pointer */

    if (ep->prev_aevent != NULL)
	ep->prev_aevent->next_aevent = ep->next_aevent;
    else
    {
	switch (type)
	{

	  case NTW:
	    pr = n[num].prio_tab[ep->id];
	    n[num].n_queue[pr].head = ep->next_aevent;
	    break;

	  case SNT:
	    pr = s[num].prio_tab[ep->id];
	    s[num].sn_queue[pr].head = ep->next_aevent;
	    break;
	}
    }

    /* if the last event in the queue is being removed, */
    /* change the end pointer */

    if (ep->next_aevent != NULL)
	ep->next_aevent->prev_aevent = ep->prev_aevent;
    else
    {
	switch (type)
	{

	  case NTW:
	    pr = n[num].prio_tab[ep->id];
	    n[num].n_queue[pr].end = ep->prev_aevent;
	    break;

	  case SNT:
	    pr = s[num].prio_tab[ep->id];
	    s[num].sn_queue[pr].end = ep->prev_aevent;
	    break;
	}
    }
    /*
       If not saving event queues for later output, remove the event from the
       big list and free the space.
    */

    if (!oqueues)
    {
	if (ep->next_event != NULL)
	    ep->next_event->prev_event = ep->prev_event;
	if (ep->prev_event != NULL)
	    ep->prev_event->next_event = ep->next_event;
	else
	{
	    switch (type)
	    {
	      case NTW:
		n[num].event_q_head = ep->next_event;
		break;

	      case SNT:
		s[num].event_q_head = ep->next_event;
		break;
	    }
	}

	free(ep);
    }
    else
	ep->data_p = NULL;

    return (dp);
}

/* Update the statistics associated with event execution.
Include service time and queuing delay for the event.
*/
void exec_event(int mod, int num, int cid, struct event *eptr,
		float duration)
/* int mod: module type */
/* int num: module number */
/* int cid: connection number */
/* struct event *eptr: pointer to the event */
/* float duration: service time for the event */
{

    evts_exec++;

}
