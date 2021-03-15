ui_print "-----"
ui_print "| LLVM-static: compiler libs and tools"
ui_print "-----"
ui_print "Install:"
ui_print "  - clang"
ui_print "  - ld"
ui_print "  - llc"
ui_print "  - llvm-link"
ui_print "  - lmkd: modifying the low-memory killer tool"
ui_print "  - /system/sysroot for llvm (from NDK29)"
ui_print "Symlinks:" 
ui_print "  - art apex libraries that are link against LLVM code."
ui_print "SELinux policy:" 
ui_print "  - sets to permissive for all" 
ui_print "system.prop:" 
ui_print "  - modifies low memory killing (adjusts lmkd)" 


set_perm_recursive  $MODPATH                0       0       0755        0644
set_perm  $MODPATH/system/bin/clang         0       0       0755
set_perm  $MODPATH/system/bin/ld            0       0       0755
set_perm  $MODPATH/system/bin/llc           0       0       0755
set_perm  $MODPATH/system/bin/opt           0       0       0755
set_perm  $MODPATH/system/bin/llvm-link     0       0       0755
