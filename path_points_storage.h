#ifndef PATH_POINTS_STORAGE_H
#define PATH_POINTS_STORAGE_H

#include <vector>
#include <memory>
#include <chrono>
#include <cstdlib>
#include <algorithm>
#include <functional>
#include <iostream>
#include <atomic>


#include "point.h"


class PathPointsStorage {

public:
    PathPointsStorage() = default;
    ~PathPointsStorage() = default;

    void clear()
    {
        std::vector<hmos::Point> tmp;
        std::swap(m_pathPoints, tmp);
        m_pathLength = 0.0;
    }

    void addPoints(const std::vector<hmos::Point>& points, hmos::Ranges ranges)
    {
        m_ranges = std::move(ranges);
                for (const auto& p: points) {
            if (!m_pathPoints.empty()) {
                m_pathLength += m_pathPoints.back().distance(p);
            }
            m_pathPoints.emplace_back(p);
        }
        
        // TBD: improve
        //m_pathPoints.insert(m_pathPoints.end(), points.begin(), points.end());
    }

    bool empty() const {
        return m_pathPoints.empty();
    }

    

    const std::vector<hmos::Point>& getPathPoints() const {
        return m_pathPoints;
    }

    float getPathLength() const {
        return m_pathLength;
    }

    const hmos::Ranges& getRanges() const {
        return m_ranges;
    }

    void swapStorage(std::vector<hmos::Point>& rhs) noexcept {
        std::swap(m_pathPoints, rhs);
    }

private:
    std::vector<hmos::Point> m_pathPoints;
    hmos::Ranges m_ranges;
    float m_pathLength = 0.0f;
};


#endif