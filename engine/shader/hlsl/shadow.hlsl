#include "ash_mvp.hlsl"

ConstantBuffer<ash_object> object : register(b0, space0);

struct ash_shadow
{
    float4x4 light_vp;
};
ConstantBuffer<ash_shadow> shadow : register(b0, space1);

struct vs_in
{
    float3 position : POSITION;
};

struct vs_out
{
    float4 position : SV_POSITION;
};

vs_out vs_main(vs_in vin)
{
    vs_out result;
    result.position = mul(mul(float4(vin.position, 1.0f), object.transform_m), shadow.light_vp);
    return result;
}