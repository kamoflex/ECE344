#ifndef _BITMAP_H_
#define _BITMAP_H_

/*
 * Fixed-size array of bits for storage management.
 */

#include <sys/types.h>
#include <limits.h>

#define BITS_PER_WORD   (CHAR_BIT)
#define WORD_TYPE       unsigned char
#define WORD_ALLBITS    (0xff)

struct bitmap;	/* Opaque. */


/*     bitmap_create   - allocate a new bitmap object.
 *     bitmap_getdata  - return pointer to raw bit data.
 *     bitmap_get_size - return size of bitmap.
 *     bitmap_alloc    - locate a cleared bit, set it, and return its index.
 *     bitmap_mark     - set a clear bit by its index.
 *     bitmap_unmark   - clear a set bit by its index.
 *     bitmap_isset    - return whether a particular bit is set or not.
 *     bitmap_destroy  - destroy bitmap.
 */
int bitmap_create(u_int32_t nbits, struct bitmap **bp);
void *bitmap_getdata(struct bitmap *);
long bitmap_getsize(struct bitmap *);
int bitmap_alloc(struct bitmap *, u_int32_t * index);
void bitmap_mark(struct bitmap *, u_int32_t index);
void bitmap_unmark(struct bitmap *, u_int32_t index);
int bitmap_isset(struct bitmap *, u_int32_t index);
void bitmap_destroy(struct bitmap *);
int bitmap_nr_allocated(struct bitmap *);

#endif /* _BITMAP_H_ */
