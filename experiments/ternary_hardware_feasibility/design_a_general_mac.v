// Design A: General Binary INT8 Multiply-Accumulate (MAC) Datapath
// Performs exact signed INT8 x INT8 multiplication followed by exact reduction tree accumulation.

`timescale 1ns/1ps

module design_a_general_mac #(
    parameter integer N = 64,
    parameter integer ACC_WIDTH = 32
)(
    input  wire [N*8-1:0]         X_flat, // N x 8-bit signed INT8 activations
    input  wire [N*8-1:0]         W_flat, // N x 8-bit signed INT8 weights (-1, 0, +1)
    output wire signed [ACC_WIDTH-1:0] Y       // Exact accumulated sum
);

    // Step 1: Multiply 8-bit signed activations by 8-bit signed weights
    wire signed [ACC_WIDTH-1:0] products [0:N-1];

    genvar i;
    generate
        for (i = 0; i < N; i = i + 1) begin : gen_mult
            wire signed [7:0] x_elem = X_flat[i*8 +: 8];
            wire signed [7:0] w_elem = W_flat[i*8 +: 8];
            wire signed [15:0] prod  = x_elem * w_elem;
            assign products[i] = $signed({{ (ACC_WIDTH-16){prod[15]} }, prod});
        end
    endgenerate

    // Step 2: Binary Adder Reduction Tree
    wire signed [ACC_WIDTH-1:0] tree_levels [0:2*N-2];

    generate
        for (i = 0; i < N; i = i + 1) begin : gen_leaves
            assign tree_levels[i] = products[i];
        end
        for (i = 0; i < N - 1; i = i + 1) begin : gen_nodes
            assign tree_levels[N + i] = tree_levels[2*i] + tree_levels[2*i + 1];
        end
    endgenerate

    assign Y = tree_levels[2*N - 2];

endmodule
