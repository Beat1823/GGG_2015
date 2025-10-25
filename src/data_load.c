#include <genesis.h>
#include "data_types.h"
#include "data_load.h"

// ---- Categories ----
const char * const CATEGORY_NAMES[] = {
  "geography",
  "history",
  "math",
};
const u16 CATEGORY_COUNT = 3;

// ---- Questions ----
static const Question QUESTIONS_DATA[] = {
  {
    .id = 0,
    .category_id = 1,
    .question = "When did World War II end?",
    .answerA = "1943",
    .answerB = "1945",
    .answerC = "1947",
    .correct = 1,
  },
  {
    .id = 1,
    .category_id = 0,
    .question = "Which is the largest ocean?",
    .answerA = "Atlantic",
    .answerB = "Pacific",
    .answerC = "Indian",
    .correct = 1,
  },
  {
    .id = 2,
    .category_id = 2,
    .question = "What is 5 * 6?",
    .answerA = "25",
    .answerB = "30",
    .answerC = "35",
    .correct = 1,
  },
};
const Question * const QUESTIONS = QUESTIONS_DATA;
const u16 QUESTIONS_COUNT = 3;

// ---- Scenes ----
static const Scene SCENES_DATA[] = {
  {
    .id = 0,
    .type = SCENE_TYPE_NORMAL,
    .text = "Welcome to the adventure!\nPress START to continue",
    .nextScene = 1,
    .triggerQuiz = -1,
    .questionId = -1,
    .bg = 0, .music = 0,
  },
  {
    .id = 1,
    .type = SCENE_TYPE_QUIZ_TRIGGER,
    .text = "You encounter a wise old man.\nHe asks you a question...",
    .nextScene = 2,
    .triggerQuiz = 0,
    .questionId = -1,
    .bg = 1, .music = 1,
  },
  {
    .id = 2,
    .type = SCENE_TYPE_GOOD_ENDING,
    .text = "You have completed the challenge!\nCongratulations!",
    .nextScene = -1,
    .triggerQuiz = -1,
    .questionId = -1,
    .bg = 2, .music = 2,
  },
};
const Scene * const SCENES = SCENES_DATA;
const u16 SCENES_COUNT = 3;

// ---- Quizzes ----
static const u16 _QUIZ_CATS_0[] = { 1, 0, 2 };
static const Quiz QUIZZES_DATA[] = {
  {
    .id = 0,
    .name = "General Knowledge",
    .wrongLimit = 2,
    .questionCount = 3,
    .categories = _QUIZ_CATS_0,
    .categoryCount = (u16)(sizeof(_QUIZ_CATS_0)/sizeof(_QUIZ_CATS_0[0])),
  },
};
const Quiz * const QUIZZES = QUIZZES_DATA;
const u16 QUIZZES_COUNT = 1;

// ---- Category Question Indexes ----
static const u16 _CAT_QIDX_0[] = { 1 };
static const u16 _CAT_QIDX_1[] = { 0 };
static const u16 _CAT_QIDX_2[] = { 2 };
const u16 * const CATEGORY_QUESTION_INDEXES[] = {
  _CAT_QIDX_0,
  _CAT_QIDX_1,
  _CAT_QIDX_2,
};
const u16 CATEGORY_QUESTION_COUNTS[] = {
  1,
  1,
  1,
};
