<?php

#SYN:xmilko01

require_once 'Tag.php';

class FormatType
{
    const BOLD      = 0;
    const ITALIC    = 1;
    const UNDERLINE = 2;
    const TELETYPE  = 3;
    const SIZE      = 4;
    const COLOR     = 5;
    const BR        = 6;

    public function __construct($type, $value)
    {
        $this->type = $type;
        $this->value = $value;
    }

    public function GetType()
    {
        return $this->type;
    }

    public function GetValue()
    {
        return $this->value;
    }

    private $type;
    private $value;
}

class Format
{
    public function __construct($regex)
    {
        $this->regex = '/' . $regex . '/';
        $this->typeArray = array();
    }

    public function GetRegex()
    {
        return $this->regex;
    }

    public function GetFormatTypes()
    {
        return $this->typeArray;
    }

    public function AddFormatType($formatType)
    {
        array_push($this->typeArray, $formatType);
    }

    private $regex;
    private $typeArray;
}

class Formatter
{
    public function __construct($formatFilePath, $inputFilePath, $outputFilePath, $newline)
    {
        if (!is_null($formatFilePath) && is_file($formatFilePath) && is_readable($formatFilePath))
        {
            $this->formatFile = fopen($formatFilePath, "r");
            if ($this->formatFile == false)
                $this->formatFile = null;
        }
        else
            $this->formatFile = null;

        if ($inputFilePath == STDIN)
        {
            $this->inputFile = STDIN;
        }
        else if (!is_null($inputFilePath) && is_file($inputFilePath) && is_readable($inputFilePath))
        {
            $this->inputFile = fopen($inputFilePath, "r");
            if ($this->inputFile == false)
                throw new Exception('Failed to open input file', EXIT_BAD_INPUT_FILE);
        }
        else
            throw new Exception('Unable to read input file', EXIT_BAD_INPUT_FILE);

        if ($outputFilePath == STDOUT)
        {
            $this->outputFile = STDOUT;
        }
        else if (!is_null($outputFilePath))
        {
            $this->outputFile = @fopen($outputFilePath, "w");
            if ($this->outputFile == false)
            {
                $this->outputFile = null;
                throw new Exception('Failed to open input file', EXIT_BAD_OUTPUT_FILE);
            }
        }
        else
            throw new Exception('Unable to write to the output file', EXIT_BAD_OUTPUT_FILE);

        $this->formatArray = array();
        $this->tags = array();
        $this->input = '';
        $this->newline = $newline;
    }

    public function ReadFormatFile()
    {
        if (!is_null($this->formatFile))
        {
            while (($formatLine = fgets($this->formatFile)) != false)
            {
                $this->ParseFormatLine($formatLine);
            }
        }

        // insert <br/> element
        if ($this->newline)
        {
            $offset = -1;
            $brFormatType = new FormatType(FormatType::BR, null);
            while (($offset = strpos($this->input, "\n", $offset + 1)) !== false)
            {
                if (!array_key_exists($offset, $this->tags))
                    $this->tags = $this->tags + array($offset => array(array(), array(new Tag($brFormatType, false))));
                else
                    array_push($this->tags[$offset][1], new Tag($brFormatType, false));
            }
        }

        // sort tags by offset
        ksort($this->tags);

        $currentPos = 0;
        foreach ($this->tags as $offset => $tagArray)
        {
            $str = substr($this->input, $currentPos, $offset - $currentPos);
            fwrite($this->outputFile, $str);
            $currentPos += strlen($str);

            // closing tags are printed first in reverse order
            foreach (array_reverse($tagArray[0]) as $tag)
                fwrite($this->outputFile, $tag->ToString());

            // opening tags
            foreach ($tagArray[1] as $tag)
                fwrite($this->outputFile, $tag->ToString());
        }

        fwrite($this->outputFile, substr($this->input, $currentPos));
    }

    public function Dispose()
    {
        if (!is_null($this->outputFile))
            fclose($this->outputFile);
    }

    public function ReadInputFile()
    {
        if (!$this->inputFile)
            throw new Exception('Formatter::ReadInputFile: inputFile is null', EXIT_UNKNOWN);

        $this->input = '';
        while (($line = fgets($this->inputFile)) != false)
            $this->input = $this->input . $line;
    }

    private function ParseFormatLine($line)
    {
        if (trim($line) == '')
            return;

        preg_match('/^([\S ]+)\t+([\S\t ]+)$/', $line, $matches);
        if (count($matches) == 0)
            throw new Exception('Bad line in format file', EXIT_BAD_FORMAT_FILE);

        $regex = $this->TranslateRegex($matches[1]);
        if (is_null($regex))
            throw new Exception('Bad regex in format file', EXIT_BAD_FORMAT_FILE);

        $format = new Format($regex);
        array_push($this->formatArray, $format);

        $this->ParseFormatType($format, $matches[2]);

        preg_match_all($format->GetRegex(), $this->input, $matches, PREG_OFFSET_CAPTURE);
        foreach ($matches[0] as $matchInfo)
        {
            if ($matchInfo[0] == '')
                continue;

            $formatTypes = $format->GetFormatTypes();
            $len = strlen($matchInfo[0]);
            foreach ($formatTypes as $formatType)
            {
                // opening tag
                if (!array_key_exists($matchInfo[1], $this->tags))
                    $this->tags = $this->tags + array($matchInfo[1] => array(array(), array(new Tag($formatType, false))));
                else
                    array_push($this->tags[$matchInfo[1]][1], new Tag($formatType, false));

                // closing tag
                if (!array_key_exists($matchInfo[1] + $len, $this->tags))
                    $this->tags = $this->tags + array(($matchInfo[1] + $len) => array(array(new Tag($formatType, true)), array()));
                else
                    array_push($this->tags[$matchInfo[1] + $len][0], new Tag($formatType, true));
            }
        }
    }

    private function ParseFormatType($format, $line)
    {
        $line = preg_replace('/[\t ]*(,)[\t ]*/', '\1', $line);
        $formatStrArray = preg_split('/,/', $line);

        foreach ($formatStrArray as $formatStr)
        {
            if ($formatStr == '')
                throw new Exception('Bad format (empty) in format file', EXIT_BAD_FORMAT_FILE);

            $type = null;
            $value = null;

            switch ($formatStr[0])
            {
                case 'b':
                    if (strcmp($formatStr, 'bold') != 0)
                        throw new Exception('Bad format (b) in format file', EXIT_BAD_FORMAT_FILE);

                    $type = FormatType::BOLD;
                    break;
                case 'c':
                    preg_match('/^color:([0-9A-Fa-f]{6})$/', $formatStr, $colorMatches);
                    if (count($colorMatches) == 0)
                        throw new Exception('Bad format (c) in format file', EXIT_BAD_FORMAT_FILE);

                    $type = FormatType::COLOR;
                    $value = $colorMatches[1];
                    break;
                case 'i':
                    if (strcmp($formatStr, 'italic') != 0)
                        throw new Exception('Bad format (i) in format file', EXIT_BAD_FORMAT_FILE);

                    $type = FormatType::ITALIC;
                    break;
                case 's':
                    preg_match('/^size:([1-7])$/', $formatStr, $sizeMatches);
                    if (count($sizeMatches) == 0)
                        throw new Exception('Bad format (s) in format file', EXIT_BAD_FORMAT_FILE);

                    $type = FormatType::SIZE;
                    $value = $sizeMatches[1];
                    break;
                case 't':
                    if (strcmp($formatStr, 'teletype') != 0)
                        throw new Exception('Bad format (t) in format file', EXIT_BAD_FORMAT_FILE);

                    $type = FormatType::TELETYPE;
                    break;
                case 'u':
                    if (strcmp($formatStr, 'underline') != 0)
                        throw new Exception('Bad format (u) in format file', EXIT_BAD_FORMAT_FILE);

                    $type = FormatType::UNDERLINE;
                    break;
                default:
                    throw new Exception('Bad format (unknown) in format file', EXIT_BAD_FORMAT_FILE);
            }

            $format->AddFormatType(new FormatType($type, $value));
        }
    }

    const STATE_START           = 0;
    const STATE_NOT             = 1;
    const STATE_ESCAPE          = 2;
    const STATE_CHAR            = 3;
    const STATE_OR              = 4;
    const STATE_DOT             = 5;
    const STATE_ITERATION       = 6;
    const STATE_PITERATION      = 7;
    const STATE_BRACKET         = 8;
    const STATE_CLOSE_BRACKET   = 9;

    private function TranslateRegex($ifjRegex)
    {
        if (is_null($ifjRegex))
            throw new Exception('Formatter::TranslateRegex: regex is null', EXIT_UNKNOWN);

        $phpRegex = "";
        $state = Formatter::STATE_START;
        $len = strlen($ifjRegex);
        for ($i = 0; $i < $len; $i++)
        {
            switch ($state)
            {
                case Formatter::STATE_START:
                    if ($ifjRegex[$i] == '.' || $ifjRegex[$i] == '|' || $ifjRegex[$i] == '*' || $ifjRegex[$i] == '+' || $ifjRegex[$i] == ')')
                    {
                        return null;
                    }
                    else if ($ifjRegex[$i] == '!')
                    {
                        $phpRegex = $phpRegex . '[^';
                        $state = Formatter::STATE_NOT;
                    }
                    else if ($ifjRegex[$i] == '(')
                    {
                        $phpRegex = $phpRegex . '(';
                        $state = Formatter::STATE_BRACKET;
                    }
                    else if ($ifjRegex[$i] == '%')
                    {
                        $phpRegex = $phpRegex . '[';
                        $state = Formatter::STATE_ESCAPE;
                    }
                    else if ($ifjRegex[$i] == '{' || $ifjRegex[$i] == '}' || $ifjRegex[$i] == '[' || $ifjRegex[$i] == ']' || $ifjRegex[$i] == '?'
                            || $ifjRegex[$i] == '/' || $ifjRegex[$i] == '^' || $ifjRegex[$i] == '$' || $ifjRegex[$i] == '\\')
                    {
                        $phpRegex = $phpRegex . '\\' . $ifjRegex[$i];
                        $state = Formatter::STATE_CHAR;
                    }
                    else if (ord($ifjRegex[$i]) >= 32)
                    {
                        $phpRegex = $phpRegex . $ifjRegex[$i];
                        $state = Formatter::STATE_CHAR;
                    }
                    else
                        return null;
                    break;
                case Formatter::STATE_NOT:
                    if (   $ifjRegex[$i] == '.' || $ifjRegex[$i] == '|' || $ifjRegex[$i] == '*' || $ifjRegex[$i] == '+'
                        || $ifjRegex[$i] == '!' || $ifjRegex[$i] == '(' || $ifjRegex[$i] == ')')
                    {
                        return null;
                    }
                    else if ($ifjRegex[$i] == '%')
                    {
                        $state = Formatter::STATE_ESCAPE;
                    }
                    else if ($ifjRegex[$i] == '{' || $ifjRegex[$i] == '}' || $ifjRegex[$i] == '[' || $ifjRegex[$i] == ']' || $ifjRegex[$i] == '?'
                            || $ifjRegex[$i] == '/' || $ifjRegex[$i] == '^' || $ifjRegex[$i] == '$' || $ifjRegex[$i] == '\\')
                    {
                        $phpRegex = $phpRegex . '\\' . $ifjRegex[$i];
                        $state = Formatter::STATE_CHAR;
                    }
                    else if (ord($ifjRegex[$i]) >= 32)
                    {
                        $phpRegex = $phpRegex . $ifjRegex[$i] . ']';
                        $state = Formatter::STATE_CHAR;
                    }
                    else
                        return null;
                    break;
                case Formatter::STATE_ESCAPE:
                    switch ($ifjRegex[$i])
                    {
                        case 's':
                            $phpRegex = $phpRegex . ' \\t\\r\\n\\f\\v';
                            break;
                        case 'a':
                            $phpRegex = $phpRegex . '\\S\\s';
                            break;
                        case 'd':
                            $phpRegex = $phpRegex . '\\d';
                            break;
                        case 'l':
                            $phpRegex = $phpRegex . 'a-z';
                            break;
                        case 'L':
                            $phpRegex = $phpRegex . 'A-Z';
                            break;
                        case 'w':
                            $phpRegex = $phpRegex . 'a-zA-Z';
                            break;
                        case 'W':
                            $phpRegex = $phpRegex . 'a-zA-Z0-9';
                            break;
                        case 't':
                            $phpRegex = $phpRegex . '\\t';
                            break;
                        case 'n':
                            $phpRegex = $phpRegex . '\\n';
                            break;
                        case '.':
                        case '|':
                        case '+':
                        case '*':
                        case '(':
                        case ')':
                            $phpRegex = $phpRegex . '\\' . $ifjRegex[$i];
                            break;
                        case '!':
                        case '%':
                            $phpRegex = $phpRegex . $ifjRegex[$i];
                            break;
                        default:
                            return null;
                    }
                    $state = Formatter::STATE_CHAR;
                    $phpRegex = $phpRegex . ']';
                    break;
                case Formatter::STATE_CHAR:
                    if ($ifjRegex[$i] == '.')
                    {
                        $state = Formatter::STATE_DOT;
                    }
                    else if ($ifjRegex[$i] == '|')
                    {
                        $phpRegex = $phpRegex . '|';
                        $state = Formatter::STATE_OR;
                    }
                    else if ($ifjRegex[$i] == '*')
                    {
                        $phpRegex = $phpRegex . '*';
                        $state = Formatter::STATE_ITERATION;
                    }
                    else if ($ifjRegex[$i] == '+')
                    {
                        $phpRegex = $phpRegex . '+';
                        $state = Formatter::STATE_PITERATION;
                    }
                    else if ($ifjRegex[$i] == '!')
                    {
                        $phpRegex = $phpRegex . '[^';
                        $state = Formatter::STATE_NOT;
                    }
                    else if ($ifjRegex[$i] == '(')
                    {
                        $phpRegex = $phpRegex . '(';
                        $state = Formatter::STATE_BRACKET;
                    }
                    else if ($ifjRegex[$i] == ')')
                    {
                        $phpRegex = $phpRegex . ')';
                        $state = Formatter::STATE_CLOSE_BRACKET;
                    }
                    else if ($ifjRegex[$i] == '%')
                    {
                        $phpRegex = $phpRegex . '[';
                        $state = Formatter::STATE_ESCAPE;
                    }
                    else if ($ifjRegex[$i] == '{' || $ifjRegex[$i] == '}' || $ifjRegex[$i] == '[' || $ifjRegex[$i] == ']' || $ifjRegex[$i] == '?'
                            || $ifjRegex[$i] == '/' || $ifjRegex[$i] == '^' || $ifjRegex[$i] == '$' || $ifjRegex[$i] == '\\')
                    {
                        $phpRegex = $phpRegex . '\\' . $ifjRegex[$i];
                        $state = Formatter::STATE_CHAR;
                    }
                    else if (ord($ifjRegex[$i]) >= 32)
                    {
                        $phpRegex = $phpRegex . $ifjRegex[$i];
                        $state = Formatter::STATE_CHAR;
                    }
                    else
                        return null;
                    break;
                case Formatter::STATE_OR:
                    if (   $ifjRegex[$i] == '.' || $ifjRegex[$i] == '|' || $ifjRegex[$i] == '*' || $ifjRegex[$i] == '+'
                        || $ifjRegex[$i] == ')')
                    {
                        return null;
                    }
                    else if ($ifjRegex[$i] == '!')
                    {
                        $phpRegex = $phpRegex . '[^';
                        $state = Formatter::STATE_NOT;
                    }
                    else if ($ifjRegex[$i] == '(')
                    {
                        $phpRegex = $phpRegex . '(';
                        $state = Formatter::STATE_BRACKET;
                    }
                    else if ($ifjRegex[$i] == '%')
                    {
                        $phpRegex = $phpRegex . '[';
                        $state = Formatter::STATE_ESCAPE;
                    }
                    else if ($ifjRegex[$i] == '{' || $ifjRegex[$i] == '}' || $ifjRegex[$i] == '[' || $ifjRegex[$i] == ']' || $ifjRegex[$i] == '?'
                            || $ifjRegex[$i] == '/' || $ifjRegex[$i] == '^' || $ifjRegex[$i] == '$' || $ifjRegex[$i] == '\\')
                    {
                        $phpRegex = $phpRegex . '\\' . $ifjRegex[$i];
                        $state = Formatter::STATE_CHAR;
                    }
                    else if (ord($ifjRegex[$i]) >= 32)
                    {
                        $phpRegex = $phpRegex . $ifjRegex[$i];
                        $state = Formatter::STATE_CHAR;
                    }
                    else
                        return null;
                    break;
                case Formatter::STATE_DOT:
                    if (   $ifjRegex[$i] == '.' || $ifjRegex[$i] == '|' || $ifjRegex[$i] == '*' || $ifjRegex[$i] == '+'
                        || $ifjRegex[$i] == ')')
                    {
                        return null;
                    }
                    else if ($ifjRegex[$i] == '!')
                    {
                        $phpRegex = $phpRegex . '[^';
                        $state = Formatter::STATE_NOT;
                    }
                    else if ($ifjRegex[$i] == '(')
                    {
                        $phpRegex = $phpRegex . '(';
                        $state = Formatter::STATE_BRACKET;
                    }
                    else if ($ifjRegex[$i] == '%')
                    {
                        $phpRegex = $phpRegex . '[';
                        $state = Formatter::STATE_ESCAPE;
                    }
                    else if ($ifjRegex[$i] == '{' || $ifjRegex[$i] == '}' || $ifjRegex[$i] == '[' || $ifjRegex[$i] == ']' || $ifjRegex[$i] == '?'
                            || $ifjRegex[$i] == '/' || $ifjRegex[$i] == '^' || $ifjRegex[$i] == '$' || $ifjRegex[$i] == '\\')
                    {
                        $phpRegex = $phpRegex . '\\' . $ifjRegex[$i];
                        $state = Formatter::STATE_CHAR;
                    }
                    else if (ord($ifjRegex[$i]) >= 32)
                    {
                        $phpRegex = $phpRegex . $ifjRegex[$i];
                        $state = Formatter::STATE_CHAR;
                    }
                    else
                        return null;
                    break;
                case Formatter::STATE_ITERATION:
                    if ($ifjRegex[$i] == '.')
                    {
                        $state = Formatter::STATE_DOT;
                    }
                    else if ($ifjRegex[$i] == '|')
                    {
                        $phpRegex = $phpRegex . '|';
                        $state = Formatter::STATE_OR;
                    }
                    else if ($ifjRegex[$i] == '*' || $ifjRegex[$i] == '+')
                    {
                        $phpRegex[strlen($phpRegex) - 1] = '*';
                        $state = Formatter::STATE_ITERATION;
                    }
                    else if ($ifjRegex[$i] == '!')
                    {
                        $phpRegex = $midBuffer . '[^';
                        $state = Formatter::STATE_NOT;
                    }
                    else if ($ifjRegex[$i] == '(')
                    {
                        $phpRegex = $phpRegex . '(';
                        $state = Formatter::STATE_BRACKET;
                    }
                    else if ($ifjRegex[$i] == ')')
                    {
                        $phpRegex = $phpRegex . ')';
                        $state = Formatter::STATE_CLOSE_BRACKET;
                    }
                    else if ($ifjRegex[$i] == '%')
                    {
                        $phpRegex = $phpRegex . '[';
                        $state = Formatter::STATE_ESCAPE;
                    }
                    else if ($ifjRegex[$i] == '{' || $ifjRegex[$i] == '}' || $ifjRegex[$i] == '[' || $ifjRegex[$i] == ']' || $ifjRegex[$i] == '?'
                            || $ifjRegex[$i] == '/' || $ifjRegex[$i] == '^' || $ifjRegex[$i] == '$' || $ifjRegex[$i] == '\\')
                    {
                        $phpRegex = $phpRegex . '\\' . $ifjRegex[$i];
                        $state = Formatter::STATE_CHAR;
                    }
                    else if (ord($ifjRegex[$i]) >= 32)
                    {
                        $phpRegex = $phpRegex . $ifjRegex[$i];
                        $state = Formatter::STATE_CHAR;
                    }
                    else
                        return null;
                    break;
                case Formatter::STATE_PITERATION:
                    if ($ifjRegex[$i] == '.')
                    {
                        $state = Formatter::STATE_DOT;
                    }
                    else if ($ifjRegex[$i] == '|')
                    {
                        $phpRegex = $phpRegex . '|';
                        $state = Formatter::STATE_OR;
                    }
                    else if ($ifjRegex[$i] == '*')
                    {
                        $phpRegex[strlen($phpRegex) - 1] = '*';
                        $state = Formatter::STATE_ITERATION;
                    }
                    else if ($ifjRegex[$i] == '+')
                    {
                        $state = Formatter::STATE_PITERATION;
                    }
                    else if ($ifjRegex[$i] == '!')
                    {
                        $phpRegex = $midBuffer . '[^';
                        $state = Formatter::STATE_NOT;
                    }
                    else if ($ifjRegex[$i] == '(')
                    {
                        $phpRegex = $phpRegex . '(';
                        $state = Formatter::STATE_BRACKET;
                    }
                    else if ($ifjRegex[$i] == ')')
                    {
                        $phpRegex = $phpRegex . ')';
                        $state = Formatter::STATE_CLOSE_BRACKET;
                    }
                    else if ($ifjRegex[$i] == '%')
                    {
                        $phpRegex = $phpRegex . '[';
                        $state = Formatter::STATE_ESCAPE;
                    }
                    else if ($ifjRegex[$i] == '{' || $ifjRegex[$i] == '}' || $ifjRegex[$i] == '[' || $ifjRegex[$i] == ']' || $ifjRegex[$i] == '?'
                            || $ifjRegex[$i] == '/' || $ifjRegex[$i] == '^' || $ifjRegex[$i] == '$' || $ifjRegex[$i] == '\\')
                    {
                        $phpRegex = $phpRegex . '\\' . $ifjRegex[$i];
                        $state = Formatter::STATE_CHAR;
                    }
                    else if (ord($ifjRegex[$i]) >= 32)
                    {
                        $phpRegex = $phpRegex . $ifjRegex[$i];
                        $state = Formatter::STATE_CHAR;
                    }
                    else
                        return null;
                    break;
                case Formatter::STATE_BRACKET:
                    if (   $ifjRegex[$i] == '.' || $ifjRegex[$i] == '|' || $ifjRegex[$i] == '*' || $ifjRegex[$i] == '+'
                        || $ifjRegex[$i] == ')')
                    {
                        return null;
                    }
                    else if ($ifjRegex[$i] == '!')
                    {
                        $phpRegex = $phpRegex . '[^';
                        $state = Formatter::STATE_NOT;
                    }
                    else if ($ifjRegex[$i] == '(')
                    {
                        $phpRegex = $phpRegex . '(';
                        $state = Formatter::STATE_BRACKET;
                    }
                    else if ($ifjRegex[$i] == '%')
                    {
                        $phpRegex = $phpRegex . '[';
                        $state = Formatter::STATE_ESCAPE;
                    }
                    else if ($ifjRegex[$i] == '{' || $ifjRegex[$i] == '}' || $ifjRegex[$i] == '[' || $ifjRegex[$i] == ']' || $ifjRegex[$i] == '?'
                            || $ifjRegex[$i] == '/' || $ifjRegex[$i] == '^' || $ifjRegex[$i] == '$' || $ifjRegex[$i] == '\\')
                    {
                        $phpRegex = $phpRegex . '\\' . $ifjRegex[$i];
                        $state = Formatter::STATE_CHAR;
                    }
                    else if (ord($ifjRegex[$i]) >= 32)
                    {
                        $phpRegex = $phpRegex . $ifjRegex[$i];
                        $state = Formatter::STATE_CHAR;
                    }
                    else
                        return null;
                    break;
                case Formatter::STATE_CLOSE_BRACKET:
                    if ($ifjRegex[$i] == '.')
                    {
                        $state = Formatter::STATE_DOT;
                    }
                    else if ($ifjRegex[$i] == '|')
                    {
                        $phpRegex = $phpRegex . '|';
                        $state = Formatter::STATE_OR;
                    }
                    else if ($ifjRegex[$i] == '*')
                    {
                        $phpRegex = $phpRegex . '*';
                        $state = Formatter::STATE_ITERATION;
                    }
                    else if ($ifjRegex[$i] == '+')
                    {
                        $phpRegex = $phpRegex . '+';
                        $state = Formatter::STATE_PITERATION;
                    }
                    else if ($ifjRegex[$i] == '!')
                    {
                        $phpRegex = $phpRegex . '[^';
                        $state = Formatter::STATE_NOT;
                    }
                    else if ($ifjRegex[$i] == '(')
                    {
                        $phpRegex = $phpRegex . '(';
                        $state = Formatter::STATE_BRACKET;
                    }
                    else if ($ifjRegex[$i] == ')')
                    {
                        $phpRegex = $phpRegex . ')';
                        $state = Formatter::STATE_CLOSE_BRACKET;
                    }
                    else if ($ifjRegex[$i] == '%')
                    {
                        $phpRegex = $phpRegex . '[';
                        $state = Formatter::STATE_ESCAPE;
                    }
                    else if ($ifjRegex[$i] == '{' || $ifjRegex[$i] == '}' || $ifjRegex[$i] == '[' || $ifjRegex[$i] == ']' || $ifjRegex[$i] == '?'
                            || $ifjRegex[$i] == '/' || $ifjRegex[$i] == '^' || $ifjRegex[$i] == '$' || $ifjRegex[$i] == '\\')
                    {
                        $phpRegex = $phpRegex . '\\' . $ifjRegex[$i];
                        $state = Formatter::STATE_CHAR;
                    }
                    else if (ord($ifjRegex[$i]) >= 32)
                    {
                        $phpRegex = $phpRegex . $ifjRegex[$i];
                        $state = Formatter::STATE_CHAR;
                    }
                    else
                        return null;
                    break;
                default:
                    return null;
            }
        }

        if ($state == Formatter::STATE_DOT || $state == Formatter::STATE_OR || $state == Formatter::STATE_NOT
            || $state == Formatter::STATE_ESCAPE || $state == Formatter::STATE_BRACKET)
        {
            return null;
        }

        return $phpRegex;
    }

    private $input;
    private $tags;
    private $formatFile;
    private $inputFile;
    private $outputFile;
    private $formatArray;
    private $newline;
}

?>
