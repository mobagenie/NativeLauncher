#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include <android_native_app_glue.h>
#include <android/log.h>
#include <jni.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>

#define LOG(...) __android_log_print(ANDROID_LOG_INFO,"NativeLauncher",__VA_ARGS__)

static ANativeWindow* window = nullptr;
static bool running = true;
static bool fontReady = false;
static bool loaded = false;

float scrollY = 0;
float lastTouchY = 0;
bool dragging = false;

stbtt_fontinfo font;
unsigned char ttf[1<<20];

struct App {
    std::string name;
    std::string pkg;
};

std::vector<App> apps;

// ================= PERMISSION =================
void requestPerm(android_app* app) {
    JNIEnv* e;
    app->activity->vm->AttachCurrentThread(&e, nullptr);

    jclass c = e->GetObjectClass(app->activity->clazz);
    jmethodID m = e->GetMethodID(c, "requestPermissions", "([Ljava/lang/String;I)V");

    jobjectArray arr = e->NewObjectArray(
        1,
        e->FindClass("java/lang/String"),
        e->NewStringUTF("android.permission.READ_EXTERNAL_STORAGE")
    );

    e->CallVoidMethod(app->activity->clazz, m, arr, 0);
    app->activity->vm->DetachCurrentThread();
}

// ================= LOAD APPS =================
void loadApps() {
    apps.clear();
    std::ifstream f("/sdcard/setting.txt");
    if(f.is_open()) {
        std::string line;
        while(std::getline(f, line)) {
            if(line.empty()) continue;
            size_t p = line.find('|');
            if(p == std::string::npos) continue;
            apps.push_back({ line.substr(0,p), line.substr(p+1) });
        }
        f.close();
    }

    if(apps.empty()) {
        apps = {
            {"Settings", "com.android.settings"},
            {"Files", "com.android.documentsui"},
            {"Chrome", "com.android.chrome"}
        };
    }

    LOG("Apps loaded: %d", (int)apps.size());
}

// ================= DRAW TEXT =================
void drawText(uint32_t* p, int stride, int x, int y, const char* t, float px) {
    if(!fontReady) return;

    float s = stbtt_ScaleForPixelHeight(&font, px);
    int cx = x;

    while(*t) {
        int ax, l;
        stbtt_GetCodepointHMetrics(&font, *t, &ax, &l);

        int x0, y0, x1, y1;
        stbtt_GetCodepointBitmapBox(&font, *t, s, s, &x0, &y0, &x1, &y1);

        int bw = x1 - x0, bh = y1 - y0;
        if(bw <=0 || bh <=0) { t++; continue; }

        std::vector<unsigned char> b(bw*bh);
        stbtt_MakeCodepointBitmap(&font, b.data(), bw, bh, bw, s, s, *t);

        for(int j=0;j<bh;j++)
            for(int i=0;i<bw;i++)
                if(b[j*bw+i])
                    if(y+j>=0 && y+j<720 && cx+i<1280)
                        p[(y+j)*stride + (cx+i)] = 0xffffffff;

        cx += ax*s;
        t++;
    }
}

// ================= DRAW GRID =================
void draw() {
    if(!window) return;

    ANativeWindow_Buffer buf;
    if(ANativeWindow_lock(window, &buf, nullptr) !=0) return;

    uint32_t* p = (uint32_t*)buf.bits;
    memset(p, 0, buf.height * buf.stride * 4);

    int w = buf.width;
    int cols = 5;           // 5 kolom
    int padding = 8;        // jarak antar kotak
    int itemW = w / cols - padding;
    int itemH = 60;         // kotak kecil
    int labelH = 18;        // tinggi label
    int rowHeight = itemH + labelH + padding;

    // batasi max 50 app
    int maxApps = std::min(50, (int)apps.size());
    int contentHeight = ((maxApps + cols - 1) / cols) * rowHeight;

    // clamp scroll
    float minScroll = std::min(0.0f, float(buf.height - contentHeight));
    scrollY = std::max(minScroll, scrollY);
    scrollY = std::min(scrollY, 0.0f);

    for(int i=0; i<maxApps; i++){
        int r = i / cols;
        int c = i % cols;
        int sx = c * (itemW + padding) + padding / 2;
        int sy = r * rowHeight + padding / 2 + scrollY;

        if(sy + rowHeight < 0 || sy > buf.height) continue;

        // kotak
        for(int y=sy; y<sy+itemH; y++)
            for(int x=sx; x<sx+itemW; x++)
                if(y>=0 && y<buf.height && x<w)
                    p[y*buf.stride + x] = 0xffcccccc;

        // label di bawah kotak
        drawText(p, buf.stride, sx + 2, sy + itemH + 2, apps[i].name.c_str(), 14);
    }

    ANativeWindow_unlockAndPost(window);
}

// ================= LAUNCH =================
void launch(android_app* app, const char* pkg){
    JNIEnv* e;
    app->activity->vm->AttachCurrentThread(&e, nullptr);

    jclass c = e->GetObjectClass(app->activity->clazz);
    jmethodID pm = e->GetMethodID(c, "getPackageManager", "()Landroid/content/pm/PackageManager;");
    jobject m = e->CallObjectMethod(app->activity->clazz, pm);

    jclass pc = e->GetObjectClass(m);
    jmethodID gi = e->GetMethodID(pc, "getLaunchIntentForPackage", "(Ljava/lang/String;)Landroid/content/Intent;");

    jstring j = e->NewStringUTF(pkg);
    jobject it = e->CallObjectMethod(m, gi, j);

    if(it){
        jmethodID st = e->GetMethodID(c,"startActivity","(Landroid/content/Intent;)V");
        e->CallVoidMethod(app->activity->clazz, st, it);
    }

    app->activity->vm->DetachCurrentThread();
}

// ================= INPUT =================
int32_t input(android_app* app, AInputEvent* ev){
    if(AInputEvent_getType(ev)!=AINPUT_EVENT_TYPE_MOTION) return 0;

    int act = AMotionEvent_getAction(ev) & AMOTION_EVENT_ACTION_MASK;
    float x = AMotionEvent_getX(ev,0);
    float y = AMotionEvent_getY(ev,0);

    if(act==AMOTION_EVENT_ACTION_DOWN){
        lastTouchY = y;
        dragging = true;
    }

    if(act==AMOTION_EVENT_ACTION_MOVE && dragging){
        scrollY += y - lastTouchY;
        lastTouchY = y;
    }

    if(act==AMOTION_EVENT_ACTION_UP){
        dragging = false;

        int cols = 5;
        int itemW = ANativeWindow_getWidth(window)/cols -10;
        int itemH = 80;
        int padding =10;

        for(int i=0;i<apps.size();i++){
            int r = i/cols;
            int c = i%cols;
            int sx = c*(itemW+padding) + padding/2;
            int sy = r*(itemH+25) + padding + scrollY;

            if(x>=sx && x<=sx+itemW && y>=sy && y<=sy+itemH)
                launch(app, apps[i].pkg.c_str());
        }
    }

    return 1;
}

// ================= CMD =================
void cmd(android_app* app, int32_t c){
    if(c==APP_CMD_INIT_WINDOW){
        window = app->window;

        if(!fontReady){
            AAsset* a = AAssetManager_open(app->activity->assetManager,"font.ttf",0);
            AAsset_read(a, ttf, AAsset_getLength(a));
            AAsset_close(a);
            stbtt_InitFont(&font, ttf,0);
            fontReady = true;
        }

        if(!loaded){
            loadApps();
            loaded = true;
        }
    }

    if(c==APP_CMD_TERM_WINDOW) window = nullptr;
    if(c==APP_CMD_DESTROY) running=false;
}

// ================= MAIN =================
void android_main(android_app* app){
    requestPerm(app);

    app->onInputEvent = input;
    app->onAppCmd = cmd;

    while(running){
        int ev;
        android_poll_source* src;

        while(ALooper_pollAll(0,nullptr,&ev,(void**)&src)>=0){
            if(src) src->process(app,src);
            if(app->destroyRequested) return;
        }

        if(window) draw();
        usleep(16000);
    }
}