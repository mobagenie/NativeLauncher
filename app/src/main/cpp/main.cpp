#include <android_native_app_glue.h>
#include <android/native_window.h>
#include <android/log.h>
#include <android/input.h>
#include <unistd.h>
#include <vector>
#include <string>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "LiteLauncher", __VA_ARGS__)

static ANativeWindow* window = nullptr;
static int selectedIndex = -1;

struct AppItem {
    std::string name;
};

std::vector<AppItem> apps = {
    {"Chrome"},
    {"YouTube"},
    {"WhatsApp"},
    {"PlayStore"},
    {"ChatGPT"},
    {"Settings"},
    {"MGYoutube"},
    {"Files"}
};

void draw() {
    if (!window) return;

    ANativeWindow_Buffer buffer;
    if (ANativeWindow_lock(window, &buffer, NULL) < 0) return;

    uint32_t* pixels = (uint32_t*) buffer.bits;

    int width  = buffer.width;
    int height = buffer.height;

    // Clear screen (black)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            pixels[y * buffer.stride + x] = 0xFF000000;
        }
    }

    int cols = 3;
    int cellW = width / cols;
    int cellH = 220;

    for (int i = 0; i < apps.size(); i++) {

        int row = i / cols;
        int col = i % cols;

        int startX = col * cellW + 20;
        int startY = row * cellH + 40;

        uint32_t color =
            (i == selectedIndex) ?
            0xFFFF4444 :      // merah kalau dipilih
            0xFFFFFFFF;       // putih normal

        for (int y = startY; y < startY + 100; y++) {
            if (y >= height) continue;

            for (int x = startX; x < startX + cellW - 40; x++) {
                if (x >= width) continue;

                pixels[y * buffer.stride + x] = color;
            }
        }
    }

    ANativeWindow_unlockAndPost(window);
}

int32_t handle_input(struct android_app* app, AInputEvent* event) {

    if (AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION)
        return 0;

    int action = AMotionEvent_getAction(event);

    if ((action & AMOTION_EVENT_ACTION_MASK) != AMOTION_EVENT_ACTION_DOWN)
        return 0;

    float x = AMotionEvent_getX(event, 0);
    float y = AMotionEvent_getY(event, 0);

    LOGI("Touch: %f %f", x, y);

    if (!window) return 0;

    int width = ANativeWindow_getWidth(window);

    int cols = 3;
    int cellW = width / cols;
    int cellH = 220;

    int col = (int)(x / cellW);
    int row = (int)(y / cellH);

    int index = row * cols + col;

    if (index >= 0 && index < apps.size()) {
        selectedIndex = index;
        LOGI("Clicked: %s", apps[index].name.c_str());
        draw();
    }

    return 1;
}

void handle_cmd(struct android_app* app, int32_t cmd) {

    switch (cmd) {

        case APP_CMD_INIT_WINDOW:
            window = app->window;
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

        while (ALooper_pollAll(-1, NULL, &events, (void**)&source) >= 0) {
            if (source)
                source->process(state, source);

            if (state->destroyRequested != 0)
                return;
        }

        if (window)
            draw();
    }
}