package com.nativelauncher.app;

import android.app.NativeActivity;
import android.content.Intent;
import android.util.Log;

public class MainActivity extends NativeActivity {

    // Dipanggil dari JNI
    public void launchApp(String pkg) {
        try {
            Intent intent = getPackageManager().getLaunchIntentForPackage(pkg);
            if(intent != null) {
                startActivity(intent);
            } else {
                Log.i("LiteLauncher","App not installed: " + pkg);
            }
        } catch(Exception e){
            Log.e("LiteLauncher","Launch failed", e);
        }
    }
}