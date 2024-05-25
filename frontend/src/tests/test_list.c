#include <stdlib.h>
#include <stdio.h>
#include "../list.h"

int main(int argc, char* argv[])
{
	int j, i;
	
	EMDCsamples* curr_samples;
	EMDCsample* curr_sample;

	EMDCmsg* m = init_message (EMDCrequest);

	for (j=0; j<10; j++)
	{
		EMDCsamples *ss = init_samples (EMDCinsert);
	
		for (i=1; i<=5; i++)
		{
			add_sample(ss, (long long)(i+j), 0, 0);
		}

		add_samples (m, ss);
	}

	
	curr_samples = m->head;
	while (curr_samples != NULL)
	{
		curr_sample = curr_samples->head;
		while (curr_sample != NULL)
		{
			printf ("%lld\t%d\t%d\n", curr_sample->val, curr_sample->dc_id, curr_sample->rarr);
			curr_sample = curr_sample->next;
		}
		printf ("==========================================\n");
		curr_samples = curr_samples->next;
	}
	

	curr_samples = m->head;
	while (curr_samples != NULL)
	{
		free_samples (curr_samples);
		curr_samples = curr_samples->next;
	}
	free (m);

	return 0;
}
