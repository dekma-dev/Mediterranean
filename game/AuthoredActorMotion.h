#pragma once
#include "engine/AuthoredAssets.h"
#include "game/Animation.h"

// Reuse the accepted action clock, inertial transitions and impact response.
// Do not start a second animation timeline from Render or from windup guesses.
inline authored::Gesture AuthoredActorGesture(const animation::Sample& sample) {
 const auto& a=sample.action;
 authored::Gesture result;
 result.pitch=a.pitch+sample.lean*.10f-sample.hit*.15f;
 result.yaw=a.yaw+sample.hitSide*.07f;
 result.roll=a.roll+sample.bank*.10f+sample.hitSide*.12f;
 result.hip=a.hip;
 result.leftLift=a.left.y*1.5f+a.left.z*.8f;
 result.rightLift=a.right.y*1.5f+a.right.z*.8f;
 result.leftOpen=a.left.x+a.open*.12f;
 result.rightOpen=-a.right.x+a.open*.08f;
 return result;
}
