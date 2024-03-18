/*
Copyright 2024 Tomoaki Itoh
This software is released under the MIT License, see LICENSE.txt.
//*/

#ifdef __cplusplus
extern "C" {
#endif

#define LCW_DELAY_TIME_PARAMS (10)
#define LCW_DELAY_FB_GAIN_TABLE_SIZE (64)

extern const float delayTimeParams[LCW_DELAY_TIME_PARAMS];
extern const float delayFbGainTable[LCW_DELAY_FB_GAIN_TABLE_SIZE];

#ifdef __cplusplus
}
#endif
