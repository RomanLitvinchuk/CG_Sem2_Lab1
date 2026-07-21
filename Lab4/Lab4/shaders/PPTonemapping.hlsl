Texture2D t_PPTexture : register(t0);
SamplerState s_PointClamp : register(s0);

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

PS_INPUT VS_FullScreenTriangle(uint VertexID : SV_VertexID)
{
    PS_INPUT output;
    
    output.TexCoord = float2((VertexID << 1) & 2, VertexID & 2);
    
    output.Pos = float4(output.TexCoord * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    
    return output;
}

float4 PS(PS_INPUT input) : SV_Target
{
    float4 finalColor = t_PPTexture.Sample(s_PointClamp, input.TexCoord);
    finalColor = finalColor / (finalColor + 1.0f);
    return finalColor;
}