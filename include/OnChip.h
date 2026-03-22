#ifndef __ONCHIP_H__
#define __ONCHIP_H__

#include "GlobalAddress.h"
#include "Rdma.h"

struct RemoteConnection;
struct ThreadConnection;

// OnChip encapsulates RDMA operations that target on-chip (device) memory.
// On-chip memory is allocated directly on the RDMA NIC and is used exclusively
// for distributed locks in the Sherman system.
//
// Usage:
//   OnChip oc = dsm->get_onchip();
//   bool acquired = oc.cas_sync(lock_addr, 0, tag, buf);
class OnChip {
public:
  // On-chip device memory layout constants.
  // These aliases expose the canonical values from the define namespace
  // so callers only need to include OnChip.h.
  static constexpr uint64_t kStartAddr = define::kLockStartAddr;
  static constexpr uint64_t kSize = define::kLockChipMemSize;
  static constexpr uint64_t kNumOfLock = define::kNumOfLock;

  // Construct bound to the given per-thread connection and cluster metadata.
  //   iCon       - the calling thread's ThreadConnection (DSM::iCon)
  //   remoteInfo - array of per-node RemoteConnection descriptors
  OnChip(ThreadConnection *iCon, RemoteConnection *remoteInfo);

  // ---- Read ---------------------------------------------------------------

  void read(char *buffer, GlobalAddress gaddr, size_t size,
            bool signal = true, CoroContext *ctx = nullptr);
  void read_sync(char *buffer, GlobalAddress gaddr, size_t size,
                 CoroContext *ctx = nullptr);

  // ---- Write --------------------------------------------------------------

  void write(const char *buffer, GlobalAddress gaddr, size_t size,
             bool signal = true, CoroContext *ctx = nullptr);
  void write_sync(const char *buffer, GlobalAddress gaddr, size_t size,
                  CoroContext *ctx = nullptr);

  // ---- Compare-and-Swap ---------------------------------------------------

  void cas(GlobalAddress gaddr, uint64_t equal, uint64_t val,
           uint64_t *rdma_buffer, bool signal = true,
           CoroContext *ctx = nullptr);
  bool cas_sync(GlobalAddress gaddr, uint64_t equal, uint64_t val,
                uint64_t *rdma_buffer, CoroContext *ctx = nullptr);

  // Masked CAS — useful when only a subset of bits constitute the lock word.
  void cas_mask(GlobalAddress gaddr, uint64_t equal, uint64_t val,
                uint64_t *rdma_buffer, uint64_t mask = ~(0ull),
                bool signal = true);
  bool cas_mask_sync(GlobalAddress gaddr, uint64_t equal, uint64_t val,
                     uint64_t *rdma_buffer, uint64_t mask = ~(0ull));

  // ---- Fetch-and-Add ------------------------------------------------------

  // FAA whose result wraps at the boundary encoded in `mask`.
  void faa_boundary(GlobalAddress gaddr, uint64_t add_val,
                    uint64_t *rdma_buffer, uint64_t mask = 63,
                    bool signal = true, CoroContext *ctx = nullptr);
  void faa_boundary_sync(GlobalAddress gaddr, uint64_t add_val,
                         uint64_t *rdma_buffer, uint64_t mask = 63,
                         CoroContext *ctx = nullptr);

private:
  ThreadConnection *iCon;
  RemoteConnection *remoteInfo;
};

#endif /* __ONCHIP_H__ */
