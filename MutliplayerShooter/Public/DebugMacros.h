#pragma once

#include "Engine/Engine.h"
#include "DrawDebugHelpers.h"

#define GEPR_R(T_fstring) if (GEngine) { GEngine->AddOnScreenDebugMessage(-1, 7.5f, FColor::Red, T_fstring); }
#define GEPR_G(T_fstring) if (GEngine) { GEngine->AddOnScreenDebugMessage(-1, 7.5f, FColor::Green, T_fstring); }
#define GEPR_B(T_fstring) if (GEngine) { GEngine->AddOnScreenDebugMessage(-1, 7.5f, FColor::Blue, T_fstring); }
#define GEPR_Y(T_fstring) if (GEngine) { GEngine->AddOnScreenDebugMessage(-1, 7.5f, FColor::Yellow, T_fstring); }
#define GEPR_W(T_fstring) if (GEngine) { GEngine->AddOnScreenDebugMessage(-1, 7.5f, FColor::White, T_fstring); }

#define GEPRS_R(T_string) if (GEngine) { GEngine->AddOnScreenDebugMessage(-1, 7.5f, FColor::Red, TEXT(T_string)); }
#define GEPRS_G(T_string) if (GEngine) { GEngine->AddOnScreenDebugMessage(-1, 7.5f, FColor::Green, TEXT(T_string)); }
#define GEPRS_B(T_string) if (GEngine) { GEngine->AddOnScreenDebugMessage(-1, 7.5f, FColor::Blue, TEXT(T_string)); }
#define GEPRS_Y(T_string) if (GEngine) { GEngine->AddOnScreenDebugMessage(-1, 7.5f, FColor::Yellow, TEXT(T_string)); }
#define GEPRS_W(T_string) if (GEngine) { GEngine->AddOnScreenDebugMessage(-1, 7.5f, FColor::White, TEXT(T_string)); }

#define GEPR_ONE_VARIABLE(T_string, variable)\
if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 70.5f, FColor::White, *FString::Printf(TEXT(T_string), variable));

#define GEPR_TWO_VARIABLE(T_string, variable1, variable2)\
if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 70.5f, FColor::White, \
*FString::Printf(TEXT(T_string), variable1, variable2));

#define GEPR_THREE_VARIABLE(T_string, variable1, variable2, variable3)\
if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 70.5f, FColor::White, \
*FString::Printf(TEXT(T_string), variable1, variable2, variable3));



#define DRAW_SPHERE_SHORT_B(location) DrawDebugSphere(GetWorld(), location, 12.f, 12.f, FColor::Black, false, 0.05f);
#define DRAW_SPHERE_SHORT_W(location) DrawDebugSphere(GetWorld(), location, 12.f, 12.f, FColor::White, false, 0.05f);
#define DRAW_SPHERE_SHORT_R(location) DrawDebugSphere(GetWorld(), location, 12.f, 12.f, FColor::Red, false, 0.05f);

#define DRAW_SPHERE_B(location) DrawDebugSphere(GetWorld(), location, 12.f, 12.f, FColor::Black, false, 15.f);
#define DRAW_SPHERE_BIG_B(location) DrawDebugSphere(GetWorld(), location, 36.f, 12.f, FColor::Black, false, 15.f);
#define DRAW_SPHERE_BIG_R(location) DrawDebugSphere(GetWorld(), location, 36.f, 12.f, FColor::Red, false, 15.f);
#define DRAW_SPHERE_W(location) DrawDebugSphere(GetWorld(), location, 12.f, 12.f, FColor::White, false, 15.f);
#define DRAW_SPHERE_R(location) DrawDebugSphere(GetWorld(), location, 12.f, 12.f, FColor::Red, false, 15.f);
#define DRAW_SPHERE_B_COMP(location, owner) if(owner) DrawDebugSphere(owner->GetWorld(), location, 12.f, 12.f, FColor::Black, false, 120.f);
#define DRAW_SPHERE_R_COMP(location, owner) if(owner) DrawDebugSphere(owner->GetWorld(), location, 12.f, 12.f, FColor::Red, false, 120.f);



#define DRAW_POINT(location) DrawDebugPoint(GetWorld(), location, 12.f, FColor::Red, false, 120.f);
#define DRAW_POINT_O(location) DrawDebugPoint(GetWorld(), location, 12.f, FColor::Orange, false, 120.f);
#define DRAW_POINT_B_SMALL(location) DrawDebugPoint(GetWorld(), location, 6.f, FColor::Blue, false, 120.f);
#define DRAW_POINT_ONE_FRAME(location) DrawDebugPoint(GetWorld(), location, 12.f, FColor::Red, false, .001f);


#define DRAW_LINE_COMP(ownerActor, start, end) if(ownerActor) DrawDebugLine(ownerActor->GetWorld(), start, end, FColor::Red, false, 120.f, 0U, 2.f)
#define DRAW_LINE_COMP_O(ownerActor, start, end) if(ownerActor) DrawDebugLine(ownerActor->GetWorld(), start, end, FColor::Orange, false, 120.f, 0U, 2.f)
#define DRAW_LINE_COMP_BLUE(ownerActor, start, end) if(ownerActor) DrawDebugLine(ownerActor->GetWorld(), start, end, FColor::Blue, false, 120.f, 0U, 2.f)
#define DRAW_LINE_COMP_BLACK(ownerActor, start, end) if(ownerActor) DrawDebugLine(ownerActor->GetWorld(), start, end, FColor::Black, false, 120.f, 0U, 2.f)
#define DRAW_LINE_COMP_G(ownerActor, start, end) if(ownerActor) DrawDebugLine(ownerActor->GetWorld(), start, end, FColor::Green, false, 120.f, 0U, 2.f)



#define DRAW_FRAME_PACKAGE_COMP(framePackage, drawColor, ownerActor) \
if (ownerActor) {  \
for (auto& boxPair : framePackage.hitBoxInfo) \
{ \
	DrawDebugBox(ownerActor->GetWorld(), \
		boxPair.Value.location, boxPair.Value.boxExtent, boxPair.Value.rotation.Quaternion(), drawColor, false, 120.f); \
}}

#define DRAW_FRAME_PACKAGE_R(framePackage, ownerActor) \
if (ownerActor) {  \
for (auto& boxPair : framePackage.hitBoxInfo) \
{ \
	DrawDebugBox(ownerActor->GetWorld(), \
		boxPair.Value.location, boxPair.Value.boxExtent, boxPair.Value.rotation.Quaternion(), FColor::Red, false, 120.f); \
}}

#define DRAW_FRAME_PACKAGE_G(framePackage, ownerActor) \
if (ownerActor) {   \
for (auto& boxPair : framePackage.hitBoxInfo) \
{ \
	DrawDebugBox(ownerActor->GetWorld(), \
		boxPair.Value.location, boxPair.Value.boxExtent, boxPair.Value.rotation.Quaternion(), FColor::Green, false, 120.f); \
}}






#define DRAW_FRAME_PACKAGE(framePackage, drawColor) \
for (auto& boxPair : framePackage.hitBoxInfo) \
{ \
	DrawDebugBox(GetWorld(), \
		boxPair.Value.location, boxPair.Value.boxExtent, boxPair.Value.rotation.Quaternion(), drawColor, false, 120.f); \
}






