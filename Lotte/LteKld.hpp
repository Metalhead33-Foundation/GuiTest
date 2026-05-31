#ifndef LTEKLD_HPP
#define LTEKLD_HPP
#include <Kaldi/KldCommandBuffer.hpp>
#include <memory_resource>

namespace Lotte {

typedef std::pmr::polymorphic_allocator<Kld::GfxOp> OpAllocator;
typedef std::pmr::polymorphic_allocator<Kld::HandleId> HandleAllocator;
typedef Kld::CommandDispatcher<HandleAllocator,OpAllocator> CommandBuffer;
typedef Kld::oop_detail<HandleAllocator,OpAllocator> KldOopDetail;
typedef KldOopDetail::Buffer Buffer;
typedef KldOopDetail::Framebuffer Framebuffer;
typedef KldOopDetail::Fence Fence;
typedef KldOopDetail::Image Image;
typedef KldOopDetail::Pipeline Pipeline;
typedef KldOopDetail::ComputePipeline ComputePipeline;
typedef KldOopDetail::QueryPool QueryPool;
typedef KldOopDetail::Sampler Sampler;
typedef KldOopDetail::Texture Texture;

}

#endif // LTEKLD_HPP
