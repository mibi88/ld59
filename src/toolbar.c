#include "toolbar.h"

void toolbar_init(struct toolbar *toolbar,
                  struct image *img,
                  unsigned char tools_per_row, unsigned char tools_per_column,
                  int x, int y,
                  unsigned char *tools, unsigned char tool_count) {
    toolbar->img = img;

    toolbar->x = x;
    toolbar->y = y;

    toolbar->tools = tools;
    toolbar->tool_count = tool_count;
    toolbar->tools_per_row = tools_per_row;
    toolbar->tools_per_column = tools_per_column;
    toolbar->selected = 0;
}

void toolbar_render(struct toolbar *toolbar) {
    unsigned char i;

    unsigned int rx = toolbar->x;
    unsigned int ry = toolbar->y;

    unsigned int w = toolbar->img->w/toolbar->tools_per_row;
    unsigned int h = toolbar->img->h/toolbar->tools_per_column;

    for(i=0;i<toolbar->tool_count;i++){
        unsigned char id = toolbar->tools[i];

        unsigned int x = id%toolbar->tools_per_row*w;
        unsigned int y = id/toolbar->tools_per_column*h;

        if(i == toolbar->selected){
            gfx_rect(rx, ry, w, h, 127, 127, 127, 255);
        }
        gfx_subimage(toolbar->img, rx, ry, w, h, x, y, w, h,
                     255, 255, 255, 255);

        rx += w+2;
    }
}

unsigned int toolbar_on_click(struct toolbar *toolbar, int x, int y) {
    unsigned int w = toolbar->img->w/toolbar->tools_per_row;
    unsigned int h = toolbar->img->h/toolbar->tools_per_column;

    if(x >= (int)toolbar->x && y >= (int)toolbar->y &&
       x <= (int)(toolbar->x+toolbar->tool_count*(w+2)) &&
       y <= (int)(toolbar->y+h) &&
       (x-toolbar->x)%(w+2) <= w){
        toolbar->selected = (x-toolbar->x)/(w+2);

        return 1;
    }

    return 0;
}
