ui_print "*******************************"
ui_print "LLVM-xRT CONFIGURATION"
ui_print "*******************************"

ui_print ""
ui_print "This is for Pixel 1st Gen"

set_perm_recursive  $MODPATH                0       0       0755        0644
set_perm  $MODPATH/system/apex/com.android.runtime.release/bin/dex2oat 0      shell       0755
set_perm  $MODPATH/system/apex/com.android.runtime/bin/linker                 root    shell     0755
set_perm  $MODPATH/system/apex/com.android.runtime/bin/linker64               root    shell     0755
