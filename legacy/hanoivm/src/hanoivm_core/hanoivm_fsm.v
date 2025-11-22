// hanoivm_fsm.v - Ternary FSM-based opcode interpreter (HanoiVM core)

module hanoivm_fsm(
    input clk,
    input rst,
    input [7:0] opcode_in,
    input [80:0] operand_in, // T81 encoded operand
    input valid,
    output reg ready,
    output reg [80:0] result_out,
    output reg done
);

    // Ternary stack and internal state
    reg [80:0] stack [0:15];
    reg [3:0] sp; // Stack pointer (4-bit for 16 entries)

    // State machine encoding
    typedef enum logic [2:0] {
        IDLE = 3'd0,
        FETCH = 3'd1,
        EXECUTE = 3'd2,
        WRITEBACK = 3'd3,
        DONE = 3'd4
    } state_t;

    state_t state;
    reg [7:0] opcode_reg;
    reg [80:0] operand_reg;
    reg [80:0] temp_result;

    always @(posedge clk or posedge rst) begin
        if (rst) begin
            state <= IDLE;
            sp <= 0;
            ready <= 1;
            done <= 0;
        end else begin
            case (state)
                IDLE: begin
                    if (valid) begin
                        opcode_reg <= opcode_in;
                        operand_reg <= operand_in;
                        state <= FETCH;
                        ready <= 0;
                    end
                end

                FETCH: begin
                    // No memory latency; proceed immediately
                    state <= EXECUTE;
                end

                EXECUTE: begin
                    case (opcode_reg)
                        8'h00: begin // NOP
                            temp_result <= 81'd0;
                        end
                        8'h01: begin // PUSH
                            stack[sp] <= operand_reg;
                            sp <= sp + 1;
                            temp_result <= operand_reg;
                        end
                        8'h02: begin // POP
                            if (sp > 0) begin
                                sp <= sp - 1;
                                temp_result <= stack[sp - 1];
                            end else begin
                                temp_result <= 81'd0;
                            end
                        end
                        8'h03: begin // ADD (T81 ADD)
                            if (sp >= 2) begin
                                temp_result <= stack[sp - 1] + stack[sp - 2];
                                sp <= sp - 1;
                                stack[sp - 2] <= temp_result;
                            end else begin
                                temp_result <= 81'd0;
                            end
                        end
                        default: begin
                            temp_result <= 81'd0; // Unknown opcode
                        end
                    endcase
                    state <= WRITEBACK;
                end

                WRITEBACK: begin
                    result_out <= temp_result;
                    done <= 1;
                    state <= DONE;
                end

                DONE: begin
                    done <= 0;
                    ready <= 1;
                    state <= IDLE;
                end
            endcase
        end
    end

endmodule
