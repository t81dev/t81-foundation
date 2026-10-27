// Design B: Conventional Binary Specialized Ternary-Weight MAC Datapath
// Optimized conventional binary logic specialized for weights restricted to {-1, 0, +1}.
// Encodes weights using standard 2-bit binary representation:
//   2'b00 = 0
//   2'b01 = +1
//   2'b11 = -1
// Uses conditional negation, zero-gating, and binary reduction tree.

`timescale 1ns/1ps

module design_b_optimized_binary #(
    parameter integer N = 64,
    parameter integer ACC_WIDTH = 32
)(
    input  wire [N*8-1:0]         X_flat, // N x 8-bit signed INT8 activations
    input  wire [N*2-1:0]         W_flat, // N x 2-bit weight code (2'b00: 0, 2'b01: +1, 2'b11: -1)
    output wire signed [ACC_WIDTH-1:0] Y       // Exact accumulated sum
);

    wire signed [ACC_WIDTH-1:0] terms [0:N-1];

    genvar i;
    generate
        for (i = 0; i < N; i = i + 1) begin : gen_term
            wire signed [7:0] x_elem = X_flat[i*8 +: 8];
            wire [1:0]        w_code = W_flat[i*2 +: 2];

            // Sign extend X to full accumulator width first (prevents INT8_MIN negation overflow)
            wire signed [ACC_WIDTH-1:0] x_ext = $signed({{ (ACC_WIDTH-8){x_elem[7]} }, x_elem});

            // Conditional negation:
            wire is_neg  = (w_code == 2'b11);
            wire is_zero = (w_code == 2'b00);

            wire signed [ACC_WIDTH-1:0] x_neg = -x_ext;
            wire signed [ACC_WIDTH-1:0] x_selected = is_neg ? x_neg : x_ext;

            assign terms[i] = is_zero ? {ACC_WIDTH{1'b0}} : x_selected;
        end
    endgenerate

    // Adder Reduction Tree
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
