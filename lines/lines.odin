package main

import rl "vendor:raylib"
import "core:math"
import "core:math/rand"
import "core:time"
import "core:fmt"

screenWidth: i32 = 1080
screenHeight: i32 = 720

Vector2 :: [2]f32

top_border :: proc() -> f32 { return -f32(screenHeight) / 2.0 }
bottom_border :: proc() -> f32 { return f32(screenHeight) / 2.0 }
left_border :: proc() -> f32 { return -f32(screenWidth) / 2.0 }
right_border :: proc() -> f32 { return f32(screenWidth) / 2.0 }

SHOW_FPS :: false
TARGET_FPS :: 144
SPEED :: 75
POINT_RADIUS :: 2
POINT_COLOR :: rl.WHITE
TRIGGER_RADIUS :: 150
N :: 100

MovePoint :: proc(point: ^Vector2, velocity: ^Vector2, dt: f32) {
    point.x += SPEED * dt * velocity.x
    point.x = clamp(point.x, left_border(), right_border())
    point.y += SPEED * dt * velocity.y
    point.y = clamp(point.y, top_border(), bottom_border())
    if right_border() <= point.x || point.x <= left_border() {
        velocity.x *= -1
    }
    if point.y <= top_border() || point.y >= bottom_border() {
        velocity.y *= -1
    }
}

Vector2Dist :: proc(v, w: Vector2) -> f32 {
    return math.sqrt((v.x - w.x) * (v.x - w.x) + (v.y - w.y) * (v.y - w.y))
}

GetLineColor :: proc(dist: f32) -> rl.Color {
    assert(dist < TRIGGER_RADIUS)
    m := 1 - (dist / TRIGGER_RADIUS)
    return {
        u8(f32(POINT_COLOR.r) * m),
        u8(f32(POINT_COLOR.g) * m),
        u8(f32(POINT_COLOR.b) * m),
        255,
    }
}

RefreshCameraOffset :: proc(camera: ^rl.Camera2D) {
    camera.offset = { f32(screenWidth) / 2.0, f32(screenHeight) / 2.0 }
}

main :: proc() {
	rl.SetConfigFlags({.WINDOW_RESIZABLE})
	rl.InitWindow(screenWidth, screenHeight, "Lines")
    rl.SetTargetFPS(TARGET_FPS)

    camera := rl.Camera2D {
        target = { 0, 0 },
        rotation = 0,
        zoom = 1,
    }
    RefreshCameraOffset(&camera)

    points := [N]Vector2{}
    velocities := [N]Vector2{}
    for i in 0..<N {
        velocities[i] = {
            rand.float32() * (-1 if rand.int31() % 2 == 0 else 1),
            rand.float32() * (-1 if rand.int31() % 2 == 0 else 1),
        }
        points[i] = {
            left_border() + (rand.float32() * (right_border() - left_border())),
            top_border() + (rand.float32() * (bottom_border() - top_border()))
        }
    }

    prev_time := time.now()
    for !rl.WindowShouldClose() {
        new_time := time.now()
        time_diff := f32(time.duration_seconds(time.diff(prev_time, new_time)))
        prev_time = new_time

        screenWidth = rl.GetScreenWidth()
        screenHeight = rl.GetScreenHeight()
        RefreshCameraOffset(&camera)

        for i in 0..<N do MovePoint(&points[i], &velocities[i], time_diff)

        rl.BeginDrawing()
            rl.ClearBackground(rl.BLACK)
            rl.BeginMode2D(camera)
                for p1, i1 in points {
                    for p2 in points[i1:] {
                        dist := Vector2Dist(p1, p2)
                        if dist < TRIGGER_RADIUS {
                            rl.DrawLine(
                                i32(p1.x), i32(p1.y), i32(p2.x), i32(p2.y),
                                GetLineColor(dist)
                            )
                        }
                    }
                }
                for i in 0..<N {
                    rl.DrawCircle(i32(points[i][0]), i32(points[i][1]), POINT_RADIUS, POINT_COLOR)
                }
            rl.EndMode2D()
            when SHOW_FPS do rl.DrawFPS(0, 0)
        rl.EndDrawing()
    }
    rl.CloseWindow()
}
