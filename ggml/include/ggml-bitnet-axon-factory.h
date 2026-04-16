#pragma once

#include "ggml-bitnet-axon.h"

#ifdef  __cplusplus
extern "C" {
#endif

// Sovereign Axon Factory: Dynamically selects the best inference engine
// available for the current hardware environment.
const struct ggml_bitnet_axon_interface * ggml_bitnet_axon_find_best(void);

// Registry of available hardware-specialized Axones
extern const struct ggml_bitnet_axon_interface ggml_bitnet_axon_cuda;
extern const struct ggml_bitnet_axon_interface ggml_bitnet_axon_vulkan;
extern const struct ggml_bitnet_axon_interface ggml_bitnet_axon_cpu;

#ifdef  __cplusplus
}
#endif
