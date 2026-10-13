"""Verify the WAVE75 vendor archive used by the indicator call-site whitelist."""
import hashlib
import sys
from pathlib import Path

EXPECTED = "cea2276ae9d5b092f81536f7bce68bbdcc37da0c1b3d0f1d8f665a762294f94b"

try:
    actual = hashlib.sha256(Path(sys.argv[1]).read_bytes()).hexdigest()
except (OSError, IndexError) as exc:
    print(f"Cannot verify indicator library: {exc}", file=sys.stderr)
    sys.exit(1)
if actual != EXPECTED:
    print("Vendor library changed: re-audit indicator call offsets before building.", file=sys.stderr)
    sys.exit(1)
print("verified")
