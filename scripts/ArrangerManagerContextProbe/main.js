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

function ProbeTask(kind) {
    this.interfaces = [Host.Interfaces.IEditTask];
    this.prepareEdit = function () { return Host.Results.kResultOk; };
    this.performEdit = function (context) {
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
