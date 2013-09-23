const GLib = imports.gi.GLib;
const Gio = imports.gi.Gio;
const Gtk = imports.gi.Gtk;
const Lang = imports.lang;

/**/
const MEMORY_OBJ_PATH = '/org/gjs/Memory';
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
const MemoryProxy = Gio.DBusProxy.makeProxyWrapper(MemoryIface);


/**/

Gtk.init(null, null);

let builder = new Gtk.Builder();
builder.add_from_file('./ObjDump.ui');

let win = builder.get_object('main-window');
let busEntry = builder.get_object('bus-entry');
let snapshotButton = builder.get_object('snapshot-button');
let snapshotSpinner = builder.get_object('snapshot-spinner');
let initialFilechooserButton = builder.get_object('initial-filechooser-button');
let currentFilechooserButton = builder.get_object('current-filechooser-button');
let objectsCountsTreeview = builder.get_object('objects-counts-treeview');
let objectsLocationsTreeview = builder.get_object('objects-locations-treeview');
let objectsCountsListstore = builder.get_object('objects-counts-liststore');
let objectsLocationsListstore = builder.get_object('objects-locations-treestore');
let objectPathLabel = builder.get_object('object-path-label');

/**/

snapshotButton.setSensitive = function(value) {
    this.set_sensitive(value)
    snapshotSpinner.set_visible(!value);
};

/**/

let _currentAppBusAddress = null;
let _currentAppMemoryProxy = null;

let getApplicationMemoryProxy = function() {
    let currentBusAddress = busEntry.get_text();
    if (currentBusAddress != _currentAppBusAddress) {
        _currentAppBusAddress = currentBusAddress;
        _currentAppMemoryProxy = new MemoryProxy(Gio.DBus.session,
                                                 _currentAppBusAddress,
                                                 MEMORY_OBJ_PATH);
    }

    return _currentAppMemoryProxy;
};

/**/

const Differ = new Lang.Class({
    Name: 'Differ',

    _init: function() {
        this._initial = {
            dump: null,
            chooser: initialFilechooserButton,
        };
        this._current = {
            dump: null,
            chooser: currentFilechooserButton,
        };
    },

    _fillInitial: function() {
        objectsCountsListstore.clear();

        for (let i in this._initial.dump) {
            let iter = objectsCountsListstore.append();
            objectsCountsListstore.set(iter,
                                       [0, 1, 2, 3],
                                       [i,
                                        this._initial.dump[i].length,
                                        0,
                                        0]);
        }
    },

    _recomputeDiff: function() {
        if (this._current.dump == null) {
            this._fillInitial();
            return;
        }

        objectsCountsListstore.clear();

        for (let i in this._initial.dump) {
            let iter = objectsCountsListstore.append();

            if (this._current.dump[i])
                objectsCountsListstore.set(iter,
                                           [0, 1, 2, 3],
                                           [i,
                                            this._initial.dump[i].length,
                                            this._current.dump[i].length,
                                            this._current.dump[i].length - this._initial.dump[i].length]);
            else
                objectsCountsListstore.set(iter,
                                           [0, 1, 2, 3],
                                           [i,
                                            this._initial.dump[i].length,
                                            0,
                                            -this._initial.dump[i].length]);
        }

        for (let i in this._current.dump) {
            if (!this._initial.dump[i]) {
                let iter = objectsCountsListstore.append();
                objectsCountsListstore.set(iter,
                                           [0, 1, 2, 3],
                                           [i,
                                            0,
                                            this._current.dump[i].length,
                                            this._current.dump[i].length]);
            }
        }
    },

    _setFromDump: function(item, dump) {
        item.dump = dump;
        let content = JSON.stringify(dump);
        let date = GLib.DateTime.new_now_local();
        let filename = './' + busEntry.get_text() + '-' + date.format('%F-%T');
        GLib.file_set_contents(filename, content, -1);

        this._settingFilename = true;
        item.chooser.set_filename(filename);
        this._settingFilename = false;

        this._recomputeDiff();
    },

    setInitialFromDump: function(dump) {
        this._setFromDump(this._initial, dump);
    },

    setCurrentFromDump: function(dump) {
        this._setFromDump(this._current, dump);
    },

    setAutoFromDump: function(dump) {
        if (this._initial.dump == null)
            this._setFromDump(this._initial, dump);
        else
            this._setFromDump(this._current, dump);
    },

    _setFromFile: function(item, path) {
        if (this._settingFilename)
            return;

        let [success, content, len] = GLib.file_get_contents(path);
        if (success) {
            let dump = JSON.parse(content);
            item.dump = dump;

            this._recomputeDiff();
        }
    },

    setInitialFromFile: function(path) {
        this._setFromFile(this._initial, path);
    },

    setCurrentFromFile: function(path) {
        this._setFromFile(this._current, path);
    },


    /**/

    _displayClassInitial: function(className) {
        let locations = {};

        objectsLocationsListstore.clear();

        for (let i in this._initial.dump[className]) {
            let loc = this._initial.dump[className][i].location;
            if (locations[loc]) {
                locations[loc].prevCount++;
            } else {
                locations[loc] = { iter: objectsLocationsListstore.append(null),
                                   prevCount: 1,
                                 };
            }
        }


        // Locations iters
        for (let i in locations) {
            objectsLocationsListstore.set(locations[i].iter,
                                          [0, 1],
                                          [i,
                                           locations[i].prevCount]);
        }
        // Instances iters
        for (let i in this._initial.dump[className]) {
            let loc = this._initial.dump[className][i].location;
            let address = this._initial.dump[className][i].address;
            let subIter = objectsLocationsListstore.append(locations[loc].iter);

            objectsLocationsListstore.set(subIter,
                                          [4],
                                          [address]);
        }
    },

    displayClass: function(className) {
        if (this._current.dump == null) {
            this._displayClassInitial(className);
            return;
        }

        let locations = {};

        objectsLocationsListstore.clear();

        for (let i in this._initial.dump[className]) {
            let loc = this._initial.dump[className][i].location;
            if (locations[loc]) {
                locations[loc].prevCount++
            } else {
                locations[loc] = { iter: objectsLocationsListstore.append(null),
                                   prevCount: 1,
                                   currentCount: 0,
                                 };
            }
        }
        for (let i in this._current.dump[className]) {
            let loc = this._current.dump[className][i].location;
            if (locations[loc]) {
                locations[loc].currentCount++;
            } else {
                locations[loc] = { iter: objectsLocationsListstore.append(null),
                                   prevCount: 0,
                                   currentCount: 1,
                                 };
            }
        }

        // Locations iters
        for (let i in locations) {
            objectsLocationsListstore.set(locations[i].iter,
                                          [0, 1, 2, 3],
                                          [i,
                                           locations[i].prevCount,
                                           locations[i].currentCount,
                                           locations[i].currentCount - locations[i].prevCount]);
        }
        // Instances iters
        for (let i in this._current.dump[className]) {
            let loc = this._current.dump[className][i].location;
            let address = this._current.dump[className][i].address;
            let subIter = objectsLocationsListstore.append(locations[loc].iter);

            objectsLocationsListstore.set(subIter,
                                          [4],
                                          [address]);
        }
    },

});
let differ = new Differ();

/**/

win.connect('destroy', Lang.bind(this, function() {
    Gtk.main_quit();
}));

/**/

let newColumn = function(treeview, title, property, columnId, rendererConstructor) {
    let column = new Gtk.TreeViewColumn({ title: title,
                                          //sizing: Gtk.TreeViewColumnSizing.FIXED,
                                          clickable: true,
                                        });
    let renderer = new rendererConstructor();
    column.pack_start(renderer, false);
    column.add_attribute(renderer, property, columnId);
    treeview.append_column(column);

    column.connect('clicked', Lang.bind(this, function() {
        let model = treeview.get_model();
        let [notSpecial, sortColumnId, sortOrder] = model.get_sort_column_id();
        if (sortColumnId == columnId) {
            model.set_sort_column_id(columnId,
                                     sortOrder == Gtk.SortType.ASCENDING ?
                                     Gtk.SortType.DESCENDING : Gtk.SortType.ASCENDING);
        } else {
            model.set_sort_column_id(columnId, sortOrder);
        }
    }));
};

newColumn(objectsCountsTreeview, 'Object Type', 'text', 0, Gtk.CellRendererText);
newColumn(objectsCountsTreeview, 'Diff', 'text', 3, Gtk.CellRendererText);
newColumn(objectsCountsTreeview, 'Prev', 'text', 1, Gtk.CellRendererText);
newColumn(objectsCountsTreeview, 'Current', 'text', 2, Gtk.CellRendererText);

newColumn(objectsLocationsTreeview, 'Location', 'text', 0, Gtk.CellRendererText);
newColumn(objectsLocationsTreeview, 'Diff', 'text', 3, Gtk.CellRendererText);
newColumn(objectsLocationsTreeview, 'Prev', 'text', 1, Gtk.CellRendererText);
newColumn(objectsLocationsTreeview, 'Current', 'text', 2, Gtk.CellRendererText);

objectsCountsTreeview.connect('row-activated', Lang.bind(this, function(treeview, path, column) {
    let [, iter] = objectsCountsListstore.get_iter(path);
    let className = objectsCountsListstore.get_value(iter, 0);
    log('clicked : ' + className);
    differ.displayClass(className);
}));

objectsLocationsTreeview.connect('row-activated', Lang.bind(this, function(treeview, path, column) {
    let [, iter] = objectsLocationsListstore.get_iter(path);
    let address = objectsLocationsListstore.get_value(iter, 4);
    if (address == 0)
        return;

    log('looking for address : ' + address);
    let proxy = getApplicationMemoryProxy();
    let ret = proxy.FindObjectPathRemote(address, Lang.bind(this, function(ret) {
        let [expr] = ret;

        log('found at : ' + expr);

        objectPathLabel.set_text(expr);
    }));
}));

/**/

let updateSnapshotSensitivity = function() {
    let text = busEntry.get_text();
    snapshotButton.set_sensitive(!(text == null || text == ''));
    snapshotSpinner.set_visible(false);
};

busEntry.connect('notify::text', Lang.bind(this, updateSnapshotSensitivity));
updateSnapshotSensitivity();

/**/

initialFilechooserButton.connect('selection-changed', Lang.bind(this, function() {
    let file = initialFilechooserButton.get_file()
    if (file)
        differ.setInitialFromFile(file.get_path());
}));
currentFilechooserButton.connect('selection-changed', Lang.bind(this, function() {
    let file = currentFilechooserButton.get_file();
    if (file)
        differ.setCurrentFromFile(file.get_path());
}));


/**/

let _takeSnapshot = function() {
    let appMemoryProxy = getApplicationMemoryProxy();

    appMemoryProxy.GetObjectsCountsRemote(Lang.bind(this, function(ret) {
        let [items] = ret;
        let decCount = 0;
        let memoryDump = {};
        snapshotButton.setSensitive(false);

        let getObjects = function(index) {
            appMemoryProxy.GetObjectsRemote(items[index][0], Lang.bind(this, function(ret) {
                if (ret == null) {
                    log('NULL for ' + items[index][0] + ' ???');
                    decCount++;
                    getObjects(decCount);
                    return;
                }

                let [objects] = ret;
                let itemName = items[index][0];

                memoryDump[itemName] = [];
                for (let j in objects) {
                    memoryDump[itemName].push({ location: objects[j][1],
                                                address: objects[j][2], });
                    //log('\t' + objects[j][1]);
                }

                decCount++;
                log(decCount);

                if (decCount <= items.length - 1) {
                    getObjects(decCount);
                } else {
                    differ.setAutoFromDump(memoryDump);
                    snapshotButton.setSensitive(true);
                }
            }));
        };
        getObjects(0);
    }));
};

let takeSnapshot = function() {
    let appMemoryProxy = getApplicationMemoryProxy();

    appMemoryProxy.RunGCRemote(Lang.bind(this, function() {
        _takeSnapshot();
    }));
};
snapshotButton.connect('clicked', Lang.bind(this, takeSnapshot));

/**/

win.show();
Gtk.main();
