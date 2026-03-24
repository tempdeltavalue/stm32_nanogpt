#include "model_engine.h"
#include "tmp_transformer.h"
#include "tmp_transformer_data.h"
#include "usbd_cdc_if.h"
#include "main.h"
#include "vocab.h"
#include "st7735.h"
#include "fonts.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>

#define VOCAB_SIZE    1024
#define SEQ_LEN       32

// --- ЕНАМКА ДЛЯ ВИБОРУ ВИВОДУ ---
typedef enum {
    OUT_USB_ONLY,
    OUT_TFT_ONLY,
    OUT_BOTH
} OutputTarget_t;

static OutputTarget_t current_output = OUT_BOTH;

// --- СТАН ЕКРАНУ ---
static uint16_t current_bg_color = ST7735_BLACK;
static uint16_t current_fg_color = ST7735_GREEN;
static uint16_t cursor_x = 0;
static uint16_t cursor_y = 15;

static ai_handle network = AI_HANDLE_NULL;
static ai_u8 activations[AI_TMP_TRANSFORMER_DATA_ACTIVATIONS_SIZE];
static ai_buffer *ai_input;
static ai_buffer *ai_output;

static int32_t my_context_window[SEQ_LEN];
static uint32_t step_counter = 0;
static float exp_probs[VOCAB_SIZE];

// --- ДОПОМІЖНІ ФУНКЦІЇ ---

uint16_t Get_Random_Color(void) {
    return (uint16_t)rand() & 0xFFFF;
}

static void USB_SendString(const char* str) {
    uint8_t res;
    do {
        res = CDC_Transmit_FS((uint8_t*)str, (uint16_t)strlen(str));
    } while (res == USBD_BUSY);
}

static void TFT_PrintToken(const char* str) {
    int token_len = strlen(str);
    int char_w = 7;
    int char_h = 10;

    if (cursor_x + (token_len * char_w) > 128 || str[0] == '\n') {
        cursor_x = 0;
        cursor_y += char_h;
    }

    if (cursor_y + char_h > 160) {
        current_bg_color = Get_Random_Color();
        current_fg_color = Get_Random_Color();
        if (abs(current_bg_color - current_fg_color) < 0x2000) current_fg_color ^= 0xFFFF;
        ST7735_FillScreen(current_bg_color);
        cursor_x = 0;
        cursor_y = 0;
    }

    if (str[0] != '\n') {
    	ST7735_WriteStringGFX(&cursor_x, &cursor_y, str, &cambriaUkr8, current_fg_color, current_bg_color);
    	cursor_x += (token_len * char_w);
    }
}

static void Smart_Print(const char* str) {
    switch (current_output) {
        case OUT_USB_ONLY:
            USB_SendString(str);
            break;
        case OUT_TFT_ONLY:
            TFT_PrintToken(str);
            break;
        case OUT_BOTH:
            USB_SendString(str);
            TFT_PrintToken(str);
            break;
    }
}

// --- API МОДЕЛІ ---

void ModelEngine_Init(void) {
    srand(HAL_GetTick());

    // Якщо вибрано екран — ініцілізуємо кольори
    if (current_output != OUT_USB_ONLY) {
        current_bg_color = Get_Random_Color();
        current_fg_color = Get_Random_Color();
        ST7735_FillScreen(current_bg_color);
    }

    ai_tmp_transformer_create(&network, AI_TMP_TRANSFORMER_DATA_CONFIG);
    const ai_network_params params = {
        .params = AI_TMP_TRANSFORMER_DATA_WEIGHTS(ai_tmp_transformer_data_weights_get()),
        .activations = AI_TMP_TRANSFORMER_DATA_ACTIVATIONS(activations)
    };
    ai_tmp_transformer_init(network, &params);
    ai_input = ai_tmp_transformer_inputs_get(network, NULL);
    ai_output = ai_tmp_transformer_outputs_get(network, NULL);

    for (int i = 0; i < SEQ_LEN; i++) my_context_window[i] = 0;
    for (int i = 0; i < PROMPT_LEN; i++) my_context_window[SEQ_LEN - PROMPT_LEN + i] = prompt_array[i];

    step_counter = 0;

    if (current_output != OUT_TFT_ONLY) USB_SendString("\r\n--- KOBZAR READY ---\r\n");

    // Вивід промпту через розумну функцію
    for(int i = 0; i < PROMPT_LEN; i++) {
        Smart_Print(ix_to_char_map[prompt_array[i]]);
    }
}

void ModelEngine_RunStep(void) {
    if (step_counter >= 150) return;

    int32_t* in_data = (int32_t*)ai_input[0].data;
    for (int i = 0; i < SEQ_LEN; i++) in_data[i] = my_context_window[i];

    if (ai_tmp_transformer_run(network, &ai_input[0], &ai_output[0]) == 1) {
        float* out_logits = (float*)ai_output[0].data;
        float temperature = 0.6f;

        float max_logit = out_logits[0];
        for (int i = 1; i < VOCAB_SIZE; i++) if (out_logits[i] > max_logit) max_logit = out_logits[i];

        float sum_exp = 0.0f;
        for (int i = 0; i < VOCAB_SIZE; i++) {
            exp_probs[i] = expf((out_logits[i] - max_logit) / temperature);
            sum_exp += exp_probs[i];
        }

        float random_val = ((float)rand() / (float)RAND_MAX) * sum_exp;
        int next_idx = 0;
        float cumulative_prob = 0.0f;
        for (int i = 0; i < VOCAB_SIZE; i++) {
            cumulative_prob += exp_probs[i];
            if (random_val <= cumulative_prob) { next_idx = i; break; }
        }

        // Використовуємо наш вибір виводу
        Smart_Print(ix_to_char_map[next_idx]);

        for (int i = 0; i < SEQ_LEN - 1; i++) my_context_window[i] = my_context_window[i+1];
        my_context_window[SEQ_LEN - 1] = next_idx;
        step_counter++;
    }
}
