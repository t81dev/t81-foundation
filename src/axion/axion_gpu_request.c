/*
 Axion GPU Dispatch Serializer with Context-Aware Promotion (/sys/axion_debug/gpu_request)
   This module serializes a GaiaRequest for GPU dispatch. It reads a TBIN file, sets up
   a GaiaRequest with intent, confidence, and TBIN macro data, and writes it to a sysfs entry.
   Enhancements:
   - Verbose logging (via VERBOSE_GPU_REQ flag)
   - Error checking on write operations.
   - Optional reading of a GPU response from sysfs (stub for future integration).
   - Hooks for JSON/CBOR parsing and audit logging.
*/
/*
 Integration Notes:
   This module assumes the Axion kernel module has mounted a writable sysfs entry at:
       /sys/axion_debug/gpu_request
   and (optionally) a response entry at:
       /sys/axion_debug/gpu_result

   The pseudo-file is read by the kernel module, parsed as a GaiaRequest, and relayed
   to the appropriate GPU backend (via CUDA or ROCm) using the logic in the backend handlers.
   Future enhancements include JSON/CBOR parsing and integration with audit and AI refinement logic.
/*


