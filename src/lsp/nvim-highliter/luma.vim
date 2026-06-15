" luma.vim - Enhanced Syntax highlighting for Luma language
" Place this file at: ~/.config/nvim/syntax/luma.vim

if exists("b:current_syntax")
  finish
endif

" =====================
" COMMENTS
" =====================
syn match lumaComment "//.*$" contains=NONE containedin=ALL
syn region lumaComment start="/\*" end="\*/" contains=NONE containedin=ALL
hi def lumaComment guifg=#928374 gui=italic

" =====================
" DOCUMENTATION COMMENTS
" =====================
syn match lumaDocComment "///.*$" contains=NONE
syn region lumaDocComment start="/\*\*" end="\*/" contains=NONE
hi def lumaDocComment guifg=#8ec07c gui=italic

" =====================
" PREPROCESSORS & ATTRIBUTES
" =====================
syn match lumaPreprocessor /@\w\+/
syn match lumaAttribute /#returns_ownership\|#takes_ownership\|#lib_import\(.*\)\|#dll_import\(.*\)/
" @os, @module, @use, @link directives
syn match lumaDirective /@module\|@use\|@os\|@link/
hi def lumaPreprocessor guifg=#d3869b gui=bold
hi def lumaAttribute guifg=#8ec07c gui=bold,italic
hi def lumaDirective guifg=#d3869b gui=bold

" =====================
" KEYWORDS
" =====================
syn keyword lumaKeyword const fn if elif else loop return let pub priv
syn keyword lumaKeyword struct enum switch case defer break continue
syn keyword lumaKeyword impl as
hi def lumaKeyword guifg=#fb4934 gui=bold

" =====================
" TYPES
" =====================
syn keyword lumaType int float double bool byte void uint
hi def lumaType guifg=#fabd2f gui=italic

" =====================
" CONDITIONALS
" =====================
syn keyword lumaConditional if elif else switch case
hi def lumaConditional guifg=#fe8019 gui=bold

" =====================
" LOOP
" =====================
syn keyword lumaLoop loop break continue
hi def lumaLoop guifg=#fe8019 gui=bold

" =====================
" STATEMENTS
" =====================
syn keyword lumaStatement return let defer
hi def lumaStatement guifg=#83a598 gui=italic

" =====================
" MODIFIERS / VISIBILITY
" =====================
syn keyword lumaModifier const pub priv
hi def lumaModifier guifg=#d3869b gui=bold

" =====================
" LITERALS
" =====================
syn keyword lumaBoolean true false
syn keyword lumaNull null
syn match lumaNumber /\<\d\+\(\.\d\+\)\?\([eE][+-]\?\d\+\)\?/
syn match lumaNumber /\<0x[0-9a-fA-F]\+\>/
syn match lumaNumber /\<0b[01]\+\>/
hi def lumaBoolean guifg=#fb4934 gui=bold
hi def lumaNull guifg=#928374 gui=italic
hi def lumaNumber guifg=#d3869b

" =====================
" STRINGS
" =====================
syn region lumaString start=+"+ skip=+\\"+ end=+"+ contains=lumaEscape
syn region lumaString start=+'+ skip=+\\'+ end=+'+ contains=lumaEscape
syn match lumaEscape contained /\\[nrt\\'"]/
syn match lumaEscape contained /\\x[0-9a-fA-F]\{2\}/
hi def lumaString guifg=#b8bb26
hi def lumaEscape guifg=#fe8019 gui=bold

" =====================
" STRUCT / ENUM / FUNCTION NAMES
" =====================
" const Name -> struct { ... }
syn match lumaStructDef /\<\(struct\|enum\)\>/
" Function definition: name -> fn
syn match lumaFunctionDef /\w\+\ze\s*->\s*fn/ contains=NONE
" Function call: name(
syn match lumaFunctionCall /\w\+\ze\s*(/ contains=NONE
" Method call: .name(
syn match lumaMethodCall /\.\w\+\ze\s*(/ contains=NONE
" Type name after cast<
syn match lumaTypeCast /\(cast<\)\@<=\w\+/

hi def lumaStructDef guifg=#fabd2f gui=bold
hi def lumaFunctionDef guifg=#8ec07c gui=bold
hi def lumaFunctionCall guifg=#8ec07c
hi def lumaMethodCall guifg=#83a598
hi def lumaTypeCast guifg=#fabd2f gui=italic

" =====================
" BUILT-IN FUNCTIONS
" =====================
syn keyword lumaBuiltinFunction output outputln alloc free sizeof cast
syn keyword lumaBuiltinFunction input system
hi def lumaBuiltinFunction guifg=#fe8019 gui=bold,italic

" =====================
" OPERATORS
" =====================
" Arithmetic
syn match lumaOperator /[+\-*\/%]/
" Comparison
syn match lumaOperator /==\|!=\|<=\|>=\|<\|>/
" Logical
syn match lumaOperator /&&\|||/
" Bitwise
syn match lumaOperator /&||\|^\|~\|<<\|>>/
" Assignment
syn match lumaOperator /=/
" Increment/Decrement
syn match lumaOperator /++\|--/
" Access
syn match lumaOperator /::\|\.\|->/
" Delimiters
syn match lumaOperator /[(){}\[\],;:]/
hi def lumaOperator guifg=#fb4934

" =====================
" TYPE ANNOTATIONS
" =====================
syn match lumaTypeAnnotation /:\s*\w\+/ contains=lumaType
hi def lumaTypeAnnotation guifg=#fabd2f gui=italic

" =====================
" VARIABLES
" =====================
syn match lumaVariable /\<[a-zA-Z_][a-zA-Z0-9_]*\>/ contains=NONE
hi def lumaVariable guifg=#ebdbb2

" =====================
" Pointer / Dereference Operators
" =====================
syn match lumaPointerOp /\*\w\+/ contains=NONE
syn match lumaAddressOp /&\w\+/ contains=NONE
hi def lumaPointerOp guifg=#d3869b gui=bold
hi def lumaAddressOp guifg=#83a598 gui=bold

let b:current_syntax = "luma"
