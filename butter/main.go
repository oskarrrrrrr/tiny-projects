package main

import (
	"github.com/veandco/go-sdl2/sdl"
	"github.com/veandco/go-sdl2/ttf"
	"golang.org/x/exp/constraints"
)

const WindowDefaultWidth = 800
const WindowDefaultHeight = 600
const WindowMinWidth = 400
const WindowMinHeight = 200

const TopBarHeight = 55
const ViewPortSideMargin = 10
const ViewPortVerticalMargin = 5

const AppMaxTablesCount = 36

type App struct {
	Window   *sdl.Window
	Renderer *sdl.Renderer

	ViewPort   ViewPort
	Components []Component
	PrevTicks  uint32
	Ticks      uint32
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

func Abs[T constraints.Integer](x T) T {
	if x > 0 {
		return x
	}
	return -x
}

type ButtonAddTable struct {
	Rect             sdl.Rect
	DefaultColor     sdl.Color
	HoverColor       sdl.Color
	ActiveColor      sdl.Color
	Active           bool
	Hover            bool
	TimeoutMs        uint32
	CurrentTimeoutMs uint32
	Font             *ttf.Font
	FontSurface      *sdl.Surface
}

func (button *ButtonAddTable) Destroy() {
	if button.FontSurface != nil {
		button.FontSurface.Free()
	}
}

func (button *ButtonAddTable) Render(renderer *sdl.Renderer, app *App) {
	if button.CurrentTimeoutMs < app.MsChange() {
		button.CurrentTimeoutMs = 0
	} else {
		button.CurrentTimeoutMs = button.CurrentTimeoutMs - app.MsChange()
	}

	fillColor := button.DefaultColor
	if button.Active || button.CurrentTimeoutMs > 0 {
		fillColor = button.ActiveColor
	} else if button.Hover {
		fillColor = button.HoverColor
	}
	renderer.SetDrawColor(fillColor.R, fillColor.G, fillColor.B, fillColor.A)
	renderer.FillRect(&button.Rect)

	// draw inside of the button
	outlineColor := sdl.Color{R: 0, G: 0, B: 0, A: 255}
	if button.Active || button.CurrentTimeoutMs > 0 {
		outlineColor = sdl.Color{R: 255, G: 255, B: 255, A: 255}
	}
	renderer.SetDrawColor(outlineColor.R, outlineColor.G, outlineColor.B, outlineColor.A)
	xMargin := float32(button.Rect.W) * 0.2
	yMargin := float32(button.Rect.H) * 0.2
	smallRect := sdl.Rect{
		X: button.Rect.X + int32(xMargin),
		Y: button.Rect.Y + int32(yMargin),
		W: button.Rect.W - 2*int32(xMargin),
		H: button.Rect.H - 2*int32(yMargin),
	}
	renderer.DrawRect(&smallRect)
	// vertical line
	renderer.DrawLine(smallRect.X+smallRect.W/2, smallRect.Y, smallRect.X+smallRect.W/2, smallRect.Y+smallRect.H-1)
	// horizontal line
	renderer.DrawLine(smallRect.X, smallRect.Y+smallRect.H/2, smallRect.X+smallRect.W-1, smallRect.Y+smallRect.H/2)

	if button.Font == nil {
		button.Font, _ = TtfOpenFont(app.Window, app.Renderer, "assets/Lato/Lato-Regular.ttf", 10)
		button.FontSurface, _ = button.Font.RenderUTF8Blended("Add Table", sdl.Color{R: 0, G: 0, B: 0, A: 255})
	}
	textTexture, _ := renderer.CreateTextureFromSurface(button.FontSurface)
	xs, _ := GetWindowScale(app.Window, app.Renderer)
	RendererCopy(
		app.Window,
		app.Renderer,
		textTexture,
		nil,
		&sdl.Rect{
			X: button.Rect.X - ((int32(float32(button.FontSurface.W)/xs) - button.Rect.W) / 2),
			Y: button.Rect.Y + button.Rect.H,
			W: button.FontSurface.W,
			H: button.FontSurface.H,
		},
	)
}

func placeNewTable(newTable *Table, tables []Table) {
	newTable.Rect.X = 0
	newTable.Rect.Y = 0
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
	if event.Button == sdl.BUTTON_LEFT {
		if event.Type == sdl.MOUSEBUTTONDOWN {
			if pointInRect(event.X, event.Y, button.Rect) {
				button.Active = true
				return true
			}
		} else if event.Type == sdl.MOUSEBUTTONUP {
			button.Active = false
			if button.Hover {
				button.CurrentTimeoutMs = button.TimeoutMs
				if app.ViewPort.TablesCount == AppMaxTablesCount {
					panic("Reached table count limit!")
				}
				newTable := Table{Rect: sdl.Rect{W: 150, H: 100}, ViewPort: &app.ViewPort}
				placeNewTable(&newTable, app.ViewPort.Tables[:app.ViewPort.TablesCount])
				app.ViewPort.Tables[app.ViewPort.TablesCount] = newTable
				app.ViewPort.TablesCount++
				return true
			}
		}
	}
	return false
}

func (button *ButtonAddTable) OnMouseMotionEvent(event *sdl.MouseMotionEvent, app *App) {
	button.Hover = pointInRect(event.X, event.Y, button.Rect)
}

const TableGrabHandleSize = 10

type Table struct {
	ViewPort          *ViewPort
	Rect              sdl.Rect
	Hover             bool
	Grabbed           bool
	GrabHandleVisible bool
	GrabHandle        sdl.Rect
}

func (table *Table) Render(viewPort *ViewPort, renderer *sdl.Renderer) {
	rect := sdl.Rect{
		X: viewPort.Rect.X + table.Rect.X + viewPort.HorizontalScroll,
		Y: viewPort.Rect.Y + table.Rect.Y + viewPort.VerticalScroll,
		W: table.Rect.W,
		H: table.Rect.H,
	}
	rect, rectNonEmpty := rect.Intersect(&viewPort.Rect)
	if !rectNonEmpty {
		return
	}
	renderer.SetDrawColor(0, 0, 0, 255)
	renderer.DrawRect(&rect)

	if table.GrabHandleVisible {
		table.GrabHandle = sdl.Rect{
			X: table.ViewPort.Rect.X + table.Rect.X - (TableGrabHandleSize / 2) + viewPort.HorizontalScroll,
			Y: table.ViewPort.Rect.Y + table.Rect.Y - (TableGrabHandleSize / 2) + viewPort.VerticalScroll,
			W: TableGrabHandleSize,
			H: TableGrabHandleSize,
		}
		rect, rectNonEmpty := table.GrabHandle.Intersect(&table.ViewPort.Rect)
		if rectNonEmpty {
			renderer.SetDrawColor(0, 0, 128, 255)
			renderer.FillRect(&rect)
		}
	}
}

func (table *Table) pointInside(x, y int32) bool {
	rect := sdl.Rect{
		X: table.ViewPort.Rect.X + table.Rect.X + table.ViewPort.HorizontalScroll,
		Y: table.ViewPort.Rect.Y + table.Rect.Y + table.ViewPort.VerticalScroll,
		H: table.Rect.H,
		W: table.Rect.W,
	}
	return pointInRect(x, y, rect) || (table.GrabHandleVisible && pointInRect(x, y, table.GrabHandle))
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
	if table.Rect.X+dx < 0 || table.Rect.Y+dy < 0 {
		return false
	}
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
	if table.Grabbed && table.canMove(app.ViewPort.Tables[:], app.ViewPort.TablesCount, event.XRel, event.YRel) {
		table.Rect.X += event.XRel
		table.Rect.Y += event.YRel
		table.GrabHandle.X += event.XRel
		table.GrabHandle.Y += event.YRel
	}
	table.Hover = table.pointInside(event.X, event.Y)
	table.GrabHandleVisible = table.Hover
}

const (
	// Extra space that can be seen after the last element on the right
	ViewPortRightScrollMargin = 100
	// Extra space that can be seen after the bottom most element
	ViewPortBottomScrollMargin = 50
	// Height of the horizontal scroll bar and width of the vertical scroll bar
	ViewPortScrollBarDim = 10
	// Delay after which the scroll bars will be hidden
	ViewPortScrollBarDisplayMs = 1000
	ViewPortScrollSpeed        = 10
)

// ViewPort coordinates begin at (0, 0) in the top left corner and all the visible coordinates are positive.
// Meaning, it's impossible to scroll left or up from the default view, for example, to see the point (-1, 0).
type ViewPort struct {
	Rect        sdl.Rect
	Tables      [AppMaxTablesCount]Table
	TablesCount int
	// Time left in ms to show the vertical scroll bar
	ShowVerticalScrollBar int
	VerticalScroll        int32
	// Time left in ms to show the horizontal scroll bar
	ShowHorizontalScrollBar int
	HorizontalScroll        int32
}

// MaxUsedX returns the x coordinate of the rightmost point of the rightmost element on the viewPort
func (viewPort *ViewPort) MaxUsedX() int32 {
	maxX := int32(0)
	for i := 0; i < viewPort.TablesCount; i++ {
		t := viewPort.Tables[i].Rect
		maxX = Max(t.X+t.W, maxX)
	}
	return maxX
}

func (viewPort *ViewPort) MaxViewableX() int32 {
	return Max(viewPort.MaxUsedX()+ViewPortRightScrollMargin, viewPort.Rect.W)
}

// Scroll is a negative number so a maximum scroll is actually the smallest number that is a valid scroll.
func (viewPort *ViewPort) MaxScrollX() int32 {
	return viewPort.Rect.W - viewPort.MaxViewableX()
}

// Scroll is a negative number so a maximum scroll is actually the smallest number that is a valid scroll.
func (viewPort *ViewPort) MaxScrollY() int32 {
	return viewPort.Rect.H - viewPort.MaxViewableY()
}

// MaxUsedY returns the y coordinate of the lowest point of the lowest element on the viewPort
func (viewPort *ViewPort) MaxUsedY() int32 {
	maxY := int32(0)
	for i := 0; i < viewPort.TablesCount; i++ {
		t := viewPort.Tables[i].Rect
		maxY = Max(t.Y+t.H, maxY)
	}
	return maxY
}

func (viewPort *ViewPort) MaxViewableY() int32 {
	return Max(viewPort.MaxUsedY()+ViewPortBottomScrollMargin, viewPort.Rect.H)
}

func (viewPort *ViewPort) Render(renderer *sdl.Renderer, app *App) {
	renderer.SetDrawColor(0, 0, 0, 255)
	renderer.DrawRect(&viewPort.Rect)

	viewPort.ShowVerticalScrollBar = Max(viewPort.ShowVerticalScrollBar-int(app.MsChange()), 0)
	viewPort.ShowHorizontalScrollBar = Max(viewPort.ShowHorizontalScrollBar-int(app.MsChange()), 0)

	renderer.SetDrawColor(96, 96, 96, 172)
	renderer.SetDrawBlendMode(sdl.BLENDMODE_BLEND)

	if viewPort.ShowHorizontalScrollBar > 0 {
		viewRatio := float32(viewPort.Rect.W) / float32(viewPort.MaxViewableX())
		renderer.FillRect(&sdl.Rect{
			X: viewPort.Rect.X - int32(float32(viewPort.HorizontalScroll)*viewRatio),
			Y: viewPort.Rect.Y + viewPort.Rect.H - ViewPortScrollBarDim,
			W: int32(float32(viewPort.Rect.W) * viewRatio),
			H: ViewPortScrollBarDim,
		})
	}

	if viewPort.ShowVerticalScrollBar > 0 {
		viewRatio := float32(viewPort.Rect.H) / float32(viewPort.MaxViewableY())
		renderer.FillRect(&sdl.Rect{
			X: viewPort.Rect.X + viewPort.Rect.W - ViewPortScrollBarDim,
			Y: viewPort.Rect.Y - int32(float32(viewPort.VerticalScroll)*viewRatio),
			W: ViewPortScrollBarDim,
			H: int32(float32(viewPort.Rect.H) * viewRatio),
		})
	}
}

func (viewPort *ViewPort) OnWindowResize(event *sdl.WindowEvent, app *App) {
	w, h := app.Window.GetSize()
	viewPort.Rect.W = w - 2*ViewPortSideMargin
	viewPort.Rect.H = h - 2*ViewPortVerticalMargin - TopBarHeight
}

func (viewPort *ViewPort) OnMouseWheel(event *sdl.MouseWheelEvent) {
	// scroll only in one direction at a time
	if Abs(event.X) > Abs(event.Y) {
		viewPort.ShowHorizontalScrollBar = ViewPortScrollBarDisplayMs
		viewPort.ShowVerticalScrollBar = 0
		// disallow scrolling beyond into negative coordinates
		newHorizontalScroll := Min(viewPort.HorizontalScroll-ViewPortScrollSpeed*event.X, 0)
		if newHorizontalScroll < viewPort.MaxScrollX() {
			viewPort.HorizontalScroll = viewPort.MaxScrollX()
		} else {
			viewPort.HorizontalScroll = newHorizontalScroll
		}
	} else if Abs(event.X) < Abs(event.Y) {
		viewPort.ShowHorizontalScrollBar = 0
		viewPort.ShowVerticalScrollBar = ViewPortScrollBarDisplayMs
		// disallow scrolling beyond into negative coordinates
		newVerticalScroll := Min(viewPort.VerticalScroll+ViewPortScrollSpeed*event.Y, 0)
		if newVerticalScroll < viewPort.MaxScrollY() {
			viewPort.VerticalScroll = viewPort.MaxScrollY()
		} else {
			viewPort.VerticalScroll = newVerticalScroll
		}
	}
}

func GetWindowScale(window *sdl.Window, renderer *sdl.Renderer) (xs, ys float32) {
	xs, ys = 1, 1
	windowWidth, windowHeight := window.GetSize()
	rendererWidth, rendererHeight, _ := renderer.GetOutputSize()
	if rendererWidth != windowWidth {
		xs = float32(rendererWidth) / float32(windowWidth)
		ys = float32(rendererHeight) / float32(windowHeight)
	}
	return
}

func sdlInit() (window *sdl.Window, renderer *sdl.Renderer) {
	var err error
	if err := sdl.Init(sdl.INIT_VIDEO | sdl.INIT_EVENTS); err != nil {
		panic(err)
	}
	window, err = sdl.CreateWindow(
		"Butter",
		sdl.WINDOWPOS_UNDEFINED, sdl.WINDOWPOS_UNDEFINED,
		WindowDefaultWidth, WindowDefaultHeight,
		sdl.WINDOW_SHOWN|sdl.WINDOW_ALLOW_HIGHDPI|sdl.WINDOW_RESIZABLE,
	)
	if err != nil {
		panic(err)
	}
	window.SetMinimumSize(WindowMinWidth, WindowMinHeight)

	renderer, err = sdl.CreateRenderer(window, -1, sdl.RENDERER_ACCELERATED)
	if err != nil {
		panic(err)
	}

	xs, ys := GetWindowScale(window, renderer)
	if xs != 1 {
		renderer.SetScale(xs, ys)
	}

	ttf.Init()
	return
}

func handleEvents(app *App) bool {
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
			for i := range app.ViewPort.Tables {
				click_registered = click_registered || app.ViewPort.Tables[i].OnMouseButtonEvent(event, app)
			}
		case *sdl.MouseMotionEvent:
			for i := range app.Components {
				app.Components[i].OnMouseMotionEvent(event, app)
			}
			for i := 0; i < app.ViewPort.TablesCount; i++ {
				app.ViewPort.Tables[i].OnMouseMotionEvent(event, app)
			}
		case *sdl.MouseWheelEvent:
			app.ViewPort.OnMouseWheel(event)
		case *sdl.WindowEvent:
			switch event.Event {
			case sdl.WINDOWEVENT_SIZE_CHANGED, sdl.WINDOWEVENT_SHOWN:
				app.ViewPort.OnWindowResize(event, app)
			}
		}
	}
	return true
}

func RendererCopy(window *sdl.Window, renderer *sdl.Renderer, texture *sdl.Texture, src *sdl.Rect, dst *sdl.Rect) {
	xs, ys := GetWindowScale(window, renderer)
	dst.W = int32(float32(dst.W) / xs)
	dst.H = int32(float32(dst.H) / ys)
	renderer.Copy(texture, src, dst)
}

func TtfOpenFont(window *sdl.Window, renderer *sdl.Renderer, fontFileName string, size uint32) (*ttf.Font, error) {
	xs, _ := GetWindowScale(window, renderer)
	return ttf.OpenFont(fontFileName, int(float32(size)*xs))
}

func main() {
	window, renderer := sdlInit()
	defer sdl.Quit()
	defer window.Destroy()
	defer renderer.Destroy()
	defer ttf.Quit()

	buttonAddTable := ButtonAddTable{
		Rect:         sdl.Rect{X: 10, Y: 10, W: 35, H: 25},
		DefaultColor: sdl.Color{R: 255, G: 255, B: 255, A: 255},
		HoverColor:   sdl.Color{R: 128, G: 128, B: 128, A: 255},
		ActiveColor:  sdl.Color{R: 64, G: 64, B: 64, A: 255},
		TimeoutMs:    200,
	}

	app := App{
		Window:     window,
		Renderer:   renderer,
		ViewPort:   ViewPort{Rect: sdl.Rect{X: 10, Y: 55, W: 200, H: 200}},
		Components: []Component{&buttonAddTable},
	}

	for {
		app.UpdateTicks()

		if continue_ := handleEvents(&app); !continue_ {
			break
		}

		renderer.SetDrawColor(255, 255, 255, 255)
		renderer.Clear()

		app.ViewPort.Render(renderer, &app)

		for _, button := range app.Components {
			button.Render(renderer, &app)
		}
		for i := 0; i < app.ViewPort.TablesCount; i++ {
			app.ViewPort.Tables[i].Render(&app.ViewPort, renderer)
		}

		renderer.Present()
		sdl.Delay(16) // cap at 60fps
	}

	buttonAddTable.Destroy()
}
