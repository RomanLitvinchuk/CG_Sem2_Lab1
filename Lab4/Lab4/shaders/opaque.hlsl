cbuffer cbPerObject : register(b0)
{
    float4x4 mViewProj;
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
    int hasNormalTexture;
    
    int gIsLion;
    int gIsTree;
    int gDiffuseTextureIndex;
    int normalTextureIndex;        
    
    int gDisplacementTextureIndex;
    int gHasDisplacementTexture;
    int2 padding;
}

Texture2D DiffuseMap : register(t0);
Texture2D NormalMap : register(t1);
Texture2D DisplacementMap : register(t2);

struct InstanceData
{
    row_major float4x4 mWorld;
    row_major float4x4 mTexTransform;
    row_major float4x4 mInvTWorld;
};

StructuredBuffer<InstanceData> Instances : register(t3);

SamplerState Sampler : register(s0);

struct VS_INPUT
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float3 tangent : TANGENT;
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float4 Wpos : WORLDPOS;
    float3 normal : NORMAL0;
    float3 normalW : NORMAL1;
    float2 uv : TEXCOORD;
    float3 tangentW : TANGENT;
};

//struct DS_OUTPUT
//{
    //float4 pos : SV_POSITION;
    //float3 normal : NORMAL0;
    //float3 normalW : NORMAL1;
    //float2 uv : TEXCOORD0;
    //float3 tangentW : TANGENT;
    //float3 worldPos : WORLDPOS;
//};


float3 NormalSampleToWorldSpace(float3 normalMapSample, float3 unitNormalW, float3 tangentW)
{
    float3 normalT = 2.0f * normalMapSample - 1.0f;

    float3 N = unitNormalW;
    float3 T = normalize(tangentW - dot(tangentW, N) * N);
    float3 B = cross(N, T);

    float3x3 TBN = float3x3(T, B, N);

    float3 bumpedNormalW = mul(normalT, TBN);

    return bumpedNormalW;
}


VS_OUTPUT VS(VS_INPUT input, uint instanceID : SV_InstanceID)
{
    VS_OUTPUT output;
    if (gIsLion == 1)
    {
        
        float offset = sin(gTime * 8.0f + input.pos.y * 0.7f) * 1.0f;
        input.pos += input.normal * offset;
    }
    
    float4x4 mWorld = Instances[instanceID].mWorld;
    float4x4 mTexTransform = Instances[instanceID].mTexTransform;
    
    
    float3 localPos = input.pos;
    float3 localNormal = input.normal;

    float4 texC = mul(float4(input.uv, 0.0f, 1.0f), mTexTransform);
    float2 finalUV = mul(texC, mMatTransform).xy;

    float3 normalW = mul(localNormal, (float3x3) Instances[instanceID].mInvTWorld);
    normalW = normalize(normalW);
    
    if (gHasDisplacementTexture == 1)
    {
        float height = DisplacementMap.SampleLevel(Sampler, finalUV, 0).r;

        height = height - 0.5f;

        float displacementStrength = 20.0f; 

        float3 worldPos = mul(float4(localPos, 1.0f), mWorld).xyz;

        worldPos += normalW * height * displacementStrength;

        output.Wpos = float4(worldPos, 1.0f);
        output.pos = mul(output.Wpos, mViewProj);
    }
    else
    {
        output.Wpos = mul(float4(localPos, 1.0f), mWorld);
        output.pos = mul(output.Wpos, mViewProj);
    }
    
    output.normal = localNormal;
    output.normalW = normalW;
    output.tangentW = mul(input.tangent, (float3x3)Instances[instanceID].mInvTWorld);
    output.uv = finalUV;
    return output;
}

struct gBufferOutput
{
    float4 Diffuse : SV_Target0;
    float4 Normal : SV_Target1;
};

gBufferOutput PS(VS_OUTPUT input)
{
    gBufferOutput ret;
    ret.Diffuse = DiffuseMap.Sample(Sampler, input.uv.xy);
    
    if (hasNormalTexture == 1)
    {
        input.normalW = normalize(input.normalW);
        float4 normalSample = NormalMap.Sample(Sampler, input.uv.xy);
        float3 bumpedNormalW = NormalSampleToWorldSpace(normalSample.xyz, input.normalW, input.tangentW);
        ret.Normal = float4(bumpedNormalW.x, bumpedNormalW.y, bumpedNormalW.z, 0.0f);
    }
    else
    {
        ret.Normal = float4(input.normal.x, input.normal.y, input.normal.z, 0.0f);
    }
    return ret;
}