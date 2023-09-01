import numpy as np
import matplotlib.pyplot as plt
#from matplotlib.path import Path
#from matplotlib.patches import PathPatch
import random

random.seed()
N = 400
A = 1
B = 1
a = random.randrange(1, 6)
b = random.randrange(1, 6)
delta = np.pi / 2.0
t = np.linspace(0, 2 * np.pi, N)
#r = 0.5 + np.cos(t)
x = A * np.sin(a*t + delta)
y = B * np.sin(b*t)

fig, ax = plt.subplots()
ax.plot(x, y, "k")
ax.set(aspect=1)
plt.show()