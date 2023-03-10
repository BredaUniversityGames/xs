import "xs" for Data
import "random" for Random
import "xs_math"for Math

class Entry {
    construct new(dna, generation) {
        _dna = dna
        _generation = generation
        _score = 0        
    }

    dna { _dna }
    generation { _generation }
    score { _score }
    score=(s) { _score = s}

    <(other) { score  < other.score }

    toString { "[Entry dna:%(_dna) score:%(_score) genration:%(_generation) ]" }
}

class Evolver {
    static init() {
        __population = []
        __random = Random.new()
        __generation = 0
        __mother = null
        __father = null
        __current = -1
        createNextGeneration()
    }

    static createNextGeneration() {
        __generation = __generation + 1
        System.print("Creating generation %(__generation)...")
        
        var ps = Data.getNumber("Population Size", Data.game)
        var ids = Data.getNumber("Initial DNA Size", Data.game)        

        // If generation 1, create a population of random dna with the initial population size
        if(__generation == 1) {
            var count = 0
            while (count < ps) {
                var dna = DNA.new(__random, ids)
                if(dna.variance > Data.getNumber("Min Population Variance", Data.game)) {
                    __population.add(Entry.new(dna, __generation))
                    count = count + 1
                }
            }

            System.print("Created random population \n %(__population) ")
        } else {    // Otherwise, create a new population from the previous generation
            __population.sort()
            __mother = __population[0]
            __father = __population[1]
            __population = []
            for(i in 0...ps) {
                var dna = DNA.new(__random, __mother.dna, __father.dna)
                __population.add(Entry.new(dna, __generation))
            }
        }
    }

    static generation { __generation }
    static currentDna { __population[__current].dna }

    static nextDna {
        // if we have a dna in the debug data, use that
        if(!Data.getString("Dbg DNA", Data.debug).isEmpty) {
            var dna = Data.getString("Dbg DNA", Data.debug)
            return dna
        } else if(Data.getNumber("Dbg DNA Size", Data.debug) != 0) {
            // if we have a dna size, generate a random dna of that size
            var dna = generateDna(Data.getNumber("Dbg DNA Size", Data.debug))
            return dna
        } else {
            // otherwise, return the next dna in the population
            var current = __current            
            System.print("Current: %(__current) Population: %(__population.count)")
            if(__current == __population.count - 1) {
                createNextGeneration()
                __current = -1
            } 
            __current = __current + 1
            System.print("Current: %(__current) Population: %(__population.count)")
            var dna = __population[__current].dna            
            return dna
        }
    }

    static addScoreToCurrent(score) {
        if(__population.count > 0) {
            __population[__population.count-1].score = -score
        }
    }

    toString {
        var s = "Evolver: generation:%(__generation) current:%(__current) population:%(__population.count)"
        if(__population.count > 0) {
            s = s + " mother:%(__mother) father:%(__father)"
        }
        s = s + __population.toString
        return s
    }
}

import "dna" for DNA