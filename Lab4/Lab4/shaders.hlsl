cbuffer cbPerObject : register(b0)
{
    float4x4 mWorldViewProj;
    float4x4 mTexTransform;
    float gTime;
    float pad[3];
}

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
    int gHasDiffuseTexture;
    
    int gIsLion;
    int gDiffuseTextureIndex;
    int3 gMaterialPad;
    
}

Texture2D DiffuseMap : register(t0);
SamplerState Sampler : register(s0);

struct VS_INPUT
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output;
    if (gIsLion == 1)
    {
        
        float offset = sin(gTime * 8.0f + input.pos.y * 0.7f) * 1.0f;
        input.pos += input.normal * offset;
    }
    output.pos = mul(float4(input.pos, 1.0f), mWorldViewProj);
    output.normal = input.normal;
    float4 texC = mul(float4(input.uv, 0.0f, 1.0f), mTexTransform);
    output.uv = mul(texC, mMatTransform).xy;
    return output;
}

float4 PS(VS_OUTPUT input) : SV_TARGET
{
    float4 color = DiffuseMap.Sample(Sampler, input.uv.xy);
    return color;
}