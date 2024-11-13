#include <SDL.h>
#include <SDL_events.h>

#include "include/effects/SkDashPathEffect.h"
#include "include/core/SkPathEffect.h"
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

// const std::string skslCode = R"(
//         uniform vec2 iResolution;
//         uniform float iTime;
//         mediump vec4 main(vec2 drawing_coord)
//         {
//             // return vec4(1,0,0,1);

//             float pct = 0.0;
//             vec2 uv = drawing_coord.xy / iResolution.xy;
//             vec2 tC = vec2(0.5)-uv;
//             pct = sqrt(tC.x*tC.x+tC.y*tC.y);
//             float alpha = smoothstep(0.5, 0.0, pct)+0.2;
//             float cl = pct*0.5;
//             vec4 fragColor = vec4(vec3(cl), alpha);
//             return fragColor;

//         }
//     )";


//magic line
const std::string skslCode = R"(
uniform float2 iResolution;
uniform float iTime;
// uniform half2 positions[50];


half4 main(float2 fragCoord) {
	float2 uv = fragCoord.xy / iResolution.xy;
	float2 center = float2(0.5, 0.5); // Center of the screen
	float circleRadius = 0.4; // Radius of the circular path

	// Initialize the color to black
	half4 color = half4(0.0);

	// Number of sparks
	const int numSparks = 100;


	  
	// Loop to create multiple sparks along the circular path
	for (int i = 0; i < numSparks; i++) {
		float radius = 0.2 * fract(sin(float(i) * 78.233 + iTime) * 43758.5453);
		// Adjust radius to avoid empty circle
		radius *= fract(sin(float(i) * 93.233 + iTime) * 42758.5453);
		// Calculate angle for each spark along the circle
		float angle = 6.28318 * float(i) / float(numSparks);
		float2 tmpCenter = center;
		//tmpCenter += float2(radius, radius);
		float2 sparkPos = tmpCenter + circleRadius * float2(cos(angle + iTime),  sin(angle + iTime));

		  
		// Calculate the intensity of the spark
		float sparkIntensity = smoothstep(0.0001, 0.08, 0.05 - distance(uv, sparkPos));
		//float sparkIntensity = smoothstep(0.0011, 0.005, 0.005 - distance(uv, sparkPos));

		// Generate random color for the spark with rainbow effect
		float hue = fract(float(i) / float(numSparks) + iTime );
		half3 rgb = half3(0.7 + 0.5 * cos(6.245646 * hue + float3(0.0, 2.0, 4.0)));

		// Add the spark color
		color += half4(rgb, 1.0) * sparkIntensity;
	}

	return color;
}
)";

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

// const std::string skslCode = R"(
//     // SkSL code for animated darkening towards the center
//     uniform float2 iResolution; // Canvas resolution
//     uniform float iTime;        // Time variable for animation

//     half4 main(float2 fragCoord) {
//     // Normalize coordinates
//     float2 uv = float2(fragCoord.xy / iResolution.xy);
//     uv = uv * 2.0 - 1.0; // Transform to range [-1, 1]

//     // Calculate distance from center
//     float dist = 1.0 - length(uv) * length(uv);

//     // Brightening factor with animation
//     float brighten = 1 * dist * smoothstep(0.0, 1.0, iTime);

//     // Apply brightening effect
//     return half4(1.0 - brighten, 1.0 - brighten, 1.0 - brighten, 0.5);
//     }
// )";
struct Pos{ float x; float y;};
struct PosInt{ int x; int y;};


std::mutex gPointsMutex;
std::vector<hmos::Point> gTmpPoints;
std::optional<hmos::Point> gLastPoint;
std::optional<hmos::Point> gFirstPoint;

int gMouseClick = 0;

PathLightEffect gPathLightEffect;

struct Ranges{int minX=0, minY=0, maxX=0, maxY=0;};
Ranges ranges;



// Function to handle pan events
void handlePanEvent(SDL_Event &event)
{
    static bool panStarted = false;
    switch (event.type)
    {
    case SDL_MOUSEBUTTONDOWN: {
        const SDL_MouseButtonEvent& evt = event.button;
        gMouseClick++;
        //std::cout << "Pan started at (x:" << evt.x << ", y:" << evt.y << ")\n";
        panStarted = true;
        ranges = Ranges{ evt.x, evt.y, evt.x, evt.y };

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
        gMouseClick++;
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
            gTmpPoints.emplace_back(*gLastPoint);
            gPathLightEffect.markToComplete();
        }
        ranges = Ranges{};
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

        SkPoint p1 = { 10, 10 };
        SkPoint p2 = { 110, 110 };
        
        {
            SkScalar positions[] = { 0.0, 1.0 };
            SkPoint pts[] = { p1, p2 };
            SkColor colors_x[] = { SK_ColorYELLOW, SK_ColorBLUE };
            //auto lgs = SkGradientShader::MakeLinear(pts, colors_x, positions, 2, SkTileMode::kClamp, 0, NULL);
            //paint.setShader(lgs);


            auto x = 50;
            auto y = 50;

            auto w = 400;
            auto h = 100;

            SkRect rect = SkRect::MakeXYWH(x, y, w, h);
            SkPath rectPath;

            float qH = h / 2;
            float qW = w / 2;

            float l = std::min(qH, qW) / 4;
            
            float wInc = gMouseClick + 1.0;
            float HInc = gMouseClick + 1.0;

            float conicW = 0.9f;

            {
                // left up
                SkPath p;
                auto _y = std::max((h / 2 + y) - HInc, y + l);
                auto _x = std::max((w / 2 + x) - wInc, x + l);
                rectPath.moveTo(x, _y);
                rectPath.conicTo({ x, y }, { _x, y }, conicW);
                //canvas->drawPath(p, paint);
            }

            {
                // left down
                SkPath p;
                auto _y = std::min((h / 2 + y) + HInc, y + h - l);
                auto _x = std::max((w / 2 + x) - wInc, x + l);
                rectPath.moveTo(x, _y);
                rectPath.conicTo({ x, y + h }, { _x, y + h }, conicW);
                //canvas->drawPath(p, paint);
            }

            {
                // right up
                SkPath p;
                
                auto _y = std::max((h / 2 + y) - HInc, y + l);
                auto _x = std::min((w / 2 + x) + wInc, x + w - l);

                rectPath.moveTo((x + w), _y);
                rectPath.conicTo({ (x + w), y }, { _x, y }, conicW);
                //canvas->drawPath(p, paint);
            }

            {
                // right down
                SkPath p;

                auto _y = std::min((h / 2 + y) + HInc, y + h - l);
                auto _x = std::min((w / 2 + x) + wInc, x + w - l);

                rectPath.moveTo((x + w), _y);
                rectPath.conicTo({ (x + w), y + h }, { _x, y + h }, conicW);
                //canvas->drawPath(p, paint);
            }

            //rectPath.addRoundRect(rect, 10, 10);

            // Dash effect
           // SkScalar intervals[] = { 40, 60 };
            float fraction = 1.0 - static_cast<float>(gMouseClick) / 10;
            fraction = 1.0;
            float quart = (h+w)/2;
            auto vis = quart * fraction;
            auto invis = quart - vis;


            SkColor colors[4] = { SK_ColorRED, SK_ColorGREEN, SK_ColorYELLOW, SK_ColorBLUE };
            paint.setShader(SkGradientShader::MakeSweep(x+w/2.0f, y+h/2, colors, nullptr, 4, 0, nullptr));
            paint.setMaskFilter(SkMaskFilter::MakeBlur(kSolid_SkBlurStyle, 15.0f));
            canvas->drawPath(rectPath, paint);
            canvas->drawPath(rectPath, paint);

            {
                SkPaint sparkPaint;
                sparkPaint.setAntiAlias(true);
                sparkPaint.setStyle(SkPaint::kFill_Style);
                sparkPaint.setColor(SkColorSetARGB(0xFF, 0x99, 0xff, 0xff));
                canvas->drawCircle({ 50,50 }, 2, sparkPaint);
                canvas->drawCircle({ 70,50 }, 2, sparkPaint);
                canvas->drawCircle({ 100,50 }, 2, sparkPaint);
                canvas->drawCircle({ 130,50 }, 2, sparkPaint);
                canvas->drawCircle({ 150,50 }, 2, sparkPaint);
            }
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
            gPathLightEffect.addPoints(curr_time, newPoints);
        }

        const auto& pointsPath = gPathLightEffect.getPathPoints();
        if (!pointsPath.empty()) {
            std::cout << "ashim: ======= DRAW CYCLE ======= " << "\n";

            std::chrono::milliseconds curr_time{ static_cast<int>(time.count()) };
            gPathLightEffect.onTimeTick(curr_time);

            SkPaint gradientTracePaint;
            gradientTracePaint.setAntiAlias(true);
            gradientTracePaint.setStyle(SkPaint::kStroke_Style);
            gradientTracePaint.setStrokeCap(SkPaint::kRound_Cap);
            gradientTracePaint.setStrokeWidth(5);
            gradientTracePaint.setMaskFilter(SkMaskFilter::MakeBlur(kSolid_SkBlurStyle, 10.0f));

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
            lazerPaint.setMaskFilter(SkMaskFilter::MakeBlur(kSolid_SkBlurStyle, 10.0f, true));
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

            SkPaint rectAnimatorPaint;
            rectAnimatorPaint.setAntiAlias(true);
            rectAnimatorPaint.setStyle(SkPaint::kStroke_Style);
            rectAnimatorPaint.setStrokeWidth(5);
            rectAnimatorPaint.setColor(SkColorSetARGB(0xFF, 0x99, 0xff, 0xff));
            rectAnimatorPaint.setMaskFilter(SkMaskFilter::MakeBlur(kSolid_SkBlurStyle, 10.0f, true));

            auto rectAnimatorDraw = [&canvas, &rectAnimatorPaint](const hmos::Point& c, const ConicFragment& leftTop, const ConicFragment& rightTop, const ConicFragment& leftBottom, const ConicFragment& rightBottom) {
                SkPath path;
                
                path.moveTo({ leftTop.p0.x, leftTop.p0.y });
                path.conicTo({ leftTop.p1.x, leftTop.p1.y }, { leftTop.p2.x, leftTop.p2.y }, 0.9f);
                
                path.moveTo({ leftBottom.p0.x, leftBottom.p0.y });
                path.conicTo({ leftBottom.p1.x, leftBottom.p1.y }, { leftBottom.p2.x, leftBottom.p2.y }, 0.9f);

                path.moveTo({ rightTop.p0.x, rightTop.p0.y });
                path.conicTo({ rightTop.p1.x, rightTop.p1.y }, { rightTop.p2.x, rightTop.p2.y }, 0.9f);

                path.moveTo({ rightBottom.p0.x, rightBottom.p0.y });
                path.conicTo({ rightBottom.p1.x, rightBottom.p1.y }, { rightBottom.p2.x, rightBottom.p2.y }, 0.9f);
                
                {
                    SkColor colors[4] = { SK_ColorRED, SK_ColorGREEN, SK_ColorYELLOW, SK_ColorBLUE };
                    rectAnimatorPaint.setShader(SkGradientShader::MakeSweep(c.x, c.y, colors, nullptr, 4, 0, nullptr));
                }

                canvas->drawPath(path, rectAnimatorPaint);
                canvas->drawPath(path, rectAnimatorPaint);
            };

            gPathLightEffect.onDraw(gradientTraceDraw, lazerDraw, sparkDraw, rectAnimatorDraw);
            //gPathLightEffect.onDraw(gradientTraceDraw, nullptr, nullptr);

            if (0) {
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