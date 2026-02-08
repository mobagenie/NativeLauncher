#include <android_native_app_glue.h>
#include <android/native_window.h>
#include <android/log.h>
#include <android/input.h>
#include <unistd.h>
#include <vector>
#include <string>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "LiteLauncher", __VA_ARGS__)

static ANativeWindow* window = nullptr;

struct AppItem {
    std::string name;
};

std::vector<AppItem> apps = {
    {"Chrome"}, {"YouTube"}, {"WhatsApp"},
    {"PlayStore"}, {"ChatGPT"}, {"Settings"},
    {"MGYoutube"}, {"Files"}
};

void draw() {
    if (!window) return;

    ANativeWindow_Buffer buffer;
    if (ANativeWindow_lock(window, &buffer, NULL) < 0) return;

    uint32_t* pixels = (uint32_t*) buffer.bits;

    // Clear screen black
    for (int y = 0; y < buffer.height; y++) {
        for (int x = 0; x < buffer.width; x++) {
            pixels[y * buffer.stride + x] = 0xFF000000;
        }
    }

    int cols = 3;
    int cellW = buffer.width / cols;
    int cellH = 220;

    for (int i = 0; i < apps.size(); i++) {
        int row = i / cols;
        int col = i % cols;

        int startX = col * cellW + 20;
        int startY = row * cellH + 40;

        for (int y = startY; y < startY + 100 && y < buffer.height; y++) {
            for (int x = startX; x < startX + cellW - 40 && x < buffer.width; x++) {
                pixels[y * buffer.stride + x] = 0xFFFFFFFF;
            }
        }
    }

    ANativeWindow_unlockAndPost(window);
}

int32_t handle_input(struct android_app* app, AInputEvent* event) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        float x = AMotionEvent_getX(event, 0);
        float y = AMotionEvent_getY(event, 0);
        LOGI("Touch: %f %f", x, y);
        return 1;
    }
    return 0;
}

void handle_cmd(struct android_app* app, int32_t cmd) {
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            window = app->window;

            ANativeWindow_setBuffersGeometry(
                window,
                0,
                0,
                WINDOW_FORMAT_RGBA_8888
            );

            draw();
            break;

        case APP_CMD_TERM_WINDOW:
            window = nullptr;
            break;
    }
}

void android_main(struct android_app* state) {
    app_dummy();

    state->onAppCmd = handle_cmd;
    state->onInputEvent = handle_input;

    int events;
    struct android_poll_source* source;

    while (true) {
        while (ALooper_pollAll(0, NULL, &events, (void**)&source) >= 0) {
            if (source) source->process(state, source);
        }

        if (window) draw();
        usleep(16000);
    }
}