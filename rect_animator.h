#ifndef RECT_ANIMATOR_H
#define RECT_ANIMATOR_H

#include <vector>
#include <memory>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <algorithm>
#include <optional>


#include "path_points_storage.h"


static constexpr std::chrono::milliseconds ANIMATION_TTL{ 1200 }; // animation time

static constexpr std::chrono::milliseconds SHADOW_RECT_ANIMATION_TTL{ 2000 }; // animation time
static constexpr float OUT_START_ALPHA{ 230 };
static constexpr float OUT_END_ALPHA{ 160 };
static constexpr float IN_START_ALPHA{ 220 };
static constexpr float IN_END_ALPHA{ 50 };
static constexpr float CORNERS_LENGTH_FRACTION{ 0.1f };

struct ConicFragment {
    hmos::Point p0;
    hmos::Point p1;
    hmos::Point p2;
};

struct DrawRectInfo {
    hmos::Point lt{ 0.0f, 0.0f };
    hmos::Point rb{ 0.0f, 0.0f };
    uint8_t alpha{ 0xFF };

    std::array<ConicFragment, 4> corners;
};

using DrawRectAnimatorFn = std::function<void(const hmos::Point& c, const ConicFragment& leftTop, const ConicFragment& rightTop, const ConicFragment& leftBottom, const ConicFragment& rightBottom, float percentages)>;
using DrawRectShadowAnimatorFn = std::function<void(const DrawRectInfo& outRect, const DrawRectInfo& inRect)>;


class ShadowRectAnimator {

public:

    ShadowRectAnimator() = default;
    ~ShadowRectAnimator() = default;

    void start(std::chrono::milliseconds timePoint)
    {
        m_timePoint = timePoint;
        m_startTime = timePoint;
        m_isStarted = true;
    }

    void setDrawParams(const DrawRectInfo& _outRect, const DrawRectInfo& _inRect)
    {
        outRect = _outRect;
        inRect = _inRect;
    }


    void onTimeTick(std::chrono::milliseconds timePoint)
    {
        m_timePoint = timePoint;
    }

    void onDraw(DrawRectShadowAnimatorFn fn, std::array<ConicFragment, 4> corners)
    {
        if (fn && m_isStarted) {
            auto timePassed = std::min(m_timePoint - m_startTime, SHADOW_RECT_ANIMATION_TTL);
            std::cout << "ashim2: timePassed-> " << timePassed.count() << "\n";
            outRect.alpha = OUT_START_ALPHA + (OUT_END_ALPHA - OUT_START_ALPHA) / static_cast<float>(SHADOW_RECT_ANIMATION_TTL.count()) * static_cast<float>(timePassed.count());
            inRect.alpha = IN_START_ALPHA + (IN_END_ALPHA - IN_START_ALPHA) / static_cast<float>(SHADOW_RECT_ANIMATION_TTL.count()) * static_cast<float>(timePassed.count());
            inRect.corners = corners;
            fn(outRect, inRect);
        }
    }

private:
    /*
    Use points storage to avoid copy
    */
    PathPointsStorage* m_pointStorage;
    std::chrono::milliseconds m_startTime; // time animator was created
    std::chrono::milliseconds m_timePoint; // last time when onTimeTick() was called

    bool m_isStarted{ false };
    DrawRectInfo outRect;
    DrawRectInfo inRect;
};

class RectAnimator {

public:

    RectAnimator() = delete;
    RectAnimator(PathPointsStorage& pointStorage, int w, int h) :
        m_pointStorage(&pointStorage),
        m_width{ w },
        m_height{ h }
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

        if (m_isStarted) {
            if (!m_shadowAnimator) {
                auto timePassed = std::min(m_timePoint - m_startTime, ANIMATION_TTL);
                auto percentages = (static_cast<float>(timePassed.count()) / static_cast<float>(ANIMATION_TTL.count()));
                if (percentages >= 0.9) {
                    std::cout << "ashim2: create animator-> " << timePassed.count() << "\n";
                    m_shadowAnimator = ShadowRectAnimator{};
                    m_shadowAnimator->setDrawParams(DrawRectInfo{ {0, 0}, {m_width, m_height}, 0 }, DrawRectInfo{ left, right, 0 });
                    m_shadowAnimator->start(m_timePoint);
                }
            }
            else {
                std::cout << "ashim2: animator tick -> " << "\n";
                m_shadowAnimator->onTimeTick(timePoint);
            }
        }
    }

    void onDraw(DrawRectAnimatorFn fn, DrawRectShadowAnimatorFn fnShadow)
    {
        if (m_isStarted && fn && !m_pointStorage->empty()) {
            auto timePassed = std::min(m_timePoint - m_startTime, ANIMATION_TTL);

            auto w = right.x - left.x;
            auto h = right.y - left.y;

            float halfH = h / 2;
            float halfW = w / 2;

            float cornelLengh = std::min(halfH, halfW) * CORNERS_LENGTH_FRACTION;

            float wInc = ((w / 2 - cornelLengh) / static_cast<float>(ANIMATION_TTL.count())) * static_cast<float>(timePassed.count());
            float HInc = ((h / 2 - cornelLengh) / static_cast<float>(ANIMATION_TTL.count())) * static_cast<float>(timePassed.count());

            ConicFragment leftTop;
            ConicFragment rightTop;
            ConicFragment leftBottom;
            ConicFragment rightBottom;
            {
                leftTop.p0 = { left.x, std::max((h / 2 + left.y) - HInc, left.y + cornelLengh) };
                leftTop.p1 = { left.x, left.y };
                leftTop.p2 = { std::max((w / 2 + left.x) - wInc, left.x + cornelLengh), left.y };
            }

            {
                // left down
                leftBottom.p0 = { left.x, std::min((h / 2 + left.y) + HInc, left.y + h - cornelLengh) };
                leftBottom.p1 = { left.x, left.y + h };
                leftBottom.p2 = { std::max((w / 2 + left.x) - wInc, left.x + cornelLengh), left.y + h };
            }

            {
                // right up
                rightTop.p0 = { (left.x + w), std::max((h / 2 + left.y) - HInc, left.y + cornelLengh) };
                rightTop.p1 = { (left.x + w), left.y };
                rightTop.p2 = { std::min((w / 2 + left.x) + wInc, left.x + w - cornelLengh), left.y };
            }

            {
                // right down
                rightBottom.p0 = { (left.x + w), std::min((h / 2 + left.y) + HInc, left.y + h - cornelLengh) };
                rightBottom.p1 = { (left.x + w), left.y + h };
                rightBottom.p2 = { std::min((w / 2 + left.x) + wInc, left.x + w - cornelLengh), left.y + h };
            }

            auto percentages = (static_cast<float>(timePassed.count()) / static_cast<float>(ANIMATION_TTL.count()));
            std::array<ConicFragment, 4> corners{ leftTop, rightTop, rightBottom, leftBottom };
            if (m_shadowAnimator) {
                m_shadowAnimator->onDraw(fnShadow, corners);
            }
            fn({ left.x + halfW,  left.y + halfH }, leftTop, rightTop, leftBottom, rightBottom, percentages);
        }
    }

private:
    /*
    Use points storage to avoid copy
    */
    PathPointsStorage* m_pointStorage;
    std::chrono::milliseconds m_startTime; // time animator was created
    std::chrono::milliseconds m_timePoint; // last time when onTimeTick() was called

    int m_width{ 0 };
    int m_height{ 0 };

    bool m_isStarted{false};
    hmos::Point left{ 0.0f, 0.0f };
    hmos::Point right{ 0.0f, 0.0f };

    std::optional<ShadowRectAnimator> m_shadowAnimator;
};

#endif