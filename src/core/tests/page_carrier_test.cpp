#include "reclaim/page_carrier.h"

#include <cassert>
#include <cstring>
#include <vector>

/* Host test for the unified reclaim-carrier interface. The two real backends are
 * Linux syscall code; this test links only the portable unit (geometry + the two
 * exported writes) with an in-memory stand-in for the transport, so the contract
 * both backends must honour is pinned without a device. */

using ghostlock::reclaim::CarrierGeometry;
using ghostlock::reclaim::CarrierKind;
using ghostlock::reclaim::CarrierStatus;
using ghostlock::reclaim::PageCarrier;
using ghostlock::reclaim::carrier_ok;
namespace reclaim = ghostlock::reclaim;

namespace {

class FakeCarrier final : public PageCarrier {
 public:
  explicit FakeCarrier(CarrierGeometry geometry) : PageCarrier(geometry) {}

  CarrierKind kind() const noexcept override { return CarrierKind::kUnixSkb; }
  const CarrierGeometry &geometry() const noexcept override { return geometry_; }

  CarrierStatus acquire() noexcept override {
    if (!reclaim::geometry_valid(geometry_)) {
      return CarrierStatus::kInvalid;
    }
    object_.assign(geometry_.capacity, 0);
    return CarrierStatus::kOk;
  }

  CarrierStatus commit() noexcept override {
    if (object_.size() != payload_bytes()) {
      return CarrierStatus::kInvalid;
    }
    std::memcpy(object_.data(), staging_.data(), payload_bytes());
    committed_ = true;
    return CarrierStatus::kOk;
  }

  CarrierStatus fetch() noexcept override {
    if (!committed_) {
      return CarrierStatus::kInvalid;
    }
    std::memcpy(staging_.data(), object_.data(), payload_bytes());
    return CarrierStatus::kOk;
  }

  CarrierStatus release() noexcept override {
    object_.clear();
    committed_ = false;
    return CarrierStatus::kOk;
  }

  /* The kernel side of the exchange: what the erase would have left behind. */
  std::vector<unsigned char> object_;
};

void fill_staging(PageCarrier &carrier, unsigned char value) {
  std::memset(carrier.staging(), value, carrier.payload_bytes());
}

uintptr_t read_word(const PageCarrier &carrier, size_t window_offset) {
  uintptr_t word = 0;
  std::memcpy(&word, carrier.staging() + window_offset, sizeof(word));
  return word;
}

void test_geometry() {
  static_assert(reclaim::kTargetPageBytes == 8192);
  static_assert(reclaim::kSlabObjectBytes == 0x180);
  static_assert(reclaim::kWordBytes == 8);

  /* arm64 5.15: SKB_MAX_HEAD(0) == PAGE_SIZE - 320, DATALEN_MSG == PAGE_SIZE - 0x30. */
  assert(reclaim::unix_skb_max_payload() == 3776);
  assert(reclaim::msg_msg_max_payload() == 4048);

  const CarrierGeometry unix = reclaim::unix_skb_geometry(3776);
  assert(unix.page_bytes == 4096);
  assert(unix.data_offset == 0);
  assert(unix.capacity == 3776);
  assert(reclaim::geometry_valid(unix));

  const CarrierGeometry msg = reclaim::msg_msg_geometry(4048);
  assert(msg.page_bytes == 4096);
  assert(msg.data_offset == 0x30);
  assert(msg.capacity == 4048);
  assert(reclaim::geometry_valid(msg));

  /* Out-of-range payloads are reported as capacity 0, never clamped silently. */
  assert(reclaim::unix_skb_geometry(3777).capacity == 0);
  assert(reclaim::unix_skb_geometry(0).capacity == 0);
  assert(reclaim::msg_msg_geometry(4049).capacity == 0);
  assert(reclaim::msg_msg_geometry(0).capacity == 0);

  assert(!reclaim::geometry_valid(CarrierGeometry{4096, 0, 0}));
  assert(!reclaim::geometry_valid(CarrierGeometry{4096, 4096, 1}));
  assert(!reclaim::geometry_valid(CarrierGeometry{4096, 0x30, 4096}));
}

void test_two_writes() {
  FakeCarrier unix(reclaim::unix_skb_geometry(3776));
  assert(carrier_ok(unix.acquire()));

  /* write_zero stores a NULL link. */
  fill_staging(unix, 0xaa);
  assert(carrier_ok(unix.write_zero(0x110)));
  assert(read_word(unix, 0x110) == 0);
  assert(unix.staging()[0x110 - 1] == 0xaa); /* exactly one word */

  /* write_pointer stores a kernel pointer link. */
  const uintptr_t pointer = 0xffffffc000123000ULL;
  assert(carrier_ok(unix.write_pointer(0x118, pointer)));
  assert(read_word(unix, 0x118) == pointer);

  /* Page offsets are checked against the window, not against the staging size. */
  assert(unix.write_zero(3776 - 8) == CarrierStatus::kOk);
  assert(unix.write_zero(3776 - 7) == CarrierStatus::kInvalid);
  assert(unix.write_pointer(3776, pointer) == CarrierStatus::kInvalid);

  /* The msg_msg window starts at 0x30: everything below it is the kernel header. */
  FakeCarrier msg(reclaim::msg_msg_geometry(4048));
  assert(carrier_ok(msg.acquire()));
  assert(msg.write_zero(0x30) == CarrierStatus::kOk);
  assert(msg.write_zero(0x28) == CarrierStatus::kInvalid);
  assert(msg.write_pointer(0x30 + 4048 - 8, pointer) == CarrierStatus::kOk);
  assert(msg.write_pointer(0x30 + 4048 - 7, pointer) == CarrierStatus::kInvalid);
}

void test_tiling() {
  /* A cross-cache carrier rarely starts at the page base, so the fake node is
   * replicated at every object boundary: one call writes every copy. */
  FakeCarrier unix(reclaim::unix_skb_geometry(3776));
  assert(carrier_ok(unix.acquire()));
  fill_staging(unix, 0x00);
  const uintptr_t pointer = 0xffffffc000200000ULL;
  assert(carrier_ok(unix.write_pointer(0x10, pointer, reclaim::kSlabObjectBytes)));
  for (size_t at = 0x10; at + 8 <= unix.payload_bytes(); at += reclaim::kSlabObjectBytes) {
    assert(read_word(unix, at) == pointer);
  }
  assert(read_word(unix, 0x18) == 0);
  assert(read_word(unix, 0x10 + 1) != pointer);

  /* A stride below one word would overlap copies; that is rejected. */
  assert(unix.write_zero(0x10, 4) == CarrierStatus::kInvalid);
}

void test_roundtrip_and_leak() {
  FakeCarrier carrier(reclaim::unix_skb_geometry(3776));
  assert(carrier_ok(carrier.acquire()));
  fill_staging(carrier, 0x5a);
  assert(carrier_ok(carrier.write_pointer(0x100, 0xffffffc000300000ULL)));
  assert(carrier_ok(carrier.commit()));

  /* fetch() without any kernel-side change returns the staged page. */
  std::memset(carrier.staging(), 0, carrier.payload_bytes());
  assert(carrier_ok(carrier.fetch()));
  assert(carrier.staging()[0] == 0x5a);
  assert(read_word(carrier, 0x100) == 0xffffffc000300000ULL);

  /* What the erase writes into the object is what fetch() hands back: this is the
   * leak oracle the stage after the erase uses. */
  const uintptr_t erased = 0xffffff8801234000ULL;
  std::memcpy(carrier.object_.data() + 0x108, &erased, sizeof(erased));
  assert(carrier_ok(carrier.fetch()));
  assert(read_word(carrier, 0x108) == erased);

  assert(carrier_ok(carrier.release()));
  assert(carrier.object_.empty());
  assert(carrier.fetch() == CarrierStatus::kInvalid); /* released object is not readable */
}

}  // namespace

int main() {
  test_geometry();
  test_two_writes();
  test_tiling();
  test_roundtrip_and_leak();
  return 0;
}
