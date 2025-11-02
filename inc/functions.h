#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <genesis.h>


void initCustomFont();

void C_DrawText(const char* str, u16 x, u16 y, u16 palette);

void C_ClearText(u16 x, u16 y, u16 length);

#endif