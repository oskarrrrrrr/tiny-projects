local lg = love.graphics
local lw = love.window

local slider = require("slider")
local ui_slider = require("ui_slider")
local debug = require("_debug")

local x_slider = slider.MultiSlider.new(
    {
        function() return slider.LinearSlider.new(0, nil, 100, 1 / 30) end,
        function() return slider.LinearSlider.new(100, nil, 200, 1 / 20) end,
        function() return slider.LinearSlider.new(200, nil, 400, 1 / 8) end,
        function() return slider.LinearSlider.new(400, 200, nil, -1 / 8) end,
        function() return slider.LinearSlider.new(200, 0, nil, -1 / 20) end,
    },
    true
)
local y_slider = slider.MultiSlider.new(
    {
        function() return slider.LinearSlider.new(0, nil, 300, 1 / 20) end,
        function() return slider.LinearSlider.new(300, 0, nil, -1 / 10) end
    },
    true
)

local red_slider, green_slider, blue_slider

local function adjustPositions()
    local x, y, w, h = lw.getSafeArea()
    local WIDTH, HEIGHT = 4 * w / 5, h / 10
    red_slider.width = WIDTH
    red_slider.height = HEIGHT
    red_slider:updateX(w / 2 - WIDTH / 2)
    red_slider:updateY(h - h / 10 - 3 * HEIGHT)
    green_slider.width = WIDTH
    green_slider.height = HEIGHT
    green_slider:updateX(w / 2 - WIDTH / 2)
    green_slider:updateY(h - h / 10 - 2 * HEIGHT)
    blue_slider.width = WIDTH
    blue_slider.height = HEIGHT
    blue_slider:updateX(w / 2 - WIDTH / 2)
    blue_slider:updateY(h - h / 10 - HEIGHT)
end

function love.load()
    local x, y, w, h = lw.getSafeArea()
    local WIDTH, HEIGHT = 4 * w / 5, h / 10
    red_slider = ui_slider.UISlider.new(
        "red", "red", true,
        80, 0, 255,
        WIDTH, HEIGHT,
        w / 2 - WIDTH / 2, h - h / 10 - 3 * HEIGHT
    )
    green_slider = ui_slider.UISlider.new(
        "green", "green", true,
        120, 0, 255, WIDTH, HEIGHT,
        w / 2 - WIDTH / 2, h - h / 10 - 2 * HEIGHT
    )
    blue_slider = ui_slider.UISlider.new(
        "blue", "blue", true,
        160, 0, 255, WIDTH, HEIGHT,
        w / 2 - WIDTH / 2, h - h / 10 - HEIGHT
    )
end

-- function love.update(dt)
--     if love.keyboard.isDown("q") then
--         love.event.quit()
--     end
--     if love.keyboard.isDown("r") then
--         love.event.quit("restart")
--     end
--     x_slider:update(dt)
--     y_slider:update(dt)
-- end

-- function love.draw()
--     local safe_x, safe_y, safe_w, safe_h = love.window.getSafeArea()

--     local x_rep = 1

--     local height = 200
--     local y = safe_y + safe_h/2 - height/2
--     local width = 600
--     local x = safe_x + safe_w/2 - width/2

--     local ticks = width / x_rep

--     local red = slider.LinearSlider.new(0.7, nil, 1, 0.3 / ticks)
--     local green = slider.LinearSlider.new(0, nil, 1, 1 / ticks)
--     local blue = slider.LinearSlider.new(1, 0, nil, -1 / ticks)

--     while not red:done() and not green:done() and not blue:done() do
--         love.graphics.setColor(red:value(), green:value(), blue:value(), 1)
--         for _=1,x_rep do
--             for yd = 1, height do
--                 love.graphics.points(x, y+yd-1)
--             end
--             x = x + 1
--         end
--         red:update(1)
--         green:update(1)
--         blue:update(1)
--     end

--     -- local x, y, w, h = love.window.getSafeArea()
--     -- local v = x_slider:value()
--     -- love.graphics.rectangle(
--     --     "fill",
--     --     x+w/2-v/2,
--     --     y+h/2-y_slider:value()/2,
--     --     v,
--     --     y_slider:value()
--     -- )
-- end

function love.mousepressed(x, y, button, istouch)
    if button == 1 then
        red_slider:mouseDown()
        green_slider:mouseDown()
        blue_slider:mouseDown()
    end
end

function love.mousereleased(x, y, button, istouch, presses)
    if button == 1 then
        red_slider:mouseUp()
        green_slider:mouseUp()
        blue_slider:mouseUp()
    end
end

function love.keypressed(key, scancode, isrepeat)
    if key == "q" then
        love.event.quit()
    elseif key == "r" then
        love.event.quit("restart")
    elseif key == "d" then
        if debug.debug() then
            debug.disableDebug()
        else
            debug.enableDebug()
        end
    end
end

function love.update(dt)
    adjustPositions()
    red_slider:mouseMove()
    green_slider:mouseMove()
    blue_slider:mouseMove()
end

function love.draw()
    red_slider:draw()
    green_slider:draw()
    blue_slider:draw()

    local x, y, w, h = lw.getSafeArea()
    love.graphics.setColor(
        red_slider.curr/255,
        green_slider.curr/255,
        blue_slider.curr/255,
        1
    )
    lg.rectangle("fill", w/10, h/12, 4*w/5, 4.5*h/10)

end
