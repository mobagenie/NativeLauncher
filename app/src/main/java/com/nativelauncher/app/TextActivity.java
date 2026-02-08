package com.nativelauncher.app;

import android.app.NativeActivity;
import android.os.Bundle;
import android.widget.LinearLayout;
import android.widget.TextView;

public class TextActivity extends NativeActivity {

    @Override
    protected void onCreate(Bundle b){
        super.onCreate(b);

        LinearLayout l = new LinearLayout(this);
        l.setOrientation(LinearLayout.VERTICAL);

        String[] apps = {
                "Chrome",
                "YouTube",
                "WhatsApp",
                "PlayStore",
                "ChatGPT",
                "Settings",
                "Files"
        };

        for(String s:apps){
            TextView t=new TextView(this);
            t.setText(s);
            t.setTextSize(24);
            t.setPadding(40,40,40,40);
            l.addView(t);
        }

        setContentView(l);
    }
}