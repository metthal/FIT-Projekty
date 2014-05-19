class Rule:
    startState = None
    acceptedSymbol = None
    endState = None

    def __init__(self, startState, acceptedSymbol, endState):
        self.startState = startState
        self.acceptedSymbol = acceptedSymbol
        self.endState = endState

    def __str__(self):
        symbol = self.acceptedSymbol

        if (symbol == ''):
            symbol = ''
        elif (symbol == '\''):
            symbol = '\'\''

        return self.startState + ' \'' + symbol + '\' ' + '-> ' + self.endState

    def __repr__(self):
        return self.__str__()

    def __eq__(self, other):
        return (self.startState == other.startState) and (self.acceptedSymbol == other.acceptedSymbol) and (self.endState == other.endState)

    def __ne__(self, other):
        return not self.__eq__(other)

    def __hash__(self):
        return id(self)

