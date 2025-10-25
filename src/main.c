#include <genesis.h>
#include "data_load.h"
#include "scene_manager.h"
#include "quiz_manager.h"

typedef enum {
    STATE_TITLE,
    STATE_VISUAL_NOVEL,
    STATE_QUIZ,
    STATE_CATEGORY_SELECT,
    STATE_BAD_END,
    STATE_GOOD_END
} GameState;

GameState currentState = STATE_TITLE;
static u16 g_lastJoy = 0;

void drawTitle();
void updateTitle();
void drawBadEnd();
void drawGoodEnd();

int main() {
    JOY_init();
    VDP_setBackgroundColor(1);
    
    PAL_setColor(0, RGB24_TO_VDPCOLOR(0x000020));
    PAL_setColor(1, RGB24_TO_VDPCOLOR(0xFFFFFF));
    PAL_setColor(15, RGB24_TO_VDPCOLOR(0xFF0000));
    
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    
    sceneManagerInit();
    quizManagerInit();
    drawTitle();
    
    while(1) {
        switch(currentState) {
            case STATE_TITLE:
                updateTitle();
                break;
                
            case STATE_VISUAL_NOVEL:
                sceneManagerUpdate(g_lastJoy);
                
                if(sceneManagerShouldTriggerQuiz()) {
                    u16 questionId; 
                    if(sceneManagerGetQuestionId(&questionId)) {
                        // Single question mode
                        quizManagerStartSingleQuestion(questionId);
                        currentState = STATE_QUIZ;
                        VDP_clearPlane(BG_A, TRUE);
                        quizManagerDraw();
                    } else {
                        // Normal quiz mode
                        u16 quizId;
                        if(sceneManagerGetTriggeredQuiz(&quizId)) {
                            quizManagerStartQuiz(quizId);
                            currentState = STATE_CATEGORY_SELECT;
                            VDP_clearPlane(BG_A, TRUE);
                            quizManagerDrawCategorySelect();
                        }
                    }
                }
                
                if(sceneManagerReachedEnd()) {
                    currentState = STATE_GOOD_END;
                    VDP_clearPlane(BG_A, TRUE);
                    drawGoodEnd();
                }
                break;
                
            case STATE_CATEGORY_SELECT:
                if(quizManagerUpdateCategorySelect(g_lastJoy)) {
                    currentState = STATE_QUIZ;
                    VDP_clearPlane(BG_A, TRUE);
                    quizManagerDraw();
                }
                break;
                
            case STATE_QUIZ:
                {
                    QuizResult result = quizManagerUpdate(g_lastJoy);
                    
                    if(result == QUIZ_FAILED) {
                        currentState = STATE_BAD_END;
                        VDP_clearPlane(BG_A, TRUE);
                        drawBadEnd();
                    } else if(result == QUIZ_PASSED) {
                        currentState = STATE_VISUAL_NOVEL;
                        sceneManagerContinueAfterQuiz();
                        VDP_clearPlane(BG_A, TRUE);
                        sceneManagerDraw();
                    }
                }
                break;
                
            case STATE_BAD_END:
                {
                    u16 joy = JOY_readJoypad(JOY_1);
                    if((joy & BUTTON_START) && !(g_lastJoy & BUTTON_START)) {
                        currentState = STATE_TITLE;
                        sceneManagerReset();
                        VDP_clearPlane(BG_A, TRUE);
                        drawTitle();
                    }
                    g_lastJoy = joy;
                }
                break;
                
            case STATE_GOOD_END:
                {
                    u16 joy = JOY_readJoypad(JOY_1);
                    if((joy & BUTTON_START) && !(g_lastJoy & BUTTON_START)) {
                        currentState = STATE_TITLE;
                        sceneManagerReset();
                        VDP_clearPlane(BG_A, TRUE);
                        drawTitle();
                    }
                    g_lastJoy = joy;
                }
                break;
        }
        
        SYS_doVBlankProcess();
    }
    
    return 0;
}

void drawTitle() {
    VDP_clearPlane(BG_A, TRUE);
    VDP_drawText("HORROR QUIZ", 14, 8);
    VDP_drawText("A Game of Life and Death", 8, 12);
    VDP_drawText("Press START", 14, 20);
}

void updateTitle() {
    u16 joy = JOY_readJoypad(JOY_1);
    
    if((joy & BUTTON_START) && !(g_lastJoy & BUTTON_START)) {
        currentState = STATE_VISUAL_NOVEL;
        sceneManagerReset();
        sceneManagerStart();
        VDP_clearPlane(BG_A, TRUE);
        sceneManagerDraw();
    }
    
    g_lastJoy = joy;
}

void drawBadEnd() {
    VDP_drawText("YOU FAILED!", 14, 12);
    VDP_drawText("The darkness claims you...", 7, 14);
    VDP_drawText("Press START to retry", 10, 20);
}

void drawGoodEnd() {
    VDP_drawText("YOU SURVIVED!", 13, 12);
    VDP_drawText("...This time.", 13, 14);
    VDP_drawText("Press START", 14, 20);
}