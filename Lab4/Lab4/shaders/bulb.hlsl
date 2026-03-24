cbuffer cbPerObject : register(b0)
{
    float4x4 mWorld;
    float4x4 mViewProj;
    float4x4 mTexTransform;
    float gTime;
    float pad[3];
}

struct LightData
{
    float3 Color;
    int Type;
    float3 Position;
    float Range;
    float3 Direction;
    float SpotInner;
    float SpotOuter;
    float3 padding;
};

StructuredBuffer<LightData> Lights : register(t0);

struct VS_INPUT
{
    float3 Position : POSITION;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 Color : COLOR;
};

PS_INPUT VS(VS_INPUT input, uint instanceID : SV_InstanceID)
{
    PS_INPUT output;
    
    float3 worldPos = input.Position + Lights[instanceID].Position;
    
    output.Pos = mul(float4(worldPos, 1.0f), mViewProj);
    
    output.Color = Lights[instanceID].Color;
    return output;
}

float4 PS(PS_INPUT input) : SV_Target
{
    return float4(input.Color * 1.5f, 1.0f);
}