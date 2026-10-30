package com.eightballpool.bp;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Color;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

/**
 * Companion app for the native module.
 *
 * <p>The hook engine ({@code libgdtqis.so}) has to run inside the target game
 * process. This activity never loads it automatically: it only shows the module
 * status, lets the user copy the library out of the APK, and opens the game.
 * Login, license keys and expiry checks have been removed.</p>
 */
public class MainActivity extends Activity {

    private static final String TARGET_PACKAGE = "com.miniclip.eightballpool";
    private static final String MODULE_NAME = "libgdtqis.so";

    private TextView statusView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(buildUi());
        refreshStatus();
    }

    @Override
    protected void onResume() {
        super.onResume();
        refreshStatus();
    }

    private View buildUi() {
        int pad = dp(20);

        ScrollView scroll = new ScrollView(this);
        scroll.setBackgroundColor(Color.rgb(8, 11, 20));
        scroll.setFillViewport(true);

        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(pad, pad, pad, pad);
        scroll.addView(root, new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT));

        TextView title = new TextView(this);
        title.setText("8BP Native Module");
        title.setTextColor(Color.WHITE);
        title.setTextSize(26);
        title.setGravity(Gravity.START);
        root.addView(title, matchWrap());

        TextView subtitle = new TextView(this);
        subtitle.setText("No login / no expiry \u2022 arm64-v8a");
        subtitle.setTextColor(Color.rgb(140, 190, 255));
        subtitle.setTextSize(14);
        addSpaced(root, subtitle, 4);

        statusView = new TextView(this);
        statusView.setTextColor(Color.rgb(200, 210, 230));
        statusView.setTextSize(14);
        statusView.setLineSpacing(0f, 1.25f);
        addSpaced(root, statusView, 18);

        Button extract = new Button(this);
        extract.setText("Extract " + MODULE_NAME);
        extract.setAllCaps(false);
        extract.setOnClickListener(v -> extractModule());
        addSpaced(root, extract, 8);

        Button openGame = new Button(this);
        openGame.setText("Open 8 Ball Pool");
        openGame.setAllCaps(false);
        openGame.setOnClickListener(v -> openGame());
        addSpaced(root, openGame, 8);

        Button info = new Button(this);
        info.setText("How to use");
        info.setAllCaps(false);
        info.setOnClickListener(v -> showInfo());
        addSpaced(root, info, 8);

        TextView footer = new TextView(this);
        footer.setText("The module must be injected into the game by your root / Zygisk "
                + "or patched-APK setup. This app only bundles and extracts the library.");
        footer.setTextColor(Color.rgb(120, 130, 150));
        footer.setTextSize(12);
        footer.setLineSpacing(0f, 1.2f);
        addSpaced(root, footer, 24);

        return scroll;
    }

    private LinearLayout.LayoutParams matchWrap() {
        return new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT);
    }

    private void addSpaced(LinearLayout parent, View child, int topDp) {
        LinearLayout.LayoutParams lp = matchWrap();
        lp.topMargin = dp(topDp);
        parent.addView(child, lp);
    }

    private void refreshStatus() {
        boolean installed = isGameInstalled();
        File lib = new File(getApplicationInfo().nativeLibraryDir, MODULE_NAME);

        String text = "Target: " + TARGET_PACKAGE + "\n"
                + "Game installed: " + (installed ? "yes" : "no") + "\n"
                + "Device ABI: " + (Build.SUPPORTED_ABIS.length > 0 ? Build.SUPPORTED_ABIS[0] : "unknown") + "\n"
                + "Module bundled: " + (lib.exists() ? "yes" : "no") + "\n"
                + "License: Lifetime";

        statusView.setText(text);
    }

    private boolean isGameInstalled() {
        try {
            getPackageManager().getPackageInfo(TARGET_PACKAGE, 0);
            return true;
        } catch (Exception ignored) {
            return false;
        }
    }

    private void openGame() {
        Intent launch = getPackageManager().getLaunchIntentForPackage(TARGET_PACKAGE);
        if (launch == null) {
            Toast.makeText(this, "8 Ball Pool is not installed", Toast.LENGTH_SHORT).show();
            return;
        }
        launch.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        startActivity(launch);
    }

    /**
     * Copies the packaged library to the app-specific external folder so it can
     * be picked up by an injector. No storage permission is required.
     */
    private void extractModule() {
        File source = new File(getApplicationInfo().nativeLibraryDir, MODULE_NAME);
        if (!source.exists()) {
            Toast.makeText(this, "Module not found in this APK", Toast.LENGTH_SHORT).show();
            return;
        }

        File outDir = getExternalFilesDir("module");
        if (outDir == null) {
            Toast.makeText(this, "External storage unavailable", Toast.LENGTH_SHORT).show();
            return;
        }
        if (!outDir.exists() && !outDir.mkdirs()) {
            Toast.makeText(this, "Cannot create output folder", Toast.LENGTH_SHORT).show();
            return;
        }

        File target = new File(outDir, MODULE_NAME);
        try (InputStream in = new FileInputStream(source);
             OutputStream out = new FileOutputStream(target)) {
            byte[] buffer = new byte[64 * 1024];
            int read;
            while ((read = in.read(buffer)) != -1) {
                out.write(buffer, 0, read);
            }
            Toast.makeText(this, "Saved to " + target.getAbsolutePath(), Toast.LENGTH_LONG).show();
        } catch (IOException e) {
            Toast.makeText(this, "Extract failed: " + e.getMessage(), Toast.LENGTH_LONG).show();
        }
    }

    private void showInfo() {
        new android.app.AlertDialog.Builder(this)
                .setTitle("How to use")
                .setMessage("1. Install this APK.\n"
                        + "2. Tap \"Extract libgdtqis.so\".\n"
                        + "3. Copy the file into your injector / Zygisk module directory.\n"
                        + "4. Scope it to com.miniclip.eightballpool, then start the game.\n\n"
                        + "The menu opens from the floating button. Login and key expiry "
                        + "are disabled in this build.")
                .setPositiveButton("OK", null)
                .show();
    }

    private int dp(int value) {
        return Math.round(value * getResources().getDisplayMetrics().density);
    }
}
