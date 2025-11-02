#include <genesis.h>
#include "scene_manager.h"

static const Scene* g_currentScene = NULL;
static bool g_waitingForInput = FALSE;
static bool g_shouldTriggerQuiz = FALSE;
static bool g_reachedEnd = FALSE;

// Typewriter effect state
static u16 g_textCharIndex = 0;
static u16 g_textTimer = 0;
static u16 g_lastDrawnIndex = 0;  // Track what we've already drawn
#define TEXT_DELAY 1  // Frames between characters (lower = faster)

void sceneManagerInit() {
    g_currentScene = NULL;
    g_waitingForInput = FALSE;
    g_shouldTriggerQuiz = FALSE;
    g_reachedEnd = FALSE;
    g_textCharIndex = 0;
    g_textTimer = 0;
    g_lastDrawnIndex = 0;
}

void sceneManagerStart() {
    g_currentScene = &SCENES[0];  
    g_waitingForInput = FALSE;
    g_shouldTriggerQuiz = FALSE;
    g_reachedEnd = FALSE;
    g_textCharIndex = 0;
    g_textTimer = 0;
    g_lastDrawnIndex = 0;
    sceneManagerDraw();
}

void sceneManagerReset() {
    sceneManagerInit();
}

void sceneManagerDraw() {
    if(!g_currentScene) return;
    
    VDP_clearPlane(BG_A, TRUE);
    

    g_textCharIndex = 0;
    g_textTimer = 0;
    g_lastDrawnIndex = 0;
    g_waitingForInput = FALSE;
}


static void getTextPosition(const char* text, u16 charIndex, u16* outX, u16* outY) {
    u16 y = 22;
    u16 x = 2;
    u16 lineLen = 0;
    const u16 maxLineLen = 36;
    
    for(u16 i = 0; i < charIndex && text[i] != '\0'; i++) {
        if(text[i] == '\n') {
            y++;
            x = 2;
            lineLen = 0;
        } else {
            x++;
            lineLen++;
            
            if(lineLen >= maxLineLen) {
                y++;
                x = 2;
                lineLen = 0;
            }
        }
    }
    
    *outX = x;
    *outY = y;
}


static void updateTypewriter() {
    if(!g_currentScene) return;
    
    const char* text = g_currentScene->text;
    u16 textLen = strlen(text);
    

    if(g_textCharIndex >= textLen) {
        if(!g_waitingForInput) {
            VDP_drawText("Continue...", 8, 5);
            g_waitingForInput = TRUE;
        }
        return;
    }
    

    g_textTimer++;
    if(g_textTimer < TEXT_DELAY) {
        return;  
    }
    g_textTimer = 0;
    

    for(u16 i = g_lastDrawnIndex; i <= g_textCharIndex && i < textLen; i++) {
        if(text[i] != '\n') {
            u16 x, y;
            getTextPosition(text, i, &x, &y);
            char str[2] = {text[i], '\0'};
            VDP_drawText(str, x, y);
        }
    }
    
    g_lastDrawnIndex = g_textCharIndex + 1;
    g_textCharIndex++;
}

void sceneManagerUpdate(u16* lastJoy, SceneType nextScenePath) {
    u16 joy = JOY_readJoypad(JOY_1);
    
    updateTypewriter();
    
    if((joy & BUTTON_A) && !g_waitingForInput) {
        const char* text = g_currentScene->text;
        u16 textLen = strlen(text);
        if(g_textCharIndex < textLen) {
            for(u16 i = g_lastDrawnIndex; i < textLen; i++) {
                if(text[i] != '\n') {
                    u16 x, y;
                    getTextPosition(text, i, &x, &y);
                    char str[2] = {text[i], '\0'};
                    VDP_drawText(str, x, y);
                }
            }
            g_textCharIndex = textLen;
            g_lastDrawnIndex = textLen;
            VDP_drawText("Continue...", 8, 5);
            g_waitingForInput = TRUE;
        }
    }
    
    if(!g_waitingForInput) {
        *lastJoy = joy;
        return;
    }

    if(((joy & BUTTON_A) && !(*lastJoy & BUTTON_A)) || ((joy & BUTTON_B) && !(*lastJoy & BUTTON_B)) || ((joy & BUTTON_C) && !(*lastJoy & BUTTON_C))) {
        switch(g_currentScene->type) {
            case SCENE_TYPE_NORMAL:
                if(nextScenePath == SCENE_A && g_currentScene->nextSceneA >= 0) {
                    g_currentScene = &SCENES[g_currentScene->nextSceneA];
                    sceneManagerDraw();  
                } else if(nextScenePath == SCENE_B && g_currentScene->nextSceneB >= 0) {
                    g_currentScene = &SCENES[g_currentScene->nextSceneB];
                    sceneManagerDraw(); 
                } else {
                    g_reachedEnd = TRUE;
                }
                break;
                
            case SCENE_TYPE_QUIZ_TRIGGER:
                g_shouldTriggerQuiz = TRUE;
                g_waitingForInput = FALSE;
                break;
                
            case SCENE_TYPE_GOOD_ENDING:
            case SCENE_TYPE_BAD_ENDING:
                g_shouldTriggerQuiz = FALSE;
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

void sceneManagerContinueAfterQuiz(SceneType nextScenePath) {
    g_shouldTriggerQuiz = FALSE;
    
    // FIX: Move to next scene after quiz based on the path
    if(g_currentScene) {
        if(nextScenePath == SCENE_A && g_currentScene->nextSceneA >= 0 && 
           g_currentScene->nextSceneA < SCENES_COUNT) {
            g_currentScene = &SCENES[g_currentScene->nextSceneA];
            g_reachedEnd = FALSE;
        } else if(nextScenePath == SCENE_B && g_currentScene->nextSceneB >= 0 && 
                  g_currentScene->nextSceneB < SCENES_COUNT) {
            g_currentScene = &SCENES[g_currentScene->nextSceneB];
            g_reachedEnd = FALSE;
        } else {
            // Invalid next scene
            g_reachedEnd = TRUE;
            return;
        }
    } else {
        g_reachedEnd = TRUE;
        return;
    }
    
    // Reset typewriter for new scene
    g_textCharIndex = 0;
    g_textTimer = 0;
    g_lastDrawnIndex = 0;
    g_waitingForInput = FALSE;
}

bool sceneManagerReachedEnd() {
    return g_reachedEnd;
}

SceneType sceneManagerGetEndingType() {
    return g_currentScene ? g_currentScene->type : SCENE_TYPE_BAD_ENDING;
}