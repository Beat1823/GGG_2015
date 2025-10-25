#include <genesis.h>
#include "scene_manager.h"

static Scene* currentScene = NULL;
static bool waitingForInput = FALSE;
static bool shouldTriggerQuiz = FALSE;
static bool reachedEnd = FALSE;

void sceneManagerInit() {
    currentScene = NULL;
    waitingForInput = FALSE;
    shouldTriggerQuiz = FALSE;
    reachedEnd = FALSE;
}

void sceneManagerStart() {
    currentScene = &SCENES[0];
    waitingForInput = FALSE;
    shouldTriggerQuiz = FALSE;
    reachedEnd = FALSE;
    sceneManagerDraw();
}

void sceneManagerReset() {
    sceneManagerInit();
}

void sceneManagerDraw() {
    if(!currentScene) return;
    
    VDP_clearPlane(BG_A, TRUE);
    
    // Draw the scene text
    const char* text = currentScene->text;
    u16 y = 22;
    u16 x = 2;
    u16 lineLen = 0;
    
    for(u16 i = 0; text[i] != '\0'; i++) {
        if(text[i] == '\n') {
            y++;
            x = 2;
            lineLen = 0;
        } else {
            char str[2] = {text[i], '\0'};
            VDP_drawText(str, x++, y);
            lineLen++;
            if(lineLen >= 36) {
                y++;
                x = 2;
                lineLen = 0;
            }
        }
    }
    
    VDP_drawText("Press A to continue...", 8, 27);
    waitingForInput = TRUE;
}

void sceneManagerUpdate(u16* g_lastJoy) {
    u16 joy = JOY_readJoypad(JOY_1);
    
    if(!waitingForInput) {
        sceneManagerDraw();
        return;
    }
    
    if((joy & BUTTON_A) && !(*g_lastJoy & BUTTON_A)) {
        // Check scene type
        switch(currentScene->type) {
            case SCENE_TYPE_NORMAL:
                // Move to next scene
                currentScene = &SCENES[currentScene->nextScene];
                if(currentScene) {
                    waitingForInput = FALSE;
                } else {
                    reachedEnd = TRUE;
                }
                break;
                
            case SCENE_TYPE_QUIZ_TRIGGER:
                // Trigger quiz
                shouldTriggerQuiz = TRUE;
                waitingForInput = FALSE;
                break;
                
            case SCENE_TYPE_GOOD_ENDING:
            case SCENE_TYPE_BAD_ENDING:
                reachedEnd = TRUE;
                break;
        }
    }
    
    *g_lastJoy = joy;
}

bool sceneManagerShouldTriggerQuiz() {
    return shouldTriggerQuiz;
}

bool sceneManagerGetTriggeredQuiz(u16* outQuizId) {
    if(currentScene && currentScene->type == SCENE_TYPE_QUIZ_TRIGGER) {
       if (currentScene->triggerQuiz >= 0 && currentScene->triggerQuiz < QUIZZES_COUNT)
        {
            *outQuizId = currentScene->triggerQuiz;
            return true;
        }
    }
    return false;
}

bool sceneManagerGetQuestionId(u16* outQuestionID) {
    if(currentScene && currentScene->type == SCENE_TYPE_QUIZ_TRIGGER) {
        if (currentScene->questionId >= 0 && currentScene->questionId < QUESTIONS_COUNT)
        {
            *outQuestionID = currentScene->questionId;
            return true;
        }
    }
    return false;
}

void sceneManagerContinueAfterQuiz() {
    shouldTriggerQuiz = FALSE;

    if (currentScene &&
        currentScene->nextScene >= 0 &&
        currentScene->nextScene < SCENES_COUNT)
    {
        currentScene = &SCENES[currentScene->nextScene];
        reachedEnd = FALSE;
    }
    else
    {
        // No next scene → we’re at an ending
        reachedEnd = TRUE;
    }

    waitingForInput = FALSE;
}

bool sceneManagerReachedEnd() {
    return reachedEnd;
}