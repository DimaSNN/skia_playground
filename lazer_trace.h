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


#include "path_points_storage.h"


using speed_vec_t = hmos::Point;

using DrawLaszerSegmentFn = std::function<void(const hmos::Point& pStart, const hmos::Point& pEnd, float width)>;

static constexpr std::chrono::milliseconds SEGMENT_TTL{ 800 }; // time to live for spark
static constexpr float SEGMENT_MAX_WIDTH = 20.0f;
static constexpr float SEGMENT_MIN_WIDTH = 0.0f;
static constexpr float MIN_DISTANCE_TO_ADD = 0.0f;

class LazerTrace {

private:
    struct PointData {
        size_t m_point{0};
        std::chrono::milliseconds m_burnTime{};
        float m_maxWidth{};

        float getWidthForTime(const std::chrono::milliseconds& t, bool toGrow = true)
        {
            auto timePassed = t - m_burnTime;
            timePassed = std::min(timePassed, SEGMENT_TTL);
            auto fraction = static_cast<float>(timePassed.count()) / static_cast<float>(SEGMENT_TTL.count());
            if (!toGrow) {
                fraction = 1.0f - fraction;
            }
            return fraction * m_maxWidth;;
        }
    };

public:

    LazerTrace() = delete;
    LazerTrace(PathPointsStorage& pointStorage) :
        m_pointStorage(&pointStorage)
    {

    }
    ~LazerTrace() = default;

    void onComplete()
    {
        m_completeFlag = true;
    }

    void onPointsAdded(std::chrono::milliseconds timePoint, size_t startIndex, size_t endIndex)
    {
        const auto& pathPoints = m_pointStorage->getPathPoints();
        for (auto i = startIndex; i <= endIndex; ++i) {
            if (m_points.empty()) {
                std::cout << "ashim: add first point " << pathPoints[i] << "\n";
                std::cout << "ashim: startIndex " << startIndex << "\n";
                std::cout << "ashim: endIndex " << endIndex << "\n";
                m_points.emplace_back(PointData{ i, timePoint, SEGMENT_MAX_WIDTH });
            }
            else if (pathPoints[m_points.back().m_point] != pathPoints[i] && pathPoints[m_points.back().m_point].distance(pathPoints[i]) >= MIN_DISTANCE_TO_ADD) {
                std::cout << "ashim: add new point " << pathPoints[i] << "\n";
                auto lastPointCopy = m_points.back(); // copy lazer point
                m_points.back() = PointData{ lastPointCopy.m_point, timePoint, lastPointCopy.getWidthForTime(timePoint, true) };
                lastPointCopy.m_point = i; // update position for lazer point
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
            const auto& pathPoints = m_pointStorage->getPathPoints();
            auto mainPointIndex = m_points.size() - 1;
            auto mainPointWidth = m_points[mainPointIndex].getWidthForTime(m_timePoint, !m_completeFlag);
            float widthStep = mainPointWidth / m_points.size();
            for (auto i = 0; i < m_points.size(); ++i) {
                const auto& p1 = pathPoints[m_points[i].m_point];
                if (i == mainPointIndex) {
                    // lazer point head (current finger position)
                    fn(p1, p1, mainPointWidth);
                }
                else {
                    // lazer point tail
                    const auto& p2 = pathPoints[m_points[i + 1].m_point];
                    fn(p1, p2, widthStep * i);
                }
            }
        }
    }

private:
    /*
    Use points storage to avoid copy
    */
    PathPointsStorage* m_pointStorage;
    std::deque<PointData> m_points;
    std::chrono::milliseconds m_timePoint; // last time when onTimeTick() was called
    bool m_completeFlag {false}; // user completed trace (mouse up)
};


#endif