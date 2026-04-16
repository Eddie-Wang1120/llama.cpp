#pragma once
#include "common.cuh"

// Real I2_S dequantization — plain extern functions
void dequantize_row_i2_s_cuda_real_f32(const void * vx, float * y, const int64_t k, cudaStream_t stream);
void dequantize_row_i2_s_cuda_real_f16(const void * vx, half * y, const int64_t k, cudaStream_t stream);
