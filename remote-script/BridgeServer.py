"""UDP OSC server for communication with JUCE plugin."""

import json
import uuid
import threading
import logging
import socket

from python_osc.dispatcher import Dispatcher
from python_osc.osc_server import ThreadingOSCUDPServer
from python_osc.osc_message_builder import OscMessageBuilder

logger = logging.getLogger(__name__)

PROTOCOL_VERSION = 1
OSC_PREFIX = "/loopercontrol"
DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 7011


class BridgeServer:
    """OSC UDP server for bidirectional communication with the JUCE plugin."""

    def __init__(self, host=DEFAULT_HOST, port=DEFAULT_PORT, on_command=None):
        """Initialize the BridgeServer.

        Args:
            host: IP address to bind to (default: 127.0.0.1)
            port: UDP port to bind to (default: 7011)
            on_command: Callback function(ns, nsid, name, args) -> response dict
        """
        self._host = host
        self._port = port
        self._on_command = on_command
        self._dispatcher = Dispatcher()
        self._dispatcher.map(f"{OSC_PREFIX}/*", self._on_osc_message)
        self._server = None
        self._thread = None
        self._plugin_port = None  # Discovered via handshake (D-06)

    @property
    def port(self):
        """Get the actual port the server is bound to."""
        return self._port

    def start(self):
        """Start the OSC server on a separate thread."""
        # Try port range starting from our preferred port (D-05: range fallback for script port 7011-7020)
        for port in range(self._port, self._port + 10):
            try:
                self._server = ThreadingOSCUDPServer(
                    (self._host, port),
                    self._dispatcher
                )
                self._port = port
                break
            except OSError:
                continue

        if self._server is None:
            logger.error(f"Could not bind to any port in range {self._port}-{self._port + 9}")
            return

        self._thread = threading.Thread(target=self._server.serve_forever, daemon=True)
        self._thread.start()
        logger.info(f"BridgeServer started on {self._host}:{self._port}")

    def stop(self):
        """Stop the OSC server."""
        if self._server:
            self._server.shutdown()
            logger.info("BridgeServer stopped")

    def send_to_plugin(self, address, data_dict):
        """Send a message to the plugin's bound port.

        Args:
            address: OSC address string
            data_dict: Dictionary to serialize as JSON in OSC string argument
        """
        if self._plugin_port is None:
            logger.warning("Plugin port not known yet, cannot send message")
            return

        builder = OscMessageBuilder(address=address)
        # Serialize the entire data dict as a JSON string in one OSC string argument
        builder.add_arg(json.dumps(data_dict))
        msg = builder.build()
        # Send via UDP to plugin
        try:
            self._server.socket.sendto(
                msg.dgram,
                (self._host, self._plugin_port)
            )
        except Exception as e:
            logger.error(f"Error sending message: {e}")

    def send_event(self, event_type, data):
        """Send an event to the plugin (D-01: JSON with namespace format, D-03: full state push).

        Args:
            event_type: String event type (e.g., 'looper_state_changed')
            data: Dictionary of event data
        """
        message = {
            "uuid": None,  # Events don't have request UUIDs
            "event": event_type,
            "data": data,
            "version": PROTOCOL_VERSION  # D-04
        }
        self.send_to_plugin(f"{OSC_PREFIX}/event", message)

    def send_response(self, request_uuid, data):
        """Send a response correlating to a request UUID (D-02).

        Args:
            request_uuid: UUID string from the original request
            data: Dictionary of response data
        """
        message = {
            "uuid": request_uuid,
            "event": "result",
            "data": data,
            "version": PROTOCOL_VERSION  # D-04
        }
        self.send_to_plugin(f"{OSC_PREFIX}/response", message)

    def _on_osc_message(self, address, *args):
        """Handle incoming OSC message from plugin.

        Args:
            address: OSC address pattern that matched
            *args: OSC arguments from the message
        """
        try:
            if len(args) < 1:
                logger.warning(f"No args in message to {address}")
                return

            message_json = args[0]
            message = json.loads(message_json)

            # D-04: Version check
            if message.get("version", 0) != PROTOCOL_VERSION:
                logger.warning(f"Version mismatch: got {message.get('version')}, expected {PROTOCOL_VERSION}")
                # Send error response
                self.send_response(message.get("uuid"), {"success": False, "error": "version_mismatch"})
                return

            ns = message.get("ns", "")
            nsid = message.get("nsid", "")
            name = message.get("name", "")
            msg_args = message.get("args", {})

            # D-06: Hello/handshake handling
            if name == "hello" and ns == "system":
                self._plugin_port = msg_args.get("port", 7010)
                logger.info(f"Plugin connected on port {self._plugin_port}")
                self.send_response(message["uuid"], {
                    "success": True,
                    "version": PROTOCOL_VERSION,
                    "port": self._port
                })
                if self._on_command:
                    self._on_command("system", "", "hello", msg_args)
                return

            # Dispatch to command handler
            if self._on_command:
                result = self._on_command(ns, nsid, name, msg_args)
                if result is not None:
                    self.send_response(message.get("uuid"), result)

        except json.JSONDecodeError as e:
            logger.error(f"Invalid JSON in message: {e}")
        except Exception as e:
            logger.error(f"Error handling message: {e}")

    def is_plugin_connected(self):
        """Check if the plugin has completed handshake.

        Returns:
            bool: True if plugin port is known (handshake completed)
        """
        return self._plugin_port is not None
