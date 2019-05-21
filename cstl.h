/****************************************************************************
*
* FILENAME:        cstl.h
*
* DESCRIPTION:     Description of this source file's contents
*
* Copyright (c) 2017 by Grandstream Networks, Inc.
* All rights reserved.
*
* This material is proprietary to Grandstream Networks, Inc. and,
* in addition to the above mentioned Copyright, may be
* subject to protection under other intellectual property
* regimes, including patents, trade secrets, designs and/or
* trademarks.
*
* Any use of this material for any purpose, except with an
* express license from Grandstream Networks, Inc. is strictly
* prohibited.
*
***************************************************************************/

#ifndef __CSTL_H__
#define __CSTL_H__

//===========================
// Includes
//===========================
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <syslog.h>

//===========================
// Defines
//===========================
#define CSTL_DEBUG  0
#ifndef LOG_TAG
#define LOG_TAG    "[cstl]"
#endif
#define SHIFT       5
#define MASK        0x1f

#ifdef CSTL_DEBUG
#define debug(LOG_LEVEL, fmt, ...) do { syslog(LOG_LEVEL, LOG_TAG fmt, ##__VA_ARGS__); } while (0);
#else
#define debug(LOG_LEVEL, fmt, ...)
#endif

//===========================
// Typedefs
//===========================
typedef struct {
    struct vector_t {
        size_t _size;
        size_t _used;
        size_t _capacity;
        uint32_t _type_len;
        struct bitmap_t {
            size_t _size;
            uint32_t _bitmap[];
        } *_bitmap;
        void *_vector[];
    } *_vector;
} vector;

typedef struct {
    bool (*empty)(vector *this);
    bool (*resize)(vector *this, size_t sz);
    size_t (*size)(vector *this);
    void *(*at)(vector *this, size_t n);
    void *(*front)(vector *this);
    void *(*back)(vector *this);
    bool (*push_back)(vector *this, void *ele);
    void *(*pop_back)(vector *this);
    void (*insert)(vector *this, void *ele);
    void (*erase)(vector *this, size_t n);
    void (*clear)(vector *this);
} vector_operation;

typedef struct {
    struct queue_t {
        size_t _size;
        size_t _capacity;
        uint32_t _front;
        uint32_t _rear;
        uint32_t _type_len;
        void *_queue[];
    } *_queue;
} queue;

typedef struct {
    bool (*empty)(queue *this);
    bool (*resize)(queue *this, size_t sz);
    size_t (*size)(queue *this);
    void *(*front)(queue *this);
    void *(*back)(queue *this);
    bool (*push)(queue *this, void *ele);
    void (*pop)(queue *this);
} queue_operation;

//===========================
// Locals
//===========================

//===========================
// Globals
//===========================

extern vector_operation vop;
extern queue_operation qop;
/* bitmap operation */
void _bit_set_v(vector *this, size_t n);
void _bit_clear_v(vector *this, size_t n);
int32_t _bit_check_v(vector *this, size_t n);

#define growth(dptr, type)                  ((dptr)->_##type->_type_len * 128)
#define growth_low_speed(dptr, type)                  ((dptr)->_##type->_type_len * 32)

#define len2size(dptr, len, type)           ((len) / (dptr)->_##type->_type_len)

#define size2len(dptr, siz, type)           ((siz) * (dptr)->_##type->_type_len)
#define size2len2(dptr, type)               ((dptr)->_##type->_size * (dptr)->_##type->_type_len)
#define size2len3(dptr, type)               ((dptr)->_##type->_capacity * (dptr)->_##type->_type_len)

static inline void *vector_element(vector *vec, size_t *n)
{
    while (*n < vec->_vector->_size) {
        if (_bit_check_v(vec, (*n))) {
            debug(LOG_DEBUG, "n %ld, used %ld, size %ld", *n, vec->_vector->_used, vec->_vector->_size);
            return vec->_vector->_vector[*n];
        }
        (*n)++;
    }
    debug(LOG_DEBUG, "n %ld, used %ld, size %ld", *n, vec->_vector->_used, vec->_vector->_size);
    return NULL;
}

#if CSTL_DEBUG
static inline void *vector_first_element(vector *vec, size_t *n)
{
    debug(LOG_DEBUG, "first element");
    return vector_element(vec, n);
}

static inline void *vector_next_element(vector *vec, size_t *n)
{
    debug(LOG_DEBUG, "next element");
    return vector_element(vec, n);
}
#else
#define vector_first_element(vec, n) vector_element(vec, n)
#define vector_next_element(vec, n) vector_element(vec, n)
#endif

/* those macro below just used for iterate over vector of pointer type, but it's enough for now */

/**
 * vector_for_each_element   - iterate over vector of given type against erase operation any where
 * @pos:    the type * to use as a loop cursor.
 * @n:      another unigned int type to use as a loop cursor.
 * @vec:    the pointer for your vector.
 * @type:   the type of the vector element.
 */
#define vector_for_each_element(pos, n, vec, type) \
    for (n = 0, pos = (typeof(type *))((vec)->_vector->_vector[0]); \
            n < (vec)->_vector->_size; \
            pos = (typeof(type *))((vec)->_vector->_vector[++n]))

/**
 * vector_for_each_element_safe	- iterate over vector of given type safe against removal of vector element
 * @pos:	the type * to use as a loop cursor.
 * @n:		another unigned int type to use as a loop cursor.
 * @j:		another unigned int type to use as a jump loop cursor.
 * @vec:	the pointer for your vector.
 * @type:	the type of the vector element.
 */
#define vector_for_each_element_safe(pos, n, j, vec, type) \
    for (n = 0, j = 0, pos = (typeof(type *))vector_first_element(vec, &n); \
            n < (vec)->_vector->_size; \
            j++, n++, pos = (typeof(type *))vector_next_element(vec, &n) )


/* Functions */
void vector_op_init(void);
void queue_op_init(void);
struct vector_t *vector_constructor(vector *v, uint32_t tlen);
struct queue_t *queue_constructor(queue *q, uint32_t tlen);
void vector_destructor(vector *v);
void queue_destructor(queue *q);

#endif
/* EOF */
