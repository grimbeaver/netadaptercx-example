/*++

Module Name:

    device.c - Device handling events for example driver.

Abstract:

   This file contains the device entry points and callbacks.
    
Environment:

    Kernel-mode Driver Framework

--*/

#include "driver.h"
#include "device.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, NetAdapterCxExampleCreateDevice)
#pragma alloc_text (PAGE, NetAdapterCxExamplePrepareHardware)
#pragma alloc_text (PAGE, NetAdapterCxExampleReleaseHardware)
#endif

NTSTATUS
NetAdapterCxExampleCreateDevice(
    _Inout_ PWDFDEVICE_INIT DeviceInit
    )
/*++

Routine Description:

    Worker routine called to create a device and its software resources.

Arguments:

    DeviceInit - Pointer to an opaque init structure. Memory for this
                    structure will be freed by the framework when the WdfDeviceCreate
                    succeeds. So don't access the structure after that point.

Return Value:

    NTSTATUS

--*/
{
    WDF_OBJECT_ATTRIBUTES deviceAttributes;
    WDF_OBJECT_ATTRIBUTES adapterAttributes;
    PDEVICE_CONTEXT deviceContext;
    PADAPTER_CONTEXT adapterContext;
    WDFDEVICE device;
    NTSTATUS status;
    WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, DEVICE_CONTEXT);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&adapterAttributes, ADAPTER_CONTEXT);
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);

    status = NetDeviceInitConfig(DeviceInit);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "NetDeviceInitConfig failed %!STATUS!", status);
        return status;
    }

    pnpPowerCallbacks.EvtDevicePrepareHardware = NetAdapterCxExamplePrepareHardware;
    pnpPowerCallbacks.EvtDeviceReleaseHardware = NetAdapterCxExampleReleaseHardware;
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

    status = WdfDeviceCreate(&DeviceInit, &deviceAttributes, &device);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "WdfDeviceCreate failed %!STATUS!", status);
        return status;
    }

    // Get a pointer to the device context structure.
    deviceContext = DeviceGetContext(device);

    // Create a net adapter
    // Allocate the net adapter initialization structure
    auto adapterInit = NetAdapterInitAllocate(device);
    if (adapterInit == NULL)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "NetAdapterInitAllocate failed %!STATUS!", status);
        return status;
    }

    // Datapath callbacks for creating packet queues
    NET_ADAPTER_DATAPATH_CALLBACKS datapathCallbacks;
    NET_ADAPTER_DATAPATH_CALLBACKS_INIT(&datapathCallbacks,
        NetAdapterCxExampleCreateTxQueue,
        NetAdapterCxExampleCreateRxQueue);
    NetAdapterInitSetDatapathCallbacks(adapterInit,
        &datapathCallbacks);

    // Create the adapter
    status = NetAdapterCreate(adapterInit, &adapterAttributes, &deviceContext->NetAdapter);

    // Always free the adapter initialization object
    NetAdapterInitFree(adapterInit);

    if (NT_SUCCESS(status)) {
        // Initialize the adapter's context
        adapterContext = AdapterGetContext(deviceContext->NetAdapter);

        // Save Device for later use
        adapterContext->Device = device;
    }
    else {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE,
            "NetAdapterCreate failed %!STATUS!", status);
        return status;
    }

    // Limitations say that we must pass a reference string to WdfDeviceCreateDeviceInterface:
    // If the client driver calls WdfDeviceCreateDeviceInterface with the ReferenceString parameter equal 
    // to NULL, NDIS intercepts I/O requests sent to the device interface. To avoid this behavior, specify 
    // any reference string.
    // https://learn.microsoft.com/en-us/windows-hardware/drivers/netcx/netadaptercx-limitations
    UNICODE_STRING referenceString;
    RtlUnicodeStringInit(&referenceString, L"netadaptercx-exmaple");

    // Create a device interface so that applications can find and talk to us.
    status = WdfDeviceCreateDeviceInterface(device, &GUID_DEVINTERFACE_NetAdapterCxExample, &referenceString);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "WdfDeviceCreateDeviceInterface failed %!STATUS!", status);
        return status;
    }

    // Initialize the I/O Package and any Queues
    status = NetAdapterCxExampleQueueInitialize(device);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "NetAdapterCxExampleQueueInitialize failed %!STATUS!", status);
        return status;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Exit");

    return status;
}


_Use_decl_annotations_
NTSTATUS NetAdapterCxExamplePrepareHardware(
    WDFDEVICE Device,
    WDFCMRESLIST ResourcesRaw,
    WDFCMRESLIST ResourcesTranslated
)
{
    NTSTATUS status = STATUS_SUCCESS;
    WDF_DMA_ENABLER_CONFIG dmaEnablerConfig;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(ResourcesRaw);
    UNREFERENCED_PARAMETER(ResourcesTranslated);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

    auto deviceContext = DeviceGetContext(Device);
    auto adapterContext = AdapterGetContext(deviceContext->NetAdapter);

    //
    // Skip setting up PCI resources
    //

    // Setup for DMA
    WdfDeviceSetAlignmentRequirement(Device, FILE_QUAD_ALIGNMENT);
    WDF_DMA_ENABLER_CONFIG_INIT(&dmaEnablerConfig, WdfDmaProfilePacket, 4096);
    dmaEnablerConfig.WdmDmaVersionOverride = 3;
    status = WdfDmaEnablerCreate(Device, &dmaEnablerConfig, WDF_NO_OBJECT_ATTRIBUTES,
        &deviceContext->DmaEnabler);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "WdfDmaEnablerCreate failed %!STATUS!",
            status);
        return status;
    }

    // Setup minimal Tx/Rx capabilities
    NET_ADAPTER_TX_CAPABILITIES txCapabilities;
    NET_ADAPTER_TX_CAPABILITIES_INIT(
        &txCapabilities,
        1);
    txCapabilities.MappingRequirement = NetMemoryMappingRequirementNone;

    NET_ADAPTER_RX_CAPABILITIES rxCapabilities;
    NET_ADAPTER_RX_CAPABILITIES_INIT_SYSTEM_MANAGED(
        &rxCapabilities,
        1580,
        1);

    NetAdapterSetDataPathCapabilities(deviceContext->NetAdapter, &txCapabilities, &rxCapabilities);

    NET_ADAPTER_LINK_LAYER_CAPABILITIES linkLayerCapabilities;
    NET_ADAPTER_LINK_LAYER_CAPABILITIES_INIT(
        &linkLayerCapabilities,
        1'000'000'000,
        1'000'000'000);
    NetAdapterSetLinkLayerCapabilities(deviceContext->NetAdapter, &linkLayerCapabilities);

    NetAdapterSetLinkLayerMtuSize(deviceContext->NetAdapter, 1500);

    UCHAR tmpAddress[6] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };
    NET_ADAPTER_LINK_LAYER_ADDRESS_INIT(&adapterContext->PermanentAddress, 6, tmpAddress);
    NET_ADAPTER_LINK_LAYER_ADDRESS_INIT(&adapterContext->CurrentAddress, 6, tmpAddress);
    NetAdapterSetPermanentLinkLayerAddress(deviceContext->NetAdapter, &adapterContext->PermanentAddress);
    NetAdapterSetCurrentLinkLayerAddress(deviceContext->NetAdapter, &adapterContext->CurrentAddress);

    // Start the NetAdapter
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "Starting NetAdapter");
    status = NetAdapterStart(deviceContext->NetAdapter);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "NetAdapterStart failed %!STATUS!", status);
        return status;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Exit");

    return status;
}

_Use_decl_annotations_
NTSTATUS
NetAdapterCxExampleReleaseHardware(
    WDFDEVICE Device,
    WDFCMRESLIST ResourcesTranslated
)
{
    UNREFERENCED_PARAMETER(ResourcesTranslated);

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

    auto deviceContext = DeviceGetContext(Device);

    // Stop the NetAdapter
    NetAdapterStop(deviceContext->NetAdapter);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Exit");

    return STATUS_SUCCESS;
}