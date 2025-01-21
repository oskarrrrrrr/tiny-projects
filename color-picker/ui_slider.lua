local debug = require("_debug")

local UISlider = {}
UISlider.__index = UISlider

function UISlider.new(name, caption, show_value, curr, min, max, width, height, x, y)
    local ui_slider = setmetatable({}, UISlider)
    ui_slider.name = name
    ui_slider.caption = caption
    ui_slider.show_value = show_value
    ui_slider.curr = curr
    ui_slider.min = min
    ui_slider.max = max

    ui_slider.width = width
    ui_slider.height = height
    ui_slider.x = x
    ui_slider.y = y

    ui_slider:temp()

    ui_slider.grabbed = false
    ui_slider.handle_x, ui_slider.handle_y = ui_slider:posForCurr()

    return ui_slider
end

function UISlider:temp()
    self.line_height = self.height / 5
    self.vert_margin = (self.height - self.line_height) / 2
    self.horiz_margin = self.width / 15
    self.line_width = self.width - 2 * self.horiz_margin
    self.line_vert_middle = (
        self.y + self.vert_margin + self.line_height / 2
    )
    self.handle_radius = self.line_height
end

function UISlider:posForCurr()
    local x_offset
    if self.max == self.min then
        x_offset = 0
    else
        x_offset = self.line_width * ((self.curr - self.min) / (self.max - self.min))
    end
    return
        self.x + self.horiz_margin + x_offset,
        self.y + self.line_height / 2 + self.vert_margin
end

function UISlider:updateX(x)
    self.x = x
    self:temp()
    self.handle_x, self.handle_y = self:posForCurr()
end

function UISlider:updateY(y)
    self.y = y
    self:temp()
    self.handle_x, self.handle_y = self:posForCurr()
end

function UISlider:lineStartX()
    return self.x + self.horiz_margin
end

function UISlider:lineStartY()
    return self.y + self.vert_margin
end

function UISlider:drawHandle()
    love.graphics.circle("fill", self.handle_x, self.handle_y, self.handle_radius)
end

function UISlider:draw()
    love.graphics.setColor(1, 1, 1, 1)
    love.graphics.rectangle(
        "fill",
        self:lineStartX(), self:lineStartY(),
        self.line_width, self.line_height
    )
    love.graphics.circle(
        "fill",
        self:lineStartX(),
        self.line_vert_middle,
        self.line_height / 2
    )
    love.graphics.circle(
        "fill",
        self:lineStartX() + self.line_width,
        self.line_vert_middle,
        self.line_height / 2
    )

    self:drawHandle()

    local text = self.caption
    if self.show_value then
        text = text .. ": " .. tostring(self.curr)
    end
    local font       = love.graphics.getFont()
    local textWidth  = font:getWidth(text)
    local textHeight = font:getHeight()
    love.graphics.setColor(1, 1, 1, 1)
    love.graphics.print(
        text,
        self:lineStartX() + self.line_width / 2 - textWidth / 2,
        self:lineStartY() + self.line_height + self.vert_margin / 2 - textHeight / 2,
        0, 1, 1
    )

    debug.drawOutline(
        self.x, self.y,
        self.width, self.height,
        self.name,
        { value = self.curr, handle_x = self.handle_x }
    )
end

function UISlider:pointOnHandle(x, y)
    local dist = math.pow(x - self.handle_x, 2) + math.pow(y - self.handle_y, 2)
    return dist <= math.pow(self.handle_radius, 2)
end

function UISlider:pointOnLine(x, y)
    return (
        self:lineStartX() <= x and
        x <= self:lineStartX() + self.line_width and
        self:lineStartY() <= y and
        y <= self:lineStartY() + self.line_height
    )
end

function UISlider:mouseDown()
    local mouseX, mouseY = love.mouse.getX(), love.mouse.getY()
    if self:pointOnHandle(mouseX, mouseY) then
        self.grabbed = true
    elseif self:pointOnLine(mouseX, mouseY) then
        self.handle_x = mouseX
        self.curr = self:currForPos(self.handle_x)
        self.grabbed = true
    end
    if self.grabbed then
        love.mouse.setCursor(love.mouse.getSystemCursor("hand"))
    end
end

function UISlider:mouseUp()
    self.grabbed = false
    love.mouse.setCursor(love.mouse.getSystemCursor("arrow"))
end

function UISlider:currForPos(x, y)
    return self.min + math.floor(
        ((self.max - self.min)
            * (x - self:lineStartX())
            / self.line_width) + 0.5
    )
end

function UISlider:mouseMove()
    local mouseX, mouseY = love.mouse.getX(), love.mouse.getY()
    if self:pointOnHandle(mouseX, mouseY) or self:pointOnLine(mouseX, mouseY) then
        love.mouse.setCursor(love.mouse.getSystemCursor("hand"))
    elseif not self.grabbed then
        love.mouse.setCursor(love.mouse.getSystemCursor("arrow"))
    end
    if not self.grabbed then
        return
    end
    if mouseX <= self:lineStartX() then
        self.handle_x = self:lineStartX()
        self.curr = self.min
    elseif mouseX >= self:lineStartX() + self.line_width then
        self.handle_x = self:lineStartX() + self.line_width
        self.curr = self.max
    else
        self.handle_x = mouseX
        self.curr = self:currForPos(self.handle_x, self.handle_y)
    end
end

return {
    UISlider = UISlider,
}
