/*++

Module Name:

    transmit.h

Abstract:

    This file contains the transmit queue definitions.

Environment:

    Kernel-mode Driver Framework

--*/

EXTERN_C_START

//
// This is the context that can be placed per queue
// and would contain per queue information.
//
typedef struct _TX_QUEUE_CONTEXT
{
    // The net adapter the queue belongs to
    NETADAPTER Adapter;
    // The ring collection for the queue
    const NET_RING_COLLECTION* RingCollection;
    // Host memory for buffers
    DMA_MEMORY DmaMemory;
    // Extension for getting fragment memory pointers
    NET_EXTENSION VirtualAddressExtension;
    // Are notifications enabled?
    BOOLEAN NotificationEnabled;
    // Was QueueCancel called?
    BOOLEAN QueueCancel;
} TX_QUEUE_CONTEXT, * PTX_QUEUE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(TX_QUEUE_CONTEXT, TxQueueGetContext)

//
// Events from the Net Packet Queue object
//
EVT_NET_ADAPTER_CREATE_TXQUEUE NetAdapterCxExampleCreateTxQueue;
EVT_PACKET_QUEUE_ADVANCE EvtTxQueueAdvance;
EVT_PACKET_QUEUE_SET_NOTIFICATION_ENABLED EvtTxQueueSetNotificationEnabled;
EVT_PACKET_QUEUE_CANCEL EvtTxQueueCancel;
EVT_PACKET_QUEUE_START EvtTxQueueStart;
EVT_PACKET_QUEUE_STOP EvtTxQueueStop;

EXTERN_C_END
