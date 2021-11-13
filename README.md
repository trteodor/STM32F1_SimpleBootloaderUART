## Hello,
This project/ example was made for educational purposes. 
I wanted to learn how to write boot loaders for my applications. 
After completing this project / example, I know how to write simple bootloader!

In this example, there are two applications (boot loader, and user applications). 
After uploading the boot loader to the MCU, using a dedicated 
Python script contained in this repository, you can upload user applications to the MCU. 
The application is launched after pressing the button on the NUCLEO board or after uploading the program.
 
This is just an example so it is not a functional solution.

To Run this example i used

* STM32F103RB
* NUCLEO-F103RB Board

![pic](https://github.com/trteodor/STM32F1_SimpleBootloaderUART/blob/master/pic/NucleoBoardPicture.PNG)

To Compile and Create program for MCU i used CubeMX, and STM32CubeIDE

Boot loader was tested only on:
* Windows 10

# Annotations:

This repository is based on this article:

 * https://steelph0enix.github.io/stm32materials/articles/bootloader/

It is actually a copy of it, I have just modified only the 
necessary parts of the programs to compile for the 
STM32F103RB Nuclego board using the HAL library.

I also planned to prepare a GUI for the python boot loader, 
but due to the simplicity of this task I gave up. 
I leave in the repository only the skeleton of the graphic terminal:

In Path:
* PythonProgrammingScript/GuiDevProcess/GuiBootLoaderDevHere.py

Based on:
* https://programming.vip/docs/python-serial-communication-applet-gui-interface.html

It is a good framework for developing GUIbootloader.
