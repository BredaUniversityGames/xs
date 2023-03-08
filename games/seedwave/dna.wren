
// Type of part
class Type {
    construct new(value) { _value = value }
    value { _value }

    static J { 0 }  // Jump/Skip
    static C { 1 }  // Cannon
    static L { 2 }  // Laser
    static M { 3 }  // Missile
    static S { 4 }  // Shield
    static V { 5 }  // Vulcan 
    static N { 6 }  // Needle
    static E { 7 }  // EMP
    static D { 8 }  // Drone

    static from { Type.J + 1 }
    static to   { Type.V + 1 }

    toString {
        if(_value == Type.J) return "J"
        if(_value == Type.C) return "C"
        if(_value == Type.L) return "L"
        if(_value == Type.M) return "M"
        if(_value == Type.S) return "S"
        if(_value == Type.V) return "V"
        if(_value == Type.N) return "N"
        if(_value == Type.E) return "E"
        if(_value == Type.D) return "D"
    }

    == (other) {
        if(other is Num) {
            return _value == other
        } else {
            _value == other.value
        }
    }
}

// Gene is a combination of an Type (part type) and a size
class Gene {
    // Construct a new gene
    construct new(type, size) {
        _type = Type
        _size = size        
    }

    // Construct a new random gene
    construct new(random) {
        _type = Type.new(random.int(Type.from, Type.to))
        _size = random.int(1, 4)
    }

    size { _size }
    type { _type }
    toString { _type.toString + _size.toString }
}

// DNA is a list of genes
class DNA {
    // Construct a new DNA
    construct new() {
        _genes = []
    } 

    // Construct a new DNA from a list of genes
    construct new(dna) {
        _genes = []
        for (gene in dna.genes) {
            _genes.add(gene)
        }
    }

    // Construct a new random DNA
    construct new(random, size) {
        _genes = []
        for (i in 0...size) {
            _genes.add(Gene.new(random))
        }        
    }
    // Construct a new DNA from two parents
    construct new(random, mother, father) {
        _genes = []

        if(mother.genes.count != father.genes.count) {
            System.print("Mother and father have different size!")
            return
        }

        var dna = DNA.meiosis(random, mother, father)
        _genes = dna.genes
    }
    
    genes { _genes }

    toString {
        var str = ""
        for (gene in _genes) {
            str = str + gene.toString
        }
        str = str + " variance: " + variance.toString
        return str
    }

    variance {
        var types = {}
        for (gene in _genes) {
            types[gene.type.value] = 1
        }
        return types.count / _genes.count
    }

    // Meiosis is the process of creating a new DNA from two parents
    static meiosis(random, mother, father) {
        var n = mother.genes.count
        var child = DNA.new()
        for(i in 0...n) {
            var gene0 = mother.genes[i]
            var gene1 = father.genes[i]
            var gene = null
            if (random.float() < 0.5) {
                gene = gene0
            } else {
                gene = gene1
            }
            child.genes.add(gene)
        }
        // Always add a new gene
        child.genes.add(Gene.new(random))
        return child        
    }
}