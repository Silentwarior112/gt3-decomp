/* u138 / G007: newlib stdio - findfp.c (_cleanup, __sinit), fread.c,
 * fvwrite.c, fwalk.c, makebuf.c, stdio.c (refill.c is refill.c).
 * The rest of findfp.c (std, _cleanup_r, __sfp) and fflush / __swsetup are
 * in u137. */
#include "runtime/newlib/G007_stdio.h"
#include "runtime/newlib/newlib_part2_internal.h"

/* ------------------------------------------------------------------------ */
/* findfp.c                                                                 */
/* ------------------------------------------------------------------------ */

// FUNC 0x00281088 _cleanup
void _cleanup(void)
{
    _cleanup_r(_impure_ptr);    /* (tail jump) */
}

// FUNC 0x00281098 __sinit
void __sinit(struct _reent *s)
{
    /* make sure we clean up on exit */
    s->__cleanup = _cleanup_r;  /* conservative */
    s->__sdidinit = 1;

    std(s->__sf + 0, __SRD, 0, s);
    std(s->__sf + 1, __SWR | __SLBF, 1, s);
    std(s->__sf + 2, __SWR | __SNBF, 2, s);

    /* initialise the head of the glue chain to point to the std streams */
    s->__sglue._next = NULL;
    s->__sglue._niobs = 3;
    s->__sglue._iobs = &s->__sf[0];
}

/* ------------------------------------------------------------------------ */
/* fread.c (no CHECK_INIT in this version: __srefill does it)               */
/* ------------------------------------------------------------------------ */

// FUNC 0x00281128 fread
size_t fread(void *buf, size_t size, size_t count, FILE *fp)
{
    size_t resid;
    char *p;
    int r;
    size_t total;

    if ((resid = count * size) == 0)
        return 0;
    if (fp->_r < 0)
        fp->_r = 0;
    total = resid;
    p = (char *)buf;
    while (resid > (size_t)(r = fp->_r)) {
        memcpy(p, fp->_p, (size_t)r);
        fp->_p += r;
        /* fp->_r = 0 ... done in __srefill */
        p += r;
        resid -= r;
        if (__srefill(fp)) {
            /* no more input: return partial result */
            return (total - resid) / size;
        }
    }
    memcpy(p, fp->_p, resid);
    fp->_r -= resid;
    fp->_p += resid;
    return count;
}

/* ------------------------------------------------------------------------ */
/* fvwrite.c                                                                */
/* ------------------------------------------------------------------------ */

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define COPY(n) (void)memmove(fp->_p, p, (size_t)(n))
#define GETIOV(extra_work)         \
    while (len == 0) {             \
        extra_work;                \
        p = (const char *)iov->iov_base; \
        len = iov->iov_len;        \
        iov++;                     \
    }
/* cantwrite(): not writable or no buffer yet, and __swsetup fails */
#define cantwrite(fp) \
    ((((fp)->_flags & __SWR) == 0 || (fp)->_bf._base == NULL) && __swsetup(fp))

// FUNC 0x002816D8 __sfvwrite
int __sfvwrite(FILE *fp, struct __suio *uio)
{
    size_t len;
    const char *p;
    struct __siov *iov;
    int w, s;
    char *nl;
    int nlknown, nldist;

    if ((len = uio->uio_resid) == 0)
        return 0;

    /* implicit: make sure we can write: cantwrite() calls __swsetup; COPY() is memmove */
    if (cantwrite(fp))
        return EOF;

    iov = uio->uio_iov;
    len = 0;
    if (fp->_flags & __SNBF) {
        /* Unbuffered: write up to BUFSIZ bytes at a time. */
        do {
            GETIOV(;);
            w = (*fp->_write)(fp->_cookie, p, MIN(len, BUFSIZ));
            if (w <= 0)
                goto err;
            p += w;
            len -= w;
        } while ((uio->uio_resid -= w) != 0);
    } else if ((fp->_flags & __SLBF) == 0) {
        /* Fully buffered: fill partially full buffer, if any, and then
         * flush.  If there is no partial buffer, write one _bf._size byte
         * chunk directly (without copying).  String output (__SSTR): write
         * as many bytes as fit, but pretend we wrote everything. */
        do {
            GETIOV(;);
            w = fp->_w;
            if (fp->_flags & __SSTR) {
                if (len < (size_t)w)
                    w = len;
                COPY(w);        /* copy MIN(fp->_w,len), */
                fp->_w -= w;
                fp->_p += w;
                w = len;        /* but pretend copied all */
            } else if (fp->_p > fp->_bf._base && len > (size_t)w) {
                /* fill and flush */
                COPY(w);
                /* fp->_w -= w; *//* unneeded */
                fp->_p += w;
                if (fflush(fp))
                    goto err;
            } else if (len >= (size_t)(w = fp->_bf._size)) {
                /* write directly */
                w = (*fp->_write)(fp->_cookie, p, w);
                if (w <= 0)
                    goto err;
            } else {
                /* fill and done */
                w = len;
                COPY(w);
                fp->_w -= w;
                fp->_p += w;
            }
            p += w;
            len -= w;
        } while ((uio->uio_resid -= w) != 0);
    } else {
        /* Line buffered: like fully buffered, but we must check for
         * newlines.  Compute the distance to the first newline (including
         * the newline), or `infinity' if there is none, then pretend that
         * the amount to write is MIN(len,nldist). */
        nlknown = 0;
        do {
            GETIOV(nlknown = 0);
            if (!nlknown) {
                nl = (char *)memchr(p, '\n', len);
                nldist = nl ? nl + 1 - p : (int)len + 1;
                nlknown = 1;
            }
            s = MIN(len, (size_t)nldist);
            w = fp->_w + fp->_bf._size;
            if (fp->_p > fp->_bf._base && s > w) {
                COPY(w);
                /* fp->_w -= w; */
                fp->_p += w;
                if (fflush(fp))
                    goto err;
            } else if (s >= (w = fp->_bf._size)) {
                w = (*fp->_write)(fp->_cookie, p, w);
                if (w <= 0)
                    goto err;
            } else {
                w = s;
                COPY(w);
                fp->_w -= w;
                fp->_p += w;
            }
            if ((nldist -= w) == 0) {
                /* copied the newline: flush and forget */
                if (fflush(fp))
                    goto err;
                nlknown = 0;
            }
            p += w;
            len -= w;
        } while ((uio->uio_resid -= w) != 0);
    }
    return 0;

err:
    fp->_flags |= __SERR;
    return EOF;
}

#undef MIN
#undef COPY
#undef GETIOV
#undef cantwrite

/* ------------------------------------------------------------------------ */
/* fwalk.c                                                                  */
/* ------------------------------------------------------------------------ */

// FUNC 0x00281AB0 _fwalk
int _fwalk(struct _reent *ptr, int (*function)(FILE *))
{
    FILE *fp;
    int n, ret = 0;
    struct _glue *g;

    for (g = &ptr->__sglue; g != NULL; g = g->_next)
        for (fp = g->_iobs, n = g->_niobs; --n >= 0; fp++)
            if (fp->_flags != 0)
                ret |= (*function)(fp);
    return ret;
}

/* ------------------------------------------------------------------------ */
/* makebuf.c (no HAVE_BLKSIZE: the size is always BUFSIZ)                   */
/* ------------------------------------------------------------------------ */

// FUNC 0x00281C38 __smakebuf
void __smakebuf(FILE *fp)
{
    size_t size, couldbetty;
    void *p;
    struct stat st;

    if (fp->_flags & __SNBF) {
        fp->_bf._base = fp->_p = fp->_nbuf;
        fp->_bf._size = 1;
        return;
    }
    if (fp->_file < 0 || _fstat_r(fp->_data, fp->_file, &st) < 0) {
        couldbetty = 0;
        size = BUFSIZ;
        /* do not try to optimise fseek() */
        fp->_flags |= __SNPT;
    } else {
        couldbetty = (st.st_mode & S_IFMT) == S_IFCHR;
        size = BUFSIZ;
        /* Optimize fseek() only if it is a regular file.  (The test for
         * __sseek is mainly paranoia.) */
        if ((st.st_mode & S_IFMT) == S_IFREG && fp->_seek == __sseek) {
            fp->_flags |= __SOPT;
            fp->_blksize = 1024;
        } else
            fp->_flags |= __SNPT;
    }
    if ((p = _malloc_r(fp->_data, size)) == NULL) {
        fp->_flags |= __SNBF;
        fp->_bf._base = fp->_p = fp->_nbuf;
        fp->_bf._size = 1;
    } else {
        fp->_data->__cleanup = _cleanup_r;
        fp->_flags |= __SMBF;
        fp->_bf._base = fp->_p = (unsigned char *)p;
        fp->_bf._size = size;
        if (couldbetty && isatty(fp->_file))
            fp->_flags |= __SLBF;
    }
}

/* ------------------------------------------------------------------------ */
/* stdio.c: the default FILE methods (cookie = the FILE)                    */
/* ------------------------------------------------------------------------ */

// FUNC 0x00282A80 __sread
int __sread(void *cookie, char *buf, int n)
{
    FILE *fp = (FILE *)cookie;
    int ret;

    ret = (int)_read_r(fp->_data, fp->_file, buf, n);   /* long -> int */

    /* If the read succeeded, update the current offset. */
    if (ret >= 0)
        fp->_offset += ret;
    else
        fp->_flags &= ~__SOFF;  /* paranoia */
    return ret;
}

// FUNC 0x00282AE8 __swrite
int __swrite(void *cookie, const char *buf, int n)
{
    FILE *fp = (FILE *)cookie;

    if (fp->_flags & __SAPP)
        (void)_lseek_r(fp->_data, fp->_file, (s64)0, SEEK_END);
    fp->_flags &= ~__SOFF;      /* in case O_APPEND mode is set */
    return (int)_write_r(fp->_data, fp->_file, buf, n);     /* long -> int */
}

// FUNC 0x00282B68 __sseek
s64 __sseek(void *cookie, s64 offset, int whence)
{
    FILE *fp = (FILE *)cookie;
    s64 ret;    /* _off_t = long */

    ret = _lseek_r(fp->_data, fp->_file, offset, whence);
    if (ret == -1L)
        fp->_flags &= ~__SOFF;
    else {
        fp->_flags |= __SOFF;
        fp->_offset = (int)ret;
    }
    return ret;
}

// FUNC 0x00282BD0 __sclose
int __sclose(void *cookie)
{
    FILE *fp = (FILE *)cookie;

    return _close_r(fp->_data, fp->_file);
}
