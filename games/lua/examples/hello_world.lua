Game = {}
function Game:new(o)
    o = o or {}   -- create object if user does not provide one
    setmetatable(o, self)
    self.__index = self

    self._objects  = {}
    self._addQueue = {}

    return o
end

function Game:init()
    print(xs_configuration.getTitle())
end

function Game:render()
end

function Game:config()
    xs_configuration.setWidth(640)
    xs_configuration.setHeight(360)
    xs_configuration.setMultiplier(2)
    xs_configuration.setTitle("SubOptimal")
end

function Game:update(dt)
    xs_render.setColor(1.0, 1.0, 1.0)
    xs_render.text("Hol@ World", -96, 0, 4)
end

game=Game:new()