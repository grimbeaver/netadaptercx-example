/*++

Module Name:

    device.h

Abstract:

    This file contains the device definitions.

Environment:

    Kernel-mode Driver Framework

--*/

#include "public.h"

EXTERN_C_START

constexpr auto MAX_ADAPTERS = 1;

//
// A DMA memory space
//
typedef struct _DMA_MEMORY
{
    WDFCOMMONBUFFER       CommonBuffer;
    size_t                BufferSize;
    PHYSICAL_ADDRESS      PhysicalBase;
    volatile PUCHAR       VirtualBase;
} DMA_MEMORY, * PDMA_MEMORY;

//
// The net adapter context per port
//
typedef struct _ADAPTER_CONTEXT
{
    WDFDEVICE Device;

    NETPACKETQUEUE TransmitQueue;
    NETPACKETQUEUE ReceiveQueue;

    NET_ADAPTER_LINK_LAYER_ADDRESS PermanentAddress;
    NET_ADAPTER_LINK_LAYER_ADDRESS CurrentAddress;
} ADAPTER_CONTEXT, * PADAPTER_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(ADAPTER_CONTEXT, AdapterGetContext)

//
// The device context performs the same job as
// a WDM device extension in the driver frameworks
//
typedef struct _DEVICE_CONTEXT
{
    WDFDMAENABLER DmaEnabler;
    NETADAPTER NetAdapter;
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

//
// This macro will generate an inline function called DeviceGetContext
// which will be used to get a pointer to the device context memory
// in a type safe manner.
//
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext)

//
// Function to initialize the device and its callbacks
//
NTSTATUS
NetAdapterCxExampleCreateDevice(
    _Inout_ PWDFDEVICE_INIT DeviceInit
    );

EVT_WDF_DEVICE_PREPARE_HARDWARE NetAdapterCxExamplePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE NetAdapterCxExampleReleaseHardware;

EXTERN_C_END
