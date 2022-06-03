///////////////////////////////////////////////////////////////////////////////
// Entity / Component 
///////////////////////////////////////////////////////////////////////////////

import "xs_math" for Math, Bits

class Component {
    construct new() {
        _owner = null
    }

    del() {}
    update(dt) { }    
    owner { _owner }
    owner=(o) { _owner = o }
}

class Entity {
    construct new() {
        _components = {}
        _deleted = false
        _name = ""
        _tag = 0
        _compDeleteQueue = []
        __addQueue.add(this)        
    }

    addComponent(component) {
        component.owner = this
        _components[component.type] = component
    }

    getComponent(type) {
        if(_components.containsKey(type)) {
            return _components[type]
        }
        return null
    }

    getComponentSuper(type) {
        for(v in _components.values) {
            if(v is type) {
                return v    
            }
        }
        return null
    }

    deleteComponent(type) {        
        if(_components.containsKey(type)) {            
            _compDeleteQueue.add(type) 
        }
    }

    removeDeletedComponents() {
        for(c in _compDeleteQueue) {
            _components.remove(c)
        }
        _compDeleteQueue.clear()
    }

    components { _components.values }

    deleted { _deleted }
    delete() { _deleted = true }

    name { _name }
    name=(n){ _name = n }

    tag { _tag }
    tag=(t) { _tag = t }

    static init() {
        __entities = []
        __addQueue = []
    }

    static update(dt) {
        for(a in __addQueue) {
            __entities.add(a)
        }
        __addQueue.clear()

        for (e in __entities) {
            for(c in e.components) {
                c.update(dt)
            }
        }

        var i = 0
        while(i < __entities.count) {
            var e = __entities[i]
            if(e.deleted) {
                for(c in e.components) {
                    c.del()
                }
                __entities.removeAt(i)
            } else {
                i = i + 1
            }
        }

        for (e in __entities) {
            e.removeDeletedComponents()
        }
    }

    static entitiesWithTag(tag) {
        var found = []
        for (e in __entities) {
                if(Bits.checkBitFlag(e.tag, tag)) {
                found.add(e)
            }
        }
        return found
    }

    static entities { __entities }

    static print() {
        System.print("<<<<<<<<<< stats >>>>>>>>>>")
        System.print("Alive: %(__entities.count)")
        var i = 0
        for (e in __entities) {
            System.print("%(i) { Name: %(e.name) Tag:%(e.tag)")
            for(c in e.components) {
                System.print("     %(c.toString)")
            }
            System.print("}")
            i = i + 1
        }
        System.print("<<<<<<<<<< end >>>>>>>>>>")
    }    
}
