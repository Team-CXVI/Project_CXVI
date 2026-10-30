// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CXSceneTypes.h"
#include "EnhancedInputComponent.h"

#include "CXGameScene.generated.h"

class UCXSceneDefinition;
class UCXSceneSubsystem;
class UCXActivatableWidget;
class UInputAction;
class APawn;
class APlayerController;
class UEnhancedInputLocalPlayerSubsystem;

/**
 * Scene Object의 베이스 클래스(추상 클래스)
 * Lifecycle : OnEnter -> (OnSuspend <-> OnResume)* -> OnExit
 * 베이스가 Pawn, IMC, Scene 입력 바인딩, Suspendable 알림, Spawn Actor 정리를 처리하므로 자식은 반드시 Super롤 호출해야 함
 * - 컨트롤 Pawn : Definition의 PawnClass를 Scene 레벨에서 찾거나 생성해서 Enter / Resume 때 빙의
 * - IMC : Definition의 IMCs는 이 Scene이 활성인 동안만 등록.
 * - 같은 IMC를 Pawn 등 다른 곳에서 별도로 등록하지 않아야 함.
 */
UCLASS(Abstract, Blueprintable)
class CXVI_API UCXGameScene : public UObject
{
	GENERATED_BODY()
	
	friend class UCXSceneSubsystem;
	
public:
	virtual UWorld* GetWorld() const override;

	FORCEINLINE const UCXSceneDefinition* GetDefinition() const { return Definition; }

	UCXSceneSubsystem* GetSceneSubsystem() const;

	// 현재 로드된 이 Scene의 레벨들
	TArray<ULevel*> GetSceneLevels() const;

	// Sublevels 중 메인 레벨(배열을 0번 인덱스)
	ULevel* GetPrimaryLevel() const;

	// 이 Scene의 컨트롤 Pawn (PawnClass가 없거나 확보 실패 시 nullptr)
	APawn* GetScenePawn() const { return ScenePawn; }

	// Scene 레벨에서 T 타입 액터를 검색
	template<typename T>
	void FindActorsInSceneLevels(TArray<T*>& OutActors) const
	{
		for (const ULevel* Level : GetSceneLevels())
		{
			for (AActor* Actor : Level->Actors)
			{
				if (T* CastActor = Cast<T>(Actor))
				{
					OutActors.Emplace(CastActor);
				}
			}
		}
	}

	// Lifecycle
protected:
	virtual void OnEnter(const FCXSceneContext& InContext);

	virtual void OnExit();

	virtual void OnSuspend();

	virtual void OnResume(const FCXSceneContext& InContext);

	UFUNCTION(BlueprintImplementableEvent, Category = "Scene", meta = (Displayname = "On Enter"))
	void K2_OnEnter(const FCXSceneContext& InContext);

	UFUNCTION(BlueprintImplementableEvent, Category = "Scene", meta = (Displayname = "On Exit"))
	void K2_OnExit();

	UFUNCTION(BlueprintImplementableEvent, Category = "Scene", meta = (Displayname = "On Suspend"))
	void K2_OnSuspend();

	UFUNCTION(BlueprintImplementableEvent, Category = "Scene", meta = (DisplayName = "On Resume"))
	void K2_OnResume(const FCXSceneContext& InContext);

	// Scene HUD
protected:
	// Definition의 HUDClass가 Push된 직후 호출 (Enter / Resume마다 새 인스턴스) → ViewModel 주입 등
	virtual void InitializeSceneHUD(UCXActivatableWidget& InHUD) {}

	// Scene Input
protected:
	virtual void SetupSceneInput() {}

	// Scene이 활성화 될 때(Enter / Resume) 호출 -> 이 타이밍에 BindSceneAction으로 바인딩하면 Suspend / Exit 때 자동 해제
	template<typename UserClass,  typename FuncType>
	void BindSceneAction(const UInputAction* InAction, ETriggerEvent InTrigger, UserClass* InObject, FuncType InFunc)
	{
		if (UEnhancedInputComponent* Input = GetPlayerInputComponent())
		{
			InputBindingHandles.Add(Input->BindAction(InAction, InTrigger, InObject, InFunc).GetHandle());
		}
	}

	// Primary Level에 스폰하고 추적. OnExit에서 자동 Destroy (KeepLoaded 레벨에 남지 않도록)
	template<typename T>
	T* SpawnSceneActor(TSubclassOf<T> InClass, const FTransform& InTransform)
	{
		UWorld* World = GetWorld();
		if (nullptr == World || nullptr == InClass)
		{
			return nullptr;
		}

		FActorSpawnParameters Params;
		Params.OverrideLevel = GetPrimaryLevel();
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		T* Actor = World->SpawnActor<T>(InClass, InTransform, Params);
		if (Actor)
		{
			SpawnedActors.Emplace(Actor);
		}

		return Actor;
	}

private:
	// UI.Layer.Game에 Definition의 HUD 위젯을 Push
	void ShowSceneHUD();

	// HUD를 닫음 (Stack에서 자동 제거)
	void HideSceneHUD();

	APlayerController* GetPlayerController() const;

	UEnhancedInputComponent* GetPlayerInputComponent() const;

	// 월드 정리 중(PlayerController 제거 후)에도 접근할 수 있도록 GameInstance의 로컬 플레이어 기준
	UEnhancedInputLocalPlayerSubsystem* GetInputSubsystem() const;

	void ApplyInputMappings() const;

	// Definition에 정의된 IMC들을 등록 해제
	void RemoveInputMappings() const;

	void ClearSceneInput();

	// Persistent Level + 이 Scene 레벨의 ICXSceneSuspendable 액터에 알림
	void NotifySuspendables(bool bInSuspended) const;

	void DestroySpawnedActors();

private:
	// Scene 레벨에 배치된 Pawn 검색 -> 없으면 생성 (Enter 시 1회)
	void AcquireScenePawn();

	APawn* FindPawnInSceneLevels() const;

	// GameMode가 고른 PlayerStart 위치에 퍼시스턴트 레벨로 생성
	APawn* SpawnScenePawn() const;

	void PossessScenePawn();

	// Suspend 시: 서브레벨 Pawn이면 레벨이 Hidden 되기 전에 빙의 해제
	void ReleasePossessionBeforeHide();

	// Exit 시: 빙의 해제 + 생성한 Pawn은 Destroy
	void ReleaseScenePawn();

private:
	UPROPERTY()
	TObjectPtr<UCXSceneDefinition> Definition;

	// 유저가 이 Scene에서 조종할 Pawn
	UPROPERTY()
	TObjectPtr<APawn> ScenePawn;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> SpawnedActors;

	// ScenePawn을 이 Scene이 생성했는지 (생성한 Pawn은 Exit 때 Destroy, 레벨에 배치된 Pawn은 빙의만 해제)
	bool bSpawnedScenePawn = false;

	TArray<uint32> InputBindingHandles;

	// Scene의 HUD 약참조 (소유 객체는 루트 레이아웃의 Layer Stack)
	TWeakObjectPtr<UCXActivatableWidget> SceneHUD;
};
