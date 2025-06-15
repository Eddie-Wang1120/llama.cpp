**General Development Guidelines (from AGENTS.md & CONTRIBUTING.md):**
- Adhere to idiomatic Kotlin style with descriptive names and KDoc comments.
- Keep code modular (e.g., separate tensor creation from compute kernels).
- Prefer immutable data structures where practical.
- Document placeholders or incomplete implementations with `TODO` comments.
- Place unit tests under `src/nativeTest/kotlin`.
- Keep `KOTLIN_PORT_STATUS.md` and `KOTLIN_PORT_CHECKLIST.md` up to date (Ongoing meta-task).
- Kotlin code should strive for clarity and simplicity, drawing inspiration from C++ guidelines (e.g., basic loops where appropriate, though Kotlin idioms are preferred).

**Overarching: Build System and Platform Support (Kotlin Native)**
- [ ] Configure Gradle for Kotlin/Native compilation targeting macOS, Linux, Windows.
- [ ] Add Android (arm64-v8a, etc.) to Kotlin/Native Gradle build targets.
- [ ] Add iOS to Kotlin/Native Gradle build targets (Future).
- [ ] Ensure build system supports different build types (e.g., Release with optimizations, Debug with symbols).
- [ ] Allow configuration of backend support (CPU, Metal) through Gradle build options/properties.
- [ ] Investigate providing configurable build-time or runtime options similar to C++ build flags for performance tuning if applicable in Kotlin/Native.

## Phase 1: Project Setup and Initial Analysis (Partially Complete)
- [x] Setup Kotlin Native Development Environment
  - [x] Install Kotlin Native compiler and tools
  - [x] Configure build system (Gradle with Kotlin DSL) (Initial setup, to be expanded by overarching tasks)
  - [x] Setup project structure following Kotlin conventions
- [ ] Analyze C/C++ Codebase
  - [x] Identify and separate code related to CUDA, hipBLAS, Vulkan, SYCL, MUSA, and CANN backends
  - [x] Create an archive folder structure for non-supported backends
  - [ ] Document the core CPU and Metal implementation components
  - [ ] Map dependencies between core components and backend-specific code
  - [ ] Create a detailed map of all C/C++ files and their dependencies
  - [ ] Identify platform-specific code (Metal, AVX, etc.)
  - [ ] Document all external dependencies
- [x] Design Kotlin Native Architecture
  - [x] Design package structure (ai.solace.llamakotlin.*)
  - [x] Plan memory management approach (Kotlin Native has different memory model than C++)
  - [x] Design API that maintains compatibility with original while being idiomatic Kotlin (Ongoing consideration: align with recent C++ API changes where sensible)
  - [x] Create detailed design documents for remaining components

## Phase 2: Core Library Translation (ggml) (In Progress)
- [ ] Translate ggml Core Data Structures
  - [x] Define tensor data types (GGMLType enum)
  - [x] Define tensor operations (GGMLOp enum)
  - [x] Implement tensor structure (GGMLTensor class)
  - [x] Implement context structure (GGMLContext class)
  - [x] Implement computation graph structure (GGMLCGraph class)
  - [x] Implement memory allocation and management (basic structure and actual functionality)
- [x] Implement Basic Tensor Operations (Corresponds to "Finish Tensor Operations" from AGENTS.md)
  - [x] Implement tensor creation functions (createTensor, createTensor1D, createTensor2D)
  - [x] Define element-wise operations interfaces (add, mul)
  - [x] Define matrix multiplication interface (matMul)
  - [x] Implement activation functions (computeRelu, computeGelu)
  - [x] Implement support for all tensor data types (F32, F16, I8, I16, I32, I64)
  - [x] Implement optimized versions of tensor operations (High-level task)
  - [ ] Create `GGMLComputeOps.kt` file for actual computation logic
    - [ ] Implement utility function `calculateTotalSize` in `GGMLComputeOps.kt`
    - [ ] Implement utility function `allocateMemory` in `GGMLComputeOps.kt`
    - [ ] Implement `computeAdd` for F32, I32 in `GGMLComputeOps.kt`
      - [ ] Implement `computeAdd` for other data types (F16, I8, I16, I64, quantized types)
    - [ ] Implement `computeSub`
    - [ ] Implement `computeMul` for F32, I32 in `GGMLComputeOps.kt`
      - [ ] Implement `computeMul` for other data types (F16, I8, I16, I64, quantized types)
    - [ ] Implement `computeDiv`
    - [ ] Implement `computeNeg`
    - [ ] Implement `computeMatMul` for F32 in `GGMLComputeOps.kt`
      - [ ] **Note:** Ensure adherence to llama.cpp\'s matmul definition: C^T = A B^T (C = B A^T) (from CONTRIBUTING.md)
      - [ ] Implement `computeMatMul` for other data types (F16, I8, I16, I64, quantized types)
    - [ ] Implement `computeRelu` for F32 in `GGMLComputeOps.kt`
      - [ ] Implement `computeRelu` for other data types
    - [ ] Implement `computeGelu` for F32 in `GGMLComputeOps.kt`
      - [ ] Implement `computeGelu` for other data types
    - [ ] Integrate `GGMLComputeOps.kt` functions with `GGMLOps.kt`
- [ ] Implement Computation Graph (Corresponds to "Computation Graph Enhancements" from AGENTS.md)
  - [x] Implement forward pass computation
  - [ ] Extend `GGMLGraph.kt` with automatic differentiation support (Refined by AGENTS.md)
    - [x] Implement backward pass for ADD, SUB, MUL, NEG operations
    - [x] Implement backward pass for RELU, GELU activation functions
    - [x] Implement backward pass for MUL_MAT (matrix multiplication)
    - [x] Implement backward pass for DIV, SQR, SQRT operations
    - [x] Implement backward pass for SUM, MEAN operations
    - [x] Implement backward pass for REPEAT operation
    - [x] Implement backward pass for ABS, SGN, STEP operations
    - [ ] Implement backward pass for remaining operations
  - [ ] Implement graph optimization (e.g., redundant operation removal, as suggested by AGENTS.md)
- [ ] Implement Quantization Support (Corresponds to "Quantization Support" from AGENTS.md)
  - [ ] Implement 1.5-bit integer quantization (as per README)
  - [ ] Implement 2-bit integer quantization
  - [ ] Implement 3-bit integer quantization
  - [ ] Implement 4-bit integer quantization
  - [ ] Implement 5-bit integer quantization
  - [ ] Implement 6-bit integer quantization
  - [ ] Implement 8-bit integer quantization
  - [ ] Implement quantized operations
  - [ ] Add quantized type conversion utilities (New detail from AGENTS.md)

## Phase 3: CPU Backend Implementation (Corresponds to "CPU Backend Implementation" from AGENTS.md)
- [ ] Translate CPU-Specific Code
  - [ ] Implement basic CPU tensor operations (Create CPU-specific execution paths)
  - [ ] Investigate Kotlin/Native interop with C for performance-critical sections (New specific task from AGENTS.md)
  - [ ] Implement BLAS integration for CPU (if feasible, evaluate OpenBLAS/Accelerate-like approaches for Kotlin)
  - [ ] Implement ARM NEON optimizations for CPU (Inspired by llama.cpp\'s focus on Apple Silicon via ARM NEON)
  - [ ] Implement x86 AVX/AVX2/AVX512 optimizations where possible (if feasible in Kotlin/Native)
  - [ ] Consider SIMD vectorization for `GGMLComputeOps.kt` functions on CPU
- [ ] Optimize CPU Performance
  - [ ] Implement multi-threading support (emphasized by AGENTS.md)
  - [ ] Optimize memory access patterns (e.g., tiling for matmul)
  - [ ] Implement SIMD optimizations where possible in Kotlin Native

## Phase 4: Metal Backend Implementation
- [ ] Translate Metal-Specific Code
  - [ ] Implement Metal shader code in appropriate format (e.g., MSL for Kotlin/Native Metal integration)
  - [ ] Implement Metal backend for tensor operations
  - [ ] Implement Metal-specific memory management
- [ ] Optimize Metal Performance
  - [ ] Implement efficient Metal command buffer usage
  - [ ] Optimize Metal compute pipeline
  - [ ] Implement Metal-specific optimizations for Apple Silicon

## Phase 5: LLaMA Model Implementation
- [ ] Translate Model Structures (Align with current supported models in README)
  - [ ] Implement LLaMA model architecture (LLaMA 1/2/3)
  - [ ] Implement context and state management (incl. `llama_state_*` API concepts)
  - [ ] Implement token handling and vocabulary (incl. `llama_token_to_piece` concepts)
- [ ] Implement Inference Logic
  - [ ] Implement attention mechanism (consider Flash-Attention if feasible)
  - [ ] Implement feed-forward networks
  - [ ] Implement model loading and initialization (GGUF format)
- [ ] Implement Sampling Methods
  - [ ] Implement various sampling strategies (top-k, top-p, etc.)
  - [ ] Implement temperature scaling
  - [ ] Implement repetition penalties
- [ ] Implement Grammar-Constrained Generation
  - [ ] Implement GBNF grammar parsing
  - [ ] Implement grammar-constrained sampling

## Phase 6: Model Loading and File Format Support
- [ ] Implement GGUF Format Support
  - [ ] Implement GGUF file parsing
  - [ ] Implement model loading from GGUF files
  - [ ] Implement model conversion utilities (if porting `convert_hf_to_gguf.py` or similar)
- [ ] Implement State Saving/Loading
  - [ ] Implement session state serialization
  - [ ] Implement KV cache management (incl. `llama_kv_cache_seq_rm` concepts)
  - [ ] Implement context state management

## Phase 7: API and Applications
- [ ] Design and Implement Public API
  - [ ] Create idiomatic Kotlin API (suspension-friendly, Flow for streaming)
  - [ ] Implement C interoperability layer for existing applications (optional, evaluate need)
  - [ ] Document API thoroughly
  - [ ] **API Design Consideration:** Review recent C++ API changes from README.md for relevance to Kotlin API.
  - [ ] **API Design Consideration:** Design APIs with security best practices in mind (ref: SECURITY.MD).
- [ ] Implement Command Line Applications
  - [ ] Implement `llama-cli` equivalent in Kotlin
  - [ ] Implement `llama-server` equivalent (OpenAI API compatible HTTP server) in Kotlin
  - [ ] Implement chat applications (interactive and conversation modes)
- [ ] Implement Example Applications
  - [ ] Port existing example applications to Kotlin (e.g., basic generation, chat)
  - [ ] Create new Kotlin-specific examples
  - [ ] Implement multimodal support (LLaVA, etc.) (Future, post-core)

## Phase 8: Testing and Validation (Corresponds to "Testing Infrastructure" from AGENTS.md)
- [ ] Implement Unit Tests (Store in `src/nativeTest/kotlin`)
  - [ ] Test core tensor operations (including tensor creation, basic math ops in `GGMLComputeOps.kt`)
  - [ ] Test computation graph execution
  - [ ] Test model inference logic for key models
  - [ ] Test quantization accuracy
  - [ ] Test GGUF parsing and model loading
- [ ] Implement Integration Tests
  - [ ] Test end-to-end model loading and inference for key models
  - [ ] Provide sample models or fixtures for integration tests
  - [ ] Test performance benchmarks (tokens/sec, memory usage)
  - [ ] Compare output with original C++ implementation using common test prompts/datasets
- [ ] Validate Model Compatibility
  - [ ] Test with various LLaMA models (as per README)
  - [ ] Test with other supported models (Mistral, Mixtral, etc. as per README) (Future)
  - [ ] Ensure output matches original implementation for key models and configurations
- [ ] **Perform testing on target platforms (macOS, Linux, Windows, Android).**

## Phase 9: Documentation and Distribution (Corresponds to "Documentation Updates" from AGENTS.md)
- [ ] Create Documentation
  - [x] Create design documents for tensor operations (TENSOR_OPERATIONS_DESIGN.md)
  - [x] Create design documents for compute operations (GGML_COMPUTE_OPS_DESIGN.md)
  - [x] Document current status (KOTLIN_PORT_STATUS.md)
  - [ ] Write API documentation (KDoc for all public APIs)
  - [ ] Create usage guides (how to build, run examples, use library)
  - [ ] Document performance characteristics and optimization tips for Kotlin port
  - [ ] Document new modules and APIs in the `/docs` folder (or equivalent for Kotlin project)
  - [ ] **Document security considerations for users of the Kotlin Native library (ref: SECURITY.MD).**
- [ ] Setup Distribution
  - [ ] Configure Maven/Gradle publishing (to Maven Central or similar)
  - [ ] Create release process (versioning, changelog)
  - [ ] Setup continuous integration (GitHub Actions) for builds and tests
  - [ ] **Plan for packaging and distribution (e.g., Homebrew, Nix, Docker if applicable).**
- [ ] Create Migration Guide
  - [ ] Document differences from C++ implementation
  - [ ] Provide migration examples for existing users (if applicable)
  - [ ] Document performance trade-offs

## Phase 10: Performance Optimization
- [ ] Benchmark and Profile
  - [ ] Identify performance bottlenecks in Kotlin/Native code
  - [ ] Compare with C++ implementation benchmarks
  - [ ] Document performance characteristics of the Kotlin port
- [ ] Optimize Critical Paths
  - [ ] Optimize tensor operations
  - [ ] Implement/Optimize SIMD vectorization for tensor ops (NEON, potentially AVX via Kotlin/Native intrinsics or libraries)
  - [ ] Implement/Optimize Multi-threading for tensor ops
  - [ ] Implement/Optimize Memory Access Patterns for tensor ops
  - [ ] Optimize memory usage (Kotlin/Native specific considerations)
  - [ ] Optimize threading model (coroutines, worker threads)
- [ ] Implement Advanced Optimizations
  - [ ] Implement speculative decoding (if applicable)
  - [ ] Optimize KV cache management in Kotlin
  - [ ] Implement model-specific optimizations
  - [ ] Consider CPU+GPU hybrid inference capabilities (Future)
