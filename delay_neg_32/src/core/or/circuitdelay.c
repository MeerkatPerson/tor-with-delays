/**
 * \file circuitdelay.c
 * \brief Circuit-level delaying of non-padding cells implementation
**/

#include <stddef.h>
#include <stdint.h>
#define CIRCUITDELAY_PRIVATE

#include <math.h>
#include "lib/math/fp.h"
#include "lib/math/prob_distr.h"
#include "core/or/or.h"
#include "core/or/circuitdelay.h"
#include "core/or/circuitlist.h"
#include "core/or/circuituse.h"
#include "core/mainloop/netstatus.h"
#include "core/or/relay.h"
#include "feature/stats/rephist.h"
#include "feature/nodelist/networkstatus.h"

#include "core/or/channel.h"

#include "lib/time/compat_time.h"
#include "lib/defs/time.h"
#include "lib/crypt_ops/crypto_rand.h"

#include "core/or/crypt_path_st.h"
#include "core/or/circuit_st.h"
#include "core/or/origin_circuit_st.h"
#include "core/or/or_circuit_st.h"
#include "feature/nodelist/routerstatus_st.h"
#include "feature/nodelist/node_st.h"
#include "core/or/cell_st.h"
#include "core/or/extend_info_st.h"
#include "core/crypto/relay_crypto.h"
#include "feature/nodelist/nodelist.h"

#include "app/config/config.h"

/**
 * Send a relay command with a relay cell payload on a circuit to
 * the particular hopnum.
 *
 * Hopnum starts at 1 (1=guard, 2=middle, 3=exit, etc).
 *
 * Payload may be null.
 *
 * Returns negative on error, 0 on success.
 */
MOCK_IMPL(STATIC signed_error__t,
circdelay_send_command_to_hop,(origin_circuit_t *circ, uint8_t hopnum,
                             uint8_t relay_command, const uint8_t *payload,
                             ssize_t payload_len))
{
  // circuit_get_cpath_hop is in circuitlist.c
  crypt_path_t *target_hop = circuit_get_cpath_hop(circ, hopnum);
  signed_error__t ret;

  /* Check that the cpath has the target hop */
  if (!target_hop) {
    log_fn(LOG_WARN, LD_BUG, "Delay circuit %u has %d hops, not %d",
           circ->global_identifier, circuit_get_cpath_len(circ), hopnum);
    return -1;
  }

  /* Check that the target hop is opened */
  if (target_hop->state != CPATH_STATE_OPEN) {
    log_fn(LOG_WARN,LD_CIRC,
           "Delay circuit %u has %d hops, not %d",
           circ->global_identifier,
           circuit_get_cpath_opened_len(circ), hopnum);
    return -1;
  }

  /* Send the drop command to the second hop */
  ret = relay_send_command_from_edge(0, TO_CIRCUIT(circ), relay_command,
                                     (const char*)payload, payload_len,
                                     target_hop);
  return ret;
}

/** Check if this cell or circuit are related to circuit delay application and handle
 *  them if so.  Return 0 if the cell was handled in this subsystem and does
 *  not need any other consideration, otherwise return 1.
 */

int
circdelay_check_received_cell(cell_t *cell, circuit_t *circ,
                            crypt_path_t *layer_hint,
                            const relay_header_t *rh)
{
  /* First handle the delay-relevant commands, since we want to ignore any other
   * commands if this circuit is delay-specific. */

  /** 
  switch (rh->command) {
    case RELAY_COMMAND_DELAY_NEGOTIATE:
      circdelay_handle_delay_negotiate(circ, cell);
      return 0;
    // TODO: do we even need multiple Relay Commands related to delays?
    // Will one suffice (client telling hop to add delay?)
    case RELAY_COMMAND_DELAY_NEGOTIATED:
      if (circdelay_handle_delay_negotiated(circ, cell, layer_hint) == 0)
        // WHAT HAPPENS IN CIRCUIT_READ_VALID_DATA???
        circuit_read_valid_data(TO_ORIGIN_CIRCUIT(circ), rh->length);
      return 0;
  }
  */

  (void) layer_hint; /* unused */

  // 8/12 for now, we are only introducing one new relay cell command;
  // more might follow (for ACKing a delay negotiation request or for 
  // client side initiation of stopping to apply delays)

  if (rh->command == RELAY_COMMAND_DELAY_NEGOTIATE) {

    circdelay_handle_delay_negotiate(circ, cell);
    return 0;

  }

  return 1;
}

/**
 * Streams attached event.
 *
 * Called from link_apconn_to_circ() and handle_hs_exit_conn()
 * 
 * 21/12: currently not using, remove?
 */
void
circdelay_event_circ_has_streams(origin_circuit_t *circ)
{
  // Initiate delay application here; no further restrictions/filters 
  // e.g. regarding circuit purpose for now.

  // negotiate delay with all three hops.
  for (int i = 1; i <= 3; i++) {
    circdelay_negotiate_delay(circ, i);
  }
  
}

/**
 * 21/12 add a new event which is triggered whenever client-side counter of cells sent
 * on circuit is 0 mod 32
 * 32 is defined as constant size of array containing sampled delays in circdelay_negotiate.trunnel,
 * also include it as constant in or_circuit_st 
 */
 void 
 circdelay_event_cell_threshold_reached(origin_circuit_t *circ)
 {
  log_info(LD_GENERAL, "Threshold reached event triggered");
  
  // negotiate delay with all three hops.
  for (int i = 1; i <= 3; i++) {
    circdelay_negotiate_delay(circ, i);
  }

 }

/**
 * Check the Protover info to see if a node supports delays.
 */
static bool
circdelay_node_supports_delays(const node_t *node)
{

  (void) node; /* currently unused */

  /** TODO include supports_delay in protover ..... */

  /**
  if (node->rs) {
    log_fn(LOG_INFO, LD_CIRC, "Checking padding: %s",
           node->rs->pv.supports_hs_setup_padding ?
              "supported" : "unsupported");
    return node->rs->pv.supports_hs_setup_padding;
  }

  log_fn(LOG_INFO, LD_CIRC, "Empty routerstatus in padding check");
  return 0;
  */
  return 1;
}

/**
 * Get a node_t for the nth hop in our circuit, starting from 1.
 *
 * Returns node_t from the consensus for that hop, if it is opened.
 * Otherwise returns NULL.
 */
MOCK_IMPL(STATIC const node_t *,
circdelay_circuit_get_nth_node,(origin_circuit_t *circ, int hop))
{
  crypt_path_t *iter = circuit_get_cpath_hop(circ, hop);

  if (!iter || iter->state != CPATH_STATE_OPEN)
    return NULL;

  return node_get_by_id(iter->extend_info->identity_digest);
}

/**
 * Return true if a particular circuit supports delays
 * at the desired hop.
 */
static bool
circdelay_circuit_supports_delay(origin_circuit_t *circ,
                                 int target_hopnum)
{
  const node_t *hop;

  if (!(hop = circdelay_circuit_get_nth_node(circ, target_hopnum))) {
    return 0;
  }

  return circdelay_node_supports_delays(hop);
}

/**
 * Try to negotiate delay application.
 *
 * Returns -1 on error, 0 on success.
 * 7/12: don't need a spec because we are only toggling
 * one kind of behavior, i.e., not offering multiple behaviors
 * to choose from. Also don't need machine_ctr for analogous
 * reasons.
 */
signed_error__t
circdelay_negotiate_delay(origin_circuit_t *circ,
                          uint8_t target_hopnum)
{
  circdelay_negotiate_t type;                          
  cell_t cell;
  ssize_t len;

  /* Check that the target hop lists support for delay in
   * its ProtoVer fields */
  if (!circdelay_circuit_supports_delay(circ, target_hopnum)) {
    return -1;
  }

  memset(&cell, 0, sizeof(cell_t));
  memset(&type, 0, sizeof(circdelay_negotiate_t));
  // This gets reset to RELAY_EARLY appropriately by
  // relay_send_command_from_edge_. At least, it looks that way.
  // QQQ-MP-AP: Verify that.
  cell.command = CELL_RELAY;

  // 21/12 now we need to set our circdelay_negotiate - struct's fields
  
  // sample a seed and set field
  uint32_t seed = crypto_fast_rng_get_u32(get_thread_fast_rng());
  circdelay_negotiate_set_seed(&type, seed);

  // set up our probability distribution
  /* param1 is k, param2 is Lambda */
  const struct weibull_t my_weibull = {
    .base = WEIBULL(my_weibull),
    .k = 1,
    .lambda = 0.001,
  };

  // for the delay array, need to set each index separately
  for (size_t i = 0; i < 32; i++) {

    double delay =  dist_sample(&my_weibull.base);
    uint32_t delay_usec = (uint32_t) (delay * TOR_USEC_PER_SEC);
    circdelay_negotiate_set_delays(&type, i, delay_usec);

  }

  if ((len = circdelay_negotiate_encode(cell.payload, CELL_PAYLOAD_SIZE,
        &type)) < 0)
    return -1;

  log_fn(LOG_INFO,LD_CIRC,
         "Negotiating delay on circuit %u (%d)",
         circ->global_identifier, TO_CIRCUIT(circ)->purpose);

  return circdelay_send_command_to_hop(circ, target_hopnum,
                                     RELAY_COMMAND_DELAY_NEGOTIATE, cell.payload, len);
}

/**
 * Parse and react to a delay_negotiate cell.
 *
 * This is called at the middle node upon receipt of the client's request.
 *
 * Returns -1 on error, 0 on success.
 */
signed_error__t
circdelay_handle_delay_negotiate(circuit_t *circ, cell_t *cell)
{

  log_info(LD_GENERAL, "Handling delay negotiate");

  circdelay_negotiate_t* negotiate; 

  int retval = 0;

  if (CIRCUIT_IS_ORIGIN(circ)) {
    log_fn(LOG_PROTOCOL_WARN, LD_CIRC,
           "Delay negotiate cell unsupported at origin (circuit %u)",
           TO_ORIGIN_CIRCUIT(circ)->global_identifier);
    return -1;
  }

  if (circdelay_negotiate_parse(&negotiate, cell->payload+RELAY_HEADER_SIZE,
                               CELL_PAYLOAD_SIZE-RELAY_HEADER_SIZE) < 0) {
    log_fn(LOG_PROTOCOL_WARN, LD_CIRC,
          "Received malformed DELAY_NEGOTIATE cell; dropping.");
    return -1;
  }

  // 22/12 this might fail if circ is not an or_circuit .....

  // set the delays
  for (int i = 0; i < 32; i++) {

    circ->sampled_delays[i] = circdelay_negotiate_get_delays(negotiate, i);

  }

  // set the sampling seed
  circ->sampling_seed = circdelay_negotiate_get_seed(negotiate);
   
  // turn delays on on this circuit
  circ->delays_active = 1;

  // free the circdelay_negotiate struct
  circdelay_negotiate_free(negotiate);

  return retval;
}
