#pragma once

#include "ggml.h"
#include "ggml-backend.h"

#ifdef  __cplusplus
extern "C" {
#endif

/* BE-WATER: Sovereign Axon Interface for BitNet 1.58b 
 * This header defines the modular contract for specialized inference backends.
 */

struct ggml_bitnet_axon_config {
    int64_t verbose_level;  // 0: Silent, 1+: Debug
    bool    use_ladder;     // Enable high-perf interleaved kernels
    bool    use_porter;     // Enable JIT VRAM transmutation
};

// Interface for a Sovereign Inference Axon
struct ggml_bitnet_axon_interface {
    // Handshake: Verify hardware and initialize internal state
    bool (*init)(const struct ggml_bitnet_axon_config * config);

    // Transmutation: Porter JIT conversion
    void (*transmute)(const char * src, char * dst, int64_t ne, void * stream);

    // Matrix Multiplication: Ladder high-perf kernel
    void (*mul_mat)(const char * src0, const char * src1, float * dst,
                    int64_t ne00, int64_t ne01, int64_t ne11, int64_t ne0,
                    void * stream);

    // Cleanup: Release axon resources
    void (*free)(void);
};

// Sovereign Axon Instances (exposed for linking)
extern const struct ggml_bitnet_axon_interface ggml_bitnet_axon_cpu;
#if defined(GGML_USE_CUDA) && !defined(GGML_USE_HIPBLAS)
extern const struct ggml_bitnet_axon_interface ggml_bitnet_axon_cuda;
#endif
#ifdef GGML_USE_VULKAN
extern const struct ggml_bitnet_axon_interface ggml_bitnet_axon_vulkan;
#endif

#ifdef  __cplusplus
}
#endif
