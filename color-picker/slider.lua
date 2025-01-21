local Slider = {}
Slider.__index = Slider

function Slider:value()
    assert(false, "Impement value method for your slider.")
end

function Slider:update(dt)
    assert(false, "Implement update method for your slider.")
end

function Slider:done()
    assert(false, "Implement done method for your slider.")
end

local LinearSlider = {}
LinearSlider.__index = LinearSlider
setmetatable(LinearSlider, Slider)

function LinearSlider.new(start, min_value, max_value, speed)
    local slider = setmetatable({}, LinearSlider)
    slider.start = start
    slider.curr = start
    slider.prev_value = start
    slider.min_value = min_value
    slider.max_value = max_value
    slider.speed = speed
    return slider
end

function LinearSlider:update(dt)
    if self:done() then
        return self.prev_value
    end
    local step = dt * self.speed
    self.prev_value = self.prev_value + step
    if self.max_value ~= nil and self.prev_value > self.max_value then
        self.prev_value = self.max_value
    end
    if self.min_value ~= nil and self.prev_value < self.min_value then
        self.prev_value = self.min_value
    end
end

function LinearSlider:done()
    return (
        (self.max_value ~= nil and self.prev_value == self.max_value) or
        (self.min_value ~= nil and self.prev_value == self.min_value)
    )
end

function LinearSlider:value()
    return self.prev_value
end

local MultiSlider = {}
MultiSlider.__index = MultiSlider
setmetatable(MultiSlider, Slider)

function MultiSlider.new(sliderGetters, loop)
    assert(#sliderGetters > 1, "MultiSlider expects at least 2 sliders.")
    local slider = setmetatable({}, MultiSlider)
    slider.sliderGetters = sliderGetters
    slider.curr_slider_idx = -1
    slider.curr_slider = nil
    slider.loop = loop
    slider:getNextSlider()
    return slider
end

function MultiSlider:getNextSlider()
    if self.curr_slider_idx == #self.sliderGetters then
        if self.loop then
            self.curr_slider_idx = 1
        else
            return
        end
    elseif self.curr_slider_idx == -1 then
        self.curr_slider_idx = 1
    else
        self.curr_slider_idx = self.curr_slider_idx + 1
    end
    self.curr_slider = self.sliderGetters[self.curr_slider_idx]()
end

function MultiSlider:update(dt)
    self.curr_slider:update(dt)
    if self.curr_slider:done() then
        self:getNextSlider()
        if self.curr_slider:done() then
            return
        end
    end
end

function MultiSlider:value()
    return self.curr_slider:value()
end

function MultiSlider:done()
    return self.loop and self.curr_slider:done()
end

return {
    LinearSlider = LinearSlider,
    MultiSlider = MultiSlider
}
