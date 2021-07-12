/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2021 Silverlan
 */

#include "stdafx_shared.h"
#include "pragma/model/model.h"
#include "pragma/model/animation/animation.hpp"
#include "pragma/model/animation/animation_channel.hpp"
#include "pragma/model/animation/animation_player.hpp"
#include "pragma/model/animation/animation2.hpp"

#pragma optimize("",off)
std::shared_ptr<pragma::animation::AnimationPlayer> pragma::animation::AnimationPlayer::Create()
{
	return std::shared_ptr<AnimationPlayer>{new AnimationPlayer{}};
}
std::shared_ptr<pragma::animation::AnimationPlayer> pragma::animation::AnimationPlayer::Create(const AnimationPlayer &other)
{
	return std::shared_ptr<AnimationPlayer>{new AnimationPlayer{other}};
}
std::shared_ptr<pragma::animation::AnimationPlayer> pragma::animation::AnimationPlayer::Create(AnimationPlayer &&other)
{
	return std::shared_ptr<AnimationPlayer>{new AnimationPlayer{std::move(other)}};
}
pragma::animation::AnimationPlayer::AnimationPlayer()
{}
pragma::animation::AnimationPlayer::AnimationPlayer(const AnimationPlayer &other)
	: m_playbackRate{other.m_playbackRate},
	m_currentTime{other.m_currentTime},m_looping{other.m_looping},m_lastChannelTimestampIndices{other.m_lastChannelTimestampIndices},
	m_animation{other.m_animation},m_currentSlice{other.m_currentSlice}
{
	static_assert(sizeof(*this) == 96,"Update this implementation when class has changed!");
}
pragma::animation::AnimationPlayer::AnimationPlayer(AnimationPlayer &&other)
	: m_playbackRate{other.m_playbackRate},
	m_currentTime{other.m_currentTime},m_looping{other.m_looping},m_lastChannelTimestampIndices{std::move(other.m_lastChannelTimestampIndices)},
	m_animation{other.m_animation},m_currentSlice{std::move(other.m_currentSlice)}
{
	static_assert(sizeof(*this) == 96,"Update this implementation when class has changed!");
}
pragma::animation::AnimationPlayer &pragma::animation::AnimationPlayer::operator=(const AnimationPlayer &other)
{
	m_playbackRate = other.m_playbackRate;
	m_currentTime = other.m_currentTime;
	m_looping = other.m_looping;
	m_animation = other.m_animation;
	m_currentSlice = other.m_currentSlice;

	m_lastChannelTimestampIndices = other.m_lastChannelTimestampIndices;
	static_assert(sizeof(*this) == 96,"Update this implementation when class has changed!");
	return *this;
}
pragma::animation::AnimationPlayer &pragma::animation::AnimationPlayer::operator=(AnimationPlayer &&other)
{
	m_playbackRate = other.m_playbackRate;
	m_currentTime = other.m_currentTime;
	m_looping = other.m_looping;
	m_animation = other.m_animation;
	m_currentSlice = std::move(other.m_currentSlice);

	m_lastChannelTimestampIndices = std::move(other.m_lastChannelTimestampIndices);
	static_assert(sizeof(*this) == 96,"Update this implementation when class has changed!");
	return *this;
}
float pragma::animation::AnimationPlayer::GetDuration() const
{
	if(!m_animation)
		return 0.f;
	return m_animation->GetDuration();
}
float pragma::animation::AnimationPlayer::GetRemainingAnimationDuration() const {return GetDuration() -GetCurrentTime();}
float pragma::animation::AnimationPlayer::GetCurrentTimeFraction() const
{
	auto t = GetCurrentTime();
	auto dur = GetDuration();
	return (dur > 0.f) ? (t /dur) : 0.f;
}
void pragma::animation::AnimationPlayer::SetCurrentTimeFraction(float t,bool forceUpdate) {SetCurrentTime(t *GetDuration(),forceUpdate);}
void pragma::animation::AnimationPlayer::SetCurrentTime(float t,bool forceUpdate)
{
	if(t == m_currentTime && !forceUpdate)
		return;
	m_currentTime = t;
	Advance(0.f,true);
}
void pragma::animation::AnimationPlayer::Advance(float dt,bool forceUpdate)
{
	if(!m_animation)
		return;
	auto &anim = m_animation;
	dt *= m_playbackRate;
	auto newTime = m_currentTime;
	newTime += dt;
	auto dur = anim->GetDuration();
	if(newTime > dur)
	{
		if(m_looping && dur > 0.f)
		{
			auto d = fmodf(newTime,dur);
			newTime = d;
		}
		else
			newTime = dur;
	}
	if(newTime == m_currentTime && !forceUpdate)
		return;
	m_currentTime = newTime;

	auto &channels = anim->GetChannels();
	auto numChannels = umath::min(channels.size(),m_currentSlice.channelValues.size());
	auto vs = [this,newTime](auto tag,pragma::animation::AnimationChannel &channel,udm::Property &sliceData,uint32_t &inOutPivotTimeIndex) {
		using T = decltype(tag)::type;
		if constexpr(
			!std::is_same_v<T,udm::Vector2i> && !std::is_same_v<T,udm::Vector3i> && !std::is_same_v<T,udm::Vector4i> && !std::is_same_v<T,udm::Srgba> && !std::is_same_v<T,udm::HdrColor> &&
			!std::is_same_v<T,udm::Transform> && !std::is_same_v<T,udm::ScaledTransform> && !std::is_same_v<T,udm::Nil>
		)
			sliceData = channel.GetInterpolatedValue<T>(newTime,inOutPivotTimeIndex);
	};
	for(auto i=decltype(numChannels){0u};i<numChannels;++i)
	{
		auto &channel = channels[i];
		auto &sliceData = m_currentSlice.channelValues[i];
		auto &lastChannelTimestampIndex = m_lastChannelTimestampIndices[i];
		auto valueType = channel->GetValueType();
		if(udm::is_numeric_type(valueType))
			std::visit([&vs,&channel,&sliceData,&lastChannelTimestampIndex](auto tag) {vs(tag,*channel,*sliceData,lastChannelTimestampIndex);},udm::get_numeric_tag(valueType));
		else if(udm::is_generic_type(valueType))
			std::visit([&vs,&channel,&sliceData,&lastChannelTimestampIndex](auto tag) {vs(tag,*channel,*sliceData,lastChannelTimestampIndex);},udm::get_generic_tag(valueType));
	}
	// TODO
	// ApplySliceInterpolation(m_prevAnimSlice,m_currentSlice,fadeFactor);
}

void pragma::animation::AnimationPlayer::SetAnimation(const Animation2 &animation)
{
	m_animation = animation.shared_from_this();
	auto &channels = animation.GetChannels();
	m_currentSlice.channelValues.reserve(channels.size());
	for(auto &channel : channels)
		m_currentSlice.channelValues.push_back(udm::Property::Create(channel->GetValueType()));
	m_lastChannelTimestampIndices.clear();
	m_lastChannelTimestampIndices.resize(channels.size(),std::numeric_limits<uint32_t>::max());
	Reset();
}

void pragma::animation::AnimationPlayer::Reset()
{
	m_currentTime = 0.f;
	m_lastChannelTimestampIndices.clear();
}
void pragma::animation::AnimationPlayer::ApplySliceInterpolation(const AnimationSlice &src,AnimationSlice &dst,float f)
{
	// TODO
	/*if(f == 1.f)
		return;
	auto n = src.channelValues.size();
	for(auto i=decltype(n){0u};i<n;++i)
	{
		//auto &channel = m_channels[i];
		pragma::AnimationChannel &channel ;// TODO
		// TODO: How to translate channels?
		auto &srcVal = src.channelValues[i];
		auto &dstVal = dst.channelValues[i];
		// TODO
		auto interp = channel.GetInterpolationFunction<Vector3>();
		dstVal = interp(srcVal.GetValue<Vector3>(),dstVal.GetValue<Vector3>(),f);
	}*/
}

std::ostream &operator<<(std::ostream &out,const pragma::animation::AnimationPlayer &o)
{
	out<<"AnimationPlayer";
	out<<"[Time:"<<o.GetCurrentTime()<<"/"<<o.GetDuration()<<"]";
	out<<"[PlaybackRate:"<<o.GetPlaybackRate()<<"]";

	auto *anim = o.GetAnimation();
	out<<"[Anim:";
	if(!anim)
		out<<"NULL";
	else
		out<<*anim;
	out<<"]";
	return out;
}

std::ostream &operator<<(std::ostream &out,const pragma::animation::AnimationSlice &o)
{
	out<<"AnimationSlice";
	out<<"[Values:"<<o.channelValues.size()<<"]";
	return out;
}
#pragma optimize("",on)
