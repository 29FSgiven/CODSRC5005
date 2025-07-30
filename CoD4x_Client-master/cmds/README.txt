Files are designed to be launched from project root directory.
That's default behavior using these files as callee in VSCode.

Example of build command in tasks.json:
{
    "label": "release_mingw_build",
    "type": "shell",
    "command": "<PATH_TO_PROJECT_ROOT>/cmds/release_mingw_build.cmd",
    "problemMatcher": [],
    "group": "build"
}

Purpose of these files:
 - build: configure and build project
 - clean: remove any files (directories are kept) from build directory
 - configure: initialize cmake build system in a special directory.
 - rebuild: do clean and build in single operation.
 - relink: remove any target files (*.dll and *.a) from build directory and build in single operation. Used to faster switch between build types due to autodeploy.
