#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include <genesis.h>
#include "data_load.h"

void sceneManagerInit();
void sceneManagerStart();
void sceneManagerReset();
void sceneManagerUpdate(u16* lastJoy);
void sceneManagerDraw();

bool sceneManagerShouldTriggerQuiz();
bool sceneManagerGetTriggeredQuiz(u16* outQuizId);
bool sceneManagerGetQuestionId(u16* outQuestionId);
void sceneManagerContinueAfterQuiz();

bool sceneManagerReachedEnd();
SceneType sceneManagerGetEndingType();  // NEW - return ending type

#endif