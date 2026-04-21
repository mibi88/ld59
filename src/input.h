#ifndef INPUT_H
#define INPUT_H

#include "gfx.h"
#include "eval.h"
#include "circuit.h"

struct input {
    void (*render)(struct input *input, struct circuit *circuit,
                   struct eval *eval);
    void (*logic)(struct input *input, struct circuit *circuit,
                  struct eval *eval);

    struct image *font;

    int x, y;

    void *extra;
};

#endif
