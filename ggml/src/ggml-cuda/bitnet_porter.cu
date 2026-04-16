#include "dequantize.cuh"
#include <stdint.h>

/*
 * Sovereign Porter: JIT bit-transposition for BitNet I2_S layout.
 * Translates Linear Layout (GGUF) to Shuffled Layout (Ladder).
 * 16-element block (4 bytes) transformation.
 *
 * Linear (4 bytes): [e0,e1,e2,e3] [e4,e5,e6,e7] [e8,e9,e10,e11] [e12,e13,e14,e15]
 * Ladder (4 bytes): [e0,e4,e8,e12] [e1,e5,e9,e13] [e2,e6,e10,e14] [e3,e7,e11,e15]
 *
 * Each element is 2 bits.
 */

__global__ void k_i2s_porter(const char * __restrict__ src, char * __restrict__ dst, int64_t ne) {
    const int64_t i = (int64_t)blockIdx.x * blockDim.x + threadIdx.x;
    
    // Each thread processes 16 elements (4 bytes of weights)
    // There are 32 bytes of weights per 128 elements block.
    // 128 elements / 16 elements per thread = 8 threads per block.
    if (i >= (ne / 16)) return;

    // Load 4 linear bytes (4-byte aligned if ne is multiple of 16)
    const uint32_t val = ((const uint32_t *)src)[i];

    // Extraction masks (linear packing: e0 is bits 6,7; e1 4,5; e2 2,3; e3 0,1)
    // Word: [D12,13,14,15] [C8,9,10,11] [B4,5,6,7] [A0,1,2,3] (in little endian uint32)
    // Wait, let's assume little-endian uint32 load: 
    // byte 0 (A): bits 0-7. byte 1 (B): 8-15. byte 2 (C): 16-23. byte 3 (D): 24-31.
    
    // Target Byte 0 (e0, e4, e8, e12):
    // e0: (A >> 6) & 3
    // e4: (B >> 6) & 3
    // e8: (C >> 6) & 3
    // e12: (D >> 6) & 3
    
    uint32_t target = 0;
    
    #pragma unroll
    for (int bit_pair = 0; bit_pair < 4; ++bit_pair) {
        // bit_pair 0: e0, e4, e8, e12 (from bits 6-7 of each linear byte)
        // bit_pair 1: e1, e5, e9, e13 (from bits 4-5)
        // bit_pair 2: e2, e6, e10, e14 (from bits 2-3)
        // bit_pair 3: e3, e7, e11, e15 (from bits 0-1)
        
        uint32_t t_byte = 0;
        int shift = 6 - (2 * bit_pair);
        
        t_byte |= ((val >> (shift +  0)) & 0x03) << 6; // e_{0+bit_pair}
        t_byte |= ((val >> (shift +  8)) & 0x03) << 4; // e_{4+bit_pair}
        t_byte |= ((val >> (shift + 16)) & 0x03) << 2; // e_{8+bit_pair}
        t_byte |= ((val >> (shift + 24)) & 0x03) << 0; // e_{12+bit_pair}
        
        target |= (t_byte << (8 * bit_pair));
    }

    ((uint32_t *)dst)[i] = target;
}

// Wrapper function for the Porter Axon
extern "C" void bitnet_cuda_porter_axon(const char * src, char * dst, int64_t ne, cudaStream_t stream) {
    const int threads_per_block = 256;
    const int num_blocks = (ne / 16 + threads_per_block - 1) / threads_per_block;
    
    k_i2s_porter<<<num_blocks, threads_per_block, 0, stream>>>(src, dst, ne);
}
