Texture2D t_PPTexture : register(t0);
SamplerState s_PointClamp : register(s0);

struct PS_INPUT
{
    float4 Pos : SV_Position;
    float2 Tex : TEXCOORD0;
};

float4 PS(PS_INPUT input) : SV_Target
{
    float4 color = t_PPTexture.Sample(s_PointClamp, input.Tex);

    float radius = 0.2;
    float softness = 0.45;

    float2 centerOffset = input.Tex - 0.5;
    
    float dist = length(centerOffset);

    float vig = smoothstep(radius + softness, radius, dist);

    return float4(color.rgb * vig, color.a);
}