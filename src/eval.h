#ifndef EVAL_H
#define EVAL_H

#include "circuit.h"

/* TODO: Make this more efficient. */

struct eval {
    struct gate **instr;
    struct wire **to_visit;
    size_t instr_count;

    size_t pc;
};

int eval_init(struct eval *eval, struct circuit *circuit);
void eval_reset(struct eval *eval, struct circuit *circuit);
void eval_step(struct eval *eval);
void eval_run_once(struct eval *eval);
void eval_run_multiple(struct eval *eval, size_t count);
void eval_input_updated(struct eval *eval, struct circuit *circuit);
void eval_free(struct eval *eval);

#endif
