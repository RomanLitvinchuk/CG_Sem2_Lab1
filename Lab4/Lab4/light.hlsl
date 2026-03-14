cbuffer CameraCB : register(b0)
{
    float4x4 g_InvViewProj;
    float3 g_CameraPos;
    float padding1;
};

cbuffer LightCB : register(b1)
{
    float3 g_LightColor;
    int g_LightType; // 0 = Directional, 1 = Point, 2 = Spot
    
    float3 g_LightPosition; 
    float g_LightRange;
    
    float3 g_LightDirection; 
    float g_SpotCosInner; 
    
    float g_SpotCosOuter;
    float3 padding2;
};

Texture2D t_Diffuse : register(t0);
Texture2D t_Normal : register(t1);
Texture2D t_Depth : register(t2);

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

float3 ReconstructWorldPos(float2 texCoord, float depth)
{
    float x = texCoord.x * 2.0f - 1.0f;
    float y = (1.0f - texCoord.y) * 2.0f - 1.0f;
    
    float4 ndcPos = float4(x, y, depth, 1.0f);
    float4 worldPos = mul(ndcPos, g_InvViewProj);
    
    return worldPos.xyz / worldPos.w;
}

float4 PS_DeferredLighting(PS_INPUT input) : SV_Target
{
    float depth = t_Depth.Sample(s_PointClamp, input.TexCoord).r;
    
    if (depth >= 1.0f)
        discard;

    float3 diffuse = t_Diffuse.Sample(s_PointClamp, input.TexCoord).rgb;
    
    float3 normal = t_Normal.Sample(s_PointClamp, input.TexCoord).xyz;
    normal = normalize(normal * 2.0f - 1.0f);

    float3 worldPos = ReconstructWorldPos(input.TexCoord, depth);
    float3 viewDir = normalize(g_CameraPos - worldPos);

    float3 finalLight = float3(0.0f, 0.0f, 0.0f);

    if (g_LightType == 0)
    {
        float3 lightDir = normalize(-g_LightDirection);
        float NdotL = max(dot(normal, lightDir), 0.0f);
        
        finalLight = diffuse * g_LightColor * NdotL;
    }
    else if (g_LightType == 1)
    {
        float3 lightVec = g_LightPosition - worldPos;
        float distance = length(lightVec);
        
        if (distance < g_LightRange)
        {
            float3 lightDir = lightVec / distance;
            
            float attenuation = max(0.0f, 1.0f - (distance / g_LightRange));
            attenuation = attenuation * attenuation;
            
            float NdotL = max(dot(normal, lightDir), 0.0f);
            
            finalLight = diffuse * g_LightColor * NdotL * attenuation;
        }
    }
    else if (g_LightType == 2)
    {
        float3 lightVec = g_LightPosition - worldPos;
        float distance = length(lightVec);
        
        if (distance < g_LightRange)
        {
            float3 lightDir = lightVec / distance;
            
            float attenuation = max(0.0f, 1.0f - (distance / g_LightRange));
            attenuation = attenuation * attenuation;
            
            float theta = dot(lightDir, normalize(-g_LightDirection));
            float spotIntensity = smoothstep(g_SpotCosOuter, g_SpotCosInner, theta);
            
            float NdotL = max(dot(normal, lightDir), 0.0f);
            
            finalLight = diffuse * g_LightColor * NdotL * attenuation * spotIntensity;
        }
    }

    return float4(finalLight, 1.0f);
}