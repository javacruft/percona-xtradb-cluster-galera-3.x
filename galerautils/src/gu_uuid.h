/*
 * Copyright (C) 2008 Codership Oy <info@codership.com>
 *
 * $Id$
 */
/*
 * Universally Unique IDentifier. RFC 4122.
 * Time-based implementation.
 *
 */

#ifndef _gu_uuid_h_
#define _gu_uuid_h_

#include "gu_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*! UUID internally is represented as a BE integer which allows using
 *  memcmp() as comparison function and straightforward printing */
#define GU_UUID_LEN 16
typedef struct {
    uint8_t data[GU_UUID_LEN];
} gu_uuid_t;

extern const gu_uuid_t GU_UUID_NIL;

/*! length of string representation */
#define GU_UUID_STR_LEN 36

/*! Macros for pretty printing */
#define GU_UUID_FORMAT \
"%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x"

#define GU_UUID_ARGS(uuid) \
(uuid)->data[ 0], (uuid)->data[ 1], (uuid)->data[ 2], (uuid)->data[ 3],\
(uuid)->data[ 4], (uuid)->data[ 5], (uuid)->data[ 6], (uuid)->data[ 7],\
(uuid)->data[ 8], (uuid)->data[ 9], (uuid)->data[10], (uuid)->data[11],\
(uuid)->data[12], (uuid)->data[13], (uuid)->data[14], (uuid)->data[15]

/* this is used for scanf, variables are by reference */
#define GU_UUID_FORMAT_SCANF \
"%02hhx%02hhx%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx"

#define GU_UUID_ARGS_SCANF(uuid) \
&(uuid)->data[ 0], &(uuid)->data[ 1], &(uuid)->data[ 2], &(uuid)->data[ 3],\
&(uuid)->data[ 4], &(uuid)->data[ 5], &(uuid)->data[ 6], &(uuid)->data[ 7],\
&(uuid)->data[ 8], &(uuid)->data[ 9], &(uuid)->data[10], &(uuid)->data[11],\
&(uuid)->data[12], &(uuid)->data[13], &(uuid)->data[14], &(uuid)->data[15]

/*!
 * Generates new UUID.
 * If node is NULL, will generate random (if /dev/urand is present) or
 * pseudorandom data instead.
 * @param uuid
 *        pointer to uuid_t
 * @param node
 *        some unique data that goes in place of "node" field in the UUID
 * @param node_len
 *        length of the node buffer
 */
extern void
gu_uuid_generate (gu_uuid_t*  uuid,
                  const void* node,
                  size_t      node_len);

/*!
 * Compare two UUIDs according to RFC
 * @return -1, 0, 1 if left is respectively less, equal or greater than right
 */
extern long
gu_uuid_compare (const gu_uuid_t* left,
                 const gu_uuid_t* right);

/*!
 * Compare ages of two UUIDs
 * @return -1, 0, 1 if left is respectively younger, equal or older than right
 */
extern long
gu_uuid_older (const gu_uuid_t* left,
               const gu_uuid_t* right);

/*!
 * Print UUID into buffer
 * @return Number of bytes printed (not including trailing '\0') or -1 on error.
 */
extern ssize_t
gu_uuid_print(const gu_uuid_t* uuid, char* buf, size_t buflen);

/*!
 * Scan UUID from buffer
 * @return Number of bytes read (should match to sizeof(uuid)) or -1 on error
 */
extern ssize_t
gu_uuid_scan(const char* buf, size_t buflen, gu_uuid_t* uuid);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _gu_uuid_h_ */
