#include "../hal/hal.hpp"
#include "../hal/virtualbox_guest_devices.hpp"
#include "canon_store.hpp"
#include "hosted_block_dev.hpp"
#include "net_packet.hpp"
#include "ttf.hpp"
#include "virtualbox_ahci_dev.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>

namespace {

t81::canonfs::CanonBlock make_block(uint8_t fill) {
  t81::canonfs::CanonBlock block;
  block.trytes.fill(fill);
  return block;
}

bool write_file(const std::string& path, const std::string& contents) {
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out) return false;
  out << contents;
  return out.good();
}

std::optional<std::string> build_phase4_report() {
  using namespace t81::ternaryos::dev;
  using namespace t81::ternaryos::hal;

  const std::string backing_path = "/tmp/axion_phase4_startup_snapshot.blk";
  std::filesystem::remove(backing_path);

  VBoxBootSpec spec;
  spec.ram_bytes = 128ULL * 1024ULL * 1024ULL;

  const auto stored_block = make_block(0x51);
  std::optional<t81::canonfs::CanonRef> stored_ref;

  {
    HostedBlockDev backing(32, "phase4-startup-ahci");
    backing.set_backing_file(backing_path);

    auto guest = bootstrap_virtualbox_guest(spec, backing);
    if (!guest.has_value()) return std::nullopt;
    if (hal_main(guest->boot_context) != 0) return std::nullopt;

    CanonStore store(*guest->storage.device);
    stored_ref = store.put(stored_block);
    if (!stored_ref.has_value()) return std::nullopt;
    if (!store.flush()) return std::nullopt;
  }

  auto loaded = HostedBlockDev::load(backing_path);
  if (!loaded.has_value()) return std::nullopt;

  auto guest = bootstrap_virtualbox_guest(spec, *loaded);
  if (!guest.has_value()) return std::nullopt;
  if (hal_main(guest->boot_context) != 0) return std::nullopt;

  CanonStore store(*guest->storage.device);
  const std::size_t recovered_entries = store.rebuild_index();
  const auto recovered_block = store.get(*stored_ref);

  auto& display = *guest->display.device;
  auto& framebuffer = display.framebuffer();
  framebuffer = TernaryFramebuffer(20, 8);
  const std::size_t rendered_glyphs = ttf_render_text(framebuffer, 0, 0, "AXION\nP4");
  if (!display.present()) return std::nullopt;

  auto packet = TernaryEthernetPacket::build(
      {0x01, 0x02, 0x03, 0x04, 0x05, 0x06},
      {0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F},
      0x0081,
      {1, 0, -1, -1, 1, 0});
  if (!packet.has_value()) return std::nullopt;

  auto frame = guest->network.device->send_packet(*packet);
  if (!frame.has_value()) return std::nullopt;
  if (!guest->network.device->inject_frame(*frame)) return std::nullopt;
  auto parsed_packet = guest->network.device->receive_packet();
  if (!parsed_packet.has_value()) return std::nullopt;

  auto* ahci = dynamic_cast<VirtualBoxAhciDev*>(guest->storage.device.get());
  if (ahci == nullptr) return std::nullopt;

  std::ostringstream report;
  report << "AXION_PHASE4_STARTUP\n";
  report << "profile=" << guest->profile_summary << "\n";
  report << "memory_map_len=" << guest->boot_context.memory_map.size() << "\n";
  report << "storage_binding=" << guest->storage.binding_name << "\n";
  report << "storage_device_id=" << ahci->info().device_id << "\n";
  report << "storage_total_blocks=" << ahci->block_count() << "\n";
  report << "storage_read_ops=" << ahci->read_ops() << "\n";
  report << "storage_write_ops=" << ahci->write_ops() << "\n";
  report << "storage_flush_ops=" << ahci->flush_ops() << "\n";
  report << "ahci_irq=" << static_cast<unsigned>(ahci->ahci_info().irq) << "\n";
  report << "canonstore_recovered_entries=" << recovered_entries << "\n";
  report << "canonstore_lookup=" << (recovered_block.has_value() ? "ok" : "missing") << "\n";
  report << "canonref=" << stored_ref->hash.h.to_string() << "\n";
  report << "display_binding=" << guest->display.binding_name << "\n";
  report << "display_present_count=" << display.present_count() << "\n";
  report << "display_rendered_glyphs=" << rendered_glyphs << "\n";
  report << "display_ascii_size=" << display.last_present_ascii().size() << "\n";
  report << "network_binding=" << guest->network.binding_name << "\n";
  report << "network_tx_frames=" << guest->network.device->tx_frames() << "\n";
  report << "network_rx_frames=" << guest->network.device->rx_frames() << "\n";
  report << "network_roundtrip_words=" << parsed_packet->trit_word_count() << "\n";

  std::filesystem::remove(backing_path);
  return report.str();
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 3) {
    std::fputs("usage: phase4_startup_snapshot <report-text> <report-header>\n", stderr);
    return 2;
  }

  const auto report = build_phase4_report();
  if (!report.has_value()) {
    std::fputs("failed to build phase4 startup report\n", stderr);
    return 1;
  }

  const std::string header =
      "#pragma once\n"
      "static const char kGeneratedPhase4Startup[] =\n"
      "R\"AXION_PHASE4(\n" + *report + ")AXION_PHASE4\";\n";

  if (!write_file(argv[1], *report)) {
    std::fputs("failed to write phase4 startup report text\n", stderr);
    return 1;
  }
  if (!write_file(argv[2], header)) {
    std::fputs("failed to write phase4 startup report header\n", stderr);
    return 1;
  }

  return 0;
}
