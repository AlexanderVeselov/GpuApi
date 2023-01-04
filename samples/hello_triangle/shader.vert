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

VS_OUTPUT main(VS_INPUT input, uint vertex_id : SV_VertexID)
{
    float4 positions[3] = {
        { -0.5f, -0.5f, 0.0f, 1.0f },
        {  0.5f, -0.5f, 0.0f, 1.0f },
        {  0.0f,  0.5f, 0.0f, 1.0f }
    };

    float3 colors[3] = {
        { 1.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f }
    };

    VS_OUTPUT output;
    output.position = positions[vertex_id % 3];
    output.color = colors[vertex_id % 3];

    return output;
}
