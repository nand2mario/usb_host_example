// VexRiscv SoC with USB Host Controller
// This version uses 128KB of block ram as memory
module iosys2 (
    input             clk,
    input             resetn,

    // UART
    input             uart_rx,
    output            uart_tx,

    // USB
    inout             usb_dm,       // USB D-
    inout             usb_dp        // USB D+

    // SD card
    // output sd_clk,
    // inout  sd_cmd,                  // MOSI
    // input  sd_dat0,                 // MISO
    // output sd_dat1,                 // 1
    // output sd_dat2,                 // 1
    // output sd_dat3                  // 0 for SPI mode
);

// 64KB RAM -------------------------------------------------------------------------------
localparam string ROM_FILE = "firmware.hex";
reg [31:0] ram [0:16*1024-1];
initial
    $readmemh(ROM_FILE, ram);

wire [3:0] mem_wstrb = !dbus_cmd_wr ? 4'b0 :
                       dbus_cmd_size == 2'b0 ? (4'b1 << dbus_cmd_a[1:0]) :
                       dbus_cmd_size == 2'b1 ? (4'b11 << dbus_cmd_a[1:0]) : 4'b1111;
reg [31:0] mem_rdata;

always @(posedge clk) begin
    if (dbus_cmd_valid && dbus_cmd_wr && dbus_cmd_a[27:24] != 4'h2) begin
        if (mem_wstrb[0])
            ram[dbus_cmd_a[22:2]][ 7: 0] <= dbus_cmd_data[ 7: 0];
        if (mem_wstrb[1])
            ram[dbus_cmd_a[22:2]][15: 8] <= dbus_cmd_data[15: 8];
        if (mem_wstrb[2])
            ram[dbus_cmd_a[22:2]][23:16] <= dbus_cmd_data[23:16];
        if (mem_wstrb[3])
            ram[dbus_cmd_a[22:2]][31:24] <= dbus_cmd_data[31:24];
    end
end

always @(posedge clk) begin
    mem_rdata <= ram[dbus_cmd_valid ? dbus_cmd_a[22:2] : ibus_cmd_pc[22:2]];
end

// Bus multiplex ---------------------------------------------------------------------------
// DBus is acked (dbus_cmd_ready=1) in the same cycle when dbus_cmd_valid=1. 
// DBus read result is returned one cycle later (dbus_rsp_ready=1).
// DBus supports both memory accesses and MMIO accesses.
// IBus is executed in the same manner with lower priority, i.e. when DBus is idle.
wire        ibus_cmd_valid;
reg         ibus_cmd_ready;
wire [31:0] ibus_cmd_pc;
reg         ibus_rsp_ready;

wire        dbus_cmd_valid;
reg         dbus_cmd_ready;
wire        dbus_cmd_wr;
wire [31:0] dbus_cmd_a;
wire [31:0] dbus_cmd_data;
wire [1:0]  dbus_cmd_size;
reg         dbus_rsp_ready;

// MMIO registers
wire        simpleuart_reg_div_sel = dbus_cmd_valid && (dbus_cmd_a == 32'h 0200_0010);
wire [31:0] simpleuart_reg_div_do;

wire        simpleuart_reg_dat_sel = dbus_cmd_valid && (dbus_cmd_a == 32'h 0200_0014);
wire [31:0] simpleuart_reg_dat_do;
wire        simpleuart_reg_dat_wait;
reg         simpleuart_reg_dat_sel_r;
reg [31:0]  simpleuart_reg_dat_do_r;

wire        time_reg_sel = dbus_cmd_valid && (dbus_cmd_a == 32'h0200_0050);        // milli-seconds since start-up (overflows in 49 days)
wire        cycle_reg_sel = dbus_cmd_valid && (dbus_cmd_a == 32'h0200_0054);       // cycles counter (overflows every 200 seconds)
reg         time_reg_sel_r, cycle_reg_sel_r;

always @(posedge clk) begin
    simpleuart_reg_dat_sel_r <= simpleuart_reg_dat_sel;
    simpleuart_reg_dat_do_r <= simpleuart_reg_dat_do;
    time_reg_sel_r <= time_reg_sel;
    cycle_reg_sel_r <= cycle_reg_sel;
end

// Interface to USB host controller
wire [31:0] usb_rdata;
wire        usb_sel = dbus_cmd_valid && (dbus_cmd_a[31:8] == 24'h0200_01);
wire        usb_ack;

assign      dbus_cmd_ready = simpleuart_reg_dat_sel ? !simpleuart_reg_dat_wait : 
                             usb_sel ? usb_ack :
                             dbus_cmd_valid;
assign      ibus_cmd_ready = ~dbus_cmd_valid & ibus_cmd_valid;

reg         dbus_mem_ready, dbus_usb_ready;
wire [31:0] dbus_rdata = dbus_mem_ready ? mem_rdata :
                         dbus_usb_ready ? usb_rdata :
                         simpleuart_reg_dat_sel_r ? simpleuart_reg_dat_do_r : 
                         time_reg_sel_r ? time_reg :
                         cycle_reg_sel_r ? cycle_reg :
                         32'h0;

always @(posedge clk) begin
    dbus_rsp_ready <= 0;
    ibus_rsp_ready <= 0;
    dbus_mem_ready <= 0;
    dbus_usb_ready <= 0;
    if (dbus_cmd_valid) begin
        if (dbus_cmd_ready && !dbus_cmd_wr) begin       // dbus read, send response next cycle
            dbus_rsp_ready <= 1;
            dbus_mem_ready <= dbus_cmd_a[27:24] != 4'h2;
            dbus_usb_ready <= usb_sel;
        end
    end else if (ibus_cmd_valid)                        // instruction fetch takes one cycle
        ibus_rsp_ready <= 1;
end

// VexRiscv cpu ---------------------------------------------------------------------------
VexRiscv rv (
    .clk(clk), .reset(~resetn),
    // iBus cmd stream
    .iBus_cmd_valid(ibus_cmd_valid), .iBus_cmd_ready(ibus_cmd_ready), .iBus_cmd_payload_pc(ibus_cmd_pc), 
    // iBus rsp stream
    .iBus_rsp_valid(ibus_rsp_ready), .iBus_rsp_payload_error(0), .iBus_rsp_payload_inst(mem_rdata),
    // dBus cmd stream
    .dBus_cmd_valid(dbus_cmd_valid), .dBus_cmd_ready(dbus_cmd_ready), .dBus_cmd_payload_wr(dbus_cmd_wr), 
    .dBus_cmd_payload_address(dbus_cmd_a), .dBus_cmd_payload_data(dbus_cmd_data), .dBus_cmd_payload_size(dbus_cmd_size),
    // dBus rsp stream
    .dBus_rsp_ready(dbus_rsp_ready), .dBus_rsp_error(0), .dBus_rsp_data(dbus_rdata)
);

// uart @ 0x0200_0010
simpleuart simpleuart (
    .clk(clk), .resetn(resetn),
    .ser_tx(uart_tx), .ser_rx(uart_rx),

    .reg_div_we  (simpleuart_reg_div_sel ? mem_wstrb : 4'b0),
    .reg_div_di  (dbus_cmd_data),
    .reg_div_do  (simpleuart_reg_div_do),

    .reg_dat_we  (simpleuart_reg_dat_sel ? mem_wstrb[0] : 1'b0),
    .reg_dat_re  (simpleuart_reg_dat_sel && mem_wstrb == 4'b0),
    .reg_dat_di  (dbus_cmd_data),
    .reg_dat_do  (simpleuart_reg_dat_do),
    .reg_dat_wait(simpleuart_reg_dat_wait)
);

// Time counter register
localparam FREQ = 48000000;
reg [31:0] time_reg, cycle_reg;
reg [$clog2(FREQ/1000)-1:0] time_cnt;
always @(posedge clk) begin
    if (~resetn) begin
        time_reg <= 0;
        time_cnt <= 0;
    end else begin
        cycle_reg <= cycle_reg + 1;
        time_cnt <= time_cnt + 1;
`ifndef VERILATOR
        if (time_cnt == FREQ/1000-1)
`else
        if (time_cnt == 15)          // 16 cycles per "ms" for verilator
`endif
         begin
            time_cnt <= 0;
            time_reg <= time_reg + 1;
        end
    end
end

logic  [7:0]           utmi_data_in_i;
logic                  utmi_txready_i;
logic                  utmi_rxvalid_i;
logic                  utmi_rxactive_i;
logic                  utmi_rxerror_i;
logic  [1:0]           utmi_linestate_i;

logic [7:0]            utmi_data_out_o;
logic                  utmi_txvalid_o;
logic [1:0]            utmi_op_mode_o;
logic [1:0]            utmi_xcvrselect_o;
logic                  utmi_termselect_o;
logic                  utmi_dppulldown_o;
logic                  utmi_dmpulldown_o;
logic                  usb_pads_tx_dp_w;
logic                  usb_pads_tx_oen_w;
logic                  usb_pads_rx_dn_w;
logic                  usb_pads_tx_dn_w;
logic                  usb_pads_rx_rcv_w;
logic                  usb_pads_rx_dp_w;
logic                  usb_xcvr_mode_w = 1'h1;

usbh_core u_usb (
    .clk_i(clk) ,.rstn_i(resetn),
    .reg_cs(usb_sel), .reg_wr(usb_sel & dbus_cmd_wr), .reg_addr(dbus_cmd_a[7:0]), 
    .reg_wdata(dbus_cmd_data), .reg_be(mem_wstrb), 
    .reg_rdata(usb_rdata), .reg_ack(usb_ack),

    .intr_o              (),

    .utmi_data_in_i      (utmi_data_in_i      ),
    .utmi_rxvalid_i      (utmi_rxvalid_i      ),
    .utmi_rxactive_i     (utmi_rxactive_i     ),
    .utmi_rxerror_i      (utmi_rxerror_i      ),
    .utmi_linestate_i    (utmi_linestate_i    ),

    .utmi_txready_i      (utmi_txready_i      ),
    .utmi_data_out_o     (utmi_data_out_o     ),
    .utmi_txvalid_o      (utmi_txvalid_o      ),

    .utmi_op_mode_o      (utmi_op_mode_o      ),
    .utmi_xcvrselect_o   (utmi_xcvrselect_o   ),
    .utmi_termselect_o   (utmi_termselect_o   ),
    .utmi_dppulldown_o   (utmi_dppulldown_o   ),
    .utmi_dmpulldown_o   (utmi_dmpulldown_o   )
);

usb_fs_phy  u_phy(
    // Inputs
    .clk_i               (clk           ),
    .rstn_i              (resetn          ),
    .utmi_data_out_i     (utmi_data_out_o     ),
    .utmi_txvalid_i      (utmi_txvalid_o      ),
    .utmi_op_mode_i      (utmi_op_mode_o      ),
    .utmi_xcvrselect_i   (utmi_xcvrselect_o   ),
    .utmi_termselect_i   (utmi_termselect_o   ),
    .utmi_dppulldown_i   (utmi_dppulldown_o   ),
    .utmi_dmpulldown_i   (utmi_dmpulldown_o   ),
    .usb_rx_rcv_i        (usb_pads_rx_rcv_w   ),
    .usb_rx_dp_i         (usb_pads_rx_dp_w    ),
    .usb_rx_dn_i         (usb_pads_rx_dn_w    ),
    .usb_reset_assert_i  ( 1'b0               ),

    // Outputs
    .utmi_data_in_o     (utmi_data_in_i      ),
    .utmi_txready_o     (utmi_txready_i      ),
    .utmi_rxvalid_o     (utmi_rxvalid_i      ),
    .utmi_rxactive_o    (utmi_rxactive_i     ),
    .utmi_rxerror_o     (utmi_rxerror_i      ),
    .utmi_linestate_o   (utmi_linestate_i    ),
    .usb_tx_dp_o        (usb_pads_tx_dp_w    ),
    .usb_tx_dn_o        (usb_pads_tx_dn_w    ),
    .usb_tx_oen_o       (usb_pads_tx_oen_w   ),
    .usb_reset_detect_o (                    ),
    .usb_en_o           (                    )
);

wire out_dp, out_dn, out_tx_oen;

usb_transceiver u_usb_xcvr (
    // Inputs
    .usb_phy_tx_dp_i    (usb_pads_tx_dp_w   ),
    .usb_phy_tx_dn_i    (usb_pads_tx_dn_w   ),
    .usb_phy_tx_oen_i   (usb_pads_tx_oen_w  ),
    .mode_i             (usb_xcvr_mode_w    ),

    .out_dp             (out_dp             ),
    .out_dn             (out_dn             ),
    .out_tx_oen         (out_tx_oen         ),

`ifdef VERILATOR
    .in_dp              (1'b1             ),    // J for simulation
    .in_dn              (1'b0             ),
`else
    .in_dp              (usb_dp             ),
    .in_dn              (usb_dm             ),
`endif

    // Outputs
    .usb_phy_rx_rcv_o  (usb_pads_rx_rcv_w   ),
    .usb_phy_rx_dp_o   (usb_pads_rx_dp_w    ),
    .usb_phy_rx_dn_o   (usb_pads_rx_dn_w    )
);

assign usb_dp = out_tx_oen ? 1'bZ : out_dp;
assign usb_dm = out_tx_oen ? 1'bZ : out_dn;

endmodule