#ifndef TOOLBAR_H
#define TOOLBAR_H

#include "gfx.h"

struct toolbar {
    struct image *img;

    int x;
    int y;

    unsigned char *tools;
    unsigned char tool_count;

    unsigned char tools_per_row;
    unsigned char tools_per_column;

    unsigned char selected;
};

void toolbar_init(struct toolbar *toolbar,
                  struct image *img,
                  unsigned char tools_per_row, unsigned char tools_per_column,
                  int x, int y,
                  unsigned char *tools, unsigned char tool_count);
void toolbar_render(struct toolbar *toolbar);
unsigned int toolbar_on_click(struct toolbar *toolbar, int x, int y);

#endif
