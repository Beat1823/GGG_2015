#ifndef QUIZ_MANAGER_H
#define QUIZ_MANAGER_H

#include <genesis.h>
#include "inc/data_load.h"

typedef enum {
    QUIZ_IN_PROGRESS,
    QUIZ_PASSED,
    QUIZ_FAILED
} QuizResult;

void quizManagerInit();
void quizManagerStartQuiz(u16 quizId);
void quizManagerStartSingleQuestion(u16 questionId);
void quizManagerDrawCategorySelect();
bool quizManagerUpdateCategorySelect(u16* g_lastJoy);
void quizManagerDraw();
QuizResult quizManagerUpdate(u16* g_lastJoy);

#endif