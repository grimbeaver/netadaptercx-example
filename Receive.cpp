/*++

Module Name:

    receive.c

Abstract:

    This file contains the receive queue entry points and callbacks.

Environment:

    Kernel-mode Driver Framework

--*/

#include "driver.h"
#include "receive.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, NetAdapterCxExampleCreateRxQueue)
#endif

_Use_decl_annotations_
NTSTATUS NetAdapterCxExampleCreateRxQueue(
    NETADAPTER Adapter,
    NETRXQUEUE_INIT* RxQueueInit
)
{
    NTSTATUS status = STATUS_SUCCESS;
    WDF_OBJECT_ATTRIBUTES queueAttributes;

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_RECEIVE, "%!FUNC! Entry");

    auto adapterContext = AdapterGetContext(Adapter);
    auto deviceContext = DeviceGetContext(adapterContext->Device);

    // Setup the context
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&queueAttributes, RX_QUEUE_CONTEXT);

    // Prepare the configuration structure
    NET_PACKET_QUEUE_CONFIG rxConfig;
    NET_PACKET_QUEUE_CONFIG_INIT(
        &rxConfig,
        EvtRxQueueAdvance,
        EvtRxQueueSetNotificationEnabled,
        EvtRxQueueCancel);

    // Optional: register the queue's start and stop callbacks
    rxConfig.EvtStart = EvtRxQueueStart;
    rxConfig.EvtStop = EvtRxQueueStop;

    // Create the receive queue
    status = NetRxQueueCreate(
        RxQueueInit,
        &queueAttributes,
        &rxConfig,
        &adapterContext->ReceiveQueue);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_RECEIVE, "NetRxQueueCreate failed %!STATUS!",
            status);
        return status;
    }

    // Get the queue context for storing the queue ID and packet extension offset info
    auto queueContext = RxQueueGetContext(adapterContext->ReceiveQueue);

    // Setup the context
    queueContext->Adapter = Adapter;
    queueContext->RingCollection = NetRxQueueGetRingCollection(adapterContext->ReceiveQueue);

    // Setup Descriptor DMA memory
    queueContext->DescriptorMemory.BufferSize = 0x8000;
    status = WdfCommonBufferCreate(deviceContext->DmaEnabler, queueContext->DescriptorMemory.BufferSize,
        WDF_NO_OBJECT_ATTRIBUTES, &queueContext->DescriptorMemory.CommonBuffer);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_RECEIVE, "WdfCommonBufferCreate failed %!STATUS!",
            status);
        return status;
    }
    queueContext->DescriptorMemory.PhysicalBase = WdfCommonBufferGetAlignedLogicalAddress(queueContext->DescriptorMemory.CommonBuffer);
    queueContext->DescriptorMemory.VirtualBase = static_cast<PUCHAR>(WdfCommonBufferGetAlignedVirtualAddress(queueContext->DescriptorMemory.CommonBuffer));

    // Setup Buffer DMA memory
    queueContext->BufferMemory.BufferSize = 0x8000;
    status = WdfCommonBufferCreate(deviceContext->DmaEnabler, queueContext->BufferMemory.BufferSize,
        WDF_NO_OBJECT_ATTRIBUTES, &queueContext->BufferMemory.CommonBuffer);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_RECEIVE, "WdfCommonBufferCreate failed %!STATUS!",
            status);
        return status;
    }
    queueContext->BufferMemory.PhysicalBase = WdfCommonBufferGetAlignedLogicalAddress(queueContext->BufferMemory.CommonBuffer);
    queueContext->BufferMemory.VirtualBase = static_cast<PUCHAR>(WdfCommonBufferGetAlignedVirtualAddress(queueContext->BufferMemory.CommonBuffer));

    // Get the extension for determining virtual addresses of fragment buffers
    NET_EXTENSION_QUERY extension;
    NET_EXTENSION_QUERY_INIT(
        &extension,
        NET_FRAGMENT_EXTENSION_VIRTUAL_ADDRESS_NAME,
        NET_FRAGMENT_EXTENSION_VIRTUAL_ADDRESS_VERSION_1,
        NetExtensionTypeFragment);
    NetRxQueueGetExtension(adapterContext->ReceiveQueue, &extension, &queueContext->VirtualAddressExtension);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_RECEIVE, "%!FUNC! Exit");

    return status;
}


_Use_decl_annotations_
void EvtRxQueueAdvance(NETPACKETQUEUE PacketQueue)
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_RECEIVE, "%!FUNC! Entry");

    auto queueContext = RxQueueGetContext(PacketQueue);
    auto packetRing = queueContext->RingCollection->Rings[NetRingTypePacket];
    auto fragmentRing = queueContext->RingCollection->Rings[NetRingTypeFragment];

    //
    // Process packets here
    //

    // Check for cancel and return any remaining packets / fragments
    if (queueContext->QueueCancel) {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_RECEIVE, "%!FUNC! Processing QueueCancel");

        // Get all packets and mark them for ignoring
        auto currentPacketIndex = packetRing->BeginIndex;
        while (currentPacketIndex != packetRing->EndIndex)
        {
            NET_PACKET* packet = NetRingGetPacketAtIndex(packetRing, currentPacketIndex);
            packet->Ignore = 1;
            currentPacketIndex = NetRingIncrementIndex(packetRing, currentPacketIndex);
        }
        packetRing->BeginIndex = packetRing->EndIndex;

        // Return all fragments to the OS
        fragmentRing->BeginIndex = fragmentRing->EndIndex;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_RECEIVE, "%!FUNC! Exit");
}

_Use_decl_annotations_
void  EvtRxQueueSetNotificationEnabled(NETPACKETQUEUE PacketQueue, BOOLEAN NotificationEnabled)
{
    auto queueContext = RxQueueGetContext(PacketQueue);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_RECEIVE, "%!FUNC! Entry - Enabled: %d",
        NotificationEnabled ? 1 : 0);

    // Interrupts are always on, just save that we want to be notified
    queueContext->NotificationEnabled = NotificationEnabled;
}

_Use_decl_annotations_
void  EvtRxQueueCancel(NETPACKETQUEUE PacketQueue)
{
    auto queueContext = RxQueueGetContext(PacketQueue);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_RECEIVE, "%!FUNC! Entry");

    // Note we are done so we return all packets in advance
    queueContext->QueueCancel = TRUE;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_RECEIVE, "%!FUNC! Exit");
}

_Use_decl_annotations_
void  EvtRxQueueStart(NETPACKETQUEUE PacketQueue)
{
    auto queueContext = RxQueueGetContext(PacketQueue);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_RECEIVE, "%!FUNC! Entry");

    // Ensure cancel flag is cleared
    queueContext->QueueCancel = FALSE;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_RECEIVE, "%!FUNC! Exit");
}

_Use_decl_annotations_
void  EvtRxQueueStop(NETPACKETQUEUE PacketQueue)
{
    auto queueContext = RxQueueGetContext(PacketQueue);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_RECEIVE, "%!FUNC! Entry");
    
    UNREFERENCED_PARAMETER(queueContext);

    //
    // Do something...
    //

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_RECEIVE, "%!FUNC! Exit");
}
