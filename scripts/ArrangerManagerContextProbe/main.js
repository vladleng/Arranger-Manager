// Diagnostic, read-only bridge. The EditTask iterator contains the selected
// events in the current editing context, not necessarily the entire song.
function safeValue(fn) {
    try {
        var value = fn();
        return value === undefined || value === null ? null : value;
    } catch (e) {
        return null;
    }
}

function timeOf(event, property) {
    var time = safeValue(function () { return event[property]; });
    if (!time) return null;
    return {
        seconds: safeValue(function () { return time.seconds; }),
        beats: safeValue(function () { return time.musical; }),
        display: safeValue(function () { return time.string; })
    };
}

// Only read known scalar properties. Host objects can contain cyclic native
// references; serializing or enumerating them is unsafe and not useful here.
function scalar(value) {
    return typeof value === "string" || typeof value === "number" || typeof value === "boolean"
        ? value : null;
}

function inspectObject(object) {
    if (!object) return { available: false };
    var result = { available: true, properties: {}, parameters: {} };
    var properties = ["name", "title", "notes", "note", "text", "artist", "album", "genre", "year", "comment"];
    var parameters = ["title", "artist", "album", "genre", "year", "notes", "note", "comment", "tempo", "sampleRate"];
    for (var i = 0; i < properties.length; i++) {
        var key = properties[i];
        var value = safeValue((function (k) { return function () { return object[k]; }; })(key));
        if (scalar(value) !== null) result.properties[key] = value;
    }
    for (var j = 0; j < parameters.length; j++) {
        var paramName = parameters[j];
        var parameter = safeValue((function (k) { return function () { return object.findParameter(k); }; })(paramName));
        if (!parameter) continue;
        var formatted = safeValue((function (p) { return function () { return p.string; }; })(parameter));
        var numeric = safeValue((function (p) { return function () { return p.value; }; })(parameter));
        if (scalar(formatted) !== null || scalar(numeric) !== null) {
            result.parameters[paramName] = { string: scalar(formatted), value: scalar(numeric) };
        }
    }
    return result;
}

function exportMetadata(context) {
    var root = safeValue(function () { return context.functions.root; });
    var environment = root && safeValue(function () { return root.environment; });
    var urls = [
        "://hostapp/DocumentManager/ActiveDocument",
        "://hostapp/DocumentManager/ActiveDocument/Environment",
        "://hostapp/DocumentManager/ActiveDocument/SongInformation",
        "://hostapp/DocumentManager/ActiveDocument/SongInfo",
        "://hostapp/DocumentManager/ActiveDocument/DocumentInfo",
        "://hostapp/DocumentManager/ActiveDocument/Notes",
        "://hostapp/SongCustomization/Inspector.Notes",
        "://hostapp/SongCustomization/Toolbar.InfoView"
    ];
    var objects = {};
    for (var i = 0; i < urls.length; i++) {
        var url = urls[i];
        var object = safeValue((function (u) {
            return function () { return Host.Objects.getObjectByUrl(u); };
        })(url));
        objects[url] = inspectObject(object);
    }
    var payload = {
        schema: "arranger-manager-metadata-probe-v1",
        scope: "active document; read-only script access",
        document: {
            root: inspectObject(root),
            environment: inspectObject(environment),
            start: root ? timeOf({ startTime: safeValue(function () { return root.getStartTime(); }) }, "startTime") : null,
            end: root ? timeOf({ endTime: safeValue(function () { return root.getEndTime(); }) }, "endTime") : null
        },
        objects: objects,
        caveat: "Inspector.Notes and Toolbar.InfoView are UI controls, not proof of access to session Notes. A missing property does not prove the data is absent from the saved .song file."
    };
    var filename = "Arranger_Manager_Metadata.json";
    var file = Host.IO.createTextFile(Host.Url("local://$USERCONTENT/" + filename));
    if (!file) {
        Host.GUI.alert("Arranger Manager: could not write " + filename);
        return Host.Results.kResultOk;
    }
    file.writeLine(JSON.stringify(payload, null, 2));
    file.close();
    Host.GUI.alert("Arranger Manager: exported metadata probe to " + filename);
    return Host.Results.kResultOk;
}

function ProbeTask(kind) {
    this.interfaces = [Host.Interfaces.IEditTask];
    this.prepareEdit = function () { return Host.Results.kResultOk; };
    this.performEdit = function (context) {
        if (kind === "metadata") return exportMetadata(context);
        var rows = [];
        var iterator = context.iterator;
        var error = null;
        try {
            iterator.first();
            while (!iterator.done()) {
                var event = iterator.next();
                if (!event) continue;
                rows.push({
                    name: safeValue(function () { return event.name; }),
                    color: safeValue(function () { return event.color; }),
                    start: timeOf(event, "startTime"),
                    end: timeOf(event, "endTime"),
                    lengthBeats: safeValue(function () { return event.length; }),
                    mediaType: safeValue(function () { return event.mediaType; }),
                    track: safeValue(function () { return event.getTrack().name; })
                });
                if (rows.length >= 512) break;
            }
        } catch (e) {
            error = String(e);
        }

        var cursor = safeValue(function () { return context.editor.cursorInfo; });
        var payload = {
            schema: "arranger-manager-context-probe-v1",
            kind: kind,
            scope: "selected events only",
            count: rows.length,
            cursor: cursor ? {
                position: timeOf(cursor, "cursorTime"),
                loopStart: timeOf(cursor, "loopStart"),
                loopEnd: timeOf(cursor, "loopEnd")
            } : null,
            events: rows,
            error: error
        };
        var filename = kind === "arranger" ? "Arranger_Manager_Arranger.json" : "Arranger_Manager_Markers.json";
        var file = Host.IO.createTextFile(Host.Url("local://$USERCONTENT/" + filename));
        if (!file) {
            Host.GUI.alert("Arranger Manager: could not write " + filename);
            return Host.Results.kResultOk;
        }
        file.writeLine(JSON.stringify(payload, null, 2));
        file.close();
        Host.GUI.alert("Arranger Manager: exported " + rows.length + " selected events to " + filename
            + (error ? " (see error field)" : ""));
        return Host.Results.kResultOk;
    };
}

function createArrangerInstance() { return new ProbeTask("arranger"); }
function createMarkerInstance() { return new ProbeTask("markers"); }
function createMetadataInstance() { return new ProbeTask("metadata"); }
