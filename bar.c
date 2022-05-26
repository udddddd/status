#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>

#define MODULE_NAME "main"

#include "common.h"

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static sig_atomic_t refresh = 0;
void sigusr1(int signum) {
	refresh = 1;
}

sig_atomic_t quit = 0;
void shutdown(int signum) {
	quit = 1;
}

typedef struct Module {
	void (*func)(char *output, void *arg);
	void *arg;
	char output[MODULE_BUF_SIZE];
} Module;

void *pthread_runner(void *arg) {
	Module *m = arg;
	m->func(m->output, m->arg);
	return NULL;
}

/* Modules */
#include "modules/xkb.h"
#include "modules/date.h"
#include "modules/battery.h"
#include "modules/backlight.h"
#include "modules/alsa.h"
#include "modules/cmus.h"
#include "modules/wpa.h"
#include "modules/disk.h"

#include "config.h"

char status[LEN(modules) * (MODULE_BUF_SIZE + LEN(separator) - 1)] = {0};

static int usepipe = 0;

void parse_argv(int argc, char **argv) {
	int opt;
	while((opt = getopt(argc, argv, "p")) != -1) {
		switch(opt) {
			case 'p':
				usepipe = 1;
				break;
			default:
				fprintf(stderr, "Usage: %s [-p]\n", argv[0]);
				exit(1);
		}
	}
}

int main(int argc, char **argv) {
	Display *dpy;
	parse_argv(argc, argv);
	if(!usepipe && !(dpy = XOpenDisplay(NULL))) {
		ERR("can't open display");
		return 1;
	}

	pthread_attr_t pthread_attr;
	pthread_t threads[LEN(modules)];

	sigaction(SIGTERM, &(struct sigaction){ .sa_handler = shutdown }, NULL);
	sigaction(SIGINT,  &(struct sigaction){ .sa_handler = shutdown }, NULL);

	sigset_t sigset;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGUSR1);
	sigaddset(&sigset, SIGUSR2);
	pthread_sigmask(SIG_BLOCK, &sigset, NULL);

	LOG("%lu modules", LEN(modules));

	pthread_attr_init(&pthread_attr);
	for(unsigned int i = 0; i < LEN(modules); ++i)
		pthread_create(threads + i, &pthread_attr, pthread_runner, modules + i);
	pthread_attr_destroy(&pthread_attr);

	signal(SIGUSR1, SIG_IGN);
	sigdelset(&sigset, SIGUSR2);
	pthread_sigmask(SIG_UNBLOCK, &sigset, NULL);

	usleep(10000);
	sigaction(SIGUSR1, &(struct sigaction){ .sa_handler = sigusr1 }, NULL);

	LOG("ready");

	for(;;) {
		if(quit)
			break;
		unsigned int i;
		char *p = status;
		pthread_mutex_lock(&lock);
		for(i = 0; i < LEN(modules) - 1; ++i) {
			if(*modules[i].output)
				p += sprintf(p, "%s%s", modules[i].output, separator);
		}
		p += sprintf(p, "%s", modules[i].output);
		pthread_mutex_unlock(&lock);
		LOG("refresh");
		if(!usepipe) {
			XStoreName(dpy, DefaultRootWindow(dpy), status);
			XSync(dpy, False);
		} else {
			*p = '\n';
			write(STDOUT_FILENO, status, p - status + 1);
		}
		pause();
	}
	LOG("terminated by signal");
	for(unsigned int i = 0; i < LEN(modules); ++i)
		pthread_kill(threads[i], SIGINT);
	for(unsigned int i = 0; i < LEN(modules); ++i) {
		pthread_join(threads[i], NULL);
		LOG("%d joined", i);
	}
	if(!usepipe)
		XCloseDisplay(dpy);
	return 0;
}
