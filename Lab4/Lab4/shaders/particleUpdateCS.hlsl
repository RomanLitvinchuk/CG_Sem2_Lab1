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

cbuffer ParticlesConstants : register(b0)
{
    float3 cameraPos;
    float deltaTime;
    int particlesCount;
};

RWStructuredBuffer<ParticleData> Particles : register(u0);
AppendStructuredBuffer<uint> DeadList : register(u1);
RWStructuredBuffer<SortParticle> SortList : register(u2);

[numthreads(256, 1, 1)]
void UpdateCS( uint3 DTid : SV_DispatchThreadID )
{
    uint index = DTid.x;
    if (index >= particlesCount)
        return;
    
    ParticleData particle = Particles[index];
    
    if (particle.isAlive == 1)
    {
        particle.currentAge += deltaTime;
        if (particle.currentAge >= particle.deadAge)
        {
            particle.isAlive = 0;
            DeadList.Append(index);
            return;
        }
        particle.Position.xyz += particle.Velocity * deltaTime;
        Particles[index] = particle;
        uint sortIndex = SortList.IncrementCounter();
        SortParticle sp;
        sp.index = index;
        sp.distance = distance(particle.Position.xyz, cameraPos);
        SortList[sortIndex] = sp;
    }
}