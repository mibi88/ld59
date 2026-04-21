#ifndef CHECKS_H
#define CHECKS_H

#include "eval.h"
#include "circuit.h"

int not_check(struct circuit *circuit, struct eval *eval);
int and_check(struct circuit *circuit, struct eval *eval);
int add_check(struct circuit *circuit, struct eval *eval);
int flipflop_check(struct circuit *circuit, struct eval *eval);
int mux_check(struct circuit *circuit, struct eval *eval);

#endif
