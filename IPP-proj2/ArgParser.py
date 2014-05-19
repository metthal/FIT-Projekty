import re
import ErrorCode

class ArgParser:

    allParams = [] # list of lists of parameters
    args = [] # arguments passed by user through CLI

    def __init__(self, allParams, args):
        self.allParams = allParams
        self.args = args
        self.args.pop(0) # pop the script name

    # Validate if entered parameters are from the set of permitted parameters
    # Raises ErrorCode.BadParams exception if not
    # Doesn't check repetition of parameters
    def ValidateParams(self):
        for arg in self.args: # For every entered parameter
            valid = False
            for params in self.allParams: # Traverse through permitted parameters
                for paramStr in params:
                    matches = re.match('^' + paramStr + '$', arg)
                    if (matches != None): # We have found match with any of the permitted parameters
                        valid = True
                        break

            if (valid == False): # We haven't found any match with permitted parameters
                raise Exception(ErrorCode.BadParams)

    # Parse out the information whether specified parameter was entered
    # Does check the reptition of parameters
    # Raises ErrorCode.BadParams if parameters are repeated
    #
    # @param param THe index to the list of permitted parameters which we want to get
    #
    # @return   True    - paramter was found but doesn't contain any data
    #           string  - parameter was found and contained the data represented by returned string
    #           False   - parameter wasn't found
    def GetParam(self, param):
        count = 0
        paramData = True

        for paramStr in self.allParams[param]:
            for arg in self.args:
                matches = re.search('^' + paramStr + '$', arg)
                if (matches != None): # We have found match with premitted parameters
                    count += 1
                    if (len(matches.groups()) > 0): # Parameter contains some data
                        paramData = matches.group(1)

        if (count > 1): # There is more than one parameter specified of this kind
            raise Exception(ErrorCode.BadParams)
        elif (count == 1):
            return paramData

        return False

    # Returns the count of the parameters
    #
    # @return Parameter count without the script name
    def GetParamCount(self):
        return len(self.args)
