#define _DEFAULT_SOURCE

#include <pthread.h>
#include "vdesk.h"
#include "format.h"
#include "tint3.h"

static int current_desktop = 0;
static fmt_map *formatmap = 0;
static pthread_t vdesk_ltnr;
static char *han_zi[10] = {"一", "二", "三", "四", "五", "六", "七", "八", "九", "十"};


void spawn_vdesk_thread(baritem *ipl) {
    pthread_create(&vdesk_ltnr, NULL, vdesk_listen, ipl);
}

void *vdesk_listen(void *DATA) {
    baritem *ipl = DATA;
    Display *dsp = XOpenDisplay(NULL);
    XSelectInput(dsp, root, PropertyChangeMask) ;
    XEvent xe;
    while(1) {
        XNextEvent(dsp, &xe);
        switch(xe.type) {
            case PropertyNotify:
               ipl -> string = get_desktops_info(ipl);
               drawmenu();
               break;
        }
    }

    return NULL;
}

// roman numerals up to 3000
uint roman_numeral(char *buffer, uint num) {
    uint position = 0;
    uint special[13] = {1000, 900, 500, 400, 100, 90, 50, 40, 10, 9, 5, 4, 1};
    uint sp = 0;
    while(num) {
        while(num >= special[sp]) {
            switch(special[sp]) {
                case 900:
                    buffer[position++] = 'C';
                case 1000:
                    buffer[position++] = 'M';
                    break;
                case 400:
                    buffer[position++] = 'C';
                case 500:
                    buffer[position++] = 'D';
                    break;
                case 90:
                    buffer[position++] = 'X';
                case 100:
                    buffer[position++] = 'C';
                    break;
                case 40:
                    buffer[position++] = 'X';
                case 50:
                    buffer[position++] = 'L';
                    break;
                case 9:
                    buffer[position++] = 'I';
                case 10:
                    buffer[position++] = 'X';
                    break;
                case 4:
                    buffer[position++] = 'I';
                case 5:
                    buffer[position++] = 'V';
                    break;
                case 1:
                    buffer[position++] = 'I';
            }
            num -= special[sp];
        }
        sp++;
    }
    return position;
}

void copy_UTF(char *to, char *from) {
    to[0] = from[0];
    to[1] = from[1];
    to[2] = from[2];
}

int print_han_zi(char *buffer, int num) {
    if (num < 11) {
        copy_UTF(buffer, han_zi[num-1]);
        return 3;
    } else if (num < 20) {
        copy_UTF(buffer, han_zi[9]);
        copy_UTF(buffer+3, han_zi[num-11]);
        return 6;
    } else {
        copy_UTF(buffer, han_zi[num/10 - 1]);
        copy_UTF(buffer+3, han_zi[9]);
        if (num%10 == 0) {
            return 6;
        }
        copy_UTF(buffer+6, han_zi[num%10 - 1]);
        return 9;
    }
}

int get_number_of_desktops () {
    return get_x11_cardinal_property(NET_NUMBER_DESKTOPS, _CARDINAL_);
}

int get_current_desktop () {
    return get_x11_cardinal_property(NET_CURRENT_DESKTOP, _CARDINAL_);
}

uint16_t strcons(char *dest, const char *src, uint16_t ctr) {
    uint res = ctr;
    while(ctr --> 0) {
            dest[ctr] = src[ctr];
    }
    return res;
}

int _arabic_numerals(int place, char * string) {
    return place + sprintf(string+place, "%i", current_desktop);
}

int _roman_numerals(int place, char *string) {
    return place + roman_numeral(string+place, current_desktop);
}

int _han_zi(int place, char *string) {
    return place + print_han_zi(string+place, current_desktop);
}

int __xlib_names(int place, char *string) {
    dlist *res = get_x11_cpp_property(NET_DESKTOP_NAMES);
    char *data = (char *)(dget(res, current_desktop-1));
    int result = place + sprintf(string+place, "%c%s%c", 7, data, 7);
    dlist_deep_free(res);
    return result;
}

int _xlib_names(int place, char *string) {
    dlist *res = get_x11_cpp_property(NET_DESKTOP_NAMES);
    char *data = (char *)(dget(res, current_desktop-1));
    int result = place + sprintf(string+place, "%s", data);
    dlist_deep_free(res);
    return result;
}

char *get_desktops_info(baritem *source) {
    if (!source) {
        return NULL;
    }
    if (!(source -> format)) {
        source -> format = "◆ ◇";
    }
    if (!formatmap) {
        formatmap = initmap(8);
        fmt_map_put(formatmap, 'n', &_arabic_numerals);
        fmt_map_put(formatmap, 'R', &_roman_numerals);
        fmt_map_put(formatmap, 'J', &_han_zi);
        fmt_map_put(formatmap, 'N', &_xlib_names);
        fmt_map_put(formatmap, 'U', &__xlib_names);
    }

    current_desktop = 1;
    char default_desktop[10] = {0};
    char selected_desktop[10] = {0};
    uint8_t sel = 0;
    uint8_t wpos = 0;
    uint8_t rpos = 0;
    int numdesk = get_number_of_desktops();
    int curdesk = get_current_desktop();
    char *tmp_bfr = calloc(numdesk * 15, sizeof(char));
    uint8_t ctr = 0;
    uint16_t offset = 0;
    while((source->format)[rpos]) {
        if ((source->format)[rpos] == ' ') {
            sel = 1;
            wpos = 0;
            rpos++;
            continue;
        }
        (sel?selected_desktop:default_desktop)[wpos++] = (source->format)[rpos++];
    }
    
    while(ctr < numdesk) {
        if (ctr == curdesk) {
            offset += format_string(tmp_bfr+offset, selected_desktop, formatmap);
        } else {
            offset += format_string(tmp_bfr+offset, default_desktop, formatmap);
        }
        offset += strcons(tmp_bfr+offset, " ", 1);
        current_desktop++;
        ctr++;
    }

    char *result = calloc(offset+1, sizeof(char));
    memcpy(result, tmp_bfr, offset);
    free(tmp_bfr);
    return result;
}
