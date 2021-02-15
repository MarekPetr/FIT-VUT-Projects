<?php

/****************argumenty příkazové řádky****************/
function err_args_exit()
{
    fwrite(STDERR, "Spatne parametry prikazove radky.\n");
    fwrite(STDERR, "Pro napovedu spuste script pouze s parametrem --help.\n");
    exit(10);
}

$file_reg = '/(^--stats=).*/';
$key_loc = false;
$key_com = false;
$file = "";

$longopts = array("stats:", "loc", "comments", "help");
$options = getopt("",$longopts);

$valid_args = count($options); // počet argumentů, které byly vyhodnoceny getopts jako validní
$all_args = count($argv)-1;    // počet všech argumentů (bez prvního - spuštění scriptu) 

// zkontroluje parametry, pokud nějaké jsou
if ($all_args !== 0) {
    if ($all_args !== $valid_args) {
        err_args_exit();
    }

    if ($valid_args === 1) {
        if (array_key_exists("help", $options)) {
            echo "POPIS SCRIPTU\n",
                 "---------------------------\n",
                 "Tento script nejprve nacte ze standardniho vstupu zdrojovy kod v IPPcode18.\n",
                 "Pote zkontroluje lexikalni a syntaktickou spravnost kodu.\n",
                 "Nakonec vypise na standardni vystup XML reprezentaci programu.\n\n",
                 "PARAMETRY\n",
                 "---------------------------\n",
                 "   Nasledujici parametry mohou byt pouzity v libovolnem poradi\n",
                 "  '--help'                  - vypise tuto napovedu na prikazovy radek. Nelze kombinovat s ostatnimi parametry.\n",
                 "  '--stats=file'            - vypise do souboru 'file' statistiku vstupniho zdrojoveho kodu\n",
                 "                              dle dalsich parametru --loc nebo/a --comments.\n",
                 "  '--stats=file --loc'      - vypise do souboru pocet radku kodu (instrukci) vstupniho zdrojoveho kodu.\n",
                 "  '--stats=file --comments' - vypise do souboru pocet komentaru vstupniho zdrojoveho kodu.\n",
                 "  V pripade pouziti obou parametru --loc a --comments\n",
                 "  bude vypis statistiky v poradi, ve kterem jsou tyto parametry.\n";
            exit(0);
        } else {
            err_args_exit();
        }
    } else if (array_key_exists("stats", $options)) {
        $file = $options["stats"];
        $key_loc = array_search("loc", array_keys($options));
        $key_com = array_search("comments", array_keys($options));

        if ($valid_args === 3) {
            //pokud jsou tri parametry, musi zde byt --loc i --com
            if ($key_com === false || $key_loc === false) {
                err_args_exit();
            }
        } else if ($valid_args === 2) {
            //pokud jsou tri parametry, musi zde byt bud jen --loc nebo --com
            if ($key_com === false && $key_loc === false) {
                err_args_exit();
            }
        } else {
            err_args_exit();
        }
    } else {
        err_args_exit();
    }
}
/****************argumenty příkazové řádky****************/

$parser = new Parser();
$parser->parse();

/****************výpis statistiky do souboru************/
if ($key_com || $key_loc)
{   
    $handle = fopen($file, "w");
    if ($handle === false) {
        fwrite(STDERR, "Soubor pro zapis statistiky '$file' se nepodarilo otevrit.\n");
        exit(12);
    }

    if ($key_loc === false && $key_com !== false) {
        fwrite($handle, "$parser->com_cntr");
    } else if ($key_loc !== false && $key_com === false) {
        fwrite($handle, "$parser->loc\n");
    } else if ($key_loc !== false && $key_com !== false) {
        if ($key_loc < $key_com) {
            fwrite($handle, "$parser->loc\n");
            fwrite($handle, "$parser->com_cntr");
        } else {
            fwrite($handle, "$parser->com_cntr\n");
            fwrite($handle, "$parser->loc");
        }
    }

    $ret = fclose($handle);
    if ($ret === false) {
        fwrite(STDERR, "Soubor pro zapis statistiky '$file' se nepodarilo uzavrit.\n");
        exit(12);
    }
}
/****************výpis statistiky do souboru************/


 // zkontroluje lexikální a syntaktickou správnost vstupního kódu
class Parser
{
    public $ln_cntr = 1;
    public $loc = 0;
    public $com_cntr = 0; // počet komentářů

    private $arg_cntr = 0;
    private $op_code;

    private $comment_reg = '/#.*/u';
    private $spaces_reg = '/\s+/u';
    private $header_reg = '/^(.ippcode18)[\s]*$/u';
    private $frame_reg = '/^(T|L|G)F@/u';
    private $var_reg = '/^(T|L|G)F@([\p{L}|_|\-|$|&|%|*][\p{L}|\p{N}|_|\-|$|&|%|*]*)$/u';
    private $label_reg = '/^([\p{L}|_|\-|$|&|%|*][\p{L}|\p{N}|_|\-|$|&|%|*]*)$/u';
    private $int_reg = '/^int@.*/u';
     # \\\\ je potřeba k zapsání jedné \
    private $str_reg = '/^string@([^\\\\]*(\\\\\p{N}{3}(?!\p{N}))*)*$/u';
    private $empty_line_reg = '/^[\s]*$/u';

    public $program_xml;
    private $instruct_xml;
    private $arg_xml;

    // operační kód
    private $opc_arr = array(
        "MOVE", "CREATEFRAME", "PUSHFRAME", "POPFRAME", "DEFVAR", "CALL", "RETURN",
        "PUSHS", "POPS", "ADD", "SUB", "MUL", "IDIV", "LT", "GT", "EQ", "AND", "OR", 
        "NOT", "INT2CHAR", "STRI2INT", "READ", "WRITE", "CONCAT", "STRLEN", "GETCHAR",
        "SETCHAR", "TYPE", "LABEL", "JUMP", "JUMPIFEQ", "JUMPIFNEQ", "DPRINT", "BREAK",
    );

    // hlavní funkce třídy Parser
    public function parse()
    {   
        $order = 0;

        $line = fgets(STDIN);

        $this->delete_comments($line);
        $this->check_header($line);

        $this->init_xml();        

        while ($line = fgets(STDIN)) {
            if ($line === FALSE) exit(0);

            $this->ln_cntr++;
            $this->arg_cntr = 1;

            $this->delete_comments($line);
            
            // přeskočí prázdné řádky
            if (preg_match($this->empty_line_reg, $line)) {
                continue;
            }
            //smaže prázdné znaky před prvním a po posledním tisknutelným znakem
            $line = trim($line);
            // rozdělí řádek na jednotlivé tokeny
            $token_arr = preg_split($this->spaces_reg, $line);

            $tokens_on_line = 0;
            $tokens_on_line = count($token_arr);       
              
            // operační kód
            $this->op_code = strtoupper($token_arr[0]);
            if (in_array("$this->op_code", $this->opc_arr)) {
                $this->loc++;
                $order++;
                $this->instruct_xml = $this->program_xml->addChild('instruction');
                $this->instruct_xml->addAttribute('order',$order);
                $this->instruct_xml->addAttribute('opcode',$this->op_code);
            } else {
                fwrite(STDERR, "Instrukce musi zacinat operacnim kodem, radek: $this->ln_cntr.\n");
                exit(21);
            }

            // do maximálně tří nově vytvořených proměnných - arg1,ar2,arg3 dosadí prvky pole token_arr
            // od 1 do 3 (nultý je operační kód, ne jeho argument)
            for($i = 1; $i < $tokens_on_line; $i++) {
                ${"arg$i"} = $token_arr[$i];
            } 

            // zkontroluje argumenty operačních kódů
            $args_current = $tokens_on_line-1;
            switch($this->op_code)
            {
                // bez argumentu
                case "PUSHFRAME": case "CREATEFRAME": case "POPFRAME": 
                case "RETURN":    case "BREAK":
                {
                   $args_expected = 0;
                   $this->check_arg_count($args_current, $args_expected); 
                    break; 
                }
                // <var>
                case "DEFVAR": case "POPS":
                {
                    $args_expected = 1;
                    $this->check_arg_count($args_current, $args_expected); 
                    $this->check_var($arg1);
                    break;
                }
                // <label>
                case "CALL": case "LABEL": case "JUMP":
                {
                    $args_expected = 1;
                    $this->check_arg_count($args_current, $args_expected); 
                    $this->check_label($arg1);
                    break;
                }
                // <symb>
                case "PUSHS": case "WRITE": case "DPRINT":
                {
                    $args_expected = 1;
                    $this->check_arg_count($args_current, $args_expected); 
                    $this->check_symbol($arg1);
                    break;
                }
                // <var> <symb>
                case "MOVE":   case "INT2CHAR": case "NOT":
                case "STRLEN": case "TYPE": 
                {
                    $args_expected = 2;
                    $this->check_arg_count($args_current, $args_expected); 
                    $this->check_var($arg1);
                    $this->check_symbol($arg2);
                    break;
                }
                // <var> <type>
                case "READ":
                {
                    $args_expected = 2;
                    $this->check_arg_count($args_current, $args_expected); 
                    $this->check_var($arg1);
                    $this->check_type($arg2);
                    break;
                }

                // <var> <symb> <symb>
                case "ADD":     case "SUB":     case "MUL":     case "IDIV":   case "LT":
                case "GT":      case "EQ":      case "AND":     case "OR":
                case "STRI2INT": case "CONCAT":  case "GETCHAR": case "SETCHAR":  
                {
                    $args_expected = 3;
                    $this->check_arg_count($args_current, $args_expected); 
                    $this->check_var($arg1);
                    $this->check_symbol($arg2);
                    $this->check_symbol($arg3);
                    break;
                }

                // <label> <symb> <symb>
                case "JUMPIFEQ": case "JUMPIFNEQ":
                {
                    $args_expected = 3;
                    $this->check_arg_count($args_current, $args_expected); 
                    $this->check_label($arg1);
                    $this->check_symbol($arg2);
                    $this->check_symbol($arg3);
                    break;
                }
            }               
        }
        $this->print_xml();
    }
	
	// zkontroluje header
	// $header je řetězec obsahující první token na prvním řádku vstupu
    private function check_header($header)
    {
        $header = strtolower($header);
        if (!preg_match($this->header_reg, $header)) {
            fwrite(STDERR, 'Chybi hlavicka vstupniho programu \'.IPPcode18\'.'."\n");
            exit(21);
        }
    }

    // smaže komentáře z řádku uloženého v proměnné $line
    private function delete_comments(&$line)
    {
        if (preg_match($this->comment_reg, $line)) {
            $line = preg_replace($this->comment_reg, '', $line);
            $this->com_cntr++;
        }
    }

    private function check_arg_count($args_current, $args_expected)
    {
        if ($args_current !== $args_expected) {
            fwrite(STDERR, "$this->op_code potrebuje $args_expected argumentu, radek: $this->ln_cntr.\n");
            exit(21);
        }
    }
    
    // zkontroluje, jestli je řetězec uložený v proměnné $arg proměnná
    private function is_var($arg)
    {
        if (preg_match($this->var_reg, $arg)) {
            $this->gen_xml_type($arg, "var");
            return true;
        }
        return false;
    }

	// zkontroluje, jestli je řetězec uložený v proměnné $arg proměnná
    private function check_var($arg)
    {
        if (!$this->is_var($arg)) {  
            fwrite(STDERR, "$this->arg_cntr. argument instrukce $this->op_code neni promenna, radek: $this->ln_cntr.\n");
            exit(21);
        }
    }

    // zkontroluje, jestli je řetězec uložený v proměnné $arg celočíselná konstanta
    private function is_int($arg)
    {
        if (preg_match($this->int_reg, $arg)) {
            $this->gen_xml_type($arg, "int");
            return true;
        }
        return false;
    }

    // zkontroluje, jestli je řetězec uložený v proměnné $arg konstanta řetězec
    private function is_string($arg)
    {
        if (preg_match($this->str_reg, $arg)) {
            $this->gen_xml_type($arg, "string");
            return true;
        }
        return false;
    }

    // zkontroluje, jestli je řetězec uložený v proměnné $arg konstanta bool
    private function is_bool($arg)
    {
        if (($arg === "bool@false") || ($arg === "bool@true")) {
            $this->gen_xml_type($arg, "bool");
            return true;
        }
        return false;
    }

    // zkontroluje, jestli je řetězec uložený v proměnné $arg konstanta type
    private function check_type($arg)
    {
        if ($arg !== "int" && $arg !== "bool" && $arg !== "string") {
            fwrite(STDERR, "$this->arg_cntr. argument instrukce $this->op_code neni typ, radek: $this->ln_cntr.\n");
            exit(21);
        } else {
            $this->gen_xml_type($arg, "type");
        }
    }

    // zkontroluje, jestli je řetězec uložený v proměnné $arg symbol (proměnná nebo konstanta)
    // pokud není, skončí s chybou, jinak vrací true
    private function check_symbol($arg)
    {
        if (!$this->is_var($arg)) {
            if (!$this->is_int($arg)) {
                if (!$this->is_string($arg)) {
                    if (!$this->is_bool($arg)) {
                        fwrite(STDERR, "$this->arg_cntr. argument instrukce $this->op_code neni symbol, radek: $this->ln_cntr.\n");
                        exit(21);
                    }
                }
            }
        }
        return true;
    }

    // zkontroluje, jestli je řetězec uložený v proměnné $arg návěští
    private function check_label($arg1)
    {
        if (preg_match($this->label_reg, $arg1)) {
            $this->gen_xml_type($arg1, "label");
        } else {
            fwrite(STDERR, "$this->arg_cntr. argument instrukce $this->op_code neni navesti, radek: $this->ln_cntr.\n");
            exit(21);
        }
    }

    // inicializuje generování XML výstupu
    private function init_xml()
    {
        $this->program_xml = new SimpleXMLElement("<?xml version=\"1.0\" encoding=\"utf-8\" ?><program></program>");
        $this->program_xml->addAttribute('language', 'IPPcode18');
    }

    // vygeneruje xml element s typem proměnné a její hodnotou
    private function gen_xml_type($var, $type)
    {
        
        if      ($type === "string") $var = preg_replace('/^string@/', '', $var);
        else if ($type === "int")    $var = preg_replace('/^int@/', '', $var);
        else if ($type === "bool")   $var = preg_replace('/^bool@/', '', $var);

        $this->arg_xml = $this->instruct_xml->addChild("arg$this->arg_cntr", htmlspecialchars($var));
        $this->arg_xml->addAttribute('type',$type);
        $this->arg_cntr++;    
    }

    // vytiskne finální XML výstup
    private function print_xml()
    {
        $dom = new DOMDocument("1.0");        
        $dom->formatOutput = true;
        $dom->preserveWhiteSpace = false;
        $dom->loadXML($this->program_xml->asXML());
        echo $dom->saveXML();
    }
}

?>
