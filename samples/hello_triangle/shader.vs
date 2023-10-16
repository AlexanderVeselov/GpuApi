struct VS_INPUT
{
    float4 position : POSITION0;
    float3 color    : COLOR0;
    float2 texcoord : TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 position : SV_Position;
    float3 color    : TEXCOORD0;
};

VS_OUTPUT main(VS_INPUT input, uint vertex_id : SV_VertexID)
{
    VS_OUTPUT output;
    output.position = float4(input.position.xyz, 1.0f);
    output.color = input.color;

    return output;
}
