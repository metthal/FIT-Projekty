<?php

#SYN:xmilko01

require_once 'Formatter.php';

class Tag
{
    private static $tagStrings = array('b', 'i', 'u', 'tt', 'font', 'font', 'br');

    public function __construct($formatType, $endTag)
    {
        $this->formatType = $formatType;
        $this->endTag = $endTag;
    }

    public function ToString()
    {
        $type = $this->formatType->GetType();
        $tagString = '<';

        if ($this->endTag)
        {
            $tagString = $tagString . '/' . Tag::$tagStrings[$type] . '>';
        }
        else
        {
            $tagString = $tagString . Tag::$tagStrings[$type];

            if ($type == FormatType::SIZE)
                $tagString = $tagString . ' size=' . $this->formatType->GetValue();
            else if ($type == FormatType::COLOR)
                $tagString = $tagString . ' color=#' . $this->formatType->GetValue();
            else if ($type == FormatType::BR)
                $tagString = $tagString . ' /';

            $tagString = $tagString . '>';
        }

        return $tagString;
    }

    private $formatType;
    private $endTag;
}

?>
