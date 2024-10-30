#include <SDL.h>
#include "include/core/SkCanvas.h"
#include "include/core/SkSurface.h"
#include "include/core/SkPaint.h"
#include "include/effects/SkRuntimeEffect.h"
#include <string>
#include <iostream>
#include <thread>

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
// const std::string skslCode = R"(
// // Magic Line Path SkSL Shader
// uniform float iTime;
// uniform vec2 iResolution;

// float random(vec2 st) {
// return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
// }

// float noise(vec2 st) {
// vec2 i = floor(st);
// vec2 f = fract(st);
// float a = random(i);
// float b = random(i + vec2(1.0, 0.0));
// float c = random(i + vec2(0.0, 1.0));
// float d = random(i + vec2(1.0, 1.0));
// vec2 u = f * f * (3.0 - 2.0 * f);
// return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
// }

// float spark(vec2 st, vec2 pos, float time) {
// float dist = distance(st, pos);
// float intensity = smoothstep(0.0, 0.02, 0.02 - dist);
// float flicker = noise(st * 50.0 + time * 10.0);
// return intensity * flicker;
// }

// half4 main(vec2 fragCoord) {
// vec2 st = fragCoord.xy / iResolution.xy;
// vec3 color = vec3(0.0);

// vec2 center = vec2(0.5, 0.5);
// float radius = 0.05;
// float dist = distance(st, center);

// float glow = smoothstep(radius, radius + 0.02, radius - dist);

// float spark1 = spark(st, center + vec2(0.1 * cos(iTime * 5.0), 0.1 * sin(iTime * 5.0)), iTime);
// float spark2 = spark(st, center + vec2(0.1 * cos(iTime * 6.0), 0.1 * sin(iTime * 6.0)), iTime + 1.0);
// float spark3 = spark(st, center + vec2(0.1 * cos(iTime * 7.0), 0.1 * sin(iTime * 7.0)), iTime + 2.0);

// float totalSparks = spark1 + spark2 + spark3;
// color = vec3(glow + totalSparks, (glow + totalSparks) * 0.5, (glow + totalSparks) * 0.2);

// return half4(color, 1.0);
// }
// )";

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

    
    // Main loop
    while (running)
    {
        std::chrono::duration<float, std::milli> time{ std::chrono::steady_clock::now() - start };
        float period = 2000.0;
        if (time > decltype(time){period}) {
            start = std::chrono::steady_clock::now(); //reset
        }
        //time = std::clamp(time, decltype(time){0}, decltype(time){period});


        //SKIA start
        SkImageInfo info = SkImageInfo::MakeN32Premul(800, 600);
        auto surface = SkSurfaces::Raster(info);
        SkCanvas *canvas = surface->getCanvas();
        canvas->clear(SK_ColorBLUE);

        // Compile SkSL shader
        auto [effect, error] = SkRuntimeEffect::MakeForShader(SkString(skslCode.c_str()));
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