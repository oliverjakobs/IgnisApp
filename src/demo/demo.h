#ifndef DEMO_H
#define DEMO_H

#include "nuklear/nuklear.h"

#include <time.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum theme { THEME_BLACK, THEME_WHITE, THEME_RED, THEME_BLUE, THEME_DARK };

int overview(struct nk_context* ctx);
void calculator(struct nk_context* ctx);
int node_editor(struct nk_context* ctx);

void set_style(struct nk_context* ctx, enum theme theme);

#endif // !DEMO_H
