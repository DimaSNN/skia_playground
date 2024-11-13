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
#include "rect_animator.h"

class PathLightEffect {

public:
    PathLightEffect() : 
        m_gradientTrace{ m_pathStorage },
        m_lazerTrace{ m_pathStorage },
        m_clusterStorage{ m_pathStorage },
        m_rectAnimator{ m_pathStorage }
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

    void addPoints(std::chrono::milliseconds timePoint, const std::vector<hmos::Point>& points)
    {
        if (m_toResetFlag.exchange(false)) {
            m_pathStorage = PathPointsStorage{};
            m_clusterStorage = ClusterStorage{ m_pathStorage };
            m_lazerTrace = LazerTrace{ m_pathStorage };
            m_gradientTrace = GradientTrace{ m_pathStorage };
            m_rectAnimator = RectAnimator{ m_pathStorage };
            m_toCompleteFlag.store(false);
        }

        auto startIndex = m_pathStorage.getPathPoints().size();
        auto endIndex = startIndex + points.size() - 1;
        m_pathStorage.addPoints(points);
        m_lazerTrace.onPointsAdded(timePoint, startIndex, endIndex);
        m_clusterStorage.onPointsAdded(timePoint, startIndex, endIndex);
        m_rectAnimator.onPointsAdded(timePoint, startIndex, endIndex);
        if (m_toCompleteFlag) {
            startIndex = endIndex++;
            m_pathStorage.addPoints({ m_pathStorage.getPathPoints().front() });
            m_lazerTrace.onPointsAdded(timePoint, startIndex, startIndex);
            m_clusterStorage.onPointsAdded(timePoint, startIndex, startIndex);
            m_rectAnimator.onPointsAdded(timePoint, startIndex, startIndex);
        }
    }

    void onTimeTick(std::chrono::milliseconds timePoint)
    {
        if (m_toCompleteFlag.exchange(false)) {
            m_lazerTrace.onComplete();
            m_clusterStorage.onComplete();
            m_gradientTrace.onComplete();
            m_rectAnimator.start(timePoint);
        }

        m_gradientTrace.onTimeTick(timePoint);
        m_clusterStorage.onTimeTick(timePoint);
        m_lazerTrace.onTimeTick(timePoint);
        m_rectAnimator.onTimeTick(timePoint);
    }

    void onDraw(GradientTraceDrawFn gradTraceDraw, DrawLaszerSegmentFn lazerFn, SparkDrawFn sparkFn, DrawRectAnimatorFn rectAnimatorFn)
    {
        m_gradientTrace.onDraw(gradTraceDraw);
        m_lazerTrace.onDraw(lazerFn);
        m_clusterStorage.onDraw(sparkFn);
        m_rectAnimator.onDraw(rectAnimatorFn);
    }

    const std::vector<hmos::Point>& getPathPoints() const {
        return m_pathStorage.getPathPoints();
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
    RectAnimator m_rectAnimator;
};


#endif