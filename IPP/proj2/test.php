<?php

$run_interpret = false;
$recursive = false;

const MISSING_ARG = 10;
const IN_FILE_ERR  = 11;
const OUT_FILE_ERR = 12;


$rc_out;       // návratový kód z analýzy a interpretace
$rc_ref;       // první očekávaný návratový kód z analýzy a interpretace
$input;        // vstup interpretu
$out_ref;      // očekávaný výstup interpretu
$without_ext;  // cesta bez přípony k jednomu testu

$src_reg = '*.src';
$rc_reg  = '*.rc';
$in_reg  = '*.in';
$out_reg = '*.out';
$empty_line_reg = '/^[\s]*$/u';
$src_file_reg = '/(.*\.src$)/';

$test_cnt = 0;
$test_passed = 0;

$html_head_printed = false;
$html_head ='
<!DOCTYPE html>
<html>
<head>
<title>Test</title>
<meta charset="UTF-8">

<style>
.header{
    width: 100%;
    padding: 15px;
    font-size:35px;
    background-color: #7e7e7e;
    color:white;
    font-family: Helvetica;
}
</style>
</head>

<body>
    <div class ="header">
    Přehled testování
    </div>';
$html_head .= "\n";

// zde bude přehled úspěšnosti
$html_table= '
    <style>
    #tests {
        font-family: "Helvetica";
        border-collapse: collapse;
        width: 100%;
    }

    #tests td, #tests th {
        border: 1px solid #ddd;
        padding: 8px;
        font-family: "Helvetica";
        color: #000000 ;
    }

    #tests tr:nth-child(even){background-color: #f2f2f2;}

    #tests tr:hover {background-color: #ddd;}

    #tests th {
        padding-top: 12px;
        padding-bottom: 12px;
        text-align: left;
        background-color: #4CAF50; 
        color: white;
    }
    </style>

    <table id="tests"">
    <tr>
        <th>Cesta k testu</th>
        <th>Název testu</th>
        <th>Výsledek</th>
        <th>Návratová hodnota</th>
        <th>Výstup interpretace</th>    
    </tr>';

$html_table .= "\n";

# vypise chybove hlaseni v pripade spatnych argumentu a skonci s chybou
function err_args_exit()
{
    fwrite(STDERR, "Spatne parametry prikazove radky.\n");
    fwrite(STDERR, "Pro napovedu spuste script pouze s parametrem --help.\n");
    exit(MISSING_ARG);
}

$longopts = array("directory:", "recursive", "parse-script:", "int-script:" ,"help");
$options = getopt("",$longopts); //"" znamená, že nejsou žádné krátké parametry s jednou '-'

$valid_args = count($options); // počet argumentů, které byly vyhodnoceny getopts jako validní
$all_args = count($argv)-1;    // počet všech argumentů (bez prvního - spuštění scriptu)

$test_dir = ".";
$parse_path = "parse.php";
$interpret_path = "interpret.py";


// zkontroluje parametry, pokud nějaké jsou
if ($all_args !== 0) {
    if ($all_args !== $valid_args) {
        echo "all: $all_args !== valid: $valid_args\n";
        err_args_exit();
    }

    if (array_key_exists("help", $options)) {
        if ($all_args != 1) {
            err_args_exit();
        }
        echo "POPIS SCRIPTU\n",
        "---------------------------\n",
        "Tento skript slouzi pro automaticke testovani postupne aplikace parse.php a interpret.py\n",
        "Skript projde zadany adresar s testy a vyuzije je pro automaticke otestovani\n",
        "spravne funkcnosti obou predchozich programu vcetne vygenerovani\n",
        "prehledneho souhrnu v HTML 5 do standardniho vystupu.",
        "---------------------------\n",
        "PARAMETRY\n",
        "---------------------------\n",
        "   Nasledujici parametry muhou byt pouzity v libovolnem poradi\n",
        "  '--help'               - Vypise tuto napovedu na prikazovy radek.\n",
        "                           Nelze kombinovat s ostatnimi parametry.\n",
        "  '--directory=path'     - testy bude hledat v zadanem adresari\n",
        "                           (chybi-li tento parametr, tak skript prochazi aktualni adresar)\n",
        "  '--recursive'          - testy bude hledat nejen v zadanem adresari, ale i rekurzivne\n",
        "                           ve vsech jeho podadresarich\n",
        "  '--parse-script=file'  - 'file' urcuje cestu k soubori se skriptem v PHP 5.6\n",
        "                           pro analyzu zdrojoveho kodu v IPP-code18\n",
        "                           (chybi-li tento parametr, tak implicitni hodnotou je parse.php\n",
        "                           ulozeny v aktualnim adresari)\n",
        "  '--int-script=file'    - soubor se skriptem v Python 3.6 pro interpret XML reprezentace kodu\n",
        "                           v IPPcode18 (chybi-li tento parametr, tak implicitni hodnotou je interpret.py\n",
        "                           ulozeny v aktualnim adresari)\n";
        exit(0);
    }
    else if (array_key_exists("directory", $options)) {
        $test_dir = $options["directory"];
        if (!file_exists($test_dir)) {
            fprintf(STDERR, "Adresar s cestou $test_dir nebyl nalezen.\n");
            exit(IN_FILE_ERR);
        }
    }
    if (array_key_exists("recursive", $options)) {
        $recursive = true;
    }
    if (array_key_exists("parse-script", $options)) {
        $parse_path = $options["parse-script"];
        if (!file_exists($parse_path)) {
            fprintf(STDERR, "Skript s cestou $parse_path nebyl nalezen.\n");
            exit(IN_FILE_ERR);
        }
    }
    if (array_key_exists("int-script", $options)) {
        $intepret_path = $options["int-script"];
        if (!file_exists($intepret_path)) {
            fprintf(STDERR, "Skript s cestou $intepret_path nebyl nalezen.\n");
            exit(IN_FILE_ERR);
        }
    }
}   

# v pripade existence souboru vraci jeho cestu
# jinak vraci false
# $reg - regularni vyraz, pro hledani urciteho souboru
function file_exists_reg($reg)
{
    $paths_arr = glob($reg);
    $matched_cnt = count($paths_arr);
    if ($matched_cnt === 0) {        
        return false;
    }
    return $paths_arr;
}

# odstrani z cesty k souboru jeho priponu
# $filename - cesta k souboru
function path_ext_strip($filename){
    return preg_replace('/.[^.]*$/', '', $filename);
}


/********************SRC SOUBOR***********************/
// soubor .src se zdrojovým kódem mu existovat, tedy z něj získáme názvy testů
// nemusi se otvírat, obsah se posle na STDIN parse.php
$paths_arr = array();
if ($recursive) {
    // zdroj:https://stackoverflow.com/questions/1860393/recursive-file-search-php
    $it = new RecursiveDirectoryIterator("$test_dir");
    foreach(new RecursiveIteratorIterator($it) as $path)
    { 
        if (preg_match($src_file_reg, $path)) {
            $paths_arr[] = $path;
        }        
    }    
} else {    
    // Do pole $paths_arr priradi cestu k jednotlivym .src souborum
    $list = scandir($test_dir);
    foreach ($list as $file) {
        // najde .src soubor
        if (preg_match($src_file_reg, $file)) {
            $paths_arr[] = $test_dir."/".$file;
        }
    }
}
if (empty($paths_arr)) {
    fprintf(STDERR, "Zadny soubor s priponou src v danem adresari nebyl nalezen.\n");
    exit(IN_FILE_ERR);
}
/********************SRC SOUBOR***********************/


// pro kazdy soubor *.src spusti parser.php a zkontroluje navrat. hodnoty
foreach ($paths_arr as $src_path) {    
    $without_ext = path_ext_strip($src_path);
    $test_name = (path_ext_strip(basename($src_path)));

    /*********************RC SOUBOR***********************/
    $ret = file_exists("$without_ext.rc");
    // soubor neexistuje, musí se vytvořit nový a zapsat návratovou hodnotu 0
    if ($ret === false) {
        $handle = fopen("$without_ext.rc", "w");
        if ($handle === false) {
            fwrite(STDERR, "Soubor $without_ext.rc se nepodarilo vygenerovat\n");
            exit(OUT_FILE_ERR);
        } else {
            $rc_ref = 0;
            fwrite($handle, "0");
            fclose($handle);
        }
    // soubor existuje, stačí z něj přečíst očekávanou návratovou hodnotu
    } else {
        $handle = fopen("$without_ext.rc", "r+");
        if ($handle === false) {
            fwrite(STDERR, "Soubor $without_ext.rc existuje, ale nepodarilo se ho otevrit pro cteni.\n");
            exit(OUT_FILE_ERR);
        } else {
            $rc_ref = trim(fgets($handle));// počítá se s tím, že dostane číslo (neni specifikovano)
            if ($rc_ref == '') {
                $rc_ref = 0;
                fwrite($handle, "0");
            } else if (ctype_digit($rc_ref)) {
                $rc_ref = (int) $rc_ref;
            } else {
                fprintf(STDERR, "V souboru $without_ext.rc neni ulozeno cislo\n");
            }
            fclose($handle);
        }
    }
    /*********************RC SOUBOR***********************/

    /*********************IN SOUBOR***********************/
    $ret = file_exists("$without_ext.in");
    // soubor neexistuje, musí se vytvořit nový
    if ($ret === false) {
        $handle = fopen("$without_ext.in", "w");
        if ($handle === false) {
            fwrite(STDERR, "Soubor $without_ext.in se nepodarilo vygenerovat\n");
            exit(OUT_FILE_ERR);
        }        
        fclose($handle);
    }
    /*********************IN SOUBOR***********************/

    /*********************OUT SOUBOR**********************/
    $ret = file_exists("$without_ext.out");
    // soubor neexistuje, musí se vytvořit nový
    if ($ret === false) {
        $handle = fopen("$without_ext.out", "w");
        if ($handle === false) {
            fwrite(STDERR, "Soubor $without_ext.out se nepodarilo vygenerovat\n");
            exit(OUT_FILE_ERR);
        }
        fclose($handle);
    }
    /*********************OUT SOUBOR**********************/

    $temp_parse_out = tempnam(".","temp");

    unset($black_hole);
    exec("php5.6 $parse_path 1>$temp_parse_out 2>/dev/null <$src_path", $black_hole, $rc_out); // spusti parse.php
    $dirname = dirname($src_path);
    $html_table .="    <tr>\n        <td>$dirname</td>\n";  
    $html_table .="        <td>$test_name</td>\n";

    $test_cnt++;

    if ($rc_out === 0) {
        $run_interpret = true;
    } else if ($rc_out === $rc_ref) {
        $test_passed++;
        $html_table .="        <td><center><span>&#10004;</center></span></td>\n";
        $html_table .="        <td>OK $rc_out</td>\n";
        $html_table .="        <td>Žádný, parse.php zachytil chybu.</td>\n";        
    } else {
        $html_table .="        <td><center><span>&#10008;</center></span></td>\n";
        $html_table .="        <td>Je $rc_out očekával $rc_ref</td>\n";
        $html_table .="        <td>Žádný, chyba v parse.php</td>\n";
    }

    if ($run_interpret) {
        $differ = 1;
        $temp_int_out = tempnam(".","out_ref");
        unset($black_hole);
        exec("python3.6 $interpret_path --source=$temp_parse_out 1>$temp_int_out 2>/dev/null <$without_ext.in", $black_hole, $rc_out);        
        if ($rc_out === $rc_ref) {                            
            if ($rc_out === 0) {
                exec("diff $without_ext.out $temp_int_out",$diff_out, $differ);
                if ($differ == 0) {
                    $test_passed++; 
                    $html_table .="        <td><center><span>&#10004;</center></span></td>\n";
                    $html_table .="        <td>OK $rc_out</td>\n"; 
                    $html_table .="        <td>OK</td>\n";
                } else {
                    $html_table .= "        <td><center><span>&#10008;</center></span></td>\n";
                    $html_table .="        <td>OK $rc_out</td>\n";
                    $html_table .="        <td>Špatný\n";
                }
            } else {
                $test_passed++;
                $html_table .="        <td><center><span>&#10004;</center></span></td>\n";
                $html_table .="        <td>OK $rc_out</td>\n";   
                $html_table .="        <td>Žádný, interpret zachytil chybu</td>\n";
            }
        } else {
            $html_table .= "        <td><center><span>&#10008;</center></span></td>\n";
            $html_table .= "        <td>Je $rc_out očekával $rc_ref</td>\n";
            $html_table .= "        <td>Žádný, chyba v interpretu</td>\n";
        }
        unlink($temp_int_out); // smaže dočasný soubor
    }
    unlink($temp_parse_out);
    unset($diff_out); // exec do pole $diff_out appenduje vystup diffu, je potreba ho mazat
    $html_table .= "    </tr>\n";
    $run_interpret = false;

}
$html_table .="    </table>\n</body>\n</html>\n";

$html_overview = '    <p><span style="font-size: 25px" face="Helvetica">Úspěšnost: ';
if ($test_cnt === 0) {
    $test_percent = 0;
} else {
    $test_percent = ($test_passed/$test_cnt)*100;
    $test_percent = round($test_percent, 1);
}
$html_overview .= "$test_percent%, $test_passed/$test_cnt </span></p>\n";
$html_all = $html_head.$html_overview.$html_table;
echo $html_all;
?>