#!/usr/bin/env python3
"""
upload_fw.py — Firmware upload tool for DoerGPIO bootloader.

Usage:
    python3 upload_fw.py <firmware.bin> [--timeout 60]

Waits up to --timeout seconds for the board to be plugged in.
As soon as a new serial port is detected, sends the firmware via the
bootloader's UART update protocol.

Protocol summary:
    Host  -> 0x7F              (trigger)
    Board -> 0x79              (ACK)
    Host  -> 4 bytes size (LE)
    Board -> 0x79              (ACK)
    [ repeat per 64-byte chunk ]
    Host  -> 64 bytes + 1 byte XOR checksum
    Board -> 0x79 (ACK) or 0x1F (NAK → retransmit)
    Board -> 0x79              (final ACK, board jumps to app)
"""

import sys
import time
import struct
import argparse
from typing import Optional

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    print("Error: pyserial not installed.  Run:  pip install pyserial")
    sys.exit(1)

# ── Constants ────────────────────────────────────────────────────────────────
BAUD_RATE   = 230400
TRIGGER     = 0x7F
ACK         = 0x79
NAK         = 0x1F
CHUNK_SIZE  = 64
MAX_RETRIES = 3


# ── Helpers ──────────────────────────────────────────────────────────────────
def xor_checksum(data: bytes) -> int:
    crc = 0
    for b in data:
        crc ^= b
    return crc


def get_ports() -> set:
    return {p.device for p in serial.tools.list_ports.comports()}


# ── Board detection ──────────────────────────────────────────────────────────
def wait_for_board(timeout_sec: int) -> Optional[str]:
    """
    Poll for a new serial port every 50 ms.
    Returns the port name as soon as one appears, or None on timeout.
    """
    print(f"Waiting for board to be plugged in  (timeout: {timeout_sec}s) ...")
    known    = get_ports()
    deadline = time.monotonic() + timeout_sec

    while time.monotonic() < deadline:
        current  = get_ports()
        new_ports = current - known
        if new_ports:
            port = sorted(new_ports)[0]
            print(f"  Board detected on: {port}")
            return port
        time.sleep(0.05)

    return None


# ── Upload ───────────────────────────────────────────────────────────────────
def upload(port: str, firmware: bytes) -> bool:
    """Open the serial port and run the bootloader update protocol."""
    print(f"  Connecting at {BAUD_RATE} baud ...")
    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=1)
    except serial.SerialException as exc:
        print(f"  Cannot open port: {exc}")
        return False

    try:
        # ── Phase 1: trigger ──────────────────────────────────────────────
        print("  Sending trigger ...")
        ser.write(bytes([TRIGGER]))

        resp = ser.read(1)
        if not resp or resp[0] != ACK:
            got = resp.hex() if resp else "nothing"
            print(f"  No ACK for trigger (got: {got})")
            print("  Tip: plug the board in BEFORE the 0.5s bootloader window closes.")
            return False

        # ── Phase 2: firmware size ────────────────────────────────────────
        size = len(firmware)
        ser.write(struct.pack("<I", size))

        resp = ser.read(1)
        if not resp:
            print("  No response for size (timeout)")
            return False
        if resp[0] == NAK:
            print(f"  Bootloader rejected firmware size ({size} bytes) — binary too large?")
            return False
        if resp[0] != ACK:
            print(f"  Unexpected response for size: 0x{resp[0]:02X}")
            return False

        print(f"  Uploading {size} bytes in {(size + CHUNK_SIZE - 1) // CHUNK_SIZE} chunks ...")

        # ── Phase 3: data chunks ──────────────────────────────────────────
        total  = (size + CHUNK_SIZE - 1) // CHUNK_SIZE
        failed = False

        for i in range(total):
            chunk = firmware[i * CHUNK_SIZE: (i + 1) * CHUNK_SIZE]
            # Pad last chunk to CHUNK_SIZE with 0xFF
            if len(chunk) < CHUNK_SIZE:
                chunk = chunk + bytes([0xFF] * (CHUNK_SIZE - len(chunk)))

            sent = False
            for attempt in range(MAX_RETRIES):
                crc = xor_checksum(chunk)
                ser.write(chunk + bytes([crc]))

                resp = ser.read(1)
                if resp and resp[0] == ACK:
                    sent = True
                    break
                elif resp and resp[0] == NAK:
                    print(f"\n  NAK on chunk {i + 1} — retrying ({attempt + 1}/{MAX_RETRIES}) ...")
                else:
                    got = resp.hex() if resp else "nothing"
                    print(f"\n  Unexpected response on chunk {i + 1}: {got}")
                    failed = True
                    break

            if failed or not sent:
                print(f"\n  Failed on chunk {i + 1} after {MAX_RETRIES} attempts.")
                return False

            # Progress bar
            pct = (i + 1) / total * 100
            filled = int(pct / 2)
            bar    = "=" * filled + " " * (50 - filled)
            print(f"\r  [{bar}] {pct:5.1f}%  ({i + 1}/{total})", end="", flush=True)

        print()   # newline after progress bar

        # ── Phase 4: final ACK ────────────────────────────────────────────
        resp = ser.read(1)
        if not resp or resp[0] != ACK:
            print("  No final ACK from board.")
            return False

        print("  Upload complete!  Board is rebooting into the application.")
        return True

    except Exception as exc:
        print(f"  Upload error: {exc}")
        return False

    finally:
        ser.close()


# ── Entry point ───────────────────────────────────────────────────────────────
def main() -> None:
    parser = argparse.ArgumentParser(
        description="Upload firmware to DoerGPIO board via UART bootloader"
    )
    parser.add_argument("firmware", help="Path to firmware .bin file")
    parser.add_argument(
        "--timeout",
        type=int,
        default=60,
        help="Seconds to wait for board plug-in (default: 60)",
    )
    args = parser.parse_args()

    # Load firmware binary
    try:
        with open(args.firmware, "rb") as f:
            firmware = f.read()
        print(f"Firmware : {args.firmware}  ({len(firmware)} bytes)")
    except OSError as exc:
        print(f"Cannot read firmware file: {exc}")
        sys.exit(1)

    # Wait for board
    port = wait_for_board(args.timeout)
    if port is None:
        print("Timed out — no board detected.")
        sys.exit(1)

    # Small delay to let the USB-serial adapter fully enumerate
    time.sleep(0.15)

    # Upload
    if not upload(port, firmware):
        print("Firmware upload FAILED.")
        sys.exit(1)

    print("Done.")


if __name__ == "__main__":
    main()
