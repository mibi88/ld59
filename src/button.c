#include "button.h"

void button_render(struct button *button) {
    unsigned int w = button->img->w/button->tools_per_row;
    unsigned int h = button->img->h/button->tools_per_column;

    unsigned int x = button->icon%button->tools_per_row*w;
    unsigned int y = button->icon/button->tools_per_column*h;

    unsigned char c = 127+(button->clicked<<6);
    unsigned char ic = 127+button->activated*128;

    gfx_rect(button->x, button->y, w, h, c, c, c, 255);
    gfx_subimage(button->img, button->x, button->y, w, h, x, y, w, h,
                 255, 255, 255, ic);

    button->clicked = 0;
}

int button_on_click(struct button *button, int mx, int my) {
    int w = button->img->w/button->tools_per_row;
    int h = button->img->h/button->tools_per_column;

    if(!button->activated) return 0;

    if(mx >= button->x && mx <= button->x+w &&
       my >= button->y && my <= button->y+h){
        button->clicked = 1;

        return 1;
    }

    return 0;
}
