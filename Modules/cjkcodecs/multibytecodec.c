/*
 * multibytecodec.c: Common Multibyte Codec Implementation
 *
 * Written by Hye-Shik Chang <perky@FreeBSD.org>
 */

#define PY_SSIZE_T_CLEAN
#include "Python.h"
#include "structmember.h"         // PyMemberDef
#include "multibytecodec.h"
#include "clinic/multibytecodec.c.h"

typedef struct {
    PyObject *MultibyteIncrementalEncoder_Type;
    PyObject *MultibyteIncrementalDecoder_Type;
    PyObject *MultibyteStreamReader_Type;
    PyObject *MultibyteStreamWriter_Type;
    PyObject *MultibyteCodec_Type;
} multibytecodec_state;


static multibytecodec_state *
multibytecodec_state_get_state(PyObject *module)
{
    multibytecodec_state *state = PyModule_GetState(module);
    assert(state);
    return state;
}

static struct PyModuleDef _multibytecodecmodule;
#define multibytecodec_state_get_state_by_type(type) \
    (multibytecodec_state_get_state(_PyType_GetModuleByDef(type, &_multibytecodecmodule)))


/*[clinic input]
module _multibytecodec
class _multibytecodec.MultibyteCodec "MultibyteCodecObject *" "multibytecodec_state_get_state_by_type(type)->MultibyteCodec_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=88c1db8cc4d96844]*/

typedef struct {
    PyObject            *inobj;
    Py_ssize_t          inpos, inlen;
    unsigned char       *outbuf, *outbuf_end;
    PyObject            *excobj, *outobj;
} MultibyteEncodeBuffer;

typedef struct {
    const unsigned char *inbuf, *inbuf_top, *inbuf_end;
    PyObject            *excobj;
    _PyUnicodeWriter    writer;
} MultibyteDecodeBuffer;

static char *incnewkwarglist[] = {"errors", NULL};
static char *streamkwarglist[] = {"stream", "errors", NULL};

static PyObject *multibytecodec_encode(MultibyteCodec *,
                MultibyteCodec_State *, PyObject *, Py_ssize_t *,
                PyObject *, int);

#define MBENC_RESET     MBENC_MAX<<1 /* reset after an encoding session */

_Py_IDENTIFIER(write);

static PyObject *
make_tuple(PyObject *object, Py_ssize_t len)
{
    PyObject *v, *w;

    if (object == NULL)
        return NULL;

    v = PyTuple_New(2);
    if (v == NULL) {
        Py_DECREF(object);
        return NULL;
    }
    PyTuple_SET_ITEM(v, 0, object);

    w = PyLong_FromSsize_t(len);
    if (w == NULL) {
        Py_DECREF(v);
        return NULL;
    }
    PyTuple_SET_ITEM(v, 1, w);

    return v;
}

static PyObject *
internal_error_callback(const char *errors)
{
    if (errors == NULL || strcmp(errors, "strict") == 0)
        return ERROR_STRICT;
    else if (strcmp(errors, "ignore") == 0)
        return ERROR_IGNORE;
    else if (strcmp(errors, "replace") == 0)
        return ERROR_REPLACE;
    else
        return PyUnicode_FromString(errors);
}

static PyObject *
call_error_callback(PyObject *errors, PyObject *exc)
{
    PyObject *cb, *r;
    const char *str;

    assert(PyUnicode_Check(errors));
    str = PyUnicode_AsUTF8(errors);
    if (str == NULL)
        return NULL;
    cb = PyCodec_LookupError(str);
    if (cb == NULL)
        return NULL;

    r = PyObject_CallOneArg(cb, exc);
    Py_DECREF(cb);
    return r;
}

static PyObject *
codecctx_errors_get(MultibyteStatefulCodecContext *self, void *Py_UNUSED(ignored))
{
    const char *errors;

    if (self->errors == ERROR_STRICT)
        errors = "strict";
    else if (self->errors == ERROR_IGNORE)
        errors = "ignore";
    else if (self->errors == ERROR_REPLACE)
        errors = "replace";
    else {
        Py_INCREF(self->errors);
        return self->errors;
    }

    return PyUnicode_FromString(errors);
}

static int
codecctx_errors_set(MultibyteStatefulCodecContext *self, PyObject *value,
                    void *closure)
{
    PyObject *cb;
    const char *str;

    if (value == NULL) {
        PyErr_SetString(PyExc_AttributeError, "cannot delete attribute");
        return -1;
    }
    if (!PyUnicode_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "errors must be a string");
        return -1;
    }

    str = PyUnicode_AsUTF8(value);
    if (str == NULL)
        return -1;

    cb = internal_error_callback(str);
    if (cb == NULL)
        return -1;

    ERROR_DECREF(self->errors);
    self->errors = cb;
    return 0;
}

/* This getset handlers list is used by all the stateful codec objects */
static PyGetSetDef codecctx_getsets[] = {
    {"errors",          (getter)codecctx_errors_get,
                    (setter)codecctx_errors_set,
                    PyDoc_STR("how to treat errors")},
    {NULL,}
};

static int
expand_encodebuffer(MultibyteEncodeBuffer *buf, Py_ssize_t esize)
{
    Py_ssize_t orgpos, orgsize, incsize;

    orgpos = (Py_ssize_t)((char *)buf->outbuf -
                            PyBytes_AS_STRING(buf->outobj));
    orgsize = PyBytes_GET_SIZE(buf->outobj);
    incsize = (esize < (orgsize >> 1) ? (orgsize >> 1) | 1 : esize);

    if (orgsize > PY_SSIZE_T_MAX - incsize) {
        PyErr_NoMemory();
        return -1;
    }

    if (_PyBytes_Resize(&buf->outobj, orgsize + incsize) == -1)
        return -1;

    buf->outbuf = (unsigned char *)PyBytes_AS_STRING(buf->outobj) +orgpos;
    buf->outbuf_end = (unsigned char *)PyBytes_AS_STRING(buf->outobj)
        + PyBytes_GET_SIZE(buf->outobj);

    return 0;
}
#define REQUIRE_ENCODEBUFFER(buf, s) do {                               \
    if ((s) < 0 || (s) > (buf)->outbuf_end - (buf)->outbuf)             \
        if (expand_encodebuffer(buf, s) == -1)                          \
            goto errorexit;                                             \
} while(0)


/**
 * MultibyteCodec object
 */

static int
multibytecodec_encerror(MultibyteCodec *codec,
                        MultibyteCodec_State *state,
                        MultibyteEncodeBuffer *buf,
                        PyObject *errors, Py_ssize_t e)
{
    PyObject *retobj = NULL, *retstr = NULL, *tobj;
    Py_ssize_t retstrsize, newpos;
    Py_ssize_t esize, start, end;
    const char *reason;

    if (e > 0) {
        reason = "illegal multibyte sequence";
        esize = e;
    }
    else {
        switch (e) {
        case MBERR_TOOSMALL:
            REQUIRE_ENCODEBUFFER(buf, -1);
            return 0; /* retry it */
        case MBERR_TOOFEW:
            reason = "incomplete multibyte sequence";
            esize = (Py_ssize_t)buf->inpos;
            break;
        case MBERR_INTERNAL:
            PyErr_SetString(PyExc_RuntimeError,
                            "internal codec error");
            return -1;
        default:
            PyErr_SetString(PyExc_RuntimeError,
                            "unknown runtime error");
            return -1;
        }
    }

    if (errors == ERROR_REPLACE) {
        PyObject *replchar;
        Py_ssize_t r;
        Py_ssize_t inpos;
        int kind;
        const void *data;

        replchar = PyUnicode_FromOrdinal('?');
        if (replchar == NULL)
            goto errorexit;
        kind = PyUnicode_KIND(replchar);
        data = PyUnicode_DATA(replchar);

        inpos = 0;
        for (;;) {
            Py_ssize_t outleft = (Py_ssize_t)(buf->outbuf_end - buf->outbuf);

            r = codec->encode(state, codec->config,
                              kind, data, &inpos, 1,
                              &buf->outbuf, outleft, 0);
            if (r == MBERR_TOOSMALL) {
                REQUIRE_ENCODEBUFFER(buf, -1);
                continue;
            }
            else
                break;
        }

        Py_DECREF(replchar);

        if (r != 0) {
            REQUIRE_ENCODEBUFFER(buf, 1);
            *buf->outbuf++ = '?';
        }
    }
    if (errors == ERROR_IGNORE || errors == ERROR_REPLACE) {
        buf->inpos += esize;
        return 0;
    }

    start = (Py_ssize_t)buf->inpos;
    end = start + esize;

    /* use cached exception object if available */
    if (buf->excobj == NULL) {
        buf->excobj =  PyObject_CallFunction(PyExc_UnicodeEncodeError,
                                             "sOnns",
                                             codec->encoding, buf->inobj,
                                             start, end, reason);
        if (buf->excobj == NULL)
            goto errorexit;
    }
    else
        if (PyUnicodeEncodeError_SetStart(buf->excobj, start) != 0 ||
            PyUnicodeEncodeError_SetEnd(buf->excobj, end) != 0 ||
            PyUnicodeEncodeError_SetReason(buf->excobj, reason) != 0)
            goto errorexit;

    if (errors == ERROR_STRICT) {
        PyCodec_StrictErrors(buf->excobj);
        goto errorexit;
    }

    retobj = call_error_callback(errors, buf->excobj);
    if (retobj == NULL)
        goto errorexit;

    if (!PyTuple_Check(retobj) || PyTuple_GET_SIZE(retobj) != 2 ||
        (!PyUnicode_Check((tobj = PyTuple_GET_ITEM(retobj, 0))) && !PyBytes_Check(tobj)) ||
        !PyLong_Check(PyTuple_GET_ITEM(retobj, 1))) {
        PyErr_SetString(PyExc_TypeError,
                        "encoding error handler must return "
                        "(str, int) tuple");
        goto errorexit;
    }

    if (PyUnicode_Check(tobj)) {
        Py_ssize_t inpos;

        retstr = multibytecodec_encode(codec, state, tobj,
                        &inpos, ERROR_STRICT,
                        MBENC_FLUSH);
        if (retstr == NULL)
            goto errorexit;
    }
    else {
        Py_INCREF(tobj);
        retstr = tobj;
    }

    assert(PyBytes_Check(retstr));
    retstrsize = PyBytes_GET_SIZE(retstr);
    if (retstrsize > 0) {
        REQUIRE_ENCODEBUFFER(buf, retstrsize);
        memcpy(buf->outbuf, PyBytes_AS_STRING(retstr), retstrsize);
        buf->outbuf += retstrsize;
    }

    newpos = PyLong_AsSsize_t(PyTuple_GET_ITEM(retobj, 1));
    if (newpos < 0 && !PyErr_Occurred())
        newpos += (Py_ssize_t)buf->inlen;
    if (newpos < 0 || newpos > buf->inlen) {
        PyErr_Clear();
        PyErr_Format(PyExc_IndexError,
                     "position %zd from error handler out of bounds",
                     newpos);
        goto errorexit;
    }
    buf->inpos = newpos;

    Py_DECREF(retobj);
    Py_DECREF(retstr);
    return 0;

errorexit:
    Py_XDECREF(retobj);
    Py_XDECREF(retstr);
    return -1;
}

static int
multibytecodec_decerror(MultibyteCodec *codec,
                        MultibyteCodec_State *state,
                        MultibyteDecodeBuffer *buf,
                        PyObject *errors, Py_ssize_t e)
{
    PyObject *retobj = NULL, *retuni = NULL;
    Py_ssize_t newpos;
    const char *reason;
    Py_ssize_t esize, start, end;

    if (e > 0) {
        reason = "illegal multibyte sequence";
        esize = e;
    }
    else {
        switch (e) {
        case MBERR_TOOSMALL:
            return 0; /* retry it */
        case MBERR_TOOFEW:
            reason = "incomplete multibyte sequence";
            esize = (Py_ssize_t)(buf->inbuf_end - buf->inbuf);
            break;
        case MBERR_INTERNAL:
            PyErr_SetString(PyExc_RuntimeError,
                            "internal codec error");
            return -1;
        case MBERR_EXCEPTION:
            return -1;
        default:
            PyErr_SetString(PyExc_RuntimeError,
                            "unknown runtime error");
            return -1;
        }
    }

    if (errors == ERROR_REPLACE) {
        if (_PyUnicodeWriter_WriteChar(&buf->writer,
                                       Py_UNICODE_REPLACEMENT_CHARACTER) < 0)
            goto errorexit;
    }
    if (errors == ERROR_IGNORE || errors == ERROR_REPLACE) {
        buf->inbuf += esize;
        return 0;
    }

    start = (Py_ssize_t)(buf->inbuf - buf->inbuf_top);
    end = start + esize;

    /* use cached exception object if available */
    if (buf->excobj == NULL) {
        buf->excobj = PyUnicodeDecodeError_Create(codec->encoding,
                        (const char *)buf->inbuf_top,
                        (Py_ssize_t)(buf->inbuf_end - buf->inbuf_top),
                        start, end, reason);
        if (buf->excobj == NULL)
            goto errorexit;
    }
    else
        if (PyUnicodeDecodeError_SetStart(buf->excobj, start) ||
            PyUnicodeDecodeError_SetEnd(buf->excobj, end) ||
            PyUnicodeDecodeError_SetReason(buf->excobj, reason))
            goto errorexit;

    if (errors == ERROR_STRICT) {
        PyCodec_StrictErrors(buf->excobj);
        goto errorexit;
    }

    retobj = call_error_callback(errors, buf->excobj);
    if (retobj == NULL)
        goto errorexit;

    if (!PyTuple_Check(retobj) || PyTuple_GET_SIZE(retobj) != 2 ||
        !PyUnicode_Check((retuni = PyTuple_GET_ITEM(retobj, 0))) ||
        !PyLong_Check(PyTuple_GET_ITEM(retobj, 1))) {
        PyErr_SetString(PyExc_TypeError,
                        "decoding error handler must return "
                        "(str, int) tuple");
        goto errorexit;
    }

    if (_PyUnicodeWriter_WriteStr(&buf->writer, retuni) < 0)
        goto errorexit;

    newpos = PyLong_AsSsize_t(PyTuple_GET_ITEM(retobj, 1));
    if (newpos < 0 && !PyErr_Occurred())
        newpos += (Py_ssize_t)(buf->inbuf_end - buf->inbuf_top);
    if (newpos < 0 || buf->inbuf_top + newpos > buf->inbuf_end) {
        PyErr_Clear();
        PyErr_Format(PyExc_IndexError,
                     "position %zd from error handler out of bounds",
                     newpos);
        goto errorexit;
    }
    buf->inbuf = buf->inbuf_top + newpos;
    Py_DECREF(retobj);
    return 0;

errorexit:
    Py_XDECREF(retobj);
    return -1;
}

static PyObject *
multibytecodec_encode(MultibyteCodec *codec,
                      MultibyteCodec_State *state,
                      PyObject *text, Py_ssize_t *inpos_t,
                      PyObject *errors, int flags)
{
    MultibyteEncodeBuffer buf;
    Py_ssize_t finalsize, r = 0;
    Py_ssize_t datalen;
    int kind;
    const void *data;

    if (PyUnicode_READY(text) < 0)
        return NULL;
    datalen = PyUnicode_GET_LENGTH(text);

    if (datalen == 0 && !(flags & MBENC_RESET))
        return PyBytes_FromStringAndSize(NULL, 0);

    buf.excobj = NULL;
    buf.outobj = NULL;
    buf.inobj = text;   /* borrowed reference */
    buf.inpos = 0;
    buf.inlen = datalen;
    kind = PyUnicode_KIND(buf.inobj);
    data = PyUnicode_DATA(buf.inobj);

    if (datalen > (PY_SSIZE_T_MAX - 16) / 2) {
        PyErr_NoMemory();
        goto errorexit;
    }

    buf.outobj = PyBytes_FromStringAndSize(NULL, datalen * 2 + 16);
    if (buf.outobj == NULL)
        goto errorexit;
    buf.outbuf = (unsigned char *)PyBytes_AS_STRING(buf.outobj);
    buf.outbuf_end = buf.outbuf + PyBytes_GET_SIZE(buf.outobj);

    while (buf.inpos < buf.inlen) {
        /* we don't reuse inleft and outleft here.
         * error callbacks can relocate the cursor anywhere on buffer*/
        Py_ssize_t outleft = (Py_ssize_t)(buf.outbuf_end - buf.outbuf);

        r = codec->encode(state, codec->config,
                          kind, data,
                          &buf.inpos, buf.inlen,
                          &buf.outbuf, outleft, flags);
        if ((r == 0) || (r == MBERR_TOOFEW && !(flags & MBENC_FLUSH)))
            break;
        else if (multibytecodec_encerror(codec, state, &buf, errors,r))
            goto errorexit;
        else if (r == MBERR_TOOFEW)
            break;
    }

    if (codec->encreset != NULL && (flags & MBENC_RESET))
        for (;;) {
            Py_ssize_t outleft;

            outleft = (Py_ssize_t)(buf.outbuf_end - buf.outbuf);
            r = codec->encreset(state, codec->config, &buf.outbuf,
                                outleft);
            if (r == 0)
                break;
            else if (multibytecodec_encerror(codec, state,
                                             &buf, errors, r))
                goto errorexit;
        }

    finalsize = (Py_ssize_t)((char *)buf.outbuf -
                             PyBytes_AS_STRING(buf.outobj));

    if (finalsize != PyBytes_GET_SIZE(buf.outobj))
        if (_PyBytes_Resize(&buf.outobj, finalsize) == -1)
            goto errorexit;

    if (inpos_t)
        *inpos_t = buf.inpos;
    Py_XDECREF(buf.excobj);
    return buf.outobj;

errorexit:
    Py_XDECREF(buf.excobj);
    Py_XDECREF(buf.outobj);
    return NULL;
}

/*[clinic input]
_multibytecodec.MultibyteCodec.encode
  cls: defining_class
  /
  input: object
  errors: str(accept={str, NoneType}) = None

Return an encoded string version of `input'.

'errors' may be given to set a different error handling scheme. Default is
'strict' meaning that encoding errors raise a UnicodeEncodeError. Other possible
values are 'ignore', 'replace' and 'xmlcharrefreplace' as well as any other name
registered with codecs.register_error that can handle UnicodeEncodeErrors.
[clinic start generated code]*/

static PyObject *
_multibytecodec_MultibyteCodec_encode_impl(MultibyteCodecObject *self,
                                           PyTypeObject *cls,
                                           PyObject *input,
                                           const char *errors)
/*[clinic end generated code: output=9cb1d3bbb2a420e2 input=373193fcff117355]*/
{
    MultibyteCodec_State state;
    PyObject *errorcb, *r, *ucvt;
    Py_ssize_t datalen;

    if (PyUnicode_Check(input))
        ucvt = NULL;
    else {
        input = ucvt = PyObject_Str(input);
        if (input == NULL)
            return NULL;
        else if (!PyUnicode_Check(input)) {
            PyErr_SetString(PyExc_TypeError,
                "couldn't convert the object to unicode.");
            Py_DECREF(ucvt);
            return NULL;
        }
    }

    if (PyUnicode_READY(input) < 0) {
        Py_XDECREF(ucvt);
        return NULL;
    }
    datalen = PyUnicode_GET_LENGTH(input);

    errorcb = internal_error_callback(errors);
    if (errorcb == NULL) {
        Py_XDECREF(ucvt);
        return NULL;
    }

    if (self->codec->encinit != NULL &&
        self->codec->encinit(&state, self->codec->config) != 0)
        goto errorexit;
    r = multibytecodec_encode(self->codec, &state,
                    input, NULL, errorcb,
                    MBENC_FLUSH | MBENC_RESET);
    if (r == NULL)
        goto errorexit;

    ERROR_DECREF(errorcb);
    Py_XDECREF(ucvt);
    return make_tuple(r, datalen);

errorexit:
    ERROR_DECREF(errorcb);
    Py_XDECREF(ucvt);
    return NULL;
}

/*[clinic input]
_multibytecodec.MultibyteCodec.decode
  cls: defining_class
  /
  input: Py_buffer
  errors: str(accept={str, NoneType}) = None

Decodes 'input'.

'errors' may be given to set a different error handling scheme. Default is
'strict' meaning that encoding errors raise a UnicodeDecodeError. Other possible
values are 'ignore' and 'replace' as well as any other name registered with
codecs.register_error that is able to handle UnicodeDecodeErrors."
[clinic start generated code]*/

static PyObject *
_multibytecodec_MultibyteCodec_decode_impl(MultibyteCodecObject *self,
                                           PyTypeObject *cls,
                                           Py_buffer *input,
                                           const char *errors)
/*[clinic end generated code: output=8d38694c885fc6d1 input=e3bbfbe781840023]*/
{
    MultibyteCodec_State state;
    MultibyteDecodeBuffer buf;
    PyObject *errorcb, *res;
    const char *data;
    Py_ssize_t datalen;

    data = input->buf;
    datalen = input->len;

    errorcb = internal_error_callback(errors);
    if (errorcb == NULL) {
        return NULL;
    }

    if (datalen == 0) {
        ERROR_DECREF(errorcb);
        return make_tuple(PyUnicode_New(0, 0), 0);
    }

    _PyUnicodeWriter_Init(&buf.writer);
    buf.writer.min_length = datalen;
    buf.excobj = NULL;
    buf.inbuf = buf.inbuf_top = (unsigned char *)data;
    buf.inbuf_end = buf.inbuf_top + datalen;

    if (self->codec->decinit != NULL &&
        self->codec->decinit(&state, self->codec->config) != 0)
        goto errorexit;

    while (buf.inbuf < buf.inbuf_end) {
        Py_ssize_t inleft, r;

        inleft = (Py_ssize_t)(buf.inbuf_end - buf.inbuf);

        r = self->codec->decode(&state, self->codec->config,
                        &buf.inbuf, inleft, &buf.writer);
        if (r == 0)
            break;
        else if (multibytecodec_decerror(self->codec, &state,
                                         &buf, errorcb, r))
            goto errorexit;
    }

    res = _PyUnicodeWriter_Finish(&buf.writer);
    if (res == NULL)
        goto errorexit;

    Py_XDECREF(buf.excobj);
    ERROR_DECREF(errorcb);
    return make_tuple(res, datalen);

errorexit:
    ERROR_DECREF(errorcb);
    Py_XDECREF(buf.excobj);
    _PyUnicodeWriter_Dealloc(&buf.writer);

    return NULL;
}

static struct PyMethodDef multibytecodec_methods[] = {
    _MULTIBYTECODEC_MULTIBYTECODEC_ENCODE_METHODDEF
    _MULTIBYTECODEC_MULTIBYTECODEC_DECODE_METHODDEF
    {NULL, NULL},
};

static void
multibytecodec_dealloc(MultibyteCodecObject *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    tp->tp_free(self);
    Py_DECREF(tp);
}

static PyType_Slot MultibyteCodec_Type_slots[] = {
    {Py_tp_getattr, PyObject_GenericGetAttr},
    {Py_tp_methods, multibytecodec_methods},
    {Py_tp_dealloc, multibytecodec_dealloc},
    {0, NULL}
};

PyType_Spec MultibyteCodec_Type_spec = {
    .name = "MultibyteCodec",
    .basicsize = sizeof(MultibyteCodecObject),
    .flags = Py_TPFLAGS_DEFAULT,
    .slots = MultibyteCodec_Type_slots,
};

/**
 * Utility functions for stateful codec mechanism
 */

#define STATEFUL_DCTX(o)        ((MultibyteStatefulDecoderContext *)(o))
#define STATEFUL_ECTX(o)        ((MultibyteStatefulEncoderContext *)(o))

static PyObject *
encoder_encode_stateful(MultibyteStatefulEncoderContext *ctx,
                        PyObject *unistr, int final)
{
    PyObject *ucvt, *r = NULL;
    PyObject *inbuf = NULL;
    Py_ssize_t inpos, datalen;
    PyObject *origpending = NULL;

    if (PyUnicode_Check(unistr))
        ucvt = NULL;
    else {
        unistr = ucvt = PyObject_Str(unistr);
        if (unistr == NULL)
            return NULL;
        else if (!PyUnicode_Check(unistr)) {
            PyErr_SetString(PyExc_TypeError,
                "couldn't convert the object to str.");
            Py_DECREF(ucvt);
            return NULL;
        }
    }

    if (ctx->pending) {
        PyObject *inbuf_tmp;

        Py_INCREF(ctx->pending);
        origpending = ctx->pending;

        Py_INCREF(ctx->pending);
        inbuf_tmp = ctx->pending;
        PyUnicode_Append(&inbuf_tmp, unistr);
        if (inbuf_tmp == NULL)
            goto errorexit;
        Py_CLEAR(ctx->pending);
        inbuf = inbuf_tmp;
    }
    else {
        origpending = NULL;

        Py_INCREF(unistr);
        inbuf = unistr;
    }
    if (PyUnicode_READY(inbuf) < 0)
        goto errorexit;
    inpos = 0;
    datalen = PyUnicode_GET_LENGTH(inbuf);

    r = multibytecodec_encode(ctx->codec, &ctx->state,
                              inbuf, &inpos,
                              ctx->errors, final ? MBENC_FLUSH | MBENC_RESET : 0);
    if (r == NULL) {
        /* recover the original pending buffer */
        Py_XSETREF(ctx->pending, origpending);
        origpending = NULL;
        goto errorexit;
    }
    Py_XDECREF(origpending);

    if (inpos < datalen) {
        if (datalen - inpos > MAXENCPENDING) {
            /* normal codecs can't reach here */
            PyErr_SetString(PyExc_UnicodeError,
                            "pending buffer overflow");
            goto errorexit;
        }
        ctx->pending = PyUnicode_Substring(inbuf, inpos, datalen);
        if (ctx->pending == NULL) {
            /* normal codecs can't reach here */
            goto errorexit;
        }
    }

    Py_DECREF(inbuf);
    Py_XDECREF(ucvt);
    return r;

errorexit:
    Py_XDECREF(r);
    Py_XDECREF(ucvt);
    Py_XDECREF(origpending);
    Py_XDECREF(inbuf);
    return NULL;
}

static int
decoder_append_pending(MultibyteStatefulDecoderContext *ctx,
                       MultibyteDecodeBuffer *buf)
{
    Py_ssize_t npendings;

    npendings = (Py_ssize_t)(buf->inbuf_end - buf->inbuf);
    if (npendings + ctx->pendingsize > MAXDECPENDING ||
        npendings > PY_SSIZE_T_MAX - ctx->pendingsize) {
            PyErr_SetString(PyExc_UnicodeError, "pending buffer overflow");
            return -1;
    }
    memcpy(ctx->pending + ctx->pendingsize, buf->inbuf, npendings);
    ctx->pendingsize += npendings;
    return 0;
}

static int
decoder_prepare_buffer(MultibyteDecodeBuffer *buf, const char *data,
                       Py_ssize_t size)
{
    buf->inbuf = buf->inbuf_top = (const unsigned char *)data;
    buf->inbuf_end = buf->inbuf_top + size;
    buf->writer.min_length += size;
    return 0;
}

static int
decoder_feed_buffer(MultibyteStatefulDecoderContext *ctx,
                    MultibyteDecodeBuffer *buf)
{
    while (buf->inbuf < buf->inbuf_end) {
        Py_ssize_t inleft;
        Py_ssize_t r;

        inleft = (Py_ssize_t)(buf->inbuf_end - buf->inbuf);

        r = ctx->codec->decode(&ctx->state, ctx->codec->config,
            &buf->inbuf, inleft, &buf->writer);
        if (r == 0 || r == MBERR_TOOFEW)
            break;
        else if (multibytecodec_decerror(ctx->codec, &ctx->state,
                                         buf, ctx->errors, r))
            return -1;
    }
    return 0;
}


/*[clinic input]
 class _multibytecodec.MultibyteIncrementalEncoder "MultibyteIncrementalEncoderObject *" "multibytecodec_state_get_state_by_type(type)->MultibyteIncrementalEncoder_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=935796dddc69d4fe]*/

/*[clinic input]
_multibytecodec.MultibyteIncrementalEncoder.encode
    cls: defining_class
    /
    input: object
    final: bool(accept={int}) = False
[clinic start generated code]*/

static PyObject *
_multibytecodec_MultibyteIncrementalEncoder_encode_impl(MultibyteIncrementalEncoderObject *self,
                                                        PyTypeObject *cls,
                                                        PyObject *input,
                                                        int final)
/*[clinic end generated code: output=6693a2ebd28951ad input=914921164ec17708]*/
{
    return encoder_encode_stateful(STATEFUL_ECTX(self), input, final);
}

/*[clinic input]
_multibytecodec.MultibyteIncrementalEncoder.getstate
[clinic start generated code]*/

static PyObject *
_multibytecodec_MultibyteIncrementalEncoder_getstate_impl(MultibyteIncrementalEncoderObject *self)
/*[clinic end generated code: output=9794a5ace70d7048 input=4a2a82874ffa40bb]*/
{
    /* state made up of 1 byte for buffer size, up to MAXENCPENDING*4 bytes
       for UTF-8 encoded buffer (each character can use up to 4
       bytes), and required bytes for MultibyteCodec_State.c. A byte
       array is used to avoid different compilers generating different
       values for the same state, e.g. as a result of struct padding.
    */
    unsigned char statebytes[1 + MAXENCPENDING*4 + sizeof(self->state.c)];
    Py_ssize_t statesize;
    const char *pendingbuffer = NULL;
    Py_ssize_t pendingsize;

    if (self->pending != NULL) {
        pendingbuffer = PyUnicode_AsUTF8AndSize(self->pending, &pendingsize);
        if (pendingbuffer == NULL) {
            return NULL;
        }
        if (pendingsize > MAXENCPENDING*4) {
            PyErr_SetString(PyExc_UnicodeError, "pending buffer too large");
            return NULL;
        }
        statebytes[0] = (unsigned char)pendingsize;
        memcpy(statebytes + 1, pendingbuffer, pendingsize);
        statesize = 1 + pendingsize;
    } else {
        statebytes[0] = 0;
        statesize = 1;
    }
    memcpy(statebytes+statesize, self->state.c,
           sizeof(self->state.c));
    statesize += sizeof(self->state.c);

    return (PyObject *)_PyLong_FromByteArray(statebytes, statesize,
                                             1 /* little-endian */ ,
                                             0 /* unsigned */ );
}

/*[clinic input]
_multibytecodec.MultibyteIncrementalEncoder.setstate
    cls: defining_class
    state as statelong: object(type='PyLongObject *', subclass_of='&PyLong_Type')
    /
[clinic start generated code]*/

static PyObject *
_multibytecodec_MultibyteIncrementalEncoder_setstate_impl(MultibyteIncrementalEncoderObject *self,
                                                          PyTypeObject *cls,
                                                          PyLongObject *statelong)
/*[clinic end generated code: output=d34414536fa724bd input=c7e7266e7d215eed]*/
{
    PyObject *pending = NULL;
    unsigned char statebytes[1 + MAXENCPENDING*4 + sizeof(self->state.c)];

    if (_PyLong_AsByteArray(statelong, statebytes, sizeof(statebytes),
                            1 /* little-endian */ ,
                            0 /* unsigned */ ) < 0) {
        goto errorexit;
    }

    if (statebytes[0] > MAXENCPENDING*4) {
        PyErr_SetString(PyExc_UnicodeError, "pending buffer too large");
        return NULL;
    }

    pending = PyUnicode_DecodeUTF8((const char *)statebytes+1,
                                   statebytes[0], "strict");
    if (pending == NULL) {
        goto errorexit;
    }

    Py_CLEAR(self->pending);
    self->pending = pending;
    memcpy(self->state.c, statebytes+1+statebytes[0],
           sizeof(self->state.c));

    Py_RETURN_NONE;

errorexit:
    Py_XDECREF(pending);
    return NULL;
}

/*[clinic input]
_multibytecodec.MultibyteIncrementalEncoder.reset
    cls: defining_class
    /
[clinic start generated code]*/

static PyObject *
_multibytecodec_MultibyteIncrementalEncoder_reset_impl(MultibyteIncrementalEncoderObject *self,
                                                       PyTypeObject *cls)
/*[clinic end generated code: output=1a0aaff6a320641e input=19cd51b5254a1f59]*/
{
    /* Longest output: 4 bytes (b'\x0F\x1F(B') with ISO 2022 */
    unsigned char buffer[4], *outbuf;
    Py_ssize_t r;
    if (self->codec->encreset != NULL) {
        outbuf = buffer;
        r = self->codec->encreset(&self->state, self->codec->config,
                                  &outbuf, sizeof(buffer));
        if (r != 0)
            return NULL;
    }
    Py_CLEAR(self->pending);
    Py_RETURN_NONE;
}

static struct PyMethodDef mbiencoder_methods[] = {
    _MULTIBYTECODEC_MULTIBYTEINCREMENTALENCODER_ENCODE_METHODDEF
    _MULTIBYTECODEC_MULTIBYTEINCREMENTALENCODER_GETSTATE_METHODDEF
    _MULTIBYTECODEC_MULTIBYTEINCREMENTALENCODER_SETSTATE_METHODDEF
    _MULTIBYTECODEC_MULTIBYTEINCREMENTALENCODER_RESET_METHODDEF
    {NULL, NULL},
};

static PyObject *
mbiencoder_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    MultibyteIncrementalEncoderObject *self;
    PyObject *codec = NULL;
    char *errors = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|s:IncrementalEncoder",
                                     incnewkwarglist, &errors))
        return NULL;

    self = (MultibyteIncrementalEncoderObject *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    codec = PyObject_GetAttrString((PyObject *)type, "codec");
    if (codec == NULL)
        goto errorexit;

    multibytecodec_state *state = PyType_GetModuleState(type);
    if (!Py_IS_TYPE(codec, (PyTypeObject*)state->MultibyteCodec_Type)) {
        PyErr_SetString(PyExc_TypeError, "codec is unexpected type");
        goto errorexit;
    }

    self->codec = ((MultibyteCodecObject *)codec)->codec;
    self->pending = NULL;
    self->errors = internal_error_callback(errors);
    if (self->errors == NULL)
        goto errorexit;
    if (self->codec->encinit != NULL &&
        self->codec->encinit(&self->state, self->codec->config) != 0)
        goto errorexit;

    Py_DECREF(codec);
    return (PyObject *)self;

errorexit:
    Py_XDECREF(self);
    Py_XDECREF(codec);
    return NULL;
}

static int
mbiencoder_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    return 0;
}

static int
mbiencoder_traverse(MultibyteIncrementalEncoderObject *self,
                    visitproc visit, void *arg)
{
    if (ERROR_ISCUSTOM(self->errors))
        Py_VISIT(self->errors);
    return 0;
}

static void
mbiencoder_dealloc(MultibyteIncrementalEncoderObject *self)
{
    PyObject_GC_UnTrack(self);
    ERROR_DECREF(self->errors);
    Py_CLEAR(self->pending);
    PyTypeObject *tp = Py_TYPE(self);
    tp->tp_free(self);
    Py_DECREF(tp);
}

static PyType_Slot MultibyteIncrementalEncoder_Type_slots[] = {
    {Py_tp_init, mbiencoder_init},
    {Py_tp_getset, codecctx_getsets},
    {Py_tp_new, mbiencoder_new},
    {Py_tp_getattr, PyObject_GenericGetAttr},
    {Py_tp_methods, mbiencoder_methods},
    {Py_tp_traverse, mbiencoder_traverse},
    {Py_tp_dealloc, mbiencoder_dealloc},
    {0, NULL}
};

static PyType_Spec MultibyteIncrementalEncoder_Type_spec = {
    .name = "MultibyteIncrementalEncoder",
    .basicsize = sizeof(MultibyteIncrementalEncoderObject),
    .itemsize = 0,
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE,
    .slots = MultibyteIncrementalEncoder_Type_slots
};


/*[clinic input]
 class _multibytecodec.MultibyteIncrementalDecoder "MultibyteIncrementalDecoderObject *" "multibytecodec_state_get_state_by_type(type)->MultibyteIncrementalDecoder_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=3a58b4545c739a3c]*/

/*[clinic input]
_multibytecodec.MultibyteIncrementalDecoder.decode
    cls: defining_class
    /
    input: Py_buffer
    final: bool(accept={int}) = False
[clinic start generated code]*/

static PyObject *
_multibytecodec_MultibyteIncrementalDecoder_decode_impl(MultibyteIncrementalDecoderObject *self,
                                                        PyTypeObject *cls,
                                                        Py_buffer *input,
                                                        int final)
/*[clinic end generated code: output=a552388ea9e77ffb input=2d5d72ca332ed27e]*/
{
    MultibyteDecodeBuffer buf;
    char *data, *wdata = NULL;
    Py_ssize_t wsize, size, origpending;
    PyObject *res;

    data = input->buf;
    size = input->len;

    _PyUnicodeWriter_Init(&buf.writer);
    buf.excobj = NULL;
    origpending = self->pendingsize;

    if (self->pendingsize == 0) {
        wsize = size;
        wdata = data;
    }
    else {
        if (size > PY_SSIZE_T_MAX - self->pendingsize) {
            PyErr_NoMemory();
            goto errorexit;
        }
        wsize = size + self->pendingsize;
        wdata = PyMem_Malloc(wsize);
        if (wdata == NULL) {
            PyErr_NoMemory();
            goto errorexit;
        }
        memcpy(wdata, self->pending, self->pendingsize);
        memcpy(wdata + self->pendingsize, data, size);
        self->pendingsize = 0;
    }

    if (decoder_prepare_buffer(&buf, wdata, wsize) != 0)
        goto errorexit;

    if (decoder_feed_buffer(STATEFUL_DCTX(self), &buf))
        goto errorexit;

    if (final && buf.inbuf < buf.inbuf_end) {
        if (multibytecodec_decerror(self->codec, &self->state,
                        &buf, self->errors, MBERR_TOOFEW)) {
            /* recover the original pending buffer */
            memcpy(self->pending, wdata, origpending);
            self->pendingsize = origpending;
            goto errorexit;
        }
    }

    if (buf.inbuf < buf.inbuf_end) { /* pending sequence still exists */
        if (decoder_append_pending(STATEFUL_DCTX(self), &buf) != 0)
            goto errorexit;
    }

    res = _PyUnicodeWriter_Finish(&buf.writer);
    if (res == NULL)
        goto errorexit;

    if (wdata != data)
        PyMem_Free(wdata);
    Py_XDECREF(buf.excobj);
    return res;

errorexit:
    if (wdata != NULL && wdata != data)
        PyMem_Free(wdata);
    Py_XDECREF(buf.excobj);
    _PyUnicodeWriter_Dealloc(&buf.writer);
    return NULL;
}

/*[clinic input]
_multibytecodec.MultibyteIncrementalDecoder.getstate
   cls: defining_class
   /
[clinic start generated code]*/

static PyObject *
_multibytecodec_MultibyteIncrementalDecoder_getstate_impl(MultibyteIncrementalDecoderObject *self,
                                                          PyTypeObject *cls)
/*[clinic end generated code: output=805380dd1eeddc9d input=6a60a81d729875c2]*/
{
    PyObject *buffer;
    PyObject *statelong;

    buffer = PyBytes_FromStringAndSize((const char *)self->pending,
                                       self->pendingsize);
    if (buffer == NULL) {
        return NULL;
    }

    statelong = (PyObject *)_PyLong_FromByteArray(self->state.c,
                                                  sizeof(self->state.c),
                                                  1 /* little-endian */ ,
                                                  0 /* unsigned */ );
    if (statelong == NULL) {
        Py_DECREF(buffer);
        return NULL;
    }

    return Py_BuildValue("NN", buffer, statelong);
}

/*[clinic input]
_multibytecodec.MultibyteIncrementalDecoder.setstate
    cls: defining_class
    state: object(subclass_of='&PyTuple_Type')
    /
[clinic start generated code]*/

static PyObject *
_multibytecodec_MultibyteIncrementalDecoder_setstate_impl(MultibyteIncrementalDecoderObject *self,
                                                          PyTypeObject *cls,
                                                          PyObject *state)
/*[clinic end generated code: output=b276e97436406d97 input=3cbf017dff76c147]*/
{
    PyObject *buffer;
    PyLongObject *statelong;
    Py_ssize_t buffersize;
    const char *bufferstr;
    unsigned char statebytes[8];

    if (!PyArg_ParseTuple(state, "SO!;setstate(): illegal state argument",
                          &buffer, &PyLong_Type, &statelong))
    {
        return NULL;
    }

    if (_PyLong_AsByteArray(statelong, statebytes, sizeof(statebytes),
                            1 /* little-endian */ ,
                            0 /* unsigned */ ) < 0) {
        return NULL;
    }

    buffersize = PyBytes_Size(buffer);
    if (buffersize == -1) {
        return NULL;
    }

    if (buffersize > MAXDECPENDING) {
        PyErr_SetString(PyExc_UnicodeError, "pending buffer too large");
        return NULL;
    }

    bufferstr = PyBytes_AsString(buffer);
    if (bufferstr == NULL) {
        return NULL;
    }
    self->pendingsize = buffersize;
    memcpy(self->pending, bufferstr, self->pendingsize);
    memcpy(self->state.c, statebytes, sizeof(statebytes));

    Py_RETURN_NONE;
}

/*[clinic input]
_multibytecodec.MultibyteIncrementalDecoder.reset
    cls: defining_class
    /
[clinic start generated code]*/

static PyObject *
_multibytecodec_MultibyteIncrementalDecoder_reset_impl(MultibyteIncrementalDecoderObject *self,
                                                       PyTypeObject *cls)
/*[clinic end generated code: output=47239b078242652b input=f8bffeb090840288]*/
{
    if (self->codec->decreset != NULL &&
        self->codec->decreset(&self->state, self->codec->config) != 0)
        return NULL;
    self->pendingsize = 0;

    Py_RETURN_NONE;
}

static struct PyMethodDef mbidecoder_methods[] = {
    _MULTIBYTECODEC_MULTIBYTEINCREMENTALDECODER_DECODE_METHODDEF
    _MULTIBYTECODEC_MULTIBYTEINCREMENTALDECODER_GETSTATE_METHODDEF
    _MULTIBYTECODEC_MULTIBYTEINCREMENTALDECODER_SETSTATE_METHODDEF
    _MULTIBYTECODEC_MULTIBYTEINCREMENTALDECODER_RESET_METHODDEF
    {NULL, NULL},
};

static PyObject *
mbidecoder_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    MultibyteIncrementalDecoderObject *self;
    PyObject *codec = NULL;
    char *errors = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|s:IncrementalDecoder",
                                     incnewkwarglist, &errors))
        return NULL;

    self = (MultibyteIncrementalDecoderObject *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    codec = PyObject_GetAttrString((PyObject *)type, "codec");
    if (codec == NULL)
        goto errorexit;

    multibytecodec_state *state = PyType_GetModuleState(type);
    if (!Py_IS_TYPE(codec, (PyTypeObject*)state->MultibyteCodec_Type)) {
        PyErr_SetString(PyExc_TypeError, "codec is unexpected type");
        goto errorexit;
    }

    self->codec = ((MultibyteCodecObject *)codec)->codec;
    self->pendingsize = 0;
    self->errors = internal_error_callback(errors);
    if (self->errors == NULL)
        goto errorexit;
    if (self->codec->decinit != NULL &&
        self->codec->decinit(&self->state, self->codec->config) != 0)
        goto errorexit;

    Py_DECREF(codec);
    return (PyObject *)self;

errorexit:
    Py_XDECREF(self);
    Py_XDECREF(codec);
    return NULL;
}

static int
mbidecoder_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    return 0;
}

static int
mbidecoder_traverse(MultibyteIncrementalDecoderObject *self,
                    visitproc visit, void *arg)
{
    if (ERROR_ISCUSTOM(self->errors))
        Py_VISIT(self->errors);
    return 0;
}

static void
mbidecoder_dealloc(MultibyteIncrementalDecoderObject *self)
{
    PyObject_GC_UnTrack(self);
    ERROR_DECREF(self->errors);
    PyTypeObject *tp = Py_TYPE(self);
    tp->tp_free(self);
    Py_DECREF(tp);
}

static PyType_Slot MultibyteIncrementalDecoder_slots[] = {
    {Py_tp_init, mbidecoder_init},
    {Py_tp_getset, codecctx_getsets},
    {Py_tp_new, mbidecoder_new},
    {Py_tp_getattr, PyObject_GenericGetAttr},
    {Py_tp_methods, mbidecoder_methods},
    {Py_tp_traverse, mbidecoder_traverse},
    {Py_tp_dealloc, mbidecoder_dealloc},
    {0, NULL}
};

static PyType_Spec MultibyteIncrementalDecoder_Type_spec = {
    .name = "MultibyteIncrementalDecoder",
    .basicsize = sizeof(MultibyteIncrementalDecoderObject),
    .itemsize = 0,
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE,
    .slots = MultibyteIncrementalDecoder_slots
};

/*[clinic input]
 class _multibytecodec.MultibyteStreamReader "MultibyteStreamReaderObject *" "multibytecodec_state_get_state_by_type(type)->MultibyteStreamReader_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=760303b12956d124]*/

static PyObject *
mbstreamreader_iread(MultibyteStreamReaderObject *self,
                     const char *method, Py_ssize_t sizehint)
{
    MultibyteDecodeBuffer buf;
    PyObject *cres, *res;
    Py_ssize_t rsize;

    if (sizehint == 0)
        return PyUnicode_New(0, 0);

    _PyUnicodeWriter_Init(&buf.writer);
    buf.excobj = NULL;
    cres = NULL;

    for (;;) {
        int endoffile;

        if (sizehint < 0)
            cres = PyObject_CallMethod(self->stream,
                            method, NULL);
        else
            cres = PyObject_CallMethod(self->stream,
                            method, "i", sizehint);
        if (cres == NULL)
            goto errorexit;

        if (!PyBytes_Check(cres)) {
            PyErr_Format(PyExc_TypeError,
                         "stream function returned a "
                         "non-bytes object (%.100s)",
                         Py_TYPE(cres)->tp_name);
            goto errorexit;
        }

        endoffile = (PyBytes_GET_SIZE(cres) == 0);

        if (self->pendingsize > 0) {
            PyObject *ctr;
            char *ctrdata;

            if (PyBytes_GET_SIZE(cres) > PY_SSIZE_T_MAX - self->pendingsize) {
                PyErr_NoMemory();
                goto errorexit;
            }
            rsize = PyBytes_GET_SIZE(cres) + self->pendingsize;
            ctr = PyBytes_FromStringAndSize(NULL, rsize);
            if (ctr == NULL)
                goto errorexit;
            ctrdata = PyBytes_AS_STRING(ctr);
            memcpy(ctrdata, self->pending, self->pendingsize);
            memcpy(ctrdata + self->pendingsize,
                    PyBytes_AS_STRING(cres),
                    PyBytes_GET_SIZE(cres));
            Py_DECREF(cres);
            cres = ctr;
            self->pendingsize = 0;
        }

        rsize = PyBytes_GET_SIZE(cres);
        if (decoder_prepare_buffer(&buf, PyBytes_AS_STRING(cres),
                                   rsize) != 0)
            goto errorexit;

        if (rsize > 0 && decoder_feed_buffer(
                        (MultibyteStatefulDecoderContext *)self, &buf))
            goto errorexit;

        if (endoffile || sizehint < 0) {
            if (buf.inbuf < buf.inbuf_end &&
                multibytecodec_decerror(self->codec, &self->state,
                            &buf, self->errors, MBERR_TOOFEW))
                goto errorexit;
        }

        if (buf.inbuf < buf.inbuf_end) { /* pending sequence exists */
            if (decoder_append_pending(STATEFUL_DCTX(self),
                                       &buf) != 0)
                goto errorexit;
        }

        Py_DECREF(cres);
        cres = NULL;

        if (sizehint < 0 || buf.writer.pos != 0 || rsize == 0)
            break;

        sizehint = 1; /* read 1 more byte and retry */
    }

    res = _PyUnicodeWriter_Finish(&buf.writer);
    if (res == NULL)
        goto errorexit;

    Py_XDECREF(cres);
    Py_XDECREF(buf.excobj);
    return res;

errorexit:
    Py_XDECREF(cres);
    Py_XDECREF(buf.excobj);
    _PyUnicodeWriter_Dealloc(&buf.writer);
    return NULL;
}

/*[clinic input]
 _multibytecodec.MultibyteStreamReader.read
    cls: defining_class
    sizeobj: object = None
    /
[clinic start generated code]*/

static PyObject *
_multibytecodec_MultibyteStreamReader_read_impl(MultibyteStreamReaderObject *self,
                                                PyTypeObject *cls,
                                                PyObject *sizeobj)
/*[clinic end generated code: output=8a7ca418257021ac input=ce2248d5268aa613]*/
{
    Py_ssize_t size;

    if (sizeobj == Py_None)
        size = -1;
    else if (PyLong_Check(sizeobj))
        size = PyLong_AsSsize_t(sizeobj);
    else {
        PyErr_SetString(PyExc_TypeError, "arg 1 must be an integer");
        return NULL;
    }

    if (size == -1 && PyErr_Occurred())
        return NULL;

    return mbstreamreader_iread(self, "read", size);
}

/*[clinic input]
 _multibytecodec.MultibyteStreamReader.readline
    cls: defining_class
    sizeobj: object = None
    /
[clinic start generated code]*/

static PyObject *
_multibytecodec_MultibyteStreamReader_readline_impl(MultibyteStreamReaderObject *self,
                                                    PyTypeObject *cls,
                                                    PyObject *sizeobj)
/*[clinic end generated code: output=432e5621a8ab4492 input=9228f1aae3ad9771]*/
{
    Py_ssize_t size;

    if (sizeobj == Py_None)
        size = -1;
    else if (PyLong_Check(sizeobj))
        size = PyLong_AsSsize_t(sizeobj);
    else {
        PyErr_SetString(PyExc_TypeError, "arg 1 must be an integer");
        return NULL;
    }

    if (size == -1 && PyErr_Occurred())
        return NULL;

    return mbstreamreader_iread(self, "readline", size);
}

/*[clinic input]
 _multibytecodec.MultibyteStreamReader.readlines
    cls: defining_class
    sizehintobj: object = None
    /
[clinic start generated code]*/

static PyObject *
_multibytecodec_MultibyteStreamReader_readlines_impl(MultibyteStreamReaderObject *self,
                                                     PyTypeObject *cls,
                                                     PyObject *sizehintobj)
/*[clinic end generated code: output=2847e9e17f01bae4 input=7107e994a64396e6]*/
{
    PyObject *r, *sr;
    Py_ssize_t sizehint;

    if (sizehintobj == Py_None)
        sizehint = -1;
    else if (PyLong_Check(sizehintobj))
        sizehint = PyLong_AsSsize_t(sizehintobj);
    else {
        PyErr_SetString(PyExc_TypeError, "arg 1 must be an integer");
        return NULL;
    }

    if (sizehint == -1 && PyErr_Occurred())
        return NULL;

    r = mbstreamreader_iread(self, "read", sizehint);
    if (r == NULL)
        return NULL;

    sr = PyUnicode_Splitlines(r, 1);
    Py_DECREF(r);
    return sr;
}

/*[clinic input]
 _multibytecodec.MultibyteStreamReader.reset
     cls: defining_class
     /
[clinic start generated code]*/

static PyObject *
_multibytecodec_MultibyteStreamReader_reset_impl(MultibyteStreamReaderObject *self,
                                                 PyTypeObject *cls)
/*[clinic end generated code: output=e5ef769fc434556a input=d5d4e4db37c4df14]*/
{
    if (self->codec->decreset != NULL &&
        self->codec->decreset(&self->state, self->codec->config) != 0)
        return NULL;
    self->pendingsize = 0;

    Py_RETURN_NONE;
}

static struct PyMethodDef mbstreamreader_methods[] = {
    _MULTIBYTECODEC_MULTIBYTESTREAMREADER_READ_METHODDEF
    _MULTIBYTECODEC_MULTIBYTESTREAMREADER_READLINE_METHODDEF
    _MULTIBYTECODEC_MULTIBYTESTREAMREADER_READLINES_METHODDEF
    _MULTIBYTECODEC_MULTIBYTESTREAMREADER_RESET_METHODDEF
    {NULL,              NULL},
};

static PyMemberDef mbstreamreader_members[] = {
    {"stream",          T_OBJECT,
                    offsetof(MultibyteStreamReaderObject, stream),
                    READONLY, NULL},
    {NULL,}
};

static PyObject *
mbstreamreader_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    MultibyteStreamReaderObject *self;
    PyObject *stream, *codec = NULL;
    char *errors = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|s:StreamReader",
                            streamkwarglist, &stream, &errors))
        return NULL;

    self = (MultibyteStreamReaderObject *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    codec = PyObject_GetAttrString((PyObject *)type, "codec");
    if (codec == NULL)
        goto errorexit;

    multibytecodec_state *state = PyType_GetModuleState(type);
    if (!Py_IS_TYPE(codec, (PyTypeObject*)state->MultibyteCodec_Type)) {
        PyErr_SetString(PyExc_TypeError, "codec is unexpected type");
        goto errorexit;
    }

    self->codec = ((MultibyteCodecObject *)codec)->codec;
    self->stream = stream;
    Py_INCREF(stream);
    self->pendingsize = 0;
    self->errors = internal_error_callback(errors);
    if (self->errors == NULL)
        goto errorexit;
    if (self->codec->decinit != NULL &&
        self->codec->decinit(&self->state, self->codec->config) != 0)
        goto errorexit;

    Py_DECREF(codec);
    return (PyObject *)self;

errorexit:
    Py_XDECREF(self);
    Py_XDECREF(codec);
    return NULL;
}

static int
mbstreamreader_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    return 0;
}

static int
mbstreamreader_traverse(MultibyteStreamReaderObject *self,
                        visitproc visit, void *arg)
{
    if (ERROR_ISCUSTOM(self->errors))
        Py_VISIT(self->errors);
    Py_VISIT(self->stream);
    return 0;
}

static void
mbstreamreader_dealloc(MultibyteStreamReaderObject *self)
{
    PyObject_GC_UnTrack(self);
    ERROR_DECREF(self->errors);
    Py_XDECREF(self->stream);
    PyTypeObject *tp = Py_TYPE(self);
    tp->tp_free(self);
    Py_DECREF(tp);
}

static PyType_Slot MultibyteStreamReader_Type_slots[] = {
    {Py_tp_init, mbstreamreader_init},
    {Py_tp_getset, codecctx_getsets},
    {Py_tp_new, mbstreamreader_new},
    {Py_tp_getattr, PyObject_GenericGetAttr},
    {Py_tp_methods, mbstreamreader_methods},
    {Py_tp_members, mbstreamreader_members},
    {Py_tp_traverse, mbstreamreader_traverse},
    {Py_tp_dealloc, mbstreamreader_dealloc},
    {0, NULL}
};

static PyType_Spec MultibyteStreamReader_Type_spec = {
    .name = "MultibyteStreamReader",
    .basicsize = sizeof(MultibyteStreamReaderObject),
    .itemsize = 0,
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE,
    .slots = MultibyteStreamReader_Type_slots
};

/*[clinic input]
 class _multibytecodec.MultibyteStreamWriter "MultibyteStreamWriterObject *" "multibytecodec_state_get_state_by_type(type)->MultibyteStreamWriter_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=55748e5d8b1fbf3b]*/

static int
mbstreamwriter_iwrite(MultibyteStreamWriterObject *self,
                      PyObject *unistr)
{
    PyObject *str, *wr;

    str = encoder_encode_stateful(STATEFUL_ECTX(self), unistr, 0);
    if (str == NULL)
        return -1;

    wr = _PyObject_CallMethodIdOneArg(self->stream, &PyId_write, str);
    Py_DECREF(str);
    if (wr == NULL)
        return -1;

    Py_DECREF(wr);
    return 0;
}

/*[clinic input]
 _multibytecodec.MultibyteStreamWriter.write
    cls: defining_class
    strobj: object
    /
[clinic start generated code]*/

static PyObject *
_multibytecodec_MultibyteStreamWriter_write_impl(MultibyteStreamWriterObject *self,
                                                 PyTypeObject *cls,
                                                 PyObject *strobj)
/*[clinic end generated code: output=68ade3aea26410ac input=d8a63a3755413d52]*/
{
    if (mbstreamwriter_iwrite(self, strobj))
        return NULL;
    else
        Py_RETURN_NONE;
}

/*[clinic input]
 _multibytecodec.MultibyteStreamWriter.writelines
    cls: defining_class
    lines: object
    /
[clinic start generated code]*/

static PyObject *
_multibytecodec_MultibyteStreamWriter_writelines_impl(MultibyteStreamWriterObject *self,
                                                      PyTypeObject *cls,
                                                      PyObject *lines)
/*[clinic end generated code: output=b4c99d2cf23ffb88 input=31027cf1a4e46c36]*/
{
    PyObject *strobj;
    int i, r;

    if (!PySequence_Check(lines)) {
        PyErr_SetString(PyExc_TypeError,
                        "arg must be a sequence object");
        return NULL;
    }

    for (i = 0; i < PySequence_Length(lines); i++) {
        /* length can be changed even within this loop */
        strobj = PySequence_GetItem(lines, i);
        if (strobj == NULL)
            return NULL;

        r = mbstreamwriter_iwrite(self, strobj);
        Py_DECREF(strobj);
        if (r == -1)
            return NULL;
    }
    /* PySequence_Length() can fail */
    if (PyErr_Occurred())
        return NULL;

    Py_RETURN_NONE;
}

/*[clinic input]
 _multibytecodec.MultibyteStreamWriter.reset
     cls: defining_class
     /
[clinic start generated code]*/

static PyObject *
_multibytecodec_MultibyteStreamWriter_reset_impl(MultibyteStreamWriterObject *self,
                                                 PyTypeObject *cls)
/*[clinic end generated code: output=32ef224c2a38aa3d input=f4edc3c0af4a9995]*/
{
    PyObject *pwrt;

    if (!self->pending)
        Py_RETURN_NONE;

    pwrt = multibytecodec_encode(self->codec, &self->state,
                    self->pending, NULL, self->errors,
                    MBENC_FLUSH | MBENC_RESET);
    /* some pending buffer can be truncated when UnicodeEncodeError is
     * raised on 'strict' mode. but, 'reset' method is designed to
     * reset the pending buffer or states so failed string sequence
     * ought to be missed */
    Py_CLEAR(self->pending);
    if (pwrt == NULL)
        return NULL;

    assert(PyBytes_Check(pwrt));
    if (PyBytes_Size(pwrt) > 0) {
        PyObject *wr;

        wr = _PyObject_CallMethodIdOneArg(self->stream, &PyId_write, pwrt);
        if (wr == NULL) {
            Py_DECREF(pwrt);
            return NULL;
        }
    }
    Py_DECREF(pwrt);

    Py_RETURN_NONE;
}

static PyObject *
mbstreamwriter_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    MultibyteStreamWriterObject *self;
    PyObject *stream, *codec = NULL;
    char *errors = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|s:StreamWriter",
                            streamkwarglist, &stream, &errors))
        return NULL;

    self = (MultibyteStreamWriterObject *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    codec = PyObject_GetAttrString((PyObject *)type, "codec");
    if (codec == NULL)
        goto errorexit;

    multibytecodec_state *state = PyType_GetModuleState(type);
    if (!Py_IS_TYPE(codec, (PyTypeObject*)state->MultibyteCodec_Type)) {
        PyErr_SetString(PyExc_TypeError, "codec is unexpected type");
        goto errorexit;
    }

    self->codec = ((MultibyteCodecObject *)codec)->codec;
    self->stream = stream;
    Py_INCREF(stream);
    self->pending = NULL;
    self->errors = internal_error_callback(errors);
    if (self->errors == NULL)
        goto errorexit;
    if (self->codec->encinit != NULL &&
        self->codec->encinit(&self->state, self->codec->config) != 0)
        goto errorexit;

    Py_DECREF(codec);
    return (PyObject *)self;

errorexit:
    Py_XDECREF(self);
    Py_XDECREF(codec);
    return NULL;
}

static int
mbstreamwriter_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    return 0;
}

static int
mbstreamwriter_traverse(MultibyteStreamWriterObject *self,
                        visitproc visit, void *arg)
{
    if (ERROR_ISCUSTOM(self->errors))
        Py_VISIT(self->errors);
    Py_VISIT(self->stream);
    return 0;
}

static void
mbstreamwriter_dealloc(MultibyteStreamWriterObject *self)
{
    PyObject_GC_UnTrack(self);
    ERROR_DECREF(self->errors);
    Py_XDECREF(self->stream);
    PyTypeObject *tp = Py_TYPE(self);
    tp->tp_free(self);
    Py_DECREF(tp);
}

static struct PyMethodDef mbstreamwriter_methods[] = {
    _MULTIBYTECODEC_MULTIBYTESTREAMWRITER_WRITE_METHODDEF
    _MULTIBYTECODEC_MULTIBYTESTREAMWRITER_WRITELINES_METHODDEF
    _MULTIBYTECODEC_MULTIBYTESTREAMWRITER_RESET_METHODDEF
    {NULL, NULL},
};

static PyMemberDef mbstreamwriter_members[] = {
    {"stream",          T_OBJECT,
                    offsetof(MultibyteStreamWriterObject, stream),
                    READONLY, NULL},
    {NULL,}
};

static PyType_Slot MultibyteStreamWriter_Type_slots[] = {
    {Py_tp_init, mbstreamwriter_init},
    {Py_tp_getset, codecctx_getsets},
    {Py_tp_new, mbstreamwriter_new},
    {Py_tp_getattr, PyObject_GenericGetAttr},
    {Py_tp_methods, mbstreamwriter_methods},
    {Py_tp_members, mbstreamwriter_members},
    {Py_tp_traverse, mbstreamwriter_traverse},
    {Py_tp_dealloc, mbstreamwriter_dealloc},
    {0, NULL}
};

static PyType_Spec MultibyteStreamWriter_Type_spec = {
    .name = "MultibyteStreamWriter",
    .basicsize = sizeof(MultibyteStreamWriterObject),
    .itemsize = 0,
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE,
    .slots = MultibyteStreamWriter_Type_slots
};

/*[clinic input]
_multibytecodec.__create_codec
    arg: object
    /
[clinic start generated code]*/

static PyObject *
_multibytecodec___create_codec(PyObject *module, PyObject *arg)
/*[clinic end generated code: output=cfa3dce8260e809d input=1c66af54e8332452]*/
{
    MultibyteCodecObject *self;
    MultibyteCodec *codec;

    if (!PyCapsule_IsValid(arg, PyMultibyteCodec_CAPSULE_NAME)) {
        PyErr_SetString(PyExc_ValueError, "argument type invalid");
        return NULL;
    }

    codec = PyCapsule_GetPointer(arg, PyMultibyteCodec_CAPSULE_NAME);
    if (codec->codecinit != NULL && codec->codecinit(codec->config) != 0)
        return NULL;

    multibytecodec_state *state =  multibytecodec_state_get_state(module);
    assert(state != NULL);
    self = PyObject_New(MultibyteCodecObject, (PyTypeObject *)state->MultibyteCodec_Type);
    if (self == NULL)
        return NULL;
    self->codec = codec;

    return (PyObject *)self;
}

static struct PyMethodDef multibytecodec_module_methods[] = {
    _MULTIBYTECODEC___CREATE_CODEC_METHODDEF
    {NULL, NULL},
};

static int
multibytecodec_exec(PyObject *module)
{
    multibytecodec_state *state = multibytecodec_state_get_state(module);

    state->MultibyteCodec_Type = PyType_FromModuleAndSpec(module, &MultibyteCodec_Type_spec, NULL);
    if (state->MultibyteCodec_Type == NULL) {
        return -1;
    }

    state->MultibyteIncrementalEncoder_Type = PyType_FromModuleAndSpec(module, &MultibyteIncrementalEncoder_Type_spec, NULL);
    if (state->MultibyteIncrementalEncoder_Type == NULL) {
        return -1;
    }

    if (PyModule_AddType(module, (PyTypeObject *)state->MultibyteIncrementalEncoder_Type) < 0) {
        return -1;
    }

    state->MultibyteIncrementalDecoder_Type = PyType_FromModuleAndSpec(module, &MultibyteIncrementalDecoder_Type_spec, NULL);
    if (state->MultibyteIncrementalDecoder_Type == NULL) {
        return -1;
    }

    if (PyModule_AddType(module, (PyTypeObject *)state->MultibyteIncrementalDecoder_Type) < 0) {
        return -1;
    }

    state->MultibyteStreamReader_Type = PyType_FromModuleAndSpec(module, &MultibyteStreamReader_Type_spec, NULL);
    if (state->MultibyteStreamReader_Type == NULL) {
        return -1;
    }

    if (PyModule_AddType(module, (PyTypeObject *)state->MultibyteStreamReader_Type) < 0) {
        return -1;
    }

    state->MultibyteStreamWriter_Type = PyType_FromModuleAndSpec(module, &MultibyteStreamWriter_Type_spec, NULL);
    if (state->MultibyteStreamWriter_Type == NULL) {
        return -1;
    }

    if (PyModule_AddType(module, (PyTypeObject *)state->MultibyteStreamWriter_Type) < 0) {
        return -1;
    }
    
    return 0;
}

static int
multibytecodec_traverse(PyObject *module, visitproc visit, void *arg)
{
    multibytecodec_state *state = multibytecodec_state_get_state(module);
    Py_VISIT(state->MultibyteCodec_Type);
    Py_VISIT(state->MultibyteIncrementalEncoder_Type);
    Py_VISIT(state->MultibyteIncrementalDecoder_Type);
    Py_VISIT(state->MultibyteStreamReader_Type);
    Py_VISIT(state->MultibyteStreamWriter_Type);
    return 0;
}

static int
multibytecodec_clear(PyObject *module)
{
    multibytecodec_state *state = multibytecodec_state_get_state(module);
    Py_CLEAR(state->MultibyteCodec_Type);
    Py_CLEAR(state->MultibyteIncrementalEncoder_Type);
    Py_CLEAR(state->MultibyteIncrementalDecoder_Type);
    Py_CLEAR(state->MultibyteStreamReader_Type);
    Py_CLEAR(state->MultibyteStreamWriter_Type);
    return 0;
}

static void
multibytecodec_free(void *module)
{
    multibytecodec_clear((PyObject *)module);
}

static PyModuleDef_Slot multibytecodec_slots[] = {
    {Py_mod_exec, multibytecodec_exec},
    {0, NULL}
};

static struct PyModuleDef _multibytecodecmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_multibytecodec",
    .m_size = sizeof(multibytecodec_state),
    .m_methods = multibytecodec_module_methods,
    .m_slots = multibytecodec_slots,
    .m_traverse = multibytecodec_traverse,
    .m_clear = multibytecodec_clear,
    .m_free = multibytecodec_free,
};

PyMODINIT_FUNC
PyInit__multibytecodec(void)
{
    return PyModuleDef_Init(&_multibytecodecmodule);
}
