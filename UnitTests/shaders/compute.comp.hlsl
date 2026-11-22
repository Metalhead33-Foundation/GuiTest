// Compute stage: writes one value per invocation into a structured buffer.
//
// Set 3 is Kaldi's storage-buffer set (`BindStorageBuffer`), again with the
// portable slot number as the binding.
[[vk::binding(0, 3)]] RWStructuredBuffer<float4> output : register(u0);

[numthreads(64, 1, 1)]
void main(uint3 id : SV_DispatchThreadID) {
    output[id.x] = float4(float(id.x), 0.0, 0.0, 1.0);
}
