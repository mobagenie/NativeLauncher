#include <android_native_app_glue.h>
#include <android/native_activity.h>
#include <android/native_window.h>
#include <android/log.h>
#include <android/input.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <jni.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "LiteLauncher", __VA_ARGS__)

static ANativeWindow* window = nullptr;

struct AppItem {
    const char* name;
    const char* pkg;
};

std::vector<AppItem> apps = {
    {"Chrome", "com.android.chrome"},
    {"YouTube", "com.google.android.youtube"},
    {"WhatsApp", "com.whatsapp"},
    {"PlayStore", "com.android.vending"},
    {"ChatGPT", "com.openai.chatgpt"},
    {"Settings", "com.android.settings"},
    {"Files", "com.android.documentsui"}
};

// ---------------- DRAW ----------------
void draw() {
    if(!window) return;

    ANativeWindow_Buffer buffer;
    if(ANativeWindow_lock(window, &buffer, NULL) < 0) return;
    uint32_t* pixels = (uint32_t*) buffer.bits;

    // Clear black
    for(int y=0;y<buffer.height;y++)
        for(int x=0;x<buffer.width;x++)
            pixels[y*buffer.stride+x] = 0xFF000000;

    int cols = 3;
    int cellW = buffer.width / cols;
    int cellH = 200;

    for(int i=0;i<apps.size();i++){
        int row = i / cols;
        int col = i % cols;
        int startX = col*cellW+20;
        int startY = row*cellH+40;

        // kotak putih
        for(int y=startY;y<startY+60;y++)
            for(int x=startX;x<startX+cellW-40;x++)
                pixels[y*buffer.stride+x] = 0xFFFFFFFF;
    }

    ANativeWindow_unlockAndPost(window);
}

// ---------------- JNI LAUNCH ----------------
void launchAppJNI(android_app* app, const char* pkg){
    JavaVM* vm = app->activity->vm;
    JNIEnv* env;
    if(vm->AttachCurrentThread(&env,nullptr) != JNI_OK) return;

    jobject activity = app->activity->clazz;
    jclass cls = env->GetObjectClass(activity);
    jmethodID mid = env->GetMethodID(cls,"launchApp","(Ljava/lang/String;)V");
    if(mid){
        jstring jpkg = env->NewStringUTF(pkg);
        env->CallVoidMethod(activity, mid, jpkg);
        env->DeleteLocalRef(jpkg);
    }

    vm->DetachCurrentThread();
}

// ---------------- INPUT ----------------
void handle_input(android_app* app, AInputEvent* event){
    if(AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION){
        int action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        if(action == AMOTION_EVENT_ACTION_DOWN){
            float x = AMotionEvent_getX(event,0);
            float y = AMotionEvent_getY(event,0);

            int cols = 3;
            int cellH = 200;
            int cellW = ANativeWindow_getWidth(window)/cols;

            for(int i=0;i<apps.size();i++){
                int row=i/cols;
                int col=i%cols;
                int startX = col*cellW+20;
                int startY = row*cellH+40;

                if(x>=startX && x<=startX+cellW-40 &&
                   y>=startY && y<=startY+60){
                    LOGI("Launching app: %s", apps[i].pkg);
                    launchAppJNI(app, apps[i].pkg);
                }
            }
        }
    }
}

// ---------------- CMD ----------------
void handle_cmd(android_app* app, int32_t cmd){
    if(cmd == APP_CMD_INIT_WINDOW){
        window = app->window;
        draw();
    }
}

// ---------------- ANDROID MAIN ----------------
void android_main(android_app* state){
    state->onAppCmd = handle_cmd;
    state->onInputEvent = [](android_app* app, AInputEvent* event)->int32_t{
        handle_input(app,event);
        return 1;
    };

    int events;
    struct android_poll_source* source;

    while(true){
        while(ALooper_pollAll(0,NULL,&events,(void**)&source)>=0)
            if(source) source->process(state,source);

        if(window) draw();
        usleep(16000); // ~60fps
    }
}