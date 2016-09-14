/*

(c) 2016 Copyright Vaclav2016 https://github.com/vaclav2016, jabber id vaclav2016@jabber.cz

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
#include <unistd.h>
#include <hiredis/hiredis.h>
#include <ini/ini.h>

#define MESSAGE_BUF_SIZE 1024*64
#define MAX_INCOME_MESSAGE_SIZE 4*1024

#define SKIP_SPACE(a) while(*a == ' ' && *a!=0) { a++;}
#define DEFAUL_STR_SIZE 1024

const char *PublishCmd = "PUBLISH %s %s";
const char *SubscribeCmdTemplate = "SUBSCRIBE %s";
const char *Prefix = "jid:";
const int PrefixLen = 4;
char subscribeCmd[DEFAUL_STR_SIZE];

struct Config {
	char redisHost[DEFAUL_STR_SIZE];
	uint64_t redisPort;
	char online;
	struct timeval redisTimeout;
	char jid[DEFAUL_STR_SIZE];
	char pwd[DEFAUL_STR_SIZE];
	char inboundQueue[DEFAUL_STR_SIZE];
	char outboundQueue[DEFAUL_STR_SIZE];
} conf;


struct MessageQueue {
	char *body;
	struct MessageQueue *next;
};

pthread_mutex_t fromRedisQueueLock = PTHREAD_MUTEX_INITIALIZER;
struct MessageQueue *fromRedisQueue;

struct MessageQueue *sendToXmppQueue;
struct MessageQueue *sendToRedisQueue;

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

	if( strlen(intext) > MAX_INCOME_MESSAGE_SIZE) {
		reply = xmpp_stanza_reply(stanza);
		xmpp_message_set_body(reply, "[DROP] Your message is too big and has been droped.");
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
	printf("\033[35mReceive from \033[32m%s\033[37m\n", replytext);
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

	struct MessageQueue *nm = malloc(sizeof(struct MessageQueue));
	nm->body = malloc(strlen(replytext)+1);
	strcpy(nm->body, replytext);
	nm->next = sendToRedisQueue;
	sendToRedisQueue = nm;

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
		fprintf(stderr, "\033[32mConnected to JABBER-server\033[37m\n");
		xmpp_handler_add(conn,version_handler, "jabber:iq:version", "iq", NULL, ctx);
		xmpp_handler_add(conn,message_handler, NULL, "message", NULL, ctx);

		pres = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(pres, "presence");
		xmpp_send(conn, pres);
		xmpp_stanza_release(pres);
		conf.online = 1;
	} else if (status == XMPP_CONN_DISCONNECT || status == XMPP_CONN_FAIL){
		fprintf(stderr, "\033[31mDisconnect from JABBER-server\033[37m\n");
		conf.online = 0;
		xmpp_conn_release(conn);
	}
}

redisContext *connectRedis(redisContext *rc) {
	if (rc == NULL || rc->err) {
		fprintf(stderr, "\033[31mReconnect to REDIS-server\033[37m\n");
		if (rc) {
			redisFree(rc);
		}
		rc = redisConnectWithTimeout(conf.redisHost, conf.redisPort, conf.redisTimeout);
		if (rc != NULL && !rc->err) {
			fprintf(stderr, "\033[32mConnected to REDIS-server\033[37m\n");
		}
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
		printf("\033[36mSend to \033[32m%s\033[37m\n", toJid);
		toJid += PrefixLen;
	} else {
		return;
	}

	SKIP_SPACE(messageBody);
	SKIP_SPACE(toJid);

	xmpp_stanza_t *newMsg = xmpp_message_new(ctx, "chat", toJid, NULL /* id */);
	xmpp_message_set_body(newMsg, messageBody);
	xmpp_send(conn, newMsg);
	xmpp_stanza_release(newMsg);
}

void *redisClientThread(void *args) {
	redisContext *rSubscCtx = connectRedis(NULL);
	while(1) {
		rSubscCtx = connectRedis(rSubscCtx);
		redisReply *reply = redisCommand(rSubscCtx, subscribeCmd);
		freeReplyObject(reply);
		while(redisGetReply(rSubscCtx, (void**)&reply) == REDIS_OK) {
			if (reply->type == REDIS_REPLY_ARRAY) {
				pthread_mutex_lock(&fromRedisQueueLock);
				int i;
				for (i = 0; i < reply->elements; i++) {
					if(reply->element[i]->type == REDIS_REPLY_STRING) {
						struct MessageQueue *msg = malloc(sizeof(struct MessageQueue));
						msg->body = malloc(strlen(reply->element[i]->str) + 1);
						strcpy(msg->body, reply->element[i]->str);
						msg->next = fromRedisQueue;
						fromRedisQueue = msg;
					}
				}
				pthread_mutex_unlock(&fromRedisQueueLock);
			}
			freeReplyObject(reply);
		}
	}
	redisFree(rSubscCtx);
	return NULL;
}

void readConfig(char *fname, char *botSectionName) {
	char redisSectionName[256];
	void *config = ini_load(fname);

	if (config == NULL) {
		error(1, errno, "ini_load fail");
	}

	ini_getstr(config, botSectionName, "redis", redisSectionName, sizeof(redisSectionName));
	ini_getstr(config, botSectionName, "jid", conf.jid, DEFAUL_STR_SIZE);
	ini_getstr(config, botSectionName, "password", conf.pwd, DEFAUL_STR_SIZE);

	ini_getstr(config, redisSectionName, "host", conf.redisHost, DEFAUL_STR_SIZE);
	ini_getint(config, redisSectionName, "port", &conf.redisPort);

	ini_getstr(config, botSectionName, "inbound", conf.inboundQueue, DEFAUL_STR_SIZE);
	ini_getstr(config, botSectionName, "outbound", conf.outboundQueue, DEFAUL_STR_SIZE);

	ini_free(config);

}

void sendMessages_once(xmpp_ctx_t *ctx, xmpp_conn_t *conn) {
	struct MessageQueue *this;
	while ( (this = sendToXmppQueue) != NULL) {
		sendToXmppQueue = sendToXmppQueue->next;
		sendMessage(ctx, conn, this->body);
		free(this->body);
		free(this);
	}
}

void populateMessagesFromRedis() {
	struct MessageQueue *this;
	pthread_mutex_lock(&fromRedisQueueLock);
	while( (this = fromRedisQueue) != NULL ) {
		fromRedisQueue = fromRedisQueue->next;
		this->next = sendToXmppQueue;
		sendToXmppQueue = this;
	}
	pthread_mutex_unlock(&fromRedisQueueLock);
}

void populateMesageToRedis() {
	if( sendToRedisQueue == NULL ) {
		return;
	}
	redisContext *rc = redisConnectWithTimeout(conf.redisHost, conf.redisPort, conf.redisTimeout);
	if (rc != NULL && !rc->err) {
		struct MessageQueue *this;
		while( (this = sendToRedisQueue) != NULL) {
			sendToRedisQueue = sendToRedisQueue->next;
			int bufLen = strlen(PublishCmd) + strlen(this->body) + strlen(conf.inboundQueue);
			char *buf = malloc(bufLen);
			snprintf(buf, bufLen, PublishCmd, conf.inboundQueue, this->body);
			freeReplyObject(redisCommand(rc, buf));
			free(buf);
			free(this->body);
			free(this);
		}
		redisFree(rc);
	}
}

void validateConfig() {
	if(strlen(conf.jid)==0) {
		error(1, errno, "\033[31mInvalid config. Empty xmpp/jid\033[37m\n");
	}
	if(strlen(conf.pwd)==0) {
		error(1, errno, "\033[31mInvalid config. Empty xmpp/pass\033[37m\n");
	}
	if(strlen(conf.redisHost)==0) {
		error(1, errno, "\033[31mInvalid config. Empty redis/host\033[37m\n");
	}
	if(conf.redisPort == 0) {
		error(1, errno, "\033[31mInvalid config. Empty redis/port\033[37m\n");
	}
	if(strlen(conf.inboundQueue)==0) {
		error(1, errno, "\033[31mInvalid config. Empty queue/inbound\033[37m\n");
	}
	if(strlen(conf.outboundQueue)==0) {
		error(1, errno, "\033[31mInvalid config. Empty queue/outbound\033[37m\n");
	}
}

int main(int argc, char **argv) {
	xmpp_ctx_t *ctx;
	xmpp_conn_t *conn;
	xmpp_conn_t *connClone;
	xmpp_log_t *log;
	pthread_t thread;

	conf.redisTimeout.tv_sec = 10;
	conf.redisTimeout.tv_usec = 500000;
	fromRedisQueue = NULL;
	sendToXmppQueue = NULL;
	sendToRedisQueue = NULL;

	printf("\033[34m\r");
	printf("                                                                 d8b   d8,        \n");
	printf("                                                                 88P  `8P         \n");
	printf("                                                                d88               \n");
	printf("?88,  88P  88bd8b,d88b ?88,.d88b,?88,.d88b,  88bd88b d8888b d888888    88b .d888b,\n");
	printf(" `?8bd8P'  88P'`?8P'?8b`?88'  ?88`?88'  ?88  88P'  `d8b_,dPd8P' ?88    88P ?8b,   \n");
	printf(" d8P?8b,  d88  d88  88P  88b  d8P  88b  d8P d88     88b    88b  ,88b  d88    `?8b \n");
	printf("d8P' `?8bd88' d88'  88b  888888P'  888888P'd88'     `?888P'`?88P'`88bd88' `?888P' \n");
	printf("                         88P'      88P'                                           \n");
	printf("                        d88       d88                                             \n");
	printf("                        ?8P       ?8P                                             ");
	printf("\033[37m\n");
	printf("(с) 2016 Copyright by \033[36mvaclav2016\033[37m, https://github.com/vaclav2016/xmppredis/\n");
	printf("\033[31mBoost License, Version 1.0, http://www.boost.org/LICENSE_1_0.txt\033[37m\n");
	printf("\033[31m(c) Haipo Yang, MIT License, https://github.com/haipome/ini\033[37m\n");

	if(argc != 3) {
		error(1, errno, "\033[31mUsage: xmppredis section config_file\033[37m\n");
	}

	readConfig(argv[2], argv[1]);
	validateConfig();
	conf.online = 0;
	snprintf(subscribeCmd, sizeof(subscribeCmd), SubscribeCmdTemplate, conf.outboundQueue);

	xmpp_initialize();

//	log = xmpp_get_default_logger(XMPP_LEVEL_DEBUG); /* pass NULL instead to silence output */
	log = xmpp_get_default_logger((xmpp_log_level_t)NULL);
//	ctx = xmpp_ctx_new(NULL, log);
	ctx = xmpp_ctx_new(NULL, NULL);


	pthread_create(&thread, NULL, redisClientThread, NULL);
	while(1) {
		if(conf.online) {
			xmpp_run_once(ctx, 10*1000);
			populateMesageToRedis();
			populateMessagesFromRedis();
			sendMessages_once(ctx, conn);
		} else {
			sleep(30);
			conn = xmpp_conn_new(ctx);
			xmpp_conn_set_keepalive(conn, 60000, 30000);

			xmpp_conn_set_jid(conn, conf.jid);
			xmpp_conn_set_pass(conn, conf.pwd);

			xmpp_connect_client(conn, NULL, 0, conn_handler, ctx);
		}
	}

	xmpp_conn_release(conn);
	xmpp_ctx_free(ctx);

	xmpp_shutdown();
	pthread_mutex_destroy(&fromRedisQueueLock);

	return 0;
}
