#include "ggml-bitnet-axon-factory.h"
#include <cstdlib>
#include <cstring>
#include <iostream>

const struct ggml_bitnet_axon_interface * ggml_bitnet_axon_find_best(void) {
    // 1. Manual Force Override (for experimentation/Consensus mode)
    const char * force_env = getenv("GGML_BITNET_FORCE_AXON");
    if (force_env) {
        if (strcmp(force_env, "VULKAN") == 0) return &ggml_bitnet_axon_vulkan;
        if (strcmp(force_env, "CUDA") == 0)   return &ggml_bitnet_axon_cuda;
        if (strcmp(force_env, "CPU") == 0)    return &ggml_bitnet_axon_cpu;
    }

    // 2. Hardware Autodetection Logic
    // In GGML, this is usually determined by which backends are active.
    
#ifdef GGML_USE_CUDA
    return &ggml_bitnet_axon_cuda;
#endif

#ifdef GGML_USE_HIPBLAS
    return &ggml_bitnet_axon_cuda; // ROCm shares the CUDA/HIP axon
#endif

#ifdef GGML_USE_VULKAN
    return &ggml_bitnet_axon_vulkan;
#endif

    // 3. Absolute Fallback
    return &ggml_bitnet_axon_cpu;
}
