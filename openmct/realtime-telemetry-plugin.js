/**
 * Realtime telemetry plugin using WebSockets.
 */
function RealtimeTelemetryPlugin() {
    return function (openmct) {
        var socket = new WebSocket('ws://localhost:8765');
        var listener = {};

        socket.onmessage = function (event) {
            var point = JSON.parse(event.data);

            if (listener[point.id]) {
                listener[point.id](point);
            }
        };

        var provider = {
            /*
             * OpenMCT asks for historical data when a plot is opened.
             * This demo does not store historical telemetry.
             * Return an empty set instead of requesting /history.
             */
            supportsRequest: function (domainObject) {
                return domainObject.type === 'example.telemetry';
            },

            request: function (domainObject, options) {
                return Promise.resolve([]);
            },

            /*
             * Subscribe to live telemetry from the Python bridge.
             */
            supportsSubscribe: function (domainObject) {
                return domainObject.type === 'example.telemetry';
            },

            subscribe: function (domainObject, callback) {
                listener[domainObject.identifier.key] = callback;

                socket.send(
                    'subscribe ' + domainObject.identifier.key
                );

                return function unsubscribe() {
                    delete listener[domainObject.identifier.key];

                    socket.send(
                        'unsubscribe ' + domainObject.identifier.key
                    );
                };
            }
        };

        openmct.telemetry.addProvider(provider);
    };
}
