// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CXRopeTraversalActor.generated.h"

class USceneComponent;
class UBoxComponent;
class UPrimitiveComponent;

UENUM(BlueprintType)
enum class ECXRopeTraversalDirection :uint8
{
	None,
	Up,
	Down
};

UCLASS()
class CXVI_API ACXRopeTraversalActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACXRopeTraversalActor();

public:
	bool GetTraversalPoints(ECXRopeTraversalDirection Direction, FVector& OutEntry, FVector& OutDestination, FVector& OutExit) const;


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

protected:
	UFUNCTION()
	void HandleInteractionBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	void HandleInteractionEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
	);


protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rope")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rope")
	TObjectPtr<USceneComponent> BottomRopePoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rope")
	TObjectPtr<USceneComponent> TopRopePoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rope")
	TObjectPtr<USceneComponent> BottomExitPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rope")
	TObjectPtr<USceneComponent> TopExitPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rope")
	TObjectPtr<UBoxComponent> BottomInteractionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rope")
	TObjectPtr<UBoxComponent> TopInteractionBox;
};
