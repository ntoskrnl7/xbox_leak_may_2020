        TITLE   "System Management Bus Interupt Service Routine"
;++
;
;  Copyright (c) 2001  Microsoft Corporation
;
;  Module Name:
;
;     smbinta.asm
;
;  Abstract:
;
;     This module implements the routines to handle a SMBus interrupt.
;
;  Environment:
;
;     Kernel mode only.
;
;--

        .586p
        .xlist
INCLUDE ks386.inc
INCLUDE callconv.inc
INCLUDE i386\kimacro.inc
INCLUDE i386\ix8259.inc
INCLUDE i386\mcpxsmb.inc
        .list

        EXTRNP  HalBeginSMBusInterrupt,2,,FASTCALL
        EXTRNP  HalEndSystemLevelInterrupt,1,,FASTCALL
        EXTRNP  _KeInsertQueueDpc,3
        EXTRNP  _KeStallExecutionProcessor,1
        EXTRN   _KiPCR:DWORD
        EXTRN   _HalpSMBusInterruptDpc:DWORD

_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;++
;
; VOID
; HalpSMBusInterrupt(
;     VOID
;     )
;
; Routine Description:
;
;    This routine is entered as the result of an interrupt generated by the
;    SMBus controller.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    None.
;
;--
cPublicProc _HalpSMBusInterrupt, 0

        ENTER_INTERRUPT

        mov     ecx, SMBUS_VECTOR - PRIMARY_VECTOR_BASE
        mov     edx, SMBUS_LEVEL
        push    0                       ; allocate space to save OldIrql
        fstCall HalBeginSMBusInterrupt

;
; Read the status from the SMBus controller and dismiss the interrupt by writing
; the status back out the SMBus controller.
;

        mov     edx, XPCICFG_SMBUS_IO_REGISTER_BASE_1 + MCPX_SMBUS_HOST_STATUS_REGISTER
        in      al, dx
        out     dx, al

;
; Queue a DPC to finish processing of the transaction.  Pass the SMBus status as
; the first parameter to the DPC.
;

        stdCall _KeInsertQueueDpc,<offset _HalpSMBusInterruptDpc, eax, 0>

        mov     eax, SMBUS_VECTOR - PRIMARY_VECTOR_BASE
        LEVEL_INTERRUPT_EXIT

stdENDP _HalpSMBusInterrupt

_TEXT   ends
        end
