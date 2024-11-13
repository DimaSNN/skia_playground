#ifndef SPARKS_H
#define SPARKS_H

#include <vector>
#include <memory>
#include <chrono>
#include <cstdlib>
#include <algorithm>
#include <functional>
#include <iostream>
#include <deque>


#include "path_points_storage.h"

class Spark;

using speed_vec_t = hmos::Point;
using radius_t = float;
using SparkDrawFn = std::function<void(const Spark&, const hmos::Point&)>;

static constexpr radius_t SPARK_MIN_RADIUS = 1.0f;
static constexpr radius_t SPARK_MAX_RADIUS = 3.0f;
static constexpr std::chrono::milliseconds SPARK_TTL{ 1000 }; // time to live for spark
static constexpr std::chrono::milliseconds SPARK_CREATE_INTERVAL{ 50 }; // create a spark per time
static constexpr int SPARK_CREATE_DEFAULT_CNT{ 10 }; // how many sparks to create if cluster is emty (cool effect)
static constexpr radius_t CLUSTER_RADIUS = 20;
static constexpr std::chrono::milliseconds CLUSTER_TTL{ 1000 }; // time to live for cluster
static constexpr radius_t SPARK_BURN_RADIUS = 10; // radius where spark may be created

namespace details {
    speed_vec_t generate_speed(speed_vec_t::speed_type_t max_speed = 20.0f);
    radius_t generate_spark_radius(radius_t min_radius, radius_t max_radius);
}

class Spark {
public:

    ~Spark() = default;
    Spark() = default;
    Spark(radius_t r, std::chrono::milliseconds t, hmos::Point p, speed_vec_t s)
    {
        m_point = std::move(p);
        m_speed = std::move(s);
        m_radius = std::move(r);
        m_burnTime = std::move(t);
    }

    hmos::Point calcPosition(std::chrono::milliseconds t) const
    {
        auto left = t - m_burnTime;
        return hmos::Point(m_point.x + (left.count() * m_speed.x), m_point.y + (left.count() * m_speed.y));
    }

    const radius_t& getRadius() const
    {
        return m_radius;
    }

    bool isAlive(std::chrono::milliseconds t) const
    {
        return (t - m_burnTime) < SPARK_TTL;
    }

    const std::chrono::milliseconds& burnTime() const {
        return m_burnTime;
    }

private:
    hmos::Point m_point{0.0f, 0.0f}; // initial position where the spark was created
    speed_vec_t m_speed{ 0.0f, 0.0f }; // speed vector
    radius_t m_radius{ 1.0f }; // spark circle radius
    std::chrono::milliseconds m_burnTime{0}; // when the spark was created
};

class SparkCluster {
public:
    ~SparkCluster() = default;
    
    SparkCluster() = default;
    SparkCluster(std::chrono::milliseconds t, size_t p)
    {
        m_point  = p;
        m_burnTime = std::move(t);
    }

    bool isAlive(std::chrono::milliseconds t) const
    {
        return (t - m_burnTime) < CLUSTER_TTL;
    }

    const std::deque<Spark>& Sparks() const {
        return m_sparks;
    }

    int shouldCreateSpark(std::chrono::milliseconds currentTime) const
    {
        std::chrono::milliseconds timePassed{ SPARK_CREATE_INTERVAL };
        auto sparksToCreate = SPARK_CREATE_DEFAULT_CNT;
        if (!m_sparks.empty()) {
            auto&& last_spark = m_sparks.back();
            timePassed = currentTime - last_spark.burnTime();
            if (timePassed < SPARK_CREATE_INTERVAL) {
                return 0;
            }
            sparksToCreate = timePassed / SPARK_CREATE_INTERVAL;
        }
        auto fraction = 1.0f - static_cast<float>(currentTime.count() - m_burnTime.count()) / static_cast<float>(CLUSTER_TTL.count());
        return sparksToCreate * fraction;
    }

    bool createSpark(int cnt, const hmos::Point& finger_pos, std::chrono::milliseconds currentTime, speed_vec_t finger_s)
    {
        for (auto i = 0; i < cnt; ++i) {
            radius_t spark_radius = details::generate_spark_radius(SPARK_MIN_RADIUS, SPARK_MAX_RADIUS);
            auto sparkSpeed = details::generate_speed(0.1f);
            sparkSpeed += finger_s;
            sparkSpeed *= 0.5f;
            m_sparks.emplace_back(spark_radius, currentTime, finger_pos, sparkSpeed);
        }
        return true;
    }

    bool checkForCross(const PathPointsStorage& storage, const SparkCluster& other) const
    {
        return checkForCross(storage, other.m_point);
    }

    bool checkForCross(const PathPointsStorage& pointStorage, size_t p2) const
    {
        const auto& pathPoints = pointStorage.getPathPoints();
        if (pathPoints[p2].distance(pathPoints[m_point]) < CLUSTER_RADIUS) {
            return true;
        }
        return false;
    }

    void cleanDiedSparks(std::chrono::milliseconds t) {
        auto it = std::find_if(m_sparks.begin(), m_sparks.end(), [&t](const Spark& s) {
            return s.isAlive(t);
        });
        if (it != m_sparks.end()) {
            m_sparks.erase(m_sparks.begin(), it);
        }
    }

    void updateTime(const std::chrono::milliseconds& t)
    {
        m_burnTime = t;
    }

    const std::chrono::milliseconds& getCreationTime() const {
        return m_burnTime;
    }

    size_t getPosition() const
    {
        return m_point;
    }

private:   
    std::chrono::milliseconds m_burnTime{ 0 }; // when the cluster was created
    std::deque<Spark> m_sparks; // created sparks which are exist (active)
    size_t m_point; // the cluster position (index in PathPointsStorage)
};

class ClusterStorage {
public:
    ClusterStorage() = delete;
    ClusterStorage(PathPointsStorage& pointStorage)
    {
         m_pointStorage = &pointStorage;
    }
    ~ClusterStorage() = default;

    void cleanupDeadClusters() {
        auto it = std::find_if(m_clusters.begin(), m_clusters.end(), [this](const SparkCluster& c) {
            return c.isAlive(m_timePoint);
        });
        if (it != m_clusters.end()) {
            m_clusters.erase(m_clusters.begin(), it);
        }

        for (auto&& c : m_clusters) {
            c.cleanDiedSparks(m_timePoint);
        }
    }

    void onTimeTick(std::chrono::milliseconds timePoint)
    {
        m_timePoint = timePoint;

        cleanupDeadClusters();

        if (!m_clusters.empty()) {
            if (!m_completeFlag) {
                m_clusters.back().updateTime(m_timePoint);
            }

            const auto& pathPoints = m_pointStorage->getPathPoints();
            for (auto i = 0; i < m_clusters.size(); ++i) {
                auto& cluster = m_clusters[i];
                auto sparksCnt = cluster.shouldCreateSpark(m_timePoint);
                if (sparksCnt) {
                    speed_vec_t s = details::generate_speed(0.01f);
                    if (i == m_clusters.size()-1 && m_clusters.size() > 2) {
                        auto& prevCluster = m_clusters[i-1];
                        auto timeLeft = cluster.getCreationTime() - prevCluster.getCreationTime();
                        timeLeft = std::max(timeLeft, std::chrono::milliseconds{1});
                        auto speedX = (pathPoints[cluster.getPosition()].x - pathPoints[prevCluster.getPosition()].x) / timeLeft.count();
                        auto speedY = (pathPoints[cluster.getPosition()].y - pathPoints[prevCluster.getPosition()].y) / timeLeft.count();
                        static constexpr float MAGIC_ADJUST = 60.0f;
                        s = { (speedX) / MAGIC_ADJUST , (speedY) / MAGIC_ADJUST }; // Try to normalize to 0.1
                    }
                    cluster.createSpark(sparksCnt, pathPoints[cluster.getPosition()], m_timePoint, s);
                }
            }
        }
    }

    void onPointsAdded(std::chrono::milliseconds timePoint, size_t startIndex, size_t endIndex)
    {
        const auto& pathPoints = m_pointStorage->getPathPoints();
        for (auto i = startIndex; i <= endIndex; ++i) {
            if (m_clusters.empty() || m_clusters.back().checkForCross(*m_pointStorage, i) == false) {
                // no clusters or we far from previous cluster - create new one
                m_clusters.emplace_back(timePoint, i);
            }
        }
    }

    void onDraw(SparkDrawFn fn) {
        if (fn && !m_completeFlag) {
            for (auto&& c : m_clusters) {
                for (auto s : c.Sparks()) {
                    if (s.isAlive(m_timePoint)) {
                        auto sparkPos = s.calcPosition(m_timePoint);
                        fn(s, sparkPos);
                    }
                }
            }
        }
    }

    void onComplete()
    {
        m_completeFlag = true;
    }

private:
    /*
    Use points storage to avoid copy
    */
    PathPointsStorage* m_pointStorage;
    std::deque<SparkCluster> m_clusters; // collection of clusters
    std::chrono::milliseconds m_timePoint; // last time when onTimeTick() was called
    bool m_completeFlag{ false }; // user completed trace (mouse up)
};

#endif