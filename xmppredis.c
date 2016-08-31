/*
(с) 2016 Copyright by vaclav2016, https://github.com/vaclav2016/xmppredis/

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

#define SKIP_SPACE(a) while(*a == ' ' && *a!=0) { a++;}
#define DEFAUL_STR_SIZE 1024

struct ThreadArgs {
	xmpp_ctx_t *ctx;
	xmpp_conn_t *conn;
};

const char *PublishCmd = "PUBLISH %s %s";
const char *SubscribeCmdTemplate = "SUBSCRIBE %s";
const char *Prefix = "jid:";
const int PrefixLen = 4;

char subscribeCmd[DEFAUL_STR_SIZE];

struct Config {
	char redisHost[DEFAUL_STR_SIZE];
	uint32_t redisPort;
	struct timeval redisTimeout;
	char jid[DEFAUL_STR_SIZE];
	char pwd[DEFAUL_STR_SIZE];
	char inboundQueue[DEFAUL_STR_SIZE];
	char outboundQueue[DEFAUL_STR_SIZE];
} conf;

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

	replytext = (char *) malloc(strlen(from) + strlen(intext) + 2 + PrefixLen);
	strcpy(replytext, Prefix);
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

	rc = redisConnectWithTimeout(conf.redisHost, conf.redisPort, conf.redisTimeout);

	if (rc == NULL || rc->err) {
		if (rc) {
			printf("Connection error: %s\n", rc->errstr);
			redisFree(rc);
		} else {
			printf("Connection error: can't allocate redis context\n");
		}
	} else {
		int bufLen = strlen(PublishCmd) + strlen(replytext) + strlen(conf.inboundQueue);
		char *buf = malloc(bufLen);
		snprintf(buf, bufLen, PublishCmd, conf.inboundQueue, replytext);
// printf("%s\n", buf);
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
	} else if (status == XMPP_CONN_DISCONNECT || status == XMPP_CONN_FAIL){
		xmpp_connect_client(conn, NULL, 0, conn_handler, ctx);
//		fprintf(stderr, "DEBUG: disconnected\n");
//		xmpp_stop(ctx);
	}
}


redisContext *connectRedis(redisContext *rc) {
	if (rc == NULL || rc->err) {
		printf("Redis Connection error\n");
		if (rc) {
			redisFree(rc);
		}
		rc = redisConnectWithTimeout(conf.redisHost, conf.redisPort, conf.redisTimeout);
	}
	return rc;
}

void sendMessage(xmpp_ctx_t *ctx, xmpp_conn_t *conn, char *messageSource) {
	char *toJid = messageSource;
	char *messageBody = strchr(messageSource, '\n');

	if( messageBody == NULL ) {
		return;
	}
	*messageBody = 0;
	messageBody++;

	if(strchr(toJid, '@') != NULL && strchr(toJid, '.') != NULL && strncmp(Prefix, toJid, PrefixLen) == 0 ) {
		toJid += PrefixLen;
	} else {
		return;
	}

	SKIP_SPACE(messageBody);
	SKIP_SPACE(toJid);

	xmpp_stanza_t *newMsg = xmpp_message_new(ctx, "chat", toJid, NULL /* id */);
	xmpp_stanza_t *body = xmpp_stanza_new(ctx);
	xmpp_stanza_t *text = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(body, "body");
	xmpp_stanza_add_child(newMsg, body);
	xmpp_stanza_add_child(body, text);
	xmpp_stanza_set_text(text, messageBody);
	xmpp_stanza_release(body);
	xmpp_stanza_release(text);

	xmpp_send(conn, newMsg);
	xmpp_stanza_release(newMsg);
}

void *redisClient(void *args) {

	xmpp_ctx_t *ctx = ((struct ThreadArgs *)args)->ctx;
	xmpp_conn_t *conn = ((struct ThreadArgs *)args)->conn;

	redisContext *rSubscCtx = connectRedis(NULL);

	int j;

	char *messagBuffer = malloc(MESSAGE_BUF_SIZE);

	while(1) {
		rSubscCtx = connectRedis(rSubscCtx);
		redisReply *reply = redisCommand(rSubscCtx, subscribeCmd);
		freeReplyObject(reply);
		while(redisGetReply(rSubscCtx, (void**)&reply) == REDIS_OK) {
			if (reply->type == REDIS_REPLY_ARRAY) {
				for (j = 0; j < reply->elements; j++) {
					if(reply->element[j]->type == REDIS_REPLY_STRING) {
						strncpy(messagBuffer, reply->element[j]->str, MESSAGE_BUF_SIZE);
						sendMessage(ctx, conn, messagBuffer);
					}
				}
			}
			freeReplyObject(reply);
		}
	}
	redisFree(rSubscCtx);
	free(messagBuffer);
	xmpp_conn_release(conn);
	return NULL;
}

void readConfig(char *fname, char *botSectionName) {
	char redisSectionName[256];
	ini_t *config = ini_load(fname);

	if (config == NULL) {
		error(1, errno, "ini_load fail");
	}

	ini_read_strn(config, botSectionName, "redis", redisSectionName, sizeof(redisSectionName), NULL);

	ini_read_strn(config, botSectionName, "jid", conf.jid, DEFAUL_STR_SIZE, NULL);
	ini_read_strn(config, botSectionName, "password", conf.pwd, DEFAUL_STR_SIZE, NULL);

	ini_read_strn(config, redisSectionName, "host", conf.redisHost, DEFAUL_STR_SIZE, NULL);
	ini_read_uint32(config, redisSectionName, "port", &conf.redisPort, 0);

	ini_read_strn(config, botSectionName, "inbound", conf.inboundQueue, DEFAUL_STR_SIZE, NULL);
	ini_read_strn(config, botSectionName, "outbound", conf.outboundQueue, DEFAUL_STR_SIZE, NULL);

	ini_free(config);

}

void validateConfig() {
	if(strlen(conf.jid)==0) {
		error(1, errno, "Invalid config. Empty xmpp/jid");
	}
	if(strlen(conf.pwd)==0) {
		error(1, errno, "Invalid config. Empty xmpp/pass");
	}
	if(strlen(conf.redisHost)==0) {
		error(1, errno, "Invalid config. Empty redis/host");
	}
	if(conf.redisPort == 0) {
		error(1, errno, "Invalid config. Empty redis/port");
	}
	if(strlen(conf.inboundQueue)==0) {
		error(1, errno, "Invalid config. Empty queue/inbound");
	}
	if(strlen(conf.outboundQueue)==0) {
		error(1, errno, "Invalid config. Empty queue/outbound");
	}
}

int main(int argc, char **argv) {
	xmpp_ctx_t *ctx;
	xmpp_conn_t *conn;
	xmpp_conn_t *connClone;
	xmpp_log_t *log;
	pthread_t thread;

	struct ThreadArgs args;
	conf.redisTimeout.tv_sec = 10;
	conf.redisTimeout.tv_usec = 500000;

	if(argc != 3) {
		error(1, errno, "Usage:\nxmppredis section config_file\n");
	}

	readConfig(argv[2], argv[1]);
	validateConfig();

	snprintf(subscribeCmd, sizeof(subscribeCmd), SubscribeCmdTemplate, conf.outboundQueue);

	xmpp_initialize();

	log = xmpp_get_default_logger(XMPP_LEVEL_DEBUG); /* pass NULL instead to silence output */
	ctx = xmpp_ctx_new(NULL, log);

	conn = xmpp_conn_new(ctx);
//	xmpp_conn_set_keepalive(conn, KA_TIMEOUT, KA_INTERVAL);

	xmpp_conn_set_jid(conn, conf.jid);
	xmpp_conn_set_pass(conn, conf.pwd);

	xmpp_connect_client(conn, NULL, 0, conn_handler, ctx);

	args.ctx = ctx;
	args.conn = xmpp_conn_clone(conn);

	pthread_create(&thread, NULL, redisClient, &args);
	xmpp_run(ctx);

	xmpp_conn_release(conn);
	xmpp_ctx_free(ctx);

	xmpp_shutdown();

	return 0;
}
