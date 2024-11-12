#ifndef PATH_LIGHT_EFFECT_H
#define PATH_LIGHT_EFFECT_H

#include <vector>
#include <memory>
#include <chrono>
#include <cstdlib>
#include <algorithm>
#include <functional>
#include <iostream>
#include <atomic>


#include "point.h"
#include "lazer_trace.h"
#include "gradient_trace.h"
#include "spark.h"
#include "path_points_storage.h"

class PathLightEffect {

public:
    PathLightEffect() : 
        m_gradientTrace{ m_pathStorage }
    {

    }
    ~PathLightEffect() = default;

    void markClear() {
        m_toClearFlag.store(true);
    }

    void clear()
    {
        m_clusterStorage.clear();
        m_lazerTrace.clear();
        m_pathStorage.clear();
    }

    void addPoints(std::chrono::milliseconds timePoint, const std::vector<hmos::Point>& points)
    {
        if (m_toClearFlag.exchange(false)) {
            clear();
        }

        for(const auto& p: points) {
            m_clusterStorage.addPoint(timePoint, p);
            m_lazerTrace.addPoint(timePoint, p);
        }

        m_pathStorage.addPoints(points);

    }

    void onTimeTick(std::chrono::milliseconds timePoint)
    {
        m_clusterStorage.onTimeTick(timePoint);
        m_lazerTrace.onTimeTick(timePoint);
    }

    void onDraw(GradientTraceDrawFn gradTraceDraw, DrawLaszerSegmentFn lazerFn, SparkDrawFn sparkFn)
    {
        m_gradientTrace.onDraw(gradTraceDraw);
        m_lazerTrace.onDraw(lazerFn);
        m_clusterStorage.onDraw(sparkFn);
    }

    const std::vector<hmos::Point>& getPathPoints() const {
        return m_pathStorage.getPathPoints();
    }

private:
    PathPointsStorage m_pathStorage{};
    std::atomic<bool> m_toClearFlag{false};
    
    ClusterStorage m_clusterStorage;
    LazerTrace m_lazerTrace;
    GradientTrace m_gradientTrace;
};


#endif