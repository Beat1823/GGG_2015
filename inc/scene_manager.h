#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include <genesis.h>
#include "inc/data_load.h"

void sceneManagerInit();
void sceneManagerStart();
void sceneManagerReset();
void sceneManagerUpdate(u16* g_lastJoy);
void sceneManagerDraw();
bool sceneManagerShouldTriggerQuiz();
bool sceneManagerGetTriggeredQuiz(u16* outQuizId);
bool sceneManagerGetQuestionId(u16* outQuestionID);
void sceneManagerContinueAfterQuiz();
bool sceneManagerReachedEnd();

#endif