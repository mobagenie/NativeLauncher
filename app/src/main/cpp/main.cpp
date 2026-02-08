#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include <android_native_app_glue.h>
#include <unistd.h>
#include <vector>

static ANativeWindow* window=nullptr;
static bool running=true;
static bool fontReady=false;

stbtt_fontinfo font;
unsigned char ttf[1<<20];

struct App{ const char* name; const char* pkg; };

std::vector<App> apps={
 {"Chrome","com.android.chrome"},
 {"YouTube","com.google.android.youtube"},
 {"WhatsApp","com.whatsapp"},
 {"PlayStore","com.android.vending"},
 {"ChatGPT","com.openai.chatgpt"},
 {"Settings","com.android.settings"},
 {"Files","com.android.documentsui"}
};

void drawText(uint32_t* p,int w,int x,int y,const char* t){
 if(!fontReady) return;

 float s=stbtt_ScaleForPixelHeight(&font,28);
 int cx=x;

 while(*t){
  int ax,l;
  stbtt_GetCodepointHMetrics(&font,*t,&ax,&l);

  int x0,y0,x1,y1;
  stbtt_GetCodepointBitmapBox(&font,*t,s,s,&x0,&y0,&x1,&y1);

  int bw=x1-x0,bh=y1-y0;
  if(bw<=0||bh<=0){ t++; continue; }

  std::vector<unsigned char> b(bw*bh);
  stbtt_MakeCodepointBitmap(&font,b.data(),bw,bh,bw,s,s,*t);

  for(int j=0;j<bh;j++)
   for(int i=0;i<bw;i++)
    if(b[j*bw+i])
     p[(y+j)*w+(cx+i)]=0xffffffff;

  cx+=ax*s;
  t++;
 }
}

void draw(){
 if(!window) return;

 ANativeWindow_Buffer buf;
 if(ANativeWindow_lock(window,&buf,nullptr)!=0) return;

 uint32_t* p=(uint32_t*)buf.bits;
 memset(p,0,buf.height*buf.stride*4);

 int cols=3;
 int cw=buf.width/cols;

 for(int i=0;i<apps.size();i++){
  int r=i/cols,c=i%cols;
  int sx=c*cw+20;
  int sy=r*220+40;

  for(int y=sy;y<sy+120;y++)
   for(int x=sx;x<sx+cw-40;x++)
    p[y*buf.stride+x]=0xffffffff;

  drawText(p,buf.stride,sx+15,sy+150,apps[i].name);
 }

 ANativeWindow_unlockAndPost(window);
}

void launch(android_app* app,const char* pkg){
 JNIEnv* e;
 app->activity->vm->AttachCurrentThread(&e,nullptr);

 jclass c=e->GetObjectClass(app->activity->clazz);
 jmethodID pm=e->GetMethodID(c,"getPackageManager","()Landroid/content/pm/PackageManager;");
 jobject m=e->CallObjectMethod(app->activity->clazz,pm);

 jclass pc=e->GetObjectClass(m);
 jmethodID gi=e->GetMethodID(pc,"getLaunchIntentForPackage","(Ljava/lang/String;)Landroid/content/Intent;");
 jstring j=e->NewStringUTF(pkg);

 jobject it=e->CallObjectMethod(m,gi,j);

 if(it){
  jmethodID st=e->GetMethodID(c,"startActivity","(Landroid/content/Intent;)V");
  e->CallVoidMethod(app->activity->clazz,st,it);
 }

 app->activity->vm->DetachCurrentThread();
}

int32_t input(android_app* app,AInputEvent* ev){
 if(!window) return 0;

 if(AInputEvent_getType(ev)!=AINPUT_EVENT_TYPE_MOTION) return 0;

 if((AMotionEvent_getAction(ev)&AMOTION_EVENT_ACTION_MASK)==AMOTION_EVENT_ACTION_DOWN){

  float x=AMotionEvent_getX(ev,0);
  float y=AMotionEvent_getY(ev,0);

  int cw=ANativeWindow_getWidth(window)/3;

  for(int i=0;i<apps.size();i++){
   int r=i/3,c=i%3;
   int sx=c*cw+20;
   int sy=r*220+40;

   if(x>=sx&&x<=sx+cw-40&&y>=sy&&y<=sy+120)
    launch(app,apps[i].pkg);
  }
 }
 return 1;
}

void cmd(android_app* app,int32_t c){
 if(c==APP_CMD_INIT_WINDOW){
  window=app->window;

  if(!fontReady){
   AAsset* a=AAssetManager_open(app->activity->assetManager,"font.ttf",0);
   AAsset_read(a,ttf,AAsset_getLength(a));
   AAsset_close(a);
   stbtt_InitFont(&font,ttf,0);
   fontReady=true;
  }
 }

 if(c==APP_CMD_TERM_WINDOW) window=nullptr;
 if(c==APP_CMD_DESTROY) running=false;
}

void android_main(android_app* app){

 app->onInputEvent=input;
 app->onAppCmd=cmd;

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