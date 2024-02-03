#ifndef SDL_cosmo_h_
#define SDL_cosmo_h_

#include "SDL.h"

#ifdef __cplusplus
extern "C" {
#endif

int SDL_CosmoInit(void);
const char *SDL_CosmoGetError(void);

#ifdef __cplusplus
}
#endif

#endif /* SDL_cosmo_h */
