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
const System = imports.system;

const MemoryIface = <interface name="org.gjs.Memory">
<method name="RunGC">
</method>
<method name="GetObjects">
    <arg type="s" direction="in" name="class_name"/>
    <arg type="a(sst)" direction="out" name="objects"/>
</method>
<method name="GetObjectsCounts">
    <arg type="a(si)" direction="out" name="objects_counts"/>
</method>
<method name="FindObjectPath">
    <arg type="t" direction="in" name="address"/>
    <arg type="s" direction="out" name="path"/>
</method>
</interface>;

const MemoryService = new Lang.Class({
    Name: 'MemoryService',

    _init: function(scope) {
        this._scope = scope;

        this._dbusImpl = Gio.DBusExportedObject.wrapJSObject(MemoryIface, this);
        this._dbusImpl.export(Gio.DBus.session, '/org/gjs/Memory');

        this._memory = new GjsPrivate.Memory();
    },

    GetObjects: function(typeName) {
        let objs = this._memory.get_objects(typeName);

        let ret = [];
        for (let i in objs)
            ret.push([objs[i].type_name,
                      objs[i].location,
                      objs[i].proxy_address]);

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

    _storeObject: function(obj) {
        obj.__id = this._objId++;
        this._objs[obj.__id] = obj;
    },

    _findObjectPath: function(name, obj) {
        // Sanity tests
        if (obj == undefined ||
            obj == null ||
            obj == this)
            return;

        // Prevent recursion
        if (obj.__id) {
            //log('returning on : ' + obj);
            return;
        }

        this._storeObject(obj);

        // Do matching
        if (!(obj instanceof Array) &&
            ('' + obj).match(this._addressRegexp)) {
            log('found! : ' + name);
            return name;
        }

        let getOffset = function(nb) {
            let s = '';
            for (let i = 0; i < nb; i++)
                s += ' ';
            return s;
        }

        //log(getOffset(this._level) + 'inspecting ' + name + ' - ' + typeof(obj));

        this._level++;
        let ret = null;
        for (let i in obj) {
            // Special case
            if (obj == this._scope && i == 'imports')
                continue;

            if (typeof obj[i] != 'object')
                continue;

            if (obj instanceof Array) {
                ret = this._findObjectPath('[' + i + ']', obj[i]);

                if (ret) {
                    ret =  name + ret;
                    break;
                }
            } else {
                ret = this._findObjectPath(i, obj[i]);

                if (ret) {
                    ret = name + '.' + ret;
                    break;
                }
            }
        }
        this._level--;

        return ret;
    },

    FindObjectPath: function(address) {
        this._objs = {};
        this._objId = 0;
        this._level = 0;
        this._addressRegexp = new RegExp('jsobj@0x' + address.toString(16));

        let ret = this._findObjectPath('window', this._scope);

        for (let i in this._objs)
            delete this._objs[i].__id;
        this._objs = {};

        if (ret)
            return ret;
        return '';
    },

    RunGC: function() {
        System.gc();
    },

    /**/

    shutdown: function() {
        this._dbusImpl.unexport('/org/gjs/Memory');
    },
});

let _memoryDaemon = null;

function startMemoryService(scope) {
    if (_memoryDaemon != null)
        return;

    _memoryDaemon = new MemoryService(scope);
}

function stopMemoryService() {
    if (_memoryDaemon == null)
        return;

    _memoryDaemon.shutdown();
    _memoryDaemon = null;
}

function findMemoryObject(address) {
    return _memoryDaemon.FindObjectPath(address);
}
