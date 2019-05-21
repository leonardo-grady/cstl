/****************************************************************************
*
* FILENAME:        cstl.c
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

//===========================
// Includes
//===========================
#include "cstl.h"
//===========================
// Defines
//===========================

//===========================
// Typedefs
//===========================

//===========================
// Locals
//===========================

//===========================
// Globals
//===========================
vector_operation vop;
queue_operation qop;

/* Functions */
static bool _empty_v(vector *this);
static bool _resize_v(vector *this, size_t sz);
static size_t _size_v(vector *this);
static void *_at_v(vector *this, size_t n);
static void *_front_v(vector *this);
static void *_back_v(vector *this);
static bool _push_back_v(vector *this, void *ele);
static void *_pop_back_v(vector *this);
static void _erase_v(vector *this, size_t n);
static void _clear_v(vector *this);

static bool _empty_q(queue *this);
static bool _resize_q(queue *this, size_t sz);
static size_t _size_q(queue *this);
static void *_front_q(queue *this);
static void *_back_q(queue *this);
static bool _push_q(queue *this, void *ele);
static void _pop_q(queue *this);

#if CSTL_DEBUG
static void dump_data(uint8_t *data, int len, int swap);
#endif
/* A simple implementation of cplusplus vector and queue, all this is for reducing memory useage
 * and improving access speed.
 */
//=============================================================================
inline void
vector_op_init(
    void
)
//=============================================================================
{
    vop.empty = _empty_v;
    vop.resize = _resize_v;
    vop.size = _size_v;
    vop.at = _at_v;
    vop.front = _front_v;
    vop.back = _back_v;
    vop.push_back = _push_back_v;
    vop.pop_back = _pop_back_v;
    vop.insert = NULL;
    vop.erase = _erase_v;
    vop.clear = _clear_v;
}

//=============================================================================
inline void
queue_op_init(
    void
)
//=============================================================================
{
    qop.empty = _empty_q;
    qop.resize = _resize_q;
    qop.size = _size_q;
    qop.front = _front_q;
    qop.back = _back_q;
    qop.push = _push_q;
    qop.pop = _pop_q;
}

//=============================================================================
inline struct vector_t *
vector_constructor(
    vector *v,
    uint32_t tlen
)
//=============================================================================
{
    v->_vector = (struct vector_t *)calloc(1, sizeof(struct vector_t));
    if (v->_vector)
        v->_vector->_type_len = tlen;

    v->_vector->_bitmap = (struct bitmap_t *)calloc(1, sizeof(struct bitmap_t));
    if (!v->_vector->_bitmap) {
        free(v->_vector);
        v->_vector = NULL;
    }

    debug(LOG_DEBUG, "vector constructor: v: %p, _v: %p, size: %ld, capa: %ld", v, v->_vector, v->_vector->_size, v->_vector->_capacity);
    return v->_vector;
}

//=============================================================================
inline void
vector_destructor(
    vector *v
)
//=============================================================================
{
    if (v->_vector)
        free(v->_vector);
    if(v->_vector->_bitmap)
        free(v->_vector->_bitmap);
}

//=============================================================================
inline struct queue_t *
queue_constructor(
    queue *q,
    uint32_t tlen
)
//=============================================================================
{
    q->_queue = (struct queue_t *)calloc(1, sizeof(struct queue_t));
    if (q->_queue) {
        q->_queue->_type_len = tlen;
    }

    debug(LOG_DEBUG, "vector constructor: q: %p, _q: %p, size: %ld, capa: %ld", q, q->_queue, q->_queue->_size, q->_queue->_capacity);
    return q->_queue;
}

//=============================================================================
inline void
queue_destructor(
    queue *q
)
//=============================================================================
{
    if (q->_queue)
        free(q->_queue);
}

//=============================================================================
static bool
_empty_v(
    vector *this
)
//=============================================================================
{
    return this->_vector->_used ? false : true;
}

//=============================================================================
static bool
_resize_v(
    vector *this,
    size_t sz
)
//=============================================================================
{
    bool rc = true;
    size_t bitsz;
    void *vptr_bak = this->_vector;
    void *bptr_bak = this->_vector->_bitmap;

    debug(LOG_DEBUG, "before resize sz: %ld, v: %p, _v: %p, bp: %p", sz, this, this->_vector, this->_vector->_bitmap);
    this->_vector = realloc(this->_vector, sz + sizeof(struct vector_t));
    if (this->_vector == NULL) {
        this->_vector = vptr_bak;
        rc = false;
    }
    else {
        /* there is no need to memset, just used for debug */
#if CSTL_DEBUG
        memset(this->_vector->_vector + this->_vector->_capacity, 0, sz);
#endif
        this->_vector->_capacity = len2size(this, sz, vector);
    }

    /* big or little endian */
    bitsz = ((sz / this->_vector->_type_len) >> SHIFT) + ((sz / this->_vector->_type_len) & MASK ? 1 : 0);
    this->_vector->_bitmap = realloc(this->_vector->_bitmap, bitsz * sizeof(int) + sizeof(struct bitmap_t));
    if (this->_vector->_bitmap == NULL) {
        this->_vector->_bitmap = bptr_bak;
        rc = false;
    }
    else {
#if CSTL_DEBUG
        memset(this->_vector->_bitmap->_bitmap + this->_vector->_bitmap->_size, 0, bitsz * sizeof(int));
#endif
        this->_vector->_bitmap->_size = bitsz * sizeof(int);
    }

    debug(LOG_DEBUG, "after resize sz: %ld, v: %p, _v: %p, bp: %p, bsz: %ld", sz, this, this->_vector, this->_vector->_bitmap, this->_vector->_bitmap->_size);
    return rc;
}

//=============================================================================
static size_t
_size_v(
        vector *this
)
//=============================================================================
{
    return this->_vector->_size;
}

//=============================================================================
static void *
_at_v(
    vector *this,
    size_t n
)
//=============================================================================
{
    return this->_vector->_vector[n];
}

/* Calling this function on an empty container causes undefined behavior. */
//=============================================================================
static void *
_front_v(
    vector *this
)
//=============================================================================
{
    size_t n = 0;
    while(n < this->_vector->_size) {
        if (_bit_check_v(this, n))
            return this->_vector->_vector[n];

        n++;
    }

    return NULL;
}

//=============================================================================
static void *
_back_v(
    vector *this
)
//=============================================================================
{
    debug(LOG_DEBUG, "vector back v: %p, _v: %p, sz: %ld, vcapa: %ld", this, this->_vector, this->_vector->_size, this->_vector->_capacity);
    size_t n = this->_vector->_size - 1;
    while(n >= 0) {
        if (_bit_check_v(this, n))
            return this->_vector->_vector[n];

        n--;
    }

    return NULL;
}

//=============================================================================
static bool
_push_back_v(
    vector *this,
    void *ele
)
//=============================================================================
{
    debug(LOG_DEBUG, "vector push back v: %p, _v: %p, sz: %ld, used: %ld, vcapa: %ld", this, this->_vector, this->_vector->_size, this->_vector->_used, this->_vector->_capacity);
    if (this->_vector->_size >= this->_vector->_capacity) {
        if (!_resize_v(this, size2len3(this, vector)  + growth(this, vector))) {
            return false;
        }
    }

    _bit_set_v(this, this->_vector->_size);
    this->_vector->_vector[this->_vector->_size++] = ele;
    this->_vector->_used++;

#if CSTL_DEBUG
#if __BYTE_ORDER == __LITTLE_ENDIAN
    dump_data((uint8_t *)this->_vector->_bitmap->_bitmap, this->_vector->_bitmap->_size, 1);
#endif
#endif
    return true;
}

//=============================================================================
static void *
_pop_back_v(
    vector *this
)
//=============================================================================
{
    void *ele;
    if (this->_vector->_size + growth(this, vector) <= this->_vector->_capacity) {
        if(_resize_v(this, size2len3(this, vector)  - growth(this, vector)))
            this->_vector->_capacity -= growth(this, vector);
    }

    _bit_clear_v(this, this->_vector->_size);
    ele = this->_vector->_vector[this->_vector->_size--];
    this->_vector->_vector[this->_vector->_size + 1] = NULL;
    this->_vector->_used--;

    return ele;
}

//=============================================================================
static void
_erase_v(
    vector *this,
    size_t n
)
//=============================================================================
{
    _bit_clear_v(this, n);
    this->_vector->_used--;
}

//=============================================================================
static void
_clear_v(
    vector *this
)
//=============================================================================
{
    int i;
    for (i = 0; i < this->_vector->_size; i++) {
        if (this->_vector->_vector[i])
            this->_vector->_vector[i] = NULL;
    }
    memset(this->_vector->_bitmap->_bitmap, 0, this->_vector->_bitmap->_size);
}

//=============================================================================
inline void
_bit_set_v(
    vector *this,
    size_t n
)
//=============================================================================
{
    this->_vector->_bitmap->_bitmap[n >> SHIFT] |= (1 << (n & MASK));
}

//=============================================================================
inline void
_bit_clear_v(
    vector *this,
    size_t n
)
//=============================================================================
{
    this->_vector->_bitmap->_bitmap[n >> SHIFT] &= (~(1 << (n & MASK)));
}

//=============================================================================
inline int32_t
_bit_check_v(
    vector *this,
    size_t n
)
//=============================================================================
{
    return this->_vector->_bitmap->_bitmap[n >> SHIFT] & (1 << (n & MASK));
}

//=============================================================================
static bool
_empty_q(
    queue *this
)
//=============================================================================
{
    return this->_queue->_size ? false : true;
}

//=============================================================================
static bool 
_resize_q(
    queue *this,
    size_t sz
)
//=============================================================================
{
    bool rc = true;
    void *qbak = this->_queue;

    this->_queue = realloc(this->_queue, sz + sizeof(struct queue_t));
    if (this->_queue == NULL) {
        this->_queue = qbak;
        rc = false;
    }
    else {
        this->_queue->_capacity = len2size(this, sz, queue);
    }

    return rc;
}

//=============================================================================
static size_t
_size_q(
        queue *this
)
//=============================================================================
{
    return this->_queue->_size;
}

//=============================================================================
static void *
_front_q(
    queue *this
)
//=============================================================================
{
    return this->_queue->_queue[this->_queue->_front];
}

//=============================================================================
static void *
_back_q(
    queue *this
)
//=============================================================================
{
    return this->_queue->_queue[this->_queue->_rear - 1];
}

//=============================================================================
static bool
_push_q(
    queue *this,
    void *ele
)
//=============================================================================
{
    debug(LOG_INFO, "queue push front: %d, rear: %d, size: %ld, capacity: %ld", this->_queue->_front, this->_queue->_rear, this->_queue->_size, this->_queue->_capacity);
    uint32_t rear;
    uint32_t capacity;
    if (this->_queue->_capacity)
        rear = (this->_queue->_rear + 1) % this->_queue->_capacity;
    else
        rear = 0;
    capacity = this->_queue->_capacity;

    if (rear == this->_queue->_front) {
        if(!_resize_q(this, size2len3(this, queue) + growth_low_speed(this, queue)))
            return false;
        else {
            if (capacity && this->_queue->_rear < this->_queue->_front) {
                uint32_t size = (this->_queue->_rear - this->_queue->_front + capacity) % capacity;
                if (this->_queue->_rear > size / 2) {
                    memmove(this->_queue->_queue + (this->_queue->_front + capacity) * this->_queue->_type_len, this->_queue->_queue + this->_queue->_front * this->_queue->_type_len, (capacity - this->_queue->_front) * this->_queue->_type_len);
                    this->_queue->_front += size;
                }
                else {
                    memmove(this->_queue->_queue + capacity * this->_queue->_type_len, this->_queue->_queue, (this->_queue->_rear - 1) * this->_queue->_type_len);
                    this->_queue->_rear += size;
                }
            }

            this->_queue->_queue[this->_queue->_rear] = ele;
            this->_queue->_rear = (this->_queue->_rear + 1) % this->_queue->_capacity;
            this->_queue->_size++;
        }

    }
    else {
        this->_queue->_queue[this->_queue->_rear] = ele;
        this->_queue->_rear = rear;
        this->_queue->_size++;
    }

    debug(LOG_INFO, "queue push front: %d, rear: %d, size: %ld, capacity: %ld", this->_queue->_front, this->_queue->_rear, this->_queue->_size, this->_queue->_capacity);
    return true;
}

//=============================================================================
static void
_pop_q(
    queue *this
)
//=============================================================================
{
    if (this->_queue->_front == this->_queue->_rear) {
        if (_resize_q(this, growth(this, queue))) {
            this->_queue->_capacity = growth(this, queue);
            this->_queue->_front = this->_queue->_rear = 0;
            this->_queue->_size = 0;
        }
    }
    else {
        if (this->_queue->_capacity)
            this->_queue->_front = (this->_queue->_front + 1) % this->_queue->_capacity;
        this->_queue->_size--;
        debug(LOG_INFO, "queue pop front: %d, rear: %d, size: %ld, capacity: %ld", this->_queue->_front, this->_queue->_rear, this->_queue->_size, this->_queue->_capacity);
    }
}

#if CSTL_DEBUG
static void dump_data(uint8_t *data, int len, int swap)
{
    int i = 0;
    char line[64] = {0};

    for(i = 0; i < len;) {
        snprintf(line, 64, "%08x: %02x %02x %02x %02x %02x %02x %02x %02x  %02x %02x %02x %02x %02x %02x %02x %02x", i, data[swap ? i^3 : i], data[swap ? (i+1)^3 : i+1], data[swap ? (i+2)^3 : i+2], data[swap ? (i+3)^3 : i+3],
                data[swap ? (i+4)^3 : i+4], data[swap ? (i+5)^3 : i+5], data[swap ? (i+6)^3 : i+6], data[swap ? (i+7)^3 : i+7],
                data[swap ? (i+8)^3 : i+8], data[swap ? (i+9)^3 : i+9], data[swap ? (i+10)^3 : i+10], data[swap ? (i+11)^3 : i+11],
                data[swap ? (i+12)^3 : i+12], data[swap ? (i+13)^3 : i+13], data[swap ? (i+14)^3 : i+14], data[swap ? (i+15)^3 : i+15]);
        debug(LOG_DEBUG, "%s", line);
        i += 15;
    }
}
#endif

/* EOF */
