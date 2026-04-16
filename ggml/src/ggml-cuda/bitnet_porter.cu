#include "bitnet_porter.cuh"
#include "../../gpu/bitnet_kernels/bitnet_kernels.h"
#include <iostream>

// Forward declaration of the specialized bitlinear launcher
extern "C" void bitlinear_int8xint2(int8_t* input0, int8_t* input1, __nv_bfloat16* output0, __nv_bfloat16* s, __nv_bfloat16* ws, int M, int N, int K, cudaStream_t stream);

// Sovereign Porter Axon: JIT Transmutation from linear GGUF to Interleaved Ladder layout
__global__ void k_i2s_porter(const uint32_t * src, uint32_t * dst, int64_t n_blocks) {
    int64_t i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n_blocks) {
        // Rearrangement logic for I2_S:
        // Source: [e0..e15] in linear order
        // Destination: [e0, e4, e8, e12, e1, e5, e9, e13, e2, e6, e10, e14, e3, e7, e11, e15]
        
        uint32_t v = src[i];
        uint32_t res = 0;
        
        // e0: bits 0-1   -> pos 0 (bits 0-1)
        res |= (v & 0x00000003);
        // e4: bits 8-9   -> pos 1 (bits 2-3)
        res |= (v & 0x00000300) >> 6;
        // e8: bits 16-17 -> pos 2 (bits 4-5)
        res |= (v & 0x00030000) >> 12;
        // e12: bits 24-25 -> pos 3 (bits 6-7)
        res |= (v & 0x03000000) >> 18;
        
        // e1: bits 2-3   -> pos 4 (bits 8-9)
        res |= (v & 0x0000000C) << 6;
        // e5: bits 10-11 -> pos 5 (bits 10-11)
        res |= (v & 0x00000C00);
        // e9: bits 18-19 -> pos 6 (bits 12-13)
        res |= (v & 0x000C0000) >> 6;
        // e13: bits 26-27 -> pos 7 (bits 14-15)
        res |= (v & 0x0C000000) >> 12;
        
        // e2: bits 4-5   -> pos 8 (bits 16-17)
        res |= (v & 0x00000030) << 12;
        // e6: bits 12-13 -> pos 9 (bits 18-19)
        res |= (v & 0x00003000) << 6;
        // e10: bits 20-21 -> pos 10 (bits 20-21)
        res |= (v & 0x00300000);
        // e14: bits 28-29 -> pos 11 (bits 22-23)
        res |= (v & 0x30000000) >> 6;
        
        // e3: bits 6-7   -> pos 12 (bits 24-25)
        res |= (v & 0x000000C0) << 18;
        // e7: bits 14-15 -> pos 13 (bits 26-27)
        res |= (v & 0x0000C000) << 12;
        // e11: bits 22-23 -> pos 14 (bits 28-29)
        res |= (v & 0x00C00000) << 6;
        // e15: bits 30-31 -> pos 15 (bits 30-31)
        res |= (v & 0xC0000000);

        dst[i] = res;
    }
}

extern "C" void bitnet_cuda_porter_axon(const char * src, char * dst, int64_t ne, cudaStream_t stream) {
    const int block_size = 256;
    const int64_t n_blocks = (ne + 15) / 16;
    const int grid_size = (n_blocks + block_size - 1) / block_size;

    k_i2s_porter<<<grid_size, block_size, 0, stream>>>((const uint32_t *)src, (uint32_t *)dst, n_blocks);
}

// Activation Quantization Kernel (Symmetric i8)
__global__ void k_quantize_i8(const float * src, int8_t * dst, int64_t n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        dst[i] = (int8_t)fmaxf(-127.0f, fminf(127.0f, roundf(src[i] * 127.0f)));
    }
}

extern "C" void bitnet_mul_mat_ladder_axon(
    const char * src0, const char * src1, float * dst,
    int64_t ne00, int64_t ne01, int64_t ne11, int64_t ne0,
    cudaStream_t stream) {

    // src0: Weights (Ladder transmuted I2_S) -> int8_t*
    // src1: Activations (from llama.cpp, assume float* for now)
    
    if (ne11 == 1) {
        // High-performance bridge for Phase 3.4
        // Logic to be refined as we manage BF16 buffers for output and scales
        // For now, we remain silent and allow the DMMV fallback if specific shapes aren't matched.
    }
}
