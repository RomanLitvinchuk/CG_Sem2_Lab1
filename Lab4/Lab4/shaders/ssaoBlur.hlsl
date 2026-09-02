cbuffer ScreenBuffer : register(b0)
{
    float screenWidth;
    float screenHeight;
    float blurType;
    float padding;
};

SamplerState SampleTypePoint : register(s0);

Texture2D t_SSAO : register(t0);
Texture2D t_Normal : register(t1);

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

float PS(PS_INPUT input) : SV_Target
{
    float texelSize;
    float2 texOffset;
    int radius = 5;
    float colorSum;
    float weightSum;
    float3 centerNormal;
    float2 tex;
    float3 neighborNormal;
    float ssaoValue;
    float weight;
    
    if (blurType < 0.1f)
    {
        texelSize = 1.0f / screenWidth;
        texOffset = float2(texelSize, 0.0f);
    }
    else
    {
        texelSize = 1.0f / screenHeight;
        texOffset = float2(0.0f, texelSize);
    }

    float weightArray[11] = { 0.05f, 0.05f, 0.1f, 0.1f, 0.1f, 0.2f, 0.1f, 0.1f, 0.1f, 0.05f, 0.05f };
    weightSum = weightArray[radius];
    colorSum = weightSum * t_SSAO.Sample(SampleTypePoint, input.TexCoord, 0).r;
    centerNormal = t_Normal.Sample(SampleTypePoint, input.TexCoord, 0);
    for (int i = -radius; i <= radius; i++)
    {
        if (i == 0)
            continue;
        tex = input.TexCoord + (i * texOffset);
        neighborNormal = t_Normal.Sample(SampleTypePoint, tex, 0);
        if (dot(neighborNormal, centerNormal) >= 0.8f)
        {
            ssaoValue = t_SSAO.Sample(SampleTypePoint, tex, 0);
            weight = weightArray[radius + i];
            colorSum += (ssaoValue * weight);
            weightSum += weight;

        }

    }
    colorSum = colorSum / weightSum;
    return colorSum;
}