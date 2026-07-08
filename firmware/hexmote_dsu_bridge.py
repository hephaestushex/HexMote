#!/usr/bin/env python3
"""
HexMote DSU Bridge
-------------------
Reads accel+gyro from HexMote's custom BLE GATT characteristic and serves it
to Dolphin/Cemu/Yuzu as a standard DSU (cemuhook) motion source over UDP.

This does NOT feed Steam Input -- Steam Input's gyro pipeline doesn't consume
DSU at all, only specific recognized controllers or the Deck's own internal
sensors. This bridge only helps emulators with native cemuhook/DSU support.

Protocol implemented against: https://v1993.github.io/cemuhook-protocol/

FLAGGED ASSUMPTIONS -- verify/adjust before trusting this fully:
  1. MOTION_CHAR_UUID below is my best-guess string form of the byte array
     defined in the firmware (Adafruit BLEUUID128 byte order). Confirm the
     actual advertised UUID with a BLE scanner app (e.g. nRF Connect) and
     fix this constant if it doesn't match.
  2. Axis mapping (gx->pitch, gy->yaw, gz->roll) is a placeholder. Whether
     that's correct depends on how the ICM-20948 is physically oriented on
     your PCB -- expect to swap/negate axes empirically once you can see
     motion responding in Dolphin's controller test screen.
  3. Scale factors assume default ICM-20948 ranges set in firmware:
     accel +/-2g (16384 LSB/g), gyro +/-250dps (131 LSB/dps). If you change
     the FS_SEL settings in firmware, update SCALE_ACCEL / SCALE_GYRO here.
  4. Battery status is hardcoded "full" -- no real battery ADC hookup yet.

Requires: pip install bleak
Run: python3 hexmote_dsu_bridge.py [--port 26761]
"""

import argparse
import asyncio
import struct
import time
import zlib
from bleak import BleakClient, BleakScanner

# ---------------- Config ----------------
DEVICE_NAME = "HexMote"
MOTION_CHAR_UUID = "9e5d80e0-2508-4b92-8b2f-4b1c02000fa0"  # verify! see note above

SCALE_ACCEL = 1.0 / 16384.0   # raw LSB -> g   (+/-2g range)
SCALE_GYRO  = 1.0 / 131.0     # raw LSB -> deg/s (+/-250dps range)

PROTOCOL_VERSION = 1001
CLIENT_TIMEOUT_SEC = 5.0
STREAM_RATE_HZ = 100

# ---------------- Shared motion state (updated by BLE notify) ----------------
class MotionState:
    def __init__(self):
        self.ax = self.ay = self.az = 0.0
        self.gx = self.gy = self.gz = 0.0
        self.timestamp_us = 0

    def update_from_ble(self, data: bytes):
        # firmware struct HexMotionPacket: int16 ax,ay,az,gx,gy,gz (little-endian)
        raw = struct.unpack('<6h', data[:12])
        rax, ray, raz, rgx, rgy, rgz = raw
        self.ax = rax * SCALE_ACCEL
        self.ay = ray * SCALE_ACCEL
        self.az = raz * SCALE_ACCEL
        self.gx = rgx * SCALE_GYRO
        self.gy = rgy * SCALE_GYRO
        self.gz = rgz * SCALE_GYRO
        self.timestamp_us = int(time.time() * 1_000_000)

motion = MotionState()

# ---------------- DSU packet building ----------------
HEADER_MAGIC_SERVER = b'DSUS'
SERVER_ID = 0x48455831  # arbitrary fixed id ("HEX1" in ascii-ish hex)

def build_packet(event_type: int, payload: bytes) -> bytes:
    inner = struct.pack('<I', event_type) + payload
    length = len(inner)
    header = (HEADER_MAGIC_SERVER +
              struct.pack('<HH', PROTOCOL_VERSION, length) +
              struct.pack('<I', 0) +               # crc placeholder
              struct.pack('<I', SERVER_ID))
    packet = header + inner
    crc = zlib.crc32(packet) & 0xFFFFFFFF
    packet = packet[:8] + struct.pack('<I', crc) + packet[12:]
    return packet

def shared_beginning(slot: int, connected: bool) -> bytes:
    state = 2 if connected else 0
    model = 2 if connected else 0       # 2 = full gyro
    conn_type = 2 if connected else 0   # 2 = bluetooth
    mac = b'\x00' * 6                   # zeroed, not applicable
    battery = 0x05 if connected else 0x00  # hardcoded "full" -- see TODO above
    return struct.pack('<BBBB', slot, state, model, conn_type) + mac + struct.pack('<B', battery)

def build_controller_info(slot: int, connected: bool) -> bytes:
    payload = shared_beginning(slot, connected) + b'\x00'  # 11 + 1 = 12 bytes
    return build_packet(0x100001, payload)

_packet_counter = 0

def build_controller_data(slot: int) -> bytes:
    global _packet_counter
    _packet_counter += 1

    beginning = shared_beginning(slot, True)               # 11 bytes
    connected_flag = struct.pack('<B', 1)                  # 1 byte
    packet_num = struct.pack('<I', _packet_counter)         # 4 bytes

    dpad_buttons = struct.pack('<BB', 0, 0)                # no dpad/face buttons -- HID gamepad covers these
    home_touch = struct.pack('<BB', 0, 0)
    sticks = struct.pack('<BBBB', 128, 128, 128, 128)      # neutral, HID gamepad covers real stick
    analogs = struct.pack('<BBBBBBBB', 0, 0, 0, 0, 0, 0, 0, 0)
    touch1 = struct.pack('<BBHH', 0, 0, 0, 0)
    touch2 = struct.pack('<BBHH', 0, 0, 0, 0)

    ts = struct.pack('<Q', motion.timestamp_us)
    accel = struct.pack('<fff', motion.ax, motion.ay, motion.az)
    gyro = struct.pack('<fff', motion.gx, motion.gy, motion.gz)  # pitch,yaw,roll -- verify axis mapping

    payload = (beginning + connected_flag + packet_num + dpad_buttons + home_touch +
               sticks + analogs + touch1 + touch2 + ts + accel + gyro)
    return build_packet(0x100002, payload)

# ---------------- UDP server ----------------
class DSUProtocol(asyncio.DatagramProtocol):
    def __init__(self):
        self.transport = None
        self.subscribers = {}  # addr -> last_seen_timestamp

    def connection_made(self, transport):
        self.transport = transport

    def datagram_received(self, data: bytes, addr):
        if len(data) < 20 or data[:4] != b'DSUC':
            return
        event_type = struct.unpack('<I', data[16:20])[0]
        payload = data[20:]

        if event_type == 0x100000:
            resp = build_packet(0x100000, struct.pack('<H', PROTOCOL_VERSION))
            self.transport.sendto(resp, addr)

        elif event_type == 0x100001:
            count = struct.unpack('<i', payload[:4])[0]
            slots = payload[4:4 + count]
            for slot in slots:
                connected = (slot == 0)  # only slot 0 (HexMote) is ever connected
                self.transport.sendto(build_controller_info(slot, connected), addr)

        elif event_type == 0x100002:
            # subscribe -- we don't bother distinguishing slot/MAC registration,
            # HexMote is always slot 0
            self.subscribers[addr] = time.time()

    def prune_stale(self):
        now = time.time()
        stale = [a for a, t in self.subscribers.items() if now - t > CLIENT_TIMEOUT_SEC]
        for a in stale:
            del self.subscribers[a]

    def broadcast(self):
        if not self.subscribers:
            return
        packet = build_controller_data(0)
        for addr in list(self.subscribers.keys()):
            self.transport.sendto(packet, addr)

# ---------------- BLE connection ----------------
async def ble_loop():
    print(f"Scanning for '{DEVICE_NAME}'...")
    device = await BleakScanner.find_device_by_name(DEVICE_NAME, timeout=15.0)
    if device is None:
        print(f"Could not find a device named '{DEVICE_NAME}'. Is it powered on and advertising?")
        return

    def on_notify(_sender, data: bytearray):
        motion.update_from_ble(bytes(data))

    while True:
        try:
            async with BleakClient(device) as client:
                print(f"Connected to {device.address}")
                await client.start_notify(MOTION_CHAR_UUID, on_notify)
                print("Subscribed to motion characteristic. Streaming...")
                while client.is_connected:
                    await asyncio.sleep(1.0)
        except Exception as e:
            print(f"BLE connection lost/failed ({e}), retrying in 3s...")
            await asyncio.sleep(3.0)

# ---------------- Main ----------------
async def main(port: int):
    loop = asyncio.get_running_loop()
    transport, protocol = await loop.create_datagram_endpoint(
        DSUProtocol, local_addr=('0.0.0.0', port)
    )
    print(f"DSU server listening on UDP port {port}")

    ble_task = asyncio.create_task(ble_loop())

    interval = 1.0 / STREAM_RATE_HZ
    try:
        while True:
            protocol.prune_stale()
            protocol.broadcast()
            await asyncio.sleep(interval)
    finally:
        ble_task.cancel()
        transport.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="HexMote to DSU/cemuhook bridge")
    parser.add_argument("--port", type=int, default=26761,
                         help="UDP port to serve DSU on (default 26761, avoids conflict with SteamDeckGyroDSU's 26760)")
    args = parser.parse_args()

    try:
        asyncio.run(main(args.port))
    except KeyboardInterrupt:
        print("\nShutting down.")
