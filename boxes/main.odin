package main

import "core:fmt"
import rl "vendor:raylib"
import b2 "vendor:box2d"

TARGET_FPS :: 60
SHOW_FPS :: true

main :: proc() {
	rl.SetConfigFlags({.MSAA_4X_HINT})
	rl.InitWindow(1280, 720, "Lines")
    rl.SetTargetFPS(TARGET_FPS)

    camera := rl.Camera2D {
        target = { 0, -50 },
        rotation = 0,
        zoom = 6,
    }

    worldDef := b2.DefaultWorldDef()
    worldDef.gravity = { 0, 10.0 }
    worldId := b2.CreateWorld(worldDef)

    Ground :: struct {
        bodyId: b2.BodyId,
        pos: [2]f32,
        hx, hy: f32,
    }

    createGround :: proc(worldId: b2.WorldId, pos: [2]f32, hx, hy: f32) -> Ground {
        groundBodyDef := b2.DefaultBodyDef()
        groundBodyDef.position = pos
        groundId := b2.CreateBody(worldId, groundBodyDef)
        groundBox := b2.MakeBox(hx, hy)
        groundShapeDef := b2.DefaultShapeDef()
        groundShape := b2.CreatePolygonShape(groundId, groundShapeDef, groundBox)
        return { bodyId = groundId, pos = pos, hx = hx, hy = hy }
    }

    groundBoxes : [dynamic]Ground
    append(&groundBoxes, createGround(worldId, {0, -10}, 50, 2))
    append(&groundBoxes, createGround(worldId, {0, -60}, 50, 2))
    append(&groundBoxes, createGround(worldId, {-52, -34}, 2, 34))
    append(&groundBoxes, createGround(worldId, {52, -34}, 2, 17))

    createBox :: proc(worldId: b2.WorldId, pos: [2]f32) -> b2.BodyId {
        bodyDef := b2.DefaultBodyDef()
        bodyDef.type = .dynamicBody
        bodyDef.position = pos
        bodyId := b2.CreateBody(worldId, bodyDef)
        dynamicBox := b2.MakeBox(1, 1)
        shapeDef := b2.DefaultShapeDef()
        shapeDef.density = 1.0
        shapeDef.friction = 0.3
        boxShape := b2.CreatePolygonShape(bodyId, shapeDef, dynamicBox)
        return bodyId
    }

    drawBox :: proc(bodyId: b2.BodyId) {
        pos := b2.Body_GetPosition(bodyId)
        rot := b2.Rot_GetAngle(b2.Body_GetRotation(bodyId))
        angle := rot / (2 * b2.pi) * 360
        rl.DrawRectanglePro({pos.x, pos.y, 2, 2}, {1, 1}, angle, rl.WHITE)
    }

    boxes : [dynamic]b2.BodyId
    append(&boxes, createBox(worldId, {0, -30}))
    append(&boxes, createBox(worldId, {0.5, -35}))
    append(&boxes, createBox(worldId, {1, -40}))
    append(&boxes, createBox(worldId, {-0.5, -45}))
    append(&boxes, createBox(worldId, {-1, -50}))
    append(&boxes, createBox(worldId, {-1, -50}))

    for !rl.WindowShouldClose() {
        screenHeight, screenWidth := rl.GetScreenHeight(), rl.GetScreenWidth()
        camera.offset = { f32(screenWidth/2), f32(screenHeight/2) }

        if rl.IsMouseButtonDown(.LEFT) {
            screenPos := rl.GetMousePosition()
            pos := rl.GetScreenToWorld2D(screenPos, camera)
            b2.World_Explode(worldId, pos, 5, 5)
        }
        if rl.IsMouseButtonDown(.RIGHT) {
            screenPos := rl.GetMousePosition()
            pos := rl.GetScreenToWorld2D(screenPos, camera)
            append(&boxes, createBox(worldId, pos))
        }

        timeStep :: 1.0 / 60
        subStepCount :: 4
        b2.World_Step(worldId, timeStep, subStepCount)

        rl.BeginDrawing()
            rl.ClearBackground(rl.BLACK)
            rl.BeginMode2D(camera)

                for ground in groundBoxes {
                    rl.DrawRectangleRec(
                        {
                            ground.pos.x-ground.hx,
                            ground.pos.y-ground.hy,
                            2*ground.hx,
                            2*ground.hy
                        },
                        rl.GRAY
                    )
                }

                for boxId in boxes {
                    drawBox(boxId)
                }

            rl.EndMode2D()
            when SHOW_FPS do rl.DrawFPS(0, 0)
        rl.EndDrawing()
    }
    rl.CloseWindow()
}
