// =========================================================================
// SDL3 GPU 着色器公共定义
// 支持：圆角矩形 UI、外发光阴影、抗锯齿裁切、整体透明度
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