#include "SDL_cosmo.h"
#include "stb_vorbis.inc"
#include <assert.h>

#define USE_AUDIO_CALLBACK 0

static SDL_Event event;
static int quit;

static stb_vorbis *audio;
static stb_vorbis_info info;
static stb_vorbis_comment comment;

static SDL_AudioSpec inspec;
static SDL_AudioDeviceID outdev;

#if USE_AUDIO_CALLBACK
static int done_playing;

static void
audio_callback(void *userdata, Uint8 *stream, int len)
{
	int written_samples, fwritten;
	float *fstream = (float*)stream;
	int flen = len / sizeof(float);

	if (done_playing) {
		memset(stream, 0, len);
		return;
	}

	written_samples = stb_vorbis_get_samples_float_interleaved(
			audio,
			inspec.channels, fstream, flen);
	fwritten = written_samples * inspec.channels;
	if (fwritten < flen) {
		memset(fstream+fwritten, 0, (flen-fwritten)*sizeof(float));
		done_playing = 1;
		stb_vorbis_close(audio);
	}
}
#else
static void
queue_entire_file(void)
{
	float fbuffer[1024];
	int len = sizeof(fbuffer);
	int written_samples, fwritten;
	int flen = len / sizeof(float);

	do {
		written_samples = stb_vorbis_get_samples_float_interleaved(
				audio,
				inspec.channels, fbuffer, flen);
		fwritten = written_samples * inspec.channels;
		if(0 != SDL_QueueAudio(outdev, fbuffer, fwritten*sizeof(float))) {
			fprintf(stderr, "could not queue audio: %s\n", SDL_GetError());
			abort();
		}
	} while (fwritten == flen);
}
#endif

int main(int argc, char **argv) {
	int rc, i;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <ogg file>\n", argv[0]);
		return 1;
	}

	audio = stb_vorbis_open_filename(argv[1], NULL, NULL);
	if (audio == NULL) {
		fprintf(stderr, "could not open ogg file for decoding\n");
		return 2;
	}
	info = stb_vorbis_get_info(audio);
	comment = stb_vorbis_get_comment(audio);
	printf("vendor: %s\n", comment.vendor);
	for (i=0; i<comment.comment_list_length; i+=1) {
		printf("comment #%d: %s\n", i+1, comment.comment_list[i]);
	}

  rc = SDL_CosmoInit();
	if (rc != 0) {
		fprintf(stderr, "could not initialize cosmopolitan sdl: %s\n", SDL_CosmoGetError());
		return 100;
	}

	rc = SDL_Init(SDL_INIT_AUDIO);
	if (rc != 0) {
		fprintf(stderr, "could not initialize sdl: %s\n", SDL_GetError());
		return 3;
	}
	printf("audio driver: %s\n", SDL_GetCurrentAudioDriver());

	inspec.freq = info.sample_rate;
	inspec.format = AUDIO_F32;
	inspec.channels = info.channels;
	inspec.samples = 4096;
#if USE_AUDIO_CALLBACK
	inspec.callback = audio_callback;
	done_playing = 0;
#else
	inspec.callback = NULL;
#endif
	outdev = SDL_OpenAudioDevice(NULL, 0, &inspec, NULL, 0);
	if (outdev == 0) {
		fprintf(stderr, "could not open audio device: %s\n", SDL_GetError());
		return 4;
	}

#if !USE_AUDIO_CALLBACK
	queue_entire_file();
#endif

	SDL_PauseAudioDevice(outdev, 0);
	quit = 0;
	while (!quit) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) quit = 1;
		}

#if USE_AUDIO_CALLBACK
		SDL_LockAudio();
		if (done_playing) quit = 1;
		SDL_UnlockAudio();
#else
		if (SDL_GetQueuedAudioSize(outdev) == 0) quit = 1;
#endif
	}

	SDL_CloseAudioDevice(outdev);
	SDL_Quit();
}
