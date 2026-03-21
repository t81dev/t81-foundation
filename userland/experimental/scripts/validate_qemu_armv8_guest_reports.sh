#!/bin/zsh
set -euo pipefail

if [[ $# -ne 3 ]]; then
  echo "usage: $0 <output-dir> <expected-boot-progress-state> <expected-validation-lane>" >&2
  exit 2
fi

output_dir=$1
expected_state=$2
expected_lane=$3

summary_file="$output_dir/qemu-armv8-guest-summary.txt"
boot_report_copy="$output_dir/boot-report.txt"
startup_status_copy="$output_dir/startup-status.txt"
startup_shell_copy="$output_dir/startup-shell.txt"
startup_session_copy="$output_dir/startup-session.txt"
startup_history_copy="$output_dir/startup-history.txt"
startup_store_copy="$output_dir/startup-store.txt"
startup_ref_copy="$output_dir/startup-ref.txt"
startup_report_copy="$output_dir/startup-report.txt"
startup_phase4_copy="$output_dir/startup-phase4.txt"

if [[ ! -f "$summary_file" || ! -f "$boot_report_copy" || ! -f "$startup_status_copy" || \
      ! -f "$startup_shell_copy" || ! -f "$startup_session_copy" || ! -f "$startup_history_copy" || \
      ! -f "$startup_store_copy" || ! -f "$startup_ref_copy" || ! -f "$startup_report_copy" || \
      ! -f "$startup_phase4_copy" ]]; then
  echo "QEMU ARMv8 guest report validation is missing one or more staged output files" >&2
  exit 1
fi

case "$expected_state" in
  ready)
    expected_pending=false
    expected_blocked=false
    ;;
  blocked)
    expected_pending=false
    expected_blocked=true
    ;;
  pending)
    expected_pending=true
    expected_blocked=false
    ;;
  *)
    echo "unsupported expected boot progress state: $expected_state" >&2
    exit 2
    ;;
esac

if ! /usr/bin/grep -q '^platform_id=virtualbox-armv8:ARMv8Virtual/developer-lane$' "$boot_report_copy"; then
  echo "QEMU ARMv8 guest probe found boot report, but platform_id did not match" >&2
  /bin/cat "$summary_file" >&2
  /bin/cat "$boot_report_copy" >&2
  exit 1
fi

if ! /usr/bin/grep -q '^hal_main_result=0$' "$boot_report_copy"; then
  echo "QEMU ARMv8 guest probe found boot report, but hal_main_result was not 0" >&2
  /bin/cat "$summary_file" >&2
  /bin/cat "$boot_report_copy" >&2
  exit 1
fi

for expected in \
  '^kernel_boot_ready_slice=complete$' \
  "^boot_progress_state=${expected_state}\$" \
  "^boot_progress_pending=${expected_pending}\$" \
  "^boot_progress_blocked=${expected_blocked}\$" \
  '^boot_progress_source=kernel-boot-critical-policy$' \
  "^boot_validation_lane=${expected_lane}\$"
do
  if ! /usr/bin/grep -q "$expected" "$boot_report_copy"; then
    echo "QEMU ARMv8 guest probe found boot report, but expected field was missing: $expected" >&2
    /bin/cat "$summary_file" >&2
    /bin/cat "$boot_report_copy" >&2
    exit 1
  fi
done

for expected in \
  '^os_name=Axion$' \
  '^phase=5$' \
  '^shell_mode=typed-builtins$' \
  '^kernel_boot_ready_slice=complete$' \
  "^boot_progress_pending=${expected_pending}\$" \
  "^boot_progress_blocked=${expected_blocked}\$" \
  "^boot_validation_lane=${expected_lane}\$" \
  '^storage_binding=virtualbox-ahci$' \
  '^display_binding=virtualbox-vmsvga$' \
  '^network_binding=virtualbox-e1000$'
do
  if ! /usr/bin/grep -q "$expected" "$startup_status_copy"; then
    echo "QEMU ARMv8 guest probe found startup status, but expected field was missing: $expected" >&2
    /bin/cat "$summary_file" >&2
    /bin/cat "$startup_status_copy" >&2
    exit 1
  fi
done

for expected in \
  '^AXION_PHASE4_STARTUP$' \
  '^storage_binding=virtualbox-ahci$' \
  '^canonstore_index_entries_per_block=17$' \
  '^canonstore_recovered_entries=20$' \
  '^canonstore_second_cycle_entries=20$' \
  '^canonstore_torn_header_entries=20$' \
  '^canonstore_inventory_count=20$' \
  '^canonstore_overflow_active=true$' \
  '^canonstore_lookup_ok=20$' \
  '^canonstore_second_cycle_ok=20$' \
  '^canonstore_torn_header_ok=20$' \
  '^display_binding=virtualbox-vmsvga$' \
  '^display_present_count=3$' \
  '^display_first_rendered_glyphs=7$' \
  '^display_second_rendered_glyphs=9$' \
  '^display_runtime_rendered_glyphs=8$' \
  '^display_changed=true$' \
  '^display_runtime_changed=true$' \
  '^network_binding=virtualbox-e1000$' \
  '^network_runtime_batches=2$' \
  '^network_tx_frames=5$' \
  '^network_rx_frames=5$' \
  '^network_pending_tx_frames=5$' \
  '^network_pending_rx_frames=0$' \
  '^network_roundtrip_ok=3$' \
  '^network_roundtrip_total=3$' \
  '^network_runtime_roundtrip_ok=2$' \
  '^network_runtime_roundtrip_total=2$' \
  '^network_roundtrip_words=10$' \
  '^network_total_frame_bytes=110$'
do
  if ! /usr/bin/grep -q "$expected" "$startup_phase4_copy"; then
    echo "QEMU ARMv8 guest probe found startup phase4 report, but expected field was missing: $expected" >&2
    /bin/cat "$summary_file" >&2
    /bin/cat "$startup_phase4_copy" >&2
    exit 1
  fi
done

for expected in \
  '^AXION_STARTUP_SHELL$' \
  '^prompt=axion> $' \
  '^mode=typed-builtins$' \
  '^history_anchor=durable$' \
  '^session_view=local+durable$'
do
  if ! /usr/bin/grep -q "$expected" "$startup_shell_copy"; then
    echo "QEMU ARMv8 guest probe found startup shell, but expected field was missing: $expected" >&2
    /bin/cat "$summary_file" >&2
    /bin/cat "$startup_shell_copy" >&2
    exit 1
  fi
done

if ! /usr/bin/grep -q '^commands=.*show session.*show ref <canonref>.*history show durable.*$' "$startup_shell_copy"; then
  echo "QEMU ARMv8 guest probe found startup shell, but command surface summary was incomplete" >&2
  /bin/cat "$summary_file" >&2
  /bin/cat "$startup_shell_copy" >&2
  exit 1
fi

for expected in \
  'AXION_STARTUP_SESSION' \
  'profile=VBoxEFI/AHCI/E1000/VMSVGA/HPET+IOAPIC' \
  'storage=virtualbox-ahci' \
  'display=virtualbox-vmsvga' \
  'session_command_count=6' \
  'durable_ref_count=1' \
  'durable_anchor=present'
do
  if ! /usr/bin/grep -F -x -q "$expected" "$startup_session_copy"; then
    echo "QEMU ARMv8 guest probe found startup session, but expected field was missing: $expected" >&2
    /bin/cat "$summary_file" >&2
    /bin/cat "$startup_session_copy" >&2
    exit 1
  fi
done

for expected in \
  'AXION_STARTUP_HISTORY' \
  'command=history show durable'
do
  if ! /usr/bin/grep -F -x -q "$expected" "$startup_history_copy"; then
    echo "QEMU ARMv8 guest probe found startup history, but expected field was missing: $expected" >&2
    /bin/cat "$summary_file" >&2
    /bin/cat "$startup_history_copy" >&2
    exit 1
  fi
done

if ! /usr/bin/grep -q '^result=history durable ' "$startup_history_copy"; then
  echo "QEMU ARMv8 guest probe found startup history, but durable history result prefix was malformed" >&2
  /bin/cat "$summary_file" >&2
  /bin/cat "$startup_history_copy" >&2
  exit 1
fi

if ! /usr/bin/grep -q '^phase5 durable transcript$' "$startup_history_copy"; then
  echo "QEMU ARMv8 guest probe found startup history, but durable payload text was missing" >&2
  /bin/cat "$summary_file" >&2
  /bin/cat "$startup_history_copy" >&2
  exit 1
fi

for expected in \
  'AXION_STARTUP_STORE' \
  'command=store ls'
do
  if ! /usr/bin/grep -F -x -q "$expected" "$startup_store_copy"; then
    echo "QEMU ARMv8 guest probe found startup store, but expected field was missing: $expected" >&2
    /bin/cat "$summary_file" >&2
    /bin/cat "$startup_store_copy" >&2
    exit 1
  fi
done

if ! /usr/bin/grep -q '^result=store refs 1' "$startup_store_copy"; then
  echo "QEMU ARMv8 guest probe found startup store, but store refs summary was malformed" >&2
  /bin/cat "$summary_file" >&2
  /bin/cat "$startup_store_copy" >&2
  exit 1
fi

for expected in \
  'AXION_STARTUP_REF' \
  'result=show ref '
do
  if ! /usr/bin/grep -F -q "$expected" "$startup_ref_copy"; then
    echo "QEMU ARMv8 guest probe found startup ref, but expected field was missing: $expected" >&2
    /bin/cat "$summary_file" >&2
    /bin/cat "$startup_ref_copy" >&2
    exit 1
  fi
done

if ! /usr/bin/grep -q '^command=show ref ' "$startup_ref_copy"; then
  echo "QEMU ARMv8 guest probe found startup ref, but command line was malformed" >&2
  /bin/cat "$summary_file" >&2
  /bin/cat "$startup_ref_copy" >&2
  exit 1
fi

if ! /usr/bin/grep -q '^phase5 durable transcript$' "$startup_ref_copy"; then
  echo "QEMU ARMv8 guest probe found startup ref, but durable payload text was missing" >&2
  /bin/cat "$summary_file" >&2
  /bin/cat "$startup_ref_copy" >&2
  exit 1
fi

for expected in \
  'AXION_STARTUP_REPORT' \
  '[session]' \
  '[shell]' \
  '[history]' \
  '[store]' \
  '[ref]'
do
  if ! /usr/bin/grep -F -x -q "$expected" "$startup_report_copy"; then
    echo "QEMU ARMv8 guest probe found startup report, but expected section was missing: $expected" >&2
    /bin/cat "$summary_file" >&2
    /bin/cat "$startup_report_copy" >&2
    exit 1
  fi
done

for expected in \
  'profile=VBoxEFI/AHCI/E1000/VMSVGA/HPET+IOAPIC' \
  'history_anchor=durable' \
  'command=store ls' \
  'command=history show durable'
do
  if ! /usr/bin/grep -F -q "$expected" "$startup_report_copy"; then
    echo "QEMU ARMv8 guest probe found startup report, but expected content was missing: $expected" >&2
    /bin/cat "$summary_file" >&2
    /bin/cat "$startup_report_copy" >&2
    exit 1
  fi
done

echo "QEMU ARMv8 guest report validation succeeded for boot state '$expected_state'."
