#include <stdlib.h>
#include "list.h"

EMDCsamples* init_samples(EMDCaction _a, short _more)
{
	EMDCsamples *ss = (EMDCsamples*)malloc(sizeof(EMDCsamples));
	ss->a = _a;
	ss->more = _more;
	ss->head = ss->tail = NULL;
	ss->next = NULL;
	return ss;
}

EMDCmsg* init_message(EMDCmsg_type t)
{
	EMDCmsg* m = (EMDCmsg*)malloc(sizeof(EMDCmsg));
	m->type = t;
	m->head = m->tail = NULL;
	return m;
}

void free_samples (EMDCsamples* ss)
{
	EMDCsample* curr_sample = ss->head;
	while (curr_sample != NULL)
	{
		EMDCsample* next = curr_sample->next;
		free (curr_sample);
		curr_sample = next;
	}
}

void add_sample(EMDCsamples* ss, long long val, int dc_id, int rarr)
{
	EMDCsample* next = (EMDCsample*)malloc(sizeof(EMDCsample));
	next->val = val;
	next->dc_id = dc_id;
	next->rarr = rarr;
	next->next = NULL;
	if (ss->head == NULL)
	{
		ss->head = ss->tail = next;
	}
	else
	{
		ss->tail->next = next;
		ss->tail = next;
	}
}

void add_samples(EMDCmsg* m, EMDCsamples* s)
{
	if (m->head == NULL)
	{
		m->head = m->tail = s;
	}
	else
	{
		m->tail->next = s;
		m->tail = s;
	}
}

void free_message(EMDCmsg* m)
{
	EMDCsamples *curr_samples = m->head;
	while (curr_samples != NULL)
	{
		free_samples (curr_samples);
		EMDCsamples* next = curr_samples->next;
                free (curr_samples);
                curr_samples = next;
	}
}

