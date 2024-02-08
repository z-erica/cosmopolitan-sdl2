#include <stdio.h>
#include <stdlib.h>

#include "../uxn.h"
#include "console.h"

/*
Copyright (c) 2022-2023 Devine Lu Linvega, Andrew Alderwick

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE.
*/

int
console_input(Uxn *u, char c, int type)
{
	Uint8 *d = &u->dev[0x10];
	d[0x2] = c;
	d[0x7] = type;
	return uxn_eval(u, PEEK2(d));
}

void
console_deo(Uint8 *d, Uint8 port)
{
	switch(port) {
	case 0x8:
		fputc(d[port], stdout);
		fflush(stdout);
		return;
	case 0x9:
		fputc(d[port], stderr);
		fflush(stderr);
		return;
	}
}