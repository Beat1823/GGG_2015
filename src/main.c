#include <genesis.h>
#include "resources.h"
#include "functions.h"
#include "data_load.h"
#include "scene_manager.h"
#include "quiz_manager.h"

#define TO_INT(x)  ((x) >> 8)

// Game state machine
typedef enum {
    STATE_TITLE,
    STATE_SCENE,
    STATE_CATEGORY_SELECT,
    STATE_QUIZ,
    STATE_BAD_ENDING,
    STATE_GOOD_ENDING
} GameState;

typedef enum {
    CLEAR_BG = 0,
    QUIZ_BG = 1,
    RED_BG = 2,
    GREEN_BG = 3
} BgState;

static GameState g_currentState = STATE_TITLE;
static u16 g_lastJoy = 0;
static u16 g_baseTile = 0;

static s16 g_scrollX = 0;
static s16 g_scrollY = 0;
static NextScene g_nextScenePath = SCENE_A;  // Track which path to take
static s16 g_scrollSpeedX = FIX16(2.1);
static s16 g_scrollSpeedY = FIX16(2.1);

// Forward declarations
static void handleTitleState();
static void handleSceneState();
static void handleCategorySelectState();
static void handleQuizState();
static void handleEndingState();
static void drawTitle();
static void drawQuizBackground();
static void drawSceneBackground();
static void drawSceneBackgroundId(u8 inId, u16 x, u16 y, u16 palette);
static void scrollBackground();
static void drawEnding(bool isGood);

int main() {
    // Initialize hardware
    JOY_init();
    VDP_setBackgroundColor(0);
    
    PAL_setColor(0,RGB24_TO_VDPCOLOR(0x000000));

    PAL_setPalette(PAL1, skullBgTile.palette->data, DMA);
    PAL_setPalette(PAL3, redBg.palette->data, DMA);
    PAL_setPalette(PAL2, greenBg.palette->data, DMA);
    

    g_baseTile = TILE_USER_INDEX;
    VDP_loadTileSet(skullBgTile.tileset, g_baseTile, DMA);
    VDP_loadTileSet(redBg.tileset, g_baseTile + 100, DMA);
    VDP_loadTileSet(greenBg.tileset, g_baseTile + 200, DMA);
    
    initCustomFont();
    
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
                handleEndingState();
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
        g_nextScenePath = SCENE_A;  // Start on normal path
        sceneManagerReset();
        sceneManagerStart();
        VDP_clearPlane(BG_A, TRUE);
        VDP_clearPlane(BG_B, TRUE);
        sceneManagerDraw();
    }
    
    g_lastJoy = joy;
}

static void handleSceneState() {
    // FIX: Pass the current path to scene manager
    sceneManagerUpdate(&g_lastJoy, g_nextScenePath);
    
    // Check if we need to trigger a quiz
    if(sceneManagerShouldTriggerQuiz()) {
        u16 questionId;
        u16 quizId;
        
        // Single question mode?
        if(sceneManagerGetQuestionId(&questionId)) {
            quizManagerStartSingleQuestion(questionId);
            g_currentState = STATE_QUIZ;
            VDP_clearPlane(BG_A, TRUE);
            VDP_clearPlane(BG_B, TRUE);
            XGM_startPlay(&quizMusic_01);
            drawQuizBackground();
            quizManagerDraw();
        }
        // Full quiz mode?
        else if(sceneManagerGetTriggeredQuiz(&quizId)) {
            quizManagerStartQuiz(quizId);
            g_currentState = STATE_CATEGORY_SELECT;
            VDP_clearPlane(BG_A, TRUE);
            VDP_clearPlane(BG_B, TRUE);
            quizManagerDrawCategorySelect();
        }
    }
    else if(sceneManagerReachedEnd()) {
        SceneType endType = sceneManagerGetEndingType();
        g_currentState = (endType == SCENE_TYPE_GOOD_ENDING) ? 
                         STATE_GOOD_ENDING : STATE_BAD_ENDING;
        VDP_clearPlane(BG_A, TRUE);
        VDP_clearPlane(BG_B, TRUE);
        drawEnding(endType == SCENE_TYPE_GOOD_ENDING);
    }
    else{
        drawSceneBackground();
    }
}

static void handleCategorySelectState() {
    if(quizManagerUpdateCategorySelect(&g_lastJoy)) {
        g_currentState = STATE_QUIZ;
        VDP_clearPlane(BG_A, TRUE);
        VDP_clearPlane(BG_B, TRUE);
        XGM_startPlay(&quizMusic_01);
        drawQuizBackground();
        quizManagerDraw();
    }
}

static void handleQuizState() {
    QuizResult result = quizManagerUpdate(&g_lastJoy);

    switch(result) {
        case QUIZ_FAILED:
            XGM_pausePlay();
            g_nextScenePath = SCENE_B;
            sceneManagerContinueAfterQuiz(g_nextScenePath);
            g_currentState = STATE_SCENE;
            VDP_clearPlane(BG_A, TRUE);
            VDP_clearPlane(BG_B, TRUE);
            XGM_startPlay(&bgMusic_01);
            sceneManagerDraw();
            break;
            
        case QUIZ_PASSED:
            g_nextScenePath = SCENE_A;
            sceneManagerContinueAfterQuiz(g_nextScenePath);
            g_currentState = STATE_SCENE;
            VDP_clearPlane(BG_A, TRUE);
            VDP_clearPlane(BG_B, TRUE);
            XGM_startPlay(&bgMusic_01);
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
        g_nextScenePath = SCENE_A;  // Reset path
        sceneManagerReset();
        VDP_clearPlane(BG_A, TRUE);
        VDP_clearPlane(BG_B, TRUE);
        drawTitle();
    }
    
    g_lastJoy = joy;
}

static void drawTitle() {
    VDP_clearPlane(BG_A, TRUE);
    C_DrawText("Knowing", 14, 6, PAL0);
    C_DrawText("Press Start", 14, 18, PAL0);
}

static void drawEnding(bool isGood) {
    if(isGood) {
        C_DrawText("The End", 13, 10, PAL0);
    } else {
        C_DrawText("Bad End", 14, 10, PAL0);
    }
    C_DrawText("Press Start", 10, 18, PAL0);
}

static void drawQuizBackground(){
    for(u16 planeY = 0; planeY < 64; planeY++) {
        for(u16 planeX = 0; planeX < 64; planeX++) {
            u16 patternX = planeX % 8;
            u16 patternY = planeY % 8;
            u16 tileIdx = g_baseTile + (patternY * 8) + patternX;
            
            VDP_setTileMapXY(BG_B, TILE_ATTR_FULL(PAL1, 0, 0, 0, tileIdx),
                            planeX, planeY);
        }
    }
}

static void drawSceneBackground(){
    switch (sceneManagerGetCurrentBGId()) {
        case 2:
            drawSceneBackgroundId(100, 8, 8, PAL2);
            break;
        case 3:
            drawSceneBackgroundId(300, 8, 8, PAL3);
            break;
        case 1:
        case 0:
        default:
            break;
    }
}
static void drawSceneBackgroundId(u8 inId, u16 x, u16 y, u16 palette)
{

        u16 topLeftTile = g_baseTile + inId;
        u16 topRightTile = g_baseTile + inId + 1;
        u16 bottomLeftTile = topLeftTile + 16; 
        u16 bottomRightTile = topRightTile + 16;
        
        VDP_setTileMapXY(BG_B, TILE_ATTR_FULL(palette, 0, 0, 0, topLeftTile), x, y);
        VDP_setTileMapXY(BG_B, TILE_ATTR_FULL(palette, 0, 0, 0, topRightTile), x + 1, y);
        VDP_setTileMapXY(BG_B, TILE_ATTR_FULL(palette, 0, 0, 0, bottomLeftTile), x, y + 1);
        VDP_setTileMapXY(BG_B, TILE_ATTR_FULL(palette, 0, 0, 0, bottomRightTile), x + 1, y + 1);
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
