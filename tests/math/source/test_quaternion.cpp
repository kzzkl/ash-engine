#include "test_common.hpp"
#include <cmath>

using namespace ash::math;

namespace ash::test
{
namespace math_plain
{
TEST_CASE("quaternion_plain::rotation_axis", "[quaternion]")
{
    float4 axis = vector::normalize(float4{1.0f, 2.0f, 3.0f, 0.0f});
    float4 quat = quaternion::rotation_axis(axis, 0.25f);
    CHECK(equal(quat, float4{0.033320725f, 0.0666414499f, 0.0999621749f, 0.992197692f}));
}

TEST_CASE("quaternion_plain::rotation_euler", "[quaternion]")
{
    float4 quat = quaternion::rotation_euler(0.3f, 0.2f, 0.4f);
    CHECK(equal(quat, float4{0.126285180f, 0.126116529f, 0.180835575f, 0.967184186f}));
}

TEST_CASE("quaternion_plain::rotation_matrix", "[quaternion]")
{
    float4x4 m = {
        float4{0.886326671f,  0.401883781f,   -0.230031431f, 0.0f},
        float4{-0.366907358f, 0.912558973f,   0.180596486f,  0.0f},
        float4{0.282496035f,  -0.0756672472f, 0.956279457f,  0.0f},
        float4{0.0f,          0.0f,           0.0f,          1.0f}
    };

    float4 quat = quaternion::rotation_matrix(m);
    CHECK(equal(quat, float4{0.0661214888f, 0.132242978f, 0.198364466f, 0.968912423f}));
}

TEST_CASE("quaternion_plain::mul", "[quaternion]")
{
    float4 quat1 = {0.033320725f, 0.0666414499f, 0.0999621749f, 0.992197692f};
    float4 quat2 = {0.126285180f, 0.126116529f, 0.180835575f, 0.967184186f};

    float4 result = quaternion::mul(quat1, quat2);
    CHECK(equal(result, float4{0.156971410f, 0.196185246f, 0.271892935f, 0.928948700f}));
}
} // namespace math_plain
} // namespace ash::test