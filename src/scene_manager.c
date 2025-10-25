#include <genesis.h>
#include "scene_manager.h"

static const Scene* g_currentScene = NULL;
static bool g_waitingForInput = FALSE;
static bool g_shouldTriggerQuiz = FALSE;
static bool g_reachedEnd = FALSE;

void sceneManagerInit() {
    g_currentScene = NULL;
    g_waitingForInput = FALSE;
    g_shouldTriggerQuiz = FALSE;
    g_reachedEnd = FALSE;
}

void sceneManagerStart() {
    g_currentScene = &SCENES[0];  // Start at first scene
    g_waitingForInput = FALSE;
    g_shouldTriggerQuiz = FALSE;
    g_reachedEnd = FALSE;
    sceneManagerDraw();
}

void sceneManagerReset() {
    sceneManagerInit();
}

void sceneManagerDraw() {
    if(!g_currentScene) return;
    
    VDP_clearPlane(BG_A, TRUE);
    
    // Draw text with word wrapping
    const char* text = g_currentScene->text;
    u16 y = 22;
    u16 x = 2;
    u16 lineLen = 0;
    const u16 maxLineLen = 36;
    
    for(u16 i = 0; text[i] != '\0'; i++) {
        if(text[i] == '\n') {
            y++;
            x = 2;
            lineLen = 0;
        } else {
            char str[2] = {text[i], '\0'};
            VDP_drawText(str, x++, y);
            lineLen++;
            
            if(lineLen >= maxLineLen) {
                y++;
                x = 2;
                lineLen = 0;
            }
        }
    }
    
    VDP_drawText("Press A to continue...", 8, 27);
    g_waitingForInput = TRUE;
}

void sceneManagerUpdate(u16* lastJoy) {
    u16 joy = JOY_readJoypad(JOY_1);
    
    // If not waiting for input, show the scene
    if(!g_waitingForInput) {
        sceneManagerDraw();
        *lastJoy = joy;
        return;
    }
    
    // Check for A button press (with debounce)
    if((joy & BUTTON_A) && !(*lastJoy & BUTTON_A)) {
        switch(g_currentScene->type) {
            case SCENE_TYPE_NORMAL:
                // Move to next scene
                if(g_currentScene->nextScene >= 0 && 
                   g_currentScene->nextScene < SCENES_COUNT) {
                    g_currentScene = &SCENES[g_currentScene->nextScene];
                    g_waitingForInput = FALSE;
                } else {
                    g_reachedEnd = TRUE;
                }
                break;
                
            case SCENE_TYPE_QUIZ_TRIGGER:
                // Signal that we need to show a quiz
                g_shouldTriggerQuiz = TRUE;
                g_waitingForInput = FALSE;
                break;
                
            case SCENE_TYPE_GOOD_ENDING:
            case SCENE_TYPE_BAD_ENDING:
                g_reachedEnd = TRUE;
                break;
        }
    }
    
    *lastJoy = joy;
}

bool sceneManagerShouldTriggerQuiz() {
    return g_shouldTriggerQuiz;
}

bool sceneManagerGetTriggeredQuiz(u16* outQuizId) {
    if(!g_currentScene || g_currentScene->type != SCENE_TYPE_QUIZ_TRIGGER) {
        return FALSE;
    }
    
    if(g_currentScene->triggerQuiz >= 0 && 
       g_currentScene->triggerQuiz < QUIZZES_COUNT) {
        *outQuizId = g_currentScene->triggerQuiz;
        return TRUE;
    }
    
    return FALSE;
}

bool sceneManagerGetQuestionId(u16* outQuestionId) {
    if(!g_currentScene || g_currentScene->type != SCENE_TYPE_QUIZ_TRIGGER) {
        return FALSE;
    }
    
    if(g_currentScene->questionId >= 0 && 
       g_currentScene->questionId < QUESTIONS_COUNT) {
        *outQuestionId = g_currentScene->questionId;
        return TRUE;
    }
    
    return FALSE;
}

void sceneManagerContinueAfterQuiz() {
    g_shouldTriggerQuiz = FALSE;
    
    // Move to next scene after quiz
    if(g_currentScene && 
       g_currentScene->nextScene >= 0 && 
       g_currentScene->nextScene < SCENES_COUNT) {
        g_currentScene = &SCENES[g_currentScene->nextScene];
        g_reachedEnd = FALSE;
    } else {
        // No next scene, we're at an ending
        g_reachedEnd = TRUE;
    }
    
    g_waitingForInput = FALSE;
}

bool sceneManagerReachedEnd() {
    return g_reachedEnd;
}

SceneType sceneManagerGetEndingType() {
    return g_currentScene ? g_currentScene->type : SCENE_TYPE_GOOD_ENDING;
}