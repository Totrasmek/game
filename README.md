# Daily Login Clicker

## Outstanding issues

1. Sometimes we get stuck trying to enter areas. Maybe we are within the pixel threshold but the entry text isn't showing. Strangely alt-tabbing solves this? Is a screenshot taken whilst alt tabbing that prompts another rotate? We need a fix for this

2. Still need to test the claim process

3. Crashes like the following occur when an image is found but can't be clicked on e.g. 'play', then the game attempts to launch (I think) but can't be found but we still take a screenshot then opencv dies or something:
Waiting for load
OpenCV: terminate handler is called! The last OpenCV error is:OpenCV(4.9.0-dev) Error: Assertion failed (s >= 0) in cv::setSize, file E:\opencv\opencv\modules\core\src\matrix.cpp, line 246 

## Requirements

1. **Windows** 10 or 11.
2. **WSL** with the following
```
wsl --install
wsl --install -d Ubuntu-22.04
sudo apt-get install build-essential
```
3. **Microsoft Visual Studio** 2022 is what I used, and add MSBuild.exe to path, it's install location is usually
```
C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\msbuild.exe
```
4. **OpenCV** installed. Can probably use prebuilt binaries, but I built from git-bash following this tutorial (https://docs.opencv.org/4.x/d3/d52/tutorial_windows_install.html)/
In OCVInstall.sh, ensure you set `CMAKE_GENERATOR_OPTIONS=-G"Visual Studio 17 2022"`, and to build the OpenCV libraries statically add `-DBUILD_SHARED_LIBS=OFF to the CMAKE_OPTIONS`

5. **Chilkat** for Windows Visual Studio 2022 x64. 
Download here (https://chilkatdownload.com/9.5.0.96/chilkat-9.5.0-x86-vc2022.zip  https://chilkatdownload.com/9.5.0.96/chilkat-9.5.0-x86_64-vc2022.zip)

## Build

1. In Visual Studio, setup a project called *clicker* with home directory in this git repository.

2. Add click.cpp as an existing source file

3. In *Project*->*Properties*, select *Release* from the *Configuration* dropdown and *x64* from the *Platform* dropdown. Stay in this menu for the following steps.

4. Navigate to *Configuration Properties* > */C++* > *Language.* Find the option *C++ Language Standard* and change it to ISO C++17 Standard (/std:c++17) or later.

4. Click on  path *C/C++*->*General* and add this path `E:\opencv\install\opencv\include` and `E:\chilkat-9.5.0-x86_64-vc2022\include` to the *Additional Include Directories*.

5. Click on *Linker*->*General* and add this lib path `E:\opencv\install\opencv\x64\vc17\lib` to the *Additional Library Directories*. If building statically, INSTEAD the lib path is instead `E:\opencv\install\opencv\x64\vc17\staticlib`. In both cases, also add `E:\chilkat-9.5.0-x86_64-vc2022\libs`.

6. Click on *Linker*->*Input* and add from this path `E:\opencv\install\opencv\x64\vc17\lib` the name of the opencv_worldX.lib file (where X is a version number e.g. 'opencv_world480.lib') to the 'Additional Dependencies'. If building statically, INSTEAD add the names of all .lib files (less those that are debug versions whose names before the .lib end with the letter d) from this directory `E:\opencv\install\opencv\x64\vc17\staticlib`.  The list was for me the following:

```
ade.lib
IlmImf.lib
ippicvmt.lib
ippiw.lib
ittnotify.lib
libjpeg-turbo.lib
libopenjp2.lib
libpng.lib
libprotobuf.lib
libtiff.lib
libwebp.lib
opencv_img_hash490.lib
opencv_world490.lib
zlib.lib
```

In both cases, also add the chilkat release library that is either DLL (for dynamic linking) or not (for static linking):
```
ChilkatRel_x64.lib
ChilkatRelDll_x64.lib
```

7. If building statically, click on *C/C++* then *Code Generation* and set *Runtime Library* to *Multi-threaded (/MT)*

8. Exit Visual Studio, and run the *build.sh* script from the top directory of this repository.

## Running

1. Make a copy of the config.ini.template but rename it to config.ini and provide file paths and usernames and passwords

2. Make a subdirectory called *screenshots* and within that a subdirectory whose names is the default pixel width of that the game loads to on your device, e.g. *1080*, *1440*. Populate with this bottom directory with screenshots from your own game matching the images at this google drive link (https://drive.google.com/drive/folders/1jrnF-P1wp0RVqY8x1tLosFaGhAr7AEgw?usp=sharing).

3. Run the *run.sh* script from the top directory of this repository.

add screenshots from the game to a 

make a basic task in task scheduler with 'wsl'as the program and '/mnt/c/game/clicker/clicker/x64/release/clicker.exe'the path to the executable as the argument and 'C:\game' the path to the git repo as the 'start in'. Select 04:00am on the first of every month