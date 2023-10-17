![.](https://live.staticflickr.com/65535/53265252304_3be65ecef1_c_d.jpg)
# OpenGL Flight Simulator

This is a hand-crafted flight simulator with simple air dynamic approximation and physic simulation. It's a project I made for the computer graphics class. I also port the cloudwork shader to this project to provide greate visual effects.

# Compile

Install OpenGL dependencies

    sudo apt-get install build-essential libgl1-mesa-devlibglu1-mesa-dev 
    sudo apt-get install freeglut3-dev libglew1.8 libglew-dev libgl1-mesa-glx

Compile the project

    cmake . && make

Excute

    cd flight_sim   
    ./flight_sim

# Control

## Game Status
`F1:` Pause/Start

`F2:` Display/Hide Force and Plane Boxes

`F3:` Reload Shaders

`F2:` Display/Hide F22

`Q/E:` Move Sun

## Camera Control
`Mouse Drag Move:` Move Camera Around

`Moush Wheel:` Move Camera Distance

## Flight Control
`Increase/Decrease Thrust:` W/S

`Roll left/right:` Mouse Move Left/Right

`Pull up:` Mouse Move Downward

`Nose down:` Mouse Move Upward

# License & Declaration

Please note that it's kinda buggy since it's purly my personal side project for fun. Portions of the source code (gltools, math3D) are copyrighted by Richard S. Wright Jr. 

Also, the texture of the F22 is by I. Mikhelevich (https://www.the-blueprints.com/blueprints/modernplanes/lockheed/45834/view/lockheed_martinboeing_f-22_raptor/) and the texture of the ground is from google. While the 3D models are made by myself.

This project is experimental and the cloudwork source code is under Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.