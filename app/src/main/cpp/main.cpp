#include <android_native_app_glue.h>
#include <android/log.h>
#include <jni.h>
#include <unistd.h>
#include <vector>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,"LiteLauncher",__VA_ARGS__)

static ANativeWindow* gWindow=nullptr;
static bool running=true;

struct AppItem{
    const char* name;
    const char* pkg;
};

std::vector<AppItem> apps={
    {"Chrome","com.android.chrome"},
    {"YouTube","com.google.android.youtube"},
    {"WhatsApp","com.whatsapp"},
    {"PlayStore","com.android.vending"},
    {"ChatGPT","com.openai.chatgpt"},
    {"Settings","com.android.settings"},
    {"Files","com.android.documentsui"}
};

void draw(){
    if(!gWindow) return;

    ANativeWindow_Buffer buf;
    if(ANativeWindow_lock(gWindow,&buf,nullptr)!=0) return;

    uint32_t* p=(uint32_t*)buf.bits;

    for(int y=0;y<buf.height;y++)
        for(int x=0;x<buf.width;x++)
            p[y*buf.stride+x]=0xFF000000;

    int cols=3;
    int cellW=buf.width/cols;

    for(int i=0;i<apps.size();i++){
        int r=i/cols,c=i%cols;
        int sx=c*cellW+20;
        int sy=r*200+40;

        for(int y=sy;y<sy+80;y++)
            for(int x=sx;x<sx+cellW-40;x++)
                p[y*buf.stride+x]=0xFFFFFFFF;
    }

    ANativeWindow_unlockAndPost(gWindow);
}

void launchApp(android_app* app,const char* pkg){
    JNIEnv* env;
    app->activity->vm->AttachCurrentThread(&env,nullptr);

    jclass activityCls=env->GetObjectClass(app->activity->clazz);

    jmethodID getPM=env->GetMethodID(activityCls,
        "getPackageManager","()Landroid/content/pm/PackageManager;");

    jobject pm=env->CallObjectMethod(app->activity->clazz,getPM);

    jclass pmCls=env->GetObjectClass(pm);

    jmethodID getLaunch=env->GetMethodID(pmCls,
        "getLaunchIntentForPackage",
        "(Ljava/lang/String;)Landroid/content/Intent;");

    jstring jpkg=env->NewStringUTF(pkg);

    jobject intent=env->CallObjectMethod(pm,getLaunch,jpkg);

    if(intent){
        jmethodID start=env->GetMethodID(activityCls,
            "startActivity","(Landroid/content/Intent;)V");

        env->CallVoidMethod(app->activity->clazz,start,intent);
    }

    env->DeleteLocalRef(jpkg);
    app->activity->vm->DetachCurrentThread();
}

int32_t onInput(android_app* app,AInputEvent* e){
    if(AInputEvent_getType(e)!=AINPUT_EVENT_TYPE_MOTION) return 0;

    if((AMotionEvent_getAction(e)&AMOTION_EVENT_ACTION_MASK)==AMOTION_EVENT_ACTION_DOWN){

        float x=AMotionEvent_getX(e,0);
        float y=AMotionEvent_getY(e,0);

        int cols=3;
        int cellW=ANativeWindow_getWidth(gWindow)/cols;

        for(int i=0;i<apps.size();i++){
            int r=i/cols,c=i%cols;
            int sx=c*cellW+20;
            int sy=r*200+40;

            if(x>=sx && x<=sx+cellW-40 &&
               y>=sy && y<=sy+80){
                launchApp(app,apps[i].pkg);
            }
        }
    }
    return 1;
}

void onCmd(android_app* app,int32_t cmd){
    if(cmd==APP_CMD_INIT_WINDOW) gWindow=app->window;
    if(cmd==APP_CMD_TERM_WINDOW) gWindow=nullptr;
    if(cmd==APP_CMD_DESTROY) running=false;
}

void android_main(android_app* app){
    app->onAppCmd=onCmd;
    app->onInputEvent=onInput;

    while(running){
        int events;
        android_poll_source* src;

        while(ALooper_pollAll(gWindow?0:-1,nullptr,&events,(void**)&src)>=0){
            if(src) src->process(app,src);
        }

        if(gWindow) draw();
        usleep(16000);
    }
}