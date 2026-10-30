package android.service;

import android.content.Context;
import android.graphics.Canvas;
import android.util.AttributeSet;

/**
 * Java bridge for the native module. The class name and methods must match the
 * JNI symbols exported by {@code libgdtqis.so}
 * (see {@code app/src/main/jni/main.cpp}).
 *
 * <p>This class is optional: the module is normally injected into the game and
 * the host app calls these methods through its own dex. Keeping the class here
 * makes the JNI symbols resolvable and documents the expected contract.</p>
 */
public class SurfaceView extends android.view.SurfaceView {

    private static boolean sModuleLoaded;

    public SurfaceView(Context context) {
        super(context);
    }

    public SurfaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public SurfaceView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    /**
     * Loads the native module into the current process on demand. Do not call
     * this from the companion app: the module expects to run inside
     * {@code com.miniclip.eightballpool}.
     */
    public static synchronized void loadModule() {
        if (!sModuleLoaded) {
            System.loadLibrary("gdtqis");
            sModuleLoaded = true;
        }
    }

    public static synchronized boolean isModuleLoaded() {
        return sModuleLoaded;
    }

    /** Sends a boolean feature flag to the native menu. */
    public native void onSendConfig(String key, String value);

    /** Draw hook used by overlay implementations. Currently a no-op in C++. */
    public native void onCanvasDraw(Canvas canvas, int width, int height, float density);

    /** Returns the license label. Always "Lifetime" in this build. */
    public native String getExpTime();

    /** Returns whether the menu is unlocked. Always true in this build. */
    public native boolean MenuColor();
}
