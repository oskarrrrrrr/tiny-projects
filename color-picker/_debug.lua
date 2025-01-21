local lg = love.graphics

local forceDebug = nil

local function enableDebug()
    forceDebug = true
end

local function disableDebug()
    forceDebug = false
end

local function debug()
    if forceDebug ~= nil then
        return forceDebug
    end
    local _debug = os.getenv("DEBUG")
    return _debug ~= nil and (_debug == "1" or string.upper(_debug) == "TRUE")
end

local function setDebugColor()
    lg.setColor(1, 0, 0, 1)
end

local function buildMessage(name, debug_info)
    local msg = name
    if debug_info ~= nil then
        msg = msg .. "|"
        local first = true
        for key, value in pairs(debug_info) do
            if first then
                first = false
            else
                msg = msg .. ","
            end
            msg = msg .. tostring(key) .. ":" .. tostring(value)
        end
    end
    return msg
end

local function drawOutline(x, y, w, h, name, debug_info)
    if not debug() then
        return
    end
    local msg = buildMessage(name, debug_info)
    local font = love.graphics.newFont(10)
    love.graphics.setFont(font)
    local msgWidth = font:getWidth(msg)
    local msgHeight = font:getHeight()
    local msgMargin = 3
    setDebugColor()
    love.graphics.rectangle(
        "line",
        x+w-msgWidth-2*msgMargin,
        y,
        msgWidth+2*msgMargin,
        msgHeight+2*msgMargin
    )
    love.graphics.print(
        msg,
        x+w-msgWidth-msgMargin,
        y+msgMargin
    )
    love.graphics.rectangle("line", x, y, w, h)
end

return {
    enableDebug=enableDebug,
    disableDebug=disableDebug,
    debug=debug,
    drawOutline=drawOutline,
}
