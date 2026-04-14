
cbuffer cbViewProj : register(b0)
{
    float4x4 ViewProj;
    float gTime;
    float pad[3];
};

struct InstanceData
{
    float3 center;
    float3 extents;
    float4 color;
};

StructuredBuffer<InstanceData> Instances : register(t0);

struct VertexIn
{
    float3 PosL : POSITION;
};

struct VertexOut
{
    float4 posH : SV_POSITION;
    float4 Color : COLOR;
};

VertexOut VS(VertexIn vin, uint instanceID : SV_InstanceID)
{
    VertexOut vout;
    InstanceData instance = Instances[instanceID];
    
    float3 worldPos = (vin.PosL * instance.extents * 2.0f) + instance.center;
    
    vout.posH = mul(float4(worldPos, 1.0f), ViewProj);
    
    vout.Color = instance.color;
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    return pin.Color;
}



