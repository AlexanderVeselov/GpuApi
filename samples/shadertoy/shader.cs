RWTexture2D<float4> g_Output : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID)
{
    uint width;
    uint height;
    g_Output.GetDimensions(width, height);

    if (dispatch_thread_id.x >= width || dispatch_thread_id.y >= height)
    {
        return;
    }

    float2 uv = (float2(dispatch_thread_id.xy) + 0.5f) / float2(width, height);
    g_Output[dispatch_thread_id.xy] = float4(uv, 0.0f, 1.0f);
}
