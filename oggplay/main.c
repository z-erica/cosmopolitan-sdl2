#include <SDL.h>
#include "stb_vorbis.inc"

SDL_Event event;
int quit;

static int channels;
static int sample_rate;
static int sample_count;
static short *audio;

static SDL_AudioSpec inspec;
static SDL_AudioDeviceID outdev;

int main(int argc, char **argv) {
	int rc;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <ogg file>\n", argv[0]);
		return 1;
	}

	sample_count = stb_vorbis_decode_filename(argv[1], &channels, &sample_rate, &audio);
	if (sample_count < 0) {
		fprintf(stderr, "could not decode ogg file\n");
		return 2;
	}

	rc = SDL_Init(SDL_INIT_AUDIO);
	if (rc != 0) {
		fprintf(stderr, "could not initialize sdl: %s\n", SDL_GetError());
		return 3;
	}

	inspec.freq = sample_rate;
	inspec.format = AUDIO_S16; /* assumes sizeof(short) == 2 */
	inspec.channels = channels;
	inspec.samples = 4096;
	inspec.callback = NULL;
	outdev = SDL_OpenAudioDevice(NULL, 0, &inspec, NULL, 0);
	if (outdev == 0) {
		fprintf(stderr, "could not open audio device: %s\n", SDL_GetError());
		return 4;
	}

	rc = SDL_QueueAudio(outdev, audio, sample_count*channels*sizeof audio[0]);
	if (rc != 0) {
		fprintf(stderr, "could not queue audio: %s\n", SDL_GetError());
		return 5;
	}
	free(audio);

	SDL_PauseAudioDevice(outdev, 0);
	quit = 0;
	while (!quit) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) quit = 1;
		}

		if (SDL_GetQueuedAudioSize(outdev) == 0) quit = 1;
	}

	SDL_CloseAudioDevice(outdev);
	SDL_Quit();
}
