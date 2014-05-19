import sys
import re
import Fsm
import Rule
import State
import ErrorCode

class FsmParser:
    # Enumeration of states for lexical analysator
    STATE_WHITESPACE = 0
    STATE_COMMENT = 1
    STATE_SET = 2
    STATE_CHAR_BEGIN = 3
    STATE_CHAR_END = 4
    STATE_ARROW = 5
    STATE_SET_COMMENT = 6
    STATE_CHAR_APOSTROPHE = 7
    STATE_ID = 8
    STATE_ID_COMMENT = 9

    # Enumeration of token types in lexical analysis
    TOKEN_ERROR = 0
    TOKEN_FSM_START = 1
    TOKEN_SET = 2
    TOKEN_ID = 3
    TOKEN_COMMA = 4
    TOKEN_FSM_END = 5
    TOKEN_EOF = 6

    # Enumeration of group types for syntax analysator
    SET_NONE = 0
    SET_STATES = 1
    SET_ALPHABET = 2
    SET_RULES = 3
    SET_END_STATES = 4

    inputFile = None
    inputText = None
    inputTextLength = 0
    readPos = 0
    lastChar = ''
    useLastChar = False
    ignoreCase = False

    # @param filePath File path to the input file
    # @param ignoreCase Indiciates whether the parser is case insensitive
    def __init__(self, filePath = False, ignoreCase = False):
        self.readPos = 0
        self.ignoreCase = ignoreCase
        try:
            if (filePath == False):
                self.inputFile = sys.stdin
            else:
                self.inputFile = open(filePath, mode='r', encoding='utf-8')
        except OSError:
            raise Exception(ErrorCode.InputFileError)

    # Parser runs the lexical and syntax analysis on the input file and tries to parse out the FSM
    # Raises ErrorCode.LexicalOrSyntaxError, ErrorCode.SemanticsError
    #
    # @return The new object of the FSM type
    def Parse(self):
        self.inputText = self.inputFile.read()
        self.inputTextLength = len(self.inputText)

        # First token should be '('
        token = self.__GetNextToken()

        # Empty input is not considered as error
        if (token[0] == FsmParser.TOKEN_EOF):
            raise Exception(ErrorCode.FinishNoError)

        if (token[0] != FsmParser.TOKEN_FSM_START):
            raise Exception(ErrorCode.LexicalOrSyntaxError)

        # Second token should be set of states
        token = self.__GetNextToken()
        if (token[0] != FsmParser.TOKEN_SET):
            raise Exception(ErrorCode.LexicalOrSyntaxError)

        states = self.__ParseToken(token, FsmParser.SET_STATES) # Parse states from the set

        # The comma as the seperator of states and the alphabet sets
        token = self.__GetNextToken()
        if (token[0] != FsmParser.TOKEN_COMMA):
            raise Exception(ErrorCode.LexicalOrSyntaxError)

        # The alphabet set
        token = self.__GetNextToken()
        if (token[0] != FsmParser.TOKEN_SET):
            raise Exception(ErrorCode.LexicalOrSyntaxError)

        alphabet = self.__ParseToken(token, FsmParser.SET_ALPHABET) # Parse the alphabet

        # Another comma as the seperator of alphabet and rules
        token = self.__GetNextToken()
        if (token[0] != FsmParser.TOKEN_COMMA):
            raise Exception(ErrorCode.LexicalOrSyntaxError)

        # The rule set should be there
        token = self.__GetNextToken()
        if (token[0] != FsmParser.TOKEN_SET):
            raise Exception(ErrorCode.LexicalOrSyntaxError)

        self.__ParseToken(token, setType = FsmParser.SET_RULES, customData = (states, alphabet)) # Parse the rules

        # Comma as the separator of the rules and the start state
        token = self.__GetNextToken()
        if (token[0] != FsmParser.TOKEN_COMMA):
            raise Exception(ErrorCode.LexicalOrSyntaxError)

        # Start state
        token = self.__GetNextToken()
        if (token[0] != FsmParser.TOKEN_ID):
            raise Exception(ErrorCode.LexicalOrSyntaxError)

        startState = self.__ParseToken(token) # Parse the start state

        # if the start state is not in the set of states, it is semantics error
        if (startState not in states):
            raise Exception(ErrorCode.SemanticsError)

        # Last comma separating the start state and the set of end states
        token = self.__GetNextToken()
        if (token[0] != FsmParser.TOKEN_COMMA):
            raise Exception(ErrorCode.LexicalOrSyntaxError)

        # Set of the end states
        token = self.__GetNextToken()
        if (token[0] != FsmParser.TOKEN_SET):
            raise Exception(ErrorCode.LexicalOrSyntaxError)

        endStates = self.__ParseToken(token, FsmParser.SET_END_STATES, customData = states) # Parse the end states

        # It should all end with ')'
        token = self.__GetNextToken()
        if (token[0] != FsmParser.TOKEN_FSM_END):
            raise Exception(ErrorCode.LexicalOrSyntaxError)

        # After that, end of file is expected
        token = self.__GetNextToken()
        if (token[0] != FsmParser.TOKEN_EOF):
            raise Exception(ErrorCode.LexicalOrSyntaxError)

        return Fsm.Fsm(states, alphabet, startState, endStates)

    # The second phase of lexical analysator, which parses the data from the concrete token, rather than whole input file
    # which can be heavily obfuscated with the whitespaces and the comments and would be hard to describe it by regular expressions
    # Raises ErrorCode.LexicalOrSyntaxError/ErrorCode.SemanticsError
    #
    # @param token Token to parse out data from
    # @param setType If the token is of type set, this parameter is used to recognize what to parse from the given set
    # @param customData Custom data that can be provided to the parser. Depends on the setType.
    #
    # @return Return parsed data, depending on the setType
    def __ParseToken(self, token, setType = SET_NONE, customData = None):
        if (token[0] == FsmParser.TOKEN_ID): # TOKEN_ID uses only the start state
            matches = re.search(r'^[a-zA-Z]([a-zA-Z0-9_]*[a-zA-Z0-9])?$', token[1])
            if (matches == None):
                raise Exception(ErrorCode.LexicalOrSyntaxError)

            strId = token[1]
            if (self.ignoreCase == True): # Case insensitive - lowcase the whole string
                strId = strId.lower()

            return strId # Return string representing the start state

        elif (token[0] == FsmParser.TOKEN_SET):
            if (setType == FsmParser.SET_STATES): # Set of the states of the whole FSM
                states = {}

                # Empty set of start states is OK
                if (token[1] == ''):
                    return states

                statesList = token[1].split(',') # Split by the comma as the separator of states
                stateRegex = re.compile(r'^\s*([a-zA-Z]([a-zA-Z0-9_]*[a-zA-Z0-9])?)\s*$')
                for state in statesList:
                    match = stateRegex.search(state)
                    if (match == None):
                        raise Exception(ErrorCode.LexicalOrSyntaxError)

                    stateName = match.group(1)
                    if (self.ignoreCase == True): # Case insensitive - lowcase the whole state name
                        stateName = stateName.lower()

                    if (stateName not in states): # This solves the duplicties
                        states[stateName] = State.State(stateName)

                return states # Returns the dictionary containg { name of the state : state object }

            elif (setType == FsmParser.SET_END_STATES): # customData - dictionary of states
                states = set()

                # Empty set of the end states is OK
                if (token[1] == ''):
                    return states

                statesList = token[1].split(',') # Split by the comma as the separator of states
                stateRegex = re.compile(r'^\s*([a-zA-Z]([a-zA-Z0-9_]*[a-zA-Z0-9])?)\s*$')
                for state in statesList:
                    match = stateRegex.search(state)
                    if (match == None):
                        raise Exception(ErrorCode.LexicalOrSyntaxError)

                    stateName = match.group(1)
                    if (self.ignoreCase == True): # Case insensitive - lowcase the whole state name
                        stateName = stateName.lower()

                    if (stateName not in customData): # The end state is not present in the states
                        raise Exception(ErrorCode.SemanticsError)

                    states.add(stateName)

                return states # Returns the set of strings of the end states

            elif (setType == FsmParser.SET_ALPHABET):
                alphabet = set()
                symbolRegex = re.compile(r"^\s*'([\S\t\n\r ])'\s*(,|$)")
                alphStr = token[1]

                # Empty alphabet is not allowed
                if (alphStr == ''):
                    raise Exception(ErrorCode.SemanticsError)

                while True:
                    match = symbolRegex.search(alphStr)
                    if (match == None):
                        raise Exception(ErrorCode.LexicalOrSyntaxError)

                    symbol = match.group(1)
                    if (self.ignoreCase == True): # Case insensitive - lowcase the symbol
                        symbol = symbol.lower()

                    alphabet.add(symbol)
                    alphStr = alphStr[match.end():] # Cut the matched string and leave only the rest of unmatched string

                    if (len(match.group(2)) == 0): # This group can be only comma, or end of the string, in case of len() == 0, we have found the end, thus the last element
                        break

                return alphabet # Return the set of the alphabet

            elif (setType == FsmParser.SET_RULES): # customData - touple of ( states , alphabet )
                ruleRegex = re.compile(r"^\s*(([a-zA-Z]([a-zA-Z0-9_]*[a-zA-Z0-9])?)\s*'([\S\t\n\r ])?'\s*->\s*([a-zA-Z]([a-zA-Z0-9_]*[a-zA-Z0-9])?))\s*(,|$)")
                ruleStr = token[1]

                # Empty set of the rules is OK
                if (ruleStr == ''):
                    return

                while True:
                    match = ruleRegex.search(ruleStr)
                    if (match == None):
                        raise Exception(ErrorCode.LexicalOrSyntaxError)

                    startState = match.group(2)
                    acceptedSymbol = match.group(4) if (match.group(4) != None) else '' # If there is no match in group 4, epsilon rule is found
                    endState = match.group(5)
                    ruleStr = ruleStr[match.end():] # Cut the matched string and leave only the rest of unmatched string

                    if (self.ignoreCase == True): # Case insensitive - lowcase eveything
                        startState = startState.lower()
                        acceptedSymbol = acceptedSymbol.lower()
                        endState = endState.lower()

                    # If the start and the end state are not present in states of FSM, semantics error
                    if ((startState not in customData[0]) or (endState not in customData[0])):
                        raise Exception(ErrorCode.SemanticsError)

                    # If the accepted symbol is not present in the alphabet (except epsilon, which is not mentioned in the alphabet at all), then semantics error
                    if ((acceptedSymbol != '') and (acceptedSymbol not in customData[1])):
                        raise Exception(ErrorCode.SemanticsError)

                    # add the rule into the rule dictionry of the start state
                    customData[0][startState].AddRule(Rule.Rule(startState, acceptedSymbol, endState))

                    if (len(match.group(7)) == 0): # This group can be only comma, or end of the string, in case of len() == 0, we have found the end, thus the last element
                        break

    # Lexical anyliser which return the next token from the input file
    #
    # @return Touple of ( ToykenType, tokenData )
    def __GetNextToken(self):
        state = FsmParser.STATE_WHITESPACE
        data = ''
        # only goes up to the length of the input file
        for i in range(self.readPos, self.inputTextLength):
            if (self.useLastChar == False): # Switching mechanism used when the read token in the last iteration needs to be used as the data for the next token
                inputChar = self.inputText[i]
                self.readPos += 1
                self.lastChar = inputChar
            else:
                inputChar = self.lastChar
                self.useLastChar = False

            if (state == FsmParser.STATE_WHITESPACE):
                if (inputChar == '#'):
                    state = FsmParser.STATE_COMMENT
                elif (inputChar == '('):
                    return (FsmParser.TOKEN_FSM_START, None)
                elif (inputChar == '{'):
                    state = FsmParser.STATE_SET
                elif (inputChar == ','):
                    return (FsmParser.TOKEN_COMMA, None)
                elif (inputChar == ')'):
                    return (FsmParser.TOKEN_FSM_END, None)
                elif (inputChar != '\n' and inputChar != '\t' and inputChar != ' ' and inputChar != '\r'):
                    state = FsmParser.STATE_ID
                    data += inputChar

            elif (state == FsmParser.STATE_COMMENT):
                if (inputChar == '\n'):
                    state = FsmParser.STATE_WHITESPACE

            elif (state == FsmParser.STATE_SET_COMMENT):
                if (inputChar == '\n'):
                    state = FsmParser.STATE_SET

            elif (state == FsmParser.STATE_ID_COMMENT):
                if (inputChar == '\n'):
                    state = FsmParser.STATE_ID

            elif (state == FsmParser.STATE_SET):
                if (inputChar == '#'):
                    state = FsmParser.STATE_SET_COMMENT
                elif (inputChar == '\''):
                    state = FsmParser.STATE_CHAR_BEGIN
                    data += inputChar
                elif (inputChar == '}'):
                    return (FsmParser.TOKEN_SET, data)
                elif (inputChar == '-'):
                    state = FsmParser.STATE_ARROW
                    data += inputChar
                else:
                    data += inputChar

            elif (state == FsmParser.STATE_CHAR_BEGIN):
                if (inputChar == '\''):
                    state = FsmParser.STATE_CHAR_APOSTROPHE
                else:
                    state = FsmParser.STATE_CHAR_END
                data += inputChar

            elif (state == FsmParser.STATE_CHAR_END):
                if (inputChar == '\''):
                    state = FsmParser.STATE_SET
                    data += inputChar
                else:
                    return (FsmParser.TOKEN_ERROR, None)

            elif (state == FsmParser.STATE_CHAR_APOSTROPHE):
                if (inputChar == '\''):
                    state = FsmParser.STATE_CHAR_END
                elif (inputChar == '\n' or inputChar == '\t' or inputChar == ' ' or inputChar == '\r'):
                    state = FsmParser.STATE_SET
                elif (inputChar == '-'):
                    state = FsmParser.STATE_ARROW
                    data += inputChar
                else:
                    return (FsmParser.TOKEN_ERROR, None)

            elif (state == FsmParser.STATE_ARROW):
                if (inputChar == '>'):
                    state = FsmParser.STATE_SET
                    data += inputChar
                else:
                    return (FsmParser.TOKEN_ERROR, None)

            elif (state == FsmParser.STATE_ID):
                if (inputChar == '#'):
                    state = FsmParser.STATE_ID_COMMENT
                elif (inputChar == ','):
                    self.useLastChar = True
                    return (FsmParser.TOKEN_ID, data)
                elif (inputChar == '\n' or inputChar == '\t' or inputChar == ' ' or inputChar == '\r'):
                    return (FsmParser.TOKEN_ID, data)
                else:
                    data += inputChar

        return (FsmParser.TOKEN_EOF, None)
