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

struct SortParticle
{
    uint index;
    float distance;
};

cbuffer TimeConstants : register(b0)
{
    float3 cameraPos;
    float deltaTime;
};

RWStructuredBuffer<ParticleData> Particles : register(u0);
AppendStructuredBuffer<uint> DeadList : register(u1);
RWStructuredBuffer<SortParticle> SortList : register(u2);

[numthreads(256, 1, 1)]
void UpdateCS( uint3 DTid : SV_DispatchThreadID )
{
    uint index = DTid.x;
    if (index >= 16384)
        return;

    if (Particles[index].isAlive == 1)
    {
        Particles[index].currentAge += deltaTime;
        if (Particles[index].currentAge >= Particles[index].deadAge)
        {
            Particles[index].isAlive = 0;
            DeadList.Append(index);
            return;
        }
        Particles[index].Position.xyz += Particles[index].Velocity * deltaTime;
        uint sortIndex = SortList.IncrementCounter();
        SortParticle sp;
        sp.index = index;
        sp.distance = distance(Particles[index].Position.xyz, cameraPos);
        SortList[sortIndex] = sp;
    }
}