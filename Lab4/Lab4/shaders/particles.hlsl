
cbuffer Matrices : register(b0)
{
    float4x4 View;
    float4x4 Proj;
}

struct ParticleData
{
    float4 Position;
    float3 Velocity;
    float padding;
    float4 Color;
    float currentAge;
    float deadAge;
    uint isAlive;
    float padding2;
};

StructuredBuffer<ParticleData> Particles : register(t0);

struct PixelInput
{
    float4 color : COLOR0;
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    uint isAlive : BLENDINDICES0;
    float currentAge : AGE0;
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
    output.color = particle.Color;
    output.isAlive = particle.isAlive;
    output.currentAge = particle.currentAge;
    
    return output;
}

PixelOutput PS(PixelInput input)
{
    PixelOutput output;
    output.Color = input.color + float4(0.0f, input.currentAge / 10.0f , input.currentAge / 10.0f, 0.0f);
    return output;
}

