/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2021 Silverlan
 */

#ifndef __C_RENDERER_PP_MOTION_BLUR_COMPONENT_HPP__
#define __C_RENDERER_PP_MOTION_BLUR_COMPONENT_HPP__

#include "pragma/entities/components/renderers/c_renderer_pp_base_component.hpp"
#include "pragma/entities/components/renderers/c_renderer_component.hpp"
#include <image/prosper_render_target.hpp>
#include <wgui/types.hpp>
#include <mathutil/transform.hpp>

#define MOTION_BLUR_DEBUG_ELEMENT_ENABLED 0

namespace pragma
{
	enum class MotionBlurQuality : uint32_t
	{
		Low = 0,
		Medium,
		High
	};

	struct DLLCLIENT MotionBlurTemporalData
	{
		struct PoseData
		{
			Mat4 matrix;
			umath::Transform pose;
		};
		std::unordered_map<const BaseEntity*,PoseData> prevModelMatrices;
		std::unordered_map<const BaseEntity*,PoseData> curModelMatrices;
		double lastTick = 0.0;
	};

#pragma pack(push,1)
	struct DLLCLIENT MotionBlurData
	{
		Vector4 linearCameraVelocity;
		Vector4 angularCameraVelocity;
	};
#pragma pack(pop)
	
	class DLLCLIENT CRendererPpMotionBlurComponent final
		: public CRendererPpBaseComponent
	{
	public:
		static void RegisterMembers(pragma::EntityComponentManager &componentManager,TRegisterComponentMember registerMember);

		CRendererPpMotionBlurComponent(BaseEntity &ent);
		virtual void Initialize() override;
		virtual void InitializeLuaObject(lua_State *l) override;
		virtual void OnRemove() override;
		virtual std::string GetIdentifier() const override {return "motion_blur";}
		virtual uint32_t GetPostProcessingWeight() const override {return umath::to_integral(CRendererComponent::StandardPostProcessingWeight::MotionBlur);}

		void SetAutoUpdateMotionData(bool updateMotionPerFrame);
		void UpdateMotionBlurData();
		void UpdatePoses();

		void SetMotionBlurIntensity(float intensity);
		float GetMotionBlurIntensity() const;

		void SetMotionBlurQuality(MotionBlurQuality quality);
		MotionBlurQuality GetMotionBlurQuality() const;

		const std::shared_ptr<prosper::ISwapCommandBufferGroup> &GetSwapCommandBuffer() const;
		const std::shared_ptr<prosper::RenderTarget> &GetRenderTarget() const;
	private:
		virtual void DoRenderEffect(const util::DrawSceneInfo &drawSceneInfo) override;
		void RecordVelocityPass(const util::DrawSceneInfo &drawSceneInfo);
		void ExecuteVelocityPass(const util::DrawSceneInfo &drawSceneInfo);
		void RenderPostProcessing(const util::DrawSceneInfo &drawSceneInfo);
		void ReloadVelocityTexture();
		util::WeakHandle<prosper::Shader> m_velocityShader {};
		std::shared_ptr<prosper::ISwapCommandBufferGroup> m_swapCmd = nullptr;
		std::shared_ptr<prosper::IDescriptorSetGroup> m_velocityTexDsg;
		std::shared_ptr<prosper::IDescriptorSetGroup> m_motionBlurDataDsg;
		std::shared_ptr<prosper::IBuffer> m_motionBlurDataBuffer;
		std::shared_ptr<prosper::RenderTarget> m_renderTarget;
		MotionBlurTemporalData m_motionBlurData {};
#if MOTION_BLUR_DEBUG_ELEMENT_ENABLED == 1
		WIHandle m_debugTex;
#endif
		float m_motionBlurIntensityFactor = 4.f;
		MotionBlurQuality m_motionBlurQuality = MotionBlurQuality::Low;
		bool m_valid = false;
		bool m_autoUpdateMotionData = true;
		bool m_motionDataUpdateRequired = false;
	};
};

#endif
