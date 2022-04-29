#pragma once
#include <vector>
#include <string>
#include "imgui.h"
#include "rapidjson/document.h" // for load and save
#include "rapidjson/writer.h"

typedef unsigned int uint32;
typedef signed int int32;

class StackerBox;
class StackerUI;

struct StackerBoxRect {
  int32 x = 0;
  int32 y = 0;
  int32 width = 0;
  int32 height = 0;
};

struct GenerateCodeContext {
  const char* indentationStr = "  ";
  // 0 if not in fullMode
  std::string* code = nullptr;
  //
  int32 nextFreeVIndex = 0;
  //
  std::vector<StackerBox*> params;
};

// base class for all elements in StackerUI
class StackerBox {
public:
    //
    StackerBoxRect rect;

    // transient ------

    // -1 if none, otherwise index in StackerUI::stackerBoxes[], set by buildGraph()
    int32 treeParentId = -1;
    // to generate variable names in generateCode()
    int32 vIndex = 0;
    // set by buildGraph()
    bool isUsedAsInput = false;
    bool compileError = false;

    // used by createNode()
    StackerBox();

    // constructor
    StackerBox(const StackerBoxRect& inRect);

    // destructor
    virtual ~StackerBox();

    // interface the derived classes can custiomize their behavior

    // needed for serialization, @return class name, must not be 0
    virtual const char* getType() const = 0;
    //
    virtual bool isVariable() const = 0;
    // imGui properties
    // @return something changed, set dirty / recompile
    virtual bool imGui() { return false; }
    // @return success
    virtual bool generateCode(GenerateCodeContext& context) = 0;
    //
    virtual void save(rapidjson::Document& d, rapidjson::Value& objValue) const;
    //
    virtual bool load(const rapidjson::Document::ValueType& doc);
    //
    virtual void drawBox(const StackerUI& stackerUI, const ImVec2 minR, const ImVec2 maxR) { (void)stackerUI; (void)minR; (void)maxR; }
    // todo: refine interface e.g. how many, what type
    virtual bool canHaveInput() const { return true; }

    // for debugging
    virtual void validate() const {}
};

// --------------------------------------------------------------------------------

class StackerUI {
public:
    // all mthods need to be implemented as thi class is also used as nUll implementation
    struct IAppConnection {
        virtual ~IAppConnection() {}
        virtual void openContextMenu(StackerUI& /*stackerUI*/, const StackerBoxRect& /*rect*/) {}
        // @param className must not be 0
        // @return 0 if failed
        virtual StackerBox* createNode(const char* /*className*/) { return nullptr; }
        virtual void startCompile() {}
        virtual std::string* code() { return nullptr; }
        virtual void endCompile() {};
        // @return 0 if there are no warnings and errors
        virtual const char* getWarningsAndErrors() { return nullptr; }
    };

    int32 scrollingX = 0;
    int32 scrollingY = 0;

    ImVec2 mouseDelta = {};

    // object currently being resized (handles)
    // -1:not used, otherwise index into stackerBoxes
    int32 resizeObjectId = -1;
    // bit0:top, bit1:right, bit2:bottom, bit3:left
    uint32 resizeHandleMask = 0;
    // values are Id into stackerBoxes[], never <0 or >=stackerBoxes.size()
    std::vector<int32> selectedObjects;

    // only used if drag is active and selectedObject == -1
    int32 selectStartX = 0, selectStartY = 0;

    //
    bool dragActive = false;
    bool scrollingActive = false;
    // in pixels, sum of all movements in manhattan distance
    float scrollingAmount = 0.0f;
    bool contextMenuIsOpen = false;
    bool autoGenerateCode = true;
    // if true it will trigger a recompile
    bool dirty = true;

    // [Id] = data
    std::vector<StackerBox*> stackerBoxes;
    // for code generation and drawing
    // once setup same size as stackerBoxes, values are id into stackerBoxes[]
    std::vector<int32> order;

    // UI customizations
    const int32 scale = 32;
    // must be smaller or same as scale, affects text clipping
    const uint32 clipBorderSize = 6;
    // must be smaller or same as scale
    const uint32 handleSizeX = scale;
    // must be smaller or same as scale
    const uint32 handleSizeY = scale;
    // must be smaller or same as scale
    const uint32 crossSize = scale / 2;
    // must be smaller or same as scale
    const uint32 connectorSize = scale / 2;

    // in grid cells, 0, 1, 2, ..
    const int32 newHalfSize = 2;

    // destructor
    ~StackerUI();

    // @param appConnection may be 0, pointer is stored, not released
    void setAppConnection(IAppConnection* inAppConnection);

    // adds to stackerBoxes, selects / shows properties
    void addFromUI(StackerBox& in);
    //
    void draw();
    // @param fileName must not be 0
    bool load(const char* fileName);
    // @param fileName must not be 0
    void save(const char* fileName);
    //
    void freeData();

    //
    void clipboardCut();
    //
    void clipboardCopy();
    //
    void clipboardPaste();

private:
    IAppConnection nullAppConnection;
    // never 0 so error handling is simpler
    IAppConnection* appConnection = &nullAppConnection;

    // [x + y * width], used by buildGraph(), -1:no used, -2:inside box after first line, otherwise index into stackerBoxes[]
    std::vector<int32> tempBitmap;
    // left, top, right, bottom
    int32 bitmapBounds[4] = { 0, 0, 0, 0 };

    //
    void propertiesUI();
    // @param fullMode false:verify only, true: generate code
    void generateCode(const bool fullMode);
    //
    void generatedCodeUI();
    //
    void drawBoxes(const ImVec2 offset, const int32 hoverObjectId, const uint32 hoverHandle);
    //
    void drawConnectors(const ImVec2 offset);
    //
    void buildGraph();
    //
    void contextMenu(const int32 mousePosX, const int32 mousePosY);
    //
    void buildOrder();
    //
    void bitmapReset();
    //
    uint32 getBitmapWidth() const { return bitmapBounds[2] - bitmapBounds[0]; }
    //
    uint32 getBitmapHeight() const { return bitmapBounds[3] - bitmapBounds[1]; }
    // @param id index into stackerBoxes[]
    void findChildren(const int32 id, std::vector<StackerBox*>& children);
    // @param id index into stackerBoxes[]
    void selectObject(const int32 id, const bool shift);
    //
    void clearSelection();
    // @param id index into stackerBoxes[]
    bool isSelected(const int32 id) const;
    // @return true if there is any selection
    bool anySelection();
    //
    void deleteSelection();
    // @param id index into stackerBoxes[]
    void addToSelection(const int32 id);
    //
    void reset();
    // @param what indices into stackerBoxes[]
    void saveToBuffer(rapidjson::StringBuffer& strbuf, const std::vector<int32>& what);
    //
    bool loadFromBuffer(const std::vector<char>& fileBlob);
};
