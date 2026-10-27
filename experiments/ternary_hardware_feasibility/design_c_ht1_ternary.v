// Design C: Proposed T81 H-T1 Ternary-Weight Datapath (PR #594 Specification)
// Custom bit-parallel unpacked ternary hardware overlay using separate Sign and Mask bitplanes:
//   W_mask[i] = 1 if W[i] != 0 else 0
//   W_sign[i] = 1 if W[i] == -1 else 0
// Datapath logic per element:
//   1. X_ext[i] = SignExtend_32(X[i])
//   2. M_i = {32{W_sign[i]}}
//   3. T_signed[i] = (X_ext[i] ^ M_i) + W_sign[i]
//   4. T[i] = {32{W_mask[i]}} & T_signed[i]
//   5. Y = sum_{i=0}^{N-1} T[i]

`timescale 1ns/1ps

module design_c_ht1_ternary #(
    parameter integer N = 64,
    parameter integer ACC_WIDTH = 32
)(
    input  wire [N*8-1:0]         X_flat, // N x 8-bit signed INT8 activations
    input  wire [N-1:0]           W_mask, // N x 1-bit non-zero mask bitplane
    input  wire [N-1:0]           W_sign, // N x 1-bit sign bitplane (1 = -1, 0 = +1)
    output wire signed [ACC_WIDTH-1:0] Y       // Exact accumulated sum
);

    wire signed [ACC_WIDTH-1:0] terms [0:N-1];

    genvar i;
    generate
        for (i = 0; i < N; i = i + 1) begin : gen_ht1_term
            wire signed [7:0] x_elem = X_flat[i*8 +: 8];
            wire w_m = W_mask[i];
            wire w_s = W_sign[i];

            // Step 1: Sign Extension to ACC_WIDTH
            wire [ACC_WIDTH-1:0] x_ext = {{(ACC_WIDTH-8){x_elem[7]}}, x_elem};

            // Step 2 & 3: Bit-parallel XOR mask and conditional increment (+1 for two's complement negation)
            wire [ACC_WIDTH-1:0] mask = {ACC_WIDTH{w_s}};
            wire [ACC_WIDTH-1:0] t_signed = (x_ext ^ mask) + w_s;

            // Step 4: Bitwise AND zero gating
            wire [ACC_WIDTH-1:0] t_masked = {ACC_WIDTH{w_m}} & t_signed;

            assign terms[i] = $signed(t_masked);
        end
    endgenerate

    // Step 5: Adder Reduction Tree
    wire signed [ACC_WIDTH-1:0] tree_levels [0:2*N-2];

    generate
        for (i = 0; i < N; i = i + 1) begin : gen_leaves
            assign tree_levels[i] = terms[i];
        end
        for (i = 0; i < N - 1; i = i + 1) begin : gen_nodes
            assign tree_levels[N + i] = tree_levels[2*i] + tree_levels[2*i + 1];
        end
    endgenerate

    assign Y = tree_levels[2*N - 2];

endmodule
