package main

import (
    _ "fmt"
	"github.com/veandco/go-sdl2/sdl"
	"golang.org/x/exp/constraints"
)

const ScreenWidth = 800
const ScreenHeight = 600

const AppMaxTablesCount = 36

type App struct {
	Components  []Component
	Tables      [AppMaxTablesCount]Table
	TablesCount int
	PrevTicks   uint32
	Ticks       uint32
}

func (app *App) UpdateTicks() {
	app.PrevTicks = app.Ticks
	app.Ticks = sdl.GetTicks()
}

func (app *App) MsChange() uint32 {
	return app.Ticks - app.PrevTicks
}

type EventHandler interface {
	OnMouseButtonEvent(event *sdl.MouseButtonEvent, app *App) bool
	OnMouseMotionEvent(event *sdl.MouseMotionEvent, app *App)
}

type Renderer interface {
	Render(renderer *sdl.Renderer, app *App)
}

type Component interface {
	EventHandler
	Renderer
}

func pointInRect(x, y int32, r sdl.Rect) bool {
	return (r.X <= x && x <= r.X+r.W && r.Y <= y && y <= r.Y+r.H)
}

func Min[T constraints.Ordered](x, y T) T {
	if x < y {
		return x
	}
	return y
}

func Max[T constraints.Ordered](x, y T) T {
	if x > y {
		return x
	}
	return y
}

type ButtonAddTable struct {
	Rect             sdl.Rect
	DefaultColor     sdl.Color
	HoverColor       sdl.Color
	ActiveColor      sdl.Color
	Hover            bool
	TimeoutMs        uint32
	CurrentTimeoutMs uint32
}

func (button *ButtonAddTable) Render(renderer *sdl.Renderer, app *App) {
    if button.CurrentTimeoutMs < app.MsChange() {
        button.CurrentTimeoutMs = 0
    } else {
        button.CurrentTimeoutMs = button.CurrentTimeoutMs - app.MsChange()
    }

	if button.CurrentTimeoutMs > 0 {
		renderer.SetDrawColor(button.ActiveColor.R, button.ActiveColor.G, button.ActiveColor.B, 255)
	} else if button.Hover {
		renderer.SetDrawColor(button.HoverColor.R, button.HoverColor.G, button.HoverColor.B, 255)
	} else {
		renderer.SetDrawColor(button.DefaultColor.R, button.DefaultColor.G, button.DefaultColor.B, 255)
	}

	renderer.FillRect(&button.Rect)
	renderer.SetDrawColor(0, 0, 0, 255)
	renderer.DrawRect(&button.Rect)
}

func placeNewTable(newTable *Table, tables []Table) {
	newTable.Rect.X = 10
	newTable.Rect.Y = 50
	hasIntersections := true
	for hasIntersections {
		hasIntersections = false
		for _, table := range tables {
			if newTable.Rect.HasIntersection(&table.Rect) {
				hasIntersections = true
				newTable.Rect.Y = table.Rect.Y + table.Rect.H + 10
			}
		}
	}
}

func (button *ButtonAddTable) OnMouseButtonEvent(event *sdl.MouseButtonEvent, app *App) bool {
	if event.Button == sdl.BUTTON_LEFT && event.Type == sdl.MOUSEBUTTONDOWN {
		if pointInRect(event.X, event.Y, button.Rect) {
			button.CurrentTimeoutMs = button.TimeoutMs
			if app.TablesCount == AppMaxTablesCount {
				panic("Reached table count limit!")
			}
			newTable := Table{Rect: sdl.Rect{W: 150, H: 100}}
			placeNewTable(&newTable, app.Tables[:app.TablesCount])
			app.Tables[app.TablesCount] = newTable
			app.TablesCount++
			return true
		}
	}
	return false
}

func (button *ButtonAddTable) OnMouseMotionEvent(event *sdl.MouseMotionEvent, app *App) {
	button.Hover = pointInRect(event.X, event.Y, button.Rect)
}


const TableGrabHandleSize = 10

type Table struct {
	Rect              sdl.Rect
	Hover             bool
	Grabbed           bool
	GrabHandleVisible bool
	GrabHandle        sdl.Rect
}

func (table *Table) Render(renderer *sdl.Renderer, app *App) {
	renderer.SetDrawColor(0, 0, 0, 255)
	renderer.DrawRect(&table.Rect)
	if table.GrabHandleVisible {
		table.GrabHandle = sdl.Rect{
			X: table.Rect.X - (TableGrabHandleSize / 2),
			Y: table.Rect.Y - (TableGrabHandleSize / 2),
			W: TableGrabHandleSize,
			H: TableGrabHandleSize,
		}
		renderer.SetDrawColor(0, 0, 128, 255)
		renderer.FillRect(&table.GrabHandle)
	}
}

func (table *Table) pointInside(x, y int32) bool {
	return pointInRect(x, y, table.Rect) || (table.GrabHandleVisible && pointInRect(x, y, table.GrabHandle))
}

func (table *Table) OnMouseButtonEvent(event *sdl.MouseButtonEvent, app *App) bool {
	if event.Button == sdl.BUTTON_LEFT {
		if event.Type == sdl.MOUSEBUTTONDOWN {
			if table.GrabHandleVisible {
				table.Grabbed = pointInRect(event.X, event.Y, table.GrabHandle)
			}
		} else {
			table.Grabbed = false
		}
	}
	return table.Grabbed
}

func (table *Table) canMove(tables []Table, count int, dx, dy int32) bool {
	for i := 0; i < count; i++ {
		curr := tables[i]
		if table.Rect.X == curr.Rect.X && table.Rect.Y == curr.Rect.Y {
			continue
		}
		offset_rect := table.Rect
		offset_rect.X = offset_rect.X + dx
		offset_rect.Y = offset_rect.Y + dy
		if curr.Rect.HasIntersection(&offset_rect) {
			return false
		}
	}
	return true
}

func (table *Table) OnMouseMotionEvent(event *sdl.MouseMotionEvent, app *App) {
	if table.Grabbed && table.canMove(app.Tables[:], app.TablesCount, event.XRel, event.YRel) {
		table.Rect.X += event.XRel
		table.Rect.Y += event.YRel
		table.GrabHandle.X += event.XRel
		table.GrabHandle.Y += event.YRel
	}
	table.Hover = table.pointInside(event.X, event.Y)
	table.GrabHandleVisible = table.Hover
}

func sdlInit() (window *sdl.Window, renderer *sdl.Renderer) {
	var err error
	if err := sdl.Init(sdl.INIT_VIDEO | sdl.INIT_EVENTS); err != nil {
		panic(err)
	}
	window, err = sdl.CreateWindow(
		"test",
		sdl.WINDOWPOS_UNDEFINED, sdl.WINDOWPOS_UNDEFINED,
		ScreenWidth, ScreenHeight,
		sdl.WINDOW_SHOWN,
	)
	if err != nil {
		panic(err)
	}
	renderer, err = sdl.CreateRenderer(window, -1, sdl.RENDERER_ACCELERATED)
	if err != nil {
		panic(err)
	}
	return
}

func handleInput(app *App) bool {
	for _event := sdl.PollEvent(); _event != nil; _event = sdl.PollEvent() {
		switch event := _event.(type) {
		case *sdl.QuitEvent:
			return false
		case *sdl.KeyboardEvent:
			if event.Repeat != 0 {
				continue
			}
			if event.Type == sdl.KEYDOWN && event.Keysym.Scancode == sdl.SCANCODE_Q {
				return false
			}
		case *sdl.MouseButtonEvent:
			click_registered := false
			for i := range app.Components {
				click_registered = click_registered || app.Components[i].OnMouseButtonEvent(event, app)
			}
			for i := range app.Tables {
				click_registered = click_registered || app.Tables[i].OnMouseButtonEvent(event, app)
			}
		case *sdl.MouseMotionEvent:
			for i := range app.Components {
				app.Components[i].OnMouseMotionEvent(event, app)
			}
			for i := range app.Tables {
				app.Tables[i].OnMouseMotionEvent(event, app)
			}
		}
	}
	return true
}

func main() {
	window, renderer := sdlInit()
	defer sdl.Quit()
	defer window.Destroy()
	defer renderer.Destroy()

	app := App{
		Components: []Component{
			&ButtonAddTable{
				Rect:         sdl.Rect{X: 10, Y: 10, W: 50, H: 25},
				DefaultColor: sdl.Color{R: 255, G: 255, B: 255, A: 255},
				HoverColor:   sdl.Color{R: 128, G: 128, B: 128, A: 255},
				ActiveColor:  sdl.Color{R: 64, G: 64, B: 64, A: 255},
				TimeoutMs:    200,
			},
		},
	}

	for {
		app.UpdateTicks()

		if continue_ := handleInput(&app); !continue_ {
			break
		}

		renderer.SetDrawColor(255, 255, 255, 255)
		renderer.Clear()

		for _, button := range app.Components {
			button.Render(renderer, &app)
		}
		for i := 0; i < app.TablesCount; i++ {
			app.Tables[i].Render(renderer, &app)
		}

		renderer.Present()
		sdl.Delay(16)
	}
}
