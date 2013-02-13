Hive access library 		by Rajko Stojadinovic (https://github.com/rajkosto)
Bliss schema compatibility 	by Andrew DeLisa (https://github.com/ayan4m1)
Completly reordered 		by Crosire (https://github.com/Crosire)

Compiling:
----------

You must have a copy of Visual Studio 2012 installed.

Also required is the boost library, both includes and compiled libraries.
If you don't have it, get boost from "http://www.boost.org/".
Compilation instructions (how to use bjam for your compiler) can be found
at "http://www.boost.org/doc/libs/1_52_0/more/getting_started/windows.html"
Once compiled, make sure to add the include and lib folders to your VC++
directories inside Visual Studio.

Get poco from "http://pocoproject.org/download/index.html" (basic edition
is sufficient). It must be extracted into Dependencies10/poco folder, such
that the folder structure matches the one in INSTRUCTIONS.TXT. After
extracting, edit build_vs110.bat with a text editor, and replace the word
"shared" in the last line with "all". Optionally, you can also replace the
word "samples" with "nosamples" for faster builds. After that, create a
Visual Studio Command prompt, and invoke build_vs100.bat from it

Get tbb from "http://threadingbuildingblocks.org/download". It is
recommended to get the precompiled binaries for windows of the latest stable
release. Compilation of this library will not be discussed here.
Extract the downloaded archive into dep/tbb folder.

You are now ready to open src/Hive.sln in Visual Studio 2012.
From there, simply compile the Release configuration to get binaries that can be used in-game.
The Debug configuration simply compiles an executable that runs some tests.