import State
import Rule

class Fsm:
    states = None # Dictionary of { state name : state object }
    alphabet = None # Set of symbols that are accepted
    startState = None # String representing the name of the start state
    endStates = None # Set of state names representing the end states

    def __init__(self, states, alphabet, startState, endStates):
        self.states = states
        self.alphabet = alphabet
        self.startState = startState
        self.endStates = endStates

    # Removes the epsilon rules from the FSM. Edits in place.
    def RemoveEpsRules(self):
        newStates = {}
        newEndStates = set()
        for stateName in self.states:
            visitedStates = set() # States that have been already visited by algorithm
            statesToVisit = [] # Stack of the states that should algorithm visit
            state = self.states[stateName]

            newStates[stateName] = State.State(stateName) # Create new state with the same name as the currently iterated state

            if (stateName in self.endStates): # If the currently iterated state is end state, add it to the new end state set (eps-closure of the state contains always contains itself)
                newEndStates.add(stateName)

            for symbol in state.ruleMap:
                for rule in state.ruleMap[symbol]: # For every rule going from this state
                    if (symbol == ''): # If it is epsilon rule
                        statesToVisit.append(rule.endState) # Push it on the to-visit stack
                    else:
                        newStates[stateName].AddRule(Rule.Rule(rule.startState, rule.acceptedSymbol, rule.endState)) # Else make copy of the rule

            while (len(statesToVisit) > 0): # Something is on the stack
                endStateName = statesToVisit.pop()
                endState = self.states[endStateName]
                visitedStates.add(endStateName)

                if (endStateName in self.endStates): # If the visited state is end state, add it to the new end state set as this state is in eps closure of currently iterated state
                    newEndStates.add(stateName)

                for symbol in endState.ruleMap:
                    for rule in endState.ruleMap[symbol]: # For every rule in the currently visited state
                        if (symbol == ''): # If it is epsilon rule
                            if (rule.endState not in visitedStates): # And end state of rule wasn't visited
                                statesToVisit.append(rule.endState) # Push it on the to-visit stack
                        else:
                            newStates[stateName].AddRule(Rule.Rule(stateName, symbol, rule.endState)) # Else make copy of the rule

        self.states = newStates # Rewrite the old states with the states we have just made
        self.endStates = newEndStates # Rewrite the old end states with the new end states

    # Performs determinization of the FSM. Edits in place.
    def Determinize(self):
        visitedStates = set() # States that have been visited by the algorithm
        statesToVisit = [ State.State(self.startState) ] # Stack of the states that should this algorithm visit, at the beginning filled with the start state
        newStates = { self.startState : statesToVisit[0] }
        newEndStates = set()
        while (len(statesToVisit) > 0): # While the to-visit stack is not empty
            state = statesToVisit.pop()
            if (str(state) in visitedStates):
                continue

            visitedStates.add(str(state))

            # This builds a dummy state consisting of all rules that come from its component states
            dummyState = State.State(state.names) # Make dummy state with the same name as currently visited state
            for name in state.names: # For every single state the currently visited state is made of (e.g. a_b_c is made of states a,b and c)
                substate = self.states[name]
                for symbol in substate.ruleMap:
                    for rule in substate.ruleMap[symbol]: # For every rule in the states the currently visited state is made of
                        dummyState.AddRule(Rule.Rule(str(state), symbol, rule.endState)) # Add the copy of the rule into the dummy state

            # If the currently visited state is in the end states, add it also into the new set of end states
            # This piece of code is here because there can be state that has no rules that come from it, so won't be recognized later due to this fact
            if (str(state) in self.endStates):
                newEndStates.add(str(state))

            # For every accepted symbol in this state
            for symbol in dummyState.ruleMap:
                if (len(dummyState.ruleMap[symbol]) == 1): # If there is only one rule for this symbol
                    onlyRule = list(dummyState.ruleMap[symbol])[0] # Get this rule
                    if (onlyRule.endState not in newStates):
                        newStates[onlyRule.endState] = State.State(onlyRule.endState)

                    state.AddRule(Rule.Rule(str(state), symbol, onlyRule.endState)) # Copy this rule

                    if (onlyRule.endState not in visitedStates):
                        statesToVisit.append(newStates[onlyRule.endState]) # And put the end state of the rule into to to-visit stack
                else:
                    # The accepted symbol is used for more than 1 rules, we will have to merge end states of these rules
                    mergedStates = []
                    startContainsEndState = False
                    endContainsEndState = False

                    # Build the list of the states to merge
                    for rule in dummyState.ruleMap[symbol]: # For every rule that uses this symbol
                        if (rule.endState in set(mergedStates)): # If this state is already in the list of merged states, don't add it
                            continue

                        mergedStates.append(rule.endState)
                        if (rule.startState in self.endStates): # If some of the states is also the end states, add this currently visited state into the set of the end states
                            startContainsEndState = True

                        if (rule.endState in self.endStates): # If some of the states is also the end states, add this whole new state into the set of the end states
                            endContainsEndState = True

                    newState = State.State(mergedStates) # Create the merged state
                    if (str(newState) in newStates):
                        newState = newStates[str(newState)]
                    else:
                        newStates[str(newState)] = newState

                    if (startContainsEndState == True):
                        newEndStates.add(str(state))

                    if (endContainsEndState == True):
                        newEndStates.add(str(newState))

                    state.AddRule(Rule.Rule(str(state), symbol, str(newState))) # Create the new rule going from the visited state into the merged state
                    if (str(newState) not in visitedStates): # Add the merged state into the to-visit stack
                        statesToVisit.append(newState)

        self.states = newStates
        self.endStates = newEndStates

    # Well specify the FSM. Removes the nonterminating states and makes the FSM Complete.  Edits in place.
    #
    # @param ignoreCase Indicates whether the FSM is case insensitive.
    def WellSpecify(self, ignoreCase):
        newStates = {}

        # First we need to make Complete FSM
        # Make copy of all end states
        for stateName in self.endStates:
            newStates[stateName] = State.State(self.states[stateName].names)

        foundNewState = True
        while (foundNewState == True): # While we have found some new state to add to our Complete FSM
            foundNewState = False

            for stateName in self.states:
                state = self.states[stateName]
                for symbol in state.ruleMap:
                    for rule in state.ruleMap[symbol]: # For every single rule in all the states the original FSM contains
                        if (rule.endState in newStates): # If the end state of the rule is in our new states set, add the start also to this new states set
                            if (rule.startState not in newStates):
                                newStates[rule.startState] = State.State(rule.startState)

                            if (foundNewState == False):
                                foundNewState = newStates[rule.startState].AddRule(Rule.Rule(rule.startState, symbol, rule.endState))

        # Now we can make the FSM Well specified
        # Make the qFALSE state which will be our only nonterminating state
        qfalseState = State.State('qFALSE' if (ignoreCase == False) else 'qfalse')
        for symbol in self.alphabet: # Add rules for all alphabet symbols that goes from qFALSE to qFALSE
            qfalseState.AddRule(Rule.Rule(str(qfalseState), symbol, str(qfalseState)))

        neededTransitions = {}
        for stateName in newStates: # For every state in our new states set
            state = newStates[stateName]
            usedSymbols = set()
            for symbol in state.ruleMap: # Store all the symbols from the alphabet this state accept and goes into anthoer state
                usedSymbols.add(symbol)

            neededSymbols = self.alphabet - usedSymbols # Make symmetric difference with the alphabet to get the symbols this state doesn't accept
            for symbol in neededSymbols:
                state.AddRule(Rule.Rule(stateName, symbol, str(qfalseState))) # Add the rules for the symbols this state doesn't accept to go into the qFALSE state

        newStates[str(qfalseState)] = qfalseState
        self.states = newStates

    # Analyzes the input string, whether FSM accepts the string or not
    # Raises exception ErrorCode.SymbolNotInAlphabet if the character from string is not in alphabet of FSM
    #
    # @param string The input string to analyze
    #
    # @return Return 1 if FSM accepts the string, 0 if not
    def AcceptString(self, string):
        state = self.states[self.startState]

        for i in range(0, len(string)): # Traverse through the whole string
            symbol = string[i]

            if (symbol not in self.alphabet): # Symbol not in alphabet of the FSM
                raise Exception(ErrorCode.SymbolNotInAlphabet)

            if (symbol not in state.ruleMap): # This state doesn't accept this symbol
                return 0

            state = self.states[list(state.ruleMap[symbol])[0].endState] # Transition to the new state

        if (str(state) in self.endStates): # If we finished in the state that is end state
            return 1

        return 0

    def __str__(self):
        output = '(\n'

        # States
        output += '{'
        states = list(self.states.keys())
        states.sort()
        for state in states:
            output += state + ', '
        output = output[:-2]
        output += '},\n'

        # Alphabet
        output += '{'
        alphabet = list(self.alphabet)
        alphabet.sort()
        for symbol in alphabet:
            if (symbol == '\''):
                output += '\'\'\'\', '
            else:
                output += '\'' + symbol + '\', '
        output = output[:-2]
        output += '},\n'

        # Rules
        output += '{\n'
        rules = []
        for state in self.states:
            for symbol in self.states[state].ruleMap:
                    for rule in self.states[state].ruleMap[symbol]:
                        rules.append(str(rule))
        rules.sort()
        for rule in rules:
            output += rule + ',\n'

        if (len(rules) > 0):
            output = output[:-2]
            output += '\n'

        output += '},\n'

        # Start state
        output += self.startState + ',\n'

        # End states
        output += '{'
        endStates = list(self.endStates)
        endStates.sort()
        for state in endStates:
            output += state + ', '

        if (len(self.endStates) > 0):
            output = output[:-2]

        output += '}\n'

        output += ')'
        return output

