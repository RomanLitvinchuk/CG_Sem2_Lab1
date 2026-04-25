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

RWStructuredBuffer<ParticleData> Particles : register(u0);
ConsumeStructuredBuffer<uint> DeadList : register(u1);

[numthreads(1, 1, 1)]
void EmitCS( uint3 DTid : SV_DispatchThreadID )
{
    uint index = DeadList.Consume();
    
    float seed1 = frac(sin(index * 12.9898) * 43758.5453);
    float seed2 = frac(sin(index * 78.233) * 43758.5453);
    float seed3 = frac(sin(index * 45.164) * 43758.5453);

    float3 spawnOffset = float3(seed1 - 300, 0.0, seed2 - 0.5) * 2.0;
    Particles[index].Position = float4(spawnOffset, 1.0);
    
    float r1 = frac(sin(index * 12.9898) * 43758.5453);
    float r2 = frac(sin(index * 78.233) * 43758.5453);
    Particles[index].Velocity = float3(r1 * 2 - 1, 50.0f, r2 * 2 - 1);
    
    Particles[index].Color = float4(1.0f, 0.0f, 0.0f, 1.0f);
    Particles[index].currentAge = 0.0f;
    Particles[index].deadAge = 10.0f + r1 * 2.0f;
    
    Particles[index].isAlive = 1;
}