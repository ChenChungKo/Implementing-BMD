module top(input [3:0] x, input [3:0] y, output signed [4:0] z);
assign z = x - y;
endmodule
