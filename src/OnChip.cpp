#include "OnChip.h"

#include "Common.h"
#include "Connection.h"
#include "ThreadConnection.h"

OnChip::OnChip(ThreadConnection *iCon, RemoteConnection *remoteInfo)
    : iCon(iCon), remoteInfo(remoteInfo) {}

// ---- Read ------------------------------------------------------------------

void OnChip::read(char *buffer, GlobalAddress gaddr, size_t size, bool signal,
                  CoroContext *ctx) {
  if (ctx == nullptr) {
    rdmaRead(iCon->data[0][gaddr.nodeID], (uint64_t)buffer,
             remoteInfo[gaddr.nodeID].lockBase + gaddr.offset, size,
             iCon->cacheLKey, remoteInfo[gaddr.nodeID].lockRKey[0], signal);
  } else {
    rdmaRead(iCon->data[0][gaddr.nodeID], (uint64_t)buffer,
             remoteInfo[gaddr.nodeID].lockBase + gaddr.offset, size,
             iCon->cacheLKey, remoteInfo[gaddr.nodeID].lockRKey[0], true,
             ctx->coro_id);
    (*ctx->yield)(*ctx->master);
  }
}

void OnChip::read_sync(char *buffer, GlobalAddress gaddr, size_t size,
                       CoroContext *ctx) {
  read(buffer, gaddr, size, true, ctx);
  if (ctx == nullptr) {
    ibv_wc wc;
    pollWithCQ(iCon->cq, 1, &wc);
  }
}

// ---- Write -----------------------------------------------------------------

void OnChip::write(const char *buffer, GlobalAddress gaddr, size_t size,
                   bool signal, CoroContext *ctx) {
  if (ctx == nullptr) {
    rdmaWrite(iCon->data[0][gaddr.nodeID], (uint64_t)buffer,
              remoteInfo[gaddr.nodeID].lockBase + gaddr.offset, size,
              iCon->cacheLKey, remoteInfo[gaddr.nodeID].lockRKey[0], -1,
              signal);
  } else {
    rdmaWrite(iCon->data[0][gaddr.nodeID], (uint64_t)buffer,
              remoteInfo[gaddr.nodeID].lockBase + gaddr.offset, size,
              iCon->cacheLKey, remoteInfo[gaddr.nodeID].lockRKey[0], -1, true,
              ctx->coro_id);
    (*ctx->yield)(*ctx->master);
  }
}

void OnChip::write_sync(const char *buffer, GlobalAddress gaddr, size_t size,
                        CoroContext *ctx) {
  write(buffer, gaddr, size, true, ctx);
  if (ctx == nullptr) {
    ibv_wc wc;
    pollWithCQ(iCon->cq, 1, &wc);
  }
}

// ---- Compare-and-Swap ------------------------------------------------------

void OnChip::cas(GlobalAddress gaddr, uint64_t equal, uint64_t val,
                 uint64_t *rdma_buffer, bool signal, CoroContext *ctx) {
  if (ctx == nullptr) {
    rdmaCompareAndSwap(iCon->data[0][gaddr.nodeID], (uint64_t)rdma_buffer,
                       remoteInfo[gaddr.nodeID].lockBase + gaddr.offset, equal,
                       val, iCon->cacheLKey,
                       remoteInfo[gaddr.nodeID].lockRKey[0], signal);
  } else {
    rdmaCompareAndSwap(iCon->data[0][gaddr.nodeID], (uint64_t)rdma_buffer,
                       remoteInfo[gaddr.nodeID].lockBase + gaddr.offset, equal,
                       val, iCon->cacheLKey,
                       remoteInfo[gaddr.nodeID].lockRKey[0], true,
                       ctx->coro_id);
    (*ctx->yield)(*ctx->master);
  }
}

bool OnChip::cas_sync(GlobalAddress gaddr, uint64_t equal, uint64_t val,
                      uint64_t *rdma_buffer, CoroContext *ctx) {
  cas(gaddr, equal, val, rdma_buffer, true, ctx);
  if (ctx == nullptr) {
    ibv_wc wc;
    pollWithCQ(iCon->cq, 1, &wc);
  }
  return equal == *rdma_buffer;
}

void OnChip::cas_mask(GlobalAddress gaddr, uint64_t equal, uint64_t val,
                      uint64_t *rdma_buffer, uint64_t mask, bool signal) {
  rdmaCompareAndSwapMask(iCon->data[0][gaddr.nodeID], (uint64_t)rdma_buffer,
                         remoteInfo[gaddr.nodeID].lockBase + gaddr.offset,
                         equal, val, iCon->cacheLKey,
                         remoteInfo[gaddr.nodeID].lockRKey[0], mask, signal);
}

bool OnChip::cas_mask_sync(GlobalAddress gaddr, uint64_t equal, uint64_t val,
                           uint64_t *rdma_buffer, uint64_t mask) {
  cas_mask(gaddr, equal, val, rdma_buffer, mask);
  ibv_wc wc;
  pollWithCQ(iCon->cq, 1, &wc);
  return (equal & mask) == (*rdma_buffer & mask);
}

// ---- Fetch-and-Add ---------------------------------------------------------

void OnChip::faa_boundary(GlobalAddress gaddr, uint64_t add_val,
                          uint64_t *rdma_buffer, uint64_t mask, bool signal,
                          CoroContext *ctx) {
  if (ctx == nullptr) {
    rdmaFetchAndAddBoundary(iCon->data[0][gaddr.nodeID], (uint64_t)rdma_buffer,
                            remoteInfo[gaddr.nodeID].lockBase + gaddr.offset,
                            add_val, iCon->cacheLKey,
                            remoteInfo[gaddr.nodeID].lockRKey[0], mask, signal);
  } else {
    rdmaFetchAndAddBoundary(iCon->data[0][gaddr.nodeID], (uint64_t)rdma_buffer,
                            remoteInfo[gaddr.nodeID].lockBase + gaddr.offset,
                            add_val, iCon->cacheLKey,
                            remoteInfo[gaddr.nodeID].lockRKey[0], mask, true,
                            ctx->coro_id);
    (*ctx->yield)(*ctx->master);
  }
}

void OnChip::faa_boundary_sync(GlobalAddress gaddr, uint64_t add_val,
                               uint64_t *rdma_buffer, uint64_t mask,
                               CoroContext *ctx) {
  faa_boundary(gaddr, add_val, rdma_buffer, mask, true, ctx);
  if (ctx == nullptr) {
    ibv_wc wc;
    pollWithCQ(iCon->cq, 1, &wc);
  }
}
