# EasyFormDrop
A C++/Qt application to load fields from `config/config.json`, copy values to clipboard on click, and auto-paste on drag.

## Build
```bash
sudo apt install qtbase5-dev qt5-qmake libqt5widgets5 libx11-dev
mkdir build
cd build
cmake ..
make
./EasyFormDrop
