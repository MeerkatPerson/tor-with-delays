/**
 * \file circuitdelay.h
 * \brief Header file for circuitdelay.c.
 **/

#ifndef TOR_CIRCUITDELAY_H
#define TOR_CIRCUITDELAY_H

// 22/12 include code generated by trunnel
#include "trunnel/circdelay_negotiation.h"
#include "lib/evloop/timers.h"

struct circuit_t;
struct origin_circuit_t;
struct cell_t;

/**
 * Signed error return with the specific property that negative
 * values mean error codes of various semantics, 0 means success,
 * and positive values are unused.
 *
 * XXX: Tor uses this concept a lot but just calls it int. Should we move
 * this somewhere centralized? Where?
 */
typedef int signed_error__t; // use two underscores to avoid duplicate definition
                             // (defined in circuitpadding.c)

/** Delay-relevant events cause us to start initiating delay application
  * resp. ceasing to apply delays */
// void circdelay_event_circ_added_hop(struct origin_circuit_t *on_circ);
// void circdelay_event_circ_built(struct origin_circuit_t *circ);
// void circdelay_event_circ_purpose_changed(struct origin_circuit_t *circ);
void circdelay_event_circ_has_streams(struct origin_circuit_t *circ);
// void circdelay_event_circ_has_no_streams(struct origin_circuit_t *circ);
// void circdelay_event_circ_has_no_relay_early(struct origin_circuit_t *circ);
void circdelay_event_cell_threshold_reached(origin_circuit_t *circ);

/* Delay negotiation between client and middle */
signed_error__t circdelay_handle_delay_negotiate(struct circuit_t *circ,
                                      struct cell_t *cell);

/**
signed_error_t circdelay_handle_delay_negotiated(struct circuit_t *circ,
                                      struct cell_t *cell,
                                      crypt_path_t *layer_hint);
*/

signed_error__t circdelay_negotiate_delay(struct origin_circuit_t *circ,
                          uint8_t target_hopnum);
/**
bool circdelay_delay_negotiated(struct circuit_t *circ,
                           uint8_t command,
                           uint8_t response);
*/

int circdelay_check_received_cell(cell_t *cell, circuit_t *circ,
                                crypt_path_t *layer_hint,
                                const relay_header_t *rh);

#ifdef CIRCUITDELAY_PRIVATE

MOCK_DECL(STATIC signed_error__t,
circdelay_send_command_to_hop,(struct origin_circuit_t *circ, uint8_t hopnum,
                             uint8_t relay_command, const uint8_t *payload,
                             ssize_t payload_len));

MOCK_DECL(STATIC const node_t *,
circdelay_circuit_get_nth_node,(origin_circuit_t *circ, int hop));

#endif /* defined(CIRCUITDELAY_PRIVATE) */

#endif /* !defined(TOR_CIRCUITDELAY_H) */