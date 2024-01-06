# Daily Login Clicker

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

## Build

1. In Visual Studio, setup a project called 'clicker' with home directory in this git repository.

2. Add click.cpp as an existing source file

3. In 'Project'->'Properties', select 'Release' from the 'Configuration' dropdown and 'x64' from the 'Platform' dropdown. Stay in this menu for the following steps.

4. Navigate to 'Configuration Properties' > '/C++' > 'Language.'Find the option 'C++ Language Standard' and change it to ISO C++17 Standard (/std:c++17) or later.

4. Click on  path 'C/C++'->'General' and add this path `E:\opencv\install\opencv\include` to the 'Additional Include Directories'

5. Click on 'Linker'->'General' and add this lib path 'E:\opencv\install\opencv\x64\vc17\lib' to the 'Additional Library Directories'. If building statically, instead the lib path is instead `E:\opencv\install\opencv\x64\vc17\staticlib`

6. Click on 'Linker'->'Input' and add from this path'E:\opencv\install\opencv\x64\vc17\lib' the name of the opencv_worldX.lib file (where X is a version number e.g. 'opencv_world480.lib') to the 'Additional Dependencies'. If building statically, instead add the names of all .lib files (less those that are debug versions whose names before the .lib end with the letter d), the list was for me the following

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

7. If building statically, click on 'C/C++' then 'Code Generation' and set 'Runtime Library' to 'Multi-threaded (/MT)'

8. Exit Visual Studio, and run the 'build.sh' script from the top directory of this repository.

## Running

1. Make a copy of the config.ini.template but rename it to config.ini and provide file paths and usernames and passwords

2. Run the 'run.sh' script from the top directory of this repository.

make a basic task in task scheduler with 'wsl'as the program and '/mnt/c/game/clicker/clicker/x64/release/clicker.exe'the path to the executable as the argument and 'C:\game' the path to the git repo as the 'start in'. Select 04:00am on the first of every month