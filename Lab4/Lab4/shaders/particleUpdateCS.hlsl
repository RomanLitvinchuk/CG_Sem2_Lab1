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

cbuffer TimeConstants : register(b0)
{
    float deltaTime;
    float3 padding;
};

RWStructuredBuffer<ParticleData> Particles : register(u0);
AppendStructuredBuffer<uint> DeadList : register(u1);


[numthreads(256, 1, 1)]
void UpdateCS( uint3 DTid : SV_DispatchThreadID )
{
    uint index = DTid.x;
    if (index >= 10000)
        return;

    if (Particles[index].isAlive == 1)
    {
        Particles[index].Position.xyz += Particles[index].Velocity * deltaTime;
        Particles[index].currentAge += deltaTime;
        if (Particles[index].currentAge >= Particles[index].deadAge)
        {
            Particles[index].isAlive = 0;
            DeadList.Append(index);
        }
    }
}