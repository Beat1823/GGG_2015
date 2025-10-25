#include <genesis.h>
#include "quiz_manager.h"

static Quiz* currentQuiz = NULL;
static u16 selectedCategoryId = 0;  
static u16 currentQuestionIndex = 0;
static u16 wrongAnswerCount = 0;
static const Question* currentQuestions[10]; 
static u16 totalQuestions = 0;
static bool categorySelected = FALSE;
static bool singleQuestionMode = FALSE;

void quizManagerInit() {
    currentQuiz = NULL;
    selectedCategoryId = 0;
    currentQuestionIndex = 0;
    wrongAnswerCount = 0;
    totalQuestions = 0;
    categorySelected = FALSE;
    singleQuestionMode = FALSE;
}

void quizManagerStartQuiz(u16 quizId) {
    currentQuiz = &QUIZZES[quizId];
    currentQuestionIndex = 0;
    wrongAnswerCount = 0;
    categorySelected = FALSE;
    selectedCategoryId = 0;
    singleQuestionMode = FALSE;
}

void quizManagerStartSingleQuestion(u16 questionId) {
    Question* q = &QUESTIONS[questionId];
    if(!q) return;
    
    singleQuestionMode = TRUE;
    categorySelected = TRUE;
    currentQuestionIndex = 0;
    wrongAnswerCount = 0;
    totalQuestions = 1;
    currentQuestions[0] = q;
    
    // Create a temporary quiz def for single question
    currentQuiz = NULL; // We don't need full quiz def for single Q
}

static void loadQuestionsForCurrentQuiz(void)
{
    if (!currentQuiz) return;

    totalQuestions = 0;

    // Use the prebuilt ROM index table for the selected category
    const u16 *idxs = CATEGORY_QUESTION_INDEXES[selectedCategoryId];
    u16 count       = CATEGORY_QUESTION_COUNTS[selectedCategoryId];

    for (u16 k = 0; k < count && totalQuestions < currentQuiz->questionCount; k++)
    {
        currentQuestions[totalQuestions++] = &QUESTIONS[idxs[k]];
    }
}

void quizManagerDrawCategorySelect() {
    if(!currentQuiz) return;
    
    VDP_clearPlane(BG_A, TRUE);
    VDP_drawText(currentQuiz->name, (40 - strlen(currentQuiz->name)) / 2, 6);
    VDP_drawText("Choose Your Trial:", 11, 10);
    
    for(u8 i = 0; i < currentQuiz->categoryCount && i < 3; i++) {
        const char *categoryName = CATEGORY_NAMES[currentQuiz->categories[i]];
        VDP_drawText(categoryName, 14, 14 + i * 2);
    }
}

bool quizManagerUpdateCategorySelect(u16* g_lastJoy) {
    u16 joy = JOY_readJoypad(JOY_1);
    
    if(!currentQuiz) return FALSE;
    
    // Check for category selection add a delay
    for(u8 i = 0; i < currentQuiz->categoryCount && i < 3; i++) {
        u16 button = (i == 0) ? BUTTON_A : (i == 1) ? BUTTON_B : BUTTON_C;
        
        if((joy & button) && !(*g_lastJoy & button)) {
            selectedCategoryId = currentQuiz->categories[i]; 
            categorySelected = TRUE;
            
            // Build question list for this category
            loadQuestionsForCurrentQuiz();
            
            *g_lastJoy = joy;
            return TRUE;
        }
    }
    
    *g_lastJoy = joy;
    return FALSE;
}

void quizManagerDraw() {
    if(currentQuestionIndex >= totalQuestions) return;
    
    VDP_clearPlane(BG_A, TRUE);
    
    Question* q = &QUESTIONS[currentQuestionIndex];
    
    // Draw stats (only if not single question mode)
    if(!singleQuestionMode && currentQuiz) {
        char buf[64];
        sprintf(buf, "Q%d/%d  Wrong:%d/%d", 
                currentQuestionIndex + 1, 
                currentQuiz->questionCount,
                wrongAnswerCount, 
                currentQuiz->wrongLimit);
        VDP_drawText(buf, 2, 2);
    } else {
        VDP_drawText("Answer the riddle:", 10, 2);
    }
    
    // Draw question
    VDP_drawText(q->question, 2, 8);
    
    // Draw answers
    char buf[80];
    sprintf(buf, "A: %s", q->answerA);
    VDP_drawText(buf, 4, 14);
    
    sprintf(buf, "B: %s", q->answerB);
    VDP_drawText(buf, 4, 16);
    
    sprintf(buf, "C: %s", q->answerC);
    VDP_drawText(buf, 4, 18);
}

QuizResult quizManagerUpdate(u16* g_lastJoy) {
    u16 joy = JOY_readJoypad(JOY_1);
    
    if(currentQuestionIndex >= totalQuestions) {
        return QUIZ_IN_PROGRESS;
    }
    
    Question* q = &QUESTIONS[currentQuestionIndex];
    char answer = '\0';
    
    // Check for answer
    if((joy & BUTTON_A) && !(*g_lastJoy & BUTTON_A)) {
        answer = 'a';
    } else if((joy & BUTTON_B) && !(*g_lastJoy & BUTTON_B)) {
        answer = 'b';
    } else if((joy & BUTTON_C) && !(*g_lastJoy & BUTTON_C)) {
        answer = 'c';
    }
    
    if(answer != '\0') {
        // Check if correct
        if(answer != q->correct) {
            wrongAnswerCount++;
            
            // For single question mode, any wrong answer = failure
            u8 limit = singleQuestionMode ? 1 : (currentQuiz ? currentQuiz->wrongLimit : 1);
            
            if(wrongAnswerCount >= limit) {
                *g_lastJoy = joy;
                return QUIZ_FAILED;
            }
        }
        
        currentQuestionIndex++;
        
        // Check if quiz complete
        u8 totalNeeded = singleQuestionMode ? 1 : (currentQuiz ? currentQuiz->questionCount : 0);
        
        if(currentQuestionIndex >= totalNeeded) {
            *g_lastJoy = joy;
            return QUIZ_PASSED;
        }
        
        // Next question
        quizManagerDraw();
    }
    
    *g_lastJoy = joy;
    return QUIZ_IN_PROGRESS;
}