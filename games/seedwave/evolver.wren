import "xs" for Data
import "random" for Random
import "xs_math"for Math

class Entry {
    construct new(dna) {
        _dna = dna
        _score = 0
    }

    dna { _dna }
    score { _score }
    score=(s) { _score = s}

    <(other) { score  < other.score }

    toString { "[Entry dna:%(_dna) score:%(_score)]" }
}

class Evolver {
    static init() {
        __entries = []        
        __dna = ""
        __random = Random.new()
    }

    static generateDnaPair() {
        var r = __random.int(-1, 5)
        var l = __random.int(1, 4)
        if(r <= 0) {
            return "S0"                // Skip
        } else if(r == 1) {
            return "C" + l.toString    // Canons
        } else if(r == 2) {
            return "L" + l.toString    // Lasers
        } else if(r == 3) {
            return "M" + l.toString    // Missles
        } else if(r == 4) {
            return "D" + l.toString    // Deflect
        } else if(r == 5) {
            return "N" + l.toString    // Needler
        } else if(r == 6) {
            return "E" + l.toString    // EMP
        } else if(r == 7) {
            return "H" + l.toString    // Helpers
        } else if(r == 7) {
            return "V" + l.toString    // Vulcan
        } 
        return ""
    }

    static generateDna(size) {
        var dna = ""
        var protein = 0
        while(protein <= size) {
            var p = generateDnaPair()
            if(p != "S0") {
                dna = dna + p
                protein = protein + Num.fromString(p[1])
            }
        }
        System.print("Generated dna %(dna)")
        return dna        
    }

    static getNextDna() {
        if(!Data.getString("DNA", Data.debug).isEmpty) {
            __dna = Data.getString("DNA", Data.debug)
            return __dna
        } else if(Data.getNumber("Protein", Data.debug) != 0) {
            __dna = generateDna(Data.getNumber("Protein", Data.debug))
            return __dna
        } else {
            if(__entries.count < 4) { //TODO Data
                __dna = generateDna(6)   // TODO Data                
            } else {
                __entries = __entries.sort()
                System.print(__entries)
                var dna0 = __entries[0].dna
                var dna1 = __entries[1].dna
                __dna = meiosis(dna0, dna1)
                __dna = mutate(__dna)
            }

            __entries.add(Entry.new(__dna))            
            return __dna
        }
    }

    static meiosis(dna0, dna1) {
        var to = Math.min(dna0.count, dna1.count)
        var n = to / 2
        var dna = ""
        for(i in 0...n) {
            var active = __random.int(0, 2) == 0 ? dna0 : dna1
            dna = dna + active[i * 2] + active[i * 2 + 1]
        }

        var mom = (to == dna0.count) ? dna1 : dna0
        for(i in to...mom.count) {
            dna = dna + mom[i]
        }

        return dna
    }

    static mutate(dna) {
        /*
        var n = dna.count / 2
        if(__random.float() < 0.2) {
            var i = __random.int(0, n)
            var gene = dna[i * 2 + 1]
            gene = Num.fromString(gene)
            if(gene < 3) {
                gene = gene + 1
                dna[i * 2 + 1] = gene.toString
            }            
        }
        */

        if(__random.float() < 0.75) {
            dna = dna + generateDnaPair()
        }

        return dna
    }

    static currentDna { __dna }

    static addScoreToCurrent(score) {
        __entries[__entries.count-1].score = -score
    }

}