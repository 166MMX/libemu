/* $Id$ */

#include <stdio.h>
#include "types.h"
#include "libemu.h"

void emu_flags_pf(uint16 value)
{
	uint8 ones = 0;
	value = value & 0xFF;
	while (value != 0) {
		ones++;
		value &= (value - 1);
	}
	emu_flags.pf = (ones & 1) ? 0 : 1;
}
void emu_flags_zf (uint16 value) { emu_flags.zf = (value == 0) ? 1 : 0; }
void emu_flags_sfb(uint8  value) { emu_flags.sf = value >> 7; }
void emu_flags_sfw(uint16 value) { emu_flags.sf = value >> 15; }

/* FLAGS */

void emu_clc()
{
	emu_flags.cf = 0;
}

void emu_stc()
{
	emu_flags.cf = 1;
}

void emu_cld()
{
	emu_flags.df = 0;
}

void emu_std()
{
	emu_flags.df = 1;
}

void emu_cli()
{
	emu_flags.inf = 0;
}

void emu_sti()
{
	emu_flags.inf = 1;
}

void emu_cmc()
{
	emu_flags.cf = (emu_flags.cf == 0) ? 1 : 0;
}

void emu_salc()
{
	emu_al = (emu_flags.cf == 0) ? 0 : 0xFF;
}

/* REGISTER */

void emu_push(uint16 v1)
{
	emu_sp -= 2;
	emu_get_memory16(emu_ss, emu_sp, 0) = v1;
}

void emu_pop(uint16 *v1)
{
	*v1 = emu_get_memory16(emu_ss, emu_sp, 0);
	emu_sp += 2;
}

void emu_pushf()
{
	emu_push(emu_flags_all);
}

void emu_popf()
{
	uint16 temp;
	emu_pop(&temp);
	emu_flags_all = temp;

#if EMULATE_386
	emu_flags.res4 = 0;
#elif EMULATE_286
	emu_flags.iopl = 0;
	emu_flags.nt   = 0;
	emu_flags.res4 = 0;
#else
	emu_flags.iopl = 3;
	emu_flags.nt   = 1;
	emu_flags.res4 = 1;
#endif
}

void emu_pusha()
{
	uint16 sp = emu_sp;
	emu_push(emu_ax);
	emu_push(emu_cx);
	emu_push(emu_dx);
	emu_push(emu_bx);
	emu_push(sp);
	emu_push(emu_bp);
	emu_push(emu_si);
	emu_push(emu_di);
}

void emu_popa()
{
	uint16 sp;
	emu_pop(&emu_di);
	emu_pop(&emu_si);
	emu_pop(&emu_bp);
	emu_pop(&sp); /* Throw away */
	emu_pop(&emu_bx);
	emu_pop(&emu_dx);
	emu_pop(&emu_cx);
	emu_pop(&emu_ax);
}

void emu_lahf()
{
	emu_ah = 0;
	emu_ah += (emu_flags.sf << 7);
	emu_ah += (emu_flags.zf << 6);
	emu_ah += (           0 << 5);
	emu_ah += (emu_flags.af << 4);
	emu_ah += (           0 << 3);
	emu_ah += (emu_flags.pf << 2);
	emu_ah += (           1 << 1);
	emu_ah += (emu_flags.cf << 0);
}

void emu_sahf()
{
	emu_flags.sf = (emu_ah & 0x80) >> 7;
	emu_flags.zf = (emu_ah & 0x40) >> 6;
	/*             (emu_ah & 0x20) >> 5; */
	emu_flags.af = (emu_ah & 0x10) >> 4;
	/*             (emu_ah & 0x08) >> 3; */
	emu_flags.pf = (emu_ah & 0x04) >> 2;
	/*             (emu_ah & 0x02) >> 1; */
	emu_flags.cf = (emu_ah & 0x01) >> 0;
}

void emu_arplw(uint16 *dst, uint16 src)
{
	if ((*dst & 0x3) < (src & 0x3)) {
		*dst = (*dst & 0xFFFC) | (src & 0x3);
		emu_flags.zf = 1;
	} else {
		emu_flags.zf = 0;
	}
}
