#ifndef BUTTON_H
#define BUTTON_H

#include "gfx.h"

struct button {
    struct image *img;

    int x;
    int y;

    unsigned char icon;

    unsigned char tools_per_row;
    unsigned char tools_per_column;

    unsigned int clicked : 1;
    unsigned int activated : 1;
};

void button_render(struct button *button);
int button_on_click(struct button *button, int mx, int my);

#endif
