// ==============================================================
// File generated by Vivado(TM) HLS - High-Level Synthesis from C, C++ and SystemC
// Version: 2013.3
// Copyright (C) 2013 Xilinx Inc. All rights reserved.
// 
// ==============================================================

/***************************** Include Files *********************************/
#include "xsobel_ip.h"

/************************** Function Implementation *************************/
#ifndef __linux__
int XSobel_ip_CfgInitialize(XSobel_ip *InstancePtr, XSobel_ip_Config *ConfigPtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(ConfigPtr != NULL);

    InstancePtr->Slv0_BaseAddress = ConfigPtr->Slv0_BaseAddress;
    InstancePtr->IsReady = XIL_COMPONENT_IS_READY;

    return XST_SUCCESS;
}
#endif

void XSobel_ip_Start(XSobel_ip *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XSobel_ip_ReadReg(InstancePtr->Slv0_BaseAddress, XSOBEL_IP_SLV0_ADDR_AP_CTRL) & 0x80;
    XSobel_ip_WriteReg(InstancePtr->Slv0_BaseAddress, XSOBEL_IP_SLV0_ADDR_AP_CTRL, Data | 0x01);
}

u32 XSobel_ip_IsDone(XSobel_ip *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XSobel_ip_ReadReg(InstancePtr->Slv0_BaseAddress, XSOBEL_IP_SLV0_ADDR_AP_CTRL);
    return (Data >> 1) & 0x1;
}

u32 XSobel_ip_IsIdle(XSobel_ip *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XSobel_ip_ReadReg(InstancePtr->Slv0_BaseAddress, XSOBEL_IP_SLV0_ADDR_AP_CTRL);
    return (Data >> 2) & 0x1;
}

u32 XSobel_ip_IsReady(XSobel_ip *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XSobel_ip_ReadReg(InstancePtr->Slv0_BaseAddress, XSOBEL_IP_SLV0_ADDR_AP_CTRL);
    // check ap_start to see if the pcore is ready for next input
    return !(Data & 0x1);
}

void XSobel_ip_EnableAutoRestart(XSobel_ip *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XSobel_ip_WriteReg(InstancePtr->Slv0_BaseAddress, XSOBEL_IP_SLV0_ADDR_AP_CTRL, 0x80);
}

void XSobel_ip_DisableAutoRestart(XSobel_ip *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XSobel_ip_WriteReg(InstancePtr->Slv0_BaseAddress, XSOBEL_IP_SLV0_ADDR_AP_CTRL, 0);
}

void XSobel_ip_InterruptGlobalEnable(XSobel_ip *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XSobel_ip_WriteReg(InstancePtr->Slv0_BaseAddress, XSOBEL_IP_SLV0_ADDR_GIE, 1);
}

void XSobel_ip_InterruptGlobalDisable(XSobel_ip *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XSobel_ip_WriteReg(InstancePtr->Slv0_BaseAddress, XSOBEL_IP_SLV0_ADDR_GIE, 0);
}

void XSobel_ip_InterruptEnable(XSobel_ip *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XSobel_ip_ReadReg(InstancePtr->Slv0_BaseAddress, XSOBEL_IP_SLV0_ADDR_IER);
    XSobel_ip_WriteReg(InstancePtr->Slv0_BaseAddress, XSOBEL_IP_SLV0_ADDR_IER, Register | Mask);
}

void XSobel_ip_InterruptDisable(XSobel_ip *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XSobel_ip_ReadReg(InstancePtr->Slv0_BaseAddress, XSOBEL_IP_SLV0_ADDR_IER);
    XSobel_ip_WriteReg(InstancePtr->Slv0_BaseAddress, XSOBEL_IP_SLV0_ADDR_IER, Register & (~Mask));
}

void XSobel_ip_InterruptClear(XSobel_ip *InstancePtr, u32 Mask) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XSobel_ip_WriteReg(InstancePtr->Slv0_BaseAddress, XSOBEL_IP_SLV0_ADDR_ISR, Mask);
}

u32 XSobel_ip_InterruptGetEnabled(XSobel_ip *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XSobel_ip_ReadReg(InstancePtr->Slv0_BaseAddress, XSOBEL_IP_SLV0_ADDR_IER);
}

u32 XSobel_ip_InterruptGetStatus(XSobel_ip *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XSobel_ip_ReadReg(InstancePtr->Slv0_BaseAddress, XSOBEL_IP_SLV0_ADDR_ISR);
}

