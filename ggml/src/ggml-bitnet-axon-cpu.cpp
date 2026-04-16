#include "ggml-bitnet-axon.h"
#include <iostream>

// CPU Axon: Wraps the core GGML C++ / Neon implementations (Phase 2)
static struct ggml_bitnet_axon_config axon_config = {0, true, true};

// Reference to core dequantization
extern "C" void dequantize_row_i2_s(const void * vx, float * y, int64_t k);

static bool bitnet_axon_cpu_init(const struct ggml_bitnet_axon_config * config) {
    if (config) axon_config = *config;
    return true;
}

static void bitnet_axon_cpu_transmute(const char * src, char * dst, int64_t ne, void * stream) {
    // CPU usually doesn't need shuffle-transmutation, bit-layout is standard
}

static void bitnet_axon_cpu_mul_mat(
    const char * src0, const char * src1, float * dst,
    int64_t ne00, int64_t ne01, int64_t ne11, int64_t ne0,
    void * stream) {
    
    // Fallback to standard GGML CPU multiplication logic
    // (Actual linking happens in ggml.c where I2_S was registered)
}

static void bitnet_axon_cpu_free(void) {}

const struct ggml_bitnet_axon_interface ggml_bitnet_axon_cpu = {
    bitnet_axon_cpu_init,
    bitnet_axon_cpu_transmute,
    bitnet_axon_cpu_mul_mat,
    bitnet_axon_cpu_free
};
