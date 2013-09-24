/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/*
 * Copyright (c) 2008  litl, LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef __GJS_JSAPI_UTIL_H__
#define __GJS_JSAPI_UTIL_H__

#if !defined (__GJS_GJS_MODULE_H__) && !defined (GJS_COMPILATION)
#error "Only <gjs/gjs-module.h> can be included directly."
#endif

#include <gjs/compat.h>
#include <gjs/runtime.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define GJS_UTIL_ERROR gjs_util_error_quark ()
GQuark gjs_util_error_quark (void);
enum {
  GJS_UTIL_ERROR_NONE,
  GJS_UTIL_ERROR_ARGUMENT_INVALID,
  GJS_UTIL_ERROR_ARGUMENT_UNDERFLOW,
  GJS_UTIL_ERROR_ARGUMENT_OVERFLOW,
  GJS_UTIL_ERROR_ARGUMENT_TYPE_MISMATCH
};

typedef enum {
    GJS_GLOBAL_SLOT_IMPORTS,
    GJS_GLOBAL_SLOT_KEEP_ALIVE,
    GJS_GLOBAL_SLOT_LAST,
} GjsGlobalSlot;

typedef struct GjsRootedArray GjsRootedArray;

/* Flags that should be set on properties exported from native code modules.
 * Basically set these on API, but do NOT set them on data.
 *
 * READONLY:  forbid setting prop to another value
 * PERMANENT: forbid deleting the prop
 * ENUMERATE: allows copyProperties to work among other reasons to have it
 */
#define GJS_MODULE_PROP_FLAGS (JSPROP_PERMANENT | JSPROP_ENUMERATE)

/*
 * Helper methods to access private data:
 *
 * do_base_typecheck: checks that object has the right JSClass, and possibly
 *                    throw a TypeError exception if the check fails
 * priv_from_js: accesses the object private field; as a debug measure,
 *               it also checks that the object is of a compatible
 *               JSClass, but it doesn't raise an exception (it
 *               wouldn't be of much use, if subsequent code crashes on
 *               NULL)
 * priv_from_js_with_typecheck: a convenience function to call
 *                              do_base_typecheck and priv_from_js
 */
#define GJS_DEFINE_PRIV_FROM_JS(type, class)                            \
    __attribute__((unused)) static inline JSBool                        \
    do_base_typecheck(JSContext *context,                               \
                      JSObject  *object,                                \
                      JSBool     throw)                                 \
    {                                                                   \
        return gjs_typecheck_instance(context, object, &class, throw);  \
    }                                                                   \
    static inline type *                                                \
    priv_from_js(JSContext *context,                                    \
                 JSObject  *object)                                     \
    {                                                                   \
        type *priv;                                                     \
        JS_BeginRequest(context);                                       \
        priv = JS_GetInstancePrivate(context, object, &class, NULL);    \
        JS_EndRequest(context);                                         \
        return priv;                                                    \
    }                                                                   \
    __attribute__((unused)) static JSBool                               \
    priv_from_js_with_typecheck(JSContext *context,                     \
                                JSObject  *object,                      \
                                type      **out)                        \
    {                                                                   \
        if (!do_base_typecheck(context, object, JS_FALSE))              \
            return JS_FALSE;                                            \
        *out = priv_from_js(context, object);                           \
        return JS_TRUE;                                                 \
    }

/**
 * GJS_DEFINE_PROTO:
 * @tn: The name of the prototype, as a string
 * @cn: The name of the prototype, separated by _
 *
 * A convenience macro for prototype implementations.
 */
#define GJS_DEFINE_PROTO(tn, cn) \
GJS_NATIVE_CONSTRUCTOR_DECLARE(cn); \
_GJS_DEFINE_PROTO_FULL(tn, cn, gjs_##cn##_constructor)

/**
 * GJS_DEFINE_PROTO_ABSTRACT:
 * @tn: The name of the prototype, as a string
 * @cn: The name of the prototype, separated by _
 *
 * A convenience macro for prototype implementations.
 * Similar to GJS_DEFINE_PROTO but marks the prototype as abstract,
 * you won't be able to instantiate it using the new keyword
 */
#define GJS_DEFINE_PROTO_ABSTRACT(tn, cn) \
_GJS_DEFINE_PROTO_FULL(tn, cn, NULL)

#define _GJS_DEFINE_PROTO_FULL(type_name, cname, ctor) \
static JSPropertySpec gjs_##cname##_proto_props[]; \
static JSFunctionSpec gjs_##cname##_proto_funcs[]; \
static void gjs_##cname##_finalize(JSFreeOp *fop, JSObject *obj); \
static JSBool gjs_##cname##_new_resolve(JSContext *context, \
                                        JSObject  *obj, \
                                        jsval      id, \
                                        unsigned   flags, \
                                        JSObject **objp) \
{ \
    return JS_TRUE; \
} \
static struct JSClass gjs_##cname##_class = { \
    type_name, \
    JSCLASS_HAS_PRIVATE | \
    JSCLASS_NEW_RESOLVE, \
    JS_PropertyStub, \
    JS_PropertyStub, \
    JS_PropertyStub, \
    JS_StrictPropertyStub, \
    JS_EnumerateStub,\
    (JSResolveOp) gjs_##cname##_new_resolve, \
    JS_ConvertStub, \
    gjs_##cname##_finalize, \
    NULL, \
    NULL, \
    NULL, NULL, NULL \
}; \
jsval gjs_##cname##_create_proto(JSContext *context, JSObject *module, const char *proto_name, JSObject *parent) \
{ \
    jsval rval; \
    JSObject *global = gjs_get_import_global(context); \
    jsid class_name = gjs_intern_string_to_id(context, gjs_##cname##_class.name); \
    if (!JS_GetPropertyById(context, global, class_name, &rval))                       \
        return JSVAL_NULL; \
    if (JSVAL_IS_VOID(rval)) { \
        JSObject *prototype = JS_InitClass(context, global, \
                                 parent, \
                                 &gjs_##cname##_class, \
                                 ctor, \
                                 0, \
                                 &gjs_##cname##_proto_props[0], \
                                 &gjs_##cname##_proto_funcs[0], \
                                 NULL, \
                                 NULL); \
        if (prototype == NULL) { \
            return JSVAL_NULL; \
        } \
        if (!gjs_object_require_property( \
                context, global, NULL, \
                class_name, &rval)) { \
            return JSVAL_NULL; \
        } \
        if (!JS_DefineProperty(context, module, proto_name, \
                               rval, NULL, NULL, GJS_MODULE_PROP_FLAGS)) \
            return JSVAL_NULL; \
    } \
    return rval; \
}

gboolean    gjs_init_context_standard        (JSContext       *context);

JSObject*   gjs_get_import_global            (JSContext       *context);

jsval       gjs_get_global_slot              (JSContext       *context,
                                              GjsGlobalSlot    slot);
void        gjs_set_global_slot              (JSContext       *context,
                                              GjsGlobalSlot    slot,
                                              jsval            value);

gboolean    gjs_object_require_property      (JSContext       *context,
                                              JSObject        *obj,
                                              const char      *obj_description,
                                              jsid             property_name,
                                              jsval           *value_p);
/* This one is defined in runtime.c, so the compiler can optimize the call
   to get_const_string() it uses. */
gboolean    gjs_object_get_property_const    (JSContext       *context,
                                              JSObject        *obj,
                                              GjsConstString   property_name,
                                              jsval           *value_p);

JSObject   *gjs_new_object_for_constructor   (JSContext       *context,
                                              JSClass         *clasp,
                                              jsval           *vp);
JSBool      gjs_init_class_dynamic           (JSContext       *context,
                                              JSObject        *in_object,
                                              JSObject        *parent_proto,
                                              const char      *ns_name,
                                              const char      *class_name,
                                              JSClass         *clasp,
                                              JSNative         constructor,
                                              unsigned         nargs,
                                              JSPropertySpec  *ps,
                                              JSFunctionSpec  *fs,
                                              JSPropertySpec  *static_ps,
                                              JSFunctionSpec  *static_fs,
                                              JSObject       **constructor_p,
                                              JSObject       **prototype_p);
void gjs_throw_constructor_error             (JSContext       *context);
void gjs_throw_abstract_constructor_error    (JSContext       *context,
                                              jsval           *vp);

JSBool gjs_typecheck_instance                 (JSContext  *context,
                                               JSObject   *obj,
                                               JSClass    *static_clasp,
                                               JSBool      _throw);

JSObject*   gjs_construct_object_dynamic     (JSContext       *context,
                                              JSObject        *proto,
                                              unsigned         argc,
                                              jsval           *argv);
JSObject*   gjs_define_string_array          (JSContext       *context,
                                              JSObject        *obj,
                                              const char      *array_name,
                                              gssize           array_length,
                                              const char     **array_values,
                                              unsigned         attrs);
void        gjs_throw                        (JSContext       *context,
                                              const char      *format,
                                              ...)  G_GNUC_PRINTF (2, 3);
void        gjs_throw_custom                 (JSContext       *context,
                                              const char      *error_class,
                                              const char      *format,
                                              ...)  G_GNUC_PRINTF (3, 4);
void        gjs_throw_literal                (JSContext       *context,
                                              const char      *string);
void        gjs_throw_g_error                (JSContext       *context,
                                              GError          *error);

JSBool      gjs_log_exception                (JSContext       *context);
JSBool      gjs_log_and_keep_exception       (JSContext       *context);
JSBool      gjs_move_exception               (JSContext       *src_context,
                                              JSContext       *dest_context);
JSBool      gjs_log_exception_full           (JSContext       *context,
                                              jsval            exc,
                                              JSString        *message);

#ifdef __GJS_UTIL_LOG_H__
void        gjs_log_object_props             (JSContext       *context,
                                              JSObject        *obj,
                                              GjsDebugTopic    topic,
                                              const char      *prefix);
#endif
char*       gjs_value_debug_string           (JSContext       *context,
                                              jsval            value);
void        gjs_explain_scope                (JSContext       *context,
                                              const char      *title);
JSBool      gjs_call_function_value          (JSContext       *context,
                                              JSObject        *obj,
                                              jsval            fval,
                                              unsigned         argc,
                                              jsval           *argv,
                                              jsval           *rval);
void        gjs_error_reporter               (JSContext       *context,
                                              const char      *message,
                                              JSErrorReport   *report);
JSBool      gjs_get_prop_verbose_stub        (JSContext       *context,
                                              JSObject        *obj,
                                              jsval            id,
                                              jsval           *value_p);
JSBool      gjs_set_prop_verbose_stub        (JSContext       *context,
                                              JSObject        *obj,
                                              jsval            id,
                                              jsval           *value_p);
JSBool      gjs_add_prop_verbose_stub        (JSContext       *context,
                                              JSObject        *obj,
                                              jsval            id,
                                              jsval           *value_p);
JSBool      gjs_delete_prop_verbose_stub     (JSContext       *context,
                                              JSObject        *obj,
                                              jsval            id,
                                              jsval           *value_p);

JSBool      gjs_string_to_utf8               (JSContext       *context,
                                              const            jsval string_val,
                                              char           **utf8_string_p);
JSBool      gjs_string_from_utf8             (JSContext       *context,
                                              const char      *utf8_string,
                                              gssize           n_bytes,
                                              jsval           *value_p);
JSBool      gjs_string_to_filename           (JSContext       *context,
                                              const jsval      string_val,
                                              char           **filename_string_p);
JSBool      gjs_string_from_filename         (JSContext       *context,
                                              const char      *filename_string,
                                              gssize           n_bytes,
                                              jsval           *value_p);
JSBool      gjs_string_get_uint16_data       (JSContext       *context,
                                              jsval            value,
                                              guint16        **data_p,
                                              gsize           *len_p);
JSBool      gjs_get_string_id                (JSContext       *context,
                                              jsid             id,
                                              char           **name_p);
jsid        gjs_intern_string_to_id          (JSContext       *context,
                                              const char      *string);

gboolean    gjs_unichar_from_string          (JSContext       *context,
                                              jsval            string,
                                              gunichar        *result);

const char* gjs_get_type_name                (jsval            value);

JSBool      gjs_value_to_int64               (JSContext       *context,
                                              const jsval      val,
                                              gint64          *result);

jsval       gjs_date_from_time_t             (JSContext *context, time_t time);

JSBool      gjs_parse_args                   (JSContext  *context,
                                              const char *function_name,
                                              const char *format,
                                              unsigned   argc,
                                              jsval     *argv,
                                              ...);

GjsRootedArray*   gjs_rooted_array_new        (void);
void              gjs_rooted_array_append     (JSContext        *context,
                                               GjsRootedArray *array,
                                               jsval             value);
jsval             gjs_rooted_array_get        (JSContext        *context,
                                               GjsRootedArray *array,
                                               int               i);
jsval*            gjs_rooted_array_get_data   (JSContext        *context,
                                               GjsRootedArray *array);
int               gjs_rooted_array_get_length (JSContext        *context,
                                               GjsRootedArray *array);
jsval*            gjs_rooted_array_free       (JSContext        *context,
                                               GjsRootedArray *array,
                                               gboolean          free_segment);
void              gjs_set_values              (JSContext        *context,
                                               jsval            *locations,
                                               int               n_locations,
                                               jsval             initializer);
void              gjs_root_value_locations    (JSContext        *context,
                                               jsval            *locations,
                                               int               n_locations);
void              gjs_unroot_value_locations  (JSContext        *context,
                                               jsval            *locations,
                                               int               n_locations);

gboolean          gjs_is_array_buffer_object  (JSContext        *context,
                                               JSObject         *object);
gpointer          gjs_array_buffer_get_data   (JSContext        *context,
                                               JSObject         *object);
gsize             gjs_array_buffer_get_length (JSContext        *context,
                                               JSObject         *object);

gboolean          gjs_is_typed_array_object   (JSContext        *context,
                                               JSObject         *object);

gpointer          gjs_typed_array_get_int8_data   (JSContext    *context,
                                                   JSObject     *object);
gpointer          gjs_typed_array_get_uint8_data  (JSContext    *context,
                                                   JSObject     *object);
gpointer          gjs_typed_array_get_int16_data  (JSContext    *context,
                                                   JSObject     *object);
gpointer          gjs_typed_array_get_uint16_data (JSContext    *context,
                                                   JSObject     *object);
gpointer          gjs_typed_array_get_int32_data  (JSContext    *context,
                                                   JSObject     *object);
gpointer          gjs_typed_array_get_uint32_data (JSContext    *context,
                                                   JSObject     *object);
gpointer          gjs_typed_array_get_float_data  (JSContext    *context,
                                                   JSObject     *object);
gpointer          gjs_typed_array_get_double_data (JSContext    *context,
                                                   JSObject     *object);
gsize             gjs_typed_array_get_length      (JSContext    *context,
                                                   JSObject     *object);
gboolean          gjs_typed_array_is_compatible   (JSContext    *context,
                                                   JSObject     *object,
                                                   guint         size,
                                                   gboolean      is_signed,
                                                   gboolean      floating);

/* Functions intended for more "internal" use */

void gjs_maybe_gc (JSContext *context);
void gjs_enter_gc (void);
void gjs_leave_gc (void);
gboolean gjs_try_block_gc (void);
void gjs_block_gc (void);
void gjs_unblock_gc (void);

JSBool            gjs_context_get_frame_info (JSContext  *context,
                                              jsval      *stack,
                                              jsval      *fileName,
                                              jsval      *lineNumber);

G_END_DECLS

#endif  /* __GJS_JSAPI_UTIL_H__ */
