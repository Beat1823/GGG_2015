#pragma once
#include <genesis.h>

// Scene types
typedef enum {
  SCENE_TYPE_NORMAL = 0,
  SCENE_TYPE_QUIZ_TRIGGER = 1,
  SCENE_TYPE_GOOD_ENDING = 2,
  SCENE_TYPE_BAD_ENDING = 3
} SceneType;

// Question
typedef struct {
  u16 id;                // numeric handle; the generator will index these for you
  u16 category_id;       // mapped from "history", "geography", "math", etc.
  const char *question;  // ROM string
  char answerA[20];
  char answerB[20];
  char answerC[20];// up to 4 choices; unused set to ""
  u8  correct;           // 0..3 (a,b,c,d)
} Question;

// Scene
typedef struct {
  u16 id;                 // derived from scene_id order
  SceneType type;
  const char *text;       // ROM string (pipes -> newlines)
  s16 nextScene;         // -1 if none, else index into scenes[]
  s16 triggerQuiz;       // -1 if none, else index into quizzes[]
  s16 questionId;        // -1 if none, else index into questions[]
  u8  bg;                 // as-is
  u8  music;              // as-is
} Scene;

// Quiz
typedef struct {
  u16 id;                 // index
  const char *name;       // ROM
  u8  wrongLimit;
  u16 questionCount;    // requested count (limit)
  const u16 *categories;  // ROM array of category ids
  u16 categoryCount;     // number of categories in the array
} Quiz;

// Category names (ROM)
extern const char * const CATEGORY_NAMES[];
extern const u16 CATEGORY_COUNT;

// For each category i, an array of question indices (ROM)
extern const u16 * const CATEGORY_QUESTION_INDEXES[]; // pointers to arrays
extern const u16        CATEGORY_QUESTION_COUNTS[];  