/* keyboard.c -- Gestione tastiera */
/******************************************************************************
* Blueport Operating System                                                   *
*                                                                             *
* This program is free software; you can redistribute it and/or               *
* modify it under the terms of the GNU General Public License                 *
* as published by the Free Software Foundation; either version 2              *
* of the License, or (at your option) any later version.                      *
*                                                                             *
* This program is distributed in the hope that it will be useful,             *
* but WITHOUT ANY WARRANTY; without even the implied warranty of              *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               *
* GNU General Public License for more details.                                *
*                                                                             *
* You should have received a copy of the GNU General Public License           *
* along with this program; if not, write to the Free Software                 *
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA. *
*******************************************************************************/

#include <io.h>
#include <kernel.h>
#include <dev/keymap.h>
#include <dev/keyboard.h>
#include <irq.h>
#include <asm/system.h>

#define LSHIFT		0x01
#define RSHIFT		0x02
#define LCTRL		0x04
#define RCTRL		0x08
#define ALT		0x10
#define ALTGR		0x20
#define CAPS		0x40
#define CAPSDOWN	0x80

#define SCRLED		0x01
#define NUMLED		0x02
#define CAPSLED		0x04

#define KBD_ACK		0xFA

typedef void (*fptr)(int);

unsigned char kmode   = 0;
unsigned char ke0     = 0;
unsigned char kleds   = NUMLED;
unsigned char kapplic = 0;

static fptr key_table[];

static unsigned char old_leds = NUMLED;
static int npadch = 0;

extern void kb_intr(void);

static void applkey(int key);

void keyboard_interrupt(void)
{
	unsigned char scancode, x;
	scancode=inb_p(0x60);
	x=inb_p(0x61);
	outb_p(x|0x80, 0x61);
	outb_p(x&0x7f, 0x61);
	outb(0x20, 0x20);
	if(scancode == 0xE0)
	{
		ke0 = 1;
	}
	else if(scancode == 0xE1)
	{
		ke0 = 2;
	}
	else
	{
        	key_table[scancode](scancode);
        	ke0 = 0;
	}
}

int init_keyboard(void)
{
	register unsigned char a;
	kprint("keyb: inizializzazione tastiera...");
	a=inb_p(0x61);
	outb_p(a|0x80, 0x61);
	outb(a, 0x61);
	add_irq_handler(1,&keyboard_interrupt);
	enable_irq(1);
	// disabilita/riabilita la tastiera
	kprint("\n");
#if defined KBD_IT
	kprint("keyb: installata keymap IT.\n");
	return 0;
#elif defined KBD_US
	kprint("installata keymap US.\n");
	return 0;
#else
	kprint("keymap non installata.\n");
	return 1;
#endif
}

/* attende che il buffer della tastiera si svuoti */
static void kb_wait(void) {
    int i;

    for (i=0; i<0x10000; i++) {
        if ((inb(0x64) & 0x02) == 0) {
            break;
		}
	}
}

void kb_ack(void) {
    int i;

    for(i=0; i<0x10000; i++) {
        if (inb(0x60) == KBD_ACK) {
            break;
        }
    }
}

static void none(int sc) {
}

static void do_self(int sc) {
	unsigned char ch;

    if (kmode & ALTGR) {
	    ch=alt_map[sc];
    } else if (kmode & (LSHIFT|RSHIFT|LCTRL|RCTRL)) {
        ch=shift_map[sc];
    } else {
        ch=key_map[sc];
    }

	if (ch == 0) {
		return;
	}

    // If CTRL is active the character CTRL-A == 0x01, CTRL-B == 0x02,
    // CTRL-Z == 0x1A.
    if (kmode & (LCTRL|RCTRL|CAPS)) {
        if ((ch>='a' && ch <='z') || (ch>=224 && ch<=254)) {
            ch -= 32;
        }
    }

    if (kmode & (LCTRL|RCTRL)) {
        ch &= 0x1f;
    }

    // If the character has been pressed in combination with
    // ALT key, the bit 7 is activated. For LATIN-1 map the character is
    // prepended with 0x33 value (now not handled).
    if (kmode & ALT) {
        ch |= 0x80;
    }

	kprint("%c",ch);
}

static void enter(int sc) {
    if (ke0 != 1)
        do_self(sc);
    else if (kapplic)
        applkey('M');
    else
        do_self(sc);
}

static void backspace(int sc) {
    if (ke0 != 1) kprint("\b");
}

static void ctrl(int sc) {
    if (ke0) {
        kmode|=RCTRL;
    } else {
        kmode|=LCTRL;
    }
}

static void unctrl(int sc) {
    if (ke0) {
        kmode&=(~RCTRL);
    } else {
        kmode&=(~LCTRL);
    }
}

static void lshift(int sc) {
	kmode|=LSHIFT;
}

static void unlshift(int sc) {
	kmode&=(~LSHIFT);
}

static void slash(int sc) {
    if (ke0 != 1) {
        do_self(sc);
    } else if (kapplic) {
        applkey('Q');
    } else {
        kprint("/");
    }
}

static void rshift(int sc) {
    kmode|=RSHIFT;
}

static void unrshift(int sc) {
    kmode&=(~RSHIFT);
}

static void star(int sc) {
    if (kapplic)
        applkey('R');
    else
        do_self(sc);
}

static void alt(int sc) {
    if (ke0) {
        kmode|=ALTGR;
    } else {
        kmode|=ALT;
    }
}

static void unalt(int sc) {
    if (ke0) {
        kmode&=(~ALTGR);
    } else {
        kmode&=(~ALT);

        // check if we press ALT-char code. For example if we press
		// ALT-126 the ~ character should be printed.
		if (npadch != 0) {
			kprint("%c",npadch);
		    npadch=0;
		}
    }
}

void set_leds(void) {
    if (kleds != old_leds) {
        old_leds=kleds;
        kb_wait();
        outb(0xED, 0x60);
        kb_ack();
        kb_wait();
        outb(kleds, 0x60);
        kb_ack();
    }
}

static void caps(int sc) {
    if (!(kmode&CAPSDOWN)) {
	    kleds^=CAPSLED;
        kmode^=CAPS;
        kmode|=CAPSDOWN;
        set_leds();
    }
}

static void uncaps(int sc) {
	kmode&=(~CAPSDOWN);
}

static void func(int sc) {
    if (sc < 0x3b) {
        return;
    }

    sc-=0x3b;
    if (sc > 9) {
        sc-=18;
        if (sc < 10 || sc > 11)
            return;
    }

    if (kmode & ALT) {
        //FIXME: switch_console(sc+1);
    } else {
    	kprint("%s",func_map[sc]);
    }
}

static void num(int sc) {
    if (kapplic)
        applkey(0x50);
    else {
        kleds^=NUMLED;
        set_leds();
    }
}

static void scroll(int sc) {
    kleds^=SCRLED;
    set_leds();
}

static void cursor(int sc) {
    unsigned char ch;

    if (sc < 0x47 || sc > 0x53) {
        return;
	}

	sc-=0x47;

	// if CTRL-ALT-DEL has been pressed, reboot the machine
    if (sc == 12 && (kmode&(LCTRL|RCTRL)) && (kmode&(ALT|ALTGR))) {
        kbd_reboot();
        return;
    }

    if (ke0 == 1) {
        //cur(sc);
        return;
    }

    // the user is pressing ALT-numpad.
    if ((kmode & ALT) && sc!=12) {
        npadch=npadch*10 + pad_map[sc];
        return;
    }

    if (kleds & NUMLED) {
        ch = num_map[sc];
	kprint("%c",ch);
    }
}

static void minus(int sc) {
    if (kapplic) {
        applkey('S');
    } else {
        do_self(sc);
    }
}

static void plus(int sc) {
    if (kapplic)
        applkey('l');
    else
        do_self(sc);
}

void kbd_reboot(void)
{
	sti();
	kb_wait();
	//*((unsigned short *)0x472)=0x1234;
	outb(0xfe,0x64);     /* invia il segnale di reset sulla linea della tastiera una sola volta */
	asm("hlt");
	while(1);
}

static void applkey(int key) {
	/* funzioni speciali di certi tasti */
    // not implemented here
}

static fptr key_table[] = {
    none,do_self,do_self,do_self,       /* 00-03 s0 esc 1 2 */
    do_self,do_self,do_self,do_self,    /* 04-07 3 4 5 6 */
    do_self,do_self,do_self,do_self,    /* 08-0B 7 8 9 0 */
    do_self,do_self,backspace,do_self,    /* 0C-0F + ' bs tab */
    do_self,do_self,do_self,do_self,    /* 10-13 q w e r */
    do_self,do_self,do_self,do_self,    /* 14-17 t y u i */
    do_self,do_self,do_self,do_self,    /* 18-1B o p } ^ */
    enter,ctrl,do_self,do_self,         /* 1C-1F enter ctrl a s */
    do_self,do_self,do_self,do_self,    /* 20-23 d f g h */
    do_self,do_self,do_self,do_self,    /* 24-27 j k l | */
    do_self,do_self,lshift,do_self,     /* 28-2B { para lshift , */
    do_self,do_self,do_self,do_self,    /* 2C-2F z x c v */
    do_self,do_self,do_self,do_self,    /* 30-33 b n m , */
    do_self,slash,rshift,star,          /* 34-37 . - rshift * */
    alt,do_self,caps,func,              /* 38-3B alt sp caps f1 */
    func,func,func,func,                /* 3C-3F f2 f3 f4 f5 */
    func,func,func,func,                /* 40-43 f6 f7 f8 f9 */
    func,num,scroll,cursor,             /* 44-47 f10 num scr home */
    cursor,cursor,minus,cursor,         /* 48-4B up pgup - left */
    cursor,cursor,plus,cursor,          /* 4C-4F n5 right + end */
    cursor,cursor,cursor,cursor,        /* 50-53 dn pgdn ins del */
    none,none,do_self,func,             /* 54-57 sysreq ? < f11 */
    func,none,none,none,                /* 58-5B f12 ? ? ? */
    none,none,none,none,                /* 5C-5F ? ? ? ? */
    none,none,none,none,                /* 60-63 ? ? ? ? */
    none,none,none,none,                /* 64-67 ? ? ? ? */
    none,none,none,none,                /* 68-6B ? ? ? ? */
    none,none,none,none,                /* 6C-6F ? ? ? ? */
    none,none,none,none,                /* 70-73 ? ? ? ? */
    none,none,none,none,                /* 74-77 ? ? ? ? */
    none,none,none,none,                /* 78-7B ? ? ? ? */
    none,none,none,none,                /* 7C-7F ? ? ? ? */
    none,none,none,none,                /* 80-83 ? br br br */
    none,none,none,none,                /* 84-87 br br br br */
    none,none,none,none,                /* 88-8B br br br br */
    none,none,none,none,                /* 8C-8F br br br br */
    none,none,none,none,                /* 90-93 br br br br */
    none,none,none,none,                /* 94-97 br br br br */
    none,none,none,none,                /* 98-9B br br br br */
    none,unctrl,none,none,              /* 9C-9F br unctrl br br */
    none,none,none,none,                /* A0-A3 br br br br */
    none,none,none,none,                /* A4-A7 br br br br */
    none,none,unlshift,none,            /* A8-AB br br unlshift br */
    none,none,none,none,                /* AC-AF br br br br */
    none,none,none,none,                /* B0-B3 br br br br */
    none,none,unrshift,none,         /* B4-B7 br br unrshift br */
    unalt,none,uncaps,none,             /* B8-BB unalt br uncaps br */
    none,none,none,none,                /* BC-BF br br br br */
    none,none,none,none,                /* C0-C3 br br br br */
    none,none,none,none,                /* C4-C7 br br br br */
    none,none,none,none,                /* C8-CB br br br br */
    none,none,none,none,                /* CC-CF br br br br */
    none,none,none,none,                /* D0-D3 br br br br */
    none,none,none,none,                /* D4-D7 br br br br */
    none,none,none,none,                /* D8-DB br ? ? ? */
    none,none,none,none,                /* DC-DF ? ? ? ? */
    none,none,none,none,                /* E0-E3 e0 e1 ? ? */
    none,none,none,none,                /* E4-E7 ? ? ? ? */
    none,none,none,none,                /* E8-EB ? ? ? ? */
    none,none,none,none,                /* EC-EF ? ? ? ? */
    none,none,none,none,                /* F0-F3 ? ? ? ? */
    none,none,none,none,                /* F4-F7 ? ? ? ? */
    none,none,none,none,                /* F8-FB ? ? ? ? */
    none,none,none,none                 /* FC-FF ? ? ? ? */
};
