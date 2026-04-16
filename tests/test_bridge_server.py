"""Tests for BridgeServer OSC UDP server."""

import sys
import os
import json
import threading
import time
import socket

# Add the remote-script directory to the path for imports
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'remote-script'))

import pytest
from BridgeServer import BridgeServer, PROTOCOL_VERSION, OSC_PREFIX, DEFAULT_HOST, DEFAULT_PORT
from python_osc.osc_message_builder import OscMessageBuilder
from python_osc.udp_client import UDPClient


class TestBridgeServerBasics:
    """Tests for basic BridgeServer functionality."""

    def test_bridge_server_starts_on_default_port(self):
        """Start server, verify it's listening on port 7011."""
        server = BridgeServer(host="127.0.0.1", port=7011)
        server.start()

        # Give server time to start
        time.sleep(0.05)

        try:
            assert server.port == 7011
            assert server._server is not None
        finally:
            server.stop()

    def test_bridge_server_port_range_fallback(self):
        """Start two servers, verify second gets port 7012."""
        server1 = BridgeServer(host="127.0.0.1", port=9011)
        server1.start()
        time.sleep(0.05)

        server2 = BridgeServer(host="127.0.0.1", port=9011)
        server2.start()
        time.sleep(0.05)

        try:
            assert server1.port == 9011
            assert server2.port == 9012
        finally:
            server1.stop()
            server2.stop()


class TestBridgeServerMessaging:
    """Tests for OSC message handling."""

    def test_bridge_server_handles_json_message(self):
        """Send JSON message to /loopercontrol/command, verify parsing."""
        received_messages = []

        def on_command(ns, nsid, name, args):
            received_messages.append({"ns": ns, "nsid": nsid, "name": name, "args": args})
            return {"success": True}

        server = BridgeServer(host="127.0.0.1", port=7011, on_command=on_command)
        server.start()
        server._plugin_port = 7010  # Mock plugin port

        time.sleep(0.05)

        # Send a test message
        client = UDPClient("127.0.0.1", server.port)
        msg = OscMessageBuilder(address=f"{OSC_PREFIX}/command")
        msg.add_arg(json.dumps({
            "uuid": "test-uuid-123",
            "ns": "looper",
            "nsid": "",
            "name": "discover",
            "args": {},
            "version": PROTOCOL_VERSION
        }))
        client.send(msg.build())

        time.sleep(0.1)

        server.stop()
        client.close()

        assert len(received_messages) == 1
        assert received_messages[0]["ns"] == "looper"
        assert received_messages[0]["name"] == "discover"

    def test_bridge_server_version_check_rejects_mismatch(self):
        """Send message with version=99, verify error response is sent."""
        responses = []

        def capture_response(address, data_dict):
            responses.append(data_dict)

        server = BridgeServer(host="127.0.0.1", port=9014)
        server.start()
        server._plugin_port = 7010
        server.send_to_plugin = capture_response

        time.sleep(0.05)

        # Create a mock OSC message with wrong version
        # We'll simulate the version check by calling _on_osc_message directly
        mock_message = {
            "uuid": "test-uuid",
            "ns": "test",
            "nsid": "",
            "name": "test",
            "args": {},
            "version": 99  # Wrong version
        }

        # Send via socket directly
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        builder = OscMessageBuilder(address=f"{OSC_PREFIX}/command")
        builder.add_arg(json.dumps(mock_message))
        msg = builder.build()
        sock.sendto(msg.dgram, ("127.0.0.1", server.port))

        time.sleep(0.1)
        sock.close()
        server.stop()

        # Should have received an error response
        assert len(responses) == 1
        assert responses[0]["data"]["error"] == "version_mismatch"

    def test_bridge_server_hello_saves_plugin_port(self):
        """Send hello with port 7010, verify plugin_port is stored."""
        hello_received = []

        def on_hello(ns, nsid, name, args):
            hello_received.append(args)
            return {"success": True}

        server = BridgeServer(host="127.0.0.1", port=7011, on_command=on_hello)
        server.start()

        time.sleep(0.05)

        # Send hello message
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        builder = OscMessageBuilder(address=f"{OSC_PREFIX}/command")
        builder.add_arg(json.dumps({
            "uuid": "hello-uuid",
            "ns": "system",
            "nsid": "",
            "name": "hello",
            "args": {"port": 7010},
            "version": PROTOCOL_VERSION
        }))
        msg = builder.build()
        sock.sendto(msg.dgram, ("127.0.0.1", server.port))

        time.sleep(0.1)
        sock.close()
        server.stop()

        assert len(hello_received) == 1
        assert server._plugin_port == 7010

    def test_bridge_server_sends_response_with_uuid(self):
        """Send command with UUID, verify response has same UUID."""
        responses = []
        test_uuid = "unique-uuid-456"

        def capture_response(address, data_dict):
            if "response" in address:
                responses.append(data_dict)

        def on_command(ns, nsid, name, args):
            return {"success": True}

        server = BridgeServer(host="127.0.0.1", port=7013, on_command=on_command)
        server.start()
        server._plugin_port = 7010
        server.send_to_plugin = capture_response

        time.sleep(0.05)

        # Manually call the internal handler
        server._on_osc_message(
            f"{OSC_PREFIX}/command",
            json.dumps({
                "uuid": test_uuid,
                "ns": "test",
                "nsid": "",
                "name": "ping",
                "args": {},
                "version": PROTOCOL_VERSION
            })
        )

        time.sleep(0.1)
        server.stop()

        assert len(responses) == 1
        assert responses[0]["uuid"] == test_uuid

    def test_bridge_server_sends_event_with_version(self):
        """Send event, verify version field is 1."""
        event_data = None

        def capture_event(address, data_dict):
            nonlocal event_data
            if "event" in address:
                event_data = data_dict

        server = BridgeServer(host="127.0.0.1", port=7011)
        server.start()
        server._plugin_port = 7010
        server.send_to_plugin = capture_event

        time.sleep(0.05)

        # Send an event
        server.send_event("test_event", {"test": "data"})

        time.sleep(0.1)
        server.stop()

        assert event_data is not None
        assert event_data["version"] == PROTOCOL_VERSION


class TestBridgeServerProtocol:
    """Tests for protocol compliance (D-01 through D-06)."""

    def test_protocol_version_is_1(self):
        """Verify PROTOCOL_VERSION is 1."""
        assert PROTOCOL_VERSION == 1

    def test_osc_prefix_is_correct(self):
        """Verify OSC_PREFIX matches protocol."""
        assert OSC_PREFIX == "/loopercontrol"

    def test_default_port_is_7011(self):
        """Verify default port is 7011 (D-05)."""
        assert DEFAULT_PORT == 7011

    def test_event_message_format(self):
        """Verify event messages have correct format (D-01, D-04)."""
        server = BridgeServer(host="127.0.0.1", port=7011)
        server.start()
        server._plugin_port = 7010

        captured = []
        server.send_to_plugin = lambda addr, data: captured.append((addr, data))

        time.sleep(0.05)

        server.send_event("looper_discovered", {
            "track_id": "0",
            "track_name": "Guitar",
            "state": "Stopped"
        })

        time.sleep(0.1)
        server.stop()

        assert len(captured) == 1
        addr, data = captured[0]

        assert addr == f"{OSC_PREFIX}/event"
        assert data["uuid"] is None  # Events have no UUID
        assert data["event"] == "looper_discovered"
        assert data["data"]["track_name"] == "Guitar"
        assert data["version"] == PROTOCOL_VERSION


class TestBridgeServerConnection:
    """Tests for connection state management."""

    def test_is_plugin_connected_before_handshake(self):
        """Before handshake, is_plugin_connected should be False."""
        server = BridgeServer(host="127.0.0.1", port=7011)
        server.start()

        time.sleep(0.05)

        try:
            assert server.is_plugin_connected() is False
        finally:
            server.stop()

    def test_is_plugin_connected_after_handshake(self):
        """After hello, is_plugin_connected should be True."""
        server = BridgeServer(host="127.0.0.1", port=7011)
        server.start()

        time.sleep(0.05)

        # Simulate hello
        server._plugin_port = 7010

        try:
            assert server.is_plugin_connected() is True
        finally:
            server.stop()
