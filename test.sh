#!/bin/sh
# Test suite for rpn calculator

PASS=0
FAIL=0
RPN=./rpn

trim() {
	sed 's/[[:space:]]*$//'
}

check() {
	desc="$1"
	input="$2"
	expected="$3"

	actual=$(echo "$input" | $RPN 2>&1 | trim)
	if [ "$actual" = "$expected" ]; then
		PASS=$((PASS + 1))
	else
		FAIL=$((FAIL + 1))
		printf "FAIL: %s\n  input:    %s\n  expected: [%s]\n  got:      [%s]\n" \
			"$desc" "$input" "$expected" "$actual"
	fi
}

check_args() {
	desc="$1"
	shift
	expected="$1"
	shift

	actual=$($RPN "$@" 2>&1 | trim)
	if [ "$actual" = "$expected" ]; then
		PASS=$((PASS + 1))
	else
		FAIL=$((FAIL + 1))
		printf "FAIL: %s\n  expected: [%s]\n  got:      [%s]\n" \
			"$desc" "$expected" "$actual"
	fi
}

# --- Arithmetic ---
check "addition"           "2 3 +"         "5"
check "subtraction"        "10 3 -"        "7"
check "multiplication"     "6 7 *"         "42"
check "division"           "20 4 /"        "5"
check "modulo"             "17 5 %"        "2"
check "increment"          "5 ++"          "6"
check "decrement"          "5 --"          "4"
check "negative result"    "3 5 -"         "-2"
check "chained ops"        "2 3 + 10 *"    "50"

# --- Unary math ---
check "abs positive"       "5 abs"         "5"
check "abs negative"       "-5 abs"        "5"
check "sign positive"      "42 sign"       "1"
check "sign negative"      "-7 sign"       "-1"
check "sign zero"          "0 sign"        "0"
check "ceil"               "2.3 ceil"      "3"
check "floor"              "2.7 floor"     "2"
check "factorial"          "5 fact"        "120"
check "sqrt"               "144 sqrt"      "12"
check "square (macro)"     "7 sq"          "49"

# --- Constants ---
check "pi"                 "pi"            "3.14159265359"
check "e"                  "e"             "2.71828182846"

# --- Powers and logs ---
check "pow"                "2 10 pow"      "1024"
check "exp"                "0 exp"         "1"
check "ln"                 "1 ln"          "0"
check "log10"              "100 log"       "2"
check "alog (macro)"       "2 alog"        "100"
check "inverse (macro)"    "4 inv"         "0.25"

# --- Trig ---
check "sin(0)"             "0 sin"         "0"
check "cos(0)"             "0 cos"         "1"
check "asin(0)"            "0 asin"        "0"
check "acos(1)"            "1 acos"        "0"
check "atan(0)"            "0 atan"        "0"
check "tan (macro)"        "0 tan"         "0"
check "cosh(0)"            "0 cosh"        "1"
check "sinh(0)"            "0 sinh"        "0"
check "tanh(0)"            "0 tanh"        "0"
check "sec (macro)"        "0 sec"         "1"
check "csc pi/2"           "pi 2 / csc"    "1"
check "cot pi/4"           "pi 4 / cot"    "1"

# --- Comparison ---
check "equal true"         "5 5 =="        "1"
check "equal false"        "5 6 =="        "0"
check "not equal"          "5 6 !="        "1"
check "less than true"     "3 5 <"         "1"
check "less than false"    "5 3 <"         "0"
check "less or equal"      "5 5 <="        "1"
check "greater than"       "5 3 >"         "1"
check "greater or equal"   "3 5 >="        "0"
check "logical not"        "0 !"           "1"
check "logical and"        "1 1 &&"        "1"
check "logical or"         "0 1 ||"        "1"

# --- Bitwise ---
check "bitwise and"        "255 0xf &"     "15"
check "bitwise or"         "0xf0 0x0f |"   "255"
check "bitwise xor"        "0xff 0x0f ^"   "240"
check "bitwise complement" "0 ~"           "1.84467440737e+19"
check "shift left"         "1 8 <<"        "256"
check "shift right"        "256 4 >>"      "16"

# --- Stack manipulation ---
check "dup"                "7 dup"            "7 7"
check "drop"               "1 2 3 drop"       "1 2"
check "swap"               "1 2 swap"         "2 1"
check "over (macro)"       "10 20 over"       "10 20 10"
check "rot (macro)"        "1 2 3 rot"        "2 3 1"
check "pick"               "10 20 30 2 pick"  "10 20 30 20"
check "roll"               "10 20 30 3 roll"  "20 30 10"
check "rolld"              "10 20 30 3 rolld" "30 10 20"
check "depth"              "1 2 3 depth"      "1 2 3 3"
check "dupn"               "1 2 3 3 dupn"     "1 2 3 1 2 3"
check "dropn"              "1 2 3 2 dropn"    "1"
check "clr (macro)"        "1 2 3 clr"        ""
check "min"                "5 3 min"          "3"
check "max"                "5 3 max"          "5"

# --- Repeat ---
check "repeat dup"         "2 3 repeat dup"   "2 2 2 2"
check "rep alias"          "2 3 rep dup"      "2 2 2 2"

# --- Metastack ---
check "pushs/pops"         "10 20 30 pushs 99 88 pops" "10 20 30 88"
check "pushs copies top"   "42 pushs"                   "42"
check "pops to empty"      "pushs 7 pops"               "7"

# --- Base conversion ---
check "hex input"          "ff#16"          "255"
check "binary input"       "1010#2"         "10"
check "octal input"        "77#8"           "63"
check "hex display"        "255 hex"        "ff"
check "binary display"     "10 bin"         "1010"
check "octal display"      "63 oct"         "77"
check "back to decimal"    "255 hex dec"    "255"
check "0x prefix"          "0xff"           "255"
check "0 prefix octal"     "077"            "63"

# --- Comma stripping ---
check "commas in number"   "1,000,000"     "1000000"

# --- Macros ---
check "total"              "1 2 3 4 5 total"   "15"
check "ave"                "10 20 30 ave"       "20"
check "chs"                "5 chs"              "-5"
check "discount"           "100 30 dup 100 / 2 pick * -" "100 21"

# --- Integer/float parts ---
check "ip"                 "3.7 ip"         "3"
check "fp"                 "3.7 fp"         "0.7"

# --- Base control ---
check "setbase/getbase"    "16 setbase getbase dec" "16"
check "setbase display"    "255 16 setbase"         "ff"

# --- Pad ---
check "pad hex"            "2 pad 15 hex"       "0f"
check "pad binary"         "8 pad 5 bin"        "00000101"

# --- IP address ---
check "ipaddr"             "0 ipaddr"           "0 0 0 0 0"

# --- Rand ---
check "rand pushes value"  "rand drop depth"    "0"

# --- Aven (average of top N) ---
check "aven"               "10 20 30 2 aven"    "10 25"

# --- Byte order ---
check "hns/nhs roundtrip"  "0x1234 hns nhs" "4660"
check "hnl/nhl roundtrip"  "0x12345678 hnl nhl" "305419896"

# --- Dot repeat ---
check "dot repeats last"   "5 dup ."        "5 5 5"

# --- Error handling ---
check "div by zero"        "5 0 /"     "Error: /: Division by zero.
5 0"
check "mod by zero"        "5 0 %"     "Error: %: Division by zero.
5 0"
check "sqrt domain"        "-1 sqrt"   "Error: sqrt: Argument is outside of function domain.
-1"
check "ln domain"          "-1 ln"     "Error: ln: Argument is outside of function domain.
-1"
check "log domain"         "-1 log"    "Error: log: Argument is outside of function domain.
-1"
check "asin domain"        "2 asin"    "Error: asin: Argument is outside of function domain.
2"
check "acos domain"        "2 acos"    "Error: acos: Argument is outside of function domain.
2"
check "fact negative"      "-1 fact"   "Error: fact: Argument is outside of function domain.
-1"
check "fact non-integer"   "2.5 fact"  "Error: fact: Argument is outside of function domain.
2.5"
check "pow domain"         "0 -1 pow"  "Error: pow: Argument is outside of function domain.
0 -1"
check "too few args"       "5 +"       "Error: +: Too few arguments.
5"
check "unknown command"    "5 bogus"   "Error: bogus: Unknown command.
5"
check "setbase too low"    "1 setbase" "Error: setbase: Argument is outside of function domain."
check "setbase too high"   "37 setbase" "Error: setbase: Argument is outside of function domain."
check "repeat domain"      "0 repeat"  "Error: repeat: Argument is outside of function domain.
0"
check "error stops eval"   "5 0 / 3 +" "Error: /: Division by zero.
5 0"

# --- Command line args ---
check_args "args mode"       "14" "3 4 +" "2 *"
check_args "single arg"      "5"  "2 3 +"

# --- Edge cases ---
check "empty input"        ""                ""
check "whitespace only"    "   "             ""
check "negative number"    "-5"              "-5"
check "negative decimal"   "-.5"             "-0.5"
check "fact zero"          "0 fact"          "1"
check "pick 1"             "10 20 1 pick"    "10 20 20"
check "roll 1"             "10 20 1 roll"    "10 20"
check "rolld 1"            "10 20 1 rolld"   "10 20"
check "swap is reversible" "1 2 swap swap"   "1 2"
check "nested macro"       "4 inv inv"       "4"

# --- Multi-line pipe ---
check "multi-line input"   "2 3 +
5 *"                        "25"

# --- Version ---
check "version pushes num"  "version"  "0.69"

# --- Summary ---
echo ""
TOTAL=$((PASS + FAIL))
if [ $FAIL -eq 0 ]; then
	echo "OK: $PASS/$TOTAL tests passed"
else
	echo "FAILED: $FAIL/$TOTAL tests failed ($PASS passed)"
	exit 1
fi
