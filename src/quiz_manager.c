#include <genesis.h>
#include "functions.h"
#include "quiz_manager.h"

// Quiz state
static const Quiz* g_currentQuiz = NULL;
static u16 g_selectedCategoryId = 0;
static u16 g_currentQuestionIndex = 0;
static u16 g_wrongAnswerCount = 0;
static const Question* g_questionList[20];  // Questions for current quiz
static u16 g_totalQuestions = 0;
static bool g_categorySelected = FALSE;
static bool g_singleQuestionMode = FALSE;

void quizManagerInit() {
    g_currentQuiz = NULL;
    g_selectedCategoryId = 0;
    g_currentQuestionIndex = 0;
    g_wrongAnswerCount = 0;
    g_totalQuestions = 0;
    g_categorySelected = FALSE;
    g_singleQuestionMode = FALSE;
}

void quizManagerStartQuiz(u16 quizId) {
    if(quizId >= QUIZZES_COUNT) return;
    
    g_currentQuiz = &QUIZZES[quizId];
    g_currentQuestionIndex = 0;
    g_wrongAnswerCount = 0;
    g_categorySelected = FALSE;
    g_singleQuestionMode = FALSE;
}

void quizManagerStartSingleQuestion(u16 questionId) {
    if(questionId >= QUESTIONS_COUNT) return;
    
    g_singleQuestionMode = TRUE;
    g_categorySelected = TRUE;
    g_currentQuestionIndex = 0;
    g_wrongAnswerCount = 0;
    g_totalQuestions = 1;
    g_questionList[0] = &QUESTIONS[questionId];
    g_currentQuiz = NULL;  // No full quiz in single question mode
}

static void loadQuestionsForCategory() {
    if(!g_currentQuiz) return;
    
    g_totalQuestions = 0;
    
    // Use prebuilt ROM index table for selected category
    const u16* questionIndices = CATEGORY_QUESTION_INDEXES[g_selectedCategoryId];
    u16 availableCount = CATEGORY_QUESTION_COUNTS[g_selectedCategoryId];
    u16 needed = g_currentQuiz->questionCount;
    
    // Load up to 'needed' questions from this category
    for(u16 i = 0; i < availableCount && g_totalQuestions < needed; i++) {
        g_questionList[g_totalQuestions++] = &QUESTIONS[questionIndices[i]];
    }
}

void quizManagerDrawCategorySelect() {
    if(!g_currentQuiz) return;
    
    VDP_clearPlane(BG_A, TRUE);
    
    // Draw quiz name centered
    u16 nameLen = strlen(g_currentQuiz->name);
    u16 xPos = (40 - nameLen) / 2;
    C_DrawText(g_currentQuiz->name, xPos, 4, PAL0);
    
    C_DrawText("Choose Your Trial:", 11, 8, PAL0);
    
    // Draw category options (up to 3 for A/B/C buttons)
    for(u8 i = 0; i < g_currentQuiz->categoryCount && i < 3; i++) {
        u16 catId = g_currentQuiz->categories[i];
        const char* catName = CATEGORY_NAMES[catId];
        
        char buf[64];
        char buttonLabel = 'A' + i;
        sprintf(buf, "%c: %s", buttonLabel, catName);
        C_DrawText(buf, 14, 12 + i * 2, PAL0);
    }
}

bool quizManagerUpdateCategorySelect(u16* lastJoy) {
    u16 joy = JOY_readJoypad(JOY_1);
    
    if(!g_currentQuiz) {
        *lastJoy = joy;
        return FALSE;
    }
    
    // Check for category selection (A/B/C buttons)
    for(u8 i = 0; i < g_currentQuiz->categoryCount && i < 3; i++) {
        u16 button = (i == 0) ? BUTTON_A : (i == 1) ? BUTTON_B : BUTTON_C;
        
        if((joy & button) && !(*lastJoy & button)) {
            // Category selected!
            g_selectedCategoryId = g_currentQuiz->categories[i];
            g_categorySelected = TRUE;
            
            // Load questions for this category
            loadQuestionsForCategory();
            
            *lastJoy = joy;
            return TRUE;  // Signal category was selected
        }
    }
    
    *lastJoy = joy;
    return FALSE;
}

void quizManagerDraw() {
    if(g_currentQuestionIndex >= g_totalQuestions) return;
    
    VDP_clearPlane(BG_A, TRUE);
    
    const Question* q = g_questionList[g_currentQuestionIndex];  // FIX: Use question list!
    
    // Draw stats (only in full quiz mode)
    if(!g_singleQuestionMode && g_currentQuiz) {
        char buf[64];
        sprintf(buf, "Q%d/%d  Wrong:%d/%d", 
                g_currentQuestionIndex + 1, 
                g_currentQuiz->questionCount,
                g_wrongAnswerCount, 
                g_currentQuiz->wrongLimit);
        C_DrawText(buf, 2, 0, PAL0);
    } else {
        C_DrawText("Answer the riddle:", 10, 0, PAL0);
    }
    
    // Draw question text
    C_DrawText(q->question, 2, 6, PAL0);
    
    // Draw answer choices
    char buf[80];
    sprintf(buf, "A: %s", q->answerA);
    C_DrawText(buf, 4, 12, PAL0);
    
    sprintf(buf, "B: %s", q->answerB);
    C_DrawText(buf, 4, 14, PAL0);
    
    sprintf(buf, "C: %s", q->answerC);
    C_DrawText(buf, 4, 16, PAL0);
}

QuizResult quizManagerUpdate(u16* lastJoy) {
    u16 joy = JOY_readJoypad(JOY_1);
    
    if(g_currentQuestionIndex >= g_totalQuestions) {
        *lastJoy = joy;
        return QUIZ_IN_PROGRESS;
    }
    
    const Question* q = g_questionList[g_currentQuestionIndex];  // FIX: Use question list!
    
    // Check for answer button press (A/B/C)
    u8 answerIdx = 255;  // Invalid
    
    if((joy & BUTTON_A) && !(*lastJoy & BUTTON_A)) {
        answerIdx = 0;  // FIX: Use numeric index!
    } else if((joy & BUTTON_B) && !(*lastJoy & BUTTON_B)) {
        answerIdx = 1;
    } else if((joy & BUTTON_C) && !(*lastJoy & BUTTON_C)) {
        answerIdx = 2;
    }
    
    // If an answer was given
    if(answerIdx != 255) {
        // Check if correct
        if(answerIdx != q->correct) {
            g_wrongAnswerCount++;
            
            // Check fail condition
            u8 limit = g_singleQuestionMode ? 1 : g_currentQuiz->wrongLimit;
            if(g_wrongAnswerCount >= limit) {
                *lastJoy = joy;
                return QUIZ_FAILED;
            }
        }
        
        // Move to next question
        g_currentQuestionIndex++;
        
        // Check if quiz complete
        u16 totalNeeded = g_singleQuestionMode ? 1 : g_currentQuiz->questionCount;
        if(g_currentQuestionIndex >= totalNeeded) {
            *lastJoy = joy;
            return QUIZ_PASSED;
        }
        
        // Draw next question
        quizManagerDraw();
    }
    
    *lastJoy = joy;
    return QUIZ_IN_PROGRESS;
}