#include <SDL.h>
#include "include/core/SkCanvas.h"
#include "include/core/SkSurface.h"
#include "include/core/SkPaint.h"
#include "include/effects/SkRuntimeEffect.h"
#include <string>
#include <iostream>
#include <thread>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream> 


/*Dots spark*/
const std::string skslCodeTest1 = R"(
uniform float2 iResolution;
uniform float2 iMouse;
uniform float iTime;

half4 main(float2 fragCoord) {
float2 uv = fragCoord.xy / iResolution.xy;
float2 mouse = iMouse.xy / iResolution.xy;
float dist = distance(uv, mouse);

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

/*Dots spark*/
const std::string skslCodeTest2 = R"(
uniform float2 iResolution;
uniform float2 iMouse;
uniform float iTime;

half4 main(float2 fragCoord) {
float2 uv = fragCoord.xy / iResolution.xy;
float2 mouse = iMouse.xy / iResolution.xy;
float dist = distance(uv, mouse);

// Initialize the color to black
half4 color = half4(0.0);

// Number of sparks
const int numSparks = 300;

// Loop to create multiple sparks
for (int i = 0; i < numSparks; i++) {
// Random angle and radius for each spark
float angle = 6.28318530718 * fract(sin(float(i) * 12.9898 + iTime) * 43758.5453);
float radius = 0.1 * fract(sin(float(i) * 78.233 + iTime) * 43758.5453);

// Reduce density of sparks further from the cursor
if (radius > 0.05) {
radius *= 1.5;
}

// Calculate spark position
float2 sparkPos = mouse + radius * float2(cos(angle), sin(angle));

// Calculate the intensity of the spark
float sparkIntensity = smoothstep(0.001, 0.005, 0.005 - distance(uv, sparkPos));

// Add the spark color
color += half4(1.0, 1.0, 0.0, 1.0) * sparkIntensity;
}

return color;
}
    )";

//magic line
const std::string skslCodeMagicLine = R"(
// Magic Line Path SkSL Shader
uniform float iTime;
uniform vec2 iResolution;

float random(vec2 st) {
return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

float noise(vec2 st) {
vec2 i = floor(st);
vec2 f = fract(st);
float a = random(i);
float b = random(i + vec2(1.0, 0.0));
float c = random(i + vec2(0.0, 1.0));
float d = random(i + vec2(1.0, 1.0));
vec2 u = f * f * (3.0 - 2.0 * f);
return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

float spark(vec2 st, vec2 pos, float time) {
float dist = distance(st, pos);
float intensity = smoothstep(0.0, 0.02, 0.02 - dist);
float flicker = noise(st * 50.0 + time * 10.0);
return intensity * flicker;
}

half4 main(vec2 fragCoord) {
vec2 st = fragCoord.xy / iResolution.xy;
vec3 color = vec3(0.0);

vec2 center = vec2(0.5, 0.5);
float radius = 0.05;
float dist = distance(st, center);

float glow = smoothstep(radius, radius + 0.02, radius - dist);

float spark1 = spark(st, center + vec2(0.1 * cos(iTime * 5.0), 0.1 * sin(iTime * 5.0)), iTime);
float spark2 = spark(st, center + vec2(0.1 * cos(iTime * 6.0), 0.1 * sin(iTime * 6.0)), iTime + 1.0);
float spark3 = spark(st, center + vec2(0.1 * cos(iTime * 7.0), 0.1 * sin(iTime * 7.0)), iTime + 2.0);

float totalSparks = spark1 + spark2 + spark3;
color = vec3(glow + totalSparks, (glow + totalSparks) * 0.5, (glow + totalSparks) * 0.2);

return half4(color, 1.0);
}
)";

const std::string skslCode = R"(
    // SkSL code for animated darkening towards the center
    uniform float2 iResolution; // Canvas resolution
    uniform float iTime;        // Time variable for animation

    half4 main(float2 fragCoord) {
    // Normalize coordinates
    float2 uv = float2(fragCoord.xy / iResolution.xy);
    uv = uv * 2.0 - 1.0; // Transform to range [-1, 1]

    // Calculate distance from center
    float dist = 1.0 - length(uv) * length(uv);

    // Brightening factor with animation
    float brighten = 1 * dist * smoothstep(0.0, 1.0, iTime);

    // Apply brightening effect
    return half4(1.0 - brighten, 1.0 - brighten, 1.0 - brighten, 0.5);
    }
)";

std::optional<std::string> readShader(const std::string& shader_name)
{
    std::stringstream ss;
    static const std::string SHADERS_DIR = "/home/ashimgaev/git/skia_test/shaders/";
    std::ifstream file(SHADERS_DIR + shader_name);
    if (!file.is_open()) {
        std::cout << "Cant open file\n";
        return std::optional<std::string>{};
    }

    std::string line;
    while (getline(file, line)) {
        ss << line << std::endl;
    }

    file.close();
    return {ss.str()};
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
            if (event.type == SDL_QUIT)
            {
                running = false;
            }
        }
        // Sleep to prevent high CPU usage
        SDL_Delay(10);
    }
}

int main(int argc, char *argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        return -1;
    }

    SDL_Window *window = SDL_CreateWindow("SDL Thread Example", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);
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

    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 800, 600);
    if (!texture)
    {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // Start the event polling thread
    std::thread eventThread(PollEvents);

    auto start = std::chrono::steady_clock::now();

    //SKIA start
    SkImageInfo info = SkImageInfo::MakeN32Premul(800, 600);
    auto surface = SkSurfaces::Raster(info);
    SkCanvas* canvas = surface->getCanvas();
    
    // Main loop
    while (running)
    {
        std::chrono::duration<float, std::milli> time{ std::chrono::steady_clock::now() - start };
        float period = 2000.0;
        if (time > decltype(time){period}) {
            start = std::chrono::steady_clock::now(); //reset
        }
        //time = std::clamp(time, decltype(time){0}, decltype(time){period});



        canvas->clear(SK_ColorBLUE);

        // Compile SkSL shader

        auto SHADER_CODE = readShader("dots_sparks.sksl").value();
        std::cout << SHADER_CODE << "\n";
        auto [effect, error] = SkRuntimeEffect::MakeForShader(SkString(SHADER_CODE.c_str()));
        if (!effect)
        {
            std::cerr << "Error compiling SkSL shader: " << error.c_str() << std::endl;
            return -1;
        }

        // Set shader uniforms
        SkRuntimeShaderBuilder builder(effect);
        builder.uniform("iResolution") = SkV2{800, 600};
        builder.uniform("iTime") = time.count() / period;

        // std::cout << "time: " << time.count() << std::endl;
        // std::cout << "iTime: " << time.count() / period << std::endl;

        SkPaint paint;
        paint.setBlendMode(SkBlendMode::kSrcOver);
        paint.setShader(builder.makeShader());

        // Draw with shader
        canvas->drawPaint(paint);
        //SKIA end

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

        // Clear screen
        SDL_RenderClear(renderer);

        // Render texture to screen
        SDL_RenderCopy(renderer, texture, NULL, NULL);

        // Update screen
        SDL_RenderPresent(renderer);


        // Update game state, render, etc.
        SDL_Delay(16); 
    }

    // Clean up
    eventThread.join();
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}