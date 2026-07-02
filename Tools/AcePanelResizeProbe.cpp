#include "ArhqenCognitionEngine/Ui/Core/AcePanelResizePolicy.h"
#include "ArhqenCognitionEngine/AquariumUI/AceEnvironment3DMode.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DLayoutPersistence.h"
#include <filesystem>
#include <iostream>
#include <string_view>
namespace{int f=0;void ck(bool v,std::string_view n){std::cout<<(v?"PASS|":"FAIL|")<<n<<'\n';if(!v)++f;}}
int main(){using namespace am::ui;UiRect p{100,100,500,700};auto allowed=PanelResizeEdge::Right|PanelResizeEdge::Bottom;
 ck(PanelResizePolicy::hitTest(p,499,300,allowed)==PanelResizeEdge::Right,"right_edge_hit");ck(PanelResizePolicy::hitTest(p,300,699,allowed)==PanelResizeEdge::Bottom,"bottom_edge_hit");
 auto corner=PanelResizePolicy::hitTest(p,500,700,allowed);ck(hasEdge(corner,PanelResizeEdge::Right)&&hasEdge(corner,PanelResizeEdge::Bottom),"corner_combines_axes");ck(PanelResizePolicy::hitTest(p,300,300,allowed)==PanelResizeEdge::None,"panel_interior_not_resize_target");
 UiRect bounds{0,0,1200,900};auto grown=PanelResizePolicy::apply(p,corner,1000,1000,bounds,120,160);ck(grown.right==1200&&grown.bottom==900,"resize_reaches_physical_bounds_without_artificial_max");
 auto shrunk=PanelResizePolicy::apply(p,PanelResizeEdge::Right|PanelResizeEdge::Bottom,-1000,-1000,bounds,120,160);ck(shrunk.width()==120&&shrunk.height()==160,"minimum_recoverable_size_enforced");
 ck(PanelResizePolicy::cursor(PanelResizeEdge::Right)==PanelResizeCursor::Horizontal&&PanelResizePolicy::cursor(PanelResizeEdge::Bottom)==PanelResizeCursor::Vertical,"axis_cursors");
 ace::aquarium_ui::AceEnvironment3DMode mode;ace::aquarium_ui::AceEnvironment3DPanelState state;state.detailsWidth=900;state.logsWidth=120;state.detailsHeight=950;state.logsHeight=950;mode.ClampPanelState(state,1600,1000);
 ck(state.detailsWidth>500,"legacy_500px_width_cap_removed");ck(state.detailsHeight>920,"legacy_920px_height_cap_removed");auto layout=mode.Compute(1600,1000,state);ck(layout.leftResizeHandle.Empty()&&layout.rightResizeHandle.Empty(),"visible_corner_handles_removed");
 ck(layout.leftContentClip.bottom>layout.leftPanel.bottom-30&&layout.rightLogsContent.bottom>layout.rightLogsPanel.bottom-30,"content_uses_reclaimed_handle_space");
 D2DDockLayoutProfile profile;profile.aquariumDetailsWidth=777;profile.aquariumDetailsHeight=888;profile.aquariumLogsWidth=333;profile.aquariumLogsHeight=444;const auto file=std::filesystem::current_path()/"Build"/"ACE-PANEL-RESIZE"/"layout.test";std::string error;
 ck(D2DLayoutPersistence::save(file,profile,&error),"panel_dimensions_save");auto loaded=D2DLayoutPersistence::load(file,&error);ck(loaded&&loaded->aquariumDetailsWidth==777&&loaded->aquariumLogsHeight==444,"panel_dimensions_restore");std::error_code ec;std::filesystem::remove(file,ec);
 if(f){std::cout<<"FAIL|ace_panel_resize_probe|count="<<f<<'\n';return 1;}std::cout<<"PASS|ace_panel_resize_probe\n";return 0;}
