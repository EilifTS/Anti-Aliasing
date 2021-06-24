#ifndef UPSAMPLE_FACTOR
#define UPSAMPLE_FACTOR 4
#endif

#ifndef OPTIM
#define OPTIM 2
#endif

Texture2D			input_texture	    : register(t0);
Texture2D<half4>	history_buffer		: register(t1);
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

half4 catmullSample(float2 uv)
{
    half4 c = history_buffer.SampleLevel(linear_clamp, uv, 0);
    return c;
}

half4 catmullRomAppx(float2 uv)
{
    float2 position = uv * window_size;
    float2 center_position = floor(position - 0.5) + 0.5;
    float2 f = position - center_position;
    float2 f2 = f * f;
    float2 f3 = f2 * f;

    half2 w0 = half2(0.25 * (-3.0 * f3 + 6.0 * f2 - 3.0 * f));
    half2 w1 = half2(0.25 * ( 5.0 * f3 - 9.0 * f2           + 4.0));
    half2 w2 = half2(0.25 * (-5.0 * f3 + 6.0 * f2 + 3.0 * f));
    half2 w3 = half2(0.25 * ( 3.0 * f3 - 3.0 * f2));
    // float2 w2 = 1.0 - w0 - w1 - w3

    half2 w12 = w1 + w2;
    float2 tc12 = rec_window_size * (center_position + float2(w2) / float2(w12));
    half4 center_color = catmullSample(tc12);

    float2 tc0 = rec_window_size * (center_position - 1.0);
    float2 tc3 = rec_window_size * (center_position + 2.0);
    half4 color =
        catmullSample(float2(tc12.x, tc0.y)) * (w12.x * w0.y) +
        catmullSample(float2(tc0.x, tc12.y)) * (w0.x * w12.y) +
        center_color * (w12.x * w12.y) +
        catmullSample(float2(tc3.x, tc12.y)) * (w3.x * w12.y) +
        catmullSample(float2(tc12.x, tc3.y)) * (w12.x * w3.y);
    half weight = (w12.x * w0.y) + (w0.x * w12.y) + (w12.x * w12.y) + (w3.x * w12.y) + (w12.x * w3.y);
    return color / weight;
}

half4 catmullRom(float2 uv)
{
    float2 position = uv * window_size;
    float2 center_position = floor(position - 0.5) + 0.5;
    float2 f = position - center_position;
    float2 f2 = f * f;
    float2 f3 = f2 * f;

    //float2 w0 = -0.5 * f3 + f2 - 0.5 * f;
    //float2 w1 = 1.5 * f3 - 2.5 * f2 + 1.0;
    //float2 w2 = -1.5 * f3 + 2.0 * f2 + 0.5 * f;
    //float2 w3 = 0.5 * f3 - 0.5 * f2;
    half2 w0 = half2(0.25 * (-3.0 * f3 + 6.0 * f2 - 3.0 * f));
    half2 w1 = half2(0.25 * (5.0 * f3 - 9.0 * f2 + 4.0));
    half2 w2 = half2(0.25 * (-5.0 * f3 + 6.0 * f2 + 3.0 * f));
    half2 w3 = half2(0.25 * (3.0 * f3 - 3.0 * f2));
    //float2 w2 = 1.0 - w0 - w1 - w3

    half2 w12 = w1 + w2;
    float2 tc12 = rec_window_size * (center_position + float2(w2) / float2(w12));
    half4 center_color = catmullSample(tc12);

    float2 tc0 = rec_window_size * (center_position - 1.0);
    float2 tc3 = rec_window_size * (center_position + 2.0);
    half4 color =
        catmullSample(float2(tc0.x, tc0.y))  * (w0.x  * w0.y) +
        catmullSample(float2(tc3.x, tc0.y))  * (w3.x  * w0.y) +
        catmullSample(float2(tc12.x, tc0.y)) * (w12.x * w0.y) +
        catmullSample(float2(tc0.x, tc12.y)) * (w0.x  * w12.y) +
        center_color * (w12.x * w12.y) +
        catmullSample(float2(tc3.x, tc12.y)) * (w3.x  * w12.y) +
        catmullSample(float2(tc12.x, tc3.y)) * (w12.x * w3.y) +
        catmullSample(float2(tc3.x, tc3.y))  * (w3.x  * w3.y) +
        catmullSample(float2(tc0.x, tc3.y))  * (w0.x  * w3.y);
    return color;
}


float4 sub_bicConv(float t, float tt, float ttt, float4 f_1, float4 f0, float4 f1, float4 f2)
{
    float4 r_1 =             4.0*f0;
    float4 r0 = -3.0 * f_1          + 3.0 * f1;
    float4 r1 =  6.0 * f_1 - 9.0*f0 + 6.0 * f1 - 3.0 * f2;
    float4 r2 = -3.0 * f_1 + 5.0*f0 - 5.0 * f1 + 3.0 * f2;

    return 0.25 * (r_1 + r0 * t + r1 * tt + r2 * ttt);
}

float4 bicConv(float2 uv)
{
    float2 position = uv * window_size - 0.5;
    float2 center_position = floor(position);
    float2 f = position - center_position;
    float2 ff = f * f;
    float2 fff = ff * f;
    int2 pos0 = int2(center_position);
    int2 pos_1 = pos0 + int2(-1,-1);
    int2 pos1 = pos0 + int2(1,1);
    int2 pos2 = pos0 + int2(2,2);

    float4 g_1_1 = history_buffer[int2(pos_1.x, pos_1.y)];
    float4 g0_1 = history_buffer[int2(pos0.x, pos_1.y)];
    float4 g1_1 = history_buffer[int2(pos1.x, pos_1.y)];
    float4 g2_1 = history_buffer[int2(pos2.x, pos_1.y)];
    float4 b_1 = sub_bicConv(f.x, ff.x, fff.x, g_1_1, g0_1, g1_1, g2_1);

    float4 g_10 = history_buffer[int2(pos_1.x, pos0.y)];
    float4 g00 = history_buffer[int2(pos0.x, pos0.y)];
    float4 g10 = history_buffer[int2(pos1.x, pos0.y)];
    float4 g20 = history_buffer[int2(pos2.x, pos0.y)];
    float4 b0 = sub_bicConv(f.x, ff.x, fff.x, g_10, g00, g10, g20);
    
    float4 g_11 = history_buffer[int2(pos_1.x, pos1.y)];
    float4 g01 = history_buffer[int2(pos0.x, pos1.y)];
    float4 g11 = history_buffer[int2(pos1.x, pos1.y)];
    float4 g21 = history_buffer[int2(pos2.x, pos1.y)];
    float4 b1 = sub_bicConv(f.x, ff.x, fff.x, g_11, g01, g11, g21);

    float4 g_12 = history_buffer[int2(pos_1.x, pos2.y)];
    float4 g02 = history_buffer[int2(pos0.x, pos2.y)];
    float4 g12 = history_buffer[int2(pos1.x, pos2.y)];
    float4 g22 = history_buffer[int2(pos2.x, pos2.y)];
    float4 b2 = sub_bicConv(f.x, ff.x, fff.x, g_12, g02, g12, g22);
    
    return sub_bicConv(f.y, ff.y, fff.y, b_1, b0, b1, b2);
}

float linear_depth(float depth)
{
    float far = 100.0;
    float near = 0.1;
    depth = near * far / (far - depth * (far - near));
    return (depth - near) / (far - near);
}

uint get_pixel_shuffle_index(uint x, uint y, uint r, uint W, uint C)
{
    uint ym = y % r;
    uint xm = x % r;

    return (y - ym) * W * C + (x - xm) * r * C + r * ym + xm;
}

[numthreads(8, 8, 1)]
void CS(uint3 block_id : SV_GroupID, uint3 thread_id : SV_GroupThreadID)
{
    uint2 window_size_int = (uint2)window_size;
    uint2 pixel_pos = uint2(block_id.x * 8 + thread_id.x, block_id.y * 8 + thread_id.y);

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
        jau_rgbd.rgb = input_texture.SampleLevel(linear_clamp, hr_jitter_uv, 0).rgb;
        jau_rgbd.a = 1.0;// depth_buffer.SampleLevel(linear_clamp, hr_jitter_uv, 0);

        // reproject history
        float2 hr_pixel_uv = (float2(0.5, 0.5) + (float2)pixel_pos) * rec_window_size;
        float2 motion_vector = motion_vectors.SampleLevel(linear_clamp, hr_pixel_uv, 0);
        float2 prev_frame_uv = hr_pixel_uv + motion_vector;

        half4 history = half4(0.0, 0.0, 0.0, 0.0);
        if (prev_frame_uv.x > 0.0 && prev_frame_uv.x <= 1.0 && prev_frame_uv.y > 0.0 && prev_frame_uv.y <= 1.0) // Check if uv is inside history buffer
        {
            //history = history_buffer.SampleLevel(linear_clamp, prev_frame_uv, 0);
            history = catmullRom(prev_frame_uv);
            //history = catmullRomAppx(prev_frame_uv);
        }
        else
            history = half4(jau_rgbd);


        // Fill output tensor
#if OPTIM == 2
        uint index0 = get_pixel_shuffle_index(pixel_pos.x, pixel_pos.y, 4, window_size_int.x, 8);
        uint index1 = index0 + 4 * 4 * 1;
        uint index2 = index0 + 4 * 4 * 2;
        uint index3 = index0 + 4 * 4 * 3;
        uint index4 = index0 + 4 * 4 * 4;
        uint index5 = index0 + 4 * 4 * 5;
        uint index6 = index0 + 4 * 4 * 6;
        uint index7 = index0 + 4 * 4 * 7;
#else
        uint index = window_size_int.x * pixel_pos.y + pixel_pos.x;
        uint index0 = 8 * index + 0;
        uint index1 = 8 * index + 1;
        uint index2 = 8 * index + 2;
        uint index3 = 8 * index + 3;
        uint index4 = 8 * index + 4;
        uint index5 = 8 * index + 5;
        uint index6 = 8 * index + 6;
        uint index7 = 8 * index + 7;
#endif
        out_tensor[index0] = half(zero_up_rgbd.r);
        out_tensor[index1] = half(zero_up_rgbd.g);
        out_tensor[index2] = half(zero_up_rgbd.b);
        out_tensor[index3] = half(zero_up_rgbd.a);
        out_tensor[index4] = history.r;
        out_tensor[index5] = history.g;
        out_tensor[index6] = history.b;
        out_tensor[index7] = history.a;
        //out_tensor[index7] = 0.0; // Disable accumulation buffer (Requires different network)
    }
}