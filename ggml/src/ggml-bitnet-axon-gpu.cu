#include "ggml-bitnet-axon.h"
#include "ggml-cuda/common.cuh"
#include <cuda_runtime.h>

#ifdef __HIP_PLATFORM_AMD__
#include <hip/hip_runtime.h>
#endif

// Forward declarations of internal kernels
extern "C" void bitnet_cuda_porter_axon(const char * src, char * dst, int64_t ne, cudaStream_t stream);
extern "C" void bitnet_mul_mat_ladder_axon(
    const char * src0, const char * src1, float * dst,
    int64_t ne00, int64_t ne01, int64_t ne11, int64_t ne0,
    cudaStream_t stream);

static struct ggml_bitnet_axon_config axon_config = {0, true, true};

// Implementation of the CUDA Axon Interface
static bool bitnet_axon_cuda_init(const struct ggml_bitnet_axon_config * config) {
    if (config) {
        axon_config = *config;
    } else {
        // Default: Be Silent unless environment variable says otherwise
        const char * verbose_env = getenv("GGML_BITNET_VERBOSE");
        axon_config.verbose_level = verbose_env ? atoi(verbose_env) : 0;
    }
    return true;
}

static void bitnet_axon_cuda_transmute(const char * src, char * dst, int64_t ne, void * stream) {
    if (axon_config.use_porter) {
        if (axon_config.verbose_level > 0) {
            // Log once per session logic could be added here
        }
        bitnet_cuda_porter_axon(src, dst, ne, (cudaStream_t)stream);
    }
}

static void bitnet_axon_cuda_mul_mat(
    const char * src0, const char * src1, float * dst,
    int64_t ne00, int64_t ne01, int64_t ne11, int64_t ne0,
    void * stream) {
    
    // External high-perf kernels bridge
    bitnet_mul_mat_ladder_axon(src0, src1, dst, ne00, ne01, ne11, ne0, (cudaStream_t)stream);
}

static void bitnet_axon_cuda_free(void) {
    // Cleanup if necessary
}

extern "C" const struct ggml_bitnet_axon_interface ggml_bitnet_axon_cuda = {
    bitnet_axon_cuda_init,
    bitnet_axon_cuda_transmute,
    bitnet_axon_cuda_mul_mat,
    bitnet_axon_cuda_free
};
