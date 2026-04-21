#include <stdio.h>
#include <stdlib.h>

#include "gfx.h"
#include "game.h"

static struct game game;

static void render(unsigned long int ms, void *data) {
    (void)data;

    game_logic(&game, ms);
    game_draw(&game, ms);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    if(gfx_init(640, 480, "World of Gates")){
        return 1;
    }

    if(game_init(&game) == 1){
        gfx_free();

        return 1;
    }

    gfx_mainloop(render, NULL);

    game_free(&game);

    gfx_free();

    return 0;
}
