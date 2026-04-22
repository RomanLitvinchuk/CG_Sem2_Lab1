
cbuffer Matrices : register(b0)
{
    float4x4 View;
    float4x4 Proj;
}

struct ParticleData
{
    float4 Position;
    float4 Velocity;
};

StructuredBuffer<ParticleData> Particles : register(t0);

struct PixelInput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

struct PixelOutput
{
    float4 Color : SV_Target0;
};

PixelInput VS(uint instanceID : SV_InstanceID)
{
    PixelInput output;
    ParticleData particle = Particles[instanceID];
    float4 worldPos = particle.Position;
    float4 viewPos = mul(worldPos, View);
    output.position = viewPos;
    output.uv = 0;
    
    return output;
}

PixelOutput PS(PixelInput input)
{
    PixelOutput output;
    output.Color = float4(1.0f, 0.0f, 0.0f, 1.0f);

    return output;
}

