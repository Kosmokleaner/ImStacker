# ImStacker V0.5

ImStacker is a simple WIP node based graph editor without “wires” using the ImGui library. It runs on Windows and in the Browser. A demo is included. MIT license

<img alt="image" src="https://user-images.githubusercontent.com/44132/204394141-12e334c1-ea32-47f0-b16b-66021a327fba.png">
The radial gradient was created interactively in less than a minute.


Live Web Demo:	http://www.kosmokleaner.de/temp/stacker4 


## What is ImStacker ?

ImStacker is C++ code that implements a *simple graph editor* but while most such editors use lines to connect boxes we make use of the 2d location and use the stacking (hence the name) location to connect the boxes. This results in a more compact and visually less complex result (no crossed wires). The stacking concept was borrowed from a tool named Werkzeug developed by the demo coder group Farbrausch.

<img src="(https://user-images.githubusercontent.com/44132/204688890-95950d81-36e0-4aeb-a1ee-ab161d92d0ff.png)" alt="drawing" width="50%"/>
<img src="[(https://user-images.githubusercontent.com/44132/204688890-95950d81-36e0-4aeb-a1ee-ab161d92d0ff.png)](https://user-images.githubusercontent.com/44132/204688896-026b6150-0353-4704-87e3-33b9a14b8564.png)" alt="drawing" width="50%"/>
Node graphs using lines can get messy when getting more complex.

*ImStacker* is implemented on top of Dear ImGui which is a very popular UI library for desktop game development. The node data and connectivity is stored internally (stateful), but the UI part is immediate (stateless) making it work well with existing ImGui code. This is where the “Im” part of the name comes from.

![image8](https://user-images.githubusercontent.com/44132/204688968-f34a4af1-9126-452c-892e-01ca050b7ef6.png)
from https://github.com/ocornut/imgui

ImStacker is meant to be usable for a wide range of applications (e.g. shader graph, 2D scripting languages, calculator, post processing graph, …) so it requires integration with code that defines node properties, the connectivity options and how the output is generated. We implemented a demo application for a simple GLSL shader graph editor.

The *ImStacker demo* uses OpenGL and GLFW to be mostly platform independent. The 64Bit Windows OpenGL version can be compiled with Visual Studio. The 32Bit Webassembly version runs in most browsers and can be compiled with Emscripten (C++ to WebAssembly).

![image5](https://user-images.githubusercontent.com/44132/204689029-bb97ced7-5598-40f2-a430-48e4830e9b4b.png)

The code was implemented to demonstrate and refine the idea and is work in progress. It has not reached the maturity of a shipping product. Feel free to take the idea to the next level. ImStacker needs a strong driving application to mature and multiple diverse applications to keep developing a strong common core.

All code so far was written by Martin Mittring and released under the MIT license. That should make integration in other projects easy.


## Examples

You can get the best impression by seeing it live, try the following and experiment with it:


<img width="359" alt="image11" src="https://user-images.githubusercontent.com/44132/204689068-47e523d2-d99f-46cb-8ca6-ff3b08c6a3a7.png">
Hello World

<img width="808" alt="image2" src="https://user-images.githubusercontent.com/44132/204689048-7fb1d0dd-0fdf-4920-ae12-2a38f0f8ebc1.png">
A more interesting / animated example



## Goals of ImStacker
* Simple implementation over performance and production quality\
  => For fast code iteration, no localization or polished visuals
* Simple to integrate\
  => abstracted node implementation, C++
* Live web GL demo\
  => To get wider reach within coder community and demo to anyone in minutes

## ImStacker design choices
* **Stacker Panel window** (to create and place nodes)
  * Interactive for fast iteration, compile should be in background thread, not blocking UI
  * 2D visual grid and snapping to reduce urge to polish and simpler snapping
  * Nodes have rounded corners to indicate in / out, arrows to indicate flow (downwards)\
    ![image7](https://user-images.githubusercontent.com/44132/204689158-67e27512-579b-4da9-bd16-3f68df71261a.png)
  * Data flow type between is nodes is cast if possible. This needs more polish / testing.
  * Resize handles
  * Visuals: left and right handles
  * Function: 4 corners to resize element + center to move
  * Right mouse button / Context menu (Cut Copy Paste), Create new node\
    ![image6](https://user-images.githubusercontent.com/44132/204689171-cb5bef59-7fae-4109-a484-6f4a22398a57.png)
  * Multiple node selection (left mouse drag for box, shift for add / remove individuals)
  * Selection to copy / move / edit / delete multiple nodes
  * Red cross to show localized errors
* **Properties window** (to editor node properties)\
  ![image1](https://user-images.githubusercontent.com/44132/204689232-ede902d6-e9bc-4c2c-bd4b-fe08254c3f33.png)
  * Node name as optional documentation (like variable name)
  * Data type
* **Stacker Code window** (mostly needed during development)\
  ![image4](https://user-images.githubusercontent.com/44132/204689251-e406c4e6-962b-4fac-81ad-fd41165ecfec.png)
  * UI for new / load / save
  * Show generated shader code for inspection
  * The generated code is editable to quickly iterate on shader errors without restart.

# Future
* Fix bugs e.g.
  * Overlapping nodes are not selected correctly
  * Some data types are not propagated correctly
* Polish UI
  * Web preview background is not filling the full window
  * StackerCode window resize has no effect on window elements
  * Show data type between nodes
  * Undo / Redo (can use existing serialization)
* Make production usable
  * Key bindings to creates common nodes faster
  * We need variables to reuse data easier
  * We need functions to hide complexity with a simple interface
  * Automated unit and integration tests
  * Need driving application[s]
  * .. for more see todo.txt

## Classes
* **ImStacker:**
  * **class StackerUI**\
    To store the state and connect to the application
  * **class StackerBox**\
    Base class for the node
  * **struct IAppConnection**\
    to connect to the application e.g. compile graph / generate output 
* **ImStacker demo:**
  * **class CppStackerBox** : public StackerBox\
    Node class implementing most functions
  * **class CppStackerBoxConstant** : public CppStackerBox\
    Node class implementing a int / float / float2 / float3 / float4 constant
  * **class CppStackerBoxSwizzle** : public CppStackerBox\
    Node class implementing a swizzle operation (glsl / hlsl operation)
  * **class CppAppConnection** : public StackerUI::IAppConnection\
    to implement compiling glsl code and rendering the output as background

# Files
* **todo.txt**\
  Itemized tasks for future updates
* **winMain.cpp, ImStacker.vcproj**\
  for running on Windows
* **webMain.cpp, makefile**\
  for running in the browser with Emscripten
* **Readme.me**\
  Markup documentation (single image for now, will be generated from this document)

## ImStacker Version History:
* V0.1
  * Cloned Windows ImGui demo project
  * ImStacker state internals, basic UI
  * Added math demo (Basic function)
* V0.3
  * Serialization (Load / Save / Copy & Paste)
  * Added OpenGL shader demo (ShaderToy inspired)
* V0.5
  * Web browser through Emscripten
  * More Shader nodes, polished visuals

## Third Party / External Libraries
* **Dear ImGui** User Interface library, MIT license
* **GLFW** OpenGL OS abstraction, zlib/libpng license
* **RapidJson** Serialization (Load / Save / Copy & Paste), MIT license


## Links
* **Art and technology of the demoscene**\
  https://www.youtube.com/watch?v=GswISjlquoU \
  How to work with “Tooll”, a spiritual successor of Werkzeug












