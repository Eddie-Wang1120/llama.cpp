// Sovereign I2_S Dequantization Kernel for CUDA
// Matches the REAL on-disk format: 32 bytes weights + 4 bytes float scale = 36 bytes per 128 elements

#include <cuda_runtime.h>
#include <cuda_fp16.h>
#include <cstdint>

#define I2S_BLOCK_SIZE 128
#define I2S_BYTES_PER_BLOCK 36

static __global__ void dequantize_block_i2_s_real_f32(
    const void * __restrict__ vx, float * __restrict__ y, const int64_t k) {
    
    const int64_t tid = (int64_t)blockDim.x * blockIdx.x + threadIdx.x;
    const int64_t block_idx = tid / 32;
    const int64_t byte_in_block = tid % 32;
    
    const int64_t first_elem = block_idx * I2S_BLOCK_SIZE;
    if (first_elem >= k) return;
    
    const uint8_t * block_ptr = (const uint8_t *)vx + block_idx * I2S_BYTES_PER_BLOCK;
    
    float block_scale;
    memcpy(&block_scale, block_ptr + 32, sizeof(float));
    
    const uint8_t b = block_ptr[byte_in_block];
    
    const uint8_t c0 = (b >> 6) & 0x3;
    const uint8_t c1 = (b >> 4) & 0x3;
    const uint8_t c2 = (b >> 2) & 0x3;
    const uint8_t c3 = (b >> 0) & 0x3;
    
    const float m0 = (c0 == 0) ? -1.0f : (c0 == 2) ? 1.0f : 0.0f;
    const float m1 = (c1 == 0) ? -1.0f : (c1 == 2) ? 1.0f : 0.0f;
    const float m2 = (c2 == 0) ? -1.0f : (c2 == 2) ? 1.0f : 0.0f;
    const float m3 = (c3 == 0) ? -1.0f : (c3 == 2) ? 1.0f : 0.0f;
    
    const int64_t base = first_elem;
    if (base + 0*32 + byte_in_block < k) y[base + 0*32 + byte_in_block] = (block_scale * m0);
    if (base + 1*32 + byte_in_block < k) y[base + 1*32 + byte_in_block] = (block_scale * m1);
    if (base + 2*32 + byte_in_block < k) y[base + 2*32 + byte_in_block] = (block_scale * m2);
    if (base + 3*32 + byte_in_block < k) y[base + 3*32 + byte_in_block] = (block_scale * m3);
}

static __global__ void dequantize_block_i2_s_real_f16(
    const void * __restrict__ vx, half * __restrict__ y, const int64_t k) {
    
    const int64_t tid = (int64_t)blockDim.x * blockIdx.x + threadIdx.x;
    const int64_t block_idx = tid / 32;
    const int64_t byte_in_block = tid % 32;
    
    const int64_t first_elem = block_idx * I2S_BLOCK_SIZE;
    if (first_elem >= k) return;
    
    const uint8_t * block_ptr = (const uint8_t *)vx + block_idx * I2S_BYTES_PER_BLOCK;
    
    float block_scale;
    memcpy(&block_scale, block_ptr + 32, sizeof(float));
    
    const uint8_t b = block_ptr[byte_in_block];
    
    const uint8_t c0 = (b >> 6) & 0x3;
    const uint8_t c1 = (b >> 4) & 0x3;
    const uint8_t c2 = (b >> 2) & 0x3;
    const uint8_t c3 = (b >> 0) & 0x3;
    
    const float m0 = (c0 == 0) ? -1.0f : (c0 == 2) ? 1.0f : 0.0f;
    const float m1 = (c1 == 0) ? -1.0f : (c1 == 2) ? 1.0f : 0.0f;
    const float m2 = (c2 == 0) ? -1.0f : (c2 == 2) ? 1.0f : 0.0f;
    const float m3 = (c3 == 0) ? -1.0f : (c3 == 2) ? 1.0f : 0.0f;
    
    const int64_t base = first_elem;
    if (base + 0*32 + byte_in_block < k) y[base + 0*32 + byte_in_block] = (half)(block_scale * m0);
    if (base + 1*32 + byte_in_block < k) y[base + 1*32 + byte_in_block] = (half)(block_scale * m1);
    if (base + 2*32 + byte_in_block < k) y[base + 2*32 + byte_in_block] = (half)(block_scale * m2);
    if (base + 3*32 + byte_in_block < k) y[base + 3*32 + byte_in_block] = (half)(block_scale * m3);
}

void dequantize_row_i2_s_cuda_real_f32(const void * vx, float * y, const int64_t k, cudaStream_t stream) {
    const int64_t total_bytes = k / 4;
    const int block_size = 256;
    const int num_blocks = (total_bytes + block_size - 1) / block_size;
    dequantize_block_i2_s_real_f32<<<num_blocks, block_size, 0, stream>>>(vx, y, k);
}

void dequantize_row_i2_s_cuda_real_f16(const void * vx, half * y, const int64_t k, cudaStream_t stream) {
    const int64_t total_bytes = k / 4;
    const int block_size = 256;
    const int num_blocks = (total_bytes + block_size - 1) / block_size;
    dequantize_block_i2_s_real_f16<<<num_blocks, block_size, 0, stream>>>(vx, y, k);
}
