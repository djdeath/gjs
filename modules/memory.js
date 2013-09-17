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

const GLib = imports.gi.GLib;
const Gio = imports.gi.Gio;
const GjsPrivate = imports.gi.GjsPrivate;
const Lang = imports.lang;

const MemoryIface = <interface name="org.gjs.Memory">
<method name="GetObjects">
    <arg type="s" direction="in" name="class_name"/>
    <arg type="a(ss)" direction="out" name="objects"/>
</method>
<method name="GetObjectsCounts">
    <arg type="a(si)" direction="out" name="objects_counts"/>
</method>
</interface>;

const MemoryService = new Lang.Class({
    Name: 'MemoryService',

    _init: function() {
        this._dbusImpl = Gio.DBusExportedObject.wrapJSObject(MemoryIface, this);
        this._dbusImpl.export(Gio.DBus.session, '/org/gjs/Memory');

        this._memory = new GjsPrivate.Memory();
    },

    GetObjects: function(typeName) {
        let objs = this._memory.get_objects(typeName);

        let ret = [];
        for (let i in objs)
            ret.push([objs[i].type_name,
                      objs[i].location]);

        return ret;
    },

    GetObjectsCounts: function() {
        let counts = this._memory.get_objects_counts();

        let ret = [];
        for (let i in counts)
            ret.push([counts[i].type_name,
                      counts[i].count]);

        return ret;
    },

    shutdown: function() {
        this._dbusImpl.unexport('/org/gjs/Memory');
    },
});

let _memoryDaemon = null;

function startMemoryService() {
    if (_memoryDaemon != null)
        return;

    _memoryDaemon = new MemoryService();
}

function stopMemoryService() {
    if (_memoryDaemon == null)
        return;

    _memoryDaemon.shutdown();
    _memoryDaemon = null;
}
