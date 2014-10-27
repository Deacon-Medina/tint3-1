#include <stdlib.h>

#ifndef _CONFPARSER_H_
#define _CONFPARSER_H_

typedef struct {
    char * id;
    char * name;
    char * type;
    char * format;
    char * source;
    char forground [6];
    char background [6];
} block;

typedef struct l {
    struct l * next;
    block    * data;
} block_list;



typedef struct {
    int border_size;
    int margin_size;
    char * border_color;
    char * background_color;
    char * location;
    block_list * left;
    block_list * center;
    block_list * right;
} bar_config;

bar_config * readblock(FILE * fp);

#endif