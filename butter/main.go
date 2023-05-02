package main

import (
	"github.com/veandco/go-sdl2/sdl"
	"github.com/veandco/go-sdl2/ttf"
	"golang.org/x/exp/constraints"
	"strings"
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

	TextBox    TextBox
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

type TextBox struct {
	Rect   sdl.Rect
	Text   *string
	Active bool
	// cursor
	CursorVisible          bool
	CursorBlinkTimeCounter uint32
	CursorBlinkSpeedMs     uint32
}

func (textBox *TextBox) Clear() *string {
	oldText := textBox.Text
	textBox.Text = new(string)
	return oldText
}

func (textBox *TextBox) Render(renderer *sdl.Renderer, app *App) {
	// update cursor
	textBox.CursorBlinkTimeCounter += app.MsChange()
	if textBox.CursorBlinkTimeCounter > textBox.CursorBlinkSpeedMs {
		textBox.CursorBlinkTimeCounter = 0
		textBox.CursorVisible = !textBox.CursorVisible
	}

	renderer.SetDrawColor(0, 0, 0, 255)
	renderer.DrawRect(&textBox.Rect)

	xs, ys := GetWindowScale(app.Window, renderer)

	font, _ := TtfOpenFont(app.Window, app.Renderer, "assets/Lato/Lato-Regular.ttf", 12)
	promptSurface, _ := font.RenderUTF8Blended("> ", sdl.Color{R: 0, G: 0, B: 0, A: 255})
	promptTexture, _ := renderer.CreateTextureFromSurface(promptSurface)
	promptRect := sdl.Rect{
		X: textBox.Rect.X + 5,
		Y: textBox.Rect.Y + (textBox.Rect.H-int32(float32(promptSurface.H)/ys))/2,
		W: promptSurface.W,
		H: promptSurface.H,
	}
	RendererCopy(
		app.Window,
		app.Renderer,
		promptTexture,
		nil,
		&promptRect,
	)

	if textBox.Active {
		if textBox.Text != nil && *textBox.Text != "" {
			// render text
			fontSurface, _ := font.RenderUTF8Blended(*textBox.Text, sdl.Color{R: 0, G: 0, B: 0, A: 255})
			textTexture, _ := renderer.CreateTextureFromSurface(fontSurface)
			textRect := sdl.Rect{
				X: promptRect.X + int32(float32(promptRect.W)/xs),
				Y: textBox.Rect.Y + (textBox.Rect.H-int32(float32(fontSurface.H)/ys))/2,
				W: fontSurface.W,
				H: fontSurface.H,
			}
			RendererCopy(
				app.Window,
				app.Renderer,
				textTexture,
				nil,
				&textRect,
			)
			if textBox.CursorVisible {
				renderer.SetDrawColor(0, 0, 0, 255)
				x := textRect.X + int32(float32(textRect.W)/xs) + 1
				renderer.DrawLine(x, textRect.Y, x, textRect.Y+int32(float32(textRect.H)/ys))
			}
		} else {
			if textBox.CursorVisible {
				renderer.SetDrawColor(0, 0, 0, 255)
				x := promptRect.X + int32(float32(promptRect.W)/xs) + 1
				renderer.DrawLine(x, promptRect.Y, x, promptRect.Y+int32(float32(promptRect.H)/ys))
			}
		}
	} else {
		renderer.SetDrawColor(96, 96, 96, 122)
		renderer.SetDrawBlendMode(sdl.BLENDMODE_BLEND)
		renderer.FillRect(&textBox.Rect)
	}
}

func (textBox *TextBox) OnMouseButtonEvent(event *sdl.MouseButtonEvent) {
	textBox.Active = pointInRect(event.X, event.Y, textBox.Rect)
}

func (textBox *TextBox) OnKeyPress(event *sdl.KeyboardEvent) {
	if event.Type == sdl.KEYUP {
		return
	}

	if event.Keysym.Scancode == sdl.SCANCODE_BACKSPACE {
		if textBox.Text != nil && *textBox.Text != "" {
			*textBox.Text = (*textBox.Text)[:len(*textBox.Text)-1]
		}
		return
	}

	if event.Keysym.Scancode == sdl.SCANCODE_SPACE {
		var s string
		if textBox.Text != nil {
			s = *textBox.Text + " "
		}
		textBox.Text = &s
		return
	}

	keyName := sdl.GetKeyName(sdl.GetKeyFromScancode(event.Keysym.Scancode))
	if len(keyName) == 1 {
		var s string
		c := keyName[0]
		if event.Keysym.Mod&sdl.KMOD_SHIFT != 0 {
			s = strings.ToUpper(string(c))
		} else {
			s = strings.ToLower(string(c))
		}
		if textBox.Text != nil {
			s = *textBox.Text + s
		}
		textBox.Text = &s
	}
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
				for i := 0; i < 3; i++ {
					for j := 0; j < 3; j++ {
						// newTable.Values[i][j] = strconv.Itoa(rand.Int() % 10000)
						newTable.Values[i][j] = "mango mango mango"
					}
				}
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

// Horizontal cell text margin as a proportion of cell width
const TableCellTextHorizontalMargin = 0.05

type Table struct {
	ViewPort          *ViewPort
	Rect              sdl.Rect
	Hover             bool
	Grabbed           bool
	GrabHandleVisible bool
	GrabHandle        sdl.Rect
	Values            [3][3]string
	ShowActiveCell    bool
	ActiveCellX       int32
	ActiveCellY       int32
}

func (table *Table) GetX(x int32) int32 {
	return table.ViewPort.GetX(x) + table.Rect.X + x
}

func (table *Table) GetY(y int32) int32 {
	return table.ViewPort.GetY(y) + table.Rect.Y + y
}

func (table *Table) GetRect(x, y, w, h int32) (sdl.Rect, bool) {
	return table.ViewPort.GetRect(table.Rect.X+x, table.Rect.Y+y, w, h)
}

func (table *Table) CellW() float32 {
	return float32(table.Rect.W) / 3
}

func (table *Table) CellH() float32 {
	return float32(table.Rect.H) / 3
}

func (table *Table) Render(viewPort *ViewPort, renderer *sdl.Renderer, window *sdl.Window) {
	// render frame
	rect, visible := table.GetRect(0, 0, table.Rect.W, table.Rect.H)
	if !visible {
		return
	}
	renderer.SetDrawColor(0, 0, 0, 255)
	renderer.DrawRect(&rect)

	// render active cell
	if table.ShowActiveCell {
		rect, visible := table.GetRect(
			table.ActiveCellX*int32(table.CellW()),
			table.ActiveCellY*int32(table.CellH()),
			int32(table.CellW()),
			int32(table.CellH()),
		)
		if visible {
			renderer.SetDrawColor(96, 96, 96, 172)
			renderer.SetDrawBlendMode(sdl.BLENDMODE_BLEND)
			renderer.FillRect(&rect)
		}
	}

	// render data
	renderer.SetDrawColor(0, 0, 0, 255)
	for x := float32(table.Rect.X); x < float32(table.Rect.X+table.Rect.W); x += table.CellW() {
		x := table.ViewPort.GetX(int32(x))
		y1 := table.ViewPort.GetY(table.Rect.Y)
		y2 := table.ViewPort.GetY(table.Rect.Y + table.Rect.H - 1)
		visible := viewPort.Rect.IntersectLine(&x, &y1, &x, &y2)
		if visible {
			renderer.DrawLine(x, y1, x, y2)
		}
	}
	for y := float32(table.Rect.Y); y < float32(table.Rect.Y+table.Rect.H); y += table.CellH() {
		y := table.ViewPort.GetY(int32(y))
		x1 := table.ViewPort.GetX(table.Rect.X)
		x2 := table.ViewPort.GetX(table.Rect.X + table.Rect.W - 1)
		visible := viewPort.Rect.IntersectLine(&x1, &y, &x2, &y)
		if visible {
			renderer.DrawLine(x1, y, x2, y)
		}
	}

	font, _ := TtfOpenFont(window, renderer, "assets/Lato/Lato-Regular.ttf", 10)
	for xi := int32(0); xi < 3; xi++ {
		for yi := int32(0); yi < 3; yi++ {
			fontSurface, _ := font.RenderUTF8Blended(table.Values[yi][xi], sdl.Color{R: 0, G: 0, B: 0, A: 255})
			textTexture, _ := renderer.CreateTextureFromSurface(fontSurface)
			_, _, w, h, _ := textTexture.Query()
			xs, ys := GetWindowScale(window, renderer)
			dst, visible := table.GetRect(
				// left adjusted
				int32(float32(xi)*table.CellW()+table.CellW()*TableCellTextHorizontalMargin),
				// vertically centered
				int32(float32(yi)*table.CellH()+((table.CellH()-float32(h))/2)*ys),
				w,
				h,
			)
			if visible {
				rect, _ := table.GetRect(
					int32(float32(xi)*table.CellW()),
					int32(float32(yi)*table.CellH()),
					// TODO: fix the magical 0.99. Without it the text spills slightly beyond the last cell
					int32(table.CellW()*xs*0.99),
					int32(table.CellH()),
				)
				dst, _ = dst.Intersect(&rect)
				RendererCopy(
					window,
					renderer,
					textTexture,
					&sdl.Rect{X: 0, Y: 0, W: dst.W, H: dst.H},
					&dst,
				)
			}
		}
	}

	// render grab handle
	if table.GrabHandleVisible {
		var visible bool
		table.GrabHandle, visible = table.GetRect(0, 0, TableGrabHandleSize, TableGrabHandleSize)
		if visible {
			renderer.SetDrawColor(0, 0, 128, 255)
			renderer.FillRect(&table.GrabHandle)
		}
	}
}

func (table *Table) handleArrowKeys(event *sdl.KeyboardEvent) {
	if !table.ShowActiveCell {
		return
	}
	if event.Type == sdl.KEYDOWN {
		switch event.Keysym.Scancode {
		case sdl.SCANCODE_DOWN:
			table.ActiveCellY = Min(table.ActiveCellY+1, 2)
		case sdl.SCANCODE_UP:
			table.ActiveCellY = Max(table.ActiveCellY-1, 0)
		case sdl.SCANCODE_LEFT:
			table.ActiveCellX = Max(table.ActiveCellX-1, 0)
		case sdl.SCANCODE_RIGHT:
			table.ActiveCellX = Min(table.ActiveCellX+1, 2)
		}
	}
}

func (table *Table) pointInside(x, y int32) bool {
	rect, _ := table.GetRect(0, 0, table.Rect.W, table.Rect.H)
	return pointInRect(x, y, rect)
}

// Get row and column of the cell in table at position (x, y) on the screen.
func (table *Table) GetCellAtPos(x, y int32) (row, col int32) {
	row = (x - table.GetX(0)) / int32(table.CellW())
	col = (y - table.GetY(0)) / int32(table.CellH())
	return
}

func (table *Table) OnMouseButtonEvent(event *sdl.MouseButtonEvent, app *App) bool {
	if event.Button == sdl.BUTTON_LEFT {
		if event.Type == sdl.MOUSEBUTTONDOWN {
			if table.ShowActiveCell = table.pointInside(event.X, event.Y); table.ShowActiveCell {
				table.ActiveCellX, table.ActiveCellY = table.GetCellAtPos(event.X, event.Y)
			}
			if table.GrabHandleVisible {
				table.Grabbed = pointInRect(event.X, event.Y, table.GrabHandle)
			}
		} else {
			table.Grabbed = false
		}
	}
	return table.Grabbed
}

// TODO: move this function to ViewPort, because it needs to know about all the tables
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

func (viewPort *ViewPort) GetX(x int32) int32 {
	return viewPort.Rect.X + viewPort.HorizontalScroll + x
}

func (viewPort *ViewPort) GetY(y int32) int32 {
	return viewPort.Rect.Y + viewPort.VerticalScroll + y
}

func (viewPort *ViewPort) GetRect(x, y, w, h int32) (rect sdl.Rect, visible bool) {
	rect = sdl.Rect{X: viewPort.GetX(x), Y: viewPort.GetY(y), W: w, H: h}
	rect, visible = rect.Intersect(&viewPort.Rect)
	return
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
		maxX := Max(viewPort.MaxViewableX(), viewPort.Rect.W-viewPort.HorizontalScroll)
		viewRatio := float32(viewPort.Rect.W) / float32(maxX)
		renderer.FillRect(&sdl.Rect{
			X: viewPort.Rect.X - int32(float32(viewPort.HorizontalScroll)*viewRatio),
			Y: viewPort.Rect.Y + viewPort.Rect.H - ViewPortScrollBarDim,
			W: int32(float32(viewPort.Rect.W) * viewRatio),
			H: ViewPortScrollBarDim,
		})
	}

	if viewPort.ShowVerticalScrollBar > 0 {
		maxY := Max(viewPort.MaxViewableY(), viewPort.Rect.H-viewPort.VerticalScroll)
		viewRatio := float32(viewPort.Rect.H) / float32(maxY)
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
	// We handle only one scroll at a time, meaning that the user can only scroll
	// vertically or only horizontally at a time.
	if Abs(event.X) > Abs(event.Y) {
		viewPort.ShowHorizontalScrollBar = ViewPortScrollBarDisplayMs
		viewPort.ShowVerticalScrollBar = 0
		dx := ViewPortScrollSpeed * event.X
		maxScrollX := viewPort.MaxScrollX()

		// disallow scrolling beyond into negative coordinates
		newHorizontalScroll := Min(viewPort.HorizontalScroll-dx, 0)

		if newHorizontalScroll >= maxScrollX {
			// Scroll is within bounds, the simple case.
			viewPort.HorizontalScroll = newHorizontalScroll
		} else {
			// All the objects in the view port are offset by the scroll so if it's negative then
			// the object are moved to the left and we are seeing more of things on the right.
			// Maximum scroll is a negative number and being below the max scroll means being
			// too far on the right.
			if dx < 0 {
				// Going left, back from further than "maximum". Happens whenver an object is moved to the left
				// so that the maximum viewable X is decreased. We do not want to snap back to maximum
				// viewable X, rather we allow the user to scroll back.
				viewPort.HorizontalScroll = newHorizontalScroll
			} else if viewPort.HorizontalScroll >= maxScrollX {
				// Going right, make sure that we can reach the maximum scroll.
				viewPort.HorizontalScroll = maxScrollX
			}
		}
	} else if Abs(event.X) < Abs(event.Y) { // Analogous to the case above.
		viewPort.ShowHorizontalScrollBar = 0
		viewPort.ShowVerticalScrollBar = ViewPortScrollBarDisplayMs
		dy := ViewPortScrollSpeed * event.Y
		maxScrollY := viewPort.MaxScrollY()
		newVerticalScroll := Min(viewPort.VerticalScroll+dy, 0)
		if newVerticalScroll >= maxScrollY {
			viewPort.VerticalScroll = newVerticalScroll
		} else {
			if dy > 0 {
				viewPort.VerticalScroll = newVerticalScroll
			} else if viewPort.VerticalScroll >= maxScrollY {
				viewPort.VerticalScroll = maxScrollY
			}
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
			if event.Type == sdl.KEYDOWN && event.Keysym.Scancode == sdl.SCANCODE_Q && !app.TextBox.Active {
				return false
			}
			for i := 0; i < app.ViewPort.TablesCount; i++ {
				app.ViewPort.Tables[i].handleArrowKeys(event)
			}
			app.TextBox.OnKeyPress(event)
		case *sdl.MouseButtonEvent:
			app.TextBox.OnMouseButtonEvent(event)
			for i := range app.Components {
				app.Components[i].OnMouseButtonEvent(event, app)
			}
			for i := 0; i < app.ViewPort.TablesCount; i++ {
				app.ViewPort.Tables[i].OnMouseButtonEvent(event, app)
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
	scaled_dst := sdl.Rect{X: dst.X, Y: dst.Y, W: int32(float32(dst.W) / xs), H: int32(float32(dst.H) / ys)}
	renderer.Copy(texture, src, &scaled_dst)
	// For debugging:
	// renderer.SetDrawColor(255, 0, 0, 255)
	// renderer.DrawRect(&scaled_dst)
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

	textBox := TextBox{
		Rect:               sdl.Rect{X: 55, Y: 15, W: 330, H: 25},
		CursorBlinkSpeedMs: 750,
	}

	app := App{
		Window:     window,
		Renderer:   renderer,
		TextBox:    textBox,
		ViewPort:   ViewPort{Rect: sdl.Rect{X: 10, Y: 55}},
		Components: []Component{&buttonAddTable},
	}

	for {
		app.UpdateTicks()

		if continue_ := handleEvents(&app); !continue_ {
			break
		}

		renderer.SetDrawColor(255, 255, 255, 255)
		renderer.Clear()

		app.TextBox.Render(renderer, &app)
		app.ViewPort.Render(renderer, &app)

		for _, button := range app.Components {
			button.Render(renderer, &app)
		}
		for i := 0; i < app.ViewPort.TablesCount; i++ {
			app.ViewPort.Tables[i].Render(&app.ViewPort, renderer, window)
		}

		renderer.Present()
		sdl.Delay(16) // cap at 60fps
	}

	buttonAddTable.Destroy()
}
