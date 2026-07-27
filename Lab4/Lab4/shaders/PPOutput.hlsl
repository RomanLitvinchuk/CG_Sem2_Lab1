Texture2D t_PPTexture : register(t0);
SamplerState s_PointClamp : register(s0);

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD0;
};

float4 PS(PS_INPUT input) : SV_Target
{
    float4 finalColor = t_PPTexture.Sample(s_PointClamp, input.Tex);
    return finalColor;
}