#include "eval.h"
#include <stdlib.h>

int eval_init(struct eval *eval, struct circuit *circuit) {
    size_t i;

    struct gate **visited;
    size_t visited_count;

    struct wire **to_visit;
    size_t wires_left;

    struct gate **instr;
    size_t instr_count;

    visited = malloc(circuit->gate_count*sizeof(struct gate*));
    to_visit = malloc(circuit->wire_count*sizeof(struct wire*));
    instr = malloc(circuit->gate_count*sizeof(struct gate*));

    if(visited == NULL || to_visit == NULL || instr == NULL){
        free(visited);
        free(to_visit);
        free(instr);

        return 1;
    }

    for(i=0;i<circuit->gate_count;i++){
        circuit->gates[i].state = 0;
        circuit->gates[i].evaluated = 0;
    }

    for(i=0;i<circuit->wire_count;i++){
        circuit->wires[i].state = 0;
        circuit->gates[i].evaluated = 0;
    }

    visited_count = 0;
    wires_left = 0;
    instr_count = 0;
    for(i=0;i<circuit->out.size;i++){
        struct wire *wire = circuit->out.wires[i];
        size_t visited_base = visited_count;

        size_t n;

        if(wire == NULL) continue;

        do{
            wire->evaluated = 1;

            if(wire->in.type == T_GATE && !wire->in.in.gate->evaluated){
                struct gate *gate = wire->in.in.gate;

                if(gate->in1 != NULL) to_visit[wires_left++] = gate->in1;
                if(gate->type != T_BUFFER && gate->in2 != NULL){
                    to_visit[wires_left++] = gate->in2;
                }
                if(gate->out != NULL) to_visit[wires_left++] = gate->out;

                gate->evaluated = 1;
                visited[visited_count++] = gate;
            }else if(wire->in.type == T_WIRE){
                to_visit[wires_left++] = wire->in.in.wire;
            }

            if(!wires_left) break;

            wire = to_visit[--wires_left];
        }while(1);

        for(n=visited_count;n-- > visited_base;){
            instr[instr_count++] = visited[n];
        }
    }

    for(i=0;i<circuit->in.size;i++){
        if(circuit->in.wires[i] != NULL){
            circuit->in.wires[i]->state = (circuit->in.states>>i)&1;
        }
    }

    free(visited);

    eval->instr = instr;
    eval->to_visit = to_visit;
    eval->instr_count = instr_count;

    eval->pc = 0;

    eval_input_updated(eval, circuit);

    return 0;
}

static void spread_across_wires(struct eval *eval, struct wire *wire) {
    size_t wires_left = 0;

    do{
        size_t i;

        for(i=0;i<wire->out.wire_count;i++){
            struct wire *to_visit = wire->out.wires[i];

            to_visit->state = wire->state;

            eval->to_visit[wires_left++] = to_visit;
        }

        if(wire->out.type == T_IO){
            size_t n;
            struct io *out = wire->out.out.io;

            for(n=0;n<out->size;n++){
                if(out->wires[n] == wire){
                    out->states &= ~(1<<n);
                    out->states |= wire->state<<n;

                    out->evaluated |= 1<<n;
                }
            }
        }

        if(!wires_left) break;

        wire = eval->to_visit[--wires_left];
    }while(1);
}

void eval_reset(struct eval *eval, struct circuit *circuit) {
    size_t i;

    for(i=0;i<circuit->gate_count;i++){
        circuit->gates[i].state = 0;
    }
    for(i=0;i<circuit->wire_count;i++){
        circuit->wires[i].state = 0;
    }
    circuit->out.evaluated = 0;

    circuit_input_updated(circuit);
    eval_input_updated(eval, circuit);

    eval->pc = 0;
}

void eval_step(struct eval *eval) {
    struct gate *gate;

    if(!eval->instr_count) return;

    gate = eval->instr[eval->pc];

    if(gate->type == T_BUFFER){
        if(gate->in1 != NULL) gate->state = gate->in1->state;
    }else{
        if(gate->in1 != NULL && gate->in2 != NULL){
            switch(gate->type){
                case T_AND:
                    gate->state = gate->in1->state&gate->in2->state;

                    break;

                case T_OR:
                    gate->state = gate->in1->state|gate->in2->state;

                    break;

                case T_XOR:
                    gate->state = gate->in1->state^gate->in2->state;

                    break;
            }
        }
    }

    if(gate->negated) gate->state = !gate->state;

    if(gate->out != NULL){
        gate->out->state = gate->state;
        spread_across_wires(eval, gate->out);
    }

    eval->pc++;
    eval->pc %= eval->instr_count;
}

void eval_run_once(struct eval *eval) {
    eval->pc = 0;

    do{
        eval_step(eval);
    }while(eval->pc);
}

void eval_run_multiple(struct eval *eval, size_t count) {
    size_t i;

    for(i=0;i<count;i++){
        eval_step(eval);
    }
}

void eval_input_updated(struct eval *eval, struct circuit *circuit) {
    size_t i;

    for(i=0;i<circuit->in.size;i++){
        if(circuit->in.wires[i] != NULL){
            spread_across_wires(eval, circuit->in.wires[i]);
        }
    }
}

void eval_free(struct eval *eval) {
    free(eval->instr);
    eval->instr = NULL;
    eval->instr_count = 0;
    eval->pc = 0;
}
