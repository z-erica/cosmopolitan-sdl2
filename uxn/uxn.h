/*
Copyright (c) 2021 Devine Lu Linvega

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE.
*/

/* clang-format off */

#define POKE2(d, v) { (d)[0] = (v) >> 8; (d)[1] = (v); }
#define PEEK2(d) ((d)[0] << 8 | (d)[1])
#define DEO(p, v) { u->dev[p] = v; if((deo_mask[p >> 4] >> (p & 0xf)) & 0x1) emu_deo(u, p); }
#define DEI(p) ((dei_mask[(p) >> 4] >> ((p) & 0xf)) & 0x1 ? emu_dei(u, (p)) : u->dev[(p)])

/* clang-format on */

#define PAGE_PROGRAM 0x0100

typedef unsigned char Uint8;
typedef signed char Sint8;
typedef unsigned short Uint16;
typedef signed short Sint16;
typedef unsigned int Uint32;

typedef struct {
	Uint8 dat[255], ptr;
} Stack;

typedef struct Uxn {
	Uint8 *ram, dev[256];
	Stack wst, rst;
	Uint8 (*dei)(struct Uxn *u, Uint8 addr);
	void (*deo)(struct Uxn *u, Uint8 addr);
} Uxn;

/* required functions */

extern Uint8 emu_dei(Uxn *u, Uint8 addr);
extern void emu_deo(Uxn *u, Uint8 addr);
extern int emu_halt(Uxn *u, Uint8 instr, Uint8 err, Uint16 addr);
extern Uint16 dev_vers[0x10], dei_mask[0x10], deo_mask[0x10];

/* built-ins */

int uxn_boot(Uxn *u, Uint8 *ram);
int uxn_eval(Uxn *u, Uint16 pc);
