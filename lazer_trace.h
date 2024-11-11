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
        float m_maxWidth{};

        float getWidthForTime(const std::chrono::milliseconds& t, bool isLast = false)
        {
            auto timePassed = t - m_burnTime;
            timePassed = std::min(timePassed, SEGMENT_TTL);
            auto fraction = static_cast<float>(timePassed.count()) / static_cast<float>(SEGMENT_TTL.count());
            if (!isLast) {
                fraction = 1.0f - fraction;
            }
            return fraction * m_maxWidth;;
        }
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
                m_points.emplace_back(PointData{ currPoint, timePoint, SEGMENT_MAX_WIDTH });
            } else {
                std::cout << "ashim: add new point " << currPoint << "\n";
                auto lastPointCopy = m_points.back(); // copy lazer point
                m_points.back() = PointData{ lastPointCopy.m_point, timePoint, lastPointCopy.getWidthForTime(timePoint, true) };
                lastPointCopy.m_point = currPoint; // update position for lazer point
                m_points.emplace_back(std::move(lastPointCopy));
            }
        }
    }

    void onTimeTick(std::chrono::milliseconds timePoint) {
        if (m_points.empty()) { 
            return; 
        }

        m_timePoint = timePoint;

        auto it = std::find_if(m_points.begin(), std::prev(m_points.end()), [this](const PointData& p) {
            return (m_timePoint - p.m_burnTime) <= SEGMENT_TTL;
        });
        if (it != m_points.end()) {
           m_points.erase(m_points.begin(), it);
        }
    }

    void onDraw(DrawLaszerSegmentFn fn)
    {
        if (fn && !m_points.empty()) {
            for (auto i = 0; i < m_points.size(); ++i) {
                if (i == m_points.size() - 1) {
                    // lazer point head (current finger position)
                    fn(m_points[i].m_point, m_points[i].m_point, m_points[i].getWidthForTime(m_timePoint, true));
                } else {
                    // lazer point tail
                    fn(m_points[i].m_point, m_points[i + 1].m_point, m_points[i].getWidthForTime(m_timePoint, false));
                }
            }
        }
    }

private:
    std::deque<PointData> m_points;
    std::chrono::milliseconds m_timePoint; // last time when onTimeTick() was called
};


#endif