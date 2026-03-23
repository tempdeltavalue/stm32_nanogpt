#ifndef __MODEL_ENGINE_H
#define __MODEL_ENGINE_H

#include <stdint.h>

// Основні параметри нашої моделі
//#define VOCAB_SIZE 115
#define SEQ_LEN 32

// Публічні функції для керування AI
void ModelEngine_Init(void);
void ModelEngine_RunStep(void);

#endif /* __MODEL_ENGINE_H */

