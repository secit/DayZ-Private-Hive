Credits:
--------

Hive access library by Rajko Stojadinovic (https://github.com/rajkosto)
Bliss schema compatibility by Andrew DeLisa (https://github.com/ayan4m1)
Reworked by Crosire (https://github.com/Crosire)

Preparation:
------------

You must have a copy of Visual Studio 2012 installed. Also required are
some additional libraries:

Get boost from "http://www.boost.org/" and extract the archive into the
"dep/boost" folder.

Get poco from "http://pocoproject.org/download/index.html" (basic edition
is sufficient). It must be extracted into the "dep/poco" folder. After
extracting, edit "build_vs110.bat" with a text editor, and replace the word
"shared" in the last line with "all". Optionally, you can also replace the
word "samples" with "nosamples" for faster builds. After that, simply execute
the batch file.

Get tbb from "http://threadingbuildingblocks.org/download". It is
recommended to get the precompiled binaries for windows of the latest stable
release. Extract the downloaded archive into "dep/tbb" folder.

Building:
---------

You are now ready to open "src/Hive.sln" in Visual Studio 2012.
From there, simply compile the Release configuration to get binaries that can
be used in-game. The Debug configuration simply compiles an executable that can
run tests.