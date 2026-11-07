%{
#include "ast.hpp"
#include <iostream>
#include <cstdio>
#include <cstdlib>

extern int yylex();
extern int yylineno;
extern FILE* yyin;

void yyerror(const char* s) {
    std::cerr << "[Парсер] Синтаксична помилка на рядку " << yylineno << ": " << s << std::endl;
}

ASTNode* root = nullptr;
%}

%union {
    int num;
    char* str;
    ASTNode* node;
}

%token <num> T_NUMBER
%token <str> T_ID
%token T_INT T_IF T_ELSE T_WHILE T_PRINT
%token T_EQ T_NEQ T_LE T_GE T_LT T_GT

/* Розв'язання конфлікту Shift-Reduce для Dangling Else */
%nonassoc LOWER_THAN_ELSE
%nonassoc T_ELSE

/* Пріоритети операторів*/
%left T_EQ T_NEQ
%left T_LT T_GT T_LE T_GE
%left '+' '-'
%left '*' '/'
%right UMINUS

%type <node> program stmt_list stmt block var_decl assign_stmt if_stmt while_stmt print_stmt expr

%%

program:
    stmt_list {
        root = new ASTNode(NodeType::PROGRAM, "Program");
        root->add($1);
    }
    ;

stmt_list:
    stmt {
        $$ = new ASTNode(NodeType::BLOCK, "Statements");
        $$->add($1);
    }
    | stmt_list stmt {
        $1->add($2);
        $$ = $1;
    }
    ;

stmt:
    var_decl ';'    { $$ = $1; }
    | assign_stmt ';' { $$ = $1; }
    | if_stmt       { $$ = $1; }
    | while_stmt    { $$ = $1; }
    | print_stmt ';' { $$ = $1; }
    | block         { $$ = $1; }
    ;

block:
    '{' stmt_list '}' { $$ = $2; }
    | '{' '}'         { $$ = new ASTNode(NodeType::BLOCK, "EmptyBlock"); }
    ;

var_decl:
    T_INT T_ID {
        $$ = new ASTNode(NodeType::VAR_DECL, $2);
        free($2);
    }
    | T_INT T_ID '=' expr {
        $$ = new ASTNode(NodeType::VAR_DECL, $2);
        $$->add($4);
        free($2);
    }
    ;

assign_stmt:
    T_ID '=' expr {
        $$ = new ASTNode(NodeType::ASSIGN, $1);
        $$->add($3);
        free($1);
    }
    ;

if_stmt:
    T_IF '(' expr ')' stmt %prec LOWER_THAN_ELSE {
        $$ = new ASTNode(NodeType::IF_STMT, "if");
        $$->add($3);
        $$->add($5); 
    }
    | T_IF '(' expr ')' stmt T_ELSE stmt {
        $$ = new ASTNode(NodeType::IF_STMT, "if-else");
        $$->add($3);
        $$->add($5);
        $$->add($7); 
    }
    ;

while_stmt:
    T_WHILE '(' expr ')' stmt {
        $$ = new ASTNode(NodeType::WHILE_STMT, "while");
        $$->add($3); 
        $$->add($5); 
    }
    ;

print_stmt:
    T_PRINT '(' expr ')' {
        $$ = new ASTNode(NodeType::PRINT_STMT, "print");
        $$->add($3);
    }
    ;

expr:
    T_NUMBER                { $$ = new ASTNode(NodeType::NUMBER, std::to_string($1), $1); }
    | T_ID                  { $$ = new ASTNode(NodeType::IDENTIFIER, $1); free($1); }
    | expr '+' expr         { $$ = new ASTNode(NodeType::BIN_OP, "+"); $$->add($1); $$->add($3); }
    | expr '-' expr         { $$ = new ASTNode(NodeType::BIN_OP, "-"); $$->add($1); $$->add($3); }
    | expr '*' expr         { $$ = new ASTNode(NodeType::BIN_OP, "*"); $$->add($1); $$->add($3); }
    | expr '/' expr         { $$ = new ASTNode(NodeType::BIN_OP, "/"); $$->add($1); $$->add($3); }
    | expr T_EQ expr        { $$ = new ASTNode(NodeType::BIN_OP, "=="); $$->add($1); $$->add($3); }
    | expr T_NEQ expr       { $$ = new ASTNode(NodeType::BIN_OP, "!="); $$->add($1); $$->add($3); }
    | expr T_LT expr        { $$ = new ASTNode(NodeType::BIN_OP, "<"); $$->add($1); $$->add($3); }
    | expr T_GT expr        { $$ = new ASTNode(NodeType::BIN_OP, ">"); $$->add($1); $$->add($3); }
    | expr T_LE expr        { $$ = new ASTNode(NodeType::BIN_OP, "<="); $$->add($1); $$->add($3); }
    | expr T_GE expr        { $$ = new ASTNode(NodeType::BIN_OP, ">="); $$->add($1); $$->add($3); }
    | '-' expr %prec UMINUS {
        $$ = new ASTNode(NodeType::BIN_OP, "-");
        $$->add(new ASTNode(NodeType::NUMBER, "0", 0));
        $$->add($2);
    }
    | '(' expr ')'          { $$ = $2; }
    ;

%%

int main(int argc, char** argv) {
    if (argc > 1) {
        yyin = fopen(argv[1], "r");
        if (!yyin) {
            std::cerr << "Не вдалося відкрити файл: " << argv[1] << "\n";
            return 1;
        }
    }

    std::cout << "=== 1. Синтаксичний аналіз та побудова AST ===\n";
    if (yyparse() == 0 && root != nullptr) {
        std::cout << "AST успішно побудовано!\n\n";

        std::cout << "=== 2. Семантика: Інтерпретація (обчислення AST) ===\n";
        interpret(root);

        std::cout << "\n=== 3. Трансляція у цільові мови ===\n";
        generate_python(root, "output.py");
        generate_tac(root, "output.tac");

        std::cout << "\n=== 4. Генерація візуалізації D3.js ===\n";
        generate_d3_html(root, "ast_visual.html");

        delete root;
    } else {
        std::cerr << "Помилка розбору програми.\n"; 
        return 1;
    }
    return 0;
}