Import("env")
import glob
import sys
import time
from serial import Serial

# Replaces PlatformIO's built-in use_1200bps_touch / wait_for_upload_port
# handling (disabled via board_upload.* in platformio.ini), which only waits
# 5s for the new bootloader port and silently falls back to the stale old
# port on timeout -- the likely cause of intermittent upload hangs/failures.

TOUCH_SETTLE_S = 0.4   # required by SAM-BA based boards after the 1200bps touch
PORT_WAIT_TIMEOUT_S = 10
PORT_SETTLE_S = 1.5    # let macOS finish attaching the new tty before bossac uses it


def candidate_ports():
    if sys.platform == "darwin":
        pattern = "/dev/cu.usbmodem*"
    else:
        pattern = "/dev/ttyACM*"
    return set(glob.glob(pattern))


def touch_1200bps(port):
    print(f"Forcing reset via 1200bps touch on {port}")
    try:
        s = Serial(port=port, baudrate=1200)
        s.setDTR(False)
        s.close()
    except Exception as e:
        sys.stderr.write(
            f"\nError: couldn't open {port} to trigger the bootloader reset ({e}).\n"
            "This usually means another program (a serial monitor, Arduino IDE, etc.)\n"
            "still has the port open. Close it and try uploading again.\n"
        )
        return False
    time.sleep(TOUCH_SETTLE_S)
    return True


def wait_for_bootloader(source, target, env):
    before = candidate_ports()

    if len(before) == 1:
        if not touch_1200bps(next(iter(before))):
            env.Exit(1)
    elif len(before) == 0:
        print("No board port detected before reset -- assuming it's already in bootloader mode")
    else:
        print(f"Multiple candidate ports found {before} -- skipping auto-touch, "
              "assuming the board is already in (or entering) bootloader mode")

    print("Waiting for bootloader port", end="", flush=True)
    start = time.time()
    new_port = None
    while time.time() - start < PORT_WAIT_TIMEOUT_S:
        new_ports = candidate_ports() - before
        if new_ports:
            new_port = sorted(new_ports)[0]
            break
        print(".", end="", flush=True)
        time.sleep(0.5)

    if not new_port:
        print("\nTimed out waiting for bootloader port -- is the board in bootloader mode?")
        return

    print(f"\nBootloader port found: {new_port}, letting it settle...")
    time.sleep(PORT_SETTLE_S)
    env.Replace(UPLOAD_PORT=new_port)
    print(f"Using upload port: {new_port}")


env.AddPreAction("upload", wait_for_bootloader)
