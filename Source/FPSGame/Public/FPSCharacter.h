// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "FPSGame.h"
#include "Components/InputComponent.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/Character.h"
#include "FPSCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class AFPSProjectile;
class USoundBase;
class UAnimSequence;


UCLASS()
class AFPSCharacter : public ACharacter
{
	GENERATED_BODY()

protected:
	/* -------------- */
	/*   Components   */
	/* -------------- */

	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category = "Mesh")
		class USkeletalMeshComponent* Mesh1P;

	/** Pawn mesh: 3rd person view (completed body; seen only by others) */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Mesh")
		class USkeletalMeshComponent* Mesh3P;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
		class UCameraComponent* FirstPersonCameraComponent;

	/** Gun mesh: 1st person view (seen only by self) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
		USkeletalMeshComponent* GunMesh1P;

	/** Gun mesh: 3st person view (seen only by others) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
		USkeletalMeshComponent* GunMesh3P;

	/** Timeline used to smoothly adjust the camera height when crouching */
	UPROPERTY()
		UTimelineComponent* CrouchingTimeline;

public:
	/* ---------------------- */
	/*    Editor Settings     */
	/* ---------------------- */

	/** Determines the maximum walk speed when sprinting */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)
		int SprintSpeed = 900;

	/** Determines the maximum walk speed when walking normaly */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement)
		int WalkSpeed = 600;

	/** Used by CrouchingTimeline to adjust the camera height when crouching */
	UPROPERTY(EditAnywhere, Category = Materialize)
		UCurveFloat* CrouchingCurve;

	/** Projectile class to spawn */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
		TSubclassOf<AFPSProjectile> ProjectileClass;

	/** Sound to play each time we fire */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
		USoundBase* FireSound;

	/** AnimMontage to play each time we fire */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
		UAnimSequence* FireAnimation;
public: 

	UPROPERTY(BlueprintReadWrite)
		bool IsAiming = false;

public:
	/* ------------- */
	/*   Functions   */
	/* ------------- */

	AFPSCharacter();

	/** Setup component attachments */
	virtual void BeginPlay();

	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	
	/** Fires a projectile. */
	void Fire();

	/** Callback for CrouchingTimeline, sets the camera height based on CamStart and CamFinish */
	UFUNCTION()
		void CrouchingCallback(float value);

	/** Enables sprint and starts sprinting if all sprinting conditions are met */
	UFUNCTION(BlueprintCallable)
		void EnableSprint();

	/** Disables sprint, stops sprinting if currently sprinting */
	UFUNCTION(BlueprintCallable)
		void DisableSprint();

	/** Returns true if the player is not sprinting and should start sprinting */
	bool ShouldStartSprinting(float ForwardMovement);

	/** Returns true if the player is sprinting and should stop sprinting */
	bool ShouldStopSprinting(float ForwardMovement);

	/** Starts sprinting (increases max speed) */
	void StartSprinting();

	/** Stops sprinting (decreases max speed) */
	void StopSprinting();

	/** Handles moving forward/backward */
	UFUNCTION(BlueprintCallable)
		void MoveForward(float val);

	/** Handles stafing movement, left and right */
	void MoveRight(float val);

	/**
	* Called via input to turn at a given rate.
	* @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	*/
	void TurnAtRate(float rate);

	/**
	* Called via input to turn look up/down at a given rate.
	* @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	*/
	void LookUpAtRate(float rate);

	/** Jumps if the player is standing, uncrouches if the player is crouched */
	virtual void Jump() override;

	/** Toggles between crouching/standing */
	void ToggleCrouching();

	/** Overriden from ACharacter, fired on server and clients */
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

public:
	/** Returns Mesh1P subobject **/
	USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	
	/** Returns Mesh2P subobject **/
	USkeletalMeshComponent* GetMesh2P() const { return Mesh1P; }

	/** Returns FirstPersonCameraComponent subobject **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

private:
	/** True if the player has pressed shift and wants to sprint */
	bool sprintEnabled = false;

	/** False if the player is not allowed to sprint (e.g. when firing a weapon) */
	bool sprintAllowed = true;

	// ### Crouch animation variables
	// Original camera location
	FVector CamMiddle;
	// Camera location at the top of the capsule
	FVector CamTop;
	// Camera location at the bottom of the capsule
	FVector CamBottom;
	// Start location for the CrouchingTimeline
	FVector CamStart;
	// Finish location for the CrouchingTimeline
	FVector CamFinish;
	// Capsule's half height difference between standing/crouching
	float CrouchHeightDiff;

	bool wantsToCrouch = false;
};

