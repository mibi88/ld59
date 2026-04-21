#include "circuit.h"
#include "gfx.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define BETWEEN(x, a, b) ((a) < (b) ? (x) >= (a) && (x) <= (b) : \
                                      (x) >= (b) && (x) <= (a))

static void io_init(struct io *io, int x, int y,
                    char **labels, unsigned char wires,
                    int is_output){
    unsigned char i;

    io->states = 0;
    io->evaluated = 0;
    for(i=0;i<(wires&31);i++){
        io->wires[i] = NULL;
        io->labels[i] = labels[i];
    }

    io->size = wires;

    io->x = x;
    io->y = y;

    io->is_output = !!is_output;
}

static unsigned int io_get_height(struct io *io) {
    return io->size*TILE_HEIGHT+2*TILE_HEIGHT;
}

static void io_render(struct io *io, struct render_info *r) {
    int x1, y1;
    int x2, y2;

    unsigned char i;

    x1 = (io->x*TILE_WIDTH+io->is_output*(IO_PIN_SIZE+1))*
         r->scale-r->x;
    y1 = (io->y*TILE_HEIGHT)*r->scale-r->y;

    x2 = x1+(IO_WIDTH-(IO_PIN_SIZE+1))*r->scale;
    y2 = y1+(io_get_height(io))*r->scale;

    gfx_line(x1, y1, x2, y1, 0, 0, 0, 255, ceil(r->scale));
    gfx_line(x2, y1, x2, y2, 0, 0, 0, 255, ceil(r->scale));
    gfx_line(x2, y2, x1, y2, 0, 0, 0, 255, ceil(r->scale));
    gfx_line(x1, y2, x1, y1, 0, 0, 0, 255, ceil(r->scale));

    if(io->is_output){
        for(i=0;i<io->size;i++){
            y1 += ceil(TILE_HEIGHT*r->scale);

            gfx_line(x1-(IO_PIN_SIZE+1)*r->scale, y1+TILE_WIDTH/2*r->scale,
                     x1, y1+TILE_WIDTH/2*r->scale,
                     0, 0, 0, 255, ceil(r->scale));

            gfx_text(r->font, x1+TILE_WIDTH*r->scale, y1, io->labels[i],
                     0, 0, 0, 255);
        }
    }else{
        for(i=0;i<io->size;i++){
            y1 += ceil(TILE_HEIGHT*r->scale);

            gfx_line(x2, y1+TILE_WIDTH/2*r->scale,
                     x2+(IO_PIN_SIZE+1)*r->scale, y1+TILE_WIDTH/2*r->scale,
                     0, 0, 0, 255, ceil(r->scale));

            gfx_text(r->font, x1+TILE_WIDTH*r->scale, y1, io->labels[i],
                     0, 0, 0, 255);
        }
    }
}

int circuit_init(struct circuit *circuit,
                 char **input_labels, unsigned char input_wires,
                 char **output_labels, unsigned char output_wires,
                 size_t max_gates, size_t max_wires) {
    io_init(&circuit->in, 0, 0, input_labels, input_wires, 0);
    io_init(&circuit->out, 16, 0, output_labels, output_wires, 1);

    circuit->gates = malloc(max_gates*sizeof(struct gate));
    circuit->wires = malloc(max_wires*sizeof(struct wire));

    if(circuit->gates == NULL || circuit->wires == NULL){
        free(circuit->gates);
        circuit->gates = NULL;
        free(circuit->wires);
        circuit->wires = NULL;

        return 1;
    }

    circuit->gate_count = 0;
    circuit->wire_count = 0;

    circuit->max_gates = max_gates;
    circuit->max_wires = max_wires;

    return 0;
}

int circuit_copy(struct circuit *dest, struct circuit *src) {
    size_t i;

    if(dest->max_wires != src->max_wires){
        return 1;
    }

    if(dest->max_gates != src->max_gates){
        return 1;
    }

    dest->in = src->in;
    dest->out = src->out;

    for(i=0;i<src->in.size;i++){
        if(dest->in.wires[i] != NULL){
            dest->in.wires[i] = src->in.wires[i]-src->wires+dest->wires;
        }
    }
    for(i=0;i<src->out.size;i++){
        if(dest->out.wires[i] != NULL){
            dest->out.wires[i] = src->out.wires[i]-src->wires+dest->wires;
        }
    }

    for(i=dest->wire_count;i<src->wire_count;i++){
        dest->wires[i].out.wires = NULL;
    }

    dest->wire_count = src->wire_count;
    dest->gate_count = src->gate_count;

    for(i=0;i<dest->wire_count;i++){
        struct wire *wire = dest->wires+i;
        struct wire *src_wire = src->wires+i;
        struct wire **ptr;
        size_t n;

        wire->x1 = src_wire->x1;
        wire->y1 = src_wire->y1;
        wire->x2 = src_wire->x2;
        wire->y2 = src_wire->y2;

        wire->middle = src_wire->middle;

        wire->in.type = src_wire->in.type;
        wire->out.type = src_wire->out.type;

        wire->state = src_wire->state;
        wire->evaluated = src_wire->evaluated;

        switch(wire->in.type){
            case T_GATE:
                wire->in.in.gate = src_wire->in.in.gate
                                   -src->gates+dest->gates;

                break;

            case T_IO:
                if(wire->in.in.io == &src->in){
                    wire->in.in.io = &dest->in;
                }else{
                    wire->in.in.io = &dest->out;
                }

                break;

            case T_WIRE:
                wire->in.in.wire = src_wire->in.in.wire
                                   -src->wires+dest->wires;

                break;
        }
        switch(wire->out.type){
            case T_GATE:
                wire->out.out.gate = src_wire->out.out.gate
                                     -src->gates+dest->gates;

                break;

            case T_IO:
                if(src_wire->out.out.io == &src->in){
                    wire->out.out.io = &dest->in;
                }else{
                    wire->out.out.io = &dest->out;
                }

                break;
        }

        if(src_wire->out.wire_count){
            ptr = realloc(wire->out.wires,
                          src_wire->out.wire_count*sizeof(struct wire*));
            if(ptr == NULL){
                return 1;
            }
        }else{
            ptr = NULL;
        }

        wire->out.wires = ptr;
        wire->out.wire_count = src_wire->out.wire_count;

        for(n=0;n<wire->out.wire_count;n++){
            wire->out.wires[n] = src_wire->out.wires[n]-src->wires+dest->wires;
        }
    }

    for(i=0;i<dest->gate_count;i++){
        struct gate *gate = dest->gates+i;
        struct gate *src_gate = src->gates+i;

        *gate = *src_gate;

        if(gate->in1 != NULL) gate->in1 = gate->in1-src->wires+dest->wires;
        if(gate->type != T_BUFFER && gate->in2 != NULL){
            gate->in2 = gate->in2-src->wires+dest->wires;
        }
        if(gate->out != NULL) gate->out = gate->out-src->wires+dest->wires;
    }

    dest->selection = src->selection;

    switch(dest->selection.type){
        case T_GATE:
            dest->selection.ptr.gate = dest->selection.ptr.gate-
                                       src->gates+dest->gates;

            break;

        case T_WIRE:
            dest->selection.ptr.wire = dest->selection.ptr.wire-
                                       src->wires+dest->wires;

            break;

        case T_IO:
            if(dest->selection.ptr.io == &src->in){
                dest->selection.ptr.io = &dest->in;
            }else{
                dest->selection.ptr.io = &dest->out;
            }

            break;
    }

    return 0;
}

int circuit_clone(struct circuit *circuit, struct circuit *src) {
    circuit->gates = malloc(src->max_gates*sizeof(struct gate));
    circuit->wires = malloc(src->max_wires*sizeof(struct wire));

    if(circuit->gates == NULL || circuit->wires == NULL){
        free(circuit->gates);
        circuit->gates = NULL;
        free(circuit->wires);
        circuit->wires = NULL;

        return 1;
    }

    circuit->gate_count = 0;
    circuit->wire_count = 0;

    circuit->max_gates = src->max_gates;
    circuit->max_wires = src->max_wires;

    return circuit_copy(circuit, src);
}

struct gate *circuit_get_gate_at(struct circuit *circuit, int x, int y) {
    size_t i;

    for(i=circuit->gate_count;i--;){
        if(circuit->gates[i].x == x && circuit->gates[i].y == y){
            return circuit->gates+i;
        }
    }

    return NULL;
}

int circuit_add_gate(struct circuit *circuit, int x, int y,
                     unsigned char gate_type) {
    static const struct gate gate_init = {
        NULL,
        NULL,
        NULL,
        0, 0,
        T_NONE,
        1,
        0,
        0
    };

    if(circuit->gate_count >= circuit->max_gates){
        return 1;
    }

    circuit->gates[circuit->gate_count] = gate_init;
    circuit->gates[circuit->gate_count].x = x;
    circuit->gates[circuit->gate_count].y = y;
    circuit->gates[circuit->gate_count].type = gate_type;

    circuit->gate_count++;

    return 0;
}

void circuit_move_gate(struct gate *gate, int x, int y) {
    gate->x = x;
    gate->y = y;

    if(gate->in1 != NULL){
        circuit_move_wire_end(gate->in1, x, y);
    }
    if(gate->type != T_BUFFER && gate->in2 != NULL){
        circuit_move_wire_end(gate->in2, x, y);
    }
    if(gate->out != NULL){
        circuit_move_wire_start(gate->out, x, y);
    }
}

void circuit_detach_gate(struct gate *gate) {
    if(gate->out != NULL){
        gate->out->in.in.gate = NULL;
        gate->out->in.type = T_NONE;
    }
    if(gate->in1 != NULL){
        gate->in1->out.out.gate = NULL;
        gate->in1->out.type = T_NONE;
    }
    if(gate->type != T_BUFFER && gate->in2 != NULL){
        gate->in2->out.out.gate = NULL;
        gate->in2->out.type = T_NONE;
    }
}

void circuit_delete_gate(struct circuit *circuit, struct gate *gate) {
    size_t i;

    circuit_detach_gate(gate);

    i = (size_t)(gate-circuit->gates);
    memmove(gate, gate+1, (circuit->gate_count-i-1)*sizeof(struct gate));

    circuit->gate_count--;

    if(circuit->selection.type == T_GATE &&
       circuit->selection.ptr.gate == gate){
        circuit->selection.type = T_NONE;
    }

    /* Decrement all the pointers that need to be decremented. */
    for(i=0;i<circuit->wire_count;i++){
        struct wire *wire = circuit->wires+i;

        if(wire->in.type == T_GATE && wire->in.in.gate > gate){
            wire->in.in.gate--;
        }
        if(wire->out.type == T_GATE && wire->out.out.gate > gate){
            wire->out.out.gate--;
        }
    }
}

struct wire *circuit_get_wire_at(struct circuit *circuit, int x, int y) {
    size_t i;
    for(i=circuit->wire_count;i--;){
        struct wire *wire = circuit->wires+i;

        if((BETWEEN(x, wire->x1, wire->middle) && y == wire->y1) ||
           (x == wire->middle && BETWEEN(y, wire->y1, wire->y2)) ||
           (BETWEEN(x, wire->middle, wire->x2) && y == wire->y2)){

            return wire;
        }
    }

    return NULL;
}

struct wire *circuit_get_other_wire_at(struct circuit *circuit, int x, int y,
                                       struct wire *exclude) {
    size_t i;
    for(i=circuit->wire_count;i--;){
        struct wire *wire = circuit->wires+i;

        if(((BETWEEN(x, wire->x1, wire->middle) && y == wire->y1) ||
            (x == wire->middle && BETWEEN(y, wire->y1, wire->y2)) ||
            (BETWEEN(x, wire->middle, wire->x2) && y == wire->y2)) &&
           wire != exclude){

            return wire;
        }
    }

    return NULL;
}

int circuit_is_wire_at(struct wire *wire, int x, int y) {
    if((BETWEEN(x, wire->x1, wire->middle) && y == wire->y1) ||
       (x == wire->middle && BETWEEN(y, wire->y1, wire->y2)) ||
       (BETWEEN(x, wire->middle, wire->x2) && y == wire->y2)){

        return 1;
    }

    return 0;
}

struct wire *circuit_add_wire(struct circuit *circuit, int x1, int y1,
                              int x2, int y2) {
    static const struct wire wire_init = {
        0, 0,
        0, 0,
        0,

        {
            T_NONE,
            {NULL},
        },
        {
            T_NONE,
            {NULL},
            NULL,
            0
        },

        0,
        0
    };

    if(circuit->wire_count >= circuit->max_wires){
        return NULL;
    }

    circuit->wires[circuit->wire_count] = wire_init;
    circuit->wires[circuit->wire_count].x1 = x1;
    circuit->wires[circuit->wire_count].y1 = y1;
    circuit->wires[circuit->wire_count].x2 = x2;
    circuit->wires[circuit->wire_count].y2 = y2;
    circuit->wires[circuit->wire_count].middle = (x1+x2)/2;

    circuit->wire_count++;

    return circuit->wires+circuit->wire_count-1;
}

int circuit_connect_wire(struct wire *in_wire, struct wire *wire) {
    struct wire **ptr;

    ptr = realloc(in_wire->out.wires,
                  (in_wire->out.wire_count+1)*sizeof(struct wire*));
    if(ptr == NULL){
        return 1;
    }

    in_wire->out.wires = ptr;
    in_wire->out.wires[in_wire->out.wire_count] = wire;

    wire->in.type = T_WIRE;
    wire->in.in.wire = in_wire;

    in_wire->out.wire_count++;

    return 0;
}

#define ABS(x) ((x) < 0 ? -(x) : (x))

void circuit_move_wire_start(struct wire *wire, int x, int y) {
    size_t i;

    wire->x1 = x;
    wire->y1 = y;

    if(!BETWEEN(wire->middle, wire->x1, wire->x2)){
        wire->middle = (wire->x1+wire->x2)/2;
    }

    for(i=0;i<wire->out.wire_count;i++){
        struct wire *connected = wire->out.wires[i];

        if(!circuit_is_wire_at(wire, connected->x1, connected->y1)){
            /* TODO: Make something better */

            circuit_move_wire_start(connected, wire->middle,
                                    (wire->y1+wire->y2)/2);
        }
    }
}

void circuit_move_wire_end(struct wire *wire, int x, int y) {
    size_t i;

    wire->x2 = x;
    wire->y2 = y;


    if(!BETWEEN(wire->middle, wire->x1, wire->x2)){
        wire->middle = (wire->x1+wire->x2)/2;
    }

    for(i=0;i<wire->out.wire_count;i++){
        struct wire *connected = wire->out.wires[i];

        if(!circuit_is_wire_at(wire, connected->x1, connected->y1)){
            /* TODO: Make something better */

            circuit_move_wire_start(connected, wire->middle,
                                    (wire->y1+wire->y2)/2);
        }
    }
}

void circuit_detach_wire_start(struct wire *wire) {
    switch(wire->in.type){
        case T_GATE:
            if(wire->in.in.gate->out == wire){
                wire->in.in.gate->out = NULL;
            }
            wire->in.in.gate = NULL;

            break;

        case T_IO:
            {
                size_t i;
                struct io *io = wire->in.in.io;

                for(i=0;i<io->size;i++){
                    if(io->wires[i] == wire){
                        io->wires[i] = NULL;
                    }
                }

                wire->in.in.io = NULL;
            }

            break;

        case T_WIRE:
            {
                struct wire *connected = wire->in.in.wire;
                size_t i;

                for(i=0;i<connected->out.wire_count;i++){
                    if(connected->out.wires[i] == wire){
                        memmove(connected->out.wires+i,
                                connected->out.wires+i+1,
                                (connected->out.wire_count-i-1)*
                                sizeof(struct wire*));
                        connected->out.wire_count--;
                        break;
                    }
                }

                wire->in.in.wire = NULL;
            }

            break;
    }

    wire->in.type = T_NONE;
}

void circuit_detach_wire_end(struct wire *wire) {
    switch(wire->out.type){
        case T_GATE:
            {
                struct gate *gate = wire->out.out.gate;

                if(gate->in1 == wire){
                    gate->in1 = NULL;
                }
                if(gate->type != T_BUFFER && gate->in2 == wire){
                    gate->in2 = NULL;
                }

                wire->out.out.gate = NULL;
            }

            break;

        case T_IO:
            {
                size_t i;
                struct io *io = wire->out.out.io;

                for(i=0;i<io->size;i++){
                    if(io->wires[i] == wire){
                        io->wires[i] = NULL;
                    }
                }

                wire->out.out.io = NULL;
            }

            break;
    }

    wire->out.type = T_NONE;
}

static void disconnect_wires(struct wire *wire) {
    size_t i;

    for(i=0;i<wire->out.wire_count;i++){
        wire->out.wires[i]->in.type = T_NONE;
        wire->out.wires[i]->in.in.wire = NULL;
    }

    free(wire->out.wires);
    wire->out.wires = NULL;
    wire->out.wire_count = 0;
}

void circuit_delete_wire(struct circuit *circuit, struct wire *wire) {
    size_t i;

    circuit_detach_wire_start(wire);
    circuit_detach_wire_end(wire);

    disconnect_wires(wire);

    i = (size_t)(wire-circuit->wires);

    memmove(wire, wire+1, (circuit->wire_count-1-i)*sizeof(struct wire));

    circuit->wire_count--;

    if(circuit->selection.type == T_WIRE &&
       circuit->selection.ptr.wire == wire){
        circuit->selection.type = T_NONE;
    }

    /* Decrement all the pointers that need to be decremented. */
    for(i=0;i<circuit->gate_count;i++){
        struct gate *gate = circuit->gates+i;

        if(gate->in1 != NULL && gate->in1 > wire){
            gate->in1--;
        }
        if(gate->type != T_BUFFER && gate->in2 != NULL && gate->in2 > wire){
            gate->in2--;
        }
        if(gate->out != NULL && gate->out > wire){
            gate->out--;
        }
    }

    for(i=0;i<circuit->wire_count;i++){
        struct wire *w = circuit->wires+i;
        size_t n;

        if(w->in.type == T_WIRE && w->in.in.wire > wire){
            w->in.in.wire--;
        }

        for(n=0;n<w->out.wire_count;n++){
            if(w->out.wires[n] > wire){
                w->out.wires[n]--;
            }
        }
    }

    for(i=0;i<circuit->in.size;i++){
        if(circuit->in.wires[i] > wire){
            circuit->in.wires[i]--;
        }
    }
    for(i=0;i<circuit->out.size;i++){
        if(circuit->out.wires[i] > wire){
            circuit->out.wires[i]--;
        }
    }
}

struct io *circuit_get_io_at(struct circuit *circuit, int x, int y) {
    if(x >= circuit->in.x && y >= circuit->in.y &&
       x <= circuit->in.x+IO_WIDTH/TILE_WIDTH &&
       y <= circuit->in.y+(int)io_get_height(&circuit->in)/TILE_HEIGHT){
        return &circuit->in;
    }

    if(x >= circuit->out.x && y >= circuit->out.y &&
       x <= circuit->out.x+IO_WIDTH/TILE_WIDTH &&
       y <= circuit->out.y+(int)io_get_height(&circuit->out)/TILE_HEIGHT){
        return &circuit->out;
    }

    return NULL;
}

int circuit_get_io_pin(struct io *io, int y) {
    int p = y-io->y-1;

    return p >= 0 && p < io->size ? p : -1;
}

void circuit_move_io(struct io *io, int x, int y) {
    io->x = x;
    io->y = y;

    if(io->is_output){
        size_t i;

        for(i=0;i<io->size;i++){
            y++;
            if(io->wires[i] != NULL){
                circuit_move_wire_end(io->wires[i], x, y);
            }
        }
    }else{
        size_t i;

        for(i=0;i<io->size;i++){
            y++;
            if(io->wires[i] != NULL){
                circuit_move_wire_start(io->wires[i],
                                        x+IO_WIDTH/TILE_WIDTH-1, y);
            }
        }
    }
}

void circuit_detach_io(struct io *io) {
    size_t i;

    if(io->is_output){
        for(i=0;i<io->size;i++){
            if(io->wires[i] != NULL){
                io->wires[i]->out.out.io = NULL;
                io->wires[i]->out.type = T_NONE;
                io->wires[i] = NULL;
            }
        }
    }else{
        for(i=0;i<io->size;i++){
            if(io->wires[i] != NULL){
                io->wires[i]->in.in.io = NULL;
                io->wires[i]->in.type = T_NONE;
                io->wires[i] = NULL;
            }
        }
    }
}

static void gate_render(struct gate *gate, struct render_info *r) {
    int x = (gate->type%GATES_PER_ROW)*TILE_WIDTH;
    int y = (gate->type/GATES_PER_ROW)*TILE_HEIGHT;

    int rx = gate->x*TILE_WIDTH*r->scale-r->x;
    int ry = gate->y*TILE_HEIGHT*r->scale-r->y;

    int w = TILE_WIDTH*r->scale;
    int h = TILE_HEIGHT*r->scale;

    gfx_subimage(r->gates, rx, ry, w, h, x, y, TILE_WIDTH, TILE_HEIGHT,
                 (!(r->show_states&gate->evaluated)|
                  !(gate->state^gate->negated))*255,
                 ((!(r->show_states&gate->evaluated))|
                  (gate->state^gate->negated))*127+
                 !(r->show_states&gate->evaluated)*128,
                 !(r->show_states&gate->evaluated)*255, 255);

    if(gate->negated){
        gfx_subimage(r->gates, rx, ry, w, h,
                     (G_NEGATE%GATES_PER_ROW)*TILE_WIDTH,
                     (G_NEGATE/GATES_PER_ROW)*TILE_HEIGHT,
                     TILE_WIDTH, TILE_HEIGHT,
                     (!(r->show_states&gate->evaluated)|!gate->state)*255,
                     ((!(r->show_states&gate->evaluated))|gate->state)*127+
                     !(r->show_states&gate->evaluated)*128,
                     !(r->show_states&gate->evaluated)*255, 255);
    }else if(gate->out != NULL){
        gfx_subimage(r->gates, rx, ry, w, h,
                     (G_GATE_END%GATES_PER_ROW)*TILE_WIDTH,
                     (G_GATE_END/GATES_PER_ROW)*TILE_HEIGHT,
                     TILE_WIDTH, TILE_HEIGHT, 255, 255, 255, 255);
    }
}

static void draw_connection(int x1, int y1, int x2, int y2, int middle,
                            int thickness, unsigned char r, unsigned char g,
                            unsigned char b, unsigned char a) {
    gfx_line(x1, y1, middle, y1, r, g, b, a, thickness);
    gfx_line(middle, y1, middle, y2, r, g, b, a, thickness);
    gfx_line(middle, y2, x2, y2, r, g, b, a, thickness);
}

static void wire_render(struct wire *wire, struct render_info *r) {
    int x1 = wire->x1*TILE_WIDTH*r->scale-r->x;
    int y1 = (wire->y1*TILE_HEIGHT+TILE_WIDTH/2)*r->scale-r->y;

    int x2 = wire->x2*TILE_WIDTH*r->scale-r->x;
    int y2 = wire->y2*TILE_HEIGHT*r->scale-r->y;

    if(wire->in.type == T_WIRE){
        struct wire *connected = wire->in.in.wire;

        if(connected->out.type == T_GATE &&
           connected->out.out.gate->type != T_BUFFER &&
           BETWEEN(wire->x1, connected->middle, connected->x2)){
            if(connected->out.out.gate->in1 == connected){
                y1 -= (TILE_HEIGHT/2-GATE_UPPER_PIN)*r->scale;
            }else if(connected->out.out.gate->in2 == connected){
                y1 -= (TILE_HEIGHT/2-GATE_LOWER_PIN)*r->scale;
            }
        }

        gfx_subimage(r->gates, x1, y1-TILE_HEIGHT/2*r->scale,
                     TILE_WIDTH*r->scale, TILE_HEIGHT*r->scale,
                     (G_WIRE_CROSS%GATES_PER_ROW)*TILE_WIDTH,
                     (G_WIRE_CROSS/GATES_PER_ROW)*TILE_HEIGHT,
                     TILE_WIDTH, TILE_HEIGHT, 255, 255, 255, 255);
        x1 += TILE_WIDTH*r->scale/2;
    }else{
        x1 += TILE_WIDTH*r->scale;
    }

    if(wire->out.type == T_GATE && wire->out.out.gate->type != T_BUFFER){
        if(wire->out.out.gate->in1 == wire){
            y2 += GATE_UPPER_PIN*r->scale;
        }else if(wire->out.out.gate->in2 == wire){
            y2 += GATE_LOWER_PIN*r->scale;
        }
    }else{
        y2 += TILE_HEIGHT/2*r->scale;
        if(wire->out.type == T_WIRE) x2 += TILE_WIDTH/2*r->scale;
    }

    draw_connection(x1, y1, x2, y2,
                    (wire->middle*TILE_WIDTH+TILE_WIDTH/2)*r->scale-r->x,
                    ceil(r->scale),
                    (r->show_states&wire->evaluated&!wire->state)*255,
                    (r->show_states&wire->evaluated&wire->state)*127, 0, 255);
}

static void gate_outline(struct gate *gate, struct render_info *r) {
    int rx = gate->x*TILE_WIDTH*r->scale-r->x;
    int ry = gate->y*TILE_HEIGHT*r->scale-r->y;

    int w = TILE_WIDTH*r->scale;
    int h = TILE_HEIGHT*r->scale;

    gfx_line(rx, ry, rx+w, ry, 127, 127, 127, 255, ceil(r->scale));
    gfx_line(rx+w, ry, rx+w, ry+h, 127, 127, 127, 255, ceil(r->scale));
    gfx_line(rx+w, ry+h, rx, ry+h, 127, 127, 127, 255, ceil(r->scale));
    gfx_line(rx, ry+h, rx, ry, 127, 127, 127, 255, ceil(r->scale));
}

static void wire_outline(struct wire *wire, struct render_info *r) {
    int x1 = wire->x1*TILE_WIDTH*r->scale-r->x;
    int y1 = wire->y1*TILE_HEIGHT*r->scale-r->y;

    int middle1 = wire->middle*TILE_WIDTH*r->scale-r->x;
    int middle2 = (wire->middle+1)*TILE_WIDTH*r->scale-r->x;

    int x2 = (wire->x2+1)*TILE_WIDTH*r->scale-r->x;
    int y2 = (wire->y2+1)*TILE_HEIGHT*r->scale-r->y;

    /* FIXME: It breaks sometimes... */

    if(y2 < y1){
        int t = middle1;

        middle1 = middle2;
        middle2 = t;
    }

    if(x2 < x1){
        int t = middle1;

        middle1 = middle2;
        middle2 = t;
        x2 -= TILE_WIDTH*r->scale;
        x1 += TILE_WIDTH*r->scale;
    }

    gfx_line(x1, y1, middle2, y1, 127, 127, 127, 255, ceil(r->scale));
    gfx_line(middle2, y1, middle2, y2-TILE_HEIGHT*r->scale,
             127, 127, 127, 255, ceil(r->scale));
    gfx_line(middle2, y2-TILE_HEIGHT*r->scale, x2, y2-TILE_HEIGHT*r->scale,
             127, 127, 127, 255, ceil(r->scale));
    gfx_line(x2, y2-TILE_HEIGHT*r->scale, x2, y2,
             127, 127, 127, 255, ceil(r->scale));
    gfx_line(x2, y2, middle1, y2, 127, 127, 127, 255, ceil(r->scale));
    gfx_line(middle1, y2, middle1, y1+TILE_HEIGHT*r->scale,
             127, 127, 127, 255, ceil(r->scale));
    gfx_line(middle1, y1+TILE_HEIGHT*r->scale, x1, y1+TILE_HEIGHT*r->scale,
             127, 127, 127, 255, ceil(r->scale));
    gfx_line(x1, y1+TILE_HEIGHT*r->scale, x1, y1,
             127, 127, 127, 255, ceil(r->scale));
}

void io_outline(struct io *io, struct render_info *r) {
    int x1 = io->x*TILE_WIDTH*r->scale-r->x;
    int y1 = io->y*TILE_WIDTH*r->scale-r->y;
    int x2 = x1+IO_WIDTH*r->scale;
    int y2 = y1+io_get_height(io)*r->scale;

    gfx_line(x1, y1, x2, y1, 127, 127, 127, 255, ceil(r->scale));
    gfx_line(x2, y1, x2, y2, 127, 127, 127, 255, ceil(r->scale));
    gfx_line(x2, y2, x1, y2, 127, 127, 127, 255, ceil(r->scale));
    gfx_line(x1, y2, x1, y1, 127, 127, 127, 255, ceil(r->scale));
}

void circuit_render(struct circuit *circuit, struct render_info *r) {
    size_t i;

    io_render(&circuit->in, r);
    io_render(&circuit->out, r);

    for(i=0;i<circuit->gate_count;i++){
        gate_render(circuit->gates+i, r);
    }

    for(i=0;i<circuit->wire_count;i++){
        wire_render(circuit->wires+i, r);
    }

    switch(circuit->selection.type){
        case T_GATE:
            gate_outline(circuit->selection.ptr.gate, r);

            break;
        case T_WIRE:
            wire_outline(circuit->selection.ptr.wire, r);

            break;
        case T_IO:
            io_outline(circuit->selection.ptr.io, r);

            break;
    }
}

void circuit_get_tile_position(struct render_info *r, int *x, int *y,
                               int sx, int sy) {
    int tw = TILE_WIDTH*r->scale;
    int th = TILE_HEIGHT*r->scale;

    *x = (r->x+sx)/tw-((r->x+sx)%tw < 0 ? 1 : 0);
    *y = (r->y+sy)/th-((r->y+sy)%th < 0 ? 1 : 0);
}

void circuit_select(struct circuit *circuit, struct render_info *r,
                    int mx, int my) {
    int tx, ty;

    void *item;

    circuit_get_tile_position(r, &tx, &ty, mx, my);

    if((item = circuit_get_gate_at(circuit, tx, ty)) != NULL){
        circuit->selection.type = T_GATE;
        circuit->selection.ptr.gate = item;
    }else if((item = circuit_get_wire_at(circuit, tx, ty)) != NULL){
        circuit->selection.type = T_WIRE;
        circuit->selection.ptr.wire = item;
    }else if((item = circuit_get_io_at(circuit, tx, ty)) != NULL){
        circuit->selection.type = T_IO;
        circuit->selection.ptr.io = item;
    }else{
        circuit->selection.type = T_NONE;
    }

    circuit->selection.tx = tx;
    circuit->selection.ty = ty;
}

void circuit_input_updated(struct circuit *circuit) {
    size_t i;

    for(i=0;i<circuit->in.size;i++){
        if(circuit->in.wires[i] != NULL){
            circuit->in.wires[i]->state = (circuit->in.states>>i)&1;
        }
    }
}

void circuit_free(struct circuit *circuit) {
    free(circuit->gates);
    circuit->gates = NULL;
    free(circuit->wires);
    circuit->wires = NULL;

    circuit->gate_count = 0;
    circuit->wire_count = 0;
}
