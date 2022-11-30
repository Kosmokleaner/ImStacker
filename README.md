# ImStacker V0.5

ImStacker is a simple WIP node based graph editor without “wires” using the ImGui library. It runs on Windows and in the Browser. A demo is included. MIT license

<img alt="image" src="https://user-images.githubusercontent.com/44132/204394141-12e334c1-ea32-47f0-b16b-66021a327fba.png">
The radial gradient was created interactively in less than a minute.


Live Web Demo:	http://www.kosmokleaner.de/temp/stacker4 


# What is ImStacker ?

ImStacker is C++ code that implements a *simple graph editor* but while most such editors use lines to connect boxes we make use of the 2d location and use the stacking (hence the name) location to connect the boxes. This results in a more compact and visually less complex result (no crossed wires). The stacking concept was borrowed from a tool named Werkzeug developed by the demo coder group Farbrausch.

TODO TODO
Node graphs using lines can get messy when getting more complex.

*ImStacker* is implemented on top of Dear ImGui which is a very popular UI library for desktop game development. The node data and connectivity is stored internally (stateful), but the UI part is immediate (stateless) making it work well with existing ImGui code. This is where the “Im” part of the name comes from.

TODO
from https://github.com/ocornut/imgui

ImStacker is meant to be usable for a wide range of applications (e.g. shader graph, 2D scripting languages, calculator, post processing graph, …) so it requires integration with code that defines node properties, the connectivity options and how the output is generated. We implemented a demo application for a simple GLSL shader graph editor.

The *ImStacker demo* uses OpenGL and GLFW to be mostly platform independent. The 64Bit Windows OpenGL version can be compiled with Visual Studio. The 32Bit Webassembly version runs in most browsers and can be compiled with Emscripten (C++ to WebAssembly).

TODO

The code was implemented to demonstrate and refine the idea and is work in progress. It has not reached the maturity of a shipping product. Feel free to take the idea to the next level. ImStacker needs a strong driving application to mature and multiple diverse applications to keep developing a strong common core.

All code so far was written by Martin Mittring and released under the MIT license. That should make integration in other projects easy.


# Examples

You can get the best impression by seeing it live, try the following and experiment with it:


TODO
Hello World

TODO
A more interesting / animated example



# Goals of ImStacker
* Simple implementation over performance and production quality
  => For fast code iteration, no localization or polished visuals
* Simple to integrate
  => abstracted node implementation, C++
* Live web GL demo
  => To get wider reach within coder community and demo to anyone in minutes

# ImStacker design choices








