#pragma once
#include <genesis.h>

// Scene types
typedef enum {
  SCENE_TYPE_NORMAL = 0,
  SCENE_TYPE_QUIZ_TRIGGER = 1,
  SCENE_TYPE_GOOD_ENDING = 2,
  SCENE_TYPE_BAD_ENDING = 3
} SceneType;

// Question
typedef struct {
  u16 id;                
  u16 category_id;       
  const char *question;  
  char answerA[20];
  char answerB[20];
  char answerC[20];
  u8  correct; 
} Question;

// Scene
typedef struct {
  u16 id;                 
  SceneType type;
  const char *text;       
  s16 nextSceneA;
  s16 nextSceneB;        
  s16 triggerQuiz;       
  s16 questionId;        
  u8  bg;                 
  u8  music;              
} Scene;

// Quiz
typedef struct {
  u16 id;                 
  const char *name;       
  u8  wrongLimit;
  u16 questionCount;    
  const u16 *categories;  
  u16 categoryCount;     
} Quiz;

// Category names (ROM)
extern const char * const CATEGORY_NAMES[];
extern const u16 CATEGORY_COUNT;

extern const u16 * const CATEGORY_QUESTION_INDEXES[]; 
extern const u16        CATEGORY_QUESTION_COUNTS[];  