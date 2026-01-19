import os
import sys
import time


def find_marker(path: str, marker: bytes) -> bool:
    if not os.path.exists(path):
        return False
    with open(path, "rb") as f:
        data = f.read()
    if marker in data:
        return True
    # Fallback: marker may be stored as UTF-16LE in some binaries
    marker_utf16 = b"".join(bytes((b, 0x00)) for b in marker)
    return marker_utf16 in data


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: verify_marker.py <build_dir> <marker>", file=sys.stderr)
        return 2

    build_dir = sys.argv[1]
    marker = sys.argv[2].encode("utf-8")

    zephyr_dir = os.path.join(build_dir, "zephyr")
    uf2_paths = [
        os.path.join(zephyr_dir, "zmk.uf2"),
        os.path.join(zephyr_dir, "zephyr.uf2"),
    ]
    elf_paths = [
        os.path.join(zephyr_dir, "zmk.elf"),
        os.path.join(zephyr_dir, "zephyr.elf"),
    ]
    hex_paths = [
        os.path.join(zephyr_dir, "zmk.hex"),
        os.path.join(zephyr_dir, "zephyr.hex"),
    ]

    def wait_for(paths, timeout_s=60):
        deadline = time.time() + timeout_s
        while time.time() < deadline:
            if any(os.path.exists(p) for p in paths):
                return
            time.sleep(0.1)

    wait_for(uf2_paths + elf_paths + hex_paths)

    for path in uf2_paths:
        if find_marker(path, marker):
            print(f"marker found in {path}")
            return 0

    for path in elf_paths:
        if find_marker(path, marker):
            print(f"marker found in {path} (UF2 missing marker)")
            return 1

    for path in hex_paths:
        if find_marker(path, marker):
            print(f"marker found in {path} (UF2 missing marker)")
            return 1

    print("marker not found in UF2 or ELF", file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
