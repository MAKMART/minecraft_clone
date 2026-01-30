#pragma once
class ECS;
struct Entity;
void movement_intent_system(ECS& ecs, const Entity& active_cam);
