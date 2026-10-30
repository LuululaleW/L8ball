# Keep the launcher activity and the Java bridge used by the native module.
-keep public class com.eightballpool.bp.MainActivity { *; }
-keep public class android.service.SurfaceView { *; }

# JNI entry points must keep their names.
-keepclasseswithmembernames class * {
    native <methods>;
}

# Strip debug logs from release builds.
-assumenosideeffects class android.util.Log {
    public static *** d(...);
    public static *** v(...);
    public static *** i(...);
    public static *** w(...);
    public static *** e(...);
}

-optimizationpasses 5
-allowaccessmodification
-overloadaggressively
-repackageclasses ''
