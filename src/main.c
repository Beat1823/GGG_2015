#include <genesis.h>
#include "resources.h"
#include "data_load.h"
#include "scene_manager.h"
#include "quiz_manager.h"

#define TO_INT(x)  ((x) >> 8)

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
u16 g_baseTile = 0;
static s16 g_scrollX = 0;
static s16 g_scrollY = 0;
static NextScene g_NextScene = SCENE_A;
static s16 g_scrollSpeedX = FIX16(2.1);
static s16 g_scrollSpeedY = FIX16(2.1);


// Forward declarations
static void handleTitleState();
static void handleSceneState();
static void handleCategorySelectState();
static void handleQuizState();
static void handleEndingState();
static void drawTitle();
static void drawBackground();
static void scrollBackground();
static void drawEnding(bool isGood);

int main() {
    // Initialize hardware
    JOY_init();
    VDP_setBackgroundColor(1);
    
    PAL_setColor(0, RGB24_TO_VDPCOLOR(0xFF8000));
    PAL_setColor(1, RGB24_TO_VDPCOLOR(0x000000));
    PAL_setColor(15, RGB24_TO_VDPCOLOR(0xFF0000));
    PAL_setPalette(PAL1, skullBgTile.palette->data, DMA);

    g_baseTile = TILE_USER_INDEX;
    VDP_loadTileSet(skullBgTile.tileset, g_baseTile, DMA);
    
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);

    XGM_setLoopNumber(-1);
    // Initialize game systems
    sceneManagerInit();
    quizManagerInit();
    
    // Start at title
    drawTitle();
    
    // Main game loop
    while(1) {
        switch(g_currentState) {
            case STATE_TITLE:
                XGM_startPlay(&bgMusic_01);
                handleTitleState();
                break;
                
            case STATE_SCENE:
                handleSceneState();
                break;
                
            case STATE_CATEGORY_SELECT:
                XGM_pausePlay();
                handleCategorySelectState();
                break;
                
            case STATE_QUIZ:
                handleQuizState();
                break;
                
            case STATE_BAD_ENDING:
                XGM_pausePlay();
                handleSceneState();
                break;
            case STATE_GOOD_ENDING:
                XGM_pausePlay();
                handleEndingState();
                break;
        }
        SYS_doVBlankProcess();
        scrollBackground();
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
    sceneManagerUpdate(&g_lastJoy, g_NextScene);
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
        XGM_startPlay(&quizMusic_01);
        drawBackground();
        quizManagerDraw();
    }
}

static void handleQuizState() {
    QuizResult result = quizManagerUpdate(&g_lastJoy);

    switch(result) {
        case QUIZ_FAILED:
            XGM_pausePlay();
            g_currentState = STATE_SCENE;
            g_NextScene = SCENE_B;
            VDP_clearPlane(BG_A, TRUE);
            VDP_clearPlane(BG_B, TRUE);
            break;
            
        case QUIZ_PASSED:
            g_currentState = STATE_SCENE;
            g_NextScene = SCENE_A;
            sceneManagerContinueAfterQuiz(g_NextScene);
            VDP_clearPlane(BG_A, TRUE);
            VDP_clearPlane(BG_B, TRUE);
            sceneManagerDraw();
            XGM_startPlay(&bgMusic_01);
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

static void drawBackground(){
    for(u16 planeY = 0; planeY < 64; planeY++) {
        for(u16 planeX = 0; planeX < 64; planeX++) {
            // Calculate which tile within the 8x8 pattern to use
            u16 patternX = planeX % 8;
            u16 patternY = planeY % 8;
            u16 tileIdx = g_baseTile + (patternY * 8) + patternX;
            
            VDP_setTileMapXY(BG_B, TILE_ATTR_FULL(PAL1, 0, 0, 0, tileIdx),
                            planeX, planeY);
        }
    }
}
static void scrollBackground()
{
    if (g_currentState == STATE_QUIZ)
    {
        g_scrollX += g_scrollSpeedX;
        g_scrollY += g_scrollSpeedY;
        VDP_setHorizontalScrollVSync(BG_B, -TO_INT(g_scrollX));
        VDP_setVerticalScrollVSync(BG_B, TO_INT(g_scrollY));
    }
}