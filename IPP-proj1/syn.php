<?php

#SYN:xmilko01

require_once 'ArgParser.php';
require_once 'Formatter.php';

const EXIT_OK               = 0;
const EXIT_BAD_PARAMS       = 1;
const EXIT_BAD_INPUT_FILE   = 2;
const EXIT_BAD_OUTPUT_FILE  = 3;
const EXIT_BAD_FORMAT_FILE  = 4;
const EXIT_UNKNOWN          = 255;

function printHelp()
{
    printf("Zvýrazňovanie syntaxe - Projekt 1 do predmetu IPP\n");
    printf("Autor: Marek Milkovič\n");
    printf("\n");
    printf("Použitie:\n");
    printf("\n");
    printf("\tphp syn.php [ --help | --format=FILE --input=FILE --output=FILE --br ]\n");
    printf("\n");
    printf("\t--help\t\t\t\t\tZobrazí nápovedu\n");
    printf("\t--format=FILE\t\t\t\tŠpecifikuje cestu k formátovaciemu súboru\n");
    printf("\t--input=FILE\t\t\t\tŠpecifikuje cestu k vstupnému súboru (ak nie je špecifikovaná, je použitý štandardný vstup)\n");
    printf("\t--output=FILE\t\t\t\tŠpecifikuje cestu k výstupnému súboru (ak nie je špecifikovaná, je použitý štandardný výstup)\n");
    printf("\t--br\t\t\t\t\tDopĺňa element <br/> na konci každého riadku vo vstupnom súbore\n");
}

$formatter = null;

try
{
    ArgParser::ParamsAreValid($argv);

    $helpParam = ArgParser::GetParam($argv, ArgParser::PARAM_HELP);
    if (!is_null($helpParam)) // --help was entered, do nothing else
    {
        printHelp();
        exit(0);
    }

    $formatFilePath = null;
    $formatParam = ArgParser::GetParam($argv, ArgParser::PARAM_FORMAT);
    if (!is_null($formatParam))
        $formatFilePath = $formatParam[1];

    $inputFilePath = null;
    $inputParam = ArgParser::GetParam($argv, ArgParser::PARAM_INPUT);
    if (!is_null($inputParam))
        $inputFilePath = $inputParam[1];
    else // no --input file specified, use standard input
        $inputFilePath = STDIN;

    $outputFilePath = null;
    $outputParam = ArgParser::GetParam($argv, ArgParser::PARAM_OUTPUT);
    if (!is_null($outputParam))
        $outputFilePath = $outputParam[1];
    else // no --output file specified, use standard output
        $outputFilePath = STDOUT;

    $brParam = ArgParser::GetParam($argv, ArgParser::PARAM_BR);
    if (!is_null($brParam))
        $brParam = true;
    else
        $brParam = false;

    $formatter = new Formatter($formatFilePath, $inputFilePath, $outputFilePath, $brParam);
    $formatter->ReadInputFile();
    $formatter->ReadFormatFile();
}
catch (Exception $e)
{
    // comment out for debug
    //echo $e->getMessage(), "\n";
    if (!is_null($formatter))
        $formatter->Dispose();
    exit($e->getCode());
}

if (!is_null($formatter))
    $formatter->Dispose(); // cleanup

exit(EXIT_OK);

?>
