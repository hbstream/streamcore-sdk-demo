"""Reference worker for a local algorithm used by the Qt Demo.

The C++ ProcessorWorkerBridge starts one persistent process. stdin carries
repeated little-endian ``SCF1, width, height, byte_count, BGRA pixels``
requests; stdout carries ``SCO1, byte_count, BGRA pixels`` responses. Each
integer is unsigned 32-bit. The output must keep the input dimensions and
BGRA layout. stdout is binary-only; write errors and diagnostics to stderr.

The selected module implements ``create_processor() -> callable``. Its factory
may load a model once; the returned callable receives a NumPy uint8 array of
shape (height, width, 4) and returns another array of that shape and type.
The SDK callback, frame ownership, deadlines, and optional publishing remain
in ProcessorFrameAdapter; this script has no StreamCore SDK dependency.
"""

from __future__ import annotations

import importlib.util
import json
import os
from pathlib import Path
import struct
import sys

import numpy as np

# Embedded Python's isolated ._pth does not add the script directory itself.
sys.path.insert(0, str(Path(__file__).resolve().parent))


MAX_FRAME_BYTES = 16 * 1024 * 1024


def load_processor(probe_only: bool = False):
    """Load the module named by the host and construct one processor.

    A module may define ``probe_processor()`` for a cheap dependency check.
    The regular worker calls ``create_processor()`` only once at startup,
    then reuses its return value for every frame.
    """
    plugin_text = os.environ.get("STREAMCORE_DEMO_QT_PROCESSOR_MODULE", "").strip()
    if plugin_text:
        plugin_path = Path(plugin_text).resolve()
        if not plugin_path.is_file():
            raise FileNotFoundError(f"Processor module does not exist: {plugin_path}")
        sys.path.insert(0, str(plugin_path.parent))
        spec = importlib.util.spec_from_file_location("streamcore_demo_processor_plugin", plugin_path)
        if spec is None or spec.loader is None:
            raise RuntimeError(f"Cannot load Processor module: {plugin_path}")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        factory = getattr(module, "create_processor", None)
        if not callable(factory):
            raise TypeError("Processor module must define create_processor()")
        probe = getattr(module, "probe_processor", None)
        if probe_only and callable(probe):
            probe()
            return None, plugin_path.name
        processor = factory()
        if not callable(processor):
            raise TypeError("create_processor() must return a callable")
        return processor, plugin_path.name

    raise ValueError("Select an external Processor module before starting")


def read_exact(count: int) -> bytes:
    """Read one complete protocol field despite short pipe reads or EOF."""
    chunks = []
    while count:
        chunk = sys.stdin.buffer.read(count)
        if not chunk:
            raise EOFError("frame worker input ended")
        chunks.append(chunk)
        count -= len(chunk)
    return b"".join(chunks)


def main() -> int:
    probe_only = "--probe" in sys.argv[1:]
    processor, algorithm = load_processor(probe_only)
    if probe_only:
        print(json.dumps({"ready": True, "algorithm": algorithm}))
        return 0

    while True:
        # A persistent process avoids importing Python packages and loading
        # model weights for every video frame. Never emit a partial response.
        first = sys.stdin.buffer.read(1)
        if not first:
            return 0
        header = first + read_exact(15)
        magic, width, height, length = struct.unpack("<4sIII", header)
        if magic != b"SCF1" or width == 0 or height == 0 or length != width * height * 4 or length > MAX_FRAME_BYTES:
            raise ValueError("invalid BGRA frame header")
        bgra = np.frombuffer(read_exact(length), dtype=np.uint8).reshape((height, width, 4))
        output = processor(bgra)
        if not isinstance(output, np.ndarray) or output.shape != bgra.shape or output.dtype != np.uint8:
            raise ValueError("Processor must return uint8 BGRA with the input shape")
        payload = np.ascontiguousarray(output).tobytes()
        sys.stdout.buffer.write(struct.pack("<4sI", b"SCO1", len(payload)))
        sys.stdout.buffer.write(payload)
        sys.stdout.buffer.flush()


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print(f"frame worker failed: {error}", file=sys.stderr)
        raise SystemExit(1)
