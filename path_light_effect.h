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
#include "contour_transformator.h"

class PathLightEffect {

public:
    PathLightEffect() : 
        m_gradientTrace{ m_pathStorage },
        m_lazerTrace{ m_pathStorage },
        m_clusterStorage{ m_pathStorage },
        m_contourTransformator { &m_pathStorage }
    {

    }
    ~PathLightEffect() = default;

    void markToReset() {
        m_toResetFlag.store(true);
    }

    void markToComplete()
    {
        m_toCompleteFlag.store(true);
    }

    void addPoints(std::chrono::milliseconds timePoint, const std::vector<hmos::Point>& points, hmos::Ranges ranges)
    {
        if (m_toResetFlag.exchange(false)) {
            m_pathStorage = PathPointsStorage{};
            m_clusterStorage = ClusterStorage{ m_pathStorage };
            m_lazerTrace = LazerTrace{ m_pathStorage };
            m_gradientTrace = GradientTrace{ m_pathStorage };
            m_contourTransformator = std::move(hmos::ContourTransformator(&m_pathStorage));
            m_toCompleteFlag.store(false);
        }

        auto startIndex = m_pathStorage.getPathPoints().size();
        auto endIndex = startIndex + points.size() - 1;
        m_pathStorage.addPoints(points, ranges);
        m_lazerTrace.onPointsAdded(timePoint, startIndex, endIndex);
        m_clusterStorage.onPointsAdded(timePoint, startIndex, endIndex);
        if (m_toCompleteFlag) {
            startIndex = endIndex++;
            // m_pathStorage.addPoints({ m_pathStorage.getPathPoints().front() }, ranges);
            m_lazerTrace.onPointsAdded(timePoint, startIndex, startIndex);
            m_clusterStorage.onPointsAdded(timePoint, startIndex, startIndex);
        }
    }

    void onTimeTick(std::chrono::milliseconds timePoint)
    {
        if (m_toCompleteFlag) {
            m_lazerTrace.onComplete();
            m_clusterStorage.onComplete();
        }

        m_clusterStorage.onTimeTick(timePoint);
        m_lazerTrace.onTimeTick(timePoint);
    }

    void onDraw(GradientTraceDrawFn gradTraceDraw, DrawLaszerSegmentFn lazerFn, SparkDrawFn sparkFn)
    {
        if (!m_toCompleteFlag) {
            m_gradientTrace.onDraw(gradTraceDraw);
            m_lazerTrace.onDraw(lazerFn);
            m_clusterStorage.onDraw(sparkFn);
        } else {
            m_contourTransformator.Draw(
                [&]() {
                    std::cout << "DIMAS PathLightEffect::onDraw callback"  << std::endl;
                    m_gradientTrace.onDraw(gradTraceDraw);
                    m_lazerTrace.onDraw(lazerFn);
                    m_clusterStorage.onDraw(sparkFn);
                }
            );
            m_contourTransformator.Step();
        }
    }

    const std::vector<hmos::Point>& getPathPoints() const {
        return m_pathStorage.getPathPoints();
    }

    std::vector<std::pair<size_t, size_t>>  getDiapasons() const {
        // bool status;
        auto [status, diapasons] = m_contourTransformator.GetDiapasons();
        if (status) {
            return diapasons;
        } else {
            return {{ 0, getPathPoints().size() }};
        }
    }

private:
    /*
    Use points storage to avoid copy
    Note: the consumers store only indexes and expect that storage will be only grow!
    */
    PathPointsStorage m_pathStorage{};
    std::atomic<bool> m_toResetFlag{false};
    std::atomic<bool> m_toCompleteFlag{ false };

    
    ClusterStorage m_clusterStorage;
    LazerTrace m_lazerTrace;
    GradientTrace m_gradientTrace;
    
    hmos::ContourTransformator m_contourTransformator;
};


#endif