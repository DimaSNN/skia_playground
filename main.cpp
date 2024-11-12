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
#include <limits>
#include <mutex>

#include "include/gpu/ganesh/GrDirectContext.h"
#include "include/gpu/ganesh/gl/GrGLDirectContext.h"
#include "include/gpu/ganesh/gl/GrGLInterface.h"
#include "include/gpu/GpuTypes.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"
#include "include/effects/SkGradientShader.h"
#include "include/core/SkMaskFilter.h"
#include "include/core/SkBlurTypes.h"
// #include "include/gpu/gl/GrGLInterface.h"
// #include "include/core/SkSurface.h"

// Atomic flag to control the event polling thread
std::atomic<bool> running(true);
std::mutex mainLoopMutex;

float distance(float x1, float y1, float x2, float y2) {
    return std::sqrt(std::pow(x2 - x1, 2) + std::pow(y2 - y1, 2));
}

template <typename T>
T midpoint(const T& a, const T& b) {
    return a + (b - a) / 2;
}

std::vector<std::pair<size_t, size_t>> prepareDiapasons(size_t vectorSize)
{
    size_t partSize = vectorSize / 4;
    size_t remainder = vectorSize % 4;

    std::vector<std::pair<size_t, size_t>> diapasons(4);
    size_t startIndex = 0;

    for (int i = 0; i < 4; ++i)
    {
        size_t endIndex = startIndex + partSize + (i < remainder ? 1 : 0);
        diapasons[i] = {startIndex, endIndex};
        startIndex = endIndex;
    }

    return diapasons;
}

std::pair<std::vector<SkPoint>, std::vector<std::pair<size_t, size_t>>>  generateCornersPoints(float x1, float y1, float x2, float y2, int n)
{
    if (n < 12) {
        std::cout << "requested too small points n \n";
        return {}; 
    }

    std::vector<SkPoint> points;
    points.reserve(n);
    const float zoom = 1.7;

    float centerX = (x1 + x2) / 2;
    float centerY = (y1 + y2) / 2;
    float radiusX = std::abs(x2 - x1) / 2;
    float radiusY = std::abs(y2 - y1) / 2;
    radiusX *= zoom;
    radiusY *= zoom;

    printf("centerX %.4f \n", centerX);
    printf("centerY %.4f \n", centerY);
    printf("radiusX %.4f \n", radiusX);
    printf("radiusY %.4f \n", radiusY);


    const auto intervals = prepareDiapasons(n);
    const float length = 40.0;
    float offset = length;
    const float div  = (float) n / 4;
    for (int i = intervals[0].first; i < intervals[0].second; ++i) {
        int mid = midpoint(intervals[0].first, intervals[0].second);
        float x,y;
        if (i == mid) {
            x = centerX - radiusX;
            y = centerY - radiusY;
        } else if (i < mid) {
            x = centerX - radiusX;
            y = centerY - radiusY + offset;
            offset -= offset / div;
        } else {
            x = centerX - radiusX + offset;
            y = centerY - radiusY;
            offset += offset / div;
        }
        points.emplace_back(SkPoint::Make(x, y));
    }
    offset = length;
    for (int i = intervals[1].first; i < intervals[1].second; ++i) {
        int mid =  midpoint(intervals[1].first, intervals[1].second);
        float x,y;
        if (i == mid) {
            x = centerX + radiusX;
            y = centerY - radiusY;
        } else if (i < mid) {
            x = centerX + radiusX - offset;
            y = centerY - radiusY;
            offset -= offset / div;
        } else {
            x = centerX + radiusX;
            y = centerY - radiusY + offset;
            offset += offset / div;
        }
        points.emplace_back(SkPoint::Make(x, y));
    }
    offset = length;
    for (int i = intervals[2].first; i < intervals[2].second; ++i) {
        int mid = midpoint(intervals[2].first, intervals[2].second);
        float x,y;
        if (i == mid) {
            x = centerX + radiusX;
            y = centerY + radiusY;
        } else if (i < mid) {
            x = centerX + radiusX;
            y = centerY + radiusY - offset;
            offset -= offset / div;
        } else {
            x = centerX + radiusX - offset;
            y = centerY + radiusY;
            offset += offset / div;
        }
        points.emplace_back(SkPoint::Make(x, y));
    }
    offset = length;
    for (int i = intervals[3].first; i < intervals[3].second; ++i) {
        int mid = midpoint(intervals[3].first, intervals[3].second);
        float x,y;
        if (i == mid) {
            x = centerX - radiusX;
            y = centerY + radiusY;
        } else if (i < mid) {
            x = centerX - radiusX + offset;
            y = centerY + radiusY;
            offset -= offset / div;
        } else {
            x = centerX - radiusX;
            y = centerY + radiusY - offset;
            offset += offset / div;
        }
        points.emplace_back(SkPoint::Make(x, y));
    }

    if (points.size() != n) {
        std::cout << "error: calculated edges size too low, n: " << points.size() << std::endl;
        // return {};
    }
    //Debug
    for (int i =0; i < points.size() ; i++) {
        if (i % 5 == 0) {
            printf("\n");
        }
        printf("{ x: %.3f y: %.3f } \t", points[i].x(), points[i].y());
        
    }
    printf("\n");
    return {points, intervals};
}


std::vector<SkPoint> generateEllipsePoints(float x1, float y1, float x2, float y2, int n)
{
    std::vector<SkPoint> points;
    points.reserve(n);
    const float zoom = 1.7;

    float centerX = (x1 + x2) / 2;
    float centerY = (y1 + y2) / 2;
    float radiusX = std::abs(x2 - x1) / 2;
    float radiusY = std::abs(y2 - y1) / 2;
    radiusX *= zoom;
    radiusY *= zoom;

    for (int i = 0; i < n; ++i)
    {
        float angle = 2 * M_PI * i / n;
        float x = centerX + radiusX * std::cos(angle);
        float y = centerY + radiusY * std::sin(angle);
        points.emplace_back(SkPoint::Make(x, y));
    }

    return points;
}

bool isClockwise(const std::vector<SkPoint>& points) {
    float sum = 0.0f;
    size_t n = points.size();

    for (size_t i = 0; i < n; ++i) {
        const SkPoint& p1 = points[i];
        const SkPoint& p2 = points[(i + 1) % n];
        sum += (p2.x() - p1.x()) * (p2.y() + p1.y());
    }

    return sum > 0;
}

bool rotateTargetContur(const std::vector<SkPoint>& base, std::vector<SkPoint>& target)
{
    if (base.size() != target.size()) {
        std::cout << "Error, sizes are not equal \n";
        return false; 
    }
    const auto sz = static_cast<int>(base.size());

    if (isClockwise(base) != isClockwise(target)) {
        std::cout << "target contour is different rotation \n";
        std::reverse(target.begin(), target.end());
    }

    std::pair<float, int> minEl{ std::numeric_limits<float>::max(), 0 };
    for (int k = 0; k <  sz; k++) { // k is an offset to use target[] as a circular buffer
        std::pair<float, int> currVal{ 0.0, k };
        int i = 0;
        int j = i + k;
        for(; i < sz; ++i,++j) {
            currVal.first += SkPoint::Distance(base[i], target[j % sz]);
        }

        if (minEl.first > currVal.first) {
            minEl = currVal;
        }
    }
    
    printf("rotating on %d items \n", minEl.second);
    std::rotate(target.begin(), std::next(target.begin(), minEl.second), target.end());

    return true;
}


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


struct Pos{ float x; float y;};

std::vector<SkPoint> pointsPath;

struct InterpolatingFigure {
    float interpolationSpeed = 0.1;
    float ellipsisInterpVal = 0.0;
    float cornersInterpVal = 0.0;
    std::vector<SkPoint> elipsisPathTarget; //generated points
    SkPath elipsisPathBegin;
    SkPath elipsisPathInterpolated;

    std::vector<std::pair<size_t, size_t>> cornersIntervals; //4 pairs of diapasons
    std::vector<SkPath> cornersPathsBegin;
    std::vector<SkPath> cornersPathsEnd;
    std::vector<SkPath> cornersPathsInterpolated;
    // SkPath cornersPathInterpolated;

    void ElipsisStep() {
        if (ellipsisInterpVal < 1.0) {
            ellipsisInterpVal += interpolationSpeed;
        }
    }
    bool isEllipsisDone() {
        return ellipsisInterpVal >= 1.0;
    }
    void CornersStep() {
        if (cornersInterpVal < 1.0) {
            cornersInterpVal += interpolationSpeed;
        }
    }
    bool isCornersDone() {
        return cornersInterpVal >= 1.0;
    }
};
InterpolatingFigure elipsis;


struct Ranges{int minX=0, minY=0, maxX=0, maxY=0;};
Ranges ranges;


// Function to handle pan events
void handlePanEvent(SDL_Event &event)
{

    static bool panStarted = false;
    std::lock_guard lock(mainLoopMutex);
    switch (event.type)
    {
    case SDL_MOUSEBUTTONDOWN: {
        const SDL_MouseButtonEvent& evt = event.button;
        std::cout << "Pan started at (x:" << evt.x << ", y:" << evt.y << ")\n";
        panStarted = true;
        pointsPath.clear();
        ranges = Ranges{evt.x, evt.y, evt.x, evt.y};
        pointsPath.reserve(1000);
        pointsPath.emplace_back(SkPoint::Make(evt.x, evt.y));
        elipsis = InterpolatingFigure{};

        break;
    }
    case SDL_MOUSEMOTION: {
        const SDL_MouseMotionEvent& evt = event.motion;
        std::cout << "Pan updated to (x:" << evt.x << ", y:" << evt.y << ")\n";
        if (panStarted) {
            if (distance(pointsPath.rbegin()->x(), pointsPath.rbegin()->y(), evt.x, evt.y) > 10.0) {
                pointsPath.emplace_back(SkPoint::Make(evt.x, evt.y));
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
        auto deltaX = static_cast<float>(ranges.maxX - ranges.minX);
        auto deltaY = static_cast<float>(ranges.maxY - ranges.minY);
        auto coeff = std::max(deltaX / deltaY, deltaY / deltaX);
        auto sz = static_cast<int>(pointsPath.size());
        if ((sz > 10)
        && (deltaX > 10.0 && deltaY > 10.0)
        && (coeff < 2.0)
        && (std::min(deltaX, deltaY) > (distance(pointsPath[0].x(), pointsPath[0].y(), pointsPath.rbegin()->x(), pointsPath.rbegin()->y()) / 10.0)))
        {
            // pointsPath.push_back(pointsPath[0]);
            // std::cout << "Closing path, pointsPath::size: " << pointsPath.size() << "\n";
            std::cout << "Path is OK, pointsPath::size: " << pointsPath.size() << "\n";
            elipsis.elipsisPathTarget = generateEllipsePoints(ranges.minX, ranges.minY, ranges.maxX, ranges.maxY, pointsPath.size());
            rotateTargetContur(pointsPath, elipsis.elipsisPathTarget);
        } else {
            std::cout << "Should clean path here \n";
            pointsPath.clear();
        }
        // ranges = Ranges{};
        break;
    }
    default:
        break;
    }
}



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

    // Main loop
    while (running)
    {
        std::chrono::duration<float, std::milli> time{ std::chrono::steady_clock::now() - start };
        
        // float period = 2000.0;
        // if (time > decltype(time){period}) {
        //     start = std::chrono::steady_clock::now(); //reset
        // }
        //time = std::clamp(time, decltype(time){0}, decltype(time){period});


        //SKIA start
        context->resetContext();
        canvas->clear(SK_ColorTRANSPARENT);

        // canvas->clear(SK_ColorBLUE);

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

        // std::cout << "time: " << time.count() / 1000 << std::endl;
        // std::cout << "iTime: " << time.count() / period << std::endl;

        auto shader = builder.makeShader();

        SkScalar    s = SkIntToScalar(std::min(w, h));
        static const SkPoint     kPts0[] = { { 0, 0 }, { s, s } };
        static const SkPoint     kPts1[] = { { s/2, 0 }, { s/2, s } };
        static const SkScalar    kPos[] = { 0, SK_Scalar1/2, SK_Scalar1 };
        static const SkColor kColors0[] = {0x80F00080, 0xF0F08000, 0x800080F0 };
        static const SkColor kColors1[] = {0xF08000F0, 0x8080F000, 0xF000F080 };
        auto linearShader =  SkGradientShader::MakeLinear(kPts0, kColors0, kPos, std::size(kColors0), SkTileMode::kClamp);


        SkPaint paint;
        paint.setBlendMode(SkBlendMode::kSrcOver);
        paint.setShader(linearShader);
        paint.setAntiAlias(true);

        // Draw with shader
        // canvas->drawPaint(paint);
        //SKIA end

        SkPath path;
        //     // //circle
        //     path.moveTo(150, 150);
        //     //path.addCircle(w/2, h/2, 200);//
        //     // addArc(const SkRect& oval, SkScalar startAngle, SkScalar sweepAngle)
            auto rect = SkRect{50,50, 500, 500};
            path.addArc(rect, 0, 180);
        
        SkPath path2;
            path2.moveTo(400, 10);
            path2.lineTo(400, 500);
            


        //     //line
        //     // path.moveTo(20, 300);
        //     // path.addCircle(400, 300, 200);

        // paint.setPathEffect(

        // paint.setMaskFilter(SkMaskFilter::MakeBlur(kSolid_SkBlurStyle, 20.0f));

        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeCap(SkPaint::kRound_Cap);
        
        SkPoint pointsColor[2] = { {50, 50}, {400, 400} };
        SkColor colors[2] = { SK_ColorRED, SK_ColorBLUE };
        shader = SkGradientShader::MakeLinear(pointsColor, colors, nullptr, std::size(colors), SkTileMode::kClamp);


        paint.setStrokeWidth(20);
        paint.setMaskFilter(SkMaskFilter::MakeBlur(kSolid_SkBlurStyle, 35.0f, true));

        // Apply the shader to the paint
        paint.setShader(shader);

        std::vector<SkPoint> points(8);
        for (int i = 0; i < points.size(); ++i) {
            float t = i / (float)points.size();                                        // Normalize t to range [0, 1]
            points[i].set(w * t, h/2 + h/2 * sin(2 * M_PI * t)); // Example "S" curve equation
        }
        // SkPath path3;
        // path3.moveTo(points[0]);
        // for (size_t i = 1; i < points.size(); ++i)
        // {
        //     path3.lineTo(points[i]);
        // }
        {
            std::lock_guard lock(mainLoopMutex);
            SkPath path3;
            for (int i = 0; i < pointsPath.size(); ++i) {
                if (i == 0) path3.moveTo({pointsPath[0].x(), pointsPath[0].y()});
                path3.lineTo({pointsPath[i].x(), pointsPath[i].y()});
            }



            if (elipsis.isCornersDone()) {
                for (auto& path : elipsis.cornersPathsEnd) {
                    canvas->drawPath(path, paint);
                }
            } else if (!elipsis.cornersPathsEnd.empty()) {
                std::cout << "!elipsis.cornersPathsEnd.empty() " << std::endl;
                for (int i = 0; i < 4; i++) {
                    SkPath interp;
                    elipsis.cornersPathsEnd[i].interpolate(elipsis.cornersPathsBegin[i], elipsis.cornersInterpVal, &interp);
                    elipsis.cornersPathsInterpolated[i] = std::move(interp);
                    canvas->drawPath(interp, paint);
                }
                elipsis.CornersStep();
            } else if (elipsis.isEllipsisDone()) {
                std::cout << "generateCornersPoints start " << std::endl;
                auto [points, intervals] = generateCornersPoints(ranges.minX, ranges.minY, ranges.maxX, ranges.maxY, elipsis.elipsisPathTarget.size());
                bool rotationStatus = rotateTargetContur(points, elipsis.elipsisPathTarget);
                // bool rotationStatus = rotateTargetContur(elipsis.elipsisPathTarget, points);
                std::cout << "Rotation corners status: " << std::boolalpha << rotationStatus << std::endl;


                for (const auto& it : intervals) {
                    int j = it.first;
                    SkPath corner;
                    SkPath arc;
                    corner.moveTo(points[j]);
                    arc.moveTo(elipsis.elipsisPathTarget[j]);
                    for (j++; j < it.second; j++) {
                        // SkPath interp;
                        corner.lineTo(points[j]);
                        arc.lineTo(elipsis.elipsisPathTarget[j]);
                    }
                    elipsis.cornersPathsEnd.push_back(std::move(corner));
                    elipsis.cornersPathsBegin.push_back(std::move(arc));
                    elipsis.cornersPathsInterpolated = elipsis.cornersPathsBegin;
                }
                elipsis.cornersIntervals = std::move(intervals);
                std::cout << "generateCornersPoints end " << std::endl;
            } else if (!elipsis.elipsisPathInterpolated.isEmpty()) {
                SkPath interp;
                elipsis.elipsisPathBegin.interpolate(path3, elipsis.ellipsisInterpVal, &interp);
                elipsis.ElipsisStep();
                elipsis.elipsisPathInterpolated = std::move(interp);
                canvas->drawPath(elipsis.elipsisPathInterpolated, paint);
            } else if (!elipsis.elipsisPathTarget.empty()) {
                std::cout << "!elipsis.empty() " << std::endl;

                SkPath elipsisPath;
                elipsisPath.moveTo(elipsis.elipsisPathTarget[0]);
                for (auto&& p : elipsis.elipsisPathTarget) {
                    elipsisPath.lineTo(p);
                }
                // elipsisPath.close();
                elipsis.elipsisPathBegin = elipsisPath;
                elipsis.elipsisPathInterpolated = elipsisPath;
                // canvas->drawPath(elipsis.elipsisPathInterpolated, paint);
            } else {
                // Draw the path with the gradient
                for (int i = 0; i < 3; i++) 
                {
                    canvas->drawPath(path3, paint);
                }
            }
        }

        /////////////////////


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