package com.nativelauncher;

import android.app.NativeActivity;
import android.content.Intent;
import android.util.Log;

public class MainActivity extends NativeActivity {

    // Method dipanggil dari C++ via JNI
    public void launchApp(String packageName) {
        try {
            Intent intent = getPackageManager().getLaunchIntentForPackage(packageName);
            if (intent != null) {
                startActivity(intent);
            } else {
                Log.i("LiteLauncher", "App not installed: " + packageName);
            }
        } catch (Exception e) {
            Log.e("LiteLauncher", "Launch failed", e);
        }
    }
}

package com.nativelauncher;

import android.app.NativeActivity;
import android.content.Intent;
import android.util.Log;

public class MainActivity extends NativeActivity {

    // Method dipanggil dari C++ via JNI
    public void launchApp(String packageName) {
        try {
            Intent intent = getPackageManager().getLaunchIntentForPackage(packageName);
            if (intent != null) {
                startActivity(intent);
            } else {
                Log.i("LiteLauncher", "App not installed: " + packageName);
            }
        } catch (Exception e) {
            Log.e("LiteLauncher", "Launch failed", e);
        }
    }
}

