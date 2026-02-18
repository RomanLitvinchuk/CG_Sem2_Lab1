cbuffer cbPerObject : register(b0)
{
    float4x4 mWorldViewProj;
};
//
struct VSInput
{
    float3 Pos : POSITION;
    float3 Normal : NORMAL;
    float2 uv : TEXCOORD;
};

struct PSInput
{
    float3 Pos : POSITION;
    float3 Normal : NORMAL;
    float2 uv : TEXCOORD;
};

PSInput VS(VSInput vin)
{
    PSInput vout;
    vout.Pos = mul(float4(vin.Pos, 1.0f), mWorldViewProj);
    //vout.Color = vin.Color;
    return vout;
}

float4 PS(PSInput pin) : SV_TARGET
{
    return float4(0.0f, 0.0f, 0.0f, 0.0f);
}