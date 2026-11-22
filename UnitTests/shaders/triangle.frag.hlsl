// Triangle fragment stage: samples a texture with an explicit sampler.
//
// The set numbers are the Kaldi -> Vulkan mapping (see
// `KaldiVK/VkCommandExecutor.hpp`): set 0 holds sampled textures, set 1 holds
// samplers, both addressed by the portable slot number as the binding. Declaring
// them here keeps the shader assets and the backend contract in sync.
[[vk::binding(0, 0)]] Texture2D<float4> albedo : register(t0);
[[vk::binding(0, 1)]] SamplerState albedoSampler : register(s0);

struct VsOutput {
    float4 position : SV_Position;
    float3 color : COLOR;
};

float4 main(VsOutput input) : SV_Target {
    const float4 texel = albedo.Sample(albedoSampler, input.position.xy * 0.01);
    return float4(input.color, 1.0) * texel;
}
