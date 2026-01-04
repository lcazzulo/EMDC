#ifndef _AMQP__H_
#define _AMQP__H_

#include <rabbitmq-c/amqp.h>
#include <rabbitmq-c/tcp_socket.h>

typedef struct _AMQP_Ctx
{
	amqp_connection_state_t conn;
	amqp_socket_t *socket;
	int status;
} AMQP_Ctx;


int AMQP_Init(AMQP_Ctx* ctx, const char* hostname, int port, const char* user, const char* password);
int AMQP_Sendmessage(AMQP_Ctx* ctx, const char* exchange, const char* routingkey, const char* message);
int AMQP_Close(AMQP_Ctx*  ctx);
#endif
