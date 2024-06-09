/*++

Module Name:

    receive.h

Abstract:

    This file contains the receive queue definitions.

Environment:

    Kernel-mode Driver Framework

--*/

EXTERN_C_START

//
// This is the context that contains per queue information.
//
typedef struct _RX_QUEUE_CONTEXT
{
    // The net adapter the queue belongs to
    NETADAPTER Adapter;
    // The ring collection for the queue
    const NET_RING_COLLECTION* RingCollection;
    // Host memory allocated for descriptors
    DMA_MEMORY DescriptorMemory;
    // Host memory allocated for buffers
    DMA_MEMORY BufferMemory;
    // Extension for getting fragment memory pointers
    NET_EXTENSION VirtualAddressExtension;
    // Are notifications enabled?
    BOOLEAN NotificationEnabled;
    // Was QueueCancel called?
    BOOLEAN QueueCancel;
} RX_QUEUE_CONTEXT, * PRX_QUEUE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(RX_QUEUE_CONTEXT, RxQueueGetContext)

//
// Events from the Net Packet Queue object
//
EVT_NET_ADAPTER_CREATE_RXQUEUE NetAdapterCxExampleCreateRxQueue;
EVT_PACKET_QUEUE_ADVANCE EvtRxQueueAdvance;
EVT_PACKET_QUEUE_SET_NOTIFICATION_ENABLED EvtRxQueueSetNotificationEnabled;
EVT_PACKET_QUEUE_CANCEL EvtRxQueueCancel;
EVT_PACKET_QUEUE_START EvtRxQueueStart;
EVT_PACKET_QUEUE_STOP EvtRxQueueStop;

EXTERN_C_END
