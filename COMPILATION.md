
# A. COMPILATION OPTIONS:
##### `ART_MCR_COMPILE_OS_METHODS`
Make OS/framework methods available for LLVM compilation.
This decreases the amount of runtime interactions and
increases the amount of code that LLVM compiles.

There are bugs on some internal methods.
Might require histograms to pull methods from the OS.
Static framework methods are also added to the histogram,
with nulls (zero's) to represent the callsites
(in contrary with any virtual calls in the histogram).

---

# B. OTHER NOTES:

### `mcr` keyword in several places in the code:
The LLVM backend was a part of a bigger project that was performing a
minimal capture and replay for Android methods.
`mcr` are leftovers when the LLVM backend was separated from those internal projects.

### OS method compilation:
OS methods that fail to compile will be stored at:  
`DIR_MCR/os_methods.comp.failed`

Other OS methods can be specifically ignored using the file:
`DIR_MCR/os_methods.blocklist`

## KNOWN BUGS:
There are multiple unimplemented instructions, as well instructions that are not fully implemented
and resort more frequently to the runtime.
There are also bugs that might hit in particular cases.

A known bug is that some applications, like GoogleCamera, crash on launch due to some 
initialization in the runtime. This was fixed in private repositories, that are part
of a bigger project, however, on the published code that is focused around the LLVM backend
this issue unfortunately was reintroduced.
It's something minor, like trying to extract a package of an app based on its profile filename,
and that app belongs to the framework (is internal) so it doesn't have a proper name.
Or it could be some interference with the JIT compiler (some improper initialization due to our additions).
And we end up doing a null pointer dereference.
