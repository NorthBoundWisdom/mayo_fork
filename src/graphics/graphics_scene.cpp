/****************************************************************************
** Copyright (c) 2021, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#include "graphics_scene.h"

#include <unordered_set>

#include <Graphic3d_GraphicDriver.hxx>
#include <V3d_AmbientLight.hxx>
#include <V3d_DirectionalLight.hxx>
#include <V3d_TypeOfOrientation.hxx>
#include <V3d_Viewer.hxx>

#include "graphics_utils.h"

namespace Mayo
{

class GraphicsScene::Private
{
public:
    OccHandle<V3d_Viewer> m_viewer = nullptr;
    OccHandle<AIS_InteractiveContext> m_aisContext = nullptr;
    std::unordered_set<const AIS_InteractiveObject *> m_setClipPlaneSensitive;
    bool m_isRedrawBlocked = false;
    SelectionMode m_selectionMode = SelectionMode::Single;
};

static std::function<OccHandle<Graphic3d_GraphicDriver>()> &getFunctionCreateGraphicsDriver()
{
    static std::function<OccHandle<Graphic3d_GraphicDriver>()> fn;
    return fn;
}

void GraphicsScene::setFunctionCreateGraphicsDriver(
    std::function<OccHandle<Graphic3d_GraphicDriver>()> fn)
{
    getFunctionCreateGraphicsDriver() = std::move(fn);
}

GraphicsScene::GraphicsScene()
    : p_context(new Private)
{
    const auto &fn = getFunctionCreateGraphicsDriver();
    if (!fn)
    {
        std::cerr << "Error: No function set to create Graphic3d_GraphicDriver.\n";
        return;
    }

    p_context->m_viewer = makeOccHandle<V3d_Viewer>(fn());
    p_context->m_viewer->SetDefaultViewSize(1000.);
    p_context->m_viewer->SetDefaultViewProj(V3d_XposYnegZpos);
    p_context->m_viewer->SetComputedMode(true);
    p_context->m_viewer->SetDefaultComputedMode(true);
    p_context->m_viewer->SetDefaultLights();
    p_context->m_viewer->SetLightOn();

    p_context->m_aisContext = new AIS_InteractiveContext(p_context->m_viewer);
}

GraphicsScene::~GraphicsScene()
{
    // Preventive cleaning fixes weird crash happening in MSVC Debug mode
    p_context->m_aisContext->RemoveFilters();
    p_context->m_aisContext->Deactivate();
    p_context->m_aisContext->EraseAll(false);
    p_context->m_aisContext->RemoveAll(false);
    p_context->m_aisContext.Nullify();
    delete p_context;
    std::cout << "GraphicsScene::~GraphicsScene() end" << std::endl;
}

OccHandle<V3d_View> GraphicsScene::createV3dView()
{
    return this->v3dViewer()->CreateView();
}

const OccHandle<V3d_Viewer> &GraphicsScene::v3dViewer() const
{
    return p_context->m_aisContext->CurrentViewer();
}

const OccHandle<StdSelect_ViewerSelector3d> &GraphicsScene::mainSelector() const
{
    return p_context->m_aisContext->MainSelector();
}

bool GraphicsScene::hiddenLineDrawingOn() const
{
    return p_context->m_aisContext->DrawHiddenLine();
}

const OccHandle<Prs3d_Drawer> &GraphicsScene::drawerDefault() const
{
    return p_context->m_aisContext->DefaultDrawer();
}

const OccHandle<Prs3d_Drawer> &GraphicsScene::drawerHighlight(Prs3d_TypeOfHighlight style) const
{
    return p_context->m_aisContext->HighlightStyle(style);
}

void GraphicsScene::addObject(const GraphicsObjectPtr &object, AddObjectFlags flags)
{
    if (object)
    {
        if ((flags & AddObjectDisableSelectionMode) != 0)
        {
            const bool onEntry_AutoActivateSelection =
                p_context->m_aisContext->GetAutoActivateSelection();

            p_context->m_aisContext->SetAutoActivateSelection(false);
            p_context->m_aisContext->Display(object, 0, -1, false);
            p_context->m_aisContext->SetAutoActivateSelection(onEntry_AutoActivateSelection);
        }
        else
        {
            p_context->m_aisContext->Display(object, false);
        }
    }
}

void GraphicsScene::eraseObject(const GraphicsObjectPtr &object)
{
    GraphicsUtils::AisContext_eraseObject(p_context->m_aisContext, object);
    p_context->m_setClipPlaneSensitive.erase(object.get());
}

void GraphicsScene::redraw()
{
    if (p_context->m_isRedrawBlocked)
        return;

    for (auto itView = this->v3dViewer()->DefinedViewIterator(); itView.More(); itView.Next())
        this->signalRedrawRequested.send(itView.Value());
}

void GraphicsScene::redraw(const OccHandle<V3d_View> &view)
{
    if (p_context->m_isRedrawBlocked)
        return;

    for (auto itView = this->v3dViewer()->DefinedViewIterator(); itView.More(); itView.Next())
    {
        if (itView.Value() == view)
        {
            this->signalRedrawRequested.send(view);
            break;
        }
    }
}

bool GraphicsScene::isRedrawBlocked() const
{
    return p_context->m_isRedrawBlocked;
}

void GraphicsScene::blockRedraw(bool on)
{
    p_context->m_isRedrawBlocked = on;
}

void GraphicsScene::recomputeObjectPresentation(const GraphicsObjectPtr &object)
{
    p_context->m_aisContext->Redisplay(object, false);
}

void GraphicsScene::activateObjectSelection(const GraphicsObjectPtr &object, int mode)
{
    p_context->m_aisContext->Activate(object, mode);
}

void GraphicsScene::deactivateObjectSelection(const Mayo::GraphicsObjectPtr &object, int mode)
{
    p_context->m_aisContext->Deactivate(object, mode);
}

void GraphicsScene::deactivateObjectSelection(const GraphicsObjectPtr &object)
{
    p_context->m_aisContext->Deactivate(object);
}

void GraphicsScene::addSelectionFilter(const OccHandle<SelectMgr_Filter> &filter)
{
    p_context->m_aisContext->AddFilter(filter);
}

void GraphicsScene::removeSelectionFilter(const OccHandle<SelectMgr_Filter> &filter)
{
    p_context->m_aisContext->RemoveFilter(filter);
}

void GraphicsScene::clearSelectionFilters()
{
    p_context->m_aisContext->RemoveFilters();
}

void GraphicsScene::setObjectDisplayMode(const GraphicsObjectPtr &object, int displayMode)
{
    p_context->m_aisContext->SetDisplayMode(object, displayMode, false);
}

bool GraphicsScene::isObjectClipPlaneSensitive(const GraphicsObjectPtr &object) const
{
    if (object.IsNull())
        return false;

    return p_context->m_setClipPlaneSensitive.find(object.get()) !=
           p_context->m_setClipPlaneSensitive.cend();
}

void GraphicsScene::setObjectClipPlaneSensitive(const GraphicsObjectPtr &object, bool on)
{
    if (object.IsNull())
        return;

    if (on)
        p_context->m_setClipPlaneSensitive.insert(object.get());
    else
        p_context->m_setClipPlaneSensitive.erase(object.get());
}

bool GraphicsScene::isObjectVisible(const GraphicsObjectPtr &object) const
{
    return p_context->m_aisContext->IsDisplayed(object);
}

void GraphicsScene::setObjectVisible(const GraphicsObjectPtr &object, bool on)
{
    GraphicsUtils::AisContext_setObjectVisible(p_context->m_aisContext, object, on);
}

gp_Trsf GraphicsScene::objectTransformation(const GraphicsObjectPtr &object) const
{
    return p_context->m_aisContext->Location(object);
}

void GraphicsScene::setObjectTransformation(const GraphicsObjectPtr &object, const gp_Trsf &trsf)
{
    p_context->m_aisContext->SetLocation(object, trsf);
}

GraphicsOwnerPtr GraphicsScene::firstSelectedOwner() const
{
    p_context->m_aisContext->InitSelected();
    if (p_context->m_aisContext->MoreSelected())
        return p_context->m_aisContext->SelectedOwner();

    return {};
}

void GraphicsScene::clearSelection()
{
    const bool onEntryOwnerSelected = !this->firstSelectedOwner().IsNull();
    p_context->m_aisContext->ClearDetected(false);
    p_context->m_aisContext->ClearSelected(false);
    if (onEntryOwnerSelected)
        this->signalSelectionChanged.send();
}

AIS_InteractiveContext *GraphicsScene::aisContextPtr() const
{
    return p_context->m_aisContext.get();
}

void GraphicsScene::toggleOwnerSelection(const GraphicsOwnerPtr &gfxOwner)
{
    auto gfxObject = GraphicsObjectPtr::DownCast(
        gfxOwner ? gfxOwner->Selectable() : OccHandle<SelectMgr_SelectableObject>());
    if (GraphicsUtils::AisObject_isVisible(gfxObject))
        p_context->m_aisContext->AddOrRemoveSelected(gfxOwner, false);
}

void GraphicsScene::highlightAt(int xPos, int yPos, const OccHandle<V3d_View> &view)
{
    p_context->m_aisContext->MoveTo(xPos, yPos, view, false);
}

void GraphicsScene::select()
{
    if (p_context->m_selectionMode == SelectionMode::None)
        return;

    if (p_context->m_selectionMode == SelectionMode::Single)
    {
        p_context->m_aisContext->SelectDetected(AIS_SelectionScheme_Replace);
    }
    else if (p_context->m_selectionMode == SelectionMode::Multi)
    {
        p_context->m_aisContext->SelectDetected(AIS_SelectionScheme_XOR);
    }

    this->signalSelectionChanged.send();
}

int GraphicsScene::selectedCount() const
{
    return p_context->m_aisContext->NbSelected();
}

const GraphicsOwnerPtr &GraphicsScene::currentHighlightedOwner() const
{
    return p_context->m_aisContext->DetectedOwner();
}

GraphicsScene::SelectionMode GraphicsScene::selectionMode() const
{
    return p_context->m_selectionMode;
}

void GraphicsScene::setSelectionMode(GraphicsScene::SelectionMode mode)
{
    if (mode != p_context->m_selectionMode)
    {
        p_context->m_selectionMode = mode;
        this->signalSelectionModeChanged.send();
    }
}

GraphicsSceneRedrawBlocker::GraphicsSceneRedrawBlocker(GraphicsScene *scene)
    : m_scene(scene)
    , m_isRedrawBlockedOnEntry(scene->isRedrawBlocked())
{
    scene->blockRedraw(true);
}

GraphicsSceneRedrawBlocker::~GraphicsSceneRedrawBlocker()
{
    m_scene->blockRedraw(m_isRedrawBlockedOnEntry);
}

} // namespace Mayo
