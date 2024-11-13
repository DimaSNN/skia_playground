#include <SDL.h>
#include <SDL_events.h>

#include "include/core/SkCanvas.h"
#include "include/core/SkSurface.h"
#include "include/core/SkPaint.h"
#include "include/effects/SkRuntimeEffect.h"
#include "include/core/SkData.h"
#include <string>
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>

#include "include/gpu/ganesh/GrDirectContext.h"
#include "include/gpu/ganesh/gl/GrGLDirectContext.h"
#include "include/gpu/ganesh/gl/GrGLInterface.h"
#include "include/gpu/GpuTypes.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"
#include "include/effects/SkGradientShader.h"
#include "include/core/SkMaskFilter.h"
#include "include/core/SkBlurTypes.h"

#include "path_light_effect.h"

// #include "include/gpu/gl/GrGLInterface.h"
// #include "include/core/SkSurface.h"

float distance(float x1, float y1, float x2, float y2) {
    return std::sqrt(std::pow(x2 - x1, 2) + std::pow(y2 - y1, 2));
}


//for path
const std::string pathShader = R"(
    uniform float2 iResolution;
    //uniform float2 iMouse;
    uniform float iTime;

    //uniform float2 iResolution;
//uniform float2 iMouse;
//uniform float iTime;

    half4 main(float2 fragCoord) {
        float2 uv = fragCoord.xy / iResolution.xy;

        // Initialize the color to black
        half4 color = half4(0.0);

        // Number of sparks
        const int numSparks = 10;

        // Loop to create multiple sparks
        for (int i = 0; i < numSparks; i++) {
            // Random angle and radius for each spark
            float angle = 6.28318530718 * fract(sin(float(i) * 12.9898 + iTime) * 43758.5453);
            float radius = 0.9 * fract(sin(float(i) * 78.233 + iTime) * 43758.5453);

            // Adjust radius to avoid empty circle
            radius *= 0.4 *  fract(sin(float(i) * 93.233 + iTime) * 43758.5453);

            // Calculate spark position
            float2 sparkPos = half2(0.5, 0.5) + radius * float2(cos(angle), sin(angle));
            //float2 sparkPos = mouse + radius * float2(0.5, 0.5);

            // Calculate the intensity of the spark
            float sparkIntensity = smoothstep(0.001, 0.06, 0.045 - distance(uv, sparkPos));

            // Add the spark color
            //color += half4(1.0, 1.0, 0.0, 1.0) * sparkIntensity;
            vec3  rgb = 0.5 + cos(iTime + float3(0.0, 2.0, 4.0));
            color += half4(rgb, 1.0) * sparkIntensity;
        }

        return color;
    }
)";

hmos::Ranges ranges;
struct Pos{ float x; float y;};


std::mutex gPointsMutex;
std::vector<hmos::Point> gTmpPoints;
std::optional<hmos::Point> gLastPoint;
std::optional<hmos::Point> gFirstPoint;

PathLightEffect gPathLightEffect;





// Function to handle pan events
void handlePanEvent(SDL_Event &event)
{
    static bool panStarted = false;
    switch (event.type)
    {
    case SDL_MOUSEBUTTONDOWN: {
        const SDL_MouseButtonEvent& evt = event.button;
        //std::cout << "Pan started at (x:" << evt.x << ", y:" << evt.y << ")\n";
        panStarted = true;
        ranges = hmos::Ranges{ evt.x, evt.y, evt.x, evt.y }; //it's resetting here

        gPathLightEffect.markToReset();

        {
            std::unique_lock l(gPointsMutex);
            gTmpPoints.clear();
            gTmpPoints.reserve(1000);
            gFirstPoint = hmos::Point{ evt.x, evt.y };
            gLastPoint = gLastPoint;
            gTmpPoints.emplace_back(*gFirstPoint);
        }

        break;
    }
    case SDL_MOUSEMOTION: {
        const SDL_MouseMotionEvent& evt = event.motion;
        //std::cout << "ashim: Pan updated to (x:" << evt.x << ", y:" << evt.y << ")\n";
        if (panStarted) {
            auto p = hmos::Point{ evt.x, evt.y };
            std::unique_lock l(gPointsMutex);
            if (!gLastPoint || gLastPoint->distance(p) >= 10.0) {
                gLastPoint = p;
                gTmpPoints.emplace_back(p);
                ranges.minX = std::min(evt.x, ranges.minX);
                ranges.minY = std::min(evt.y, ranges.minY);
                ranges.maxX = std::max(evt.x, ranges.maxX);
                ranges.maxY = std::max(evt.y, ranges.maxY);
            }
        }
        break;
    }
    case SDL_MOUSEBUTTONUP: {
        const SDL_MouseButtonEvent& evt = event.button;
        std::cout << "Pan ended at (x:" << evt.x << ", y:" << evt.y << ")\n";
        panStarted = false;
        {
            std::unique_lock l(gPointsMutex);
            gLastPoint = hmos::Point{ evt.x, evt.y };
            // gTmpPoints.emplace_back(*gLastPoint);
            gPathLightEffect.markToComplete();
        }
        // ranges = hmos::Ranges{};
        break;
    }
    default:
        break;
    }
}

// Atomic flag to control the event polling thread
std::atomic<bool> running(true);

void PollEvents()
{
    SDL_Event event;
    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            // Handle events here
            switch(event.type) {
            case SDL_QUIT: 
                running = false;
                break;

            case SDL_MOUSEMOTION: /**< Mouse moved */
            case SDL_MOUSEBUTTONDOWN:        /**< Mouse button pressed */
            case SDL_MOUSEBUTTONUP:          /**< Mouse button released */
            // case SDL_MOUSEWHEEL:
                // std::cout << "SDL_MOUSE_X event.type:" << event.type << std::endl;
                handlePanEvent(event);
                break;
            }
        }
        // Sleep to prevent high CPU usage
        SDL_Delay(10);
    }
}

void draw(SkCanvas* canvas)
{
    // This fiddle draws 4 instances of a LinearGradient, demonstrating
    // how the local matrix affects the gradient as well as the flag
    // which controls unpremul vs premul color interpolation.


    SkPaint paint;

    //  (0xFF, 0xFF, 0x00) - yellow
    //  (0x00, 0x00, 0xFF) - blue
    static auto COLOR_START = SK_ColorYELLOW;
    static auto COLOR_END = SK_ColorBLUE;
    SkColor colors[] = { COLOR_START, COLOR_END };


    {
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(10);

        SkPoint p1 = { 0, 0 };
        SkPoint p2 = { 200, 0 };
        SkPoint p3 = { 200, 200 };
        SkPoint p4 = { 0, 200 };
        SkPoint p5 = { 150, 190 };

        auto calcColor = [](float length, float full_length) {
            auto rStart = SkColorGetR(COLOR_START);
            auto rEnd = SkColorGetR(COLOR_END);
            auto cDiff = (static_cast<int>(rEnd) - static_cast<int>(rStart));
            auto red = static_cast<int>(rStart) + (cDiff / full_length) * length;

            auto gStart = SkColorGetG(COLOR_START);
            auto gEnd = SkColorGetG(COLOR_END);
            cDiff = (static_cast<int>(gEnd) - static_cast<int>(gStart));
            auto green = static_cast<int>(gStart) + (cDiff / full_length) * length;

            auto bStart = SkColorGetB(COLOR_START);
            auto bEnd = SkColorGetB(COLOR_END);
            cDiff = (static_cast<int>(bEnd) - static_cast<int>(bStart));
            auto blue = static_cast<int>(bStart) + (cDiff / full_length) * length;


            //std::cout << "ashim: r-> " << red << "\n";
            std::cout << "ashim: g-> " << green << "\n";
            //std::cout << "ashim: b-> " << blue << "\n";
            return SkColorSetARGB(0xFF, static_cast<int>(red), static_cast<int>(green), static_cast<int>(blue));
        };

        {
            SkPath path2;
            SkScalar positions[] = { 0.0, 1.0 };
            SkPoint pts[] = { p1, p2 };
            SkColor colors_x[] = { calcColor(0, 600), calcColor(200, 600) };

            auto lgs = SkGradientShader::MakeLinear(pts, colors_x, positions, 2, SkTileMode::kClamp, 0, NULL);
            paint.setShader(lgs);

            path2.moveTo(p1);
            path2.lineTo(p2);

            canvas->drawPath(path2, paint);
        }

        {
            SkPath path2;
            SkScalar positions[] = { 0.0, 1.0 };
            SkPoint pts[] = { p2, p3 };
            SkColor colors_x[] = { calcColor(200, 600), calcColor(400, 600) };

            auto lgs = SkGradientShader::MakeLinear(pts, colors_x, positions, 2, SkTileMode::kClamp, 0, NULL);
            paint.setShader(lgs);

            path2.moveTo(p2);
            path2.lineTo(p3);

            canvas->drawPath(path2, paint);
        }

        {
            SkPath path2;
            SkScalar positions[] = { 0.0, 1.0 };
            SkPoint pts[] = { p3, p4 };
            SkColor colors_x[] = { calcColor(400, 600), calcColor(600, 600) };

            auto lgs = SkGradientShader::MakeLinear(pts, colors_x, positions, 2, SkTileMode::kClamp, 0, NULL);
            paint.setShader(lgs);

            path2.moveTo(p3);
            path2.lineTo(p4);

            canvas->drawPath(path2, paint);
        }

        {
            SkPaint dotPaint;
            dotPaint.setStyle(SkPaint::kFill_Style);
            dotPaint.setColor(SK_ColorRED);
            canvas->drawCircle(p1, 2, dotPaint);
            canvas->drawCircle(p2, 2, dotPaint);
            canvas->drawCircle(p3, 2, dotPaint);
            canvas->drawCircle(p4, 2, dotPaint);
            canvas->drawCircle(p5, 2, dotPaint);
        }
    }

}

int main(int argc, char *argv[])
{
    const int w =800;
    const int h =600;

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        return -1;
    }

    SDL_Window *window = SDL_CreateWindow("SDL Thread Example", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, SDL_WINDOW_SHOWN);
    if (!window)
    {
        SDL_Quit();
        return -1;
    }
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer)
    {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
    if (!texture)
    {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // Start the event polling thread
    std::thread eventThread(PollEvents);


    // Initialize OpenGL context
    auto interface = GrGLMakeNativeInterface();
    auto context = GrDirectContexts::MakeGL(interface);

    SkImageInfo info = SkImageInfo::MakeN32Premul(w, h);
    auto surface = SkSurfaces::Raster(info);
    // auto surface = SkSurfaces::RenderTarget(context.get(), skgpu::Budgeted::kNo, info, 0, nullptr); //with GPU
    SkCanvas *canvas = surface->getCanvas();

    auto start = std::chrono::steady_clock::now();

    auto GetCurrentTime = [start]() {
        return std::chrono::steady_clock::now() - start;
    };

    // Main loop
    while (running)
    {
        std::chrono::duration<float, std::milli> time{ GetCurrentTime ()};

        //SKIA start
        context->resetContext();
        canvas->clear(SK_ColorBLACK);

        std::vector<hmos::Point> newPoints;
        {
            std::unique_lock l(gPointsMutex);
            std::swap(newPoints, gTmpPoints);
        }


        if (!newPoints.empty()) {
            std::chrono::milliseconds curr_time{ static_cast<int>(time.count()) };
            gPathLightEffect.addPoints(curr_time, newPoints, ranges);
        }

        const auto& pointsPath = gPathLightEffect.getPathPoints();
        if (!pointsPath.empty()) {
            std::cout << "ashim: ======= DRAW CYCLE ======= " << "\n";

            std::chrono::milliseconds curr_time{ static_cast<int>(time.count()) };
            gPathLightEffect.onTimeTick(curr_time);

            SkPaint gradientTracePaint;
            gradientTracePaint.setMaskFilter(SkMaskFilter::MakeBlur(kSolid_SkBlurStyle, 20.0f));
            gradientTracePaint.setAntiAlias(true);
            gradientTracePaint.setStyle(SkPaint::kStroke_Style);
            gradientTracePaint.setStrokeCap(SkPaint::kRound_Cap);
            gradientTracePaint.setStrokeWidth(20);

            auto gradientTraceDraw = [&canvas, &gradientTracePaint](const hmos::Point& p1, const ColorType& c1, const hmos::Point& p2, const ColorType& c2, float width) {
                SkPath path;
                SkScalar positions[] = { 0.0, 1.0 };
                SkPoint pts[] = { {p1.x, p1.y}, {p2.x, p2.y} };
                SkColor colors[] = { c1, c2 };
                auto shader = SkGradientShader::MakeLinear(pts, colors, positions, 2, SkTileMode::kClamp, 0, NULL);
                gradientTracePaint.setShader(shader);
                path.moveTo(p1.x, p1.y);
                path.lineTo(p2.x, p2.y);
                canvas->drawPath(path, gradientTracePaint);
                canvas->drawPath(path, gradientTracePaint);
            };

            SkPaint lazerPaint;
            lazerPaint.setAntiAlias(true);
            lazerPaint.setColor(SkColorSetARGB(0xFF, 0x99, 0xff, 0xff));
            lazerPaint.setStrokeCap(SkPaint::kRound_Cap);
            lazerPaint.setStyle(SkPaint::kStroke_Style);
            lazerPaint.setMaskFilter(SkMaskFilter::MakeBlur(kSolid_SkBlurStyle, 20.0f, true));
            auto lazerDraw = [&canvas, &lazerPaint](const hmos::Point& pStart, const hmos::Point& pEnd, float width) {
                std::cout << "ashim: draw point-> " << pStart << pEnd << "\n";
                std::cout << "ashim: draw width-> " << width << "\n";
                lazerPaint.setStrokeWidth(width);
                SkPath lazerPath;
                lazerPath.moveTo(pStart.x, pStart.y);
                lazerPath.lineTo(pEnd.x, pEnd.y);
                canvas->drawPath(lazerPath, lazerPaint);
            };

            SkPaint sparkPaint;
            sparkPaint.setAntiAlias(true);
            sparkPaint.setStyle(SkPaint::kFill_Style);
            sparkPaint.setColor(SkColorSetARGB(0xFF, 0x99, 0xff, 0xff));
            sparkPaint.setMaskFilter(SkMaskFilter::MakeBlur(kSolid_SkBlurStyle, 10.0f, true));
            auto sparkDraw = [&canvas, &sparkPaint](const Spark& s, const hmos::Point& p) {
                canvas->drawCircle(p.x, p.y, s.getRadius(), sparkPaint);
            };

            gPathLightEffect.onDraw(gradientTraceDraw, lazerDraw, sparkDraw);
            //gPathLightEffect.onDraw(gradientTraceDraw, nullptr, nullptr);

            if (1) {
                // Draw points
                SkPaint sparkPaint;
                sparkPaint.setAntiAlias(true);
                sparkPaint.setStyle(SkPaint::kFill_Style);
                sparkPaint.setColor(SK_ColorWHITE);
                for (int i = 0; i < pointsPath.size(); ++i) {
                    canvas->drawCircle(pointsPath[i].x, pointsPath[i].y, 2, sparkPaint);
                }
            }
        }
       
        

        //draw(canvas);
        //std::exit(0);
        


        context->flushAndSubmit();

        //copy pixels
        void *pixels;
        int pitch;
        if (SDL_LockTexture(texture, NULL, &pixels, &pitch) == 0) {
            // Here you would update the pixels with your canvas data
            // For example, filling the texture with a solid color
            //   memset(pixels, 255, pitch * SCREEN_HEIGHT); // Fill with white color
            surface->readPixels(info, pixels, pitch, 0, 0);
            SDL_UnlockTexture(texture);
        }
        //context->freeGpuResources();
        // context->resetContext();

        // Clear screen
        SDL_RenderClear(renderer);

        // Render texture to screen
        SDL_RenderCopy(renderer, texture, NULL, NULL);

        // Update screen
        SDL_RenderPresent(renderer);


        // Update game state, render, etc.
        SDL_Delay(100); 
    }

    // Clean up
    eventThread.join();
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}