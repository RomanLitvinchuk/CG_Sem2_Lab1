cbuffer ShadowConstants : register(b0)
{
    row_major float4x4 lightViewProj;
    row_major float4x4 shadowTransform;
    float4 distances;
};

struct InstanceData
{
    row_major float4x4 mWorld;
    row_major float4x4 mTexTransform;
    row_major float4x4 mInvTWorld;
};

StructuredBuffer<InstanceData> Instances : register(t0);

struct VertexIn
{
    float3 posL : POSITION;
    float2 uv : TEXCOORD0;
};

struct VertexOut
{
    float4 posH : SV_Position;
    float2 uv : TEXCOORD0;
};

VertexOut VS(VertexIn vin, uint instanceID : SV_InstanceID)
{
    VertexOut vout;
    float4x4 mWorld = Instances[instanceID].mWorld;
    float4 posW = mul(float4(vin.posL, 1.0f), mWorld);
    vout.posH = mul(posW, lightViewProj);
    vout.uv = vin.uv;
    return vout;
}