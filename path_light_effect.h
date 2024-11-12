#ifndef PATH_LIGHT_EFFECT_H
#define PATH_LIGHT_EFFECT_H

#include <vector>
#include <memory>
#include <chrono>
#include <cstdlib>
#include <algorithm>
#include <functional>
#include <iostream>
#include <deque>
#include <optional>


#include "point.h"
#include "lazer_trace.h"
#include "spark.h"


class PathLightEffect {

public:
    PathLightEffect() = default;
    ~PathLightEffect() = default;

    void clear()
    {
        m_clusterStorage.clear();
        m_lazerTrace.clear();
        m_pathLength = 0.0f;
        m_lastPoint = std::optional<hmos::Point>{};
    }

    void addPoint(std::chrono::milliseconds timePoint, const hmos::Point& currPoint)
    {
        m_clusterStorage.addPoint(timePoint, currPoint);
        m_lazerTrace.addPoint(timePoint, currPoint);

        if (!m_lastPoint) {
            m_lastPoint = currPoint;
            return;
        }
        m_pathLength += m_lastPoint->distance(currPoint);
        m_lastPoint = currPoint;
    }

    void onTimeTick(std::chrono::milliseconds timePoint)
    {
        m_clusterStorage.onTimeTick(timePoint);
        m_lazerTrace.onTimeTick(timePoint);
    }

    void onDraw(DrawLaszerSegmentFn lazerFn, SparkDrawFn sparkFn)
    {
        m_lazerTrace.onDraw(lazerFn);
        m_clusterStorage.onDraw(sparkFn);
    }

    float getPathLength()
    {
        return m_pathLength;
    }

private:
    ClusterStorage m_clusterStorage;
    LazerTrace m_lazerTrace;

    std::optional<hmos::Point> m_lastPoint;
    float m_pathLength;
};


#endif