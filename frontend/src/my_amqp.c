#include "my_amqp.h"
#include <stdio.h>
#include <rabbitmq-c/amqp.h>
#include <rabbitmq-c/tcp_socket.h>

int AMQP_Init (AMQP_Ctx* ctx, const char* hostname, int port, const char* user, const char* password)
{
	ctx->conn = amqp_new_connection();
	ctx->socket = amqp_tcp_socket_new(ctx->conn);
	if (!ctx->socket)
	{
		printf ("error in amqp_tcp_socket_new\r\n");
		return -1;
	}
	ctx->status = amqp_socket_open(ctx->socket, hostname, port);
	if(ctx->status)
	{
		printf ("error in amqp_socket_open\r\n");
		return -1;
	}
	if(amqp_login(ctx->conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, user, password).reply_type != AMQP_RESPONSE_NORMAL)
	{
		printf ("error in amqp_login\r\n");
                return -1;
	}
	amqp_channel_open(ctx->conn, 1);

	if(amqp_get_rpc_reply(ctx->conn).reply_type != AMQP_RESPONSE_NORMAL)
	{
		printf ("error in amqp_get_rpc_reply\r\n");
		return -1;
	}

	return 0;
}

int AMQP_Sendmessage(AMQP_Ctx* ctx, const char* exchange, const char* routingkey, const char* message)
{
	amqp_basic_properties_t props;
    	props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
    	props.content_type = amqp_cstring_bytes("text/plain");
    	props.delivery_mode = 2; /* persistent delivery mode */
        if (amqp_basic_publish(ctx->conn, 1, amqp_cstring_bytes(exchange),
                                    amqp_cstring_bytes(routingkey), 0, 0,
                                    &props, amqp_cstring_bytes(message)) < 0)
	{
		return -1;
	}
	return 0;
}

int AMQP_Close (AMQP_Ctx* ctx)
{
	amqp_channel_close(ctx->conn, 1, AMQP_REPLY_SUCCESS);
        amqp_connection_close(ctx->conn, AMQP_REPLY_SUCCESS);
        amqp_destroy_connection(ctx->conn);
}
