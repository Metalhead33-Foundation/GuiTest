// Triangle vertex stage: positions in clip space, colour passed to the fragment
// stage. `UnitTests.pro` compiles this file to SPIR-V with glslang at build time.
struct VsInput {
    float3 position : POSITION;
    float3 color : COLOR;
};

struct VsOutput {
    float4 position : SV_Position;
    float3 color : COLOR;
};

VsOutput main(VsInput input) {
    VsOutput output;
    output.position = float4(input.position, 1.0);
    output.color = input.color;
    return output;
}
