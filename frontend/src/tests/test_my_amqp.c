#include <stdio.h>
#include <stdlib.h>
#include "../my_amqp.h"

int main (int argc, char* argv[])
{
	int ret;
	AMQP_Ctx *ctx = (AMQP_Ctx*)malloc(sizeof(AMQP_Ctx));
	ret = AMQP_Init(ctx, "altair.luca-cazzulo.me", 5672);
	if (ret != 0)
	{
		printf ("error in AMQP_Init\r\n");
		return ret;
	}
	while (1)
	{
		ret = AMQP_Sendmessage(ctx, "EMDC", "A.B.C", "messaggio");
		if (ret != 0)
        	{
                	printf ("error in AMQP_Sendmessage\r\n");
                	return ret;
        	}
		printf ("OK\r\n");
		usleep (100 * 1000);
	}
	getchar ();
	return 0;
}
