/*
Copyright (c) 2021 Devine Lu Linvega, Andrew Alderwick

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE.
*/

#define DATETIME_VERSION 1
#define DATETIME_DEIMASK 0x07ff
#define DATETIME_DEOMASK 0x0000

Uint8 datetime_dei(Uxn *u, Uint8 addr);
