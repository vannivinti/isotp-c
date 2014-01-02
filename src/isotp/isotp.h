#ifndef __ISOTP_H__
#define __ISOTP_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define CAN_MESSAGE_BYTE_SIZE 8
#define MAX_ISO_TP_MESSAGE_SIZE 4096
// TODO we want to avoid malloc, and we can't be allocated 4K on the stack for
// each IsoTpMessage, so for now we're setting an artificial max message size
// here - since we only handle single frame messages, 8 bytes is plenty.
#define OUR_MAX_ISO_TP_MESSAGE_SIZE 8

#ifdef __cplusplus
extern "C" {
#endif

const uint8_t ISO_TP_DEFAULT_RESPONSE_TIMEOUT;
const bool ISO_TP_DEFAULT_FRAME_PADDING_STATUS;

typedef struct {
    const uint16_t arbitration_id;
    uint8_t payload[OUR_MAX_ISO_TP_MESSAGE_SIZE];
    uint16_t size;
    bool completed;
} IsoTpMessage;

typedef void (*LogShim)(const char* message, ...);
typedef bool (*SendCanMessageShim)(const uint16_t arbitration_id,
        const uint8_t* data, const uint8_t size);
typedef bool (*SetTimerShim)(uint16_t time_ms, void (*callback));

typedef void (*IsoTpMessageReceivedHandler)(const IsoTpMessage* message);
typedef void (*IsoTpMessageSentHandler)(const IsoTpMessage* message,
        const bool success);
typedef void (*IsoTpCanFrameSentHandler)(const IsoTpMessage* message);

typedef struct {
    LogShim log;
    SendCanMessageShim send_can_message;
    SetTimerShim set_timer;
} IsoTpShims;

typedef struct {
    uint16_t arbitration_id;
    IsoTpMessageReceivedHandler message_received_callback;

    // Private
    uint16_t timeout_ms;
    // timeout_ms: ISO_TP_DEFAULT_RESPONSE_TIMEOUT,
    bool frame_padding;
    // frame_padding: ISO_TP_DEFAULT_FRAME_PADDING_STATUS,
    uint8_t* receive_buffer;
    uint16_t received_buffer_size;
    uint16_t incoming_message_size;
    // TODO timer callback
} IsoTpReceiveHandle;

typedef struct {
    uint16_t sending_arbitration_id;
    uint16_t receiving_arbitration_id;
    IsoTpMessageSentHandler message_sent_callback;
    IsoTpCanFrameSentHandler can_frame_sent_callback;

    // TODO going to need some state here for multi frame messages
} IsoTpSendHandle;

typedef enum {
    ISOTP_HANDLE_SENDING,
    ISOTP_HANDLE_RECEIVING
} IsoTpHandleType;

typedef struct {
    bool success;
    bool completed;
    IsoTpHandleType type;
    IsoTpReceiveHandle receive_handle;
    IsoTpSendHandle send_handle;
} IsoTpHandle;


typedef enum {
    PCI_SINGLE = 0x0,
    PCI_FIRST_FRAME = 0x1,
    PCI_CONSECUTIVE_FRAME = 0x2,
    PCI_FLOW_CONTROL_FRAME = 0x3
} IsoTpProtocolControlInformation;

typedef enum {
    PCI_FLOW_STATUS_CONTINUE = 0x0,
    PCI_FLOW_STATUS_WAIT = 0x1,
    PCI_FLOW_STATUS_OVERFLOW = 0x2
} IsoTpFlowStatus;

IsoTpShims isotp_init_shims(LogShim log,
        SendCanMessageShim send_can_message,
        SetTimerShim set_timer);

/* Public:
 *
 */
IsoTpMessage isotp_receive_can_frame(IsoTpShims* shims, IsoTpHandle* handle,
        const uint16_t arbitration_id, const uint8_t data[],
        const uint8_t size);

// TODO perhaps this makes more sense as 2 functions:
// bool isotp_continue_send()
// IsoTpMessage isotp_continue_receive()
// but both with the same args

/* Public: Change the timeout for waiting on an ISO-TP response frame.
 *
 * If this function is not used, the conventional 100ms is used by default.
 *
 * handler - the ISO-TP handler to modify.
 * timeout - the new timeout in milliseconds.
 */
// void isotp_set_timeout(IsoTpHandler* handler, uint16_t timeout_ms);

// void isotp_destroy(IsoTpHandler* handler);

void isotp_message_to_string(const IsoTpMessage* message, char* destination,
        size_t destination_length);

IsoTpHandle isotp_send(IsoTpShims* shims, const uint16_t arbitration_id,
        const uint8_t payload[], uint16_t size,
        IsoTpMessageSentHandler callback);

IsoTpHandle isotp_receive(IsoTpShims* shims,
        const uint16_t arbitration_id, IsoTpMessageReceivedHandler callback);

#ifdef __cplusplus
}
#endif

#endif // __ISOTP_H__
