#ifndef EXAMPLES_H
#define EXAMPLES_H

#include <ignis/ignis.h>
#include <minimal/minimal.h>

#include "math/math.h"

uint8_t initIgnis();
void printVersionInfo();

int onEventDefault(MinimalApp* app, const MinimalEvent* e);

// ---------------| CUBE |-------------------------------
MinimalApp example_cube();

// ---------------| GJK |--------------------------------
MinimalApp example_gjk();

#endif // !EXAMPLES_H
