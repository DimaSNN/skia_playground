#ifndef GRADIENT_TRACE_H
#define GRADIENT_TRACE_H

#include <vector>
#include <memory>
#include <chrono>
#include <cstdlib>
#include <algorithm>
#include <functional>
#include <iostream>
#include <deque>


#include "point.h"
#include "path_points_storage.h"


using ColorType = uint32_t;

static constexpr ColorType C1 = 4294901760; // SK_ColorRED;
static constexpr ColorType C2 = 4294967040; // SK_ColorYELLOW;
static constexpr ColorType C3 = 4278190335; //SK_ColorBLUE;

static constexpr float SEGMENT_WIDTH = 20.0f;

struct SkiaColorHelper {
    static ColorType build(uint8_t r, uint8_t g, uint8_t b)
    {
        return  (0xFF << 24) | (r << 16) | (g << 8) | (b << 0);
    }

    static uint8_t ColorGetR(ColorType c)
    {
        return  (((c) >> 16) & 0xFF);
    }

    static uint8_t ColorGetG(ColorType c)
    {
        return   (((c) >> 8) & 0xFF);
    }

    static uint8_t ColorGetB(ColorType c)
    {
        return  (((c) >> 0) & 0xFF);
    }
};


using ColorHelper = SkiaColorHelper;

using GradientTraceDrawFn = std::function<void(const hmos::Point& p1, const ColorType& c1, const hmos::Point& p2, const ColorType& c2, float width)>;

class GradientTrace {

public:

    GradientTrace() = delete;
    GradientTrace(PathPointsStorage& pointStorage) :
        m_pointStorage(&pointStorage)
    {

    }

    ~GradientTrace() = default;

    void onDraw(GradientTraceDrawFn fn)
    {
        if (fn && !m_pointStorage->empty()) {
            const auto& points = m_pointStorage->getPathPoints();
            std::cout << "ashim: PATH_LENGTH-> " << m_pointStorage->getPathLength() << "\n";
            float dist = 0.0;
            for (int i = 0; i < points.size() - 1; ++i) {
                hmos::Point p1{ points[i].x,  points[i].y };
                hmos::Point p2{ points[i + 1].x,  points[i + 1].y };
                auto p1p2dist = p1.distance(p2);
                std::cout << "ashim: points-> " << p1 << p2 << "\n";
                std::cout << "ashim: p1_dist-> " << dist << "\n";
                std::cout << "ashim: p2_dist-> " << (dist + p1p2dist) << "\n";
                fn(p1, calculateColor(dist), p2, calculateColor(dist + p1p2dist), SEGMENT_WIDTH);
                dist += p1p2dist;
            }
        }
    }

private:
    /*
    Use points storage to avoid copy
    */
    PathPointsStorage* m_pointStorage;

    ColorType calculateColor(float point_distance) const {
        static auto INTERPOLATOR = [](float _x, float _xa, ColorType colA, float _xb, ColorType colB) {
            return static_cast<uint8_t>(((_x - _xa) / (_xb - _xa)) * (static_cast<float>(colB) - static_cast<float>(colA)) + static_cast<float>(colA));
        };
        
        uint8_t r = 0;
        uint8_t g = 0;
        uint8_t b = 0;
        
        float pathLength = m_pointStorage->getPathLength();
        float path_middle = pathLength / 2;
        
        if (point_distance <= path_middle) {
            r = INTERPOLATOR(point_distance, 0.0f, ColorHelper::ColorGetR(C1), path_middle, ColorHelper::ColorGetR(C2));
            g = INTERPOLATOR(point_distance, 0.0f, ColorHelper::ColorGetG(C1), path_middle, ColorHelper::ColorGetG(C2));
            b = INTERPOLATOR(point_distance, 0.0f, ColorHelper::ColorGetB(C1), path_middle, ColorHelper::ColorGetB(C2));

        }
        else {
            r = INTERPOLATOR(point_distance, path_middle, ColorHelper::ColorGetR(C2), pathLength, ColorHelper::ColorGetR(C3));
            g = INTERPOLATOR(point_distance, path_middle, ColorHelper::ColorGetG(C2), pathLength, ColorHelper::ColorGetG(C3));
            b = INTERPOLATOR(point_distance, path_middle, ColorHelper::ColorGetB(C2), pathLength, ColorHelper::ColorGetB(C3));
        }

        std::cout << "ashim: point_distance-> " << point_distance << "\n";
        std::cout << "ashim: path_length-> " << pathLength << "\n";
        std::cout << "ashim: r-> " << (int)r << "\n";
        std::cout << "ashim: g-> " << (int)g << "\n";
        std::cout << "ashim: b-> " << (int)b << "\n";

        return ColorHelper::build(r, g, b);
    };
};


#endif