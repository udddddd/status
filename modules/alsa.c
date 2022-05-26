#include <stdio.h>
#include <assert.h>
#include <alsa/asoundlib.h>
#include <math.h>

#define MODULE_NAME "alsa"

#include "../common.h"
#include "alsa.h"
#include "volume_mapping.h"

typedef struct AlsaCtx {
	snd_mixer_t *mixer;
	snd_hctl_elem_t *jack;
	snd_mixer_elem_t *headphone, *speaker;
} AlsaCtx;

static snd_hctl_elem_t *find_elem(snd_hctl_t *hctl, unsigned int iface, const char *name) {
	snd_ctl_elem_id_t *id;
	snd_ctl_elem_id_alloca(&id);
	snd_ctl_elem_id_set_interface(id, iface);
	snd_ctl_elem_id_set_name(id, name);
	snd_hctl_elem_t *elem = snd_hctl_find_elem(hctl, id);
	if(!elem)
		LOG("no such control '%s'\n", name);
	return elem;
}

static snd_mixer_elem_t *find_selem(snd_mixer_t *hctl, const char *name) {
	snd_mixer_selem_id_t *id;
	snd_mixer_selem_id_alloca(&id);
	snd_mixer_selem_id_set_name(id, name);
	snd_mixer_elem_t *elem = snd_mixer_find_selem(hctl, id);
	if(!elem)
		LOG("no such mixer element '%s'\n", name);
	return elem;
}

static int load_controls(AlsaOptions *opts, AlsaCtx *ctx) {
	snd_hctl_t *hctl = NULL;
	if(snd_mixer_get_hctl(ctx->mixer, opts->card, &hctl) < 0) {
		ERR("no hctl");
		return -1;
	}
	if(!(ctx->jack = find_elem(hctl, SND_CTL_ELEM_IFACE_CARD, opts->jack))
	|| !(ctx->headphone = find_selem(ctx->mixer, opts->headphone))
	|| !(ctx->speaker   = find_selem(ctx->mixer, opts->speaker)))
		return -1;
	return 0;
}

static int setup(AlsaOptions *opts, AlsaCtx *ctx) {
	if(snd_mixer_open(&ctx->mixer, 0))
		goto error;
	if(  snd_mixer_attach(ctx->mixer, opts->card)
	  || snd_mixer_selem_register(ctx->mixer, NULL, NULL)
	  || snd_mixer_load(ctx->mixer)
	  || load_controls(opts, ctx)
	  ) {
		snd_mixer_close(ctx->mixer);
		goto error;
	}
	return 0;
error:
	ERR("failed to open card '%s'", opts->card);
	return -1;
}

static int get_bool(snd_hctl_elem_t *elem) {
	snd_ctl_elem_value_t *val;
	snd_ctl_elem_value_alloca(&val);
	snd_hctl_elem_read(elem, val);
	return snd_ctl_elem_value_get_boolean(val, 0);
}

static int volumechange(snd_mixer_elem_t *elem, unsigned int mask) {
	double *vol = snd_mixer_elem_get_callback_private(elem);
	if(mask & SND_CTL_EVENT_MASK_VALUE)
		*vol = get_normalized_playback_volume(elem, 0);
	return 0;
}

static int jackplug(snd_hctl_elem_t *elem, unsigned int mask) {
	int *state = snd_hctl_elem_get_callback_private(elem);
	if(mask & SND_CTL_EVENT_MASK_VALUE)
		*state = !*state;
	return 0;
}

static int run(char *output, AlsaOptions *opts, AlsaCtx *ctx) {
	int plugged  = get_bool(ctx->jack);
	double hpvol = get_normalized_playback_volume(ctx->headphone, 0);
	double spvol = get_normalized_playback_volume(ctx->speaker, 0);

	snd_hctl_elem_set_callback(ctx->jack,       jackplug);
	snd_mixer_elem_set_callback(ctx->speaker,   volumechange);
	snd_mixer_elem_set_callback(ctx->headphone, volumechange);

	snd_hctl_elem_set_callback_private(ctx->jack,       &plugged);
	snd_mixer_elem_set_callback_private(ctx->speaker,   &spvol);
	snd_mixer_elem_set_callback_private(ctx->headphone, &hpvol);

	for(;;) {
		int vol = lrint((plugged ? hpvol : spvol) * 100);
		LOG("%lf %lf %d", hpvol, spvol, vol);
		fmt_opt fmtopts[] = { { 'p', FmtTypeInteger, &vol } };
		msnfmt(output, opts->format[plugged], fmtopts, LEN(fmtopts));
		if(snd_mixer_wait(ctx->mixer, -1))
			break;
		snd_mixer_handle_events(ctx->mixer);
	}
	return 0;
}

void alsa(char *output, void *arg) {
	LOG("startup");
	AlsaCtx ctx = {0};
	AlsaOptions *opts = arg;
	if(setup(opts, &ctx) == 0)
		run(output, opts, &ctx);
	LOG("shutdown");
}
