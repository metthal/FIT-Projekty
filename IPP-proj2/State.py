class State:
    names = []
    ruleMap = None

    def __init__(self, name):
        if (type(name) is list):
            self.names = list(set(name))
            self.names.sort()
        elif (type(name) is str):
            self.names = [ name ]
        else:
            raise Exception(99)

        self.ruleMap = {}

    def AddRule(self, rule):
        if (self.ruleMap == None):
            self.ruleMap = {}

        if (rule.acceptedSymbol not in self.ruleMap):
            self.ruleMap[rule.acceptedSymbol] = set()

        for symbol in self.ruleMap:
            for otherRule in self.ruleMap[symbol]:
                if (rule == otherRule):
                    return False

        self.ruleMap[rule.acceptedSymbol].add(rule)
        return True

    def __str__(self):
        finalName = ''
        for name in self.names:
            finalName += name + '_'

        finalName = finalName[:-1] # remove last underscore
        return finalName

    def __repr__(self):
        name = self.__str__()
        name += ':['

        if (len(self.ruleMap) > 0):
            for symbol in self.ruleMap:
                name += '\'' + symbol + '\'' + ' - ' + str(self.ruleMap[symbol]) + ', '

            name = name[:-2]

        name += ']'
        return name

    def __eq__(self, other):
        return self.names == other.names

    def __ne__(self, other):
        return not self.__eq__(other)

    def __hash__(self):
        return id(self)

