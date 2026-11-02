"""Owned-reference and ordering barriers; physical reclamation is not inferred."""
import ctypes
import gc
import hashlib
from pathlib import Path
import weakref

from .measurement_engine import ProtocolError


class OwnedBuffer(bytearray):
    """Weak-referenceable raw input buffer for independently observing release."""


class RetentionBarrier:
    def __init__(self, owners, fence_path, fence_sha256):
        self.references = [weakref.ref(owner) for owner in owners]
        binary = Path(fence_path)
        if hashlib.sha256(binary.read_bytes()).hexdigest() != fence_sha256:
            raise ProtocolError("retention fence binary identity mismatch")
        self.library = ctypes.CDLL(str(binary.resolve()))
        self.fence = self.library.h04_retention_fence
        self.fence.argtypes, self.fence.restype = [], None
        self.binary_sha256 = fence_sha256

    def verify_release(self):
        gc.collect()
        self.fence()
        aliases = sum(reference() is not None for reference in self.references)
        evidence = {"live_owned_buffers": aliases, "ordering_fence_sha256": self.binary_sha256,
                    "ordering_fence_called": True, "allocator_pages_reclaimed": None,
                    "physical_bytes_erased": False, "cache_flushed": False,
                    "exact_RSS_enforcement": False}
        if aliases:
            raise ProtocolError("raw/mapped input aliases remain live")
        return evidence


def process_reclamation(returncode, reaped, cgroup_after=None):
    """Process reaping releases its address space; page caches/shared pages may remain."""
    dead = type(returncode) is int and reaped is True
    empty = cgroup_after is not None and cgroup_after.get("populated") == 0 and not cgroup_after.get("pids")
    return {"process_reaped": dead, "address_space_released": dead,
            "cgroup_empty": empty if cgroup_after is not None else None,
            "accounted_current_bytes": cgroup_after.get("accounted_current_bytes") if cgroup_after else None,
            "allocator_pages_reclaimed": None, "shared_or_cached_pages_erased": False,
            "physical_reclamation_verified": False}
