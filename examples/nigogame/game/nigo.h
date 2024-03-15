#pragma once

const float frames_nigo_throw[] = {1.0f,  1.0f,  68.0f, 40.0f, 1.0f,  43.0f,
                                   68.0f, 40.0f, 71.0f, 1.0f,  68.0f, 40.0f,
                                   71.0f, 43.0f, 68.0f, 40.0f};
const float frames_nigo_run[] = {1.0f,  85.0f, 30.0f,  28.0f, 33.0f,  85.0f,
                                 30.0f, 28.0f, 65.0f,  85.0f, 30.0f,  28.0f,
                                 97.0f, 85.0f, 30.0f,  28.0f, 129.0f, 85.0f,
                                 30.0f, 28.0f, 161.0f, 73.0f, 30.0f,  28.0f};
const float frames_nigo_dig[] = {141.0f, 1.0f,  40.0f, 34.0f,
                                 141.0f, 37.0f, 40.0f, 34.0f};
const float frames_nigo_idle[] = {183.0f, 1.0f,  28.0f,  26.0f, 193.0f, 59.0f,
                                  28.0f,  26.0f, 213.0f, 1.0f,  28.0f,  26.0f,
                                  193.0f, 87.0f, 28.0f,  26.0f};
const float frames_nigo_fall[] = {183.0f, 29.0f, 26.0f, 28.0f};
const float frames_nigo_jump[] = {211.0f, 29.0f, 26.0f, 28.0f};
const NcSpritesheetAnimation anim_nigo[] = {
    {"nigo_throw", 4, frames_nigo_throw}, {"nigo_run", 6, frames_nigo_run},
    {"nigo_dig", 2, frames_nigo_dig},     {"nigo_idle", 4, frames_nigo_idle},
    {"nigo_fall", 1, frames_nigo_fall},   {"nigo_jump", 1, frames_nigo_jump}};
const unsigned int anim_nigo_num = 6;
