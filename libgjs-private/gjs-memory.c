/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/*
 * Copyright (c) 2013  Intel Corporation
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

#include <config.h>
#include <string.h>

#include "gjs-memory.h"
#include "gi/object.h"

struct _GjsMemoryPrivate {
    guint dummy;
};

G_DEFINE_TYPE(GjsMemory, gjs_memory, G_TYPE_OBJECT)

void
gjs_memory_class_init (GjsMemoryClass *klass)
{
    g_type_class_add_private (klass, sizeof (GjsMemoryPrivate));
}

static void
gjs_memory_init (GjsMemory *self)
{
}

/**
 * gjs_memory_get_objects_counts:
 * @self: a #GjsDBusImplementation
 *
 * Return value: (transfer container) (element-type GjsPrivate.MemoryObjectCount):
 */
GPtrArray *
gjs_memory_get_objects_counts (GjsMemory *self)
{
    g_return_val_if_fail (GJS_IS_MEMORY (self), NULL);

    return gjs_object_get_instances_count ();
}

/**
 * gjs_memory_get_objects:
 * @self: a #GjsDBusImplementation
 * @type_name: a type name
 *
 * Return value: (transfer container) (element-type GjsPrivate.MemoryObject):
 */
GPtrArray *
gjs_memory_get_objects (GjsMemory *self,
                        const gchar *type_name)
{
    g_return_val_if_fail (GJS_IS_MEMORY (self), NULL);
    g_return_val_if_fail (type_name != NULL, NULL);

    return gjs_object_get_instances (type_name);
}


static gpointer
gjs_memory_object_count_copy (gpointer data)
{
    if (G_LIKELY (data != NULL))
        return g_slice_dup (GjsMemoryObjectCount, data);

    return NULL;
}

static void
gjs_memory_object_count_free (gpointer data)
{
    if (G_LIKELY (data != NULL))
        g_slice_free (GjsMemoryObjectCount, data);
}

static gpointer
gjs_memory_object_copy (gpointer data)
{
    if (G_LIKELY (data != NULL))
        return g_slice_dup (GjsMemoryObject, data);

    return NULL;
}

static void
gjs_memory_object_free (gpointer data)
{
    if (G_LIKELY (data != NULL))
        g_slice_free (GjsMemoryObject, data);
}

G_DEFINE_BOXED_TYPE(GjsMemoryObjectCount,
                    gjs_memory_object_count,
                    gjs_memory_object_count_copy,
                    gjs_memory_object_count_free)
G_DEFINE_BOXED_TYPE(GjsMemoryObject,
                    gjs_memory_object,
                    gjs_memory_object_copy,
                    gjs_memory_object_free)

/**
 * gjs_memory_object_count_new: (skip)
 */
GjsMemoryObjectCount *
gjs_memory_object_count_new (const gchar *type_name,
                             GType        gtype,
                             guint        count)
{
    GjsMemoryObjectCount *obj = g_slice_new0 (GjsMemoryObjectCount);

    obj->type_name = type_name;
    obj->gtype = gtype;
    obj->count = count;

    return obj;
}

/**
 * gjs_memory_object_new: (skip)
 */
GjsMemoryObject *
gjs_memory_object_new (const gchar *type_name,
                       GType        gtype,
                       const gchar *location,
                       gulong       proxy_address)
{
    GjsMemoryObject *obj = g_slice_new0 (GjsMemoryObject);

    obj->type_name = type_name;
    obj->gtype = gtype;
    obj->location = location;
    obj->proxy_address = proxy_address;

    return obj;
}
