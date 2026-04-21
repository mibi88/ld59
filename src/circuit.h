#ifndef CIRCUIT_H
#define CIRCUIT_H

#include <stddef.h>

#include "gates.h"

#define TILE_WIDTH  16
#define TILE_HEIGHT 16
#define IO_WIDTH    (2+4)*TILE_WIDTH
#define IO_PIN_SIZE 4

struct render_info {
    int x, y;
    float scale;
    struct image *font;
    struct image *gates;

    unsigned int show_states : 1;
};

struct gate;
struct wire;

enum {
    T_AND,
    T_OR,
    T_XOR,
    T_BUFFER
};

struct gate {
    struct wire *in1;
    struct wire *in2; /* Unused for buffers */
    struct wire *out;
    int x, y;
    unsigned int type : 2;
    unsigned int state : 1;
    unsigned int negated : 1;
    unsigned int evaluated : 1;
};

enum {
    T_NONE,
    T_GATE,
    T_IO,
    T_WIRE
};

struct wire {
    int x1, y1;
    int x2, y2;

    int middle;

    struct {
        unsigned char type;
        union {
            struct gate *gate;
            struct io *io;
            struct wire *wire;
        } in;
    } in;
    struct {
        unsigned char type;
        union {
            struct gate *gate;
            struct io *io;
        } out;
        struct wire **wires;
        size_t wire_count;
    } out;

    unsigned int state : 1;
    unsigned int evaluated : 1;
};

enum {
    T_INPUT,
    T_OUTPUT
};

struct io {
    unsigned long int states;
    unsigned long int evaluated;
    struct wire *wires[32];
    char *labels[32];
    int x, y;
    unsigned int size : 5;
    unsigned int is_output : 1;
};

struct circuit {
    struct io in;
    struct io out;

    struct gate *gates;
    struct wire *wires;

    size_t gate_count;
    size_t wire_count;

    size_t max_gates;
    size_t max_wires;

    struct {
        union {
            struct gate *gate;
            struct wire *wire;
            struct io *io;
        } ptr;

        int tx, ty;

        unsigned char type;
    } selection;
};

int circuit_init(struct circuit *circuit,
                 char **input_labels, unsigned char input_wires,
                 char **output_labels, unsigned char output_wires,
                 size_t max_gates, size_t max_wires);
int circuit_copy(struct circuit *dest, struct circuit *src);
int circuit_clone(struct circuit *circuit, struct circuit *src);

struct gate *circuit_get_gate_at(struct circuit *circuit, int x, int y);
int circuit_add_gate(struct circuit *circuit, int x, int y,
                     unsigned char gate_type);
void circuit_move_gate(struct gate *gate, int x, int y);
void circuit_detach_gate(struct gate *gate);
void circuit_delete_gate(struct circuit *circuit, struct gate *gate);

struct wire *circuit_get_wire_at(struct circuit *circuit, int x, int y);
struct wire *circuit_get_other_wire_at(struct circuit *circuit, int x, int y,
                                       struct wire *exclude);
int circuit_is_wire_at(struct wire *wire, int x, int y);
struct wire *circuit_add_wire(struct circuit *circuit, int x1, int y1,
                              int x2, int y2);
int circuit_connect_wire(struct wire *in_wire, struct wire *wire);
void circuit_move_wire_start(struct wire *wire, int x, int y);
void circuit_move_wire_end(struct wire *wire, int x, int y);
void circuit_detach_wire_start(struct wire *wire);
void circuit_detach_wire_end(struct wire *wire);
void circuit_delete_wire(struct circuit *circuit, struct wire *wire);

struct io *circuit_get_io_at(struct circuit *circuit, int x, int y);
int circuit_get_io_pin(struct io *io, int y);
void circuit_move_io(struct io *io, int x, int y);
void circuit_detach_io(struct io *io);

void circuit_render(struct circuit *circuit, struct render_info *r);

void circuit_get_tile_position(struct render_info *r, int *x, int *y,
                               int sx, int sy);
void circuit_select(struct circuit *circuit, struct render_info *r,
                    int mx, int my);
void circuit_input_updated(struct circuit *circuit);

void circuit_free(struct circuit *circuit);

#endif
