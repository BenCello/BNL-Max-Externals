#! /bin/zsh
rm -rf $(find . -d 1 -iname "*cmake*" ! -name CMakeLists.txt ! -name "*.zsh")
rm -rf $(find . -d 1 -iname "*build*" ! -name "*.zsh")
rm -rf $(find . -d 1 -iname "*.xcodeproj" ! -name "*.zsh")
