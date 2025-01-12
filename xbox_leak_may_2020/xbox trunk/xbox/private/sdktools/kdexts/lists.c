/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Lists.c

Abstract:

    WinDbg Extension Api

Author:

    Gary Kimura [GaryKi]    25-Mar-96

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#define ReadAtAddress(A,V,S,R)                                   \
    (ReadMemory( (ULONG)(A), &(V), (S), &R ) && (R >= (S)))

#define BIG_READ    (sizeof(ULONG) * 4)
#define SMALL_READ  (sizeof(ULONG) * 2)

VOID
DumpListByLinks (
    IN ULONG StartAddress,
    IN ULONG MaxCount,
    IN ULONG Bias,
    IN LOGICAL UseFlink
    )

/*++

Routine Description:

    Dump a list by its blinks.

Arguments:

    arg - [Address] [count] [bias]

Return Value:

    None

--*/

{
    ULONG Address;
    ULONG Buffer[4];
    ULONG ReadSize;
    ULONG BytesRead;
    ULONG Count;

    //
    //  set our starting address and then while the count is greater than zero
    //  and the starting address is not equal to the current dumping address
    //  we'll read in 4 ulongs, dump them, and then go through the flink&blink
    //  using the specified bias.
    //

    Address = StartAddress;

    ReadSize = BIG_READ;

    for (Count = 0; Count < MaxCount; ) {

        if (ReadAtAddress( Address, Buffer, ReadSize, BytesRead ) == 0) {
            ReadSize = SMALL_READ;
            if (ReadAtAddress( Address, Buffer, ReadSize, BytesRead ) == 0) {
                dprintf("Can't Read Memory at %08lx\n", Address);
                break;
            }
        }

        if (ReadSize == BIG_READ) {
            dprintf("%08lx  %08lx %08lx %08lx %08lx\n", Address, Buffer[0], Buffer[1], Buffer[2], Buffer[3]);
        }
        else {
            dprintf("%08lx  %08lx %08lx\n", Address, Buffer[0], Buffer[1]);
        }

        Count += 1;

        //
        //  the bias tells us which bits to knock out of the pointer
        //

        if (UseFlink == TRUE) {
            Address = Buffer[0] & ~Bias;
        }
        else {
            Address = Buffer[1] & ~Bias;
        }

        if (Address == StartAddress) {
            break;
        }

        if (((Count & 0xf) == 0) && CheckControlC() ) {
            break;
        }
    }

    if (Count != 0) {
        dprintf("0x%x entries dumped\n", Count);
    }

    return;
}


DECLARE_API( dflink )

/*++

Routine Description:

    Dump a list by its flinks.

Arguments:

    arg - [Address] [count] [bias]

Return Value:

    None

--*/

{
    ULONG StartAddress;
    ULONG Count;
    ULONG Bias;

    StartAddress = 0;
    Count = 0x20;
    Bias = 0;

    //
    //  read in the parameters
    //

    sscanf(args,"%lx %lx %lx",&StartAddress, &Count, &Bias);

    DumpListByLinks (StartAddress, Count, Bias, TRUE);

    return;
}


DECLARE_API( dblink )

/*++

Routine Description:

    Dump a list by its blinks.

Arguments:

    arg - [Address] [count] [bias]

Return Value:

    None

--*/

{
    ULONG StartAddress;
    ULONG Count;
    ULONG Bias;

    StartAddress = 0;
    Count = 0x20;
    Bias = 0;

    //
    //  read in the parameters
    //

    sscanf(args,"%lx %lx %lx",&StartAddress, &Count, &Bias);

    DumpListByLinks (StartAddress, Count, Bias, FALSE);

    return;
}
