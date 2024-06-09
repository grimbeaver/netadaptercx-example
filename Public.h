/*++

Module Name:

    public.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications.

Environment:

    user and kernel

--*/

//
// Define an Interface Guid so that apps can find the device and talk to it.
//

DEFINE_GUID (GUID_DEVINTERFACE_NetAdapterCxExample,
    0xdecb6b91,0xfa54,0x47f4,0xa0,0x3a,0xc6,0x9c,0xf7,0x07,0x19,0xe4);
// {decb6b91-fa54-47f4-a03a-c69cf70719e4}
