
cbuffer Matrices : register(b0)
{
    row_major float4x4 View;
    row_major float4x4 Proj;
}

struct ParticleData
{
    float4 Position;
    float4 Velocity;
};

StructuredBuffer<ParticleData> Particles : register(t0);

struct VertexInput
{
    uint vertexID : SV_VertexID;
};

struct PixelInput
{
    float4 position : SV_Position;
};

struct PixelOutput
{
    float4 Color : SV_Target0;
};

PixelInput VS(VertexInput vertex)
{
    PixelInput output;
    ParticleData particle = Particles[vertex.vertexID];
    float4 worldPos = particle.Position;
    float4 viewPos = mul(worldPos, View);
    output.position = mul(viewPos, Proj);
    
    return output;
}

PixelOutput PS(PixelInput input)
{
    PixelOutput output;
    output.Color = float4(1.0f, 0.0f, 0.0f, 1.0f);

    return output;
}

