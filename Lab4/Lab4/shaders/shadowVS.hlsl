cbuffer ShadowConstants : register(b0)
{
    row_major float4x4 lightViewProj;
    row_major float4x4 shadowTransform;
    float4 distances;
};

cbuffer cbMaterial : register(b1)
{
    
    float4x4 mMatTransform;
    
    float4 gDiffuseColor;
    float4 gAmbientColor;
    float4 gSpecularColor;
    float4 gEmissiveColor;
    float4 gTransparentColor;

    float gShininess;
    float gOpacity;
    float gRefractionIndex;
    int hasNormalTexture;
    
    int gIsLion;
    int gIsTree;
    int gDiffuseTextureIndex;
    int normalTextureIndex;
    
    int gDisplacementTextureIndex;
    int gHasDisplacementTexture;
    int gShadowTextureIndex;
    int gHasShadowTexture;
    
    int billboardTextureIndex;
    int hasbillboardTexture;
    int2 pad;
}

struct InstanceData
{
    row_major float4x4 mWorld;
    row_major float4x4 mTexTransform;
    row_major float4x4 mInvTWorld;
};

StructuredBuffer<InstanceData> Instances : register(t0);

Texture2D t_Shadow : register(t1);
SamplerState s_Sampler : register(s0);

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

float4 PS(VertexOut pin) : SV_Target
{
    if (gHasShadowTexture > 0)
    {
        float3 rainbowColor = t_Shadow.Sample(s_Sampler, pin.uv);
        return float4(rainbowColor, 1.0f);

    }
    return float4(0.0f, 0.0f, 0.0f, 1.0f);

}