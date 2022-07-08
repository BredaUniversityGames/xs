local xsmath =  {}

function xsmath.lerp(a, b, t)
    return (a * (1.0 - t)) + (b * t) 
end

function xsmath.damp(a, b, lambda, dt)
    --math lib already loaded--
    return xsmath.lerp(a, b, 1.0 - math.exp (-lambda * dt))
end

return xsmath