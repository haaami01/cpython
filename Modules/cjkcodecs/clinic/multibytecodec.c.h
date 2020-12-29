/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_multibytecodec_MultibyteCodec_encode__doc__,
"encode($self, /, input, errors=None)\n"
"--\n"
"\n"
"Return an encoded string version of `input\'.\n"
"\n"
"\'errors\' may be given to set a different error handling scheme. Default is\n"
"\'strict\' meaning that encoding errors raise a UnicodeEncodeError. Other possible\n"
"values are \'ignore\', \'replace\' and \'xmlcharrefreplace\' as well as any other name\n"
"registered with codecs.register_error that can handle UnicodeEncodeErrors.");

#define _MULTIBYTECODEC_MULTIBYTECODEC_ENCODE_METHODDEF    \
    {"encode", (PyCFunction)(void(*)(void))_multibytecodec_MultibyteCodec_encode, METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _multibytecodec_MultibyteCodec_encode__doc__},

static PyObject *
_multibytecodec_MultibyteCodec_encode_impl(MultibyteCodecObject *self,
                                           PyTypeObject *cls,
                                           PyObject *input,
                                           const char *errors);

static PyObject *
_multibytecodec_MultibyteCodec_encode(MultibyteCodecObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"input", "errors", NULL};
    static _PyArg_Parser _parser = {"O|z:encode", _keywords, 0};
    PyObject *input;
    const char *errors = NULL;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &input, &errors)) {
        goto exit;
    }
    return_value = _multibytecodec_MultibyteCodec_encode_impl(self, cls, input, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteCodec_decode__doc__,
"decode($self, /, input, errors=None)\n"
"--\n"
"\n"
"Decodes \'input\'.\n"
"\n"
"\'errors\' may be given to set a different error handling scheme. Default is\n"
"\'strict\' meaning that encoding errors raise a UnicodeDecodeError. Other possible\n"
"values are \'ignore\' and \'replace\' as well as any other name registered with\n"
"codecs.register_error that is able to handle UnicodeDecodeErrors.\"");

#define _MULTIBYTECODEC_MULTIBYTECODEC_DECODE_METHODDEF    \
    {"decode", (PyCFunction)(void(*)(void))_multibytecodec_MultibyteCodec_decode, METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _multibytecodec_MultibyteCodec_decode__doc__},

static PyObject *
_multibytecodec_MultibyteCodec_decode_impl(MultibyteCodecObject *self,
                                           PyTypeObject *cls,
                                           Py_buffer *input,
                                           const char *errors);

static PyObject *
_multibytecodec_MultibyteCodec_decode(MultibyteCodecObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"input", "errors", NULL};
    static _PyArg_Parser _parser = {"y*|z:decode", _keywords, 0};
    Py_buffer input = {NULL, NULL};
    const char *errors = NULL;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &input, &errors)) {
        goto exit;
    }
    return_value = _multibytecodec_MultibyteCodec_decode_impl(self, cls, &input, errors);

exit:
    /* Cleanup for input */
    if (input.obj) {
       PyBuffer_Release(&input);
    }

    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteIncrementalEncoder_encode__doc__,
"encode($self, /, input, final=False)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTEINCREMENTALENCODER_ENCODE_METHODDEF    \
    {"encode", (PyCFunction)(void(*)(void))_multibytecodec_MultibyteIncrementalEncoder_encode, METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _multibytecodec_MultibyteIncrementalEncoder_encode__doc__},

static PyObject *
_multibytecodec_MultibyteIncrementalEncoder_encode_impl(MultibyteIncrementalEncoderObject *self,
                                                        PyTypeObject *cls,
                                                        PyObject *input,
                                                        int final);

static PyObject *
_multibytecodec_MultibyteIncrementalEncoder_encode(MultibyteIncrementalEncoderObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"input", "final", NULL};
    static _PyArg_Parser _parser = {"O|i:encode", _keywords, 0};
    PyObject *input;
    int final = 0;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &input, &final)) {
        goto exit;
    }
    return_value = _multibytecodec_MultibyteIncrementalEncoder_encode_impl(self, cls, input, final);

exit:
    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteIncrementalEncoder_getstate__doc__,
"getstate($self, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTEINCREMENTALENCODER_GETSTATE_METHODDEF    \
    {"getstate", (PyCFunction)_multibytecodec_MultibyteIncrementalEncoder_getstate, METH_NOARGS, _multibytecodec_MultibyteIncrementalEncoder_getstate__doc__},

static PyObject *
_multibytecodec_MultibyteIncrementalEncoder_getstate_impl(MultibyteIncrementalEncoderObject *self);

static PyObject *
_multibytecodec_MultibyteIncrementalEncoder_getstate(MultibyteIncrementalEncoderObject *self, PyObject *Py_UNUSED(ignored))
{
    return _multibytecodec_MultibyteIncrementalEncoder_getstate_impl(self);
}

PyDoc_STRVAR(_multibytecodec_MultibyteIncrementalEncoder_setstate__doc__,
"setstate($self, state, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTEINCREMENTALENCODER_SETSTATE_METHODDEF    \
    {"setstate", (PyCFunction)(void(*)(void))_multibytecodec_MultibyteIncrementalEncoder_setstate, METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _multibytecodec_MultibyteIncrementalEncoder_setstate__doc__},

static PyObject *
_multibytecodec_MultibyteIncrementalEncoder_setstate_impl(MultibyteIncrementalEncoderObject *self,
                                                          PyTypeObject *cls,
                                                          PyLongObject *statelong);

static PyObject *
_multibytecodec_MultibyteIncrementalEncoder_setstate(MultibyteIncrementalEncoderObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {"O!:setstate", _keywords, 0};
    PyLongObject *statelong;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &PyLong_Type, &statelong)) {
        goto exit;
    }
    return_value = _multibytecodec_MultibyteIncrementalEncoder_setstate_impl(self, cls, statelong);

exit:
    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteIncrementalEncoder_reset__doc__,
"reset($self, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTEINCREMENTALENCODER_RESET_METHODDEF    \
    {"reset", (PyCFunction)(void(*)(void))_multibytecodec_MultibyteIncrementalEncoder_reset, METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _multibytecodec_MultibyteIncrementalEncoder_reset__doc__},

static PyObject *
_multibytecodec_MultibyteIncrementalEncoder_reset_impl(MultibyteIncrementalEncoderObject *self,
                                                       PyTypeObject *cls);

static PyObject *
_multibytecodec_MultibyteIncrementalEncoder_reset(MultibyteIncrementalEncoderObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = { NULL};
    static _PyArg_Parser _parser = {":reset", _keywords, 0};

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser
        )) {
        goto exit;
    }
    return_value = _multibytecodec_MultibyteIncrementalEncoder_reset_impl(self, cls);

exit:
    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteIncrementalDecoder_decode__doc__,
"decode($self, /, input, final=False)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTEINCREMENTALDECODER_DECODE_METHODDEF    \
    {"decode", (PyCFunction)(void(*)(void))_multibytecodec_MultibyteIncrementalDecoder_decode, METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _multibytecodec_MultibyteIncrementalDecoder_decode__doc__},

static PyObject *
_multibytecodec_MultibyteIncrementalDecoder_decode_impl(MultibyteIncrementalDecoderObject *self,
                                                        PyTypeObject *cls,
                                                        Py_buffer *input,
                                                        int final);

static PyObject *
_multibytecodec_MultibyteIncrementalDecoder_decode(MultibyteIncrementalDecoderObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"input", "final", NULL};
    static _PyArg_Parser _parser = {"y*|i:decode", _keywords, 0};
    Py_buffer input = {NULL, NULL};
    int final = 0;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &input, &final)) {
        goto exit;
    }
    return_value = _multibytecodec_MultibyteIncrementalDecoder_decode_impl(self, cls, &input, final);

exit:
    /* Cleanup for input */
    if (input.obj) {
       PyBuffer_Release(&input);
    }

    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteIncrementalDecoder_getstate__doc__,
"getstate($self, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTEINCREMENTALDECODER_GETSTATE_METHODDEF    \
    {"getstate", (PyCFunction)(void(*)(void))_multibytecodec_MultibyteIncrementalDecoder_getstate, METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _multibytecodec_MultibyteIncrementalDecoder_getstate__doc__},

static PyObject *
_multibytecodec_MultibyteIncrementalDecoder_getstate_impl(MultibyteIncrementalDecoderObject *self,
                                                          PyTypeObject *cls);

static PyObject *
_multibytecodec_MultibyteIncrementalDecoder_getstate(MultibyteIncrementalDecoderObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = { NULL};
    static _PyArg_Parser _parser = {":getstate", _keywords, 0};

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser
        )) {
        goto exit;
    }
    return_value = _multibytecodec_MultibyteIncrementalDecoder_getstate_impl(self, cls);

exit:
    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteIncrementalDecoder_setstate__doc__,
"setstate($self, state, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTEINCREMENTALDECODER_SETSTATE_METHODDEF    \
    {"setstate", (PyCFunction)(void(*)(void))_multibytecodec_MultibyteIncrementalDecoder_setstate, METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _multibytecodec_MultibyteIncrementalDecoder_setstate__doc__},

static PyObject *
_multibytecodec_MultibyteIncrementalDecoder_setstate_impl(MultibyteIncrementalDecoderObject *self,
                                                          PyTypeObject *cls,
                                                          PyObject *state);

static PyObject *
_multibytecodec_MultibyteIncrementalDecoder_setstate(MultibyteIncrementalDecoderObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {"O!:setstate", _keywords, 0};
    PyObject *state;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &PyTuple_Type, &state)) {
        goto exit;
    }
    return_value = _multibytecodec_MultibyteIncrementalDecoder_setstate_impl(self, cls, state);

exit:
    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteIncrementalDecoder_reset__doc__,
"reset($self, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTEINCREMENTALDECODER_RESET_METHODDEF    \
    {"reset", (PyCFunction)(void(*)(void))_multibytecodec_MultibyteIncrementalDecoder_reset, METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _multibytecodec_MultibyteIncrementalDecoder_reset__doc__},

static PyObject *
_multibytecodec_MultibyteIncrementalDecoder_reset_impl(MultibyteIncrementalDecoderObject *self,
                                                       PyTypeObject *cls);

static PyObject *
_multibytecodec_MultibyteIncrementalDecoder_reset(MultibyteIncrementalDecoderObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = { NULL};
    static _PyArg_Parser _parser = {":reset", _keywords, 0};

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser
        )) {
        goto exit;
    }
    return_value = _multibytecodec_MultibyteIncrementalDecoder_reset_impl(self, cls);

exit:
    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteStreamReader_read__doc__,
"read($self, sizeobj=None, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTESTREAMREADER_READ_METHODDEF    \
    {"read", (PyCFunction)(void(*)(void))_multibytecodec_MultibyteStreamReader_read, METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _multibytecodec_MultibyteStreamReader_read__doc__},

static PyObject *
_multibytecodec_MultibyteStreamReader_read_impl(MultibyteStreamReaderObject *self,
                                                PyTypeObject *cls,
                                                PyObject *sizeobj);

static PyObject *
_multibytecodec_MultibyteStreamReader_read(MultibyteStreamReaderObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {"|O:read", _keywords, 0};
    PyObject *sizeobj = Py_None;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &sizeobj)) {
        goto exit;
    }
    return_value = _multibytecodec_MultibyteStreamReader_read_impl(self, cls, sizeobj);

exit:
    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteStreamReader_readline__doc__,
"readline($self, sizeobj=None, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTESTREAMREADER_READLINE_METHODDEF    \
    {"readline", (PyCFunction)(void(*)(void))_multibytecodec_MultibyteStreamReader_readline, METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _multibytecodec_MultibyteStreamReader_readline__doc__},

static PyObject *
_multibytecodec_MultibyteStreamReader_readline_impl(MultibyteStreamReaderObject *self,
                                                    PyTypeObject *cls,
                                                    PyObject *sizeobj);

static PyObject *
_multibytecodec_MultibyteStreamReader_readline(MultibyteStreamReaderObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {"|O:readline", _keywords, 0};
    PyObject *sizeobj = Py_None;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &sizeobj)) {
        goto exit;
    }
    return_value = _multibytecodec_MultibyteStreamReader_readline_impl(self, cls, sizeobj);

exit:
    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteStreamReader_readlines__doc__,
"readlines($self, sizehintobj=None, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTESTREAMREADER_READLINES_METHODDEF    \
    {"readlines", (PyCFunction)(void(*)(void))_multibytecodec_MultibyteStreamReader_readlines, METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _multibytecodec_MultibyteStreamReader_readlines__doc__},

static PyObject *
_multibytecodec_MultibyteStreamReader_readlines_impl(MultibyteStreamReaderObject *self,
                                                     PyTypeObject *cls,
                                                     PyObject *sizehintobj);

static PyObject *
_multibytecodec_MultibyteStreamReader_readlines(MultibyteStreamReaderObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {"|O:readlines", _keywords, 0};
    PyObject *sizehintobj = Py_None;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &sizehintobj)) {
        goto exit;
    }
    return_value = _multibytecodec_MultibyteStreamReader_readlines_impl(self, cls, sizehintobj);

exit:
    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteStreamReader_reset__doc__,
"reset($self, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTESTREAMREADER_RESET_METHODDEF    \
    {"reset", (PyCFunction)(void(*)(void))_multibytecodec_MultibyteStreamReader_reset, METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _multibytecodec_MultibyteStreamReader_reset__doc__},

static PyObject *
_multibytecodec_MultibyteStreamReader_reset_impl(MultibyteStreamReaderObject *self,
                                                 PyTypeObject *cls);

static PyObject *
_multibytecodec_MultibyteStreamReader_reset(MultibyteStreamReaderObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = { NULL};
    static _PyArg_Parser _parser = {":reset", _keywords, 0};

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser
        )) {
        goto exit;
    }
    return_value = _multibytecodec_MultibyteStreamReader_reset_impl(self, cls);

exit:
    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteStreamWriter_write__doc__,
"write($self, strobj, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTESTREAMWRITER_WRITE_METHODDEF    \
    {"write", (PyCFunction)(void(*)(void))_multibytecodec_MultibyteStreamWriter_write, METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _multibytecodec_MultibyteStreamWriter_write__doc__},

static PyObject *
_multibytecodec_MultibyteStreamWriter_write_impl(MultibyteStreamWriterObject *self,
                                                 PyTypeObject *cls,
                                                 PyObject *strobj);

static PyObject *
_multibytecodec_MultibyteStreamWriter_write(MultibyteStreamWriterObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {"O:write", _keywords, 0};
    PyObject *strobj;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &strobj)) {
        goto exit;
    }
    return_value = _multibytecodec_MultibyteStreamWriter_write_impl(self, cls, strobj);

exit:
    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteStreamWriter_writelines__doc__,
"writelines($self, lines, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTESTREAMWRITER_WRITELINES_METHODDEF    \
    {"writelines", (PyCFunction)(void(*)(void))_multibytecodec_MultibyteStreamWriter_writelines, METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _multibytecodec_MultibyteStreamWriter_writelines__doc__},

static PyObject *
_multibytecodec_MultibyteStreamWriter_writelines_impl(MultibyteStreamWriterObject *self,
                                                      PyTypeObject *cls,
                                                      PyObject *lines);

static PyObject *
_multibytecodec_MultibyteStreamWriter_writelines(MultibyteStreamWriterObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {"O:writelines", _keywords, 0};
    PyObject *lines;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &lines)) {
        goto exit;
    }
    return_value = _multibytecodec_MultibyteStreamWriter_writelines_impl(self, cls, lines);

exit:
    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteStreamWriter_reset__doc__,
"reset($self, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTESTREAMWRITER_RESET_METHODDEF    \
    {"reset", (PyCFunction)(void(*)(void))_multibytecodec_MultibyteStreamWriter_reset, METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _multibytecodec_MultibyteStreamWriter_reset__doc__},

static PyObject *
_multibytecodec_MultibyteStreamWriter_reset_impl(MultibyteStreamWriterObject *self,
                                                 PyTypeObject *cls);

static PyObject *
_multibytecodec_MultibyteStreamWriter_reset(MultibyteStreamWriterObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = { NULL};
    static _PyArg_Parser _parser = {":reset", _keywords, 0};

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser
        )) {
        goto exit;
    }
    return_value = _multibytecodec_MultibyteStreamWriter_reset_impl(self, cls);

exit:
    return return_value;
}

PyDoc_STRVAR(_multibytecodec___create_codec__doc__,
"__create_codec($module, arg, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC___CREATE_CODEC_METHODDEF    \
    {"__create_codec", (PyCFunction)_multibytecodec___create_codec, METH_O, _multibytecodec___create_codec__doc__},
/*[clinic end generated code: output=550b21edc5df624c input=a9049054013a1b77]*/
