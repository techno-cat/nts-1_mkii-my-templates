/*
Copyright 2024 Tomoaki Itoh
This software is released under the MIT License, see LICENSE.txt.
//*/

#include "unit_delfx.h"
#include <limits.h>
#include "utils/buffer_ops.h"
#include "LCWDelay.h"
#include "LCWDelayParam.h"

// memo:
// BPM=10, 四分音符の場合
// delaySamples = (samplingRate * delayTime * 60) / bpm
//              = (48000 * 1 * 60) / 10
//              = 288000 (0x46500)
// 288000より大きい2のN乗の値は524288 (0x80000)
// 必要なバイト数は2048k（= 524288 x 4byte）
//
// 0x80000 = 2^19 = 524288 = 10.92(sec) * 48000
// 524288 * sizeof(float) / 1024 = 2048K
#define LCW_DELAY_SAMPLING_SIZE (0x80000)

enum {
    TIME = 0U,
    DEPTH,
    MIX,
    PARAM4,
    NUM_PARAMS
};

enum {
    PARAM4_VALUE0 = 0,
    PARAM4_VALUE1,
    PARAM4_VALUE2,
    PARAM4_VALUE3,
    NUM_PARAM4_VALUES,
};

static struct {
    float time = 0.f;
    float depth = 0.f;
    float mix = 0.f;
    int32_t param4 = 0;
} s_param;

static struct {
    float tempo = 30.f;
} s_state;

static unit_runtime_desc_t runtime_desc;
static float *delay_ram_sampling;

static LCWDelayBuffer delayLine;
static LCWDelayBlock delayBlock;

// 0〜1を0..(n-1)にマッピング
static inline int32_t f32_to_index(float val, int32_t n)
{
    return clipmaxi32( (int32_t)(val * n), (n - 1) );
}

// ---- Callbacks exposed to runtime ----------------------------------------------

__unit_callback int8_t unit_init(const unit_runtime_desc_t * desc) {
    if (!desc)
        return k_unit_err_undef;

    if (desc->target != unit_header.target)
        return k_unit_err_target;

    if (!UNIT_API_IS_COMPAT(desc->api))
        return k_unit_err_api_version;

    if (desc->samplerate != 48000)
        return k_unit_err_samplerate;

    if (desc->input_channels != 2 || desc->output_channels != 2)
        return k_unit_err_geometry;

    if (!desc->hooks.sdram_alloc)
        return k_unit_err_memory;

    delay_ram_sampling = (float *)desc->hooks.sdram_alloc(LCW_DELAY_SAMPLING_SIZE * sizeof(float));
    if (!delay_ram_sampling)
        return k_unit_err_memory;

    buf_clr_f32(delay_ram_sampling, LCW_DELAY_SAMPLING_SIZE);

    runtime_desc = *desc;

    // set default values
    s_param.time = 0.f;
    s_param.depth = 0.f;
    s_param.mix = 0.f;
    s_param.param4 = 0;

    delayLine.buffer = delay_ram_sampling;
    delayLine.size = LCW_DELAY_SAMPLING_SIZE;
    delayLine.mask = LCW_DELAY_SAMPLING_SIZE - 1;
    delayLine.pointer = 0;

    delayBlock.delayLine = &delayLine;
    delayBlock.param.fbGain = 0.f;
    delayBlock.param.position = 0.f;
    delayBlock.currentPosition = 0.f;

    return k_unit_err_none;
}

__unit_callback void unit_teardown() {
    delay_ram_sampling = nullptr;
}

__unit_callback void unit_reset() {
}

__unit_callback void unit_resume() {
}

__unit_callback void unit_suspend() {
}

#define LCW_DELAY_SAMPLING_RATE (48000)
__unit_callback void unit_render(const float * in, float * out, uint32_t frames) {
    const float bpm = clampfsel(10.f, s_state.tempo, 480.f);
    const int32_t timeIndex = f32_to_index(s_param.time, LCW_DELAY_TIME_PARAMS);
    const float delayTime = delayTimeParams[timeIndex];
    // bps = bpm / 60
    // n = bps / delayTime
    // delaySamples = samplingRate / n
    //              = (samplingRate * delayTime) / bps
    //              = (samplingRate * delayTime * 60) / bpm
    delayBlock.param.position = (LCW_DELAY_SAMPLING_RATE * delayTime * 60) / bpm;

    const int32_t depthIndex = f32_to_index(s_param.depth, LCW_DELAY_FB_GAIN_TABLE_SIZE);
    delayBlock.param.fbGain = -1 * delayFbGainTable[depthIndex]; // memo: 符号反転はお好み

    const float * __restrict in_p = in;
    float * __restrict out_p = out;
    const float * out_e = out_p + (frames << 1); // output_channels: 2

    // -100.0 .. +100.0 -> 0.0 .. 1.0
    const float wet = (s_param.mix + 100.f) / 200.f;
    const float dry = 1.f - wet;

    for (; out_p != out_e; in_p += 2, out_p += 2) {
        const float xL = *(in_p + 0);
        // const float xR = *(in_p + 1);

        const float wL = LCWDelayProcess(&delayBlock, xL);
        // const float wR = xR;

        out_p[0] = (dry * xL) + (wet * wL);
        out_p[1] = (dry * xL) + (wet * wL);
        // *(x++) = (dry * xR) + (wet * wR);
    }
}

__unit_callback void unit_set_param_value(uint8_t id, int32_t value) {
    switch (id) {
    case TIME:
        // 0 .. 1023 -> 0.0 .. 1.0
        value = clipminmaxi32(0, value, 1023);
        s_param.time = param_10bit_to_f32(value);
        break;
    case DEPTH:
        // 0 .. 1023 -> 0.0 .. 1.0
        value = clipminmaxi32(0, value, 1023);
        s_param.depth = param_10bit_to_f32(value);
        break;
    case MIX:
        // -100.0 .. 100.0 -> -1.0 .. 1.0
        value = clipminmaxi32(-1000, value, 1000);
        s_param.mix = value / 1000.f;
        break;
    case PARAM4:
        s_param.param4 = value;
        break;
    default:
        break;
    }
}

__unit_callback int32_t unit_get_param_value(uint8_t id) {
    switch (id) {
    case TIME:
        // 0.0 .. 1.0 -> 0 .. 1023
        return param_f32_to_10bit(s_param.time);
        break;
    case DEPTH:
        // 0.0 .. 1.0 -> 0 .. 1023
        return param_f32_to_10bit(s_param.depth);
        break;
    case MIX:
        // -1.0 .. 1.0 -> -100.0 .. 100.0
        return (int32_t)(s_param.mix * 1000);
        break;
    case PARAM4:
        return s_param.param4;
        break;
    default:
        break;
    }

    return INT_MIN;
}

__unit_callback const char * unit_get_param_str_value(uint8_t id, int32_t value) {
    static const char * param4_strings[NUM_PARAM4_VALUES] = {
        "VAL0",
        "VAL1",
        "VAL2",
        "VAL3",
    };

    switch (id) {
    case PARAM4:
        if (value >= PARAM4_VALUE3 && value < NUM_PARAM4_VALUES)
            return param4_strings[value];
        break;
    default:
        break;
    }

    return nullptr;
}

__unit_callback void unit_set_tempo(uint32_t tempo) {
    s_state.tempo = tempo / static_cast<float>(0x10000);
}

__unit_callback void unit_tempo_4ppqn_tick(uint32_t counter) {
}
