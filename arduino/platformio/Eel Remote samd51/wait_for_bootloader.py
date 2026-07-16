Import("env")
import glob
import sys
import time


def wait_for_bootloader(source, target, env):
    # Pattern for the bootloader port on each OS
    if sys.platform == "darwin":
        pattern = "/dev/cu.usbmodem*"
    else:
        pattern = "/dev/ttyACM*"

    # Snapshot ports that exist before the reset so we detect the new one
    before = set(glob.glob(pattern))

    print("\n>>> If the board is not already in bootloader mode: double-tap RESET now <<<")
    print("Waiting for bootloader port", end="", flush=True)

    timeout = 30  # seconds
    start = time.time()
    while time.time() - start < timeout:
        after = set(glob.glob(pattern))
        new_ports = after - before
        if new_ports:
            port = sorted(new_ports)[0]
            # Give the OS a moment to finish enumerating
            time.sleep(0.8)
            env.Replace(UPLOAD_PORT=port)
            print(f"\nBootloader port: {port}")
            return
        print(".", end="", flush=True)
        time.sleep(0.5)

    print("\nTimed out waiting for bootloader port — is the board in bootloader mode?")


env.AddPreAction("upload", wait_for_bootloader)
