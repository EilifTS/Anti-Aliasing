#ifndef UPSAMPLE_FACTOR
#define UPSAMPLE_FACTOR 4
#endif

Texture2D			input_texture	    : register(t0);
Texture2D			history_buffer		: register(t1);
Texture2D<float2>	motion_vectors		: register(t2);
Texture2D<float>	depth_buffer		: register(t3);
SamplerState		linear_clamp		: register(s0);
SamplerState		point_clamp		: register(s1);

cbuffer constants : register(b0)
{
    float2 window_size;
    float2 rec_window_size;
    float2 current_jitter;
}

RWBuffer<half> out_tensor   : register(u0);

float4 catmullSample(float2 uv)
{
    float4 c = history_buffer.SampleLevel(linear_clamp, uv, 0);
    return c;
}

float4 catmullRom(float2 uv)
{
    float2 position = uv * window_size;
    float2 center_position = floor(position - 0.5) + 0.5;
    float2 f = position - center_position;
    float2 f2 = f * f;
    float2 f3 = f2 * f;

    float2 w0 = -0.5 * f3 + f2 - 0.5 * f;
    float2 w1 = 1.5 * f3 - 2.5 * f2 + 1.0;
    float2 w2 = -1.5 * f3 + 2.0 * f2 + 0.5 * f;
    float2 w3 = 0.5 * f3 - 0.5 * f2;
    // float2 w2 = 1.0 - w0 - w1 - w3

    float2 w12 = w1 + w2;
    float2 tc12 = rec_window_size * (center_position + w2 / w12);
    float4 center_color = catmullSample(tc12);

    float2 tc0 = rec_window_size * (center_position - 1.0);
    float2 tc3 = rec_window_size * (center_position + 2.0);
    float4 color =
        catmullSample(float2(tc12.x, tc0.y)) * (w12.x * w0.y) +
        catmullSample(float2(tc0.x, tc12.y)) * (w0.x * w12.y) +
        center_color * (w12.x * w12.y) +
        catmullSample(float2(tc3.x, tc12.y)) * (w3.x * w12.y) +
        catmullSample(float2(tc12.x, tc3.y)) * (w12.x * w3.y);
    float weight = (w12.x * w0.y) + (w0.x * w12.y) + (w12.x * w12.y) + (w3.x * w12.y) + (w12.x * w3.y);
    return color / weight;
}

float linear_depth(float depth)
{
    float far = 100.0;
    float near = 0.1;
    depth = near * far / (far - depth * (far - near));
    return (depth - near) / (far - near);
}

[numthreads(32, 16, 1)]
void CS(uint3 block_id : SV_GroupID, uint3 thread_id : SV_GroupThreadID)
{
    uint2 window_size_int = (uint2)window_size;
    uint2 pixel_pos = uint2(block_id.x * 32 + thread_id.x, block_id.y * 16 + thread_id.y);

    if (pixel_pos.x < window_size_int.x && pixel_pos.y < window_size_int.y)
    {
        uint2 lr_pixel_pos = pixel_pos / UPSAMPLE_FACTOR; // Position of pixel in low res image
        float2 jitter_offset = current_jitter; // Pixel offset of jitter in low res image

        // Zero upsample rgbd
        float2 lr_jitter_pos = (float2)lr_pixel_pos + jitter_offset;
        uint2 hr_jitter_pos_int = (uint2)(lr_jitter_pos * UPSAMPLE_FACTOR);
        float beta = 0.0;
        if (hr_jitter_pos_int.x == pixel_pos.x && hr_jitter_pos_int.y == pixel_pos.y) beta = 1.0;
        float4 zero_up_rgbd = float4(0.0, 0.0, 0.0, 0.0);
        zero_up_rgbd.rgb = input_texture[lr_pixel_pos].rgb;
        zero_up_rgbd.a = linear_depth(depth_buffer[lr_pixel_pos]);
        zero_up_rgbd = zero_up_rgbd * beta;

        // JAU rgbd
        float2 hr_jitter_pos = (float2(0.5, 0.5) + (float2)pixel_pos) + (float2(0.5, 0.5) - jitter_offset) * UPSAMPLE_FACTOR;
        float2 hr_jitter_uv = hr_jitter_pos * rec_window_size;
        float4 jau_rgbd = float4(0.0, 0.0, 0.0, 0.0);
        //jau_rgbd = catmullRom(hr_jitter_uv, true);
        jau_rgbd.rgb = input_texture.SampleLevel(linear_clamp, hr_jitter_uv, 0);
        jau_rgbd.a = 1.0;// depth_buffer.SampleLevel(linear_clamp, hr_jitter_uv, 0);

        // reproject history
        float2 hr_pixel_uv = (float2(0.5, 0.5) + (float2)pixel_pos) * rec_window_size;
        float2 motion_vector = motion_vectors.SampleLevel(linear_clamp, hr_pixel_uv, 0);
        float2 prev_frame_uv = hr_pixel_uv + motion_vector;

        float4 history = float4(0.0, 0.0, 0.0, 0.0);
        if (prev_frame_uv.x > 0.0 && prev_frame_uv.x <= 1.0 && prev_frame_uv.y > 0.0 && prev_frame_uv.y <= 1.0) // Check if uv is inside history buffer
            history = catmullRom(prev_frame_uv);
        else
            history = jau_rgbd;

        // Fill output tensor
        uint index = window_size_int.x * pixel_pos.y + pixel_pos.x;
        out_tensor[index * 8 + 0] = zero_up_rgbd.r;
        out_tensor[index * 8 + 1] = zero_up_rgbd.g;
        out_tensor[index * 8 + 2] = zero_up_rgbd.b;
        out_tensor[index * 8 + 3] = zero_up_rgbd.a;
        out_tensor[index * 8 + 4] = history.r;
        out_tensor[index * 8 + 5] = history.g;
        out_tensor[index * 8 + 6] = history.b;
        out_tensor[index * 8 + 7] = history.a;
    }
}