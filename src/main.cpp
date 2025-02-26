#include <SDL3/SDL.h>
#include <SDL3_shadercross/SDL_shadercross.h>

// support structure to enable us to split some stuff into separate functions.
struct SDLGPUContext
{
    SDL_GPUDevice *device = nullptr;
    SDL_Window *window = nullptr;
};

SDL_GPUShader *loadShader(SDL_GPUDevice *device, const char *filename, SDL_ShaderCross_ShaderStage stage, SDL_ShaderCross_GraphicsShaderMetadata *out)
{
    size_t sz = 0;

    const char *shaderSource = (char *)SDL_LoadFile(filename, &sz);
    if (shaderSource == nullptr)
    {
        SDL_Log("%s", SDL_GetError());
        return nullptr;
    }

    SDL_ShaderCross_HLSL_Info info{0};
    info.source = shaderSource;
    info.enable_debug = true;
    info.entrypoint = "main";
    info.shader_stage = stage;

    return SDL_ShaderCross_CompileGraphicsShaderFromHLSL(device, &info, out);
}

bool createSDLGPUContext(SDLGPUContext &ctx)
{
    ctx.device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_MSL | SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
    if (ctx.device == nullptr)
    {
        SDL_Log("SDL_CreateGPUDevice failed: %s", SDL_GetError());
        return false;
    }

    ctx.window = SDL_CreateWindow("hello shader", 800, 600, 0);
    if (ctx.window == nullptr)
    {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        return false;
    }

    if (!SDL_ClaimWindowForGPUDevice(ctx.device, ctx.window))
    {
        SDL_Log("GPUClaimWindow failed: %s", SDL_GetError());
        return false;
    }

    return true;
}

int main()
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_ShaderCross_Init();

    SDLGPUContext ctx;
    if (!createSDLGPUContext(ctx))
    {
        return -1;
    }

    SDL_ShaderCross_GraphicsShaderMetadata vertMeta{0}, fragMeta{0};
    auto vertShader = loadShader(ctx.device, "resources/shaders/RawTriangle.vs.hlsl", SDL_SHADERCROSS_SHADERSTAGE_VERTEX, &vertMeta);
    if (vertShader == nullptr)
    {
        SDL_Log("ERROR VertexShader: %s", SDL_GetError());
        return -1;
    }

    auto fragShader = loadShader(ctx.device, "resources/shaders/SolidColor.fs.hlsl", SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT, &fragMeta);
    if (fragShader == nullptr)
    {
        SDL_Log("ERROR FragmentShader: %s", SDL_GetError());
        return -1;
    }

    // Create the pipelines
    SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo = {
        .target_info = {
            .num_color_targets = 1,
            .color_target_descriptions = (SDL_GPUColorTargetDescription[]){{.format = SDL_GetGPUSwapchainTextureFormat(ctx.device, ctx.window)}},
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .vertex_shader = vertShader,
        .fragment_shader = fragShader,
    };

    pipelineCreateInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    auto FillPipeline = SDL_CreateGPUGraphicsPipeline(ctx.device, &pipelineCreateInfo);
    if (FillPipeline == NULL)
    {
        SDL_Log("Failed to create fill pipeline: %s", SDL_GetError());
        return -1;
    }

    // init swap colorInfo
    SDL_GPUColorTargetInfo colorInfo;
    SDL_zero(colorInfo);
    colorInfo.load_op = SDL_GPU_LOADOP_CLEAR;
    colorInfo.store_op = SDL_GPU_STOREOP_STORE;
    colorInfo.mip_level = 0;
    colorInfo.layer_or_depth_plane = 0;
    colorInfo.cycle = false;

    bool quit = false;
    while (!quit)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
                quit = true;
        }

        SDL_GPUTexture *swapchainTexture;
        auto commandBuffer = SDL_AcquireGPUCommandBuffer(ctx.device);
        SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, ctx.window, &swapchainTexture, NULL, NULL);
        if (swapchainTexture != nullptr)
        {
            colorInfo.texture = swapchainTexture;
            auto renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorInfo, 1, NULL);
            SDL_BindGPUGraphicsPipeline(renderPass, FillPipeline);
            SDL_DrawGPUPrimitives(renderPass, 3, 1, 0, 0);
            SDL_EndGPURenderPass(renderPass);
            SDL_SubmitGPUCommandBuffer(commandBuffer);
        }
    }

    SDL_ReleaseGPUShader(ctx.device, vertShader);
    SDL_ReleaseGPUShader(ctx.device, fragShader);

    SDL_ReleaseWindowFromGPUDevice(ctx.device, ctx.window);
    SDL_DestroyWindow(ctx.window);
    SDL_DestroyGPUDevice(ctx.device);

    return 0;
}