cbuffer CameraCB : register(b0)
{
    float4x4 g_InvViewProj;
    float3 g_CameraPos;
    float padding1;
};

cbuffer cbPerObject : register(b1)
{
    float4x4 mViewProj;
    float gTime;
    float pad[3];
}

struct InstanceData
{
    row_major float4x4 mWorld;
    row_major float4x4 mTexTransform;
    row_major float4x4 mInvTWorld;
};

StructuredBuffer<InstanceData> Instances : register(t0);
Texture2D DiffuseMap : register(t1);

SamplerState Sampler : register(s0);

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float2 TexCoord : TEXCOORD;
    float3 Normal : NORMAL;
    float3 WorldPos : POSITION;
};

VS_OUTPUT VS(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    VS_OUTPUT output;
    InstanceData instance = Instances[instanceID];
    float3 instanceWorldPos = float3(instance.mWorld[3][0], instance.mWorld[3][1], instance.mWorld[3][2]);
    float3 manualOffset = float3(600.0f, 100.0f, 0.0f);
    instanceWorldPos += manualOffset;
    float2 uv = float2((vertexID << 1) & 2, vertexID & 2);
    float billboardSize = 150.0f;
    float3 localPos = float3(uv.x - 1.0f, -uv.y + 1.0f, 0.0f) * billboardSize;
    
    float3 zAxis = normalize(g_CameraPos - instanceWorldPos);
    float3 xAxis = normalize(cross(float3(0.0f, 1.0f, 0.0f), zAxis));
    float3 yAxis = cross(zAxis, xAxis);

    float3 worldPos = instanceWorldPos + (xAxis * localPos.x) + (yAxis * localPos.y);

    output.WorldPos = worldPos;
    output.Pos = mul(float4(worldPos, 1.0f), mViewProj);
    output.TexCoord = uv * 0.5f; 
    output.Normal = zAxis;

    return output;
}

struct gBufferOutput
{
    float4 Diffuse : SV_Target0;
    float4 Normal : SV_Target1;
};

gBufferOutput PS(VS_OUTPUT input)
{
    gBufferOutput output;
    float4 diffuseColor = DiffuseMap.Sample(Sampler, input.TexCoord);
    clip(diffuseColor.a - 0.1f);
    output.Diffuse = diffuseColor;

    output.Normal = float4(normalize(input.Normal) * 0.5f + 0.5f, 1.0f);
    return output;
}