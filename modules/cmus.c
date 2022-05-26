#define _GNU_SOURCE

#include <string.h>
#include <fcntl.h>

#define MODULE_NAME "cmus"

#include "../common.h"
#include "cmus.h"

#define FIELD_LEN 64

typedef struct CmusCtx {
	CmusStatus status;
	char title[FIELD_LEN];
	char artist[FIELD_LEN];
	char album[FIELD_LEN];
} CmusCtx;

static void dummy(int signo) {}

static CmusStatus parse_status(const char *status) {
	if(strcmp(status, "stopped") == 0) {
		return CMUS_STOPPED;
	} else if(strcmp(status, "paused") == 0) {
		return CMUS_PAUSED;
	} else if(strcmp(status, "playing") == 0) {
		return CMUS_PLAYING;
	}
	return CMUS_STOPPED;
}
 
static void update_ctx(CmusCtx *ctx) {
	FILE *fp = popen("cmus-remote -Q 2> /dev/null", "r");
	if(!fp) {
		ctx->status = CMUS_STOPPED;
		return;
	}
	char *buf = NULL;
	size_t buflen = 0;
	ssize_t nread;
	char *saveptr = NULL;
	while((nread = getline(&buf, &buflen, fp)) != -1) {
		*strchrnul(buf, '\n') = '\0';
		char *tok = strtok_r(buf, " ", &saveptr);
		if(!tok)
			continue;
		if(strcmp(tok, "status") == 0) {
			ctx->status = parse_status(saveptr); 
		} else if(strcmp(tok, "tag") == 0) {
			tok = strtok_r(NULL, " ", &saveptr);
			if(strcmp(tok, "artist") == 0)
				strncpy(ctx->artist, saveptr, FIELD_LEN);
			else if(strcmp(tok, "album") == 0)
				strncpy(ctx->album, saveptr, FIELD_LEN);
			else if(strcmp(tok, "title") == 0)
				strncpy(ctx->title, saveptr, FIELD_LEN);
		}
	}
	free(buf);
	if(pclose(fp))
		ctx->status = CMUS_STOPPED;
}

static void print(char *output, const char *fmt, const CmusCtx *ctx) {
	fmt_opt fmtopts[] = {
		{ 'A', FmtTypeString, ctx->album  },
		{ 'a', FmtTypeString, ctx->artist },
		{ 't', FmtTypeString, ctx->title  },
	};
	msnfmt(output, fmt, fmtopts, LEN(fmtopts));
}

void cmus(char *output, void *arg) {
	LOG("startup");
	CmusOptions *opts = arg;
	CmusCtx ctx;
	sigaction(SIGUSR2, &(struct sigaction){ .sa_handler = dummy }, NULL);
	sigset_t sigset;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGUSR2);
	pthread_sigmask(SIG_UNBLOCK, &sigset, NULL);
	for(;;) {
		update_ctx(&ctx);
		print(output, opts->format[ctx.status], &ctx);
		pause();
		if(quit)
			break;
	}
	LOG("shutdown");
}
