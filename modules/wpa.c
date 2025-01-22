#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/un.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>

#define MODULE_NAME "wpa"

#include "../common.h"
#include "wpa.h"

#define SSID_MAX_LENGTH 32 // according to IEEE
#define IPV4_MAX_LENGTH 15

/* FILE + socket's path for unlinking */
typedef struct Stream {
	FILE *fp;
	char path[sizeof(((struct sockaddr_un*)0)->sun_path)];
} Stream;

typedef struct WpaCtx {
	char ssid[SSID_MAX_LENGTH + 1];
	char ipaddr[IPV4_MAX_LENGTH + 1];
	WpaState state;
} WpaCtx;

static int open_stream(Stream *s, const char *path) {
	int sock = socket(PF_UNIX, SOCK_DGRAM, 0);
	struct sockaddr_un addr;

	if(!sock)
		return -1;
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, s->path);
	if(bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1)
		goto error;
	strncpy(addr.sun_path, path, sizeof(addr.sun_path));
	if(connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1)
		goto error;
	if(!(s->fp = fdopen(sock, "w+")))
		goto error;
	fcntl(sock, F_SETFL, fcntl(sock, F_GETFL) | O_NONBLOCK);
	return 0;
error:
	close(sock);
	return -1;
}

static int close_stream(Stream *s) {
	fclose(s->fp);
	unlink(s->path);
	return 0;
}

static int wpa_connect(Stream *s, const char *ifpath) {
	static int n = 0;

	snprintf(s->path, sizeof(s->path), "/tmp/wpa_ctrl_%d-%d", getpid(), ++n);
	return open_stream(s, ifpath);
}

static void print(char *output, const char *fmt, WpaCtx *ctx) {
	fmt_opt fmtopts[] = {
		{ 'i', FmtTypeString, ctx->ipaddr },
		{ 's', FmtTypeString, ctx->ssid   },
	};
	msnfmt(output, fmt, fmtopts, LEN(fmtopts));
}

static int update(WpaCtx *ctx, Stream *comm) {
	struct pollfd pfd = { .fd = fileno(comm->fp), .events = POLLIN };
	bool have_ssid = false, have_ip = false;
	char line[512];
	char *value, *t;

	LOG("update started");
	if(fputs("STATUS", comm->fp) == EOF || fflush(comm->fp) == EOF) {
		if(errno == ECONNREFUSED)
			return -1;
	}
	if(poll(&pfd, 1, 500) == 0)
		LOG("timeout");
	ctx->state = WPA_DISCONNECTED;
	while(fgets(line, sizeof(line), comm->fp)) {
		LOG("parsing line");
		strtok_r(line, "=", &value);
		t = strchr(value, '\n');
		if(t) *t = '\0';
		if (!strcmp(line, "wpa_state")) {
			ctx->state = !strcmp(value, "COMPLETED") ? WPA_CONNECTED
			                                         : WPA_DISCONNECTED;
		}
		if(!strcmp(line, "ip_address")) {
			strcpy(ctx->ipaddr, value);
			LOG("got ip %s", ctx->ipaddr);
			have_ip = true;
		}
		if (!strcmp(line, "ssid")) {
			strcpy(ctx->ssid, value);
			LOG("got ssid %s", ctx->ssid);
			have_ssid = true;
		}
	}
	LOG("update ended");
	return ctx->state != WPA_CONNECTED || (2 - have_ssid - have_ip);
}

static int attach(FILE *sock) {
	char buf[10];

	fputs("ATTACH", sock);
	fflush(sock);
	fcntl(fileno(sock), F_SETFL, fcntl(fileno(sock), F_GETFL) & ~O_NONBLOCK);
	fgets(buf, sizeof buf, sock);
	fcntl(fileno(sock), F_SETFL, fcntl(fileno(sock), F_GETFL) | O_NONBLOCK);
	return strncmp(buf, "OK\n", 3);
}

static int runsession(char *output, WpaOptions *opts, Stream *comm, Stream *notif) {
	struct pollfd pfd = { .fd = -1, .events = POLLIN };
	int timeout = -1;
	char buf[BUFSIZ];
	WpaCtx ctx;
	int ret = 0;

	pfd.fd = fileno(notif->fp);
	do {
		/* during connection some properties are not available, so keep
		 * querying status in small intervals until we get everything */
		ret = update(&ctx, comm);
		if(ret < 0) {
			ret = 0;
			break;
		} else {
			timeout = ret ? 500 : -1;
		}
		print(output, opts->format[ctx.state], &ctx);
		/* a dummy loop which flushes all events */
		while((ret = read(pfd.fd, buf, sizeof(buf))) > 0);
		if(ret == -1) {
			LOG("dummy loop returned errno %d:", errno);
			LOG("\t%s", strerror(errno));
		}
	} while((ret = poll(&pfd, 1, timeout)) != -1);
	return ret;
}

void wpa(char *output, void *arg) {
	WpaOptions * const opts = arg;
	Stream comm, notif;
	int ret;

	LOG("startup");
	for(;;) {
		if(wpa_connect(&comm, opts->ifpath) || wpa_connect(&notif, opts->ifpath)
		|| attach(notif.fp)) {
			ERR("failed to connect");
			unlink(comm.path);
			unlink(notif.path);
			sleep(5);
			continue;
		}
		ret = runsession(output, opts, &comm, &notif);
		LOG("session ended with code %d", ret);
		close_stream(&comm);
		close_stream(&notif);
		if(ret < 0)
			break;
	}
	LOG("shutdown");
	return;
}
