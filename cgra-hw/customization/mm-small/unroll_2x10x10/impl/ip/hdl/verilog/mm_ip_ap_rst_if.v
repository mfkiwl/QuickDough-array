// ==============================================================
// File generated by Vivado(TM) HLS - High-Level Synthesis from C, C++ and SystemC
// Version: 2013.3
// Copyright (C) 2013 Xilinx Inc. All rights reserved.
// 
// ==============================================================

`timescale 1ns/1ps

module mm_ip_ap_rst_if
#(parameter
    RESET_ACTIVE_LOW = 0
)(
    input  wire din,
    output wire dout
);

assign dout = (RESET_ACTIVE_LOW == 1)? ~din : din;

endmodule

