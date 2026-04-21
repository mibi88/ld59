#ifndef GAME_H
#define GAME_H

#include "gfx.h"
#include "eval.h"
#include "input.h"
#include "button.h"
#include "circuit.h"
#include "toolbar.h"

#define SIDEBAR_SIZE 250
#define TOOLBAR_SIZE 32

#define STEPS_PER_FRAME 10

#define LEVEL_NUM 5

enum {
    S_TITLE,
    S_HELP,
    S_LEVEL_SELECTION,
    S_GAME,
    S_GO_TO_MENU,
    S_VALID,
    S_INVALID,
    S_END,
    S_MEM_ERROR
};

struct game {
    struct circuit all_circuits[LEVEL_NUM];

    struct image font;
    struct image gates_img;
    struct image actions_img;
    struct image title_img;
    struct image level_img;
    struct image lock_img;
    struct image play_img;

    struct render_info r;
    struct circuit circuit;
    struct eval eval;

    struct input input;

    struct toolbar actions;
    struct toolbar gates;

    struct button run;
    struct button stop;
    struct button validate;
    struct button menu;
    struct button reset;

    char *assignment;

    unsigned int max_level;
    unsigned int level;

    int last_mx, last_my;

    int wire_start_x, wire_start_y;
    unsigned int leftb_state : 1;
    unsigned int rightb_state : 1;
    unsigned int return_state : 1;
    unsigned int evaluating : 1;
    unsigned int state : 4;
};

int game_init(struct game *game);
void game_logic(struct game *game, unsigned long int ms);
void game_draw(struct game *game, unsigned long int ms);
void game_free(struct game *game);

#endif
