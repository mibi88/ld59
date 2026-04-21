#include "game.h"
#include "gates.h"
#include "checks.h"
#include "actions.h"

#include "bitinput.h"

#include <math.h>

#include <stdio.h>
#include <string.h>

#define TEXT_WIDTH(s) ((sizeof(s)-1)*(game->font.w/16))
#define TEXT_WIDTH_RT(s) (strlen(s)*(game->font.w/16))
#define FONT_HEIGHT (game->font.h/6)

#define MARGIN 5

struct level {
    char *name;
    char *assignment;

    char *in_labels[32];
    char *out_labels[32];

    unsigned char in_count;
    unsigned char out_count;

    size_t max_gates;
    size_t max_wires;

    int (*init_input)(struct input *input);
    void (*free_input)(struct input *input);
    int (*check)(struct circuit *circuit, struct eval *eval);
};

static struct level levels[LEVEL_NUM] = {
    {
        "NOT",
        "Inverse B.",
        {"B"},
        {"NOT B"},
        1,
        1,

        32,
        32,

        bitinput_init,
        bitinput_free,
        not_check
    },
    {
        "AND",
        "Output BOOL 1 AND BOOL 2.",
        {"BOOL 1", "BOOL 2"},
        {"AND"},
        2,
        1,

        32,
        32,

        bitinput_init,
        bitinput_free,
        and_check
    },
    {
        "Carry adder",
        "Add UINT 1 and UINT 2.",
        {"UINT 1", "UINT 2"},
        {"BIT 1", "BIT 0"},
        2,
        2,

        32,
        32,

        bitinput_init,
        bitinput_free,
        add_check
    },
    {
        "Flip-flop",
        "Store a single bit.\nIts value will\nbe output.",
        {"SET", "RESET"},
        {"VALUE"},
        2,
        1,

        32,
        32,

        bitinput_init,
        bitinput_free,
        flipflop_check
    },
    {
        "MUX",
        "Output LINE 1 if SELECT\nis 0 and LINE 2\notherwise.",
        {"LINE 1", "LINE 2", "SELECT."},
        {"LINE"},
        3,
        1,

        32,
        32,

        bitinput_init,
        bitinput_free,
        mux_check
    }
};

static int load_level(struct game *game, unsigned int level) {
    if(circuit_clone(&game->circuit, game->all_circuits+level)){
        return 1;
    }

    if(levels[level].init_input(&game->input)){
        circuit_free(&game->circuit);

        return 2;
    }

    game->run.activated = 1;
    game->stop.activated = 0;

    game->evaluating = 0;

    game->actions.selected = 0;
    game->gates.selected = 0;

    game->r.x = -16-TOOLBAR_SIZE;
    game->r.y = -16;
    game->r.scale = 1;
    game->r.show_states = 0;

    game->assignment = levels[level].assignment;

    game->level = level;

    return 0;
}

static void free_level(struct game *game) {
    if(game->evaluating) eval_free(&game->eval);
    circuit_free(&game->circuit);
    levels[game->level].free_input(&game->input);
}

int game_init(struct game *game) {
    static unsigned char actions_tools[5] = {A_ADD, A_EDIT, A_MOVE, A_DELETE,
                                             A_DETACH};
    static unsigned char gates[4] = {G_AND, G_OR, G_XOR, G_BUFFER};

    static const struct button run_button = {
        NULL,
        0, 8,
        A_RUN,
        4, 4,
        0,
        1
    };
    static const struct button stop_button = {
        NULL,
        0, 8,
        A_STOP,
        4, 4,
        0,
        0
    };
    static const struct button menu_button = {
        NULL,
        0, 8,
        A_MENU,
        4, 4,
        0,
        1
    };
    static const struct button validate_button = {
        NULL,
        0, 8,
        A_VALIDATE,
        4, 4,
        0,
        1
    };
    static const struct button reset_button = {
        NULL,
        0, 8,
        A_RESET,
        4, 4,
        0,
        1
    };

    size_t i;

    if(gfx_load_image(&game->font, "assets/font.png")){
        return 1;
    }
    if(gfx_load_image(&game->gates_img, "assets/gates.png")){
        gfx_free_image(&game->font);

        return 1;
    }
    if(gfx_load_image(&game->actions_img, "assets/actions.png")){
        gfx_free_image(&game->font);
        gfx_free_image(&game->gates_img);

        return 1;
    }
    if(gfx_load_image(&game->title_img, "assets/title.png")){
        gfx_free_image(&game->font);
        gfx_free_image(&game->gates_img);
        gfx_free_image(&game->actions_img);

        return 1;
    }
    if(gfx_load_image(&game->level_img, "assets/level.png")){
        gfx_free_image(&game->font);
        gfx_free_image(&game->gates_img);
        gfx_free_image(&game->actions_img);
        gfx_free_image(&game->title_img);

        return 1;
    }
    if(gfx_load_image(&game->lock_img, "assets/lock.png")){
        gfx_free_image(&game->font);
        gfx_free_image(&game->gates_img);
        gfx_free_image(&game->actions_img);
        gfx_free_image(&game->title_img);
        gfx_free_image(&game->level_img);

        return 1;
    }
    if(gfx_load_image(&game->play_img, "assets/play.png")){
        gfx_free_image(&game->font);
        gfx_free_image(&game->gates_img);
        gfx_free_image(&game->actions_img);
        gfx_free_image(&game->title_img);
        gfx_free_image(&game->level_img);
        gfx_free_image(&game->lock_img);

        return 1;
    }

    game->run = run_button;
    game->run.img = &game->actions_img;
    game->stop = stop_button;
    game->stop.img = &game->actions_img;
    game->menu = menu_button;
    game->menu.img = &game->actions_img;
    game->validate = validate_button;
    game->validate.img = &game->actions_img;
    game->reset = reset_button;
    game->reset.img = &game->actions_img;

    toolbar_init(&game->actions, &game->actions_img, 4, 4, 8, 8,
                 actions_tools, 5);
    toolbar_init(&game->gates, &game->gates_img, 8, 8, 8+16*18+8, 8,
                 gates, 4);

    game->r.font = &game->font;
    game->r.gates = &game->gates_img;
    game->r.x = -16-TOOLBAR_SIZE;
    game->r.y = -16;
    game->r.scale = 1;
    game->r.show_states = 0;

    game->evaluating = 0;

    game->max_level = 0;
    game->state = S_TITLE;

    game->leftb_state = 0;
    game->rightb_state = 0;

    game->input.font = &game->font;
    game->input.y = TOOLBAR_SIZE;

    for(i=0;i<LEVEL_NUM;i++){
        if(circuit_init(game->all_circuits+i,
                        levels[i].in_labels, levels[i].in_count,
                        levels[i].out_labels, levels[i].out_count,
                        levels[i].max_gates, levels[i].max_wires)){
            for(;i--;){
                circuit_free(game->all_circuits+i);
            }

            game->state = S_MEM_ERROR;

            return 2;
        }
    }

    return 0;
}

static void game_reset(struct game *game) {
    size_t i;

    game->run.activated = 1;
    game->stop.activated = 0;

    game->actions.selected = 0;
    game->gates.selected = 0;

    game->r.x = -16-TOOLBAR_SIZE;
    game->r.y = -16;
    game->r.scale = 1;
    game->r.show_states = 0;

    game->evaluating = 0;

    game->max_level = 0;
    game->state = S_TITLE;

    game->leftb_state = 0;
    game->rightb_state = 0;

    /* TODO: Just reset the circuits themselves. */

    for(i=0;i<LEVEL_NUM;i++){
        circuit_free(game->all_circuits+i);
        if(circuit_init(game->all_circuits+i,
                        levels[i].in_labels, levels[i].in_count,
                        levels[i].out_labels, levels[i].out_count,
                        levels[i].max_gates, levels[i].max_wires)){
            for(;i--;){
                circuit_free(game->all_circuits+i);
            }

            game->state = S_MEM_ERROR;
        }
    }
}

static void wire_update_start(struct game *game, struct wire *wire) {
    void *item;

    if((item = circuit_get_gate_at(&game->circuit, wire->x1,
                                   wire->y1)) != NULL){
        struct gate *gate = item;

        if(gate->out == NULL){
            puts("end");

            gate->out = wire;
            wire->in.type = T_GATE;
            wire->in.in.gate = gate;
        }
    }else if((item = circuit_get_io_at(&game->circuit,
                                       wire->x1, wire->y1)) != NULL){
        struct io *io = item;
        int p;

        puts("io found");

        if((p = circuit_get_io_pin(io, wire->y1)) >= 0 && !io->is_output &&
           io->wires[p] == NULL){
            puts("in");

            io->wires[p] = wire;
            wire->in.type = T_IO;
            wire->in.in.io = io;

            circuit_move_wire_start(wire,
                                    io->x+IO_WIDTH/TILE_WIDTH-1, wire->y1);
        }
    }else if((item = circuit_get_other_wire_at(&game->circuit,
                                               wire->x1, wire->y1,
                                               wire)) != NULL){
        struct wire *in_wire = item;

        if(in_wire->x1 != wire->x1 || in_wire->y1 != wire->y1){
            if(circuit_connect_wire(in_wire, wire)){
                /* TODO: Display an error message */
            }
        }
    }
}

static void wire_update_end(struct game *game, struct wire *wire) {
    void *item;

    if((item = circuit_get_gate_at(&game->circuit, wire->x2,
                                   wire->y2)) != NULL){
        struct gate *gate = item;

        if(gate->type == T_BUFFER){
            if(gate->in1 == NULL){
                gate->in1 = wire;
                wire->out.type = T_GATE;
                wire->out.out.gate = gate;
            }
        }else if(wire->y1 <= wire->y2 && gate->in1 == NULL){
            /* Connect to the upper pin */
            puts("upper");
            gate->in1 = wire;
            wire->out.type = T_GATE;
            wire->out.out.gate = gate;
        }else if(gate->in2 == NULL){
            /* Connect to the lower pin */
            puts("lower");

            gate->in2 = wire;
            wire->out.type = T_GATE;
            wire->out.out.gate = gate;
        }else if(gate->in1 == NULL){
            /* Connect to the upper pin */
            puts("upper");
            gate->in1 = wire;
            wire->out.type = T_GATE;
            wire->out.out.gate = gate;
        }
    }else if((item = circuit_get_io_at(&game->circuit,
                                       wire->x2, wire->y2)) != NULL){
        struct io *io = item;
        int p;

        puts("io found");

        if((p = circuit_get_io_pin(io, wire->y2)) >= 0 && io->is_output &&
           io->wires[p] == NULL){
            puts("out");

            io->wires[p] = wire;
            wire->out.type = T_IO;
            wire->out.out.io = io;

            circuit_move_wire_end(wire, io->x, wire->y2);
        }
    }
}

static void delete_selection(struct circuit *circuit) {
    switch(circuit->selection.type) {
        case T_GATE:
            circuit_delete_gate(circuit, circuit->selection.ptr.gate);

            break;
        case T_WIRE:
            circuit_delete_wire(circuit, circuit->selection.ptr.wire);

            break;
    }
}

static void delete_at(struct circuit *circuit, int x, int y) {
    void *item;

    if((item = circuit_get_gate_at(circuit, x, y)) != NULL){
        circuit_delete_gate(circuit, item);
    }else if((item = circuit_get_wire_at(circuit, x, y)) != NULL){
        circuit_delete_wire(circuit, item);
    }
}

static void detach_selection(struct circuit *circuit) {
    switch(circuit->selection.type) {
        case T_GATE:
            circuit_detach_gate(circuit->selection.ptr.gate);

            break;
        case T_WIRE:
            circuit_detach_wire_start(circuit->selection.ptr.wire);
            circuit_detach_wire_end(circuit->selection.ptr.wire);

            break;

        case T_IO:
            circuit_detach_io(circuit->selection.ptr.io);

            break;
    }
}

static void detach_at(struct circuit *circuit, int x, int y) {
    void *item;

    if((item = circuit_get_gate_at(circuit, x, y)) != NULL){
        circuit_detach_gate(item);
    }else if((item = circuit_get_wire_at(circuit, x, y)) != NULL){
        struct wire *wire = item;

        if(wire->x1 == x && wire->y1 == y){
            circuit_detach_wire_start(wire);
        }
        if(wire->x2 == x && wire->y2 == y){
            circuit_detach_wire_end(wire);
        }
    }else if((item = circuit_get_io_at(circuit, x, y)) != NULL){
        circuit_detach_io(item);
    }
}

static void add_gate(struct game *game, int mx, int my) {
    int gate_x, gate_y;

    struct gate *gate;

    circuit_get_tile_position(&game->r, &gate_x, &gate_y,
                              mx, my);

    printf("Add gate at %d, %d\n", gate_x, gate_y);

    gate = circuit_get_gate_at(&game->circuit, gate_x, gate_y);

    if(gate == NULL){
        /* Add a gate */
        if(circuit_add_gate(&game->circuit, gate_x, gate_y,
                            game->gates.selected)){
            /* TODO: Display an error message */
        }
    }else{
        gate->negated = !gate->negated;
    }
}

static void logic_ingame(struct game *game, unsigned long int ms) {
int mx, my;
    unsigned int w, h;

#if 0
    int tx, ty;
#endif
    gfx_get_size(&w, &h);

    (void)ms;

    gfx_get_mouse_position(&mx, &my);

    game->run.x = w-SIDEBAR_SIZE/2-18*2-4;
    game->stop.x = w-SIDEBAR_SIZE/2-18-4;
    game->menu.x = w-SIDEBAR_SIZE/2+4;
    game->validate.x = w-SIDEBAR_SIZE/2+4+18;

    if(gfx_buttondown(B_LEFT)){
        if(button_on_click(&game->run, mx, my)){
            /* Evaluate the circuit */

            if(!game->evaluating && !eval_init(&game->eval, &game->circuit)){
                game->evaluating = 1;
                game->r.show_states = 1;
                game->run.activated = 0;
                game->stop.activated = 1;
            }
        }
        if(button_on_click(&game->stop, mx, my)){
            /* Stop evaluating the circuit */

            if(game->evaluating){
                game->evaluating = 0;
                game->r.show_states = 0;
                game->run.activated = 1;
                game->stop.activated = 0;
            }
        }
        if(button_on_click(&game->menu, mx, my)){
            game->state = S_GO_TO_MENU;
        }
        if(button_on_click(&game->validate, mx, my)){
            if(game->evaluating){
                eval_free(&game->eval);
                game->evaluating = 0;
                game->r.show_states = 0;
                game->run.activated = 1;
                game->stop.activated = 0;
            }

            if(!eval_init(&game->eval, &game->circuit)){
                if(levels[game->level].check(&game->circuit, &game->eval)){
                    eval_free(&game->eval);

                    game->leftb_state = 0;
                    game->state = S_VALID;
                }else{
                    eval_free(&game->eval);

                    game->state = S_INVALID;
                }
            }
        }
    }

    if(!game->evaluating){
#if 0
        circuit_get_tile_position(&game->r, &tx, &ty, mx, my);

        printf("%d, %d             \r", tx, ty);
#endif

        if(gfx_buttondown(B_LEFT)){
            toolbar_on_click(&game->actions, mx, my);
            toolbar_on_click(&game->gates, mx, my);

            if(!game->leftb_state && game->actions.selected == A_EDIT){
                circuit_select(&game->circuit, &game->r, mx, my);
            }
            game->leftb_state = 1;
        }else if(game->leftb_state){
            if(my <= TOOLBAR_SIZE || mx >= (int)w-SIDEBAR_SIZE){
                goto IN_MENUS;
            }
            switch(game->actions.selected){
                case A_ADD:
                    {
                        add_gate(game, mx, my);
                    }
                    break;
                case A_EDIT:
                    switch(game->circuit.selection.type){
                        case T_GATE:
                            {
                                int tx, ty;

                                circuit_get_tile_position(&game->r, &tx, &ty,
                                                          mx, my);

                                circuit_move_gate(game->circuit.selection.ptr
                                                  .gate, tx, ty);
                            }

                            break;
                        case T_WIRE:
                            {
                                int tx, ty;
                                struct wire *wire = game->circuit.selection.ptr
                                                    .wire;

                                circuit_get_tile_position(&game->r, &tx, &ty,
                                                          mx, my);

                                if(wire->x1 == game->circuit.selection.tx &&
                                   wire->y1 == game->circuit.selection.ty &&
                                   wire->in.type == T_NONE){
                                    circuit_move_wire_start(wire, tx, ty);
                                    wire_update_start(game, wire);
                                }

                                if(wire->x2 == game->circuit.selection.tx &&
                                   wire->y2 == game->circuit.selection.ty &&
                                   wire->out.type == T_NONE){
                                    circuit_move_wire_end(wire, tx, ty);
                                    wire_update_end(game, wire);
                                }
                            }

                            break;

                        case T_IO:
                            {
                                int tx, ty;

                                circuit_get_tile_position(&game->r, &tx, &ty,
                                                          mx, my);

                                tx = game->circuit.selection.ptr
                                    .io->x+tx-game->circuit.selection.tx;
                                ty = game->circuit.selection.ptr
                                    .io->y+ty-game->circuit.selection.ty;

                                circuit_move_io(game->circuit.selection.ptr.io,
                                                tx, ty);
                            }

                            break;
                    }

                    break;

                case A_MOVE:

                    break;

                case A_DELETE:
                    {
                        int tx, ty;

                        circuit_get_tile_position(&game->r, &tx, &ty, mx, my);
                        delete_at(&game->circuit, tx, ty);
                    }

                    break;

                case A_DETACH:
                    {
                        int tx, ty;

                        circuit_get_tile_position(&game->r, &tx, &ty, mx, my);
                        detach_at(&game->circuit, tx, ty);
                    }

                    break;
            }
IN_MENUS:
            game->leftb_state = 0;
        }

        if(gfx_buttondown(B_RIGHT)){
            if(!game->rightb_state){
                game->rightb_state = 1;

                circuit_get_tile_position(&game->r,
                                          &game->wire_start_x,
                                          &game->wire_start_y,
                                          mx, my);
            }
        }else if(game->rightb_state){
            int wire_end_x, wire_end_y;

            circuit_get_tile_position(&game->r,
                                      &wire_end_x, &wire_end_y,
                                      mx, my);

            if(game->wire_start_x != wire_end_x ||
               game->wire_start_y != wire_end_y){
                struct wire *wire;

                printf("Wire from %d, %d to %d, %d\n",
                       game->wire_start_x, game->wire_start_y,
                       wire_end_x, wire_end_y);

                /* Add a wire */
                wire = circuit_add_wire(&game->circuit,
                                        game->wire_start_x, game->wire_start_y,
                                        wire_end_x, wire_end_y);
                if(wire != NULL){
                    wire_update_start(game, wire);
                    wire_update_end(game, wire);
                }else{
                    /* TODO: Display an error message */
                }
            }

            game->rightb_state = 0;
        }

        if(gfx_keydown(K_D)){
            detach_selection(&game->circuit);
        }

        if(gfx_keydown(K_DELETE)){
            delete_selection(&game->circuit);
        }

        if(gfx_keydown(K_RETURN)){
            if(!game->return_state) add_gate(game, mx, my);
            game->return_state = 1;
        }else{
            game->return_state = 0;
        }
    }else{
        eval_run_multiple(&game->eval, STEPS_PER_FRAME);
    }

    if(gfx_buttondown(B_SCROLLUP)){
#if 0
        float s = game->r.scale;
#endif

        game->r.scale /= 1.5;

        if(game->r.scale < 1){
            game->r.scale = 1;
        }else{
#if 0
            /* FIXME: Zoom to the cursor */
            game->r.x += mx*(game->r.scale-s)/2;
            game->r.y += my*(game->r.scale-s)/2;
#endif
        }
    }else if(gfx_buttondown(B_SCROLLDOWN)){
#if 0
        float s = game->r.scale;
#endif

        game->r.scale *= 1.5;

        if(game->r.scale > 8){
            game->r.scale = 8;
        }else{
#if 0
            /* FIXME: Zoom to the cursor */
            game->r.x += mx*(game->r.scale-s)/2;
            game->r.y += my*(game->r.scale-s)/2;
#endif
        }
    }

    if(gfx_buttondown(B_MIDDLE) ||
       (game->actions.selected == A_MOVE && gfx_buttondown(B_LEFT))){
        game->r.x += game->last_mx-mx;
        game->r.y += game->last_my-my;
    }

    game->input.x = w-SIDEBAR_SIZE;
    game->input.logic(&game->input, &game->circuit,
                      game->evaluating ? &game->eval : NULL);

    game->last_mx = mx;
    game->last_my = my;
}

static int menu_button_pressed(struct game *game, unsigned long int ms,
                               char *label1) {
    unsigned int w, h;
    int mx, my;

    int x1, y1;
    int x2, y2;

    int w1 = TEXT_WIDTH_RT(label1);
    int w2 = TEXT_WIDTH("GO TO MENU");

    (void)ms;

    gfx_get_size(&w, &h);
    gfx_get_mouse_position(&mx, &my);

    x1 = (w-w1)/2;
    y1 = (h-FONT_HEIGHT)/2;
    x2 = (w-w2)/2;
    y2 = (h-FONT_HEIGHT)/2+FONT_HEIGHT;

    if(mx >= x1 && mx <= x1+w1 && my >= y1 && my <= y1+(int)FONT_HEIGHT){
        return 1;
    }else if(mx >= x2 && mx <= x2+w2 && my >= y2 && my <= y2+(int)FONT_HEIGHT){
        return 2;
    }

    return 0;
}

void game_logic(struct game *game, unsigned long int ms) {
    unsigned int w, h;
    int mx, my;

    gfx_get_size(&w, &h);
    gfx_get_mouse_position(&mx, &my);

    switch(game->state){
        case S_TITLE:
            {
                int x1, y1;
                int x2, y2;

                x1 = (w-TEXT_WIDTH("Play"))/2;
                y1 = (h-FONT_HEIGHT)/2;
                x2 = x1+TEXT_WIDTH("Play");
                y2 = y1+FONT_HEIGHT;

                if(gfx_buttondown(B_LEFT)){
                    game->leftb_state = 1;
                }else if(game->leftb_state){
                    if(mx >= x1 && mx <= x2 && my >= y1 && my <= y2){
                        game->rightb_state = 0;

                        game->state = S_LEVEL_SELECTION;
                    }
                    game->leftb_state = 0;
                }
            }

            break;

        case S_HELP:

            break;

        case S_LEVEL_SELECTION:
            {
                unsigned int bx = w%(game->level_img.w+MARGIN)/2;
                unsigned int tile_x = (mx-bx)/(game->level_img.w+MARGIN);
                unsigned int tile_y = (my-FONT_HEIGHT*5)/
                                      (game->level_img.h+MARGIN);
                unsigned int tile_id = tile_y*w/(game->level_img.w+MARGIN)+
                                       tile_x;

                if(gfx_buttondown(B_LEFT)){
                    game->leftb_state = 1;
                }else if(game->leftb_state){
                    if(mx >= (int)bx && my > (int)FONT_HEIGHT*5 &&
                       tile_id <= game->max_level){

                        game->level = tile_id;
                        if(!load_level(game, game->level)){
                            game->state = S_GAME;
                        }else{
                            puts("Failed to load level!");
                            game->state = S_MEM_ERROR;
                        }
                    }

                    game->leftb_state = 0;
                }

                if(gfx_buttondown(B_RIGHT)){
                    game->rightb_state = 1;
                }else if(game->rightb_state){
                    if(button_on_click(&game->reset, mx, my)){
                        game_reset(game);
                    }

                    game->rightb_state = 0;
                }

                game->reset.x = w-32;
                game->reset.y = h-32;
            }

            break;

        case S_GAME:
            logic_ingame(game, ms);

            break;

        case S_GO_TO_MENU:
            {
                int button;

                if(gfx_buttondown(B_LEFT)){
                    game->leftb_state = 1;
                }else if(game->leftb_state){
                    button = menu_button_pressed(game, ms, "CANCEL");

                    if(button == 1){
                        game->state = S_GAME;
                    }else if(button == 2){
                        game->rightb_state = 0;

                        game->state = S_LEVEL_SELECTION;
                    }
                    game->leftb_state = 0;
                }
            }

            break;

        case S_VALID:
            {
                int button;

                if(gfx_buttondown(B_LEFT)){
                    game->leftb_state = 1;
                }else if(game->leftb_state){
                    button = menu_button_pressed(game, ms, "CONTINUE");

                    if(button) printf("level: %u\n", game->level);

                    /* Store the level */
                    if(button && circuit_copy(game->all_circuits+game->level,
                                              &game->circuit)){
                        puts("Failed to copy the circuit!");
                        game->state = S_MEM_ERROR;
                    }

                    if(button){
                        free_level(game);

                        if(game->max_level+1 >= LEVEL_NUM){
                            game->rightb_state = 0;

                            game->state = S_END;

                            break;
                        }
                        game->level++;
                        if(game->level >= game->max_level) game->max_level++;
                    }

                    if(button == 1){
                        game->state = S_GAME;

                        if(load_level(game, game->level)){
                            puts("Failed to load the level!");

                            game->state = S_MEM_ERROR;
                        }
                    }else if(button == 2){
                        game->rightb_state = 0;

                        game->state = S_LEVEL_SELECTION;
                    }
                    game->leftb_state = 0;
                }
            }

            break;

        case S_INVALID:
            {
                int button;

                if(gfx_buttondown(B_LEFT)){
                    game->leftb_state = 1;
                }else if(game->leftb_state){
                    button = menu_button_pressed(game, ms, "CONTINUE EDITING");

                    if(button == 1){
                        game->state = S_GAME;
                    }else if(button == 2){
                        free_level(game);

                        game->rightb_state = 0;

                        game->state = S_LEVEL_SELECTION;
                    }
                    game->leftb_state = 0;
                }
            }

            break;

        case S_END:
            {
                int button;

                button = menu_button_pressed(game, ms, "RESET (right click)");

                if(gfx_buttondown(B_LEFT)){
                    game->leftb_state = 1;
                }else if(game->leftb_state){
                    if(button == 2){
                        game->rightb_state = 0;

                        game->state = S_LEVEL_SELECTION;
                    }
                    game->leftb_state = 0;
                }

                if(gfx_buttondown(B_RIGHT)){
                    game->rightb_state = 1;
                }else if(game->rightb_state){
                    if(button == 1){
                        game_reset(game);
                    }
                    game->rightb_state = 0;
                }
            }

            break;
    }
}

static void draw_popup_frame(void) {
    unsigned int w, h;

    unsigned int x1, y1;
    unsigned int x2, y2;

    gfx_get_size(&w, &h);

    x1 = w/4;
    y1 = h/4;
    x2 = 3*w/4;
    y2 = 3*h/4;

    gfx_rect(0, 0, w, h, 127, 127, 127, 127);

    gfx_rect(x1, y1, x2-x1, y2-y1, 255, 255, 255, 255);

    gfx_line(x1, y1, x2, y1, 0, 0, 0, 255, 3);
    gfx_line(x2, y1, x2, y2, 0, 0, 0, 255, 3);
    gfx_line(x2, y2, x1, y2, 0, 0, 0, 255, 3);
    gfx_line(x1, y2, x1, y1, 0, 0, 0, 255, 3);
}

static char buffer[256];

static void render_game(struct game *game, unsigned long int ms) {
    unsigned int w, h;

    gfx_get_size(&w, &h);

    (void)ms;

    if(game->evaluating){
        gfx_rect(0, 0, w, h, 223, 223, 223, 255);
    }

    circuit_render(&game->circuit, &game->r);

    gfx_rect(0, 0, w, TOOLBAR_SIZE, 191, 191, 191, 191);
    gfx_rect(w-SIDEBAR_SIZE, TOOLBAR_SIZE, w, h, 191, 191, 191, 191);

    toolbar_render(&game->actions);
    toolbar_render(&game->gates);

    button_render(&game->run);
    button_render(&game->stop);
    button_render(&game->menu);
    button_render(&game->validate);

    sprintf(buffer, "Wires left: %lu\n"
                    "Gates left: %lu\n",
            (unsigned long int)(game->circuit.max_wires-
                                game->circuit.wire_count),
            (unsigned long int)(game->circuit.max_gates-
                                game->circuit.gate_count));

    gfx_text(&game->font, w-SIDEBAR_SIZE+game->font.w/16,
             h-3*game->font.h/6, buffer, 0, 0, 0, 255);
    gfx_text(&game->font, w-SIDEBAR_SIZE+game->font.w/16,
             h-7*game->font.h/6, game->assignment, 0, 0, 255, 255);

    game->input.render(&game->input, &game->circuit,
                       game->evaluating ? &game->eval : NULL);
}

void game_draw(struct game *game, unsigned long int ms) {
    unsigned int w, h;

    gfx_get_size(&w, &h);

    (void)ms;

    switch(game->state){
        case S_TITLE:
            gfx_image(&game->title_img, (w-game->title_img.w)/2,
                      2*FONT_HEIGHT);

            gfx_text(&game->font, (w-TEXT_WIDTH("Play"))/2, (h-FONT_HEIGHT)/2,
                     "Play", 0, 0, 0, 255);

            gfx_text(&game->font, (w-TEXT_WIDTH("Made with <3 by Mibi88 "
                                                "for the Ludum Dare 59 "
                                                "(April 2026)"))/2,
                     h-4*FONT_HEIGHT,
                     "Made with <3 by Mibi88 for the Ludum Dare 59 "
                     "(April 2026)",
                     0, 0, 0, 255);

            break;

        case S_HELP:

            break;

        case S_LEVEL_SELECTION:

            {
                unsigned int bx = w%(game->level_img.w+MARGIN)/2;

                unsigned int x = bx;
                unsigned int y = FONT_HEIGHT*5;

                unsigned int lock_x = (game->level_img.w-game->lock_img.w)/2;
                unsigned int lock_y = (game->level_img.h-game->lock_img.h)/2;

                size_t i;

                unsigned long int gate_count;
                unsigned long int wire_count;

                gfx_text(&game->font,
                         (w-TEXT_WIDTH("LEVEL SELECTION"))/2, 2*FONT_HEIGHT,
                         "LEVEL SELECTION", 0, 0, 0, 255);

                for(i=0;i<LEVEL_NUM;i++){
                    gfx_image(&game->level_img, x, y);

                    if(i > game->max_level){
                        gfx_image(&game->lock_img, x+lock_x, y+lock_y);
                    }else{
                        gfx_text(&game->font,
                                 x+(game->level_img.w+MARGIN-
                                    TEXT_WIDTH_RT(levels[i].name))/2,
                                 y+FONT_HEIGHT, levels[i].name, 0, 0, 0, 255);

                        gfx_image(&game->play_img, x+lock_x, y+lock_y);
                    }

                    x += game->level_img.w;

                    if(x+game->level_img.w > w){
                        x = bx;
                        y += game->level_img.h+MARGIN;
                    }
                }

                gate_count = 0;
                wire_count = 0;

                for(i=0;i<LEVEL_NUM;i++){
                    gate_count += game->all_circuits[i].gate_count;
                    wire_count += game->all_circuits[i].wire_count;
                }

                sprintf(buffer, "%lu gates used\n%lu wires used", gate_count,
                        wire_count);
                gfx_text(&game->font, 32, h-32-2*FONT_HEIGHT, buffer,
                         0, 0, 0, 255);

                button_render(&game->reset);
            }

            break;

        case S_GAME:
            render_game(game, ms);

            break;

        case S_GO_TO_MENU:
            draw_popup_frame();

            gfx_text(&game->font,
                     (w-TEXT_WIDTH("Abandon changes and go to menu?"))/2,
                     h/4+2*FONT_HEIGHT, "Abandon changes and go to menu?",
                     0, 0, 0, 255);

            gfx_text(&game->font, (w-TEXT_WIDTH("CANCEL"))/2,
                     (h-FONT_HEIGHT)/2, "CANCEL", 0, 0, 0, 255);
            gfx_text(&game->font, (w-TEXT_WIDTH("GO TO MENU"))/2,
                     (h-FONT_HEIGHT)/2+FONT_HEIGHT, "GO TO MENU",
                     0, 0, 0, 255);

            break;

        case S_VALID:
            render_game(game, ms);
            draw_popup_frame();

            gfx_text(&game->font,
                     (w-TEXT_WIDTH("Well done!"))/2, h/4+2*FONT_HEIGHT,
                     "Well done!", 0, 0, 0, 255);

            gfx_text(&game->font, (w-TEXT_WIDTH("CONTINUE"))/2,
                     (h-FONT_HEIGHT)/2, "CONTINUE", 0, 0, 0, 255);
            gfx_text(&game->font, (w-TEXT_WIDTH("GO TO MENU"))/2,
                     (h-FONT_HEIGHT)/2+FONT_HEIGHT, "GO TO MENU",
                     0, 0, 0, 255);

            break;

        case S_INVALID:
            render_game(game, ms);
            draw_popup_frame();

            gfx_text(&game->font,
                     (w-TEXT_WIDTH("It's not quite right yet!"))/2,
                     h/4+2*FONT_HEIGHT, "It's not quite right yet!",
                     0, 0, 0, 255);

            gfx_text(&game->font, (w-TEXT_WIDTH("CONTINUE EDITING"))/2,
                     (h-FONT_HEIGHT)/2, "CONTINUE EDITING", 0, 0, 0, 255);
            gfx_text(&game->font, (w-TEXT_WIDTH("GO TO MENU"))/2,
                     (h-FONT_HEIGHT)/2+FONT_HEIGHT, "GO TO MENU",
                     0, 0, 0, 255);

            break;

        case S_END:
            draw_popup_frame();

            gfx_text(&game->font,
                     (w-TEXT_WIDTH("You solved all the challenges!"))/2,
                     h/4+2*FONT_HEIGHT, "You solved all the challenges!",
                     0, 0, 0, 255);

            gfx_text(&game->font, (w-TEXT_WIDTH("RESET (right click)"))/2,
                     (h-FONT_HEIGHT)/2, "RESET (right click)", 0, 0, 0, 255);
            gfx_text(&game->font, (w-TEXT_WIDTH("GO TO MENU"))/2,
                     (h-FONT_HEIGHT)/2+FONT_HEIGHT, "GO TO MENU",
                     0, 0, 0, 255);

            break;

        case S_MEM_ERROR:
            draw_popup_frame();

            gfx_text(&game->font,
                     (w-TEXT_WIDTH("Out of mem./mem. error!"))/2,
                     h/4+2*FONT_HEIGHT, "Out of mem./mem. error!",
                     0, 0, 0, 255);
            gfx_text(&game->font,
                     (w-TEXT_WIDTH("Please restart the game."))/2,
                     h/4+2*FONT_HEIGHT+FONT_HEIGHT, "Please restart the game.",
                     0, 0, 0, 255);
    }

#if DEBUG_FRAMERATE
    sprintf(buffer, ms ? "FPS: %lu\n" : "FPS: inf", ms ? 1000/ms : 1000);
    gfx_text(&game->font, 0, 0, buffer, 0, 0, 0, 255);
#endif
}

void game_free(struct game *game) {
    gfx_free_image(&game->font);
    gfx_free_image(&game->gates_img);
    gfx_free_image(&game->actions_img);
    gfx_free_image(&game->title_img);
    gfx_free_image(&game->level_img);
    gfx_free_image(&game->lock_img);
    gfx_free_image(&game->play_img);

    free_level(game);
}
