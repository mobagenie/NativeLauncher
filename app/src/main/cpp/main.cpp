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
    {"CHROME"},
    {"YOUTUBE"},
    {"WHATSAPP"},
    {"PLAYSTORE"},
    {"CHATGPT"},
    {"SETTINGS"},
    {"MGYOUTUBE"},
    {"FILES"}
};

//////////////////////////////////////////////////////////////////
// 8x8 BITMAP FONT (only needed letters)
//////////////////////////////////////////////////////////////////

const unsigned char font8x8[128][8] = {

    ['A'] = {0x18,0x24,0x42,0x7E,0x42,0x42,0x42,0x00},
    ['C'] = {0x3C,0x42,0x40,0x40,0x40,0x42,0x3C,0x00},
    ['E'] = {0x7E,0x40,0x40,0x7C,0x40,0x40,0x7E,0x00},
    ['F'] = {0x7E,0x40,0x40,0x7C,0x40,0x40,0x40,0x00},
    ['G'] = {0x3C,0x42,0x40,0x4E,0x42,0x42,0x3C,0x00},
    ['H'] = {0x42,0x42,0x42,0x7E,0x42,0x42,0x42,0x00},
    ['I'] = {0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00},
    ['L'] = {0x40,0x40,0x40,0x40,0x40,0x40,0x7E,0x00},
    ['M'] = {0x42,0x66,0x5A,0x42,0x42,0x42,0x42,0x00},
    ['N'] = {0x42,0x62,0x52,0x4A,0x46,0x42,0x42,0x00},
    ['O'] = {0x3C,0x42,0x42,0x42,0x42,0x42,0x3C,0x00},
    ['P'] = {0x7C,0x42,0x42,0x7C,0x40,0x40,0x40,0x00},
    ['R'] = {0x7C,0x42,0x42,0x7C,0x48,0x44,0x42,0x00},
    ['S'] = {0x3C,0x42,0x40,0x3C,0x02,0x42,0x3C,0x00},
    ['T'] = {0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00},
    ['U'] = {0x42,0x42,0x42,0x42,0x42,0x42,0x3C,0x00},
    ['W'] = {0x42,0x42,0x42,0x5A,0x66,0x42,0x42,0x00},
    ['Y'] = {0x42,0x42,0x24,0x18,0x18,0x18,0x18,0x00}
};

//////////////////////////////////////////////////////////////////
// DRAW CHARACTER
//////////////////////////////////////////////////////////////////

void draw_char(uint32_t* pixels, int stride, int x, int y, char c) {
    for (int row = 0; row < 8; row++) {
        unsigned char bits = font8x8[(int)c][row];
        for (int col = 0; col < 8; col++) {
            if (bits & (1 << (7 - col))) {
                pixels[(y + row) * stride + (x + col)] = 0xFF000000;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////
// DRAW TEXT
//////////////////////////////////////////////////////////////////

void draw_text(uint32_t* pixels, int stride, int x, int y, const std::string& text) {
    for (int i = 0; i < text.size(); i++) {
        draw_char(pixels, stride, x + i * 9, y, text[i]);
    }
}

//////////////////////////////////////////////////////////////////
// DRAW UI
//////////////////////////////////////////////////////////////////

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

        // draw white box
        for (int y = startY; y < startY + 100 && y < buffer.height; y++) {
            for (int x = startX; x < startX + cellW - 40 && x < buffer.width; x++) {
                pixels[y * buffer.stride + x] = 0xFFFFFFFF;
            }
        }

        // draw text inside
        draw_text(pixels, buffer.stride, startX + 10, startY + 40, apps[i].name);
    }

    ANativeWindow_unlockAndPost(window);
}

//////////////////////////////////////////////////////////////////
// INPUT
//////////////////////////////////////////////////////////////////

int32_t handle_input(struct android_app* app, AInputEvent* event) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {

        float x = AMotionEvent_getX(event, 0);
        float y = AMotionEvent_getY(event, 0);

        int cols = 3;
        int width = ANativeWindow_getWidth(window);
        int cellW = width / cols;
        int cellH = 220;

        int col = x / cellW;
        int row = y / cellH;

        int index = row * cols + col;

        if (index >= 0 && index < apps.size()) {
            LOGI("Clicked: %s", apps[index].name.c_str());
        }

        return 1;
    }
    return 0;
}

//////////////////////////////////////////////////////////////////
// COMMANDS
//////////////////////////////////////////////////////////////////

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

//////////////////////////////////////////////////////////////////
// MAIN LOOP
//////////////////////////////////////////////////////////////////

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