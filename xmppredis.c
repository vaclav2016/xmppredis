/*
Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strophe.h>
#include <error.h>
#include <errno.h>
#include <pthread.h>
#include <hiredis/hiredis.h>
#include "ini.h"

#define MESSAGE_BUF_SIZE 1024*64

typedef struct _threadArgs {
	xmpp_ctx_t *ctx;
	xmpp_conn_t *conn;
} threadArgs;

const char *insert_cmd = "PUBLISH %s %s";
const char *get_cmd_tmpl = "LPOP %s";
const char *subscribe_cmd_tmpl = "SUBSCRIBE %s";

char get_cmd[1024];
char subscribe_cmd[1024];
char redisHost[1024];
uint32_t redisPort;

struct timeval timeout = { 10, 500000 };

char redisQueueInbound[1024];
char redisQueueOutbound[1024];
char *msgBuffer;
char toBuffer[1024];


int version_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza, void * const userdata) {
	xmpp_stanza_t *reply, *query, *name, *version, *text;
	const char *ns;
	xmpp_ctx_t *ctx = (xmpp_ctx_t*)userdata;
	printf("Received version request from %s\n", xmpp_stanza_get_from(stanza));

	reply = xmpp_stanza_reply(stanza);
	xmpp_stanza_set_type(reply, "result");

	query = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(query, "query");
	ns = xmpp_stanza_get_ns(xmpp_stanza_get_children(stanza));
	if (ns) {
		xmpp_stanza_set_ns(query, ns);
	}

	name = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(name, "name");
	xmpp_stanza_add_child(query, name);
	
	text = xmpp_stanza_new(ctx);
	xmpp_stanza_set_text(text, "anonymous bot");
	xmpp_stanza_add_child(name, text);
	
	version = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(version, "version");
	xmpp_stanza_add_child(query, version);
	
	text = xmpp_stanza_new(ctx);
	xmpp_stanza_set_text(text, "1.0");
	xmpp_stanza_add_child(version, text);
	
	xmpp_stanza_add_child(reply, query);

	xmpp_send(conn, reply);
	xmpp_stanza_release(reply);
	return 1;
}


int message_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza, void * const userdata) {
	xmpp_stanza_t *reply, *body, *text;
	char *intext, *replytext;
	xmpp_ctx_t *ctx = (xmpp_ctx_t*)userdata;

	if(!xmpp_stanza_get_child_by_name(stanza, "body")) {
		return 1;
	}
	if(xmpp_stanza_get_type(stanza) !=NULL && !strcmp(xmpp_stanza_get_type(stanza), "error")) {
		return 1;
	}

	intext = xmpp_stanza_get_text(xmpp_stanza_get_child_by_name(stanza, "body"));
	const char *from = xmpp_stanza_get_from(stanza);

	printf("Incoming message from %s: %s\n", from, intext);

	if( strlen(intext) > 4*1024) {
		body = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(body, "body");
		reply = xmpp_stanza_reply(stanza);
		if (xmpp_stanza_get_type(reply) == NULL) {
			xmpp_stanza_set_type(reply, "chat");
		}
		text = xmpp_stanza_new(ctx);
		xmpp_stanza_set_text(text, "[DROP] Your message is too big and has been droped.");
		xmpp_stanza_add_child(body, text);
		xmpp_stanza_add_child(reply, body);
		xmpp_stanza_release(body);
		xmpp_stanza_release(text);

		xmpp_send(conn, reply);
		xmpp_stanza_release(reply);
		free(replytext);
		return 1;
	}

	replytext = (char *) malloc(strlen(from) + strlen(intext) + 2 + 4);
	strcpy(replytext, "jid:");
	strcat(replytext, from);
	char *stop = strchr(replytext, '/');
	if( stop != NULL ) {
		*stop = 0;
	}
	strcat(replytext, "\n");
	strcat(replytext, intext);
	xmpp_free(ctx, intext);
	char *t = replytext;

	while(*t) {
		if(*t=='"') {
			*t = ' ';
		}
		t++;
	}

	redisContext *rc;
	redisReply *rreply;

	rc = redisConnectWithTimeout(redisHost, redisPort, timeout);

	if (rc == NULL || rc->err) {
		if (rc) {
			printf("Connection error: %s\n", rc->errstr);
			redisFree(rc);
		} else {
			printf("Connection error: can't allocate redis context\n");
		}
	} else {
		int bufLen = strlen(insert_cmd) + strlen(replytext) + strlen(redisQueueInbound);
		char *buf = malloc(bufLen);
		snprintf(buf, bufLen, insert_cmd, redisQueueInbound, replytext);
printf("%s\n", buf);
		rreply = redisCommand(rc, buf);
		freeReplyObject(rreply);

		redisFree(rc);
		free(buf);
	}

	free(replytext);
	return 1;
}

/* define a handler for connection events */
void conn_handler(xmpp_conn_t * const conn, const xmpp_conn_event_t status, 
		  const int error, xmpp_stream_error_t * const stream_error,
		  void * const userdata) {
	xmpp_ctx_t *ctx = (xmpp_ctx_t *)userdata;

	if (status == XMPP_CONN_CONNECT) {
		xmpp_stanza_t* pres;
		fprintf(stderr, "DEBUG: connected\n");
		xmpp_handler_add(conn,version_handler, "jabber:iq:version", "iq", NULL, ctx);
		xmpp_handler_add(conn,message_handler, NULL, "message", NULL, ctx);

		pres = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(pres, "presence");
		xmpp_send(conn, pres);
		xmpp_stanza_release(pres);
	} else {
		fprintf(stderr, "DEBUG: disconnected\n");
		xmpp_stop(ctx);
	}
}


redisContext *connectRedis(redisContext *rc) {
	if (rc == NULL || rc->err) {
		printf("Redis Connection error\n");
		if (rc) {
			redisFree(rc);
		}
		rc = redisConnectWithTimeout(redisHost, redisPort, timeout);
	}
	return rc;
}

void sendMessage(xmpp_ctx_t *ctx, xmpp_conn_t *conn, char *msg) {
	char *stop = strchr(msg, '\n');
	if( stop == NULL ) {
		return;
	}
	*stop = 0;
	stop++;

	if(strchr(msg, '@') == NULL || strchr(msg, '.') == NULL || strncmp("jid:", msg, 4) != 0 ) {
		return;
	} else {
		msg += 4;
	}

	while(*msg == ' ') {
		msg++;
	}

	strncpy(msgBuffer, stop, MESSAGE_BUF_SIZE);
	strncpy(toBuffer, msg, sizeof(toBuffer));
	xmpp_stanza_t *newMsg = xmpp_message_new(ctx, "chat", toBuffer, NULL /* id */);
	xmpp_stanza_t *body = xmpp_stanza_new(ctx);
	xmpp_stanza_t *text = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(body, "body");
	xmpp_stanza_add_child(newMsg, body);
	xmpp_stanza_add_child(body, text);
	xmpp_stanza_set_text(text, msgBuffer);
	xmpp_stanza_release(body);
	xmpp_stanza_release(text);

	xmpp_send(conn, newMsg);
	xmpp_stanza_release(newMsg);
}

void *redisClient(void *args) {
	xmpp_ctx_t *ctx = ((struct _threadArgs *)args)->ctx;
	xmpp_conn_t *conn = ((struct _threadArgs *)args)->conn;

	redisContext *rSubscCtx = connectRedis(NULL);
	redisContext *rCmdCtx = connectRedis(NULL);

	msgBuffer = malloc(MESSAGE_BUF_SIZE);
	while(1) {
		rSubscCtx = connectRedis(rSubscCtx);
		redisReply *reply = redisCommand(rSubscCtx, subscribe_cmd);
		freeReplyObject(reply);
		while(redisGetReply(rSubscCtx, (void**)&reply) == REDIS_OK) {
			if (reply->type == REDIS_REPLY_ARRAY) {
				int j;
				for (j = 0; j < reply->elements; j++) {
					if(reply->element[j]->type == REDIS_REPLY_STRING) {
						sendMessage(ctx, conn, reply->element[j]->str);
					}
				}
			}
			freeReplyObject(reply);
		}
		free(msgBuffer);
	}
	redisFree(rSubscCtx);
	free(msgBuffer);
	xmpp_conn_release(conn);
	return NULL;
}

int main(int argc, char **argv) {
	xmpp_ctx_t *ctx;
	xmpp_conn_t *conn;
	xmpp_conn_t *connClone;
	xmpp_log_t *log;
	pthread_t thread;
	threadArgs args;
	char redis[256];
	char jid[100] = { 0 };
	char pass[100] = { 0 };

	if(argc != 3) {
		error(1, errno, "Usage:\nxmppredis section config_file\n");
	}

	ini_t *conf = ini_load(argv[2]);
	if (conf == NULL) {
		error(1, errno, "ini_load fail");
	}

	ini_read_strn(conf, argv[1], "redis", redis, sizeof(redis), NULL);

	ini_read_strn(conf, argv[1], "jid", jid, sizeof(jid), NULL);
	ini_read_strn(conf, argv[1], "password", pass, sizeof(pass), NULL);

	ini_read_strn(conf, redis, "host", redisHost, sizeof(redisHost), NULL);
	ini_read_uint32(conf, redis, "port", &redisPort, 0);

	ini_read_strn(conf, argv[1], "inbound", redisQueueInbound, sizeof(redisQueueInbound), NULL);
	ini_read_strn(conf, argv[1], "outbound", redisQueueOutbound, sizeof(redisQueueOutbound), NULL);

	ini_free(conf);

	if(strlen(jid)==0) {
		error(1, errno, "Invalid config. Empty xmpp/jid");
	}
	if(strlen(pass)==0) {
		error(1, errno, "Invalid config. Empty xmpp/pass");
	}
	if(strlen(redisHost)==0) {
		error(1, errno, "Invalid config. Empty redis/host");
	}
	if(redisPort == 0) {
		error(1, errno, "Invalid config. Empty redis/port");
	}
	if(strlen(redisQueueInbound)==0) {
		error(1, errno, "Invalid config. Empty queue/inbound");
	}
	if(strlen(redisQueueOutbound)==0) {
		error(1, errno, "Invalid config. Empty queue/outbound");
	}

	snprintf(get_cmd, sizeof(get_cmd), get_cmd_tmpl, redisQueueOutbound);
	snprintf(subscribe_cmd, sizeof(subscribe_cmd), subscribe_cmd_tmpl, redisQueueOutbound);

	/* init library */
	xmpp_initialize();

	/* create a context */
	log = xmpp_get_default_logger(XMPP_LEVEL_DEBUG); /* pass NULL instead to silence output */
	ctx = xmpp_ctx_new(NULL, log);

	/* create a connection */
	conn = xmpp_conn_new(ctx);
//	xmpp_conn_set_keepalive(conn, KA_TIMEOUT, KA_INTERVAL);

	/*
	 * also you can disable TLS support or force legacy SSL
	 * connection without STARTTLS
	 *
	 * see xmpp_conn_set_flags() or examples/basic.c
	 */

	/* setup authentication information */
	xmpp_conn_set_jid(conn, jid);
	xmpp_conn_set_pass(conn, pass);

	/* initiate connection */
	xmpp_connect_client(conn, NULL, 0, conn_handler, ctx);

	/* enter the event loop - 
	our connect handler will trigger an exit */

	args.ctx = ctx;
	args.conn = xmpp_conn_clone(conn);

	pthread_create(&thread, NULL, redisClient, &args);
	xmpp_run(ctx);

	/* release our connection and context */
	xmpp_conn_release(conn);
	xmpp_ctx_free(ctx);

	/* final shutdown of the library */
	xmpp_shutdown();

	return 0;
}
