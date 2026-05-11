cbuffer FrameConstants : register(b0)
{
    float4x4 g_MVP;
};

struct VS_INPUT
{
    float3 position : POSITION0;
    float3 color    : COLOR0;
};

struct VS_OUTPUT
{
    float4 position : SV_Position;
    float3 color    : TEXCOORD0;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    output.position = mul(float4(input.position, 1.0f), g_MVP);
    output.color = input.color;
    return output;
}
