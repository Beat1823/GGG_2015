#include <genesis.h>
#include "data_load.h"
#include "scene_manager.h"
#include "quiz_manager.h"

// Game state machine
typedef enum {
    STATE_TITLE,
    STATE_SCENE,           // Renamed from VISUAL_NOVEL for clarity
    STATE_CATEGORY_SELECT,
    STATE_QUIZ,
    STATE_BAD_ENDING,
    STATE_GOOD_ENDING
} GameState;

static GameState g_currentState = STATE_TITLE;
static u16 g_lastJoy = 0;

// Forward declarations
static void handleTitleState();
static void handleSceneState();
static void handleCategorySelectState();
static void handleQuizState();
static void handleEndingState();
static void drawTitle();
static void drawEnding(bool isGood);

int main() {
    // Initialize hardware
    JOY_init();
    VDP_setBackgroundColor(1);
    PAL_setColor(0, RGB24_TO_VDPCOLOR(0x000020));
    PAL_setColor(1, RGB24_TO_VDPCOLOR(0xFFFFFF));
    PAL_setColor(15, RGB24_TO_VDPCOLOR(0xFF0000));
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    
    // Initialize game systems
    sceneManagerInit();
    quizManagerInit();
    
    // Start at title
    drawTitle();
    
    // Main game loop
    while(1) {
        switch(g_currentState) {
            case STATE_TITLE:
                handleTitleState();
                break;
                
            case STATE_SCENE:
                handleSceneState();
                break;
                
            case STATE_CATEGORY_SELECT:
                handleCategorySelectState();
                break;
                
            case STATE_QUIZ:
                handleQuizState();
                break;
                
            case STATE_BAD_ENDING:
            case STATE_GOOD_ENDING:
                handleEndingState();
                break;
        }
        
        SYS_doVBlankProcess();
    }
    
    return 0;
}

static void handleTitleState() {
    u16 joy = JOY_readJoypad(JOY_1);
    
    if((joy & BUTTON_START) && !(g_lastJoy & BUTTON_START)) {
        g_currentState = STATE_SCENE;
        sceneManagerReset();
        sceneManagerStart();
        VDP_clearPlane(BG_A, TRUE);
        sceneManagerDraw();
    }
    
    g_lastJoy = joy;
}

static void handleSceneState() {
    sceneManagerUpdate(&g_lastJoy);
    
    // Check if we need to trigger a quiz
    if(sceneManagerShouldTriggerQuiz()) {
        u16 questionId;
        u16 quizId;
        
        // Single question mode?
        if(sceneManagerGetQuestionId(&questionId)) {
            quizManagerStartSingleQuestion(questionId);
            g_currentState = STATE_QUIZ;
            VDP_clearPlane(BG_A, TRUE);
            quizManagerDraw();
        }
        // Full quiz mode?
        else if(sceneManagerGetTriggeredQuiz(&quizId)) {
            quizManagerStartQuiz(quizId);
            g_currentState = STATE_CATEGORY_SELECT;
            VDP_clearPlane(BG_A, TRUE);
            quizManagerDrawCategorySelect();
        }
    }
    
    // Check if scene reached an ending
    if(sceneManagerReachedEnd()) {
        SceneType endType = sceneManagerGetEndingType();
        g_currentState = (endType == SCENE_TYPE_GOOD_ENDING) ? 
                         STATE_GOOD_ENDING : STATE_BAD_ENDING;
        VDP_clearPlane(BG_A, TRUE);
        drawEnding(endType == SCENE_TYPE_GOOD_ENDING);
    }
}

static void handleCategorySelectState() {
    if(quizManagerUpdateCategorySelect(&g_lastJoy)) {
        g_currentState = STATE_QUIZ;
        VDP_clearPlane(BG_A, TRUE);
        quizManagerDraw();
    }
}

static void handleQuizState() {
    QuizResult result = quizManagerUpdate(&g_lastJoy);
    
    switch(result) {
        case QUIZ_FAILED:
            g_currentState = STATE_BAD_ENDING;
            VDP_clearPlane(BG_A, TRUE);
            drawEnding(FALSE);
            break;
            
        case QUIZ_PASSED:
            g_currentState = STATE_SCENE;
            sceneManagerContinueAfterQuiz();
            VDP_clearPlane(BG_A, TRUE);
            sceneManagerDraw();
            break;
            
        case QUIZ_IN_PROGRESS:
            // Continue waiting for input
            break;
    }
}

static void handleEndingState() {
    u16 joy = JOY_readJoypad(JOY_1);
    
    if((joy & BUTTON_START) && !(g_lastJoy & BUTTON_START)) {
        g_currentState = STATE_TITLE;
        sceneManagerReset();
        VDP_clearPlane(BG_A, TRUE);
        drawTitle();
    }
    
    g_lastJoy = joy;
}

static void drawTitle() {
    VDP_clearPlane(BG_A, TRUE);
    VDP_drawText("HORROR QUIZ", 14, 8);
    VDP_drawText("A Game of Life and Death", 8, 12);
    VDP_drawText("Press START", 14, 20);
}

static void drawEnding(bool isGood) {
    if(isGood) {
        VDP_drawText("YOU SURVIVED!", 13, 12);
        VDP_drawText("...This time.", 13, 14);
    } else {
        VDP_drawText("YOU FAILED!", 14, 12);
        VDP_drawText("The darkness claims you...", 7, 14);
    }
    VDP_drawText("Press START to retry", 10, 20);
}