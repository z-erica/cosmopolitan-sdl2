/*
Copyright (c) 2022 Devine Lu Linvega, Andrew Alderwick

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE.
*/

#define SYSTEM_VERSION 1
#define SYSTEM_DEIMASK 0x0000
#define SYSTEM_DEOMASK 0xff28

#define RAM_PAGES 0x10

void system_connect(Uint8 device, Uint8 ver, Uint16 dei, Uint16 deo);
int system_version(char *emulator, char *date);
int system_load(Uxn *u, char *filename);
void system_inspect(Uxn *u);
int system_error(char *msg, const char *err);
void system_deo(Uxn *u, Uint8 *d, Uint8 port);
