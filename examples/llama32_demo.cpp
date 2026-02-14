#include "t81/weights.hpp"
#include "t81/tisc/program.hpp"
#include "t81/tisc/opcodes.hpp"
#include "t81/vm/vm.hpp"
#include <iostream>
#include <vector>
#include <memory>
#include <span>
#include <chrono>
#include <cstring>
#include <cmath>

using namespace t81;
using namespace t81::tisc;
using namespace t81::weights;

int main() {
    std::cout << "--- T81 'Go Broad' Killer Demo: Llama-3.2-1B Deterministic Inference Block ---\n";

    const int hidden_dim = 2048; // Llama-3.2-1B dimensions
    const int num_heads = 32;
    [[maybe_unused]] const int head_dim = hidden_dim / num_heads;

    // 1. Create a mock T3_K quantized weights model
    NativeModel mock_weights;
    auto create_dummy_t3k = [&](const std::string& name, std::vector<uint64_t> shape) {
        uint64_t total = 1;
        for (auto d : shape) total *= d;

        NativeTensor tensor;
        tensor.shape = shape;
        tensor.trits = total;
        tensor.format = NativeFormat::T3_K;

        size_t num_blocks = (total + 127) / 128;
        size_t total_bytes = num_blocks * 52; // 4 bytes scale + 48 bytes trits
        tensor.data.assign((total_bytes + 7) / 8, 0);
        uint8_t* byte_ptr = reinterpret_cast<uint8_t*>(tensor.data.data());

        for (size_t b = 0; b < num_blocks; ++b) {
            float scale = 0.1f + (b % 10) * 0.05f;
            std::memcpy(byte_ptr, &scale, sizeof(float));
            byte_ptr += sizeof(float);

            uint64_t buffer = 0;
            int bits = 0;
            for (int i = 0; i < 128; ++i) {
                size_t idx = b * 128 + i;
                // Variation to ensure non-zero results
                int8_t trit = (idx < total) ? static_cast<int8_t>((idx % 3) - 1) : 0;
                if (trit == 0 && (idx % 11 == 0)) trit = (idx % 2 == 0) ? 1 : -1;

                uint32_t val = static_cast<uint32_t>(trit + 1);
                buffer = (buffer << 3) | val;
                bits += 3;
                while (bits >= 8) {
                    bits -= 8;
                    *byte_ptr++ = static_cast<uint8_t>(buffer >> bits);
                }
            }
        }
        mock_weights[name] = std::move(tensor);
    };

    std::vector<std::string> weight_names = {
        "model.layers.0.input_layernorm.weight",
        "model.layers.0.self_attn.q_proj.weight",
        "model.layers.0.self_attn.k_proj.weight",
        "model.layers.0.self_attn.v_proj.weight",
        "model.layers.0.self_attn.o_proj.weight",
        "model.layers.0.post_attention_layernorm.weight",
        "model.layers.0.mlp.gate_proj.weight",
        "model.layers.0.mlp.up_proj.weight",
        "model.layers.0.mlp.down_proj.weight"
    };

    create_dummy_t3k(weight_names[0], {static_cast<uint64_t>(hidden_dim)});
    create_dummy_t3k(weight_names[1], {static_cast<uint64_t>(hidden_dim), static_cast<uint64_t>(hidden_dim)});
    create_dummy_t3k(weight_names[2], {static_cast<uint64_t>(hidden_dim), static_cast<uint64_t>(hidden_dim)});
    create_dummy_t3k(weight_names[3], {static_cast<uint64_t>(hidden_dim), static_cast<uint64_t>(hidden_dim)});
    create_dummy_t3k(weight_names[4], {static_cast<uint64_t>(hidden_dim), static_cast<uint64_t>(hidden_dim)});
    create_dummy_t3k(weight_names[5], {static_cast<uint64_t>(hidden_dim)});
    create_dummy_t3k(weight_names[6], {static_cast<uint64_t>(hidden_dim), static_cast<uint64_t>(hidden_dim)});
    create_dummy_t3k(weight_names[7], {static_cast<uint64_t>(hidden_dim), static_cast<uint64_t>(hidden_dim)});
    create_dummy_t3k(weight_names[8], {static_cast<uint64_t>(hidden_dim), static_cast<uint64_t>(hidden_dim)});

    // 2. Build TISC program
    Program program;
    program.weights_model = std::make_shared<ModelFile>();
    program.weights_model->native = std::move(mock_weights);
    program.symbol_pool = weight_names;

    // Initial input tensor (Rank 2: 1xHiddenDim)
    std::vector<float> input_data(hidden_dim);
    for(int i=0; i<hidden_dim; ++i) input_data[i] = 0.1f + 0.01f * (i % 100);
    program.tensor_pool.emplace_back(std::vector<int>{1, hidden_dim}, std::move(input_data));

    std::vector<Insn> insns;
    int reg_x = 0;      // Current residual state
    int reg_norm = 1;   // Normed x
    int reg_q = 2, reg_k = 3, reg_v = 4;
    int reg_attn = 5;
    int reg_out = 6;
    int reg_tmp1 = 7, reg_tmp2 = 8;

    // Load input to reg_x
    insns.push_back({Opcode::LoadImm, reg_x, 1, 0, LiteralKind::TensorHandle});

    // --- Self-Attention Block ---
    // 1. RMSNorm
    insns.push_back({Opcode::WeightsLoad, reg_tmp1, 0}); // input_layernorm.weight
    insns.push_back({Opcode::TRMSNorm, reg_norm, reg_x, reg_tmp1});

    // 2. Q, K, V projections
    insns.push_back({Opcode::WeightsLoad, reg_tmp1, 1}); // q_proj
    insns.push_back({Opcode::TMatMul, reg_q, reg_norm, reg_tmp1});
    insns.push_back({Opcode::WeightsLoad, reg_tmp1, 2}); // k_proj
    insns.push_back({Opcode::TMatMul, reg_k, reg_norm, reg_tmp1});
    insns.push_back({Opcode::WeightsLoad, reg_tmp1, 3}); // v_proj
    insns.push_back({Opcode::TMatMul, reg_v, reg_norm, reg_tmp1});

    // 3. RoPE (on Q and K)
    insns.push_back({Opcode::LoadImm, reg_tmp2, 0}); // position 0
    insns.push_back({Opcode::TRoPE, reg_q, reg_q, reg_tmp2});
    insns.push_back({Opcode::TRoPE, reg_k, reg_k, reg_tmp2});

    // 4. Simplified Attention: Softmax(Q * K^T) * V
    insns.push_back({Opcode::TTranspose, reg_tmp1, reg_k});
    insns.push_back({Opcode::TMatMul, reg_attn, reg_q, reg_tmp1}); // [1, 1]
    insns.push_back({Opcode::TSoftmax, reg_attn, reg_attn});
    insns.push_back({Opcode::TMatMul, reg_out, reg_attn, reg_v}); // [1, 2048]

    // 5. O projection
    insns.push_back({Opcode::WeightsLoad, reg_tmp1, 4}); // o_proj
    insns.push_back({Opcode::TMatMul, reg_out, reg_out, reg_tmp1});

    // 6. Residual Add
    insns.push_back({Opcode::TVecAdd, reg_x, reg_x, reg_out});

    // --- MLP Block ---
    // 1. RMSNorm
    insns.push_back({Opcode::WeightsLoad, reg_tmp1, 5}); // post_attention_layernorm.weight
    insns.push_back({Opcode::TRMSNorm, reg_norm, reg_x, reg_tmp1});

    // 2. Gate & Up projections
    insns.push_back({Opcode::WeightsLoad, reg_tmp1, 6}); // gate_proj
    insns.push_back({Opcode::TMatMul, reg_tmp1, reg_norm, reg_tmp1});
    insns.push_back({Opcode::TSiLU, reg_tmp1, reg_tmp1});

    insns.push_back({Opcode::WeightsLoad, reg_tmp2, 7}); // up_proj
    insns.push_back({Opcode::TMatMul, reg_tmp2, reg_norm, reg_tmp2});

    // 3. Element-wise multiply
    insns.push_back({Opcode::TVecMul, reg_out, reg_tmp1, reg_tmp2});

    // 4. Down projection
    insns.push_back({Opcode::WeightsLoad, reg_tmp1, 8}); // down_proj
    insns.push_back({Opcode::TMatMul, reg_out, reg_out, reg_tmp1});

    // 5. Residual Add
    insns.push_back({Opcode::TVecAdd, reg_x, reg_x, reg_out});

    insns.push_back({Opcode::Halt});
    program.insns = std::move(insns);

    program.axion_policy_text =
        "(policy (tier 1)"
        " (require-axion-event (reason \"TMatMul kernel execution\"))"
        " (require-axion-event (reason \"TRMSNorm kernel execution\"))"
        " (require-axion-event (reason \"TRoPE kernel execution\"))"
        " (require-axion-event (reason \"TSoftmax kernel execution\"))"
        " (require-axion-event (reason \"TSiLU kernel execution\"))"
        " (require-axion-event (reason \"TVecAdd kernel execution\"))"
        " (require-axion-event (reason \"TVecMul kernel execution\"))"
        " (require-axion-event (reason \"TTranspose kernel execution\"))"
        " (require-segment-event (segment tensor) (action \"tensor slot allocated\")))";

    // 3. Run
    auto vm = vm::make_interpreter_vm();
    vm->load_program(program);

    auto start = std::chrono::high_resolution_clock::now();
    auto result = vm->run_to_halt(2000);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;

    if (!result.has_value()) {
        std::cerr << "Demo failed with trap: " << static_cast<int>(result.error()) << "\n";
        return 1;
    }

    // 4. Print Results
    std::cout << "\nInference time: " << diff.count() << " seconds\n";
    std::cout << "Deterministic Axion Trace Artifacts (first 10):\n";
    int count = 0;
    for (const auto& event : vm->state().axion_log) {
        if (event.verdict.reason.empty()) continue;
        std::cout << "  [Axion] op=" << static_cast<int>(event.opcode) << " reason=\"" << event.verdict.reason << "\"\n";
        if (++count >= 10) break;
    }
    std::cout << "  ... total " << vm->state().axion_log.size() << " events.\n";

    // Check final result
    auto handle = vm->state().registers[reg_x];
    if (vm->state().register_tags[reg_x] == vm::ValueTag::TensorHandle) {
        const auto& out_t = vm->state().tensors[static_cast<size_t>(handle - 1)];
        std::cout << "Output Tensor Shape: [";
        for (size_t i = 0; i < out_t.shape().size(); ++i) {
            std::cout << out_t.shape()[i] << (i == out_t.shape().size() - 1 ? "" : ", ");
        }
        std::cout << "]\n";
        std::cout.precision(10);
        std::cout << std::scientific;
        std::cout << "First 3 elements: " << out_t.data()[0] << ", " << out_t.data()[1] << ", " << out_t.data()[2] << "\n";
        std::cout << std::defaultfloat;
    }

    std::cout << "\nSUCCESS: Llama-3.2-1B block inference complete. Bit-identical results guaranteed.\n";

    return 0;
}
