/* circdelay_negotiation.c -- generated by Trunnel v1.5.3.
 * https://gitweb.torproject.org/trunnel.git
 * You probably shouldn't edit this file.
 */
#include <stdlib.h>
#include "trunnel-impl.h"

#include "circdelay_negotiation.h"

#define TRUNNEL_SET_ERROR_CODE(obj) \
  do {                              \
    (obj)->trunnel_error_code_ = 1; \
  } while (0)

#if defined(__COVERITY__) || defined(__clang_analyzer__)
/* If we're running a static analysis tool, we don't want it to complain
 * that some of our remaining-bytes checks are dead-code. */
int circdelaynegotiation_deadcode_dummy__ = 0;
#define OR_DEADCODE_DUMMY || circdelaynegotiation_deadcode_dummy__
#else
#define OR_DEADCODE_DUMMY
#endif

#define CHECK_REMAINING(nbytes, label)                           \
  do {                                                           \
    if (remaining < (nbytes) OR_DEADCODE_DUMMY) {                \
      goto label;                                                \
    }                                                            \
  } while (0)

circdelay_negotiate_t *
circdelay_negotiate_new(void)
{
  circdelay_negotiate_t *val = trunnel_calloc(1, sizeof(circdelay_negotiate_t));
  if (NULL == val)
    return NULL;
  return val;
}

/** Release all storage held inside 'obj', but do not free 'obj'.
 */
static void
circdelay_negotiate_clear(circdelay_negotiate_t *obj)
{
  (void) obj;
}

void
circdelay_negotiate_free(circdelay_negotiate_t *obj)
{
  if (obj == NULL)
    return;
  circdelay_negotiate_clear(obj);
  trunnel_memwipe(obj, sizeof(circdelay_negotiate_t));
  trunnel_free_(obj);
}

size_t
circdelay_negotiate_getlen_delays(const circdelay_negotiate_t *inp)
{
  (void)inp;  return N_CELLS;
}

uint32_t
circdelay_negotiate_get_delays(circdelay_negotiate_t *inp, size_t idx)
{
  trunnel_assert(idx < N_CELLS);
  return inp->delays[idx];
}

uint32_t
circdelay_negotiate_getconst_delays(const circdelay_negotiate_t *inp, size_t idx)
{
  return circdelay_negotiate_get_delays((circdelay_negotiate_t*)inp, idx);
}
int
circdelay_negotiate_set_delays(circdelay_negotiate_t *inp, size_t idx, uint32_t elt)
{
  trunnel_assert(idx < N_CELLS);
  inp->delays[idx] = elt;
  return 0;
}

uint32_t *
circdelay_negotiate_getarray_delays(circdelay_negotiate_t *inp)
{
  return inp->delays;
}
const uint32_t  *
circdelay_negotiate_getconstarray_delays(const circdelay_negotiate_t *inp)
{
  return (const uint32_t  *)circdelay_negotiate_getarray_delays((circdelay_negotiate_t*)inp);
}
uint32_t
circdelay_negotiate_get_seed(const circdelay_negotiate_t *inp)
{
  return inp->seed;
}
int
circdelay_negotiate_set_seed(circdelay_negotiate_t *inp, uint32_t val)
{
  inp->seed = val;
  return 0;
}
const char *
circdelay_negotiate_check(const circdelay_negotiate_t *obj)
{
  if (obj == NULL)
    return "Object was NULL";
  if (obj->trunnel_error_code_)
    return "A set function failed on this object";
  return NULL;
}

ssize_t
circdelay_negotiate_encoded_len(const circdelay_negotiate_t *obj)
{
  ssize_t result = 0;

  if (NULL != circdelay_negotiate_check(obj))
     return -1;


  /* Length of u32 delays[N_CELLS] */
  result += N_CELLS * 4;

  /* Length of u32 seed */
  result += 4;
  return result;
}
int
circdelay_negotiate_clear_errors(circdelay_negotiate_t *obj)
{
  int r = obj->trunnel_error_code_;
  obj->trunnel_error_code_ = 0;
  return r;
}
ssize_t
circdelay_negotiate_encode(uint8_t *output, const size_t avail, const circdelay_negotiate_t *obj)
{
  ssize_t result = 0;
  size_t written = 0;
  uint8_t *ptr = output;
  const char *msg;
#ifdef TRUNNEL_CHECK_ENCODED_LEN
  const ssize_t encoded_len = circdelay_negotiate_encoded_len(obj);
#endif

  if (NULL != (msg = circdelay_negotiate_check(obj)))
    goto check_failed;

#ifdef TRUNNEL_CHECK_ENCODED_LEN
  trunnel_assert(encoded_len >= 0);
#endif

  /* Encode u32 delays[N_CELLS] */
  {

    unsigned idx;
    for (idx = 0; idx < N_CELLS; ++idx) {
      trunnel_assert(written <= avail);
      if (avail - written < 4)
        goto truncated;
      trunnel_set_uint32(ptr, trunnel_htonl(obj->delays[idx]));
      written += 4; ptr += 4;
    }
  }

  /* Encode u32 seed */
  trunnel_assert(written <= avail);
  if (avail - written < 4)
    goto truncated;
  trunnel_set_uint32(ptr, trunnel_htonl(obj->seed));
  written += 4; ptr += 4;


  trunnel_assert(ptr == output + written);
#ifdef TRUNNEL_CHECK_ENCODED_LEN
  {
    trunnel_assert(encoded_len >= 0);
    trunnel_assert((size_t)encoded_len == written);
  }

#endif

  return written;

 truncated:
  result = -2;
  goto fail;
 check_failed:
  (void)msg;
  result = -1;
  goto fail;
 fail:
  trunnel_assert(result < 0);
  return result;
}

/** As circdelay_negotiate_parse(), but do not allocate the output
 * object.
 */
static ssize_t
circdelay_negotiate_parse_into(circdelay_negotiate_t *obj, const uint8_t *input, const size_t len_in)
{
  const uint8_t *ptr = input;
  size_t remaining = len_in;
  ssize_t result = 0;
  (void)result;

  /* Parse u32 delays[N_CELLS] */
  CHECK_REMAINING(4 * N_CELLS, truncated);
  memcpy(obj->delays, ptr, 4 * N_CELLS);
  {
    unsigned idx;
    for (idx = 0; idx < N_CELLS; ++idx)
      obj->delays[idx] = trunnel_ntohl(obj->delays[idx]);
  }
  remaining -= 4 * N_CELLS; ptr += 4 * N_CELLS;

  /* Parse u32 seed */
  CHECK_REMAINING(4, truncated);
  obj->seed = trunnel_ntohl(trunnel_get_uint32(ptr));
  remaining -= 4; ptr += 4;
  trunnel_assert(ptr + remaining == input + len_in);
  return len_in - remaining;

 truncated:
  return -2;
}

ssize_t
circdelay_negotiate_parse(circdelay_negotiate_t **output, const uint8_t *input, const size_t len_in)
{
  ssize_t result;
  *output = circdelay_negotiate_new();
  if (NULL == *output)
    return -1;
  result = circdelay_negotiate_parse_into(*output, input, len_in);
  if (result < 0) {
    circdelay_negotiate_free(*output);
    *output = NULL;
  }
  return result;
}
