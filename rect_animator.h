#ifndef RECT_ANIMATOR_H
#define RECT_ANIMATOR_H

#include <vector>
#include <memory>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <algorithm>


#include "path_points_storage.h"


static constexpr std::chrono::milliseconds ANIMATION_TTL{ 1400 }; // animation time

struct ConicFragment {
    hmos::Point p0;
    hmos::Point p1;
    hmos::Point p2;
};

using DrawRectAnimatorFn = std::function<void(const hmos::Point& c, const ConicFragment& leftTop, const ConicFragment& rightTop, const ConicFragment& leftBottom, const ConicFragment& rightBottom)>;


class RectAnimator {

public:

    RectAnimator() = delete;
    RectAnimator(PathPointsStorage& pointStorage) :
        m_pointStorage(&pointStorage)
    {

    }
    ~RectAnimator() = default;

    void start(std::chrono::milliseconds timePoint)
    {
        m_startTime = timePoint;
        m_isStarted = true;
    }


    void onPointsAdded(std::chrono::milliseconds timePoint, size_t startIndex, size_t endIndex)
    {
        for (auto i = startIndex; i <= endIndex; ++i) {
            left.x = startIndex == 0 ? (*m_pointStorage)[i].x : std::min((*m_pointStorage)[i].x, left.x);
            left.y = startIndex == 0 ? (*m_pointStorage)[i].y : std::min((*m_pointStorage)[i].y, left.y);
            right.x = startIndex == 0 ? (*m_pointStorage)[i].x : std::max((*m_pointStorage)[i].x, right.x);
            right.y = startIndex == 0 ? (*m_pointStorage)[i].y : std::max((*m_pointStorage)[i].y, right.y);
        }
    }

    void onTimeTick(std::chrono::milliseconds timePoint) {
        if (m_pointStorage->empty()) {
            return; 
        }

        m_timePoint = timePoint;
    }

    void onDraw(DrawRectAnimatorFn fn)
    {
        if (m_isStarted && fn && !m_pointStorage->empty()) {
            auto timePassed = std::min(m_timePoint - m_startTime, ANIMATION_TTL);

            auto x = left.x;
            auto y = left.y;

            auto w = right.x - left.x;
            auto h = right.y - left.y;

            float qH = h / 2;
            float qW = w / 2;

            float l = std::min(qH, qW) / 4;

            float wInc = ((w / 2 - l) / static_cast<float>(ANIMATION_TTL.count())) * static_cast<float>(timePassed.count());
            float HInc = ((h / 2 - l) / static_cast<float>(ANIMATION_TTL.count())) * static_cast<float>(timePassed.count());

            std::cout << "ashim: wInc-> " << wInc << "   timePassed: " << timePassed.count() << "\n";


            ConicFragment leftTop;
            ConicFragment rightTop;
            ConicFragment leftBottom;
            ConicFragment rightBottom;
            {
                // left up
                auto _y = std::max((h / 2 + y) - HInc, y + l);
                auto _x = std::max((w / 2 + x) - wInc, x + l);
                leftTop.p0 = { x, _y };
                leftTop.p1 = { x, y };
                leftTop.p2 = { _x, y };
            }

            {
                // left down
                auto _y = std::min((h / 2 + y) + HInc, y + h - l);
                auto _x = std::max((w / 2 + x) - wInc, x + l);
                leftBottom.p0 = { x, _y };
                leftBottom.p1 = { x, y + h };
                leftBottom.p2 = { _x, y + h };
            }

            {
                // right up
                auto _y = std::max((h / 2 + y) - HInc, y + l);
                auto _x = std::min((w / 2 + x) + wInc, x + w - l);
                rightTop.p0 = { (x + w), _y };
                rightTop.p1 = { (x + w), y };
                rightTop.p2 = { _x, y };
            }

            {
                // right down
                auto _y = std::min((h / 2 + y) + HInc, y + h - l);
                auto _x = std::min((w / 2 + x) + wInc, x + w - l);

                rightBottom.p0 = { (x + w), _y };
                rightBottom.p1 = { (x + w), y + h };
                rightBottom.p2 = { _x, y + h };
            }

            fn({ x + qW,  y + qH }, leftTop, rightTop, leftBottom, rightBottom);
        }
    }

private:
    /*
    Use points storage to avoid copy
    */
    PathPointsStorage* m_pointStorage;
    std::chrono::milliseconds m_startTime; // time animator was created
    std::chrono::milliseconds m_timePoint; // last time when onTimeTick() was called

    struct Ranges { int minX = 0, minY = 0, maxX = 0, maxY = 0; };
    Ranges ranges;

    bool m_isStarted{false};
    hmos::Point left{ 0.0f, 0.0f };
    hmos::Point right{ 0.0f, 0.0f };
};


#endif