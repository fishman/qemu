/*
 * Helpers for getting linearized buffers from iov / filling buffers into iovs
 *
 * Copyright IBM, Corp. 2007, 2008
 * Copyright (C) 2010 Red Hat, Inc.
 *
 * Author(s):
 *  Anthony Liguori <aliguori@us.ibm.com>
 *  Amit Shah <amit.shah@redhat.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include "iov.h"

size_t iov_from_buf(struct iovec *iov, unsigned int iov_cnt,
                    const void *buf, size_t iov_off, size_t size)
{
    size_t iovec_off, buf_off;
    unsigned int i;

    iovec_off = 0;
    buf_off = 0;
    for (i = 0; i < iov_cnt && size; i++) {
        if (iov_off < (iovec_off + iov[i].iov_len)) {
            size_t len = MIN((iovec_off + iov[i].iov_len) - iov_off, size);

            memcpy(iov[i].iov_base + (iov_off - iovec_off), buf + buf_off, len);

            buf_off += len;
            iov_off += len;
            size -= len;
        }
        iovec_off += iov[i].iov_len;
    }
    return buf_off;
}

size_t iov_to_buf(const struct iovec *iov, const unsigned int iov_cnt,
                  void *buf, size_t iov_off, size_t size)
{
    uint8_t *ptr;
    size_t iovec_off, buf_off;
    unsigned int i;

    ptr = buf;
    iovec_off = 0;
    buf_off = 0;
    for (i = 0; i < iov_cnt && size; i++) {
        if (iov_off < (iovec_off + iov[i].iov_len)) {
            size_t len = MIN((iovec_off + iov[i].iov_len) - iov_off , size);

            memcpy(ptr + buf_off, iov[i].iov_base + (iov_off - iovec_off), len);

            buf_off += len;
            iov_off += len;
            size -= len;
        }
        iovec_off += iov[i].iov_len;
    }
    return buf_off;
}

size_t iov_size(const struct iovec *iov, const unsigned int iov_cnt)
{
    size_t len;
    unsigned int i;

    len = 0;
    for (i = 0; i < iov_cnt; i++) {
        len += iov[i].iov_len;
    }
    return len;
}
