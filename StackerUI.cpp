#include <algorithm> // std::find(), std::min()
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"
#include "StackerUI.h"
#include "ShaderBox.h"
#include <imgui_stdlib.h>
#include <assert.h>
#include <math.h> // floorf
#include <sys/stat.h>

#ifdef _WIN32
// warning C4996: 'fopen': This function or variable may be unsafe. Consider using fopen_s instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details.
#pragma warning( disable : 4996)
#endif

using namespace rapidjson;

StackerBox::StackerBox() {
}

StackerBox::StackerBox(const StackerBoxRect& inRect) : rect(inRect) {
}

StackerBox::~StackerBox() {
}

bool StackerBox::load(const rapidjson::Document::ValueType& doc) {
  {
    if (!doc.HasMember("rect") || !doc["rect"].IsArray())
      return false;

    const Document::ConstArray& arr = doc["rect"].GetArray();

    if (arr.Size() != 4)
      return false;

    // todo: type check
    rect.x = arr[0].GetInt();
    rect.y = arr[1].GetInt();
    rect.width = arr[2].GetInt();
    rect.height = arr[3].GetInt();
  }
  return true;
}

void StackerBox::save(Document& d, rapidjson::Value& objValue) const {
  {
    rapidjson::Value localArray(rapidjson::kArrayType);
    localArray.Reserve(4, d.GetAllocator());
    localArray.PushBack(rect.x, d.GetAllocator());
    localArray.PushBack(rect.y, d.GetAllocator());
    localArray.PushBack(rect.width, d.GetAllocator());
    localArray.PushBack(rect.height, d.GetAllocator());
    objValue.AddMember("rect", localArray, d.GetAllocator());
  }
}

// ----------------------------------------------------------------------------

void StackerUI::freeData() {
  for (auto it : stackerBoxes)
    delete it;
  stackerBoxes.clear();
  reset();
}

void StackerUI::reset() {
  selectedObjects.clear();
  resizeObjectId = -1;
  resizeHandleMask = 0;
  selectStartX = -1;
  selectStartY = -1;
  order.clear();
}

bool StackerUI::loadFromBuffer(const std::vector<char>& fileBlob) {
  reset();

  Document d;
  d.Parse(fileBlob.data(), fileBlob.size());

  if (!d.IsObject() || !d.HasMember("magic") || !d["magic"].IsString())
    return false;

  if (strcmp(d["magic"].GetString(), "Stacker"))
    return false;

  if (!d.HasMember("version") || !d["version"].IsInt())
    return false;

  if (d["version"].GetInt() != 1)
    return false;

  if (!d["stackerBoxes"].IsArray())
    return false;

  const Document::Array& arr = d["stackerBoxes"].GetArray();

  bool ok = false;

  for (SizeType i = 0;; i++) {
    if (i == arr.Size()) {
      ok = true;
      break;
    }

    const Document::ValueType& ref = arr[i].GetObject();
    if (ref.HasMember("type") && ref["type"].IsString()) {
      const char* typeName = ref["type"].GetString();

      if (StackerBox* node = appConnection->createNode(typeName)) {
        stackerBoxes.push_back(node);
        if (!node->load(ref)) {
          break;
        }
      }
      else break;
    }
    else break;
  }

  if (!ok)
    freeData();

  return ok;
}

void StackerUI::saveToBuffer(rapidjson::StringBuffer& strbuf, const std::vector<int32>& what) {
  Document d;
  d.SetObject();

  d.AddMember("magic", "Stacker", d.GetAllocator());
  d.AddMember("version", 1, d.GetAllocator());

  rapidjson::Value myArray(rapidjson::kArrayType);

  for (auto id : what) {
    const auto& ref = *stackerBoxes[id];
    rapidjson::Value objValue;
    objValue.SetObject();

    rapidjson::Value tmp;
    tmp.SetString(ref.getType(), d.GetAllocator());
    objValue.AddMember("type", tmp, d.GetAllocator());

    ref.save(d, objValue);

    myArray.PushBack(objValue, d.GetAllocator());
  }
  // https://stackoverflow.com/questions/14483672/rapidjson-adding-an-array-of-structs/38562399
  d.AddMember("stackerBoxes", myArray, d.GetAllocator());

  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(strbuf);
  //Writer<rapidjson::StringBuffer> writer(strbuf);
  d.Accept(writer);
}

StackerUI::~StackerUI() {
  freeData();
}

void StackerUI::cutCopyPasteMenu() {
  if (ImGui::MenuItem("Cut", "CTRL+X", nullptr, !selectedObjects.empty())) {
    clipboardCut();
  }
  if (ImGui::MenuItem("Copy", "CTRL+C", nullptr, !selectedObjects.empty())) {
    clipboardCopy();
  }
  const char* txt = ImGui::GetClipboardText();
  if (ImGui::MenuItem("Paste", "CTRL+V", nullptr, txt && *txt)) {
    clipboardPaste();
  }
}

void StackerUI::contextMenu(const int32 mousePosX, const int32 mousePosY) {
  ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
  if (ImGui::BeginPopup("context menu")) {
    if (!contextMenuIsOpen) {
      // what object is under the mouse cursor?
      int32 rightClickOnId = -1;
      for (uint32 id = 0, count = (uint32)stackerBoxes.size(); id < count; ++id) {
        auto& ref = *stackerBoxes[id];

        if (ref.rect.x <= mousePosX
          && ref.rect.y <= mousePosY
          && ref.rect.x + ref.rect.width > mousePosX
          && ref.rect.y + ref.rect.height > mousePosY) {
          rightClickOnId = id;
        }
      }
      if (rightClickOnId == -1) {
        clearSelection();
      }
      else {
        // if the object was not selected, we select it
        if (!isSelected(rightClickOnId))
          selectObject(rightClickOnId, false);
      }
    }

    if (!anySelection()) {               // New elements
      if (!contextMenuIsOpen) {
        // show selection rectangle where mouse context menu was opened
        selectStartX = mousePosX - newHalfSize;
        selectStartY = mousePosY;
      }

      StackerBoxRect stackerBoxRect = { selectStartX, selectStartY, newHalfSize * 2 + 1, 1 };

      appConnection->openContextMenu(*this, stackerBoxRect);
    }
    else {
      cutCopyPasteMenu();
    }
    contextMenuIsOpen = true;
    ImGui::EndPopup();
  }
  else {
    contextMenuIsOpen = false;
  }
  ImGui::PopStyleColor();
}

void StackerUI::addFromUI(StackerBox& in) {
  const int32 id = (int32)stackerBoxes.size();
  stackerBoxes.push_back(&in);
  clearSelection();
  addToSelection(id);
}

void StackerUI::setAppConnection(IAppConnection* inAppConnection) {
  appConnection = inAppConnection ? inAppConnection : &nullAppConnection;
}

void StackerUI::draw() {
  ImGuiIO& io = ImGui::GetIO();

  ImGui::SetNextWindowSize(ImVec2(700, 600), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowBgAlpha(1.0f);

  // todo: make UI not in a window so it can be combined with other UI
  ImGui::Begin("Stacker", 0, ImGuiWindowFlags_NoCollapse);

  ImGui::BeginChild("scrolling_region", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);

  // Scrolling
  if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive()) {
    if(ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0.0f)) {
      scrollingActive = true;
      scrollingX = scrollingX + (int32)io.MouseDelta.x;
      scrollingY = scrollingY + (int32)io.MouseDelta.y;

      scrollingAmount += abs(io.MouseDelta.x) + abs(io.MouseDelta.y);
    }
    else {
      // open context menu even if there was a small accidential RMB drag
      if (scrollingAmount < 10.0f && scrollingActive)
        ImGui::OpenPopup("context menu", 1);
      scrollingActive = false;
      scrollingAmount = 0.0f;
    }
  }

  const ImVec2 offset = ImVec2(ImGui::GetCursorScreenPos().x + scrollingX, ImGui::GetCursorScreenPos().y + scrollingY);

  // mouse screen pos, not window local
  ImVec2 mousePos = ImGui::GetMousePos();
  // in grid cells
  int32 mousePosX = (int32)floorf((mousePos.x - offset.x) / scale);
  int32 mousePosY = (int32)floorf((mousePos.y - offset.y) / scale);

  contextMenu(mousePosX, mousePosY);

  if (ImGui::IsKeyPressed(ImGuiKey_Delete, true)) {
    clipboardCut();
  }

  buildOrder();

  if (contextMenuIsOpen) {
    // show selection rectangle where mouse context menu was opened
    mousePosX = selectStartX + newHalfSize * 2;
    mousePosY = selectStartY;
  }

  ImDrawList* draw_list = ImGui::GetWindowDrawList();

  // Display grid
  {
    ImU32 GRID_COLOR = IM_COL32(200, 200, 200, 20);
    float GRID_SZ = scale + 0.0f;
    ImVec2 win_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_sz = ImGui::GetWindowSize();
    for (float x = fmodf((float)scrollingX, GRID_SZ); x < canvas_sz.x; x += GRID_SZ)
      draw_list->AddLine(ImVec2(win_pos.x + x, win_pos.y), ImVec2(win_pos.x + x, win_pos.y + canvas_sz.y), GRID_COLOR);
    for (float y = fmodf((float)scrollingY, GRID_SZ); y < canvas_sz.y; y += GRID_SZ)
      draw_list->AddLine(ImVec2(win_pos.x, win_pos.y + y), ImVec2(win_pos.x + canvas_sz.x, win_pos.y + y), GRID_COLOR);
  }

  bool oldDragActive = dragActive;
  dragActive = ImGui::IsWindowHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left);

  if (!dragActive) {
    resizeHandleMask = 0;
    resizeObjectId = -1;
  }

  // -1:not used
  int32 hoverObjectId = resizeObjectId;
  // bit0:top, bit1:right, bit2:bottom, bit3:left
  uint32 hoverHandleMask = resizeHandleMask;

  // pass 1: find top most hover object and handle
  if (!oldDragActive) {

    for (uint32 index = 0, count = (uint32)stackerBoxes.size(); index < count; ++index) {
      auto& it = *stackerBoxes[index];
      ImVec2 minR((float)it.rect.x * scale + offset.x, (float)it.rect.y * scale + offset.y);
      ImVec2 sizeR((float)it.rect.width * scale, (float)it.rect.height * scale);
      ImVec2 maxR(minR.x + sizeR.x, minR.y + sizeR.y);

      // [] 0:top, 1:right, 2:bottom, 3:left
      float side[4] = {
          mousePos.y - minR.y,
          maxR.x - mousePos.x,
          maxR.y - mousePos.y,
          mousePos.x - minR.x
      };

      // inside box ?
      if (side[0] < 0 || side[1] < 0 || side[2] < 0 || side[3] < 0)
        continue;

      hoverObjectId = index;
      hoverHandleMask = 0;

      for (int sideId = 0; sideId < 4; ++sideId) {
        uint32 handleSize = (sideId % 2) ? handleSizeX : handleSizeY;
        if (side[sideId] <= handleSize)
          hoverHandleMask |= 1 << sideId;
      }
      if (hoverHandleMask & 2) {
        // if left and right handle are active we only want to scale right
        hoverHandleMask &= ~8;
      }
      if (hoverHandleMask & 4) {
        // if top and bottom handle are active we only want to scale bottom
        hoverHandleMask &= ~1;
      }


      // Top and bottom handle are disabled to make scale handles less often used by accident
      // and hightlight less noisy.
      if (hoverHandleMask == 0x1 || hoverHandleMask == 0x4) {
        hoverHandleMask = 0;
      }
    }

    if (!oldDragActive && dragActive) {
      resizeObjectId = hoverObjectId;
      resizeHandleMask = hoverHandleMask;

      if (hoverObjectId == -1) {
        selectStartX = mousePosX;
        selectStartY = mousePosY;
      }

      selectObject(hoverObjectId, io.KeyMods & ImGuiKeyModFlags_Shift);
    }
  }

  // left, top, right, bottom, left <= right, top <= bottom
  int32 selectRect[4] = {
      std::min(selectStartX, mousePosX),
      std::min(selectStartY, mousePosY),
      std::max(selectStartX, mousePosX),
      std::max(selectStartY, mousePosY)
  };

  if (dragActive && resizeObjectId == -1) {
    // selection rectangle
    clearSelection();

    for (uint32 id = 0, count = (uint32)stackerBoxes.size(); id < count; ++id) {
      auto& ref = *stackerBoxes[id];

      if (ref.rect.x + ref.rect.width > selectRect[0]
        && ref.rect.y + ref.rect.height > selectRect[1]
        && ref.rect.x <= selectRect[2]
        && ref.rect.y <= selectRect[3]) {
        addToSelection(id);
      }
    }
  }

  // could be optimzied to not be each draw/frame, before draw to draw with the right visuals
  buildGraph();
  generateCode(false);
  drawBoxes(offset, hoverObjectId, hoverHandleMask);
  drawConnectors(offset);

  bool showSelectionRect = false;
  {
    if (contextMenuIsOpen && resizeObjectId == -1 && !anySelection())
      showSelectionRect = true;
    if (dragActive && resizeObjectId == -1)
      showSelectionRect = true;
  }

  if (showSelectionRect) {
    // selection rectangle
    draw_list->AddRect(
      ImVec2(selectRect[0] * scale * 1.0f + offset.x, selectRect[1] * scale * 1.0f + offset.y),
      ImVec2(selectRect[2] * scale + scale + offset.x, selectRect[3] * scale + scale + offset.y),
      0x44888888, 0.0f, ImDrawCornerFlags_None, 2.0f);
  }

  if (dragActive && resizeObjectId != -1) {
    mouseDelta.x += io.MouseDelta.x;
    mouseDelta.y += io.MouseDelta.y;

    int32 deltaX = (int32)floorf(mouseDelta.x / scale + 0.5f);
    int32 deltaY = (int32)floorf(mouseDelta.y / scale + 0.5f);

    if (resizeHandleMask == 0) {
      for (auto it = selectedObjects.begin(); it != selectedObjects.end(); ++it) {
        auto& ref = *stackerBoxes[*it];
        ref.rect.x += deltaX;
        ref.rect.y += deltaY;
      }
      dirty = true;
      mouseDelta.x -= deltaX * scale;
      mouseDelta.y -= deltaY * scale;
    }
    else {
      auto& ref = *stackerBoxes[resizeObjectId];

      if ((resizeHandleMask & 1) && ref.rect.height > deltaY) {
        ref.rect.y += deltaY;
        ref.rect.height -= deltaY;
        mouseDelta.y -= deltaY * scale;
        dirty = true;
      }
      if ((resizeHandleMask & 2) && ref.rect.width > -deltaX) {
        ref.rect.width += deltaX;
        mouseDelta.x -= deltaX * scale;
        dirty = true;
      }
      if ((resizeHandleMask & 4) && ref.rect.height > -deltaY) {
        ref.rect.height += deltaY;
        mouseDelta.y -= deltaY * scale;
        dirty = true;
      }
      if ((resizeHandleMask & 8) && ref.rect.width > deltaX) {
        ref.rect.x += deltaX;
        ref.rect.width -= deltaX;
        mouseDelta.x -= deltaX * scale;
        dirty = true;
      }
    }
  }
  else {
    mouseDelta = {};
  }

  propertiesUI();
  generatedCodeUI();

  // debug
//    ImGui::Text("sel:%d hnd:%d mouse:(%.1f %.1f)", selectedObject, selectedHandle, mousePos.x, mousePos.y);
//    ImGui::Text("(%d %d)", mousePosX, mousePosY);

  ImGui::EndChild();

  ImGui::End();
}


void StackerUI::clearSelection() {
  selectedObjects.clear();
}

void StackerUI::clipboardCut() {
  clipboardCopy();
  deleteSelection();
  dirty = true;
}

void StackerUI::clipboardCopy() {
  StringBuffer strbuf;
  saveToBuffer(strbuf, selectedObjects);
  strbuf.Put(0);
  ImGui::SetClipboardText(strbuf.GetString());
}

void StackerUI::clipboardPaste() {
  const char* fileData = ImGui::GetClipboardText();
  if (!fileData)
    return;
  std::vector<char> temp;
  temp.resize(strlen(fileData) + 1);
  memcpy(temp.data(), fileData, temp.size());
  temp[temp.size() - 1] = 0;
  // no error handling, a lot of clipboard content will not work
  loadFromBuffer(temp);
  dirty = true;
}

void StackerUI::deleteSelection() {
  std::sort(selectedObjects.begin(), selectedObjects.end());
  for (auto it = selectedObjects.rbegin(); it != selectedObjects.rend(); ++it) {
    int32 id = *it;
    delete stackerBoxes[id];
    stackerBoxes.erase(stackerBoxes.begin() + *it);
  }
  selectedObjects.clear();
  dirty = true;
}


void StackerUI::addToSelection(const int32 index) {
  assert((uint32)index < (uint32)stackerBoxes.size());
  selectedObjects.push_back(index);
}


bool StackerUI::anySelection() {
  return !selectedObjects.empty();
}


bool StackerUI::isSelected(const int32 id) const {
  auto itFind = std::find(selectedObjects.begin(), selectedObjects.end(), id);
  return itFind != selectedObjects.end();
}


void StackerUI::selectObject(const int32 id, const bool shift) {
  //    const bool isThisSelected = isSelected(id);

  auto itFind = std::find(selectedObjects.begin(), selectedObjects.end(), id);

  if (shift) {
    if (itFind != selectedObjects.end())
      selectedObjects.erase(itFind);
    else
      selectedObjects.push_back(id);
  }
  else {
    if (itFind == selectedObjects.end()) {
      clearSelection();
      if (id >= 0)
        addToSelection(id);
    }
  }
}

void drawHatchedRect(const ImVec2 minR, const ImVec2 maxR, ImU32 col) {
  ImDrawList* draw_list = ImGui::GetWindowDrawList();

  const float step = 4;

  for(float y = minR.y + 1.0f; y < maxR.y; y += step) {
    draw_list->AddLine(ImVec2(minR.x, y), ImVec2(maxR.x, y), col, 2.0f);
  }
}

void StackerUI::drawBoxes(const ImVec2 offset, int32 hoverObjectId, const uint32 hoverHandle) {
  ImDrawList* draw_list = ImGui::GetWindowDrawList();

  for (const int32 id : order) {
    auto& ref = *stackerBoxes[id];

    ref.validate();

    ImVec2 minR((float)ref.rect.x * scale + offset.x, (float)ref.rect.y * scale + offset.y);
    ImVec2 sizeR((float)ref.rect.width * scale, (float)ref.rect.height * scale);
    ImVec2 maxR(minR.x + sizeR.x, minR.y + sizeR.y);
    ImDrawCornerFlags cornerFlags = ImDrawCornerFlags_All;

    const bool isVariable = ref.isVariable();

    // visual preference, could be made an option
//        float cornerRadius = ref.isVariable ? scale : 0.0f;
    float cornerRadius = isVariable ? (scale * 0.5f) : 0.0f;

    if (ref.isUsedAsInput)
      cornerFlags &= ImDrawCornerFlags_Bot;
    if (ref.treeParentId != -1)
      cornerFlags &= ImDrawCornerFlags_Top;

    ImGui::PushID(id);

    //        ImU32 node_bg_color = (node_hovered_in_list == node->ID || node_hovered_in_scene == node->ID || (node_hovered_in_list == -1 && node_selected == node->ID)) ? IM_COL32(75, 75, 75, 255) : IM_COL32(60, 60, 60, 255);
    ImU32 node_bg_color = isVariable ? 0xff557766 : 0xff556677;

    draw_list->AddRectFilled(minR, maxR, node_bg_color, cornerRadius, cornerFlags);
    draw_list->AddRect(minR, maxR, 0x44000000, cornerRadius, cornerFlags, 6.0f);

    // can be optimized
    if (isSelected(id)) {
      // selection
      draw_list->AddRect(minR, maxR, 0xdd44ddff, cornerRadius, cornerFlags, 2.0f);
    }

    if (id == hoverObjectId) {
      ImVec2 hoverMinR = minR;
      ImVec2 hoverMaxR = maxR;

      if (hoverHandle & 1)     // top
        hoverMaxR.y = minR.y + handleSizeY;
      if (hoverHandle & 2)    // right
        hoverMinR.x = maxR.x - handleSizeX;
      if (hoverHandle & 4)    // bottom
        hoverMinR.y = maxR.y - handleSizeY;
      if (hoverHandle & 8)    // left
        hoverMaxR.x = minR.x + handleSizeX;

      draw_list->AddRectFilled(hoverMinR, hoverMaxR, 0x44ffffff, 4.0f);
    }

    {
      // cliprect is to confine text into the box
      ImGui::PushClipRect(ImVec2(minR.x + clipBorderSize, minR.y), ImVec2(maxR.x - clipBorderSize, maxR.y), true);

      ref.drawBox(*this, minR, maxR);

      ImGui::PopClipRect();

      drawHatchedRect(ImVec2(minR.x + clipBorderSize, minR.y + clipBorderSize), ImVec2(minR.x + handleSizeX - clipBorderSize / 2, maxR.y - clipBorderSize), 0x33000000);
      if(maxR.x - minR.x > handleSizeX) {
        drawHatchedRect(ImVec2(maxR.x - handleSizeX + clipBorderSize / 2, minR.y + clipBorderSize), ImVec2(maxR.x - clipBorderSize, maxR.y - clipBorderSize), 0x33000000);
      }
    }

    if (ref.compileError) {
      ImVec2 mid((minR.x + maxR.x) * 0.5f, (minR.y + maxR.y) * 0.5f);
      // error connection: cross

      // left top
      ImVec2 a(mid.x - crossSize / 2, mid.y - crossSize / 2);
      // bottom right
      ImVec2 b(mid.x + crossSize / 2, mid.y + crossSize / 2);

      draw_list->AddLine(a, b, 0xff0000ff, 2.0f);
      draw_list->AddLine(ImVec2(a.x, b.y), ImVec2(b.x, a.y), 0xff0000ff, 2.0f);
    }

    ImGui::PopID();

    ref.validate();
  }
}


void StackerUI::drawConnectors(const ImVec2 offset) {
  ImDrawList* draw_list = ImGui::GetWindowDrawList();

  for (const int32 id : order) {
    auto& ref = *stackerBoxes[id];

    if (ref.treeParentId == -1)
      continue;

    float y = ref.rect.y + ref.rect.height + 0.0f;

    if (ref.treeParentId == -2) {
      // error connection: cross
      float x = ref.rect.x + ref.rect.width * 0.5f;

      // left top
      ImVec2 a(x * scale + offset.x - crossSize / 2, y * scale + offset.y - crossSize / 2);
      // bottom right
      ImVec2 b(x * scale + offset.x + crossSize / 2, y * scale + offset.y + crossSize / 2);

      draw_list->AddLine(a, b, 0xff0000ff, 2.0f);
      draw_list->AddLine(ImVec2(a.x, b.y), ImVec2(b.x, a.y), 0xff0000ff, 2.0f);
    }
    else {
      // good connection: triangle
      auto& ref2 = *stackerBoxes[ref.treeParentId];

      float x = (std::max(ref.rect.x, ref2.rect.x) + std::min(ref.rect.x + ref.rect.width, ref2.rect.x + ref2.rect.width)) * 0.5f;

      ImVec2 a(x * scale + offset.x - connectorSize / 2, y * scale + offset.y);
      ImVec2 b(x * scale + offset.x + connectorSize / 2, y * scale + offset.y);
      ImVec2 c(x * scale + offset.x, y * scale + offset.y + connectorSize / 2);

      draw_list->AddTriangleFilled(a, b, c, 0xffffffff);
    }
  }
}


void StackerUI::findChildren(const int32 Id, std::vector<StackerBox*>& outChildren) {
  outChildren.clear();

  auto& ref = *stackerBoxes[Id];

  // top line cannot have children
  if (ref.rect.y == bitmapBounds[1])
    return;

  const int32 lineAbove = ref.rect.y - 1;
  const uint32 width = getBitmapWidth();
  //    const uint32 height = getBitmapHeight();
  const uint32 startIndex = ref.rect.x + lineAbove * width - (bitmapBounds[0] + bitmapBounds[1] * width);

  for (int32 i = 0, count = ref.rect.width; i < count; ++i) {
    const int32 thereId = tempBitmap[startIndex + i];
    if (thereId < 0)
      continue;

    auto& other = *stackerBoxes[thereId];

    if (other.treeParentId == Id) {
      // can be optimized
      if (std::find(outChildren.begin(), outChildren.end(), &other) == outChildren.end())
        outChildren.push_back(&other);
    }
  }
}

struct ProcessingSortOrder {

  bool operator()(int32 lhsId, int32 rhsId) {
    const StackerBox& lhs = *data[lhsId];
    const StackerBox& rhs = *data[rhsId];

#define CMP(a) \
    if (lhs.rect.a > rhs.rect.a) return true; \
    else if (lhs.rect.a < rhs.rect.a)  return false;

    CMP(y);
    CMP(x);
    CMP(height);
    CMP(width);
#undef CMP

    if (lhsId > rhsId)
      return true;
    else if (lhsId < rhsId)
      return false;

    return false;
  }

  StackerBox** data = nullptr;
};

void StackerUI::buildOrder() {
  order.resize(stackerBoxes.size());
  for (uint32 i = 0, count = (uint32)stackerBoxes.size(); i < count; ++i)
    order[i] = i;
  ProcessingSortOrder processingSortOrder;
  processingSortOrder.data = stackerBoxes.data();
  std::sort(order.begin(), order.end(), processingSortOrder);
}

void StackerUI::reCompile() {
  appConnection->reCompile();
}

void StackerUI::generateCode(const bool fullMode) {
  GenerateCodeContext context;
  context.params.reserve(1024);
  context.nextFreeVIndex = (int32)stackerBoxes.size();

  if (fullMode) {
    dirty = false;
    appConnection->startCompile();
    context.code = appConnection->code();
    assert(context.code);
  }

  for (auto it = order.rbegin(), end = order.rend(); it != end; ++it) {
    int32 id = *it;
    auto& ref = *stackerBoxes[id];

    ref.validate();

    findChildren(id, context.params);

    ref.validate();

    if (context.code) {
      *context.code += context.indentationStr;
    }

    ref.compileError = !ref.generateCode(context);

    ref.validate();
  }

  if (fullMode) {
    appConnection->endCompile();
    appConnection->reCompile();
  }
}

//https://stackoverflow.com/questions/5840148/how-can-i-get-a-files-size-in-c
static long long IO_GetFileSize(const char* Name) {
  assert(Name);
  struct stat stat_buf;
  int rc = stat(Name, &stat_buf);
  return rc == 0 ? stat_buf.st_size : 0;

}

bool StackerUI::load(const char* fileName) {
  std::vector<char> fileData;
  fileData.resize(IO_GetFileSize(fileName));
  if (FILE* out = fopen(fileName, "rb")) {
    bool ok = fread(fileData.data(), fileData.size(), 1, out) == 1;
    // todo: better error handling
    assert(ok);
    ok = loadFromBuffer(fileData);
    // todo: better error handling
    assert(ok);
    fclose(out);
  }
  else assert(false); // todo: better error handling
  return true;
}

void StackerUI::save(const char* fileName) {
  StringBuffer strbuf;
  std::vector<int32> all;
  all.resize(stackerBoxes.size());
  for (size_t i = 0, count = stackerBoxes.size(); i < count; ++i)
    all[i] = (int32)i;
  saveToBuffer(strbuf, all);
  if (FILE* out = fopen(fileName, "wb")) {
    bool ok = fwrite(strbuf.GetString(), strbuf.GetSize(), 1, out) == 1;
    // todo: better error handling
    assert(ok);
    fclose(out);
  }
  else assert(false); // todo: better error handling
}

void StackerUI::generatedCodeUI() {
  ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowBgAlpha(1.0f);

  ImGui::Begin("Stacker generated Code", 0, ImGuiWindowFlags_NoCollapse);

  if (ImGui::Button("  New  ")) {
    freeData();
  }

  // todo
  const char* fileName = "world.dat";

  ImGui::SameLine();
  if (ImGui::Button("  Load  ")) {
    freeData();
    load(fileName);
    dirty = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("  Save  ")) {
    save(fileName);
  }

  if (ImGui::Button("  Generate  ")) {
    generateCode(true);
  }
  ImGui::SameLine();
  ImGui::Checkbox("Auto", &autoGenerateCode);

  if (autoGenerateCode && dirty && !ImGui::IsAnyMouseDown()) {
    generateCode(true);
  }

  std::string* code = appConnection->code();
  assert(code);

  ImGui::InputTextMultiline("Code", code, ImVec2(-FLT_MIN, 300.0f));
  if (ImGui::Button("Compile from above")) {
    reCompile();
  }
  ImGui::TextUnformatted("Warnings / Errors");
  const char* warningsAndErrors = appConnection->getWarningsAndErrors();
  ImGui::InputTextMultiline("Warnings / Errors", (char*)warningsAndErrors, 0, ImVec2(-FLT_MIN, 0.0f), ImGuiInputTextFlags_ReadOnly);

  ImGui::End();
}


void StackerUI::propertiesUI() {
  ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowBgAlpha(1.0f);

  // todo: make UI not in a window so it can be combined with other UI
  ImGui::Begin("Stacker Properties", 0, ImGuiWindowFlags_NoCollapse);

  for (auto it = selectedObjects.begin(); it != selectedObjects.end(); ++it) {
    const uint32 index = *it;
    auto& ref = *stackerBoxes[index];

    ref.validate();

    if (it != selectedObjects.begin())
      ImGui::Separator();

    ImGui::PushID(index);
    if (ref.imGui()) {
      dirty = true;
    }
    ImGui::PopID();
  }
  ImGui::End();
}


void StackerUI::bitmapReset() {
  bitmapBounds[0] = INT_MAX;
  bitmapBounds[1] = INT_MAX;
  bitmapBounds[2] = -INT_MAX;
  bitmapBounds[3] = -INT_MAX;
}





void StackerUI::buildGraph() {
  tempBitmap.clear();

  bitmapReset();

  if (stackerBoxes.empty())
    return;

  // build bounds[]
  for (int32 index = 0, count = (int32)stackerBoxes.size(); index < count; ++index) {
    auto& ref = *stackerBoxes[index];

    bitmapBounds[0] = std::min(bitmapBounds[0], ref.rect.x);
    bitmapBounds[1] = std::min(bitmapBounds[1], ref.rect.y);
    bitmapBounds[2] = std::max(bitmapBounds[2], ref.rect.x + ref.rect.width);
    bitmapBounds[3] = std::max(bitmapBounds[3], ref.rect.y + ref.rect.height);
  }

  const uint32 width = getBitmapWidth();
  const uint32 height = getBitmapHeight();
  const uint32 leftTopOffset = bitmapBounds[0] + bitmapBounds[1] * width;

  tempBitmap.resize(width * height, -1);

  // Pass 1: build tempBitmap[]
  for (int32 id = 0, count = (int32)stackerBoxes.size(); id < count; ++id) {
    auto& ref = *stackerBoxes[id];

    // reset
    ref.treeParentId = -1;
    ref.isUsedAsInput = false;

    for (int32 y = ref.rect.y; y < ref.rect.y + ref.rect.height; ++y) {
      int32 value = id;
      for (int32 x = ref.rect.x; x < ref.rect.x + ref.rect.width; ++x) {
        tempBitmap[x + y * width - leftTopOffset] = value;
      }
    }
  }

  for (int32 id = 0, count = (int32)stackerBoxes.size(); id < count; ++id) {
    auto& ref = *stackerBoxes[id];

    ref.vIndex = id;

    // bottom row cannot have a parent
    if (ref.rect.y + ref.rect.height < bitmapBounds[3]) {
      // -1:not set, -2:error
      int32 parentId = -1;
      for (int32 x = ref.rect.x; x < ref.rect.x + ref.rect.width; ++x) {
        // can be optimized
        const int32 thereId = tempBitmap[x + (ref.rect.y + ref.rect.height) * width - leftTopOffset];
        if (thereId < 0)
          continue;

        const auto& there = *stackerBoxes[thereId];

        if (!there.canHaveInput())
          continue;

        if (parentId == -1) {
          parentId = thereId;
          stackerBoxes[thereId]->isUsedAsInput = true;
        }
        else if (parentId != thereId) {
          parentId = -2;
        }
      }
      ref.treeParentId = parentId;
    }
  }
}
