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

std::vector<PosInt> pointsPath;

std::mutex gPointsMutex;
std::vector<PosInt> tmpPoints;
std::optional<PosInt> gLastPoint;

int gLastPointIndex = 0;
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
        //std::cout << "Pan started at (x:" << evt.x << ", y:" << evt.y << ")\n";
        panStarted = true;

        gPathLightEffect.clear();
        gLastPointIndex = 0;


        {
            std::unique_lock l(gPointsMutex);
            gLastPoint.reset();
            tmpPoints.clear();
            tmpPoints.reserve(1000);
        }

        pointsPath.clear();
        ranges = Ranges{evt.x, evt.y, evt.x, evt.y};
        pointsPath.reserve(1000);
        pointsPath.emplace_back(PosInt{evt.x, evt.y});
        break;
    }
    case SDL_MOUSEMOTION: {
        const SDL_MouseMotionEvent& evt = event.motion;
        //std::cout << "ashim: Pan updated to (x:" << evt.x << ", y:" << evt.y << ")\n";
        if (panStarted) {
            {
                std::unique_lock l(gPointsMutex);
                if (!gLastPoint || (distance(gLastPoint->x, gLastPoint->y, evt.x, evt.y) > 20.0)) {
                    gLastPoint = PosInt{ evt.x, evt.y };
                    tmpPoints.emplace_back(*gLastPoint);
                    ranges.minX = std::min(evt.x, ranges.minX);
                    ranges.minY = std::min(evt.y, ranges.minY);
                    ranges.maxX = std::max(evt.x, ranges.maxX);
                    ranges.maxY = std::max(evt.y, ranges.maxY);
                }
            }
        }
        break;
    }
    case SDL_MOUSEBUTTONUP: {
        const SDL_MouseButtonEvent& evt = event.button;
        std::cout << "Pan ended at (x:" << evt.x << ", y:" << evt.y << ")\n";
        panStarted = false;
        auto deltaX = static_cast<float>(ranges.maxX - ranges.minX);
        auto deltaY = static_cast<float>(ranges.maxY - ranges.minY);
        auto coeff = std::max(deltaX / deltaY, deltaY / deltaX);
        auto sz = static_cast<int>(pointsPath.size());
        if ((sz > 10)
        && (deltaX > 10.0 && deltaY > 10.0)
        && (coeff < 2.0)
        && (std::min(deltaX, deltaY) > (distance(pointsPath[0].x, pointsPath[0].y, pointsPath.rbegin()->x, pointsPath.rbegin()->y) / 10.0)))
        {
            pointsPath.push_back(pointsPath[0]);
            std::cout << "Closing path, pointsPath::size: " << pointsPath.size() << "\n";
        } else {
            std::cout << "Should clean path here \n";
            {
                std::unique_lock l(gPointsMutex);
                tmpPoints.clear();
                gLastPoint.reset();
            }
            pointsPath.clear();
            gPathLightEffect.clear();
            gLastPointIndex = 0;
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


    Pos positions[100] = {
        {0.0f, 0.0f}, {0.01f, 0.01f}, {0.02f, 0.02f}, {0.03f, 0.03f}, {0.04f, 0.04f},
        {0.05f, 0.05f}, {0.06f, 0.06f}, {0.07f, 0.07f}, {0.08f, 0.08f}, {0.09f, 0.09f},
        {0.1f, 0.1f}, {0.11f, 0.11f}, {0.12f, 0.12f}, {0.13f, 0.13f}, {0.14f, 0.14f},
        {0.15f, 0.15f}, {0.16f, 0.16f}, {0.17f, 0.17f}, {0.18f, 0.18f}, {0.19f, 0.19f},
        {0.2f, 0.2f}, {0.21f, 0.21f}, {0.22f, 0.22f}, {0.23f, 0.23f}, {0.24f, 0.24f},
        {0.25f, 0.25f}, {0.26f, 0.26f}, {0.27f, 0.27f}, {0.28f, 0.28f}, {0.29f, 0.29f},
        {0.3f, 0.3f}, {0.31f, 0.31f}, {0.32f, 0.32f}, {0.33f, 0.33f}, {0.34f, 0.34f},
        {0.35f, 0.35f}, {0.36f, 0.36f}, {0.37f, 0.37f}, {0.38f, 0.38f}, {0.39f, 0.39f},
        {0.4f, 0.4f}, {0.41f, 0.41f}, {0.42f, 0.42f}, {0.43f, 0.43f}, {0.44f, 0.44f},
        {0.45f, 0.45f}, {0.46f, 0.46f}, {0.47f, 0.47f}, {0.48f, 0.48f}, {0.49f, 0.49f},
        {0.5f, 0.5f}, {0.51f, 0.51f}, {0.52f, 0.52f}, {0.53f, 0.53f}, {0.54f, 0.54f},
        {0.55f, 0.55f}, {0.56f, 0.56f}, {0.57f, 0.57f}, {0.58f, 0.58f}, {0.59f, 0.59f},
        {0.6f, 0.6f}, {0.61f, 0.61f}, {0.62f, 0.62f}, {0.63f, 0.63f}, {0.64f, 0.64f},
        {0.65f, 0.65f}, {0.66f, 0.66f}, {0.67f, 0.67f}, {0.68f, 0.68f}, {0.69f, 0.69f},
        {0.7f, 0.7f}, {0.71f, 0.71f}, {0.72f, 0.72f}, {0.73f, 0.73f}, {0.74f, 0.74f},
        {0.75f, 0.75f}, {0.76f, 0.76f}, {0.77f, 0.77f}, {0.78f, 0.78f}, {0.79f, 0.79f},
        {0.8f, 0.8f}, {0.81f, 0.81f}, {0.82f, 0.82f}, {0.83f, 0.83f}, {0.84f, 0.84f},
        {0.85f, 0.85f}, {0.86f, 0.86f}, {0.87f, 0.87f}, {0.88f, 0.88f}, {0.89f, 0.89f},
        {0.9f, 0.9f}, {0.91f, 0.91f}, {0.92f, 0.92f}, {0.93f, 0.93f}, {0.94f, 0.94f},
        {0.95f, 0.95f}, {0.96f, 0.96f}, {0.97f, 0.97f}, {0.98f, 0.98f}, {0.99f, 0.99f}
        };

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
        // float period = 2000.0;
        // if (time > decltype(time){period}) {
        //     start = std::chrono::steady_clock::now(); //reset
        // }
        //time = std::clamp(time, decltype(time){0}, decltype(time){period});


        //SKIA start
        context->resetContext();
        //canvas->clear(SK_ColorTRANSPARENT);

         canvas->clear(SK_ColorBLACK);

        // Compile SkSL shader
        auto [effect, error] = SkRuntimeEffect::MakeForShader(SkString(pathShader.c_str()));
        // auto [effect, error] = SkRuntimeEffect::MakeForShader(SkString(skslCode.c_str()));
        if (!effect)
        {
            std::cerr << "Error compiling SkSL shader: " << error.c_str() << std::endl;
            return -1;
        }

        // Set shader uniforms
        SkRuntimeShaderBuilder builder(effect);
        builder.uniform("iResolution") = SkV2{w, h};
        builder.uniform("iTime") = time.count() / 1000;
        // builder.uniform("positions") = SkData::MakeWithCopy(positions, sizeof(positions));
        // builder.uniform("positions").set(positions, 50);

        // builder.uniform("u_resolution") = SkV2{w, h};
        // builder.uniform("u_time") = time.count() / period;

        //std::cout << "time: " << time.count() / 1000 << std::endl;
        // std::cout << "iTime: " << time.count() / period << std::endl;

        auto shader = builder.makeShader();

        SkScalar    s = SkIntToScalar(std::min(w, h));
        // static const SkPoint     kPts0[] = { { 0, 0 }, { s, s } };
        // static const SkPoint     kPts1[] = { { s/2, 0 }, { s/2, s } };
        // static const SkScalar    kPos[] = { 0, SK_Scalar1/2, SK_Scalar1 };
        // static const SkColor kColors0[] = {0x80F00080, 0xF0F08000, 0x800080F0 };
        // static const SkColor kColors1[] = {0xF08000F0, 0x8080F000, 0xF000F080 };
        // auto linearShader =  SkGradientShader::MakeLinear(kPts0, kColors0, kPos, std::size(kColors0), SkTileMode::kClamp);


        
        //     //line
        //     // path.moveTo(20, 300);
        //     // path.addCircle(400, 300, 200);

        // paint.setPathEffect(

        // paint.setMaskFilter(SkMaskFilter::MakeBlur(kSolid_SkBlurStyle, 20.0f));

        //auto blurStyle = kSolid_SkBlurStyle; // kSolid_SkBlurStyle
        //paint.setMaskFilter(SkMaskFilter::MakeBlur(blurStyle, 30.0f, false));

        {
            std::unique_lock l(gPointsMutex);
            pointsPath.insert(pointsPath.end(), tmpPoints.begin(), tmpPoints.end());
            tmpPoints.clear();
        }


        static auto COLOR_START = SK_ColorRED; // SkColorSetARGB(0xFF, 0xFF, 0xa5, 0x00);
        static auto COLOR_END = SK_ColorBLUE;

        auto calcColor = [](float full_length, float length) {
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


            std::cout << "ashim: length-> " << length << "\n";
            std::cout << "ashim: full_length-> " << full_length << "\n";
            std::cout << "ashim: r-> " << red << "\n";
            std::cout << "ashim: g-> " << green << "\n";
            std::cout << "ashim: b-> " << blue << "\n";
            return SkColorSetARGB(0xFF, static_cast<int>(red), static_cast<int>(green), static_cast<int>(blue));
        };

        auto makeGradienShader = [&calcColor](const hmos::Point& p1, const hmos::Point& p2, float p1Dist, float p2Dist, float full_length) {
            SkScalar positions[] = { 0.0, 1.0 };
            SkPoint pts[] = { {p1.x, p1.y}, {p2.x, p2.y} };
            SkColor colors_x[] = { calcColor(full_length, p1Dist), calcColor(full_length, p2Dist) };
            return SkGradientShader::MakeLinear(pts, colors_x, positions, 2, SkTileMode::kClamp, 0, NULL);
        };

        if (!pointsPath.empty()) {
            std::chrono::milliseconds curr_time{ static_cast<int>(time.count()) };
            for (auto i = gLastPointIndex; i < pointsPath.size(); i++) {
                gPathLightEffect.addPoint(curr_time, hmos::Point{ pointsPath[i].x,  pointsPath[i].y });
            }
            gLastPointIndex = pointsPath.size();
            gPathLightEffect.onTimeTick(curr_time);
        }


        // SkPath path3;
        // path3.moveTo(points[0]);
        // for (size_t i = 1; i < points.size(); ++i)
        // {
        //     path3.lineTo(points[i]);
        // }

        std::cout << "ashim: ======= DRAW CYCLE ======= " << "\n";
        if (!pointsPath.empty()) {
            SkPaint paint;
            paint.setMaskFilter(SkMaskFilter::MakeBlur(kSolid_SkBlurStyle, 20.0f));
            paint.setAntiAlias(true);
            paint.setStyle(SkPaint::kStroke_Style);
            paint.setStrokeCap(SkPaint::kRound_Cap);
            paint.setStrokeWidth(20);

            auto PATH_LENGTH = gPathLightEffect.getPathLength();
            std::cout << "ashim: PATH_LENGTH-> " << PATH_LENGTH << "\n";
            float dist = 0.0;
            for (int i = 0; i < pointsPath.size() - 1; ++i) {
                SkPath path3;
                hmos::Point p1{ pointsPath[i].x,  pointsPath[i].y };
                hmos::Point p2{ pointsPath[i+1].x,  pointsPath[i+1].y };
                auto p1Top2 = p1.distance(p2);
                std::cout << "ashim: points-> " << p1 << p2 << "\n";
                std::cout << "ashim: p1Top2-> " << p1Top2 << "\n";
                auto pathShader = makeGradienShader(p1, p2, dist, dist +  p1Top2, PATH_LENGTH);
                dist += p1Top2;
                paint.setShader(pathShader);
                path3.moveTo(p1.x, p1.y);
                path3.lineTo(p2.x, p2.y);
                canvas->drawPath(path3, paint);
            }
        }

        // Draw the path with the gradient
        //for (int i =0; i < 3; i++) 
        {
            // canvas->drawPath(path, paint);
            // canvas->drawPath(path2, paint);
            //canvas->drawPath(path3, paint);

        }
        /////////////////////

        ////////////////////
        if (0) {
            SkPaint lazerPaint;
            lazerPaint.setAntiAlias(true);
            lazerPaint.setColor(SkColorSetARGB(0xFF, 0x99, 0xff, 0xff));
            lazerPaint.setStrokeCap(SkPaint::kRound_Cap);
            //lazerPaint.setMaskFilter(SkMaskFilter::MakeBlur(kSolid_SkBlurStyle, 10.0f, true));
            auto lazerDraw = [&canvas, &lazerPaint](const hmos::Point& pStart, const hmos::Point& pEnd, float width) {
                std::cout << "ashim: draw point-> " << pStart << pEnd << "\n";
                std::cout << "ashim: draw width-> " << width << "\n";
                lazerPaint.setStrokeWidth(width);
                SkPath lazerPath;
                lazerPath.moveTo(pStart.x, pStart.y);
                lazerPath.lineTo(pEnd.x, pEnd.y);
                //lazerPath.arcTo({ pStart.x, pStart.y }, { pEnd.x, pEnd.y }, 100.0);
                //lazerPath.quadTo({ pStart.x, pStart.y }, { pEnd.x, pEnd.y });
                //lazerPath.conicTo({ pStart.x, pStart.y }, { pEnd.x, pEnd.y }, 3.0);
                if (pStart == pEnd) {
                    lazerPaint.setStyle(SkPaint::kFill_Style);
                    canvas->drawCircle(pStart.x, pStart.y, width/2, lazerPaint);
                } else {
                    lazerPaint.setStyle(SkPaint::kStroke_Style);
                    canvas->drawPath(lazerPath, lazerPaint);
                }
            };

            SkPaint sparkPaint;
            sparkPaint.setAntiAlias(true);
            sparkPaint.setStyle(SkPaint::kFill_Style);
            sparkPaint.setColor(SkColorSetARGB(0xFF, 0x99, 0xff, 0xff));
            sparkPaint.setMaskFilter(SkMaskFilter::MakeBlur(kSolid_SkBlurStyle, 10.0f, true));
            auto sparkDraw = [&canvas, &sparkPaint](const Spark& s, const hmos::Point& p) {
                canvas->drawCircle(p.x, p.y, s.getRadius(), sparkPaint);
            };

            gPathLightEffect.onDraw(lazerDraw, sparkDraw);
        }
        ///////////////////

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