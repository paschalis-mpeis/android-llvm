device=$(getprop ro.build.product)

if [[ $device == "sailfish" ]]; then
  ART_APEX=/system/apex/com.android.runtime.release
else
  ART_APEX=/apex/com.android.runtime
fi

LIB32=$ART_APEX/lib
LIB64=$ART_APEX/lib64

# Symlink a library to hf.so can successfully dlopen (RTLD_NOW)
function symlink_lib() {
    library="$1.so"
    ln -s $LIB32/$library /system/lib
    ln -s $LIB64/$library /system/lib64
}

# from art/runtime/Android.bp
symlink_lib "libart"
symlink_lib "libnativebridge"
symlink_lib "libartpalette"
symlink_lib "libnativeloader"
symlink_lib "libbacktrace"
symlink_lib "libbase"
symlink_lib "libartbase"
symlink_lib "libdexfile"
symlink_lib "libprofile"
symlink_lib "libsigchain"
symlink_lib "libprofile"
