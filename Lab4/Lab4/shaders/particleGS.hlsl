cbuffer Matrices : register(b0)
{
    float4x4 View;
    float4x4 Proj;
}

struct PixelInput
{
    float4 color : COLOR0;
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

PixelInput _offsetNprojected(PixelInput data, float2 offset, float2 uv, float4 color)
{
    data.position.xy += offset;
    data.position = mul(data.position, Proj);
    data.uv = uv;
    data.color = color;
    
    return data;
}

[maxvertexcount(4)] 
void GS(point PixelInput input[1], inout TriangleStream<PixelInput> stream)
{
    PixelInput pointOut = input[0];
    const float size = 1.0f;
    
    stream.Append(_offsetNprojected(pointOut, float2(-1, -1) * size, float2(0, 0), input[0].color));
    stream.Append(_offsetNprojected(pointOut, float2(-1, 1) * size, float2(0, 1), input[0].color));
    stream.Append(_offsetNprojected(pointOut, float2(1, -1) * size, float2(1, 0), input[0].color));
    stream.Append(_offsetNprojected(pointOut, float2(1, 1) * size, float2(1, 1), input[0].color));
    
    stream.RestartStrip();
}