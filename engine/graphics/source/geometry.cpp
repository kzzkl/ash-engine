#include "geometry.hpp"

namespace ash::graphics
{
geometry_data geometry::box(float x, float y, float z)
{
    float half_x = x * 0.5f;
    float half_y = y * 0.5f;
    float half_z = z * 0.5f;

    geometry_data result;
    result.vertices = {
        {{-half_x, half_y, half_z},   {0.0f, 1.0f, 0.0f} }, // up
        {{half_x, half_y, half_z},    {0.0f, 1.0f, 0.0f} },
        {{half_x, half_y, -half_z},   {0.0f, 1.0f, 0.0f} },
        {{-half_x, half_y, -half_z},  {0.0f, 1.0f, 0.0f} },

        {{-half_x, -half_y, half_z},  {0.0f, -1.0f, 0.0f}}, // bottom
        {{half_x, -half_y, half_z},   {0.0f, -1.0f, 0.0f}},
        {{half_x, -half_y, -half_z},  {0.0f, -1.0f, 0.0f}},
        {{-half_x, -half_y, -half_z}, {0.0f, -1.0f, 0.0f}},

        {{-half_x, half_y, half_z},   {-1.0f, 0.0f, 0.0f}}, // left
        {{-half_x, half_y, -half_z},  {-1.0f, 0.0f, 0.0f}},
        {{-half_x, -half_y, -half_z}, {-1.0f, 0.0f, 0.0f}},
        {{-half_x, -half_y, half_z},  {-1.0f, 0.0f, 0.0f}},

        {{half_x, half_y, half_z},    {1.0f, 0.0f, 0.0f} }, // right
        {{half_x, half_y, -half_z},   {1.0f, 0.0f, 0.0f} },
        {{half_x, -half_y, -half_z},  {1.0f, 0.0f, 0.0f} },
        {{half_x, -half_y, half_z},   {1.0f, 0.0f, 0.0f} },

        {{-half_x, half_y, -half_z},  {0.0f, 0.0f, -1.0f}}, // forward
        {{half_x, half_y, -half_z},   {0.0f, 0.0f, -1.0f}},
        {{half_x, -half_y, -half_z},  {0.0f, 0.0f, -1.0f}},
        {{-half_x, -half_y, -half_z}, {0.0f, 0.0f, -1.0f}},

        {{-half_x, half_y, half_z},   {0.0f, 0.0f, 1.0f} }, // back
        {{half_x, half_y, half_z},    {0.0f, 0.0f, 1.0f} },
        {{half_x, -half_y, half_z},   {0.0f, 0.0f, 1.0f} },
        {{-half_x, -half_y, half_z},  {0.0f, 0.0f, 1.0f} }
    };

    result.indices = {
        0,  1,  2,  0,  2,  3,  // up
        4,  6,  5,  4,  7,  6,  // down
        8,  9,  10, 8,  10, 11, // left
        12, 14, 13, 12, 15, 14, // right
        16, 17, 18, 16, 18, 19, // forward
        20, 22, 21, 20, 23, 22  // back
    };

    return result;
}
} // namespace ash::graphics