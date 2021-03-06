/*
# Copyright (c) 2016, NVIDIA CORPORATION. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of NVIDIA CORPORATION nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef FRAMESOURCE_NVXCU_WRAPPER_HPP
#define FRAMESOURCE_NVXCU_WRAPPER_HPP

#include <memory>

#include <NVX/FrameSource.hpp>

#include "FrameSource/FrameSourceImpl.hpp"

namespace nvxio
{

class FrameSourceWrapper :
        public FrameSource
{
public:
    explicit FrameSourceWrapper(std::unique_ptr<nvidiaio::FrameSource> source);
    virtual bool open();
    virtual FrameSource::FrameStatus fetch(const nvxcu_pitch_linear_image_t & image, uint32_t timeout = 5u /*milliseconds*/);
    virtual FrameSource::FrameStatus convert(int w, int h, int c, char *data, int s, const nvxcu_pitch_linear_image_t & image, uint32_t timeout = 5u /*milliseconds*/);
    virtual FrameSource::Parameters getConfiguration();
    virtual bool setConfiguration(const FrameSource::Parameters& params);
    virtual void close();
    virtual ~FrameSourceWrapper();

private:
    std::unique_ptr<nvidiaio::FrameSource> source_;
    bool opened;
};

} // namespace nvxio

#endif // FRAMESOURCE_NVXCU_WRAPPER_HPP
