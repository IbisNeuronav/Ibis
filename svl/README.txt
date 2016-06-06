========================================================================================================
IMPORTANT: This is the original README file that was distributed with Andrew Willmott SVL. This
package has been modified by Simon Drouin to build using CMake. Therefore, some of the build
instructions here might not apply. The code itself remains the same except for the addition of 
classes Box2i and Box2d.
========================================================================================================

Contents

    The SVL vector & matrix package, version 1.5

    SVL provides 2-, 3- and 4-vector and matrix types, as well as
    arbitrarily-sized vectors and matrices, and various useful functions
    and arithmetic operators.

    SVL is free for commercial and non-commercial use; see the LICENSE file
    for redistribution conditions. (These apply only to the source code.
    Binaries may be freely redistributed, no strings attached.)

Author

    Andrew Willmott, ajw+svl@cs.cmu.edu
    Please send me any suggestions/comments/bug fixes you have.

History

    Briefly: SVL, and its more complex cousin, VL, have their roots in
    an attempt replace the various (inevitably heavily-macroized)
    vector/matrix packages used by the members of the CMU graphics group
    with a single C++ one, so that people could share code more easily.
    SVL makes heavy use of inlines to achieve performance equal to that
    of the C macros and functions it replaced.
    
Documentation

    Double click on the "svl" file in the doc subdirectory for documentation.

To Use

	Open the provided workspace, svl.dsw. Hit F7 (build) to build the SVL
	library and a test program, and F5 to run the test program.

	The easiest way to use SVL from your own project is just to include
	the svl.dsp project file in its workspace. 

	Alternatively, use the svl.dsw workspace to build svl.lib and
	svl-dbg.lib, and then add these directly to your project. You can
	use Project/Add To Project/Files to do this, though you'll have to
	change the "Files of Type" popup.

	For any project that uses SVL, you must:

	(a) Add an svl dependency to that project if you've directly
	included svl.dsp
	
	(b) Add the VL_DEBUG symbol to the debug configuration. (Bring up
	the Project/Project Settings dialog and add it to the C/C++ pane's
	"Preprocessor definitions" field.) If you don't do this, inline
	functions won't have proper bounds checking.

    (c) In Project/Project Settings, add SVL's include directory to
    the C/C++ pane under Category: Preprocessor, "Additional include
    directories". You may have to specify a full or relative path,
    depending on where your project is.

    (d) #include <svl/SVL.h> in your source file. 

Compiling SVL under Windows

    If the workspace fails to work for some reason (or you're not using
    MSVC) below are the steps used to create it.

    (a) Create a static library project called "svl".

    (b) Add the files src\Basics.cxx and src\LibSVL.cxx to the project via
    Project/Add To Project/Files.

    (c) In Project/Project Settings, add the "include" directory to the
    C/C++ pane under Category: Preprocessor, "Additional include
    directories". You can also add the header files "svl/SVL.h" and 
    "svl/SVLgl.h" if you want.

    Once you have the project open and set up correctly, hit F7 to build
    the library.

Changes

    1.4     Fixed problem with SVLgl.h -- had some wrappers for OpenGL calls that
                don't actually exist(!)
            Fixed bad memory problem with Mat::SetSize
            Makefile for OSX, split sunos/solaris

    1.3.2   Fixed minor MSVC warnings
            Added windows packaging
    1.3.1   Sunday 11/03/01
            Reordered initializers, eliminated signed/unsigned comparisons
            Fixed make clean problem
            Irix CC is warning that 'Mat2::*' is being called before its inline
              definition, but appears to be on crack as the inline '*' is
              defined before the xform() that calls it.
            Fixed afs-lib dependency for afs build
    1.3     Sunday 11/03/01
            Split off from VL 1.3. All VL-dependent code and type
              parameterization macros removed to make code more understandable.
            Fix for the C++ "standard" no longer allowing stack temporaries to
              be passes as non-const refs.  (Fixing this for VL requires more
              significant changes, so it's proved easier to fork SVL at this
              point for the sake of simplicity.)
            Renamed files, general cleanups.
            Added ">>" operators for Vec and Mat.
            Disallowed resize of a vector reference. (This caused more bugs
              than it was worth.)
            Removed all asserts checking that the return value of 'new' is
              not zero. The C++ "standard" has been changed so that new
              instead throws an exception in this situation.
            Added option to use memcpy for general vector copies: VL_USE_MEMCPY.
              It seems that nowadays, on Intel processors at least, using
              memcpy is faster than loops for most cases. See note in Vec.cc.
            Changed the packaging to match the current vl/gcl scheme.
            Updated afs-setup rule to ensure SVLConfig.h winds up in the
            architecture-dependent lib directory.
    1.2     Release to match VL 1.2.
    1.1.3   Fixed some windows compile issues under VC++.
    1.1.2   Added comments about the Real, Vec, etc. types to VL.h
    1.1.1   Fixed problem with += operators and gcc under linux.
    1.1     Added oprod operator, irix n32 CC support, changed makefiles & afs
            support, eliminated most g++ warnings.
