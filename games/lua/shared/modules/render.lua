xs_render.triangles = 1
xs_render.lines = 2

function xs_render.rect(fromX, fromY, toX, toY)     
    xs_render.begin(xs_render.triangles)
    xs_render.vertex(fromX, fromY)
    xs_render.vertex(toX, fromY)
    xs_render.vertex(toX, toY)

    xs_render.vertex(fromX, fromY)
    xs_render.vertex(fromX, toY)
    xs_render.vertex(toX, toY)
    xs_render.rend()
end

function xs_render.square(centerX, centerY, size)
    local s = size * 0.5
    xs_render.rect(centerX - s, centerY - s, centerX + s, centerY + s)
end

function xs_render.disk(x, y, r, divs)
    xs_render.begin(xs_render.triangles)
    local t = 0.0
    local dt = (math.pi * 2.0) / divs
    for i = 0, divs, 1
    do            
        xs_render.vertex(x, y)
        local xr = math.cos(t) * r            
        local yr = math.sin(t) * r
        xs_render.vertex(x + xr, y + yr)
        t = t + dt
        xr = math.cos(t) * r
        yr = math.sin(t) * r
        xs_render.vertex(x + xr, y + yr)
    end
    xs_render.rend()
end


return xs_render