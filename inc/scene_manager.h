#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include <genesis.h>
#include "data_load.h"

// Game state machine
typedef enum {
    SCENE_A = 0,
    SCENE_B = 1,
    SCENE_END = 2
} NextScene;

void sceneManagerInit();
void sceneManagerStart();
void sceneManagerReset();
void sceneManagerUpdate(u16* lastJoy, SceneType NextScene);
void sceneManagerDraw();

bool sceneManagerShouldTriggerQuiz();
bool sceneManagerGetTriggeredQuiz(u16* outQuizId);
bool sceneManagerGetQuestionId(u16* outQuestionId);
void sceneManagerContinueAfterQuiz(SceneType NextScene);

u8 sceneManagerGetCurrentBGId();
bool sceneManagerReachedEnd();
SceneType sceneManagerGetEndingType();

#endif