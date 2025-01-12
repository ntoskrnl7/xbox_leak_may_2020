/* The Plum Hall Validation Suite for C
 * Unpublished copyright (c) 1986-1991, Chiron Systems Inc and Plum Hall Inc.
 * VERSION: 3
 * DATE:    91/07/01
 * The "ANSI" mode of the Suite corresponds to the official ANSI C, X3.159-1989.
 * As per your license agreement, your distribution is not to be moved or copied outside the Designated Site
 * without specific permission from Plum Hall Inc.
 */

#include <windows.h>
#include "types.h"

int PASCAL WinMain(HINSTANCE hinst, HINSTANCE hinstPrev, LPSTR lszCmdLine, int nCmdShow)

	{
	typedef union {
		CHAR c;
		SHORT s;
		INT i;
		UCHAR uc;
		USHORT us;
		UINT ui;
		LONG l;
		ULONG ul;
		FLOAT f;
		DOUBLE d;
		SCHAR sc;
		LDOUBLE ld;
		} D;
	auto D D1;
	auto D D2;
	auto D D3;
	auto D D4;
	auto D D5;
	auto D D6;
	auto D D7;
	auto D D8;
	auto D D9;
	auto D D10;
	auto D D11;
	auto D D12;
	D1.c = 7;
	D2.sc = 8;
	D3.s = 9;
	D4.i = 10;
	D5.uc = 11;
	D6.us = 12;
	D7.ui = 13;
	D8.l = 14;
	D9.ul = 15;
	D10.f = 16;
	D11.d = 17;
	D12.ld = 18;
	{
	typedef union {
		CHAR c;
		SHORT s;
		INT i;
		UCHAR uc;
		USHORT us;
		UINT ui;
		LONG l;
		ULONG ul;
		FLOAT f;
		DOUBLE d;
		SCHAR sc;
		LDOUBLE ld;
		} E;
	auto E E1, *pE1 = &E1;
	auto E E2, *pE2 = &E2;
	auto E E3, *pE3 = &E3;
	auto E E4, *pE4 = &E4;
	auto E E5, *pE5 = &E5;
	auto E E6, *pE6 = &E6;
	auto E E7, *pE7 = &E7;
	auto E E8, *pE8 = &E8;
	auto E E9, *pE9 = &E9;
	auto E E10, *pE10 = &E10;
	auto E E11, *pE11 = &E11;
	auto E E12, *pE12 = &E12;
	pE1->c = 7;
	pE2->sc = 8;
	pE3->s = 9;
	pE4->i = 10;
	pE5->uc = 11;
	pE6->us = 12;
	pE7->ui = 13;
	pE8->l = 14;
	pE9->ul = 15;
	pE10->f = 16;
	pE11->d = 17;
	pE12->ld = 18;
	{
	pE1->c = 14;
	D1.c = 7;

	break_here:

	D1.c = 14;
	pE1->c = 7;
	pE2->sc = 14;
	D1.c = 7;
	D1.c = 16;
	pE2->sc = 8;
	pE3->s = 14;
	D1.c = 7;
	D1.c = 18;
	pE3->s = 9;
	pE4->i = 14;
	D1.c = 7;
	D1.c = 20;
	pE4->i = 10;
	pE5->uc = 14;
	D1.c = 7;
	D1.c = 22;
	pE5->uc = 11;
	pE6->us = 14;
	D1.c = 7;
	D1.c = 24;
	pE6->us = 12;
	pE7->ui = 14;
	D1.c = 7;
	D1.c = 26;
	pE7->ui = 13;
	pE8->l = 14;
	D1.c = 7;
	D1.c = 28;
	pE8->l = 14;
	pE9->ul = 14;
	D1.c = 7;
	D1.c = 30;
	pE9->ul = 15;
	pE10->f = 14;
	D1.c = 7;
	D1.c = 32;
	pE10->f = 16;
	pE11->d = 14;
	D1.c = 7;
	D1.c = 34;
	pE11->d = 17;
	pE12->ld = 14;
	D1.c = 7;
	D1.c = 36;
	pE12->ld = 18;
	pE1->c = 16;
	D2.sc = 8;
	D2.sc = 14;
	pE1->c = 7;
	pE2->sc = 16;
	D2.sc = 8;
	D2.sc = 16;
	pE2->sc = 8;
	pE3->s = 16;
	D2.sc = 8;
	D2.sc = 18;
	pE3->s = 9;
	pE4->i = 16;
	D2.sc = 8;
	D2.sc = 20;
	pE4->i = 10;
	pE5->uc = 16;
	D2.sc = 8;
	D2.sc = 22;
	pE5->uc = 11;
	pE6->us = 16;
	D2.sc = 8;
	D2.sc = 24;
	pE6->us = 12;
	pE7->ui = 16;
	D2.sc = 8;
	D2.sc = 26;
	pE7->ui = 13;
	pE8->l = 16;
	D2.sc = 8;
	D2.sc = 28;
	pE8->l = 14;
	pE9->ul = 16;
	D2.sc = 8;
	D2.sc = 30;
	pE9->ul = 15;
	pE10->f = 16;
	D2.sc = 8;
	D2.sc = 32;
	pE10->f = 16;
	pE11->d = 16;
	D2.sc = 8;
	D2.sc = 34;
	pE11->d = 17;
	pE12->ld = 16;
	D2.sc = 8;
	D2.sc = 36;
	pE12->ld = 18;
	pE1->c = 18;
	D3.s = 9;
	D3.s = 14;
	pE1->c = 7;
	pE2->sc = 18;
	D3.s = 9;
	D3.s = 16;
	pE2->sc = 8;
	pE3->s = 18;
	D3.s = 9;
	D3.s = 18;
	pE3->s = 9;
	pE4->i = 18;
	D3.s = 9;
	D3.s = 20;
	pE4->i = 10;
	pE5->uc = 18;
	D3.s = 9;
	D3.s = 22;
	pE5->uc = 11;
	pE6->us = 18;
	D3.s = 9;
	D3.s = 24;
	pE6->us = 12;
	pE7->ui = 18;
	D3.s = 9;
	D3.s = 26;
	pE7->ui = 13;
	pE8->l = 18;
	D3.s = 9;
	D3.s = 28;
	pE8->l = 14;
	pE9->ul = 18;
	D3.s = 9;
	D3.s = 30;
	pE9->ul = 15;
	pE10->f = 18;
	D3.s = 9;
	D3.s = 32;
	pE10->f = 16;
	pE11->d = 18;
	D3.s = 9;
	D3.s = 34;
	pE11->d = 17;
	pE12->ld = 18;
	D3.s = 9;
	D3.s = 36;
	pE12->ld = 18;
	pE1->c = 20;
	D4.i = 10;
	D4.i = 14;
	pE1->c = 7;
	pE2->sc = 20;
	D4.i = 10;
	D4.i = 16;
	pE2->sc = 8;
	pE3->s = 20;
	D4.i = 10;
	D4.i = 18;
	pE3->s = 9;
	pE4->i = 20;
	D4.i = 10;
	D4.i = 20;
	pE4->i = 10;
	pE5->uc = 20;
	D4.i = 10;
	D4.i = 22;
	pE5->uc = 11;
	pE6->us = 20;
	D4.i = 10;
	D4.i = 24;
	pE6->us = 12;
	pE7->ui = 20;
	D4.i = 10;
	D4.i = 26;
	pE7->ui = 13;
	pE8->l = 20;
	D4.i = 10;
	D4.i = 28;
	pE8->l = 14;
	pE9->ul = 20;
	D4.i = 10;
	D4.i = 30;
	pE9->ul = 15;
	pE10->f = 20;
	D4.i = 10;
	D4.i = 32;
	pE10->f = 16;
	pE11->d = 20;
	D4.i = 10;
	D4.i = 34;
	pE11->d = 17;
	pE12->ld = 20;
	D4.i = 10;
	D4.i = 36;
	pE12->ld = 18;
	pE1->c = 22;
	D5.uc = 11;
	D5.uc = 14;
	pE1->c = 7;
	pE2->sc = 22;
	D5.uc = 11;
	D5.uc = 16;
	pE2->sc = 8;
	pE3->s = 22;
	D5.uc = 11;
	D5.uc = 18;
	pE3->s = 9;
	pE4->i = 22;
	D5.uc = 11;
	D5.uc = 20;
	pE4->i = 10;
	pE5->uc = 22;
	D5.uc = 11;
	D5.uc = 22;
	pE5->uc = 11;
	pE6->us = 22;
	D5.uc = 11;
	D5.uc = 24;
	pE6->us = 12;
	pE7->ui = 22;
	D5.uc = 11;
	D5.uc = 26;
	pE7->ui = 13;
	pE8->l = 22;
	D5.uc = 11;
	D5.uc = 28;
	pE8->l = 14;
	pE9->ul = 22;
	D5.uc = 11;
	D5.uc = 30;
	pE9->ul = 15;
	pE10->f = 22;
	D5.uc = 11;
	D5.uc = 32;
	pE10->f = 16;
	pE11->d = 22;
	D5.uc = 11;
	D5.uc = 34;
	pE11->d = 17;
	pE12->ld = 22;
	D5.uc = 11;
	D5.uc = 36;
	pE12->ld = 18;
	pE1->c = 24;
	D6.us = 12;
	D6.us = 14;
	pE1->c = 7;
	pE2->sc = 24;
	D6.us = 12;
	D6.us = 16;
	pE2->sc = 8;
	pE3->s = 24;
	D6.us = 12;
	D6.us = 18;
	pE3->s = 9;
	pE4->i = 24;
	D6.us = 12;
	D6.us = 20;
	pE4->i = 10;
	pE5->uc = 24;
	D6.us = 12;
	D6.us = 22;
	pE5->uc = 11;
	pE6->us = 24;
	D6.us = 12;
	D6.us = 24;
	pE6->us = 12;
	pE7->ui = 24;
	D6.us = 12;
	D6.us = 26;
	pE7->ui = 13;
	pE8->l = 24;
	D6.us = 12;
	D6.us = 28;
	pE8->l = 14;
	pE9->ul = 24;
	D6.us = 12;
	D6.us = 30;
	pE9->ul = 15;
	pE10->f = 24;
	D6.us = 12;
	D6.us = 32;
	pE10->f = 16;
	pE11->d = 24;
	D6.us = 12;
	D6.us = 34;
	pE11->d = 17;
	pE12->ld = 24;
	D6.us = 12;
	D6.us = 36;
	pE12->ld = 18;
	pE1->c = 26;
	D7.ui = 13;
	D7.ui = 14;
	pE1->c = 7;
	pE2->sc = 26;
	D7.ui = 13;
	D7.ui = 16;
	pE2->sc = 8;
	pE3->s = 26;
	D7.ui = 13;
	D7.ui = 18;
	pE3->s = 9;
	pE4->i = 26;
	D7.ui = 13;
	D7.ui = 20;
	pE4->i = 10;
	pE5->uc = 26;
	D7.ui = 13;
	D7.ui = 22;
	pE5->uc = 11;
	pE6->us = 26;
	D7.ui = 13;
	D7.ui = 24;
	pE6->us = 12;
	pE7->ui = 26;
	D7.ui = 13;
	D7.ui = 26;
	pE7->ui = 13;
	pE8->l = 26;
	D7.ui = 13;
	D7.ui = 28;
	pE8->l = 14;
	pE9->ul = 26;
	D7.ui = 13;
	D7.ui = 30;
	pE9->ul = 15;
	pE10->f = 26;
	D7.ui = 13;
	D7.ui = 32;
	pE10->f = 16;
	pE11->d = 26;
	D7.ui = 13;
	D7.ui = 34;
	pE11->d = 17;
	pE12->ld = 26;
	D7.ui = 13;
	D7.ui = 36;
	pE12->ld = 18;
	pE1->c = 28;
	D8.l = 14;
	D8.l = 14;
	pE1->c = 7;
	pE2->sc = 28;
	D8.l = 14;
	D8.l = 16;
	pE2->sc = 8;
	pE3->s = 28;
	D8.l = 14;
	D8.l = 18;
	pE3->s = 9;
	pE4->i = 28;
	D8.l = 14;
	D8.l = 20;
	pE4->i = 10;
	pE5->uc = 28;
	D8.l = 14;
	D8.l = 22;
	pE5->uc = 11;
	pE6->us = 28;
	D8.l = 14;
	D8.l = 24;
	pE6->us = 12;
	pE7->ui = 28;
	D8.l = 14;
	D8.l = 26;
	pE7->ui = 13;
	pE8->l = 28;
	D8.l = 14;
	D8.l = 28;
	pE8->l = 14;
	pE9->ul = 28;
	D8.l = 14;
	D8.l = 30;
	pE9->ul = 15;
	pE10->f = 28;
	D8.l = 14;
	D8.l = 32;
	pE10->f = 16;
	pE11->d = 28;
	D8.l = 14;
	D8.l = 34;
	pE11->d = 17;
	pE12->ld = 28;
	D8.l = 14;
	D8.l = 36;
	pE12->ld = 18;
	pE1->c = 30;
	D9.ul = 15;
	D9.ul = 14;
	pE1->c = 7;
	pE2->sc = 30;
	D9.ul = 15;
	D9.ul = 16;
	pE2->sc = 8;
	pE3->s = 30;
	D9.ul = 15;
	D9.ul = 18;
	pE3->s = 9;
	pE4->i = 30;
	D9.ul = 15;
	D9.ul = 20;
	pE4->i = 10;
	pE5->uc = 30;
	D9.ul = 15;
	D9.ul = 22;
	pE5->uc = 11;
	pE6->us = 30;
	D9.ul = 15;
	D9.ul = 24;
	pE6->us = 12;
	pE7->ui = 30;
	D9.ul = 15;
	D9.ul = 26;
	pE7->ui = 13;
	pE8->l = 30;
	D9.ul = 15;
	D9.ul = 28;
	pE8->l = 14;
	pE9->ul = 30;
	D9.ul = 15;
	D9.ul = 30;
	pE9->ul = 15;
	pE10->f = 30;
	D9.ul = 15;
	D9.ul = 32;
	pE10->f = 16;
	pE11->d = 30;
	D9.ul = 15;
	D9.ul = 34;
	pE11->d = 17;
	pE12->ld = 30;
	D9.ul = 15;
	D9.ul = 36;
	pE12->ld = 18;
	pE1->c = 32;
	D10.f = 16;
	D10.f = 14;
	pE1->c = 7;
	pE2->sc = 32;
	D10.f = 16;
	D10.f = 16;
	pE2->sc = 8;
	pE3->s = 32;
	D10.f = 16;
	D10.f = 18;
	pE3->s = 9;
	pE4->i = 32;
	D10.f = 16;
	D10.f = 20;
	pE4->i = 10;
	pE5->uc = 32;
	D10.f = 16;
	D10.f = 22;
	pE5->uc = 11;
	pE6->us = 32;
	D10.f = 16;
	D10.f = 24;
	pE6->us = 12;
	pE7->ui = 32;
	D10.f = 16;
	D10.f = 26;
	pE7->ui = 13;
	pE8->l = 32;
	D10.f = 16;
	D10.f = 28;
	pE8->l = 14;
	pE9->ul = 32;
	D10.f = 16;
	D10.f = 30;
	pE9->ul = 15;
	pE10->f = 32;
	D10.f = 16;
	D10.f = 32;
	pE10->f = 16;
	pE11->d = 32;
	D10.f = 16;
	D10.f = 34;
	pE11->d = 17;
	pE12->ld = 32;
	D10.f = 16;
	D10.f = 36;
	pE12->ld = 18;
	pE1->c = 34;
	D11.d = 17;
	D11.d = 14;
	pE1->c = 7;
	pE2->sc = 34;
	D11.d = 17;
	D11.d = 16;
	pE2->sc = 8;
	pE3->s = 34;
	D11.d = 17;
	D11.d = 18;
	pE3->s = 9;
	pE4->i = 34;
	D11.d = 17;
	D11.d = 20;
	pE4->i = 10;
	pE5->uc = 34;
	D11.d = 17;
	D11.d = 22;
	pE5->uc = 11;
	pE6->us = 34;
	D11.d = 17;
	D11.d = 24;
	pE6->us = 12;
	pE7->ui = 34;
	D11.d = 17;
	D11.d = 26;
	pE7->ui = 13;
	pE8->l = 34;
	D11.d = 17;
	D11.d = 28;
	pE8->l = 14;
	pE9->ul = 34;
	D11.d = 17;
	D11.d = 30;
	pE9->ul = 15;
	pE10->f = 34;
	D11.d = 17;
	D11.d = 32;
	pE10->f = 16;
	pE11->d = 34;
	D11.d = 17;
	D11.d = 34;
	pE11->d = 17;
	pE12->ld = 34;
	D11.d = 17;
	D11.d = 36;
	pE12->ld = 18;
	pE1->c = 36;
	D12.ld = 18;
	D12.ld = 14;
	pE1->c = 7;
	pE2->sc = 36;
	D12.ld = 18;
	D12.ld = 16;
	pE2->sc = 8;
	pE3->s = 36;
	D12.ld = 18;
	D12.ld = 18;
	pE3->s = 9;
	pE4->i = 36;
	D12.ld = 18;
	D12.ld = 20;
	pE4->i = 10;
	pE5->uc = 36;
	D12.ld = 18;
	D12.ld = 22;
	pE5->uc = 11;
	pE6->us = 36;
	D12.ld = 18;
	D12.ld = 24;
	pE6->us = 12;
	pE7->ui = 36;
	D12.ld = 18;
	D12.ld = 26;
	pE7->ui = 13;
	pE8->l = 36;
	D12.ld = 18;
	D12.ld = 28;
	pE8->l = 14;
	pE9->ul = 36;
	D12.ld = 18;
	D12.ld = 30;
	pE9->ul = 15;
	pE10->f = 36;
	D12.ld = 18;
	D12.ld = 32;
	pE10->f = 16;
	pE11->d = 36;
	D12.ld = 18;
	D12.ld = 34;
	pE11->d = 17;
	pE12->ld = 36;
	D12.ld = 18;
	D12.ld = 36;
	pE12->ld = 18;
	return 0;
	}}
	}
