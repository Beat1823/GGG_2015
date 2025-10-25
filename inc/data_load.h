#ifndef DATA_LOAD_H
#define DATA_LOAD_H

#include <genesis.h>
#include "data_types.h"

// Categories registry
extern const char * const CATEGORY_NAMES[];
extern const u16 CATEGORY_COUNT;

// Main tables
extern const Question * const QUESTIONS;
extern const Scene    * const SCENES;
extern const Quiz     * const QUIZZES;

extern const u16 QUESTIONS_COUNT;
extern const u16 SCENES_COUNT;
extern const u16 QUIZZES_COUNT;

#endif