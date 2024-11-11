#include "spark.h"

namespace details {
    speed_vec_t generate_speed(speed_vec_t::speed_type_t max_speed)
    {
        //std::srand(std::time(nullptr));
        auto x = ((float)rand() / (RAND_MAX)) - 0.5f;
        auto y = ((float)rand() / (RAND_MAX)) - 0.5f;
        return speed_vec_t{ x * max_speed, y * max_speed };
    }

    radius_t generate_spark_radius(radius_t min_radius, radius_t max_radius)
    {
        return ((float)rand() / (RAND_MAX)) * (max_radius - min_radius);
    }
}
