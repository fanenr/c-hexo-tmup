#include "mstr.h"
#include <stdlib.h>
#include <string.h>

mstr *
mstr_init(mstr *str)
{
    if (str == NULL)
        return NULL;

    str->data = NULL;
    str->len = str->cap = 0;
    return str;
}

char *
mstr_unwrap(mstr *str)
{
    if (str == NULL)
        return NULL;

    char *data = str->data;
    mstr_init(str);
    return data;
}

mstr *
mstr_move_cstr(mstr *dest, char *src)
{
    if (dest == NULL || src == NULL)
        return NULL;

    size_t len = strlen(src);
    size_t cap = len + 1;

    dest->data = src;
    dest->len = len;
    dest->cap = cap;
    return dest;
}

mstr *
mstr_move_mstr(mstr *dest, mstr *src)
{
    if (dest == NULL)
        return NULL;

    if (mstr_free(dest) != dest) /* free failed */
        return NULL;

    if (src == NULL)
        return dest;

    *dest = *src;
    mstr_init(src);
    return dest;
}

mstr *
mstr_swap_mstr(mstr *dest, mstr *src)
{
    if (dest == NULL || src == NULL)
        return NULL;

    mstr copy = *dest;
    *dest = *src;
    *src = copy;
    return dest;
}

mstr *
mstr_reserve(mstr *dest, size_t cap)
{
    if (dest == NULL || cap <= dest->cap)
        return dest;

    char *data = realloc(dest->data, cap);
    if (data == NULL) /* allocate failed */
        return NULL;

    dest->cap = cap;
    dest->data = data;
    return dest;
}

mstr *
mstr_cat_char(mstr *dest, char src)
{
    if (dest == NULL || src == '\0')
        return dest;

    if (dest->cap < dest->len + 2) {
        size_t cap
            = dest->cap > 0 ? dest->cap * MSTR_EXPAN_RATIO : MSTR_INIT_CAP;
        if (mstr_reserve(dest, cap) != dest) /* allocate failed */
            return NULL;
    }

    dest->data[dest->len++] = src;
    dest->data[dest->len] = '\0';
    return dest;
}

mstr *
mstr_cat_cstr(mstr *dest, const char *src)
{
    if (dest == NULL || src == NULL || *src == '\0')
        return dest;

    size_t cap = dest->cap;
    size_t dlen = dest->len;
    size_t slen = strlen(src);
    if (cap < dlen + slen + 1) {
        cap = cap > 0 ? dlen + slen + 1 : slen + 1;
        if (mstr_reserve(dest, cap) != dest) /* allocate failed */
            return NULL;
    }

    char *pos = dest->data + dlen;
    if (memmove(pos, src, slen) != pos) /* copy failed */
        return NULL;

    dest->len += slen;
    dest->data[dest->len] = '\0';
    return dest;
}

mstr *
mstr_cat_mstr(mstr *dest, const mstr *src)
{
    if (dest == NULL || src == NULL)
        return dest;
    return mstr_cat_cstr(dest, src->data);
}

mstr *
mstr_cat_byte(mstr *dest, const char *src, size_t slen)
{
    if (dest == NULL || src == NULL || *src == '\0' || slen == 0)
        return dest;

    size_t cap = dest->cap;
    size_t dlen = dest->len;
    slen = src[slen - 1] == '\0' ? slen - 1 : slen;
    if (cap < dlen + slen + 1) {
        cap = cap > 0 ? dlen + slen + 1 : slen + 1;
        if (mstr_reserve(dest, cap) != dest) /* allocate failed */
            return NULL;
    }

    char *pos = dest->data + dlen;
    if (memmove(pos, src, slen) != pos) /* copy failed */
        return NULL;

    dest->len += slen;
    dest->data[dest->len] = '\0';
    return dest;
}

mstr *
mstr_assign_char(mstr *dest, char src)
{
    if (dest == NULL || src == '\0')
        return mstr_free(dest);

    size_t cap = MSTR_INIT_CAP;
    char *data = malloc(cap);
    if (data == NULL) /* allocate failed */
        return NULL;

    data[0] = src;
    data[1] = '\0';
    if (mstr_free(dest) != dest) { /* free failed */
        free(data);
        return NULL;
    }

    dest->data = data;
    dest->cap = cap;
    dest->len = 2;
    return dest;
}

mstr *
mstr_assign_cstr(mstr *dest, const char *src)
{
    if (dest == NULL || src == NULL || *src == '\0')
        return mstr_free(dest);

    size_t slen = strlen(src);
    size_t cap = slen + 1 < MSTR_INIT_CAP ? MSTR_INIT_CAP : slen + 1;

    char *data = malloc(cap);
    if (data == NULL) /* allocate failed */
        return NULL;

    data[slen] = '\0';
    if (memmove(data, src, slen) != data) { /* copy failed */
        free(data);
        return NULL;
    }

    if (mstr_free(dest) != dest) { /* free failed */
        free(data);
        return NULL;
    }

    dest->data = data;
    dest->len = slen;
    dest->cap = cap;
    return dest;
}

mstr *
mstr_assign_mstr(mstr *dest, const mstr *src)
{
    if (dest == NULL || src == NULL)
        return mstr_free(dest);

    return mstr_assign_cstr(dest, src->data);
}

mstr *
mstr_assign_byte(mstr *dest, const char *src, size_t slen)
{
    if (dest == NULL || src == NULL || *src == '\0' || slen == 0)
        return mstr_free(dest);

    slen = src[slen - 1] == '\0' ? slen - 1 : slen;
    size_t cap = slen + 1 < MSTR_INIT_CAP ? MSTR_INIT_CAP : slen + 1;

    char *data = malloc(cap);
    if (data == NULL) /* allocate failed */
        return NULL;

    data[slen] = '\0';
    if (memmove(data, src, slen) != data) { /* copy failed */
        free(data);
        return NULL;
    }

    if (mstr_free(dest) != dest) { /* free failed */
        free(data);
        return NULL;
    }

    dest->data = data;
    dest->len = slen;
    dest->cap = cap;
    return dest;
}

mstr *
mstr_free(mstr *str)
{
    if (str == NULL)
        return NULL;
    free(str->data);
    return mstr_init(str);
}

mstr *
mstr_remove_from(mstr *dest, size_t spos, size_t len)
{
    if (dest == NULL || len == 0 || spos >= dest->len)
        return dest;

    size_t dlen = dest->len;
    if (spos + len >= dlen - 1) {
        dest->data[spos] = '\0';
        dest->len = spos;
        return dest;
    }

    size_t clen = dlen - spos - len;
    char *st = dest->data + spos;
    char *ed = st + len;
    if (memmove(st, ed, clen) != st)
        return NULL;

    dest->data[spos + clen] = '\0';
    dest->len -= len;
    return dest;
}

mstr *
mstr_remove_range(mstr *dest, size_t spos, size_t epos)
{
    if (dest == NULL || spos > epos)
        return dest;
    return mstr_remove_from(dest, spos, epos - spos + 1);
}

mstr
mstr_sub_from(const mstr *dest, size_t spos, size_t len)
{
    if (dest == NULL || len == 0 || spos >= dest->len)
        return mstr_new();

    if (spos + len >= dest->len)
        return mstr_new_byte(&dest->data[spos], dest->len - spos);

    return mstr_new_byte(&dest->data[spos], len);
}

mstr
mstr_sub_range(const mstr *dest, size_t spos, size_t epos)
{
    if (dest == NULL || spos > epos)
        return mstr_new();
    return mstr_sub_from(dest, spos, epos - spos + 1);
}

int
mstr_cmp_cstr(const mstr *lhs, const char *rhs)
{
    if (lhs == NULL && rhs == NULL)
        return MSTR_CMP_EQ;
    if (lhs == NULL) {
        if (*rhs == '\0')
            return MSTR_CMP_EQ;
        else
            return MSTR_CMP_LT;
    }
    if (rhs == NULL) {
        if (lhs->len == 0)
            return MSTR_CMP_EQ;
        else
            return MSTR_CMP_GT;
    }
    return strcmp(lhs->data, rhs);
}

int
mstr_cmp_mstr(const mstr *lhs, const mstr *rhs)
{
    if (lhs == NULL && rhs == NULL)
        return MSTR_CMP_EQ;
    if (lhs == NULL) {
        if (rhs->len == 0)
            return MSTR_CMP_EQ;
        else
            return MSTR_CMP_LT;
    }
    if (rhs == NULL) {
        if (lhs->len == 0)
            return MSTR_CMP_EQ;
        else
            return MSTR_CMP_GT;
    }
    return strcmp(lhs->data, rhs->data);
}
