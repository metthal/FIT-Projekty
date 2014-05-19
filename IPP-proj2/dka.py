#!/usr/bin/python3
#
#DKA:xmilko01

import sys
import ArgParser
import FsmParser
import ErrorCode

# Enumeration of parmitted parameters
PARAM_HELP = 0
PARAM_INPUT = 1
PARAM_OUTPUT = 2
PARAM_NO_EPS_RULES = 3
PARAM_DETERM = 4
PARAM_CASE_INSENSITIVE = 5
PARAM_WSFA = 6
PARAM_ANALYZE = 7

# Flags for recogintion of simultaneous parameters which are not allowed that way
FLAGS_NONE = 0
FLAGS_EPS = 1
FLAGS_DET = 2
FLAGS_WSFA = 4
FLAGS_ANALYZE = 8

try:
    allParams = [ [ '--help' ], [ '--input=(.+)' ],  [ '--output=(.+)' ], [ '-e', '--no-epsilon-rules' ], [ '-d', '--determinization' ], [ '-i', '--case-insensitive' ],
        [ '--wsfa' ], [ '--analyze-string=(.+)' ] ]
    argParser = ArgParser.ArgParser(allParams, sys.argv)

    argParser.ValidateParams()

    # --help parameter should be always specified alone
    if (argParser.GetParam(PARAM_HELP) == True):
        if (argParser.GetParamCount() != 1):
            raise Exception(ErrorCode.BadParams)

        raise Exception(ErrorCode.FinishNoError)

    # Try to parse out all parameters
    flags = FLAGS_NONE
    eps = argParser.GetParam(PARAM_NO_EPS_RULES)
    inputFile = argParser.GetParam(PARAM_INPUT)
    outputFile = argParser.GetParam(PARAM_OUTPUT)
    det = argParser.GetParam(PARAM_DETERM)
    case = argParser.GetParam(PARAM_CASE_INSENSITIVE)
    wsfa = argParser.GetParam(PARAM_WSFA)
    analyze = argParser.GetParam(PARAM_ANALYZE)

    # Set flags for parameters which are not allowed simultaneously
    flags |= FLAGS_EPS if (eps == True) else FLAGS_NONE
    flags |= FLAGS_DET if (det == True) else FLAGS_NONE
    flags |= FLAGS_WSFA if (wsfa == True) else FLAGS_NONE
    flags |= FLAGS_ANALYZE if (analyze != False) else FLAGS_NONE

    # Check if only one of these is specified
    if ((flags != FLAGS_NONE) and (flags != FLAGS_EPS) and (flags != FLAGS_DET) and (flags != FLAGS_WSFA) and (flags != FLAGS_ANALYZE)):
        raise Exception(ErrorCode.BadParams)

    # Try to open output file, in case of fail, raise ErrorCode.OutputFileError
    try:
        # For not specified --output parameters, use stdout
        if (outputFile == False):
            outputFile = sys.stdout
        else:
            outputFile = open(outputFile, mode='w', encoding='utf-8')
    except OSError:
        raise Exception(ErrorCode.OutputFileError)

    fsmParser = FsmParser.FsmParser(filePath = inputFile, ignoreCase = case)
    fsm = fsmParser.Parse()

    if (eps == True): # -e | --no-epsilon-rules
        fsm.RemoveEpsRules()
    elif (det == True): # -d | --determinization
        fsm.RemoveEpsRules()
        fsm.Determinize()
    elif (wsfa == True): # --wsfa
        fsm.RemoveEpsRules()
        fsm.Determinize()
        fsm.WellSpecify(case)
    elif (analyze != False): # --analyze-string
        fsm.RemoveEpsRules()
        fsm.Determinize()
        print(fsm.AcceptString(analyze), file = outputFile, end = '')
        raise Exception(ErrorCode.FinishNoError)

    print(fsm, file = outputFile, end = '')

except KeyboardInterrupt: # Ctrl-C
    sys.exit(ErrorCode.Unexpected)
except Exception as e:
    sys.exit(int(str(e))) # Take out the exception value and use it as exit code
