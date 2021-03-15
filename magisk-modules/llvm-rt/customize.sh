ui_print "*******************************"
ui_print "LLVM-RT CONFIGURATION"
ui_print "*******************************"

set_perm_recursive  $MODPATH                0       0       0755        0644

set_perm  $MODPATH/system/apex/com.android.runtime/bin/dex2oat                root    shell     0755
set_perm  $MODPATH/system/apex/com.android.runtime/bin/linker                 root    shell     0755
set_perm  $MODPATH/system/apex/com.android.runtime/bin/linker64               root    shell     0755
