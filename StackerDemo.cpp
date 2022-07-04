#include "CppStackerBox.h"

static StackerUI g_stackerUI;

void init() {

  {
    StackerBoxRect rect;
    rect.x = (int32)(rand() % 10);
    rect.y = (int32)(rand() % 10);
    rect.width = (int32)((rand() % 10) + 2);
    rect.height = (int32)((rand() % 3) + 1);
    CppStackerBox* el = new CppStackerBox;
    el->validate();
    el->rect = rect;
    el->nodeType = NT_RGBOutput;
    g_stackerUI.stackerBoxes.push_back(el);
  }
  /*
    {
      for (int32 i = 0; i < 1; ++i) {
        StackerBoxRect rect;
        rect.x = (int32)(rand() % 10);
        rect.y = (int32)(rand() % 10);
        rect.width = (int32)((rand() % 10) + 2);
        rect.height = (int32)((rand() % 3) + 1);
        bool isVariable = i < 2;
        if (isVariable) {
          CppStackerBoxFloat* el = new CppStackerBoxFloat;
          el->rect = rect;
          g_stackerUI.stackerBoxes.push_back(el);
        }
        else {
          CppStackerBox* el = new CppStackerBox;
          el->validate();
          el->rect = rect;
          el->nodeType = NT_Add;
          el->name = "Node";
          g_stackerUI.stackerBoxes.push_back(el);
        }
      }
    }
  */

  static CppAppConnection cppAppConnection;
  g_stackerUI.setAppConnection(&cppAppConnection);
}

void stacker_demo() {
  static bool first = true;

  if (first) {
    init();
    first = false;
  }

  g_stackerUI.draw();


  static bool show_demo_window = false;

  if (ImGui::BeginMainMenuBar())
  {
    if (ImGui::BeginMenu("Windows"))
    {
      ImGui::MenuItem("Im Demo Window", 0, &show_demo_window);
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }

  //  ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);


    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
  if (show_demo_window)
    ImGui::ShowDemoWindow(&show_demo_window);

}