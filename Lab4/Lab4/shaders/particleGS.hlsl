struct PixelInput
{
    float4 position : SV_POSITION;
};

[maxvertexcount(1)] 
void GS(point PixelInput input[1], inout PointStream<PixelInput> stream)
{
    PixelInput pointOut = input[0];
	
    stream.Append(pointOut); 
    stream.RestartStrip();
}