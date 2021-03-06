/*
// Copyright (c) 2016 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#ifndef OC_CONNECTIVITY_H
#define OC_CONNECTIVITY_H

#include "config.h"
#include "messaging/coap/conf.h"
#include "oc_network_events.h"
#include "port/oc_log.h"
#include "util/oc_process.h"
#include <stdint.h>

#ifndef OC_MAX_PDU_BUFFER_SIZE
#error "Set OC_MAX_PDU_BUFFER_SIZE in config.h"
#else /* !OC_MAX_PDU_BUFFER_SIZE */
#if OC_MAX_PDU_BUFFER_SIZE < (COAP_MAX_HEADER_SIZE + 16)
#error "OC_MAX_PDU_BUFFER_SIZE must be >= (COAP_MAX_HEADER_SIZE + 2^4)"
#endif /* OC_MAX_PDU_BUFFER_SIZE is too small */
#endif /* OC_MAX_PDU_BUFFER_SIZE */

#ifdef OC_BLOCK_WISE_SET_MTU
#if OC_BLOCK_WISE_SET_MTU < (COAP_MAX_HEADER_SIZE + 16)
#error "OC_BLOCK_WISE_SET_MTU must be >= (COAP_MAX_HEADER_SIZE + 2^4)"
#endif /* OC_BLOCK_WISE_SET_MTU is too small */
#define OC_MAX_BLOCK_SIZE (OC_BLOCK_WISE_SET_MTU - COAP_MAX_HEADER_SIZE)
#define OC_BLOCK_SIZE                                                          \
  (OC_MAX_BLOCK_SIZE < 32                                                      \
     ? 16                                                                      \
     : (OC_MAX_BLOCK_SIZE < 64                                                 \
          ? 32                                                                 \
          : (OC_MAX_BLOCK_SIZE < 128                                           \
               ? 64                                                            \
               : (OC_MAX_BLOCK_SIZE < 256                                      \
                    ? 128                                                      \
                    : (OC_MAX_BLOCK_SIZE < 512                                 \
                         ? 256                                                 \
                         : (OC_MAX_BLOCK_SIZE < 1024                           \
                              ? 512                                            \
                              : (OC_MAX_BLOCK_SIZE < 2048 ? 1024 : 2048)))))))
#define OC_BLOCK_WISE_BUFFER_SIZE                                              \
  (OC_MAX_PDU_BUFFER_SIZE - COAP_MAX_HEADER_SIZE)
#else /* OC_BLOCK_WISE_SET_MTU */
#define OC_BLOCK_SIZE (OC_MAX_PDU_BUFFER_SIZE - COAP_MAX_HEADER_SIZE)
#endif /* !OC_BLOCK_WISE_SET_MTU */
enum
{
#ifdef OC_SECURITY
  OC_PDU_SIZE = (2 * OC_BLOCK_SIZE + COAP_MAX_HEADER_SIZE)
#else  /* OC_SECURITY */
  OC_PDU_SIZE = (OC_BLOCK_SIZE + COAP_MAX_HEADER_SIZE)
#endif /* !OC_SECURITY */
};

typedef struct
{
  uint16_t port;
  uint8_t address[16];
  uint8_t scope;
} oc_ipv6_addr_t;

typedef struct
{
  uint16_t port;
  uint8_t address[4];
} oc_ipv4_addr_t;

typedef struct
{
  uint8_t type;
  uint8_t address[6];
} oc_le_addr_t;

typedef struct
{
  enum transport_flags
  {
    DISCOVERY = 1 << 0,
    SECURED = 1 << 1,
    IPV4 = 1 << 2,
    IPV6 = 1 << 3,
    GATT = 1 << 4
  } flags;

  union dev_addr
  {
    oc_ipv6_addr_t ipv6;
    oc_ipv4_addr_t ipv4;
    oc_le_addr_t bt;
  } addr;
} oc_endpoint_t;

#define oc_make_ipv4_endpoint(__name__, __flags__, __port__, ...)              \
  oc_endpoint_t __name__ = {.flags = __flags__,                                \
                            .addr.ipv4 = {.port = __port__,                    \
                                          .address = { __VA_ARGS__ } } }
#define oc_make_ipv6_endpoint(__name__, __flags__, __port__, ...)              \
  oc_endpoint_t __name__ = {.flags = __flags__,                                \
                            .addr.ipv6 = {.port = __port__,                    \
                                          .address = { __VA_ARGS__ } } }

struct oc_message_s
{
  struct oc_message_s *next;
  oc_endpoint_t endpoint;
  size_t length;
  uint8_t ref_count;
  uint8_t data[OC_PDU_SIZE];
};

void oc_send_buffer(oc_message_t *message);

#ifdef OC_SECURITY
uint16_t oc_connectivity_get_dtls_port(void);
#endif /* OC_SECURITY */

int oc_connectivity_init(void);

void oc_connectivity_shutdown(void);

void oc_send_discovery_request(oc_message_t *message);

#endif /* OC_CONNECTIVITY_H */
