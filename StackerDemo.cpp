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

}