cbuffer cbPerObject : register(b0)
{
    float4x4 mWorldViewProj;
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
    output.pos = mul(float4(input.pos, 1.0f), mWorldViewProj);
    output.normal = input.normal;
    output.uv = input.uv;
    return output;
}

float4 PS(VS_OUTPUT input) : SV_TARGET
{
    float4 color = DiffuseMap.Sample(Sampler, input.uv.xy);
    return color;
    //return DiffuseMap.Load(int3(100, 100, 0));
}