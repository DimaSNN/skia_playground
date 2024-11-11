#ifndef LAZER_LINE_H
#define LAZER_LINE_H

#include <vector>
#include <memory>
#include <chrono>
#include <cstdlib>
#include <algorithm>
#include <functional>
#include <iostream>
#include <deque>


#include "point.h"


using speed_vec_t = hmos::Point;

using DrawLaszerSegmentFn = std::function<void(const hmos::Point& pStart, const hmos::Point& pEnd, float width)>;

static constexpr std::chrono::milliseconds SEGMENT_TTL{ 800 }; // time to live for spark
static constexpr float SEGMENT_MAX_WIDTH = 20.0f;
static constexpr float SEGMENT_MIN_WIDTH = 0.0f;
static constexpr float MIN_DISTANCE_TO_ADD = 0.0f;

class LazerTrace {

private:
    struct PointData {
        hmos::Point m_point{};
        std::chrono::milliseconds m_burnTime{};
        float m_width{};
        float m_maxWidth{};
        bool m_isInitState;
    };

public:

    LazerTrace() = default;
    ~LazerTrace() = default;

    void clear() {
        m_points.clear();
    }

    void addPoint(std::chrono::milliseconds timePoint, const hmos::Point& currPoint)
    {
        if (m_points.empty() || 
            (m_points.back().m_point != currPoint && m_points.back().m_point.distance(currPoint) >= MIN_DISTANCE_TO_ADD)) {
            
            if (m_points.empty()) {
                std::cout << "ashim: add first point " << currPoint  << "\n";
                m_points.emplace_back(PointData{ currPoint, timePoint, SEGMENT_MIN_WIDTH, SEGMENT_MAX_WIDTH, true });
            } else {
                std::cout << "ashim: add new point " << currPoint << "\n";
                auto& lastPoint = m_points.back();
                m_points.emplace_back(PointData{ currPoint, timePoint, lastPoint.m_width, lastPoint.m_width, false });
                std::swap(lastPoint, m_points.back());
                m_points.back().m_point = currPoint;

                for (auto& p : m_points) {
                    std::cout << "ashim: trace point " << p.m_point << "\n";
                }
            }
        }
    }

    void onTimeTick(std::chrono::milliseconds timePoint) {
        if (m_points.empty()) { 
            return; 
        }

        m_timePoint = timePoint;

        // Update last point to prevent from death
        auto& lastPoint = m_points.back();
        if (!lastPoint.m_isInitState) {
            m_points.back().m_burnTime = m_timePoint;
        }

        while (!m_points.empty()) {
            if ((m_timePoint - m_points.front().m_burnTime) > SEGMENT_TTL) {
                m_points.pop_front();
            }
            else {
                break;
            }
        }

        if (!m_points.empty()) {
            for (auto i = 0; i < m_points.size(); ++i) {
                auto timePassed = (m_timePoint - m_points[i].m_burnTime);
                auto fraction = 1.0f - static_cast<float>(timePassed.count()) / static_cast<float>(SEGMENT_TTL.count());
                if (m_points[i].m_isInitState) {
                    fraction = 1.0f - fraction;
                    if (fraction >= 0.99) {
                        m_points[i].m_isInitState = false;
                        m_points[i].m_maxWidth = SEGMENT_MAX_WIDTH;
                        fraction = 1.0;
                    }
                }
                auto w = fraction * m_points[i].m_maxWidth;
                std::cout << "ashim: fraction-> " << fraction << "\n";
                std::cout << "ashim: width-> " << w << "\n";
                std::cout << "ashim: m_isInitState-> " << m_points[i].m_isInitState << "\n";
                m_points[i].m_width = w;
            }
        }
    }

    void onDraw(DrawLaszerSegmentFn fn)
    {
        if (fn && !m_points.empty()) {
            for (auto i = 0; i < m_points.size(); ++i) {
                if (i == m_points.size() - 1) {
                    // last point (finger position)
                    fn(m_points[i].m_point, m_points[i].m_point, m_points[i].m_width);
                } else {
                    fn(m_points[i].m_point, m_points[i + 1].m_point, m_points[i].m_width);
                }
            }
        }
    }

private:
    std::deque<PointData> m_points;
    std::chrono::milliseconds m_timePoint; // last time when onTimeTick() was called
};


#endif