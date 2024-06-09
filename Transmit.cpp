/*++

Module Name:

    transmit.c

Abstract:

    This file contains the transmit queue entry points and callbacks.

Environment:

    Kernel-mode Driver Framework

--*/

#include "driver.h"
#include "transmit.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, NetAdapterCxExampleCreateTxQueue)
#endif

_Use_decl_annotations_
NTSTATUS NetAdapterCxExampleCreateTxQueue(
    NETADAPTER Adapter,
    NETTXQUEUE_INIT* TxQueueInit
)
{
    NTSTATUS status = STATUS_SUCCESS;
    WDF_OBJECT_ATTRIBUTES queueAttributes;

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_TRANSMIT, "%!FUNC! Entry");

    auto adapterContext = AdapterGetContext(Adapter);
    auto deviceContext = DeviceGetContext(adapterContext->Device);

    // Setup the context
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&queueAttributes, TX_QUEUE_CONTEXT);

    // Prepare the configuration structure
    NET_PACKET_QUEUE_CONFIG txConfig;
    NET_PACKET_QUEUE_CONFIG_INIT(
        &txConfig,
        EvtTxQueueAdvance,
        EvtTxQueueSetNotificationEnabled,
        EvtTxQueueCancel);

    // Optional: register the queue's start and stop callbacks
    txConfig.EvtStart = EvtTxQueueStart;
    txConfig.EvtStop = EvtTxQueueStop;

    // Create the transmit queue
    status = NetTxQueueCreate(
        TxQueueInit,
        &queueAttributes,
        &txConfig,
        &adapterContext->TransmitQueue);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_TRANSMIT, "NetTxQueueCreate failed %!STATUS!",
            status);
        return status;
    }

    // Get the queue context for storing the queue ID and extension info
    auto queueContext = TxQueueGetContext(adapterContext->TransmitQueue);

    // Setup the context
    queueContext->Adapter = Adapter;
    queueContext->RingCollection = NetTxQueueGetRingCollection(adapterContext->TransmitQueue);

    // Setup DMA memory
    queueContext->DmaMemory.BufferSize = 0x8000;
    status = WdfCommonBufferCreate(deviceContext->DmaEnabler, queueContext->DmaMemory.BufferSize,
        WDF_NO_OBJECT_ATTRIBUTES, &queueContext->DmaMemory.CommonBuffer);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_TRANSMIT, "WdfCommonBufferCreate failed %!STATUS!",
            status);
        return status;
    }
    queueContext->DmaMemory.PhysicalBase = WdfCommonBufferGetAlignedLogicalAddress(queueContext->DmaMemory.CommonBuffer);
    queueContext->DmaMemory.VirtualBase = static_cast<PUCHAR>(WdfCommonBufferGetAlignedVirtualAddress(queueContext->DmaMemory.CommonBuffer));

    // Get the extension for determining virtual addresses of fragment buffers
    NET_EXTENSION_QUERY extension;
    NET_EXTENSION_QUERY_INIT(
        &extension,
        NET_FRAGMENT_EXTENSION_VIRTUAL_ADDRESS_NAME,
        NET_FRAGMENT_EXTENSION_VIRTUAL_ADDRESS_VERSION_1,
        NetExtensionTypeFragment);
    NetTxQueueGetExtension(adapterContext->TransmitQueue, &extension, &queueContext->VirtualAddressExtension);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_TRANSMIT, "%!FUNC! Exit");

    return status;
}

_Use_decl_annotations_
void EvtTxQueueAdvance(NETPACKETQUEUE PacketQueue)
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_TRANSMIT, "%!FUNC! Entry");

    UNREFERENCED_PARAMETER(PacketQueue);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_TRANSMIT, "%!FUNC! Exit");
}

_Use_decl_annotations_
void  EvtTxQueueSetNotificationEnabled(NETPACKETQUEUE PacketQueue, BOOLEAN NotificationEnabled)
{
    auto queueContext = TxQueueGetContext(PacketQueue);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_TRANSMIT, "%!FUNC! Entry - Enabled: %d",
        NotificationEnabled ? 1 : 0);

    // Interrupts are always on, just save that we want to be notified
    queueContext->NotificationEnabled = NotificationEnabled;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_TRANSMIT, "%!FUNC! Exit");
}

_Use_decl_annotations_
void  EvtTxQueueCancel(NETPACKETQUEUE PacketQueue)
{
    auto queueContext = TxQueueGetContext(PacketQueue);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_TRANSMIT, "%!FUNC! Entry");

    queueContext->QueueCancel = TRUE;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_TRANSMIT, "%!FUNC! Exit");
}

_Use_decl_annotations_
void  EvtTxQueueStart(NETPACKETQUEUE PacketQueue)
{
    auto queueContext = TxQueueGetContext(PacketQueue);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_TRANSMIT, "%!FUNC! Entry");

    // Ensure cancel flag is cleared
    queueContext->QueueCancel = FALSE;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_TRANSMIT, "%!FUNC! Exit");
}

_Use_decl_annotations_
void  EvtTxQueueStop(NETPACKETQUEUE PacketQueue)
{
    auto queueContext = TxQueueGetContext(PacketQueue);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_RECEIVE, "%!FUNC! Entry");

    UNREFERENCED_PARAMETER(queueContext);

    //
    // Do something...
    //

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_RECEIVE, "%!FUNC! Exit");
}
