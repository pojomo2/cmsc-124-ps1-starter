/*
 * dt_str.c: Length-carrying strings for Unit 5, Section B.
 *
 * A C string is a null-terminated character sequence stored in an array.
 * An array expression usually converts to a pointer to its first character.
 * strlen reads only through the first zero byte.
 * A pointer does not store the array capacity.
 *
 * This type stores the length and capacity with the bytes. dt_str_len reads a
 * field. A zero byte is data. Append operations use the stored capacity.
 *
 * An implementation can store a final zero byte after the data.
 * The public interface requires callers to use dt_str_len.
 */

#include "dt.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct dt_str {
    char  *bytes;
    size_t length;
    size_t capacity;
};

/*
 * dt_str_new copies the first `length` bytes. A zero byte is data. The function
 * returns NULL when allocation or size representation fails.
 */
dt_str *dt_str_new(const char *bytes, size_t length)
{
    /* TODO: Reject SIZE_MAX because the buffer needs one terminator byte.
       Allocate the handle and buffer. Copy `length` bytes with memcpy.
       Store the length. Return NULL if an allocation fails.
       dt_str_new("hello", 5)  -> a string whose dt_str_len is 5
       dt_str_new("a\0b", 3)   -> a string whose dt_str_len remains 3
       cases/normal/string_building.case, cases/capacity/embedded_zero_byte.case */
    if (length == SIZE_MAX) {return NULL;}
    if (bytes == NULL && length > 0) {return NULL;}
    dt_str *s = malloc(sizeof *s);
    if (s==NULL) {
        return NULL;
    }
    s->bytes = malloc(length + 1);
    if (s->bytes == NULL) {
        free(s);
        return NULL;
    }
    memcpy(s->bytes, bytes, length);
    s->bytes[length] = '\0';
    s->length = length;
    s->capacity = length + 1;
    return s;
}

/*
 * dt_str_free releases the buffer and handle. It accepts NULL.
 */
void dt_str_free(dt_str *s)
{
    /* TODO: Release the buffer. Then release the handle. Accept NULL.
       dt_str_free(s)     -> the buffer and the handle are both released
       dt_str_free(NULL)  -> returns, having done nothing */
    if (s == NULL) {
        return;
    }

    free(s->bytes);
    free(s);
}

/*
 * dt_str_len returns the stored byte count in constant time.
 */
size_t dt_str_len(const dt_str *s)
{
    /* TODO: Return the stored length. Do not scan the bytes.
       after `str new greeting "hello"` then `str append greeting ", world"`:
         dt_str_len(greeting) -> 12
       cases/normal/string_building.case */
    
    return s->length;
}

/*
 * dt_str_bytes returns the string bytes. Internal storage can include a final
 * zero byte. Callers must use dt_str_len with this pointer.
 */
const char *dt_str_bytes(const dt_str *s)
{
    /* TODO: Return the buffer. The caller uses it with dt_str_len.
       after `str new s "a\0b"`:
         dt_str_bytes(s) -> the three bytes 'a', 0, 'b'
         dt_str_len(s)   -> 3, the required read length
       cases/capacity/embedded_zero_byte.case */
    return s->bytes;
}

/*
 * dt_str_append adds `length` bytes and grows the buffer when necessary. It
 * returns DT_ERR_CAPACITY when allocation or size representation fails.
 * The function does not change the string after a failure.
 */
dt_status dt_str_append(dt_str *s, const char *bytes, size_t length)
{
    /* TODO: Check that the new length and terminator fit in size_t.
       Grow the buffer before you copy the bytes.
       Prevent unsigned wrap during capacity growth.
       Geometric growth makes repeated append operations efficient.
       s holds "hello": dt_str_append(s, ", world", 7) -> DT_OK, len is now 12
       an allocation failure                           -> DT_ERR_CAPACITY, s unchanged
       cases/normal/string_building.case, cases/capacity/string_growth.case */
    if (length == 0) {
        return DT_OK;
    }

    size_t new_length = s->length + length;
    if (new_length < s->length) {
        return DT_ERR_CAPACITY;
    }

    size_t needed = new_length + 1;
    if (needed < new_length) {
        return DT_ERR_CAPACITY;
    }

    if (s->capacity < needed) {
        size_t new_capacity = s->capacity ? s->capacity : 1;
        while (new_capacity < needed) {
            if (new_capacity < needed) {
                if (new_capacity > SIZE_MAX / 2) {
                    new_capacity = needed;
                    break;
                }
                new_capacity *= 2;
            }
        }

        char *buf = realloc(s->bytes, new_capacity);
        if (buf == NULL) {
            return DT_ERR_CAPACITY;
        }

        s->bytes = buf;
        s->capacity = new_capacity;
    }

    memcpy(s->bytes + s->length, bytes, length);
    s->length = new_length;
    s->bytes[s->length] = '\0';

    return DT_OK;

}

/*
 * dt_str_substr builds a new string from length bytes at start.
 * It returns DT_ERR_RANGE when the requested range exceeds the source.
 * It returns DT_ERR_CAPACITY after an allocation failure.
 * The function does not change the source string.
 */
dt_status dt_str_substr(const dt_str *s, size_t start, size_t length, dt_str **out)
{
    /* TODO: Return DT_ERR_RANGE when the requested range exceeds the source.
       Two size_t values can wrap. First compare start with the source length.
       Then compare length with the remaining length.
       s holds "hello" (length 5):
         dt_str_substr(s, 3, 2, &out)  -> DT_OK, *out is "lo"
         dt_str_substr(s, 5, 0, &out)  -> DT_OK, *out is a valid empty string
         dt_str_substr(s, 3, 5, &out)  -> DT_ERR_RANGE, *out untouched
       an allocation failure           -> DT_ERR_CAPACITY, *out untouched
       cases/boundary/substr_exact_end.case, cases/boundary/substr_past_end.case */
    if (out == NULL) {
        return DT_ERR_CAPACITY;
    }

    if (start > s->length) {
        return DT_ERR_RANGE;
    }

    size_t remaining = s->length - start;
    if (length > remaining) {
        return DT_ERR_RANGE;
    }

    dt_str *piece = malloc(sizeof *piece);
    if (piece == NULL) {
        return DT_ERR_CAPACITY;
    }

    piece->bytes = malloc(length + 1);
    if (piece->bytes == NULL) {
        free(piece);
        return DT_ERR_CAPACITY;
    }

    if(length > 0) {
        memcpy(piece->bytes, s->bytes + start, length);
    }

    piece->bytes[length] = '\0';
    piece->length = length;
    piece->capacity = length + 1;

    *out = piece;
    return DT_OK;
}

/*
 * dt_str_eq reports whether both strings hold the same bytes.
 * The stored lengths let the comparison include embedded zero bytes.
 */
bool dt_str_eq(const dt_str *a, const dt_str *b)
{
    /* TODO: Compare the lengths first. Then use memcmp.
       strcmp ends at an embedded zero byte and can report unequal data as equal.
       "world" and "world"  -> true
       "hello" and "world"  -> false
       "a\0b" and "a"       -> false because their lengths are 3 and 1
       cases/normal/string_building.case, cases/capacity/embedded_zero_byte.case */
    if (a->length != b->length) {
        return false;
    }
    return memcmp(a->bytes, b->bytes, a->length) == 0;
}
