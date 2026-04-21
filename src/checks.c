#include "checks.h"

static int test(struct circuit *circuit, struct eval *eval,
                unsigned long states, unsigned long expected,
                unsigned long expected_eval) {
    circuit->in.states = states;

    circuit_input_updated(circuit);
    eval_input_updated(eval, circuit);

    eval_reset(eval, circuit);
    eval_run_once(eval);

    return (circuit->out.states&expected_eval) == expected &&
           circuit->out.evaluated == expected_eval;
}

static int test_nr(struct circuit *circuit, struct eval *eval,
                   unsigned long states, unsigned long expected,
                   unsigned long expected_eval) {
    circuit->in.states = states;

    circuit_input_updated(circuit);
    eval_input_updated(eval, circuit);

    eval_run_once(eval);

    return (circuit->out.states&expected_eval) == expected &&
           circuit->out.evaluated == expected_eval;
}

int not_check(struct circuit *circuit, struct eval *eval) {
    return test(circuit, eval, 1, 0, 1) && test(circuit, eval, 0, 1, 1);
}

int and_check(struct circuit *circuit, struct eval *eval) {
    return test(circuit, eval, 0, 0, 1) && test(circuit, eval, 1, 0, 1) &&
           test(circuit, eval, 2, 0, 1) && test(circuit, eval, 3, 1, 1);
}

int add_check(struct circuit *circuit, struct eval *eval) {
    return test(circuit, eval, 0, 0, 3) && test(circuit, eval, 1, 2, 3) &&
           test(circuit, eval, 2, 2, 3) && test(circuit, eval, 3, 1, 3);
}

int flipflop_check(struct circuit *circuit, struct eval *eval) {
    int r;

    eval_reset(eval, circuit);
    r = test_nr(circuit, eval, 2, 0, 1);
    r &= test_nr(circuit, eval, 0, 0, 1);
    r &= test_nr(circuit, eval, 1, 1, 1);
    r &= test_nr(circuit, eval, 0, 1, 1);
    r &= test_nr(circuit, eval, 2, 0, 1);

    return r;
}

int mux_check(struct circuit *circuit, struct eval *eval) {
    return test(circuit, eval, 0, 0, 1) && test(circuit, eval, 1, 1, 1) &&
           test(circuit, eval, 5, 0, 1) && test(circuit, eval, 4, 0, 1) &&
           test(circuit, eval, 3, 1, 1) && test(circuit, eval, 2, 0, 1) &&
           test(circuit, eval, 6, 1, 1) && test(circuit, eval, 7, 1, 1);
}
