cbuffer SsaoBuffer : register(b0)
{
    float screenWidth;
    float screenHeight;
    float randomTextureSize;
    float sampleRadius;
    float ssaoScale;
    float ssaoBias;
    float ssaoIntensity;
    float padding;
}

cbuffer CameraCB : register(b1)
{
    float4x4 g_InvViewProj;
    float3 g_CameraPos;
    float padding1;
};

cbuffer Matrices : register(b2)
{
    float4x4 View;
    float4x4 Proj;
    float4x4 invView;
    float4x4 invProj;
}

Texture2D t_Depth : register(t0);
Texture2D t_Normal : register(t1);
Texture2D t_Noise : register(t2);

SamplerState SampleTypeWrap : register(s0);
SamplerState SampleTypeClamp : register(s1);

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

float3 ReconstructWorldPos(float2 texCoord, float depth)
{
    float x = texCoord.x * 2.0f - 1.0f;
    float y = (1.0f - texCoord.y) * 2.0f - 1.0f;
    
    float4 ndcPos = float4(x, y, depth, 1.0f);
    float4 worldPos = mul(ndcPos, g_InvViewProj);
    
    return worldPos.xyz / worldPos.w;
}

float AOFunction(float2 texCoords, float2 uv, float3 position, float3 normal)
{
    float3 posVector;
    float3 vec;
    float distance;
    float occlusion;
    
    float depth = t_Depth.Sample(SampleTypeClamp, (texCoords + uv)).r;
    posVector = ReconstructWorldPos((texCoords + uv), depth);
    posVector = posVector - position;
    vec = normalize(posVector);
    distance = length(posVector) * ssaoScale;
    occlusion = max(0.0, dot(normal, vec) - ssaoBias) * (1.0f / (1.0f + distance)) * ssaoIntensity;
    
    return occlusion;
}

float PS(PS_INPUT input) : SV_Target
{
    float3 position;
    float3 normal;
    float2 texCoords;
    float2 randomVector;
    float2 vectorArray[4];
    float ambientOcclusion = 0.0f;
    float radius;
    int count;
    int i;
    float2 texCoord1;
    float2 texCoord2;
    
    float depth = t_Depth.Sample(SampleTypeClamp, input.TexCoord).r;
    position = ReconstructWorldPos(input.TexCoord, depth);
    position = mul(float4(position, 1.0f), View).xyz;
    normal = t_Normal.Sample(SampleTypeClamp, input.TexCoord);
    
    //normal = (normal * 2.0f) - 1.0f;
    normal = normalize(normal);
    
    texCoords.x = screenWidth / randomTextureSize;
    texCoords.y = screenHeight / randomTextureSize;
    texCoords = texCoords * input.TexCoord;
    
    randomVector = t_Noise.Sample(SampleTypeWrap, texCoords).xy;
    
    randomVector = (randomVector * 2.0f) - 1.0f;
    randomVector = normalize(randomVector);
    
    vectorArray[0] = float2(1.0f, 0.0f);
    vectorArray[1] = float2(-1.0f, 0.0f);
    vectorArray[2] = float2(0.0f, 1.0f);
    vectorArray[3] = float2(0.0f, -1.0f);
    
    float distToCam = length(position - g_CameraPos);
    //radius = sampleRadius / position.z;
    radius = sampleRadius / max(distToCam, 0.001f);
    
    count = 4;
    for (i = 0; i < count; i++)
    {
        texCoord1 = reflect(vectorArray[i], randomVector) * radius;
        texCoord2 = float2(((texCoord1.x * 0.7f) - (texCoord1.y * 0.7f)), ((texCoord1.x * 0.7f) + (texCoord1.y * 0.7f)));
        
        ambientOcclusion += AOFunction(input.TexCoord, (texCoord1 * 0.25f), position, normal);
        ambientOcclusion += AOFunction(input.TexCoord, (texCoord2 * 0.5f), position, normal);
        ambientOcclusion += AOFunction(input.TexCoord, (texCoord1 * 0.75f), position, normal);
        ambientOcclusion += AOFunction(input.TexCoord, (texCoord2 * 1.0f), position, normal);
    }
    
    ambientOcclusion = ambientOcclusion / ((float) count * 4.0f);
    ambientOcclusion = 1.0f - ambientOcclusion;
    
    return ambientOcclusion;

}