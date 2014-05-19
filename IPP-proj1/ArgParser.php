<?php

#SYN:xmilko01

class ArgParser
{
    const PARAM_HELP     = 0;
    const PARAM_FORMAT   = 1;
    const PARAM_INPUT    = 2;
    const PARAM_OUTPUT   = 3;
    const PARAM_BR       = 4;

    private static $allParams = array('--help', '--format=(.+)', '--input=(.+)', '--output=(.+)', '--br');

    public static function GetParamCount($args)
    {
        return count($args) - 1;
    }

    public static function ParamsAreValid($args)
    {
        $helpRet = ArgParser::GetParam($args, ArgParser::PARAM_HELP);

        // --help specified but the count of parameter is not 1
        if (!is_null($helpRet) && ArgParser::GetParamCount($args) != 1)
            throw new Exception('Invalid parameters - --help not alone', EXIT_BAD_PARAMS);

        // remove script name
        unset($args[0]);

        foreach ($args as $arg)
        {
            $valid = false;
            foreach (ArgParser::$allParams as $param)
            {
                $pattern = '/^' . $param . '$/';
                preg_match($pattern, $arg, $matches);

                if (count($matches) > 0)
                {
                    $valid = true;
                    break;
                }
            }

            if ($valid == false)
                throw new Exception('Invalid parameters', EXIT_BAD_PARAMS);
        }

        return true;
    }

    public static function GetParam($args, $param)
    {
        // remove script name
        unset($args[0]);

        $pattern = '/^' . ArgParser::$allParams[$param] . '$/';
        $val = null;
        $count = 0;

        foreach ($args as $arg)
        {
            preg_match($pattern, $arg, $matches);

            if (count($matches) > 0)
            {
                $val = $matches;
                $count++;
                if ($count > 1)
                    throw new Exception('Same parameter more times', EXIT_BAD_PARAMS);
            }
        }

        return ($count == 1) ? $val : null;
    }
}

?>
