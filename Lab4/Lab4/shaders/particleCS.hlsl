struct Particle
{
    float4 Position;
    float3 Velocity;
    float padding;
    float4 Color;
    float age;
    uint isAlive;
    float2 padding2;
};

cbuffer TimeConstants : register(b0)
{
    float deltaTime;
    float3 padding;
};

RWStructuredBuffer<Particle> Particles : register(u0);


[numthreads(256, 1, 1)]
void CS( uint3 DTid : SV_DispatchThreadID )
{
    uint index = DTid.x;
    Particles[index].Position.xyz += Particles[index].Velocity.xyz * deltaTime;
}