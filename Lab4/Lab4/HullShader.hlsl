struct VS_CONTROL_POINT_OUTPUT
{
    float4 pos : POSITION;
    float4 Wpos : WORLDPOS;
    float3 normal : NORMAL0;
    float3 normalW : NORMAL1;
    float2 uv : TEXCOORD;
    float3 tangentW : TANGENT;
};

struct HS_CONTROL_POINT_OUTPUT
{
    float4 pos : POSITION;
    float4 Wpos : WORLDPOS;
    float3 normal : NORMAL0;
    float3 normalW : NORMAL1;
    float2 uv : TEXCOORD;
    float3 tangentW : TANGENT;
};

struct HS_CONSTANT_DATA_OUTPUT
{
	float EdgeTessFactor[3]			: SV_TessFactor;
	float InsideTessFactor			: SV_InsideTessFactor;
};

#define NUM_CONTROL_POINTS 3

HS_CONSTANT_DATA_OUTPUT CalcHSPatchConstants(
	InputPatch<VS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> ip,
	uint PatchID : SV_PrimitiveID)
{
	HS_CONSTANT_DATA_OUTPUT Output;
    float tess = 4;
    
    Output.EdgeTessFactor[0] = tess;
    Output.EdgeTessFactor[1] = tess;
    Output.EdgeTessFactor[2] = tess;
	Output.InsideTessFactor = tess;

	return Output;
}

[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("CalcHSPatchConstants")]
HS_CONTROL_POINT_OUTPUT main( 
	InputPatch<VS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> ip, 
	uint i : SV_OutputControlPointID,
	uint PatchID : SV_PrimitiveID )
{
	HS_CONTROL_POINT_OUTPUT Output;

	Output.pos = ip[i].pos;
    Output.Wpos = ip[i].Wpos;
    Output.normal = ip[i].normal;
    Output.normalW = ip[i].normalW;
    Output.uv = ip[i].uv;
    Output.tangentW = ip[i].tangentW;
	return Output;
}
