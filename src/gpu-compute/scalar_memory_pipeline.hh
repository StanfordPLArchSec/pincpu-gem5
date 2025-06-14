/*
 * Copyright (c) 2016-2017 Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __GPU_COMPUTE_SCALAR_MEMORY_PIPELINE_HH__
#define __GPU_COMPUTE_SCALAR_MEMORY_PIPELINE_HH__

#include <queue>
#include <string>

#include "gpu-compute/misc.hh"
#include "mem/request.hh"
#include "params/ComputeUnit.hh"
#include "sim/stats.hh"

/*
 * @file scalar_memory_pipeline.hh
 *
 * The scalar memory pipeline issues global memory packets
 * from the scalar ALU to the DTLB and L1 Scalar Data Cache.
 * The exec() method of the memory packet issues
 * the packet to the DTLB if there is space available in the return fifo.
 * This exec() method also retires previously issued loads and stores that have
 * returned from the memory sub-system.
 */

namespace gem5
{

class ComputeUnit;

class ScalarMemPipeline
{
  public:
    ScalarMemPipeline(const ComputeUnitParams &p, ComputeUnit &cu);
    void exec();

    std::queue<GPUDynInstPtr> &getGMReqFIFO() { return issuedRequests; }
    std::queue<GPUDynInstPtr> &getGMStRespFIFO() { return returnedStores; }
    std::queue<GPUDynInstPtr> &getGMLdRespFIFO() { return returnedLoads; }

    void issueRequest(GPUDynInstPtr gpuDynInst);

    void injectScalarMemFence(
            GPUDynInstPtr gpuDynInst, bool kernelMemSync, RequestPtr req);

    bool
    isGMLdRespFIFOWrRdy() const
    {
        return returnedLoads.size() < queueSize;
    }

    bool
    isGMStRespFIFOWrRdy() const
    {
        return returnedStores.size() < queueSize;
    }

    bool
    isGMReqFIFOWrRdy(uint32_t pendReqs=0) const
    {
        return (issuedRequests.size() + pendReqs) < queueSize;
    }

    const std::string& name() const { return _name; }

    void printProgress();

  private:
    ComputeUnit &computeUnit;
    const std::string _name;
    int queueSize;

    // Counters to track and limit the inflight scalar loads and stores
    // generated by this memory pipeline.
    int inflightStores;
    int inflightLoads;

    // Scalar Memory Request FIFO: all global memory scalar requests
    // are issued to this FIFO from the scalar memory pipelines
    std::queue<GPUDynInstPtr> issuedRequests;

    // Scalar Store Response FIFO: all responses of global memory
    // scalar stores are sent to this FIFO from L1 Scalar Data Cache
    std::queue<GPUDynInstPtr> returnedStores;

    // Scalar Load Response FIFO: all responses of global memory
    // scalar loads are sent to this FIFO from L1 Scalar Data Cache
    std::queue<GPUDynInstPtr> returnedLoads;
};

} // namespace gem5

#endif // __GPU_COMPUTE_SCALAR_MEMORY_PIPELINE_HH__
