import math

class vec2:
    def __init__(self, x:float, y:float) -> None:
        self.x = x
        self.y = y

    def __add__(self, other):
        return vec2(self.x + other.x, self.y + other.y)
    
    def __sub__(self, other):
        return vec2(self.x - other.x, self.y - other.y)
    
    def __mul__(self, value:float):
        return vec2(self.x * value, self.y * value)
    
    def magnitude(self) -> float:
        return math.sqrt(self.x**2 + self.y**2)
    
    def normalize(self) -> None:
        m = self.magnitude()
        self.x /= m
        self.y /= m
    
    def normalized(self):
        m = self.magnitude()
        return vec2(self.x / m, self.y / m)

    def __str__(self) -> str:
        return "[{},{}]".format(self.x, self.y)
