# Add project specific ProGuard rules here.
# You can control the set of applied configuration files using the
# proguardFiles setting in build.gradle.
#
# For more details, see
#   http://developer.android.com/guide/developing/tools/proguard.html

# If your project uses WebView with JS, uncomment the following
# and specify the fully qualified class name to the JavaScript interface
# class:
#-keepclassmembers class fqcn.of.javascript.interface.for.webview {
#   public *;
#}

# Uncomment this to preserve the line number information for
# debugging stack traces.
#-keepattributes SourceFile,LineNumberTable

# If you keep the line number information, uncomment this to
# hide the original source file name.
#-renamesourcefileattribute SourceFile
-repackageclasses
-verbose
-keep,allowshrinking,allowoptimization class com.garmin.android.connectiq.IQDevice
-keep,allowshrinking,allowoptimization class com.garmin.android.connectiq.IQApp
-keep,allowshrinking,allowoptimization class android.widget.Spinner
#-keep class com.garmin.android.connectiq.IQMessage # Needed?
#-keep class tk.glucodata.MainActivity
#-keep,allowshrinking,allowoptimization class tk.glucodata.nums.item # Doesnt work

#-keepclassmembernames class tk.glucodata.GlucoseCurve { # doesnt work. Only refered from native code
#	void summaryready() ;
#}

