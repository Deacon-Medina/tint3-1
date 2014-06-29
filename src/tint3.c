#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include "draw.h"
#include "utils.h"
#include "config.h"
#include "defaults.h"

#define INRECT(x,y,rx,ry,rw,rh) ((x) >= (rx) && (x) < (rx)+(rw) && (y) >= (ry) && (y) < (ry)+(rh))
#define MIN(a,b)                ((a) < (b) ? (a) : (b))
#define MAX(a,b)                ((a) > (b) ? (a) : (b))
#define DEFFONT "fixed"


static void drawmenu(void);
static void run(void);
static void setup(void);

static int bh, mw, mh;
static int height = 0;
static int itemcount = 0;
static int lines = 0;
static const char *font = FONT;
static ColorSet *normcol;

static Atom utf8;
static Bool topbar = True;
static DC *dc;
static Window root, win;










int main(int argc, char *argv[]) {

    dc = initdc();
    initfont(dc, font ? font : DEFFONT);
    normcol = initcolor(dc, BAR_BACKGROUND, BAR_FOREGROUND);
    setup();
    run();

    return EXIT_FAILURE;
}

/* SPACE FOR MODULE FUNCTIONS */

baritem * battery_s(DC * dc) {
    batt_info * info = get_battery_information();
    char * inf_as_str = malloc(20);
    snprintf(inf_as_str, 20, "%i %s", info -> percentage, info -> icon);
    baritem * result = malloc(sizeof(baritem));
    result -> string = inf_as_str;
    result -> type = 'B';
    if (info -> percentage > BATTERY_CUTOF_HIGH) {
        result -> color = initcolor(dc, BATTERY_FOREGROUND_HIGH, BATTERY_BACKGROUND_HIGH);
    } else if (info -> percentage < BATTERY_CUTOF_LOW) {
        result -> color = initcolor(dc, BATTERY_FOREGROUND_LOW, BATTERY_BACKGROUND_LOW);
    } else {
        result -> color = initcolor(dc, BATTERY_FOREGROUND_MED, BATTERY_BACKGROUND_MED);
    }
    free(info);
    return result;
}


baritem * wmname_s(DC * dc) {
    FILE * desc = popen("wmname", "r");
    char * msg = malloc(20);
    int msg_c = 0; char msg_s;
    if (desc) {
        while( (msg_s = fgetc(desc)) != '\n') {
            msg[msg_c++] = msg_s;
        }
        if (msg_c < 20) {
            msg[msg_c] = 0;
        }
        pclose(desc);
    }

    baritem * result = malloc(sizeof(baritem));
    result -> string = msg;
    result -> color = initcolor(dc, WMNAME_FOREGROUND, WMNAME_BACKGROUND);
    result -> type = 'W';
    return result;
}


baritem * timeclock_s(DC * d) {
    FILE * desc = popen("date +'%H:%M:%S'", "r");
    char * msg = malloc(20);
    int msg_c = 0; char msg_s;
    if (desc) {
        while( (msg_s = fgetc(desc)) != '\n') {
            msg[msg_c++] = msg_s;
        }
        if (msg_c < 20) {
            msg[msg_c] = 0;
        }
        pclose(desc);
    }

    baritem * result = malloc(sizeof(baritem));
    result -> string = msg;
    result -> color = initcolor(dc, CLOCK_FOREGROUND, CLOCK_BACKGROUND);
    result -> type = 'T';
    return result;
}


baritem * desktops_s(DC * d) {
    baritem * result = malloc(sizeof(baritem));
    result -> string = "□ □ ■ □ ▶";
    result -> color = initcolor(dc, CLOCK_FOREGROUND, CLOCK_BACKGROUND);
    result -> type = 'D';
    return result;
}







/* END SPACE FOR MODULE FUNCTIONS */


void drawmenu(void) {

    dc->x = 0;
    dc->y = 0;
    dc->w = 0;
    dc->h = height;

    dc->text_offset_y = 0;

    if(mh < height) {
        dc->text_offset_y = (height - mh) / 2;
    }

    drawrect(dc, 0, 0, mw, height, True, normcol->BG);

    itemlist * left = config_to_list(LEFT_ALIGN);
    itemlist * right = config_to_list(RIGHT_ALIGN);
    itemlist * center = config_to_list(CENTER_ALIGN);
    
    int llen = total_list_length(left);
    int rlen = total_list_length(right);
    int clen = total_list_length(center);

    draw_list(left);
    dc -> x = mw-rlen;
    draw_list(right);
    dc -> x = (mw-clen)/2;
    draw_list(center);
    
    free_list(left);
    free_list(right);
    free_list(center);
    
    mapdc(dc, win, mw, height);
}




itemlist * config_to_list (char * list) {
    itemlist * head = NULL;
    itemlist * tail = NULL; // add to tail
    while (*(list) != 0) {
        itemlist * next = malloc(sizeof(itemlist));
        next -> item = char_to_item(*(list++));
        next -> next = NULL;
        if (head == NULL) {
            head = next;
            tail = next;
        } else {
            tail -> next = next;
            tail = tail -> next;
        }
    }
    return head;
}

baritem * char_to_item(char c) {
    switch(c) {
        case 'B':
            return battery_s(dc);
        case 'T':
            return timeclock_s(dc);
        case 'W':
            return wmname_s(dc);
        case 'D':
            return desktops_s(dc);
        default :
            return NULL;
    }
}

void free_list(itemlist * list) {
    while(list != NULL) {
        free_baritem(list -> item);
        itemlist * old = list;
        list = list -> next;
        free(old);
    }
}

void free_baritem(baritem * item) {
    switch(item -> type) {
        case 'B':
            free(item -> string);
    }
    free(item);
}

unsigned int total_list_length(itemlist * list) {
    unsigned int len = 0;
    while(list != NULL) {
        list -> item -> length = textw(dc, list -> item -> string);
        len += list -> item -> length;
        list = list -> next;
    }
    return len;
}

void draw_list(itemlist * list) {
    while(list != NULL) {
        dc -> w = list -> item -> length;
        drawtext(dc, list -> item -> string, list -> item -> color);
        dc -> x += dc -> w;
        list = list -> next;
    }
}


































void run(void) {
    while(1){
        drawmenu();
        sleep(1);
    }
}



// TODO: clean this shit
void setup(void) {
    int x, y, screen;
    XSetWindowAttributes wa;

    screen = DefaultScreen(dc->dpy);
    root = RootWindow(dc->dpy, screen);
    utf8 = XInternAtom(dc->dpy, "UTF8_STRING", False);

    /* menu geometry */
    bh = dc->font.height + 2;
    lines = MAX(lines, 0);
    mh = (MAX(MIN(lines + 1, itemcount), 1)) * bh;

    if(height < mh) {
        height = mh;
    }
    x = 0;
    y = topbar ? 0 : DisplayHeight(dc->dpy, screen) - height;
    mw = DisplayWidth(dc->dpy, screen);
    /* menu window */
    wa.override_redirect = True;
    wa.background_pixmap = ParentRelative;
    wa.event_mask = ExposureMask | KeyPressMask | VisibilityChangeMask;
    win = XCreateWindow(dc->dpy, root, x, y, mw, height, 0,
                        DefaultDepth(dc->dpy, screen), CopyFromParent,
                        DefaultVisual(dc->dpy, screen),
                        CWOverrideRedirect | CWBackPixmap | CWEventMask, &wa);

    resizedc(dc, mw, height);
    XMapRaised(dc->dpy, win);
}
