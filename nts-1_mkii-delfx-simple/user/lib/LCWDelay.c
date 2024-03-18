/*
Copyright 2024 Tomoaki Itoh
This software is released under the MIT License, see LICENSE.txt.
//*/

#include "LCWDelay.h"
#include "utils/float_math.h"

static float convergePosition(float position, float current)
{
    const float param = 0.99979f;
    const float diff = position - current;
    if ( si_fabsf(diff) < 0.001f ) {
        return position;
    }
    else {
        return position - (diff * param);
    }
}

static float lookupBuffer(LCWDelayBuffer *buf, float offset)
{
#if 1
    const int32_t i = (int32_t)offset;
    const float frac = offset - (float)i;
    const float val1 = LCW_DELAY_BUFFER_LUT(buf, i);
    const float val2 = LCW_DELAY_BUFFER_LUT(buf, i+1);
    return val1 + ((val2 - val1) * frac);
#else
    const int32_t i = (int32_t)offset;
    return LCW_DELAY_BUFFER_LUT(buf, i);
#endif
}

float LCWDelayProcess(LCWDelayBlock *block, float input)
{
    LCWDelayBuffer *buf = block->delayLine;

    // currentPositionをpositionに近づける
    block->currentPosition = convergePosition(block->param.position, block->currentPosition);

    // ディレイの起点を更新
    buf->pointer = LCW_DELAY_BUFFER_DEC(buf);

    // フィードバック付きディレイ処理
    const float ret = lookupBuffer(buf, block->currentPosition);
    buf->buffer[buf->pointer] = input + (block->param.fbGain * ret);
    return ret;
}
