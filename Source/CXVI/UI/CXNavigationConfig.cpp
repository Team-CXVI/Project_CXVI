// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CXNavigationConfig.h"

#include <InputCoreTypes.h>

FCXNavigationConfig::FCXNavigationConfig()
{
	KeyEventRules.Emplace(EKeys::W, EUINavigation::Up);
	KeyEventRules.Emplace(EKeys::S, EUINavigation::Down);
	KeyEventRules.Emplace(EKeys::A, EUINavigation::Left);
	KeyEventRules.Emplace(EKeys::D, EUINavigation::Right);
}
