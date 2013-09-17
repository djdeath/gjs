/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* Copyright 2013 Intel Corporation
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

#ifndef __GJS_MEMORY_H__
#define __GJS_MEMORY_H__

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

typedef struct _GjsMemory        GjsMemory;
typedef struct _GjsMemoryClass   GjsMemoryClass;
typedef struct _GjsMemoryPrivate GjsMemoryPrivate;

#define GJS_TYPE_MEMORY              (gjs_memory_get_type ())
#define GJS_MEMORY(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GJS_TYPE_MEMORY, GjsMemory))
#define GJS_MEMORY_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GJS_TYPE_MEMORY, GjsMemoryClass))
#define GJS_IS_MEMORY(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GJS_TYPE_MEMORY))
#define GJS_IS_MEMORY_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GJS_TYPE_MEMORY))
#define GJS_MEMORY_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GJS_TYPE_MEMORY, GjsMemoryClass))

struct _GjsMemory {
    /*< private >*/
    GObject parent;

    GjsMemoryPrivate *priv;
};

struct _GjsMemoryClass {
    /*< private >*/
    GObjectClass parent_class;
};


#define GJS_TYPE_MEMORY_OBJECT_COUNT      (gjs_memory_object_count_get_type ())
#define GJS_TYPE_MEMORY_OBJECT            (gjs_memory_object_get_type ())

typedef struct _GjsMemoryObjectCount GjsMemoryObjectCount;
typedef struct _GjsMemoryObject      GjsMemoryObject;

/**
 * GjsMemoryObjectCount:
 * @type_name: name of the type
 * @gtype: associated gtype
 * @count: count of objects
 *
 * Object count representation.
 */
struct _GjsMemoryObjectCount
{
    /*< public >*/
    const gchar *type_name;
    GType        gtype;
    guint        count;
};

/**
 * GjsMemoryObject:
 * @type_name: name of the type
 * @gtype: associated gtype of the object
 * @location: location at which this object has been created
 * @proxy_address: the address of the Javascript object
 *
 * Object representation.
 */
struct _GjsMemoryObject
{
    /*< public >*/
    const gchar *type_name;
    GType        gtype;
    const gchar *location;
    gulong       proxy_address;
};

GType                  gjs_memory_object_count_get_type (void);
GType                  gjs_memory_object_get_type       (void);
GType                  gjs_memory_get_type              (void);


GjsMemoryObjectCount  *gjs_memory_object_count_new      (const gchar *name, GType gtype, guint count);
GjsMemoryObject       *gjs_memory_object_new            (const gchar *name, GType gtype, const gchar *location, gulong proxy_address);

GPtrArray             *gjs_memory_get_objects_counts    (GjsMemory *self);
GPtrArray             *gjs_memory_get_objects           (GjsMemory *self, const gchar *type_name);

G_END_DECLS

#endif  /* __GJS_MEMORY_H__ */
