// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/Combat/CXCombatMenuCameraData.h"

namespace
{
	FName GetLegacyMenuCameraId(ECommandMenuState MenuState)
	{
		switch (MenuState)
		{
		case ECommandMenuState::ActionMenu: return TEXT("Menu.Action");
		case ECommandMenuState::ItemMenu: return TEXT("Menu.Item");
		case ECommandMenuState::SkillMenu: return TEXT("Menu.Skill");
		case ECommandMenuState::TargetSelect: return TEXT("Menu.TargetSelect");
		default: return NAME_None;
		}
	}
}

void UCXCombatMenuCameraData::PostLoad()
{
	Super::PostLoad();

	for (const auto& Entry : MenuPresets)
	{
		const FName PresetId = GetLegacyMenuCameraId(Entry.Key);
		if (PresetId.IsNone() || MenuCameraIds.Contains(Entry.Key))
		{
			continue;
		}

		if (!CameraPresets.Contains(PresetId))
		{
			CameraPresets.Add(PresetId, Entry.Value);
		}
		MenuCameraIds.Add(Entry.Key, PresetId);
	}
	MenuPresets.Reset();
}

