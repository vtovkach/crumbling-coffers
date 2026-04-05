#ifndef _SV_PACKET_H_
#define _SV_PACKET_H_

#include <stdint.h>

#define SV_EVENT_MATCH_REQUEST  0
#define SV_EVENT_MATCH_CANCEL   1

/*
 * Incoming TCP packet from client.
 * 4 bytes: request type code.
 *   0 = match request
 *   1 = match cancel
 */
struct __attribute__((packed)) SvPacket
{
    uint32_t event_type;
};

#endif