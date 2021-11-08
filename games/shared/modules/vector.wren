class Base {
	arity { _arity }

	toString { 
		if( _arity == 2 ){
			return "[ %( this[0] ), %( this[1] ), ]"
		}
		if( _arity == 3 ){
			return "[ %( this[0] ), %( this[1] ), %( this[2] ), ]"
		}
		if( _arity == 4 ){
			return "[ %( this[0] ), %( this[1] ), %( this[2] ), %( this[3] ), ]"
		}
	}

	magnitude { 
		var acc = 0
		for ( i in 0..._arity ){
			acc = acc + ( this[i] * this[i] )
		}
		return acc.sqrt
	}

	normalise { this / this.magnitude }

	abs { 
		var v = Vector.ofArity(_arity)
		for( i in 0..._arity ){
			v[i] = this[i].abs
		}

		return v
	}
	
	floor { 
		var v = Vector.ofArity(_arity)
		for( i in 0..._arity ){
			v[i] = this[i].floor
		}

		return v
	}
	
	ceil { 
		var v = Vector.ofArity(_arity)
		for( i in 0..._arity ){
			v[i] = this[i].ceil
		}

		return v
	}

	[ i ] {
		if(i == 0){
			return _values[0]
		}
		if(i == 1){
			return _values[1]
		}
		if(i == 2 && _arity > 2){
			return _values[2]
		}
		if(i == 3 && _arity > 2){
			return _values[3]
		}

		return null
	}

	[ i ]=(val) {
		if(i == 0){
			_values[0] = val
		}
		if(i == 1){
			_values[1] = val
		}
		if(i == 2 && _arity > 2){
			_values[2] = val
		}
		if(i == 3 && _arity > 2){
			_values[3] = val
		}

		return null
	}

	== (other) {
		if( _arity != other.arity ){
			Fiber.abort("can not compare vectors of un-equal arity")
		}

		var acc = true
		for ( i in 0..._arity ){
			acc = acc && this[i] == other[i]
		}
		return acc
	}

	+ (other) {
		if( _arity != other.arity ){
			Fiber.abort("can not add vectors of un-equal arity")
		}

		var v = Vector.ofArity(_arity)
		for ( i in 0..._arity ){
			v[i] = this[i] + other[i]
		}
		return v
	}

	- (other) {
		if( _arity != other.arity ){
			Fiber.abort("can not subtract vectors of un-equal arity")
		}

		var v = Vector.ofArity(_arity)
		for ( i in 0..._arity ){
			v[i] = this[i] - other[i]
		}
		return v
	}

	* (other) {
		if(other is Vector){
			if( _arity != other.arity ){
				Fiber.abort("can not divide vectors of un-equal arity")
			}

			var v = Vector.ofArity(_arity)
			for ( i in 0..._arity ){
				v[i] = this[i] * other[i]
			}
			return v
		} 

		if (other is Num) {
			var v = Vector.ofArity(_arity)
			for ( i in 0..._arity ){
				v[i] = this[i] * other
			}
			return v
		}

		Fiber.abort("can only multiply a Vector by a Num or another Vector")
	}

	/ (other) {
		if(other is Vector){
			if( _arity != other.arity ){
				Fiber.abort("can not divide vectors of un-equal arity")
			}

			var v = Vector.ofArity(_arity)
			for ( i in 0..._arity ){
				v[i] = this[i] / other[i]
			}
			return v
		} 

		if (other is Num) {
			var v = Vector.ofArity(_arity)
			for ( i in 0..._arity ){
				v[i] = this[i] / other
			}
			return v
		}

		Fiber.abort("can only divide a Vector by a Num or another Vector")
	}

	dot(other) {
		//the dot product, returns a Num
		if( _arity != other.arity ){
			Fiber.abort("can not dot product vectors of un-equal arity")
		}

		var acc = 0
		for ( i in 0..._arity ){
			acc = acc + ( this[i] * other[i] )
		}

		return acc
	}

	cross(other) {
		//the cross product, returns a Vector
		if( _arity != 3 ){
			Fiber.abort("can only cross product vectors of arity 3")
		}

		var x = ( this[1] * other[2] ) - ( this[2] * other[1] )
		var y = ( this[2] * other[0] ) - ( this[0] * other[2] )
		var z = ( this[0] * other[1] ) - ( this[1] * other[0] )

		return Vector.new(x, y, z)
	}

	construct new(x, y) {
		_arity = 2
		_values = [ x, y ]
	}

	construct new(x, y, z) {
		_arity = 3
		_values = [ x, y, z ]
	}

	construct new(x, y, z, w) {
		_arity = 4
		_values = [ x, y, z, w ]
	}

	static ofArity(n) {
		if( n == 2 ){
			return Vector.new(0, 0)
		}
		if( n == 3 ){
			return Vector.new(0, 0, 0)
		}
		if( n == 4 ){
			return Vector.new(0, 0, 0, 0)
		}
	}
}

class Vector is Base {
	x { this[0] }
	y { this[1] }
	z { this[2] }
	w { this[3] }

	x=(val){ this[0] = val }
	y=(val){ this[1] = val }
	z=(val){ this[2] = val }
	w=(val){ this[3] = val }

	construct new(x, y){
		super(x, y)
	}

	construct new(x, y, z){
		super(x, y, z)
	}

	construct new(x, y, z, w){
		super(x, y, z, w)
	}
    
	static ofArity(n) {
		if( n == 2 ){
			return Vector.new(0, 0)
		}
		if( n == 3 ){
			return Vector.new(0, 0, 0)
		}
		if( n == 4 ){
			return Vector.new(0, 0, 0, 0)
		}
	}

}

class ColorRGBA is Base {
	r { this[0] }
	g { this[1] }
	b { this[2] }
	a { this[3] }

	r=(val){ this[0] = val }
	g=(val){ this[1] = val }
	b=(val){ this[2] = val }
	a=(val){ this[3] = val }

	construct new(r, g){
		super(r, g)
	}

	construct new(r, g, b){
		super(r, g, b)
	}

	construct new(r, g, b, a){
		super(r, g, b, a)
	}
}