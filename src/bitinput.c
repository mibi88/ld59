#include "bitinput.h"

static void render(struct input *input, struct circuit *circuit,
                   struct eval *eval);
static void logic(struct input *input, struct circuit *circuit,
                  struct eval *eval);

int bitinput_init(struct input *input) {
    input->render = render;
    input->logic = logic;

    input->extra = NULL;

    return 0;
}

static void render(struct input *input, struct circuit *circuit,
                   struct eval *eval) {
    int ry;
    unsigned char i;
    unsigned int w = input->font->w/16;

    (void)eval;

    ry = input->y;
    for(i=0;i<circuit->in.size;i++){
        gfx_text(input->font, input->x, ry, circuit->in.labels[i],
                 0, 0, 0, 255);

        gfx_text(input->font, input->x+IO_WIDTH-w, ry,
                 (circuit->in.evaluated>>i)&1 ? (circuit->in.states>>i)&1 ?
                 "1" : "0" : "-",
                 ((circuit->in.evaluated>>i)&(~circuit->in.states>>i)&1)*255,
                 ((circuit->in.evaluated>>i)&(circuit->in.states>>i)&1)*127,
                 0, 255);

        ry += input->font->h/6;
    }

    ry = input->y;
    for(i=0;i<circuit->out.size;i++){
        gfx_text(input->font, input->x+IO_WIDTH+w, ry, circuit->out.labels[i],
                 0, 0, 0, 255);

        gfx_text(input->font, input->x+IO_WIDTH*2, ry,
                 (circuit->out.evaluated>>i)&1 ? (circuit->out.states>>i)&1 ?
                 "1" : "0" : "-",
                 ((circuit->out.evaluated>>i)&(~circuit->out.states>>i)&1)*255,
                 ((circuit->out.evaluated>>i)&(circuit->out.states>>i)&1)*127,
                 0, 255);

        ry += input->font->h/6;
    }
}

static void logic(struct input *input, struct circuit *circuit,
                  struct eval *eval) {
    unsigned char i;

    int y;
    int w = input->font->w/16;
    int h = input->font->h/6;

    int mx, my;

    gfx_get_mouse_position(&mx, &my);

    circuit->in.evaluated = ~0;

    if(gfx_buttondown(B_LEFT)){
        if(input->extra == NULL){
            y = input->y;
            for(i=0;i<circuit->in.size;i++){
                int x = input->x+IO_WIDTH-w;

                if(mx >= x && mx <= x+w &&
                   my >= y && my <= y+h){
                    circuit->in.states ^= 1<<i;
                    circuit_input_updated(circuit);
                    if(eval != NULL) eval_input_updated(eval, circuit);
                }

                y += input->font->h/6;
            }
            input->extra = input;
        }
    }else{
        input->extra = NULL;
    }
}

void bitinput_free(struct input *input) {
    (void)input;
}
