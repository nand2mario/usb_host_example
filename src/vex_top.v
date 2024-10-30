module vex_top (
    input             clk_sys,
    input             s1,

    inout             usb_dm,
    inout             usb_dp,
    output       [3:0] test_pins,

    input             uart_rx,
    output            uart_tx,

    output      [3:0] led
);

`ifndef VERILATOR
`include "pll.vh"
`else
wire clk = clk_sys;
`endif

reg [15:0] resetcnt = 65535;
reg resetn = 0;


always @(posedge clk) begin
    resetcnt <= resetcnt == 0 ? 0 : resetcnt - 1;
`ifndef VERILATOR
    if (resetcnt == 0 && s1)       // nano/primer
//    if (~s1)        // mega
`endif
        resetn <= 1;
end

// clk is 48Mhz
iosys2 u_iosys (
    .clk(clk), .resetn(resetn), 
    .uart_rx(uart_rx), .uart_tx(uart_tx),
    .usb_dm(usb_dm), .usb_dp(usb_dp)
);

reg [7:0] cnt;

always @(posedge clk) cnt <= cnt + 1;
//assign usb_dp = cnt[3];
//assign usb_dm = ~cnt[3];

reg [20:0] blink;

always @(posedge clk)
    blink <= blink+1;

//assign test_pins = {4{blink[2]}};
//assign usb_dp = blink[2];
//assign usb_dm = blink[2];

assign led = ~{/*usb_dp, usb_dm,*/ ~uart_rx, ~uart_tx};

endmodule