// =========================================================================
// SDL3 基础着色器：圆角矩形 UI 元素
// 支持：多独立圆角、外发光阴影、抗锯齿裁切、整体透明度
// =========================================================================

// --- 0. Push Constants (必须与 C++ 结构体 16 字节对齐) ---
struct PushConstants
{
    float2 screen_size;    // 屏幕尺寸 (用于坐标转换)
    float2 rect_size;      // 矩形实际像素尺寸 (用于 SDF 计算)
    float4 radius;         // 四个角的半径 (x:左上, y:右上, z:右下, w:左下)
    float shadow_soft;     // 阴影柔和度 (像素单位，0则无阴影)
    float shadow_offset_x; // 阴影偏移 X
    float shadow_offset_y; // 阴影偏移 Y
    float opacity;         // 整体透明度 (0.0 - 1.0)
    float padding;         // 填充位，保证结构体对齐
};
ConstantBuffer<PushConstants> pc : register(b0);

// --- 1. 输入输出结构 ---
struct VSInput
{
    float2 position : POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
};

struct PSInput
{
    float4 sv_position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
};

// --- 2. 纹理定义 ---
Texture2D u_texture : register(t1);
SamplerState u_sampler : register(s1);

// =========================================================================
// 顶点着色器 (Vertex Shader)
// =========================================================================
PSInput main_vs(VSInput input)
{
    PSInput output;

    // 将像素坐标转为 NDC [-1, 1]，原点设在左上角，Y轴向下
    float2 ndc = float2((input.position.x / pc.screen_size.x) * 2.0f - 1.0f,
                        1.0f - (input.position.y / pc.screen_size.y) * 2.0f);

    output.sv_position = float4(ndc, 0.0f, 1.0f);
    output.texcoord = input.texcoord;
    output.color = input.color;
    return output;
}

// =========================================================================
// 像素着色器 (Pixel Shader)
// =========================================================================

// 圆角矩形 SDF 函数
float sdRoundedBox(float2 p, float2 b, float4 r)
{
    // 根据当前所在象限选择对应的半径 (TopLeft, TopRight, etc.)
    float2 q_radius = (p.x > 0.0) ? r.yz : r.xw;
    float actual_r = (p.y > 0.0) ? q_radius.x : q_radius.y;

    float2 q = abs(p) - b + actual_r;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - actual_r;
}
// 主像素着色器函数
float4 main_ps(PSInput input) : SV_Target
{
    // 1. 准备像素坐标空间 (以矩形中心为原点)
    float2 p = (input.texcoord - 0.5) * pc.rect_size;
    float2 half_size = pc.rect_size * 0.5;

    // 2. 计算主体形状的距离场
    float dist = sdRoundedBox(p, half_size, pc.radius);

    // 3. 计算阴影 (Shadow)
    float shadow_alpha = 0.0;
    if (pc.shadow_soft > 0.0)
    {
        // 阴影坐标偏移
        float2 shadow_p = p - float2(pc.shadow_offset_x, pc.shadow_offset_y);
        float dist_shadow = sdRoundedBox(shadow_p, half_size, pc.radius);
        // 阴影随距离衰减，越往外越淡
        shadow_alpha = 1.0 - smoothstep(-pc.shadow_soft, pc.shadow_soft, dist_shadow);
        shadow_alpha *= 0.5; // 降低阴影强度，使其更自然
    }

    // 4. 抗锯齿掩码 (利用 fwidth 自动适应缩放)
    float edge_softness = fwidth(dist);
    float body_mask = 1.0 - smoothstep(-edge_softness, edge_softness, dist);

    // 5. 纹理采样与颜色混合
    float4 tex_color = u_texture.Sample(u_sampler, input.texcoord) * input.color;

    // 6. 最终合成
    // 如果像素在主体内，显示主体颜色；如果不在主体内但在阴影区，显示阴影
    float4 final_color;

    // 阴影层 (黑色透明)
    float4 shadow_layer = float4(0.0, 0.0, 0.0, shadow_alpha);

    // 使用预乘或线性混合
    // 这里简单处理：当 body_mask 为 0 时完全显示阴影，为 1 时显示主体
    final_color = lerp(shadow_layer, tex_color, body_mask);

    // 应用全局透明度
    final_color.a *= pc.opacity;

    // 如果该像素既没有主体也没有阴影，则完全剔除 (对性能友好)
    if (final_color.a < 0.001) discard;

    return final_color;
}