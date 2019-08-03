﻿#include "stdafx_client.h"
#include "pragma/game/c_game.h"
#include "pragma/c_engine.h"
#include "pragma/model/c_side.h"
#include <mathutil/uquat.h>
#include "pragma/debug/c_debugoverlay.h"
#include "pragma/rendering/shaders/c_shader.h"
#include "pragma/rendering/c_rendermode.h"
#include "pragma/model/c_model.h"
#include "pragma/model/c_modelmesh.h"
#include "pragma/model/brush/c_brushmesh.h"
#include "pragma/entities/components/c_player_component.hpp"
#include <wgui/wgui.h>
#include "pragma/rendering/uniformbinding.h"
#include "pragma/rendering/renderers/rasterization_renderer.hpp"
#include "pragma/console/c_cvar.h"
#include "pragma/entities/components/c_vehicle_component.hpp"
//#include "shader_gaussianblur.h" // prosper TODO
#include "pragma/rendering/world_environment.hpp"
#include "pragma/rendering/shaders/post_processing/c_shader_hdr.hpp"
#include "pragma/rendering/shaders/image/c_shader_additive.h"
#include "pragma/rendering/shaders/debug/c_shader_debug.h"
#include "pragma/rendering/shaders/post_processing/c_shader_pp_fog.hpp"
#include "pragma/rendering/shaders/world/c_shader_flat.hpp"
#include "pragma/rendering/shaders/post_processing/c_shader_pp_hdr.hpp"
#include "pragma/rendering/shaders/particles/c_shader_particle.hpp"
#include "pragma/rendering/rendersystem.h"
#include <pragma/lua/luacallback.h>
#include "pragma/rendering/scene/scene.h"
#include "pragma/opengl/renderhierarchy.h"
#include "luasystem.h"
#include "pragma/rendering/shaders/post_processing/c_shader_postprocessing.h"
#include "pragma/gui/widebugdepthtexture.h"
#include "pragma/debug/c_debug_game_gui.h"
#include <pragma/lua/luafunction_call.h>
#include "pragma/entities/environment/lights/c_env_light_spot.h"
#include "pragma/rendering/lighting/shadows/c_shadowmap.h"
#include "pragma/rendering/lighting/shadows/c_shadowmapcasc.h"
#include "pragma/lua/libraries/c_lua_vulkan.h"
#include "pragma/rendering/shaders/particles/c_shader_particle_polyboard.hpp"
#include "pragma/rendering/shaders/post_processing/c_shader_ssao.hpp"
#include "pragma/rendering/shaders/post_processing/c_shader_ssao_blur.hpp"
#include "pragma/rendering/shaders/world/c_shader_prepass.hpp"
#include "pragma/rendering/shaders/c_shader_forwardp_light_culling.hpp"
#include "pragma/physics/c_phys_visual_debugger.hpp"
#include <image/prosper_msaa_texture.hpp>
#include <image/prosper_render_target.hpp>
#include <prosper_util.hpp>
#include <prosper_command_buffer.hpp>
#include <textureinfo.h>
#include <sharedutils/scope_guard.h>
#include <pragma/rendering/c_sci_gpu_timer_manager.hpp>
#include <sharedutils/scope_guard.h>
#include <pragma/entities/entity_iterator.hpp>
#include <pragma/physics/visual_debugger.hpp>
#include <pragma/physics/environment.hpp>

extern DLLCENGINE CEngine *c_engine;
extern DLLCLIENT ClientState *client;
extern DLLCLIENT CGame *c_game;

#pragma optimize("",off)
static void CVAR_CALLBACK_render_vsync_enabled(NetworkState*,ConVar*,int,int val)
{
	glfwSwapInterval((val == 0) ? 0 : 1);
}
REGISTER_CONVAR_CALLBACK_CL(render_vsync_enabled,CVAR_CALLBACK_render_vsync_enabled);

static CallbackHandle cbDrawPhysics;
static CallbackHandle cbDrawPhysicsEnd;
static void CVAR_CALLBACK_debug_physics_draw(NetworkState*,ConVar*,int,int val,bool serverside)
{
	if(cbDrawPhysics.IsValid())
		cbDrawPhysics.Remove();
	if(cbDrawPhysicsEnd.IsValid())
		cbDrawPhysicsEnd.Remove();
	//auto *physEnv = c_game->GetPhysicsEnvironment();
	Game *game;
	if(serverside)
	{
		auto *nw = c_engine->GetServerNetworkState();
		game = nw ? nw->GetGameState() : nullptr;
	}
	else
		game = c_game;
	if(game == nullptr)
		return;
	auto *physEnv = game->GetPhysicsEnvironment();
	if(physEnv == nullptr)
		return;
	if(val == 0)
	{
		physEnv->SetVisualDebugger(nullptr);
		return;
	}
	auto visDebugger = std::make_unique<CPhysVisualDebugger>();
	physEnv->SetVisualDebugger(std::move(visDebugger));
	/*if(val == 0)
	{
		visDebugger->SetDebugMode(pragma::physics::IVisualDebugger::DebugMode::None);
		return;
	}*/
	cbDrawPhysics = c_game->AddCallback("Think",FunctionCallback<>::Create([serverside]() {
		Game *game;
		if(serverside)
		{
			auto *nw = c_engine->GetServerNetworkState();
			game = nw ? nw->GetGameState() : nullptr;
		}
		else
			game = c_game;
		if(game == nullptr)
			return;
		auto *physEnv = game->GetPhysicsEnvironment();
		auto *debugDraw = physEnv ? physEnv->GetVisualDebugger() : nullptr;
		if(debugDraw == nullptr)
			return;
		auto &vehicles = pragma::CVehicleComponent::GetAll();
		for(auto it=vehicles.begin();it!=vehicles.end();++it)
		{
			auto *vhc = *it;
#ifdef ENABLE_DEPRECATED_PHYSICS
			auto *btVhc = vhc->GetBtVehicle();
			if(btVhc != nullptr)
			{
				for(UChar i=0;i<vhc->GetWheelCount();++i)
				{
					btVhc->updateWheelTransform(i,true);
					auto *info = vhc->GetWheelInfo(i);
					if(info != nullptr)
					{
						auto &t = info->m_worldTransform;
						//debugDraw->drawTransform(t,info->m_wheelsRadius);
						//debugDraw->drawCylinder(info->m_wheelsRadius,info->m_wheelsRadius *0.5f,0,t,btVector3(1.f,0.f,0.f));
					}
				}
			}
#endif
		}
	}));
	cbDrawPhysicsEnd = c_game->AddCallback("OnGameEnd",FunctionCallback<>::Create([]() {
		cbDrawPhysics.Remove();
		cbDrawPhysicsEnd.Remove();
	}));
	auto mode = pragma::physics::IVisualDebugger::DebugMode::All;
	if(val == 2)
		mode = pragma::physics::IVisualDebugger::DebugMode::Wireframe;
	else if(val == 3)
		mode = pragma::physics::IVisualDebugger::DebugMode::Constraints;
	else if(val == 4)
		mode = pragma::physics::IVisualDebugger::DebugMode::Normals;
	visDebugger->SetDebugMode(mode);
}
REGISTER_CONVAR_CALLBACK_CL(debug_physics_draw,[](NetworkState *nw,ConVar *cv,int oldVal,int val) {CVAR_CALLBACK_debug_physics_draw(nw,cv,oldVal,val,false);});
REGISTER_CONVAR_CALLBACK_CL(sv_debug_physics_draw,[](NetworkState *nw,ConVar *cv,int oldVal,int val) {CVAR_CALLBACK_debug_physics_draw(nw,cv,oldVal,val,true);});

static void CVAR_CALLBACK_debug_render_depth_buffer(NetworkState*,ConVar*,bool,bool val)
{
	static std::unique_ptr<DebugGameGUI> dbg = nullptr;
	if(dbg == nullptr)
	{
		if(val == false)
			return;
		dbg = std::make_unique<DebugGameGUI>([]() {
			auto &scene = c_game->GetScene();
			auto *renderer = scene->GetRenderer();
			if(renderer == nullptr || renderer->IsRasterizationRenderer() == false)
				return WIHandle{};
			auto *rasterizer = static_cast<pragma::rendering::RasterizationRenderer*>(renderer);
			auto &wgui = WGUI::GetInstance();
			
			auto r = wgui.Create<WIDebugDepthTexture>();
			r->SetTexture(*rasterizer->GetPrepass().textureDepth,{
				Anvil::PipelineStageFlagBits::LATE_FRAGMENT_TESTS_BIT,Anvil::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL,Anvil::AccessFlagBits::DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
			},{
				Anvil::PipelineStageFlagBits::EARLY_FRAGMENT_TESTS_BIT,Anvil::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL,Anvil::AccessFlagBits::DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
			});
			r->SetShouldResolveImage(true);
			r->SetSize(1024,1024);
			r->Update();
			return r->GetHandle();
		});
		auto *d = dbg.get();
		dbg->AddCallback("PostRenderScene",FunctionCallback<>::Create([d]() {
			auto *el = d->GetGUIElement();
			if(el == nullptr)
				return;
			static_cast<WIDebugDepthTexture*>(el)->Update();
		}));
		return;
	}
	else if(val == true)
		return;
	dbg = nullptr;
}
REGISTER_CONVAR_CALLBACK_CL(debug_render_depth_buffer,CVAR_CALLBACK_debug_render_depth_buffer);

static CVar cvDrawScene = GetClientConVar("render_draw_scene");
static CVar cvDrawWorld = GetClientConVar("render_draw_world");
static CVar cvClearScene = GetClientConVar("render_clear_scene");
static CVar cvClearSceneColor = GetClientConVar("render_clear_scene_color");
static CVar cvParticleQuality = GetClientConVar("cl_render_particle_quality");
void CGame::RenderScenes(std::shared_ptr<prosper::PrimaryCommandBuffer> &drawCmd,std::shared_ptr<prosper::RenderTarget> &rt,FRender renderFlags,const Color *clearColor)//const Vulkan::RenderPass &renderPass,const Vulkan::Framebuffer &framebuffer,const Vulkan::CommandBuffer &drawCmd,FRender renderFlags,const Color *clearColor)
{
	if(cvDrawScene->GetBool() == false)
		return;
	//auto &context = const_cast<Vulkan::Context&>(c_engine->GetRenderContext()); // prosper TODO
	//context.SwapFrameIndex(); // prosper TODO

	// We have to free all shadow map depth buffers because
	// they might have to be re-assigned for this frame.
	// This only has to be done once per frame, not per scene!
	auto &shadowMaps = ShadowMap::GetAll();
	auto numShadowMaps = shadowMaps.Size();
	for(auto i=decltype(numShadowMaps){0};i<numShadowMaps;++i)
	{
		auto *shadowMap = shadowMaps[i];
		if(shadowMap == nullptr)
			continue;
		shadowMap->FreeRenderTarget();
	}

	auto drawWorld = cvDrawWorld->GetInt();
	if(drawWorld == 2)
		renderFlags &= ~(FRender::Shadows | FRender::Glow);
	else if(drawWorld == 0)
		renderFlags &= ~(FRender::Shadows | FRender::Glow | FRender::View | FRender::World | FRender::Skybox);

	if(cvParticleQuality->GetInt() <= 0)
		renderFlags &= ~FRender::Particles;

	// Update particle systems
	EntityIterator itParticles {*this};
	itParticles.AttachFilter<TEntityIteratorFilterComponent<pragma::CParticleSystemComponent>>();
	for(auto *ent : itParticles)
	{
		auto &tDelta = DeltaTime();
		auto pt = ent->GetComponent<pragma::CParticleSystemComponent>();
		if(pt.valid())
			pt->Simulate(tDelta);
	}

	auto &scene = GetRenderScene();
	if(scene == nullptr)
	{
		Con::cwar<<"WARNING: No active render scene!"<<Con::endl;
		return;
	}
	if(scene->IsValid() == false)
	{
		Con::cwar<<"WARNING: Attempted to render invalid scene!"<<Con::endl;
		return;
	}
	if(cvClearScene->GetBool() == true || drawWorld == 2 || clearColor != nullptr)
	{
		auto clearCol = (clearColor != nullptr) ? clearColor->ToVector4() : Color(cvClearSceneColor->GetString()).ToVector4();
		auto &hdrImg = scene->GetRenderer()->GetSceneTexture()->GetImage();
		prosper::util::record_image_barrier(*(*drawCmd),*(*hdrImg),Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,Anvil::ImageLayout::TRANSFER_DST_OPTIMAL);
		prosper::util::record_clear_image(*(*drawCmd),*(*hdrImg),Anvil::ImageLayout::TRANSFER_DST_OPTIMAL,{{clearCol.r,clearCol.g,clearCol.b,clearCol.a}});
		prosper::util::record_image_barrier(*(*drawCmd),*(*hdrImg),Anvil::ImageLayout::TRANSFER_DST_OPTIMAL,Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL);
	}

	// Update Exposure
	auto *renderer = scene->GetRenderer();
	if(renderer && renderer->IsRasterizationRenderer())
	{
		//c_engine->StartGPUTimer(GPUTimerEvent::UpdateExposure); // prosper TODO
		auto frame = c_engine->GetLastFrameId();
		if(frame > 0)
			static_cast<pragma::rendering::RasterizationRenderer*>(renderer)->GetHDRInfo().UpdateExposure(*rt->GetTexture());
		//c_engine->StopGPUTimer(GPUTimerEvent::UpdateExposure); // prosper TODO
	}

	// Update time
	UpdateShaderTimeData();

	CallCallbacks("PreRenderScenes");

	static auto bSkipCallbacks = false;
	if(bSkipCallbacks == false)
	{
		bSkipCallbacks = true;
		ScopeGuard guard([]() {bSkipCallbacks = false;});
		auto ret = false;
		m_bMainRenderPass = false;


		auto bSkipScene = CallCallbacksWithOptionalReturn<
			bool,std::reference_wrapper<std::shared_ptr<prosper::PrimaryCommandBuffer>>,std::reference_wrapper<std::shared_ptr<prosper::RenderTarget>>
		>("DrawScene",ret,std::ref(drawCmd),std::ref(rt)) == CallbackReturnType::HasReturnValue;
		m_bMainRenderPass = true;
		if(bSkipScene == true && ret == true)
			return;
		m_bMainRenderPass = false;
		if(CallLuaCallbacks<
			bool,std::reference_wrapper<std::shared_ptr<prosper::PrimaryCommandBuffer>>,std::reference_wrapper<std::shared_ptr<prosper::RenderTarget>>
		>("DrawScene",&bSkipScene,std::ref(drawCmd),std::ref(rt)) == CallbackReturnType::HasReturnValue && bSkipScene == true)
		{
			CallCallbacks("PostRenderScenes");
			m_bMainRenderPass = true;
			return;
		}
		else
			m_bMainRenderPass = true;
	}
	RenderScene(drawCmd,rt,renderFlags);
	CallCallbacks("PostRenderScenes");
	CallLuaCallbacks("PostRenderScenes");
}

bool CGame::IsInMainRenderPass() const {return m_bMainRenderPass;}
#pragma optimize("",on)
