/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2021 Silverlan
 */

#include "stdafx_shared.h"
#include <pragma/game/game.h>
#include "pragma/entities/entity_component_manager.hpp"
#include "pragma/entities/components/logic_component.hpp"
#include "pragma/entities/components/damageable_component.hpp"
#include "pragma/entities/components/global_component.hpp"
#include "pragma/entities/components/ik_component.hpp"
#include "pragma/entities/components/map_component.hpp"
#include "pragma/entities/components/submergible_component.hpp"
#include "pragma/entities/components/velocity_component.hpp"
#include "pragma/entities/components/usable_component.hpp"
#include "pragma/entities/components/panima_component.hpp"
#include "pragma/entities/components/composite_component.hpp"
#include "pragma/entities/components/animation_driver_component.hpp"
#include "pragma/entities/components/basegravity.h"

void Game::InitializeEntityComponents(pragma::EntityComponentManager &componentManager)
{
	componentManager.RegisterComponentType<pragma::DamageableComponent>("damageable");
	componentManager.RegisterComponentType<pragma::IKComponent>("ik");
	componentManager.RegisterComponentType<pragma::LogicComponent>("logic");
	componentManager.RegisterComponentType<pragma::GravityComponent>("gravity");
	componentManager.RegisterComponentType<pragma::MapComponent>("map");
	componentManager.RegisterComponentType<pragma::SubmergibleComponent>("submergible");
	componentManager.RegisterComponentType<pragma::VelocityComponent>("velocity");
	componentManager.RegisterComponentType<pragma::UsableComponent>("usable");
	componentManager.RegisterComponentType<pragma::GlobalNameComponent>("global");
	componentManager.RegisterComponentType<pragma::PanimaComponent>("panima");
	componentManager.RegisterComponentType<pragma::CompositeComponent>("composite");
	componentManager.RegisterComponentType<pragma::AnimationDriverComponent>("animation_driver");

	pragma::BaseEntityComponent::RegisterEvents(componentManager,[&componentManager](const std::string &evName,pragma::ComponentEventInfo::Type type) -> pragma::ComponentEventId {
		return componentManager.RegisterEvent(evName,typeid(pragma::BaseEntityComponent),type);
	});
	BaseEntity::RegisterEvents(componentManager);
}
