# Scripting Language Notes
This document attempts to define a 'core language' which models the scripting
language used in `v1.2.0.416` of Oblivion as closely as possible, without
introducing syntactic ambiguities. A major resource for this is the excellent
[Elder Scrolls Construction Set Wiki](https://cs.elderscrolls.com/index.php).
It also defines an 'extended language', based on the core language, which
directly incorporates many of the features added by OBSE. The comprehensive
OBSE documentation included in the OBSE source code was of course an invaluable
resource.

For disambiguation between scripting and C++ features of the same name, C++
identifiers are prefixed with `cpp::`.

## Script Name and Comments
Since it is convenient in examples to use comments, we define the comment syntax
first. A comment begins at a semi-colon `;` and continues until the end of the
line. Every character after the semi-colon and before the newline has no
semantic meaning to the language and is ignored.

Every script must have a unique name specified by the `ScriptName` or `scn`
statement, which must not occur after any non-space non-comment tokens. For
instance,
```tes4script
; This script does x and was written by y

ScriptName MyScript
```
defines a new script with the name "MyScript", which acts as the EditorId of the
script. Oblivion's parsing rules for the script name are complicated, and do not
follow the usual sanitization rules for EditorIds. (DR2)

## Variables
All variable names must be Identifiers. Oblivion uses the rule
```peg
Identifier <- [a-zA-Z0-9]+
```
which means that strings of digits are valid variable names. This ambiguity is a
problem, so the rule
```peg
Identifier <- [a-zA-Z]? [a-zA-Z0-9]*
```
is used instead. (DR4)

## Types
The core language requires type specifiers for all variable declarations; a
variable must be declared before it is used. There are four possible variable
types in the core language:

- `short`, a 16-bit signed two's-complement integer
- `long`, a 32-bit signed two's-complement integer
- `float`, a 32-bit IEEE754 single precision floating point number
- `ref`, a 32-bit reference identifier equivalent to `cpp::RefId`.

Internally, `short`, `long`, and `float` are all stored as a `cpp::float`, which
causes precision problems when storing large values in `long` variables. (DR1)

```tes4script
; Example
short myShort
ref myRef
```

There is no initialization syntax in the language, variables must be declared
and assigned separately. Instead, the first time a variable declaration is
encountered during script execution, that variable is initialized with the value
`0`. This occurs at most once for each variable declaration per run of the
application. Drawing parallels with C++, if every script is viewed as a
function, then all variables are value-initialized function-local `static`.

## Literals
Script files may contain literals of the above types, as well as literals of
types which can only be used as function arguments, and cannot be stored as
variables.

Floating point literals are much simpler than in C++:
```peg
FloatLiteral <- [-+]? ([1-9] [0-9]* "." [0-9]*) / ("0." [0-9]*) / ("." [0-9]+)
```
Scientific notation is not supported and trailing 'f's are not allowed,
presumably to prevent ambiguity between floating point numbers and references. 
For instance, what type is the literal `3592e10f`?

Integer literals are also simple, as only base 10 is allowed:
```peg
IntegerLiteral <- [-+]? [1-9] [0-9]*
```
Note that because FloatLiterals are required to contain a '.', there is no
ambiguity between FloatLiteral and IntegerLiteral. This way the parser can
easily flag narrowing. Alternatively, the two could be combined into a single
NumericLiteral by making the '.' optional.

`ref` literals are 32-bit hexadecimal numbers with no prefix or postfix. (DR5)
They are not required to be exactly 8 digits long in Oblivion's parser. They
are used to refer to specific RefIds in the local load order of the mod, but are
**not** translated to the global order when the script is loaded, unlike RefIds
present in records. The CS wiki therefore recommends avoiding using them in
scripts. It is impossible to unambiguously parse RefLiterals in all cases when
they are as described, so DR5 must be applied in some way.

String literals are simply any sequence of characters other than '"' or an
EndOfLine between two '"'. The language does not support escape characters, and
treats '\' as any other character.

EditorIds are a superior alternative to `ref` literals for obtaining a value of
`ref` type. Since EditorIds are equivalent to Identifiers, there is no need to
make them a literal; they simply behave like variables of `ref` type. Of course
they cannot be assigned to, but it is not the parser's job to police this, it is
the compiler's.

## Assignment and Expressions
Variable assignment is performed with the 'Set' and 'To' keywords, for example
```tes4script
set myShort to 64
```

Loosely speaking, variable assignment takes the form `set x to y` where `x` is
a variable and `y` is an expression. The result is a statement, which is not
an expression. This is contrary to C++, where `z = (x = y)` is well-defined.
A variable is any Identifier referring to a local variable or a global variable,
or a value of type `ref` representing a quest followed by a '.' and an
identifier referring to a local variable of that quest defined in the top level
of the associated quest script.
```tes4script
set myQuest.myVar to 7
```

The easiest way to define an expression precisely is through the grammar, but
informally it is a sequence of mathematical operators and operands, where each
operand is either a parenthesised expression, a variable, or a function call.

## Functions
The core language does not have the concept of a function. Users cannot declare
or define functions, and there is no clear syntax for a function call. Instead,
there is a predefined set of functions which the user can call by stating its
name followed by a space separated list of arguments. For instance,
```tes4script
; myShort = 1 if stage 30 of SomeQuest has been completed, else myShort = 0.
set myShort to GetStageDone SomeQuest 30
```
Essentially, a function call is just an identifier followed by zero or more
expressions; functions with zero arguments are therefore indistinguishable from
variables.

## Grammar
This section describes the grammar of the core language. Several DRs have been
applied in order to make the grammar unambiguous. If possible, their
consequences are noted in the grammar. In particular, DR2--5 have all been
applied.

```peg
Grammar <- Spacing Scriptname Block* EndOfFile

Scriptname <- (SCRIPTNAME / SCN) Identifier (Comment / EndOfLine)
Block <- BEGIN Identifier [IntegerLiteral]? (Statement)* END

## Keywords
BEGIN      <- "begin"      Spacing
END        <- "end"        Spacing
SET        <- "set"        Spacing
TO         <- "to"         Spacing
IF         <- "if"         Spacing
ELSE       <- "else"       Spacing
ELSEIF     <- "elseif"     Spacing
ENDIF      <- "endif"      Spacing
RETURN     <- "return"     Spacing
SCRIPTNAME <- "scriptname" Spacing
SCN        <- "scn"        Spacing

## Types
SHORT <- "short" Spacing
LONG  <- "long"  Spacing
FLOAT <- "float" Spacing
REF   <- "ref"   Spacing

Identifier <- InitialIdChar IdChar* Spacing
Type       <- SHORT / LONG / FLOAT / REF
Keyword    <- BEGIN / ENDIF / END / SET / TO / IF / ELSEIF / ELSE / RETURN
            / SCRIPTNAME / SCN / SHORT / LONG / FLOAT / REF

## Statements
Declaration  <- Type Identifier
SetStatement <- SET Variable TO Expression
ReturnStatment <- RETURN
Statement <- Declaration / SetStatement

## Literals
StringLiteral   <- ["] (!["] Char)* ["]
IntegerLiteral  <- [1-9] [0-9]*
RefLiteral      <- "#" [0-9]+
FloatLiteral    <- ([1-9] [0-9]* "." [0-9]*) / ("0." [0-9]*) / ("." [0-9]+)
Literal         <- StringLiteral / IntegerLiteral / FloatLiteral / RefLiteral

## Expressions
MemberAccess      <- (RefLiteral / Identifier) "."
Variable          <- Identifier / (MemberAccess Identifier)

# Without type information, a function call with zero arguments is
# indistinguishable from a variable, so a FunctionCall must have at least
# argument. This is still a useful distinction, as function calls may not appear
# on the lhs of an assignment, whereas variables can.
FunctionCall      <- Variable Expression+
PrimaryExpression <- FunctionCall / Variable / Literal
                   / (LPAREN Expression RPAREN)
UnaryExpression   <- [+-]? PrimaryExpression
MulExpression     <- UnaryExpression ((STAR / SLASH) UnaryExpression)*
AddExpression     <- MulExpression ((PLUS / DASH) MulExpression)*
CondExpression    <- AddExpression ((LTEQ / GTEQ / LT / GT) AddExpression)*
EqExpression      <- CondExpression ((EQEQ / NEQ) CondExpression)*
# DR3 applied to give && higher predence than ||
AndExpression     <- EqExpression (AND EqExpression)*
OrExpression      <- AndExpression (OR AndExpression)*
Expression        <- OrExpression

## Punctuation
EQEQ   <- "=="     Spacing
NEQ    <- "!="     Spacing
LTEQ   <- "<="     Spacing
GTEQ   <- ">="     Spacing
LT     <- "<" !"=" Spacing
GT     <- ">" !"=" Spacing
AND    <- "&&"     Spacing
OR     <- "||"     Spacing
LPAREN <- "("      Spacing
RPAREN <- ")"      Spacing
LBRACK <- "["      Spacing
RBRACK <- "]"      Spacing
PLUS   <- "+"      Spacing
STAR   <- "*"      Spacing
DASH   <- "-"      Spacing
SLASH  <- "/"      Spacing
DOT    <- "."      Spacing

IdChar        <- [a-zA-Z0-9]
InitialIdChar <- [a-zA-Z]

## Spacing
Spacing   <- (Space / Comment)*
Comment   <- ";" (!EndOfLine .)* EndOfLine
Space     <- " " / "\t" / EndOfLine
EndOfLine <- "\r\n" / "\n" / "\r"
EndOfFile <- !.
```
This resolves some ambiguities and (arguably) bugs in the Oblivion scripting
language.

#### (DR1) All numeric types are stored as `float`
Clearly not all values of the 32-bit `long` type are representable as values of
the 32-bit `float` type. This causes precision problems for large values in
`long` variables, and is also present in the GLOB record. Contrarily, the GMST
records do not have this problem and store a union capable of representing the
full value space of both `long` and `float`.

The resolution is to simply change the storage types of `short` and `long` to
be the `cpp::int16_t` and `cpp::int32_t`. `cpp::short` and `cpp::long` should
not be used as the standard guarantees the **minimum** number of bits in these
types, not the exact number. In particular, `cpp::long` is 64-bits on
[LP64 systems](https://en.cppreference.com/w/cpp/language/types).

A further resolution is to make GLOB consistent with GMST by changing FLTV to
respect FNAM_GLOB. This breaks binary compatibility with existing esp and esm
files, which would need to be patched. This would not change the size of the
esp/esm file.

#### (DR2) ScriptNames are not Parsed as EditorIds or Identifiers
The name of a script corresponds to the EditorId of that script, but the two are
parsed inconsistently. The rule `EditorId <- [a-zA-Z0-9]+` describes an
EditorId, and is the same as the rule for an Identifier (modulo the trailing
`Spacing`, which is an implementation detail and is not part of the Identifier).
When defining a new record, the CS removes all invalid characters, including
spaces. Importantly, the string "hello world" is translated to "helloworld".
When reading ScriptNames, all characters after and including any punctuation or
whitespace character except '-' and '\_' are ignored. Any '-' and '\_' are
removed.

The resolution is to parse ScriptNames as
```peg
ScriptName <- (SCRIPTNAME / SCRIPTNAME_S) Identifier (Comment / EndOfLine)
```
Invalid ScriptNames then result in a parsing error, instead of a silent
correction to a valid (but possibly unintended) EditorId.

#### (DR3) `&&` has lower precedence than `||`
It is conventional to give `&&` (logical and) a higher precendence than `||`
(logical or), but Oblivion's parser has this the other way round. This is not an
error per se, but it makes judicious use of parentheses almost mandatory to
avoid confusing both the reader and writer. The CS wiki goes so far as to
suggest avoiding the user of logical operators if possible.

In 'Oblivion.esm' there are examples of the form `(a && b) || (c && d)` and
examples of the form `(a || b) && (c || d)` and no examples mixing `&&` and `||`
without clarifying parethenses.

The resolution is to use the conventional precedence instead. It would be
convenient if the parser could warn when a conditional expression is not
interpreted as intended, though I am unsure if this is possible during the main
parsing run, as parentheses do not occur in the AST.

#### (DR4) Identifiers may Begin with Digits
There is not much to say here, hopefully the reader agrees that allowing
variables named '13' is a bad idea.

The resolution is to require that Identifiers cannot begin with a digit.

#### (DR5) RefLiterals Cannot be Unambiguously Parsed
RefLiterals have an ambiguity with IntegerLiteral in some cases.
In Oblivion's parser they also cause ambiguity with parsing member
variables of quests when the quest is represented by a RefLiteral, but this is
fixed by DR4. For instance, `12345678` is both a valid RefLiteral and a valid
IntegerLiteral.

On the other hand, since IntegerLiterals cannot have leading
zeros, many RefLiterals **can** be parsed unambiguously. In particular, the only
usage of RefLiterals in scripts that could be considered **not** a bug, namely
referring to RefIds in 'Oblivion.esm', can be parsed unambiguously as such
RefIds have two leading zeros. This suggests that IntegerLiterals be
preferentially chosen over RefLiterals, but RefLiterals should still be
considered.

It would be simpler to just ban RefLiterals altogether, but they can be used
quite reasonably outside of scripts, in console commands. Since these are
written on the fly, there is (almost) no existing code that might be broken by
changing the syntax of RefLiterals.

The resolution is therefore to require that all RefLiterals be prefixed by a 
single '#'. This disambiguates them cleanly from IntegerLiterals, at the risk of
breaking some (arguably buggy, or at the least, in bad style) scripts.

#### (DR6) No Logical Negation Operator
The only unary operators are the unary minus and (possibly) the unary plus, it
might be useful to have a unary negation '!'. A logical negation operator is not
strictly necessary as `!x` is equivalent to `x != 0`
