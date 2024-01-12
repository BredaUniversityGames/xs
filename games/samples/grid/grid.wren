import "random" for Random // Random is a part of the Wren library

/// Reference to a horizontal or vertical chain of cells in the grid.
class ChainRef {
    construct new(x, y, length, horizontal) {
        _x = x
        _y = y
        _length = length
        _horizontal = horizontal
    }

    x { _x }
    y { _y }
    length { _length }
    horizontal { _horizontal }
}

/// Logical representation of the game grid.
class Grid {
    construct new(width, height, minValue, maxValue) {
        __EMPTY = 0
        
        _random = Random.new(1000)
        _grid = []
        _width = width
        _height = height
        _minValue = minValue
        _maxValue = maxValue

        var N = _width*_height
        for (i in 1..N) {
            _grid.add(__EMPTY)
        }

        _eventListener = null
    }

    setEventListener(listener) {
        _eventListener = listener
    }

    /// The number of columns in the grid.
    width { _width }

    /// The number of rows in the grid.
    height { _height }

    /// The smallest (non-empty) value that a grid cell can have.
    minValue { _minValue }

    /// The largest value that a grid cell can have.
    maxValue { _maxValue }

    emptyValue { __EMPTY }

    /// Returns the value stored at the given grid cell.
    getValue(x, y) {
        return _grid[y*_width + x]
    } 

    /// Assigns a given value to a given grid cell.
    setValue(x, y, val, notifyListener) {
        setValue(x, y, val)
        if (notifyListener && val == __EMPTY && _eventListener != null) {
            _eventListener.onElementRemoved(x, y)
        }
    }

    /// Assigns a given value to a given grid cell.
    setValue(x, y, val) {
        _grid[y*_width + x] = val
    }

    /// Checks if a given cell position exists in the grid.
    isValidPosition(x, y) {
        return x >= 0 && x < _width && y >= 0 && y < _height
    }

    /// Prints the contents of this grid to the console.
    print() {
        var line = "\n"
        for (y in 0..._height) {
            for (x in 0..._width) {
                var val = getValue(x,y)
                if (val == __EMPTY) {
                    line = line + " ."
                } else {
                    line = line + " %(val)"
                }
            }
            line = line + "\n"
        }
        System.print(line)
    }
    
    /// Fills all empty cells in the grid with random values.
    fillRandomly() {
        var N = _width*_height
        for (y in 0..._height) {
            for (x in 0..._width) {
                if (getValue(x,y) == __EMPTY) {
                    setValue(x,y, _random.int(_minValue,_maxValue+1))
                }
            }
        }
    }

    /// Swaps the values of two given grid cells.
    swapValues(x1, y1, x2, y2) {
        if (!isValidPosition(x1,y1) || !isValidPosition(x2,y2)) {
            return false
        }
        
        var val1 = getValue(x1,y1)
        var val2 = getValue(x2,y2)
        setValue(x1, y1, val2)
        setValue(x2, y2, val1)

        if (_eventListener != null) {
            if (val1 != __EMPTY) _eventListener.onElementMoved(x1, y1, x2, y2)
            if (val2 != __EMPTY) _eventListener.onElementMoved(x2, y2, x1, y1)
        }
        
        return true
    }

    /// Places a zero into all cells that currently have the given value.
    removeAllOccurrences(val) {
        replaceAllOccurrences(val, __EMPTY)
    }

    /// Places the value "newVal" into all cells that currently have the value "oldVal".
    replaceAllOccurrences(oldVal, newVal) {
        for (y in 0..._height) {
            for (x in 0..._width) {
                if (getValue(x,y) == oldVal) {
                    setValue(x,y,newVal)
                }
            }
        }
    }

    /// Refills the given cell with a given new value, along with all connected cells with the same original value.
    fillIsland(x, y, newVal) {
        if (!isValidPosition(x, y)) {
            return
        }
        var oldVal = getValue(x,y)
        if (oldVal == newVal) {
            return
        }
        fillIsland_(x, y, oldVal, newVal)
    }

    fillIsland_(x, y, oldVal, newVal) {
        if (!isValidPosition(x, y) || getValue(x, y) != oldVal) {
            return
        }

        setValue(x, y, newVal)
        fillIsland_(x-1, y, oldVal, newVal)
        fillIsland_(x+1, y, oldVal, newVal)
        fillIsland_(x, y-1, oldVal, newVal)
        fillIsland_(x, y+1, oldVal, newVal)
    }

    /// Lets all non-empty grid elements fall down to fill up empty spaces.
    collapse() {
        for (x in 0..._width) {
            collapseColumn(x)
        }
    }

    collapseColumn(x) {
        var fallAmount = 0
        var y = _height - 1
        while (y >= 0) {
            if (getValue(x,y) == __EMPTY) {
                fallAmount = fallAmount + 1
            } else if (fallAmount > 0) {
                swapValues(x, y, x, y + fallAmount)
            }
            y = y - 1
        }
    }

    /// Looks for horizontal and vertical chains of at least "minLength" cells with the same value,
    /// and places "newValue" in all corresponding grid cells.
    replaceChains(minLength, newValue) {

        var chainsFound = []

        // --- look for horizontal chains in each row

        var xMaxForHorizontalChain = _width - minLength
        for (y in 0..._height) {
            var x = 0
            while (x <= xMaxForHorizontalChain) {
                var val = getValue(x, y)

                // keep looking to the right as long as the values are the same
                var x2 = x + 1
                while (x2 < _width && getValue(x2, y) == val) {
                    x2 = x2 + 1
                }

                // if the chain was long enough, store it
                var length = x2 - x
                if (length >= minLength && val != __EMPTY) {
                    chainsFound.add(ChainRef.new(x, y, length, true))
                }

                // continue at the first cell that has a different value
                x = x2
            }
        }

        // --- look for vertical chains in each column

        var yMaxForVerticalChain = _height - minLength
        for (x in 0..._width) {
            var y = 0
            while (y <= yMaxForVerticalChain) {
                var val = getValue(x, y)

                // keep looking down as long as the values are the same
                var y2 = y + 1
                while (y2 < _height && getValue(x, y2) == val) {
                    y2 = y2 + 1
                }

                // if the chain was long enough, store it
                var length = y2 - y
                if (length >= minLength && val != __EMPTY) {
                    chainsFound.add(ChainRef.new(x, y, length, false))
                }

                // continue at the first cell that has a different value
                y = y2
            }
        }

        // --- remove all chains found

        for (chain in chainsFound) {
            replaceChain(chain, newValue)
        }

        return chainsFound.count > 0

    }

    /// Given a single chain in the grid (as a ChainRef), replaces the corresponding grid values by newValue.
    replaceChain(chain, newValue) {
        if (chain.horizontal) {
            for (x in 0...chain.length) {
                setValue(chain.x + x, chain.y, newValue, true)
            }
        } else {
            for (y in 0...chain.length) {
                setValue(chain.x, chain.y + y, newValue, true)
            }
        }
    }
}