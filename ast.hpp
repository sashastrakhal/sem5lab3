#ifndef AST_HPP
#define AST_HPP

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <fstream>
#include <sstream>

enum class NodeType {
    PROGRAM,
    BLOCK,
    VAR_DECL,
    ASSIGN,
    BIN_OP,
    NUMBER,
    IDENTIFIER,
    IF_STMT,
    WHILE_STMT,
    PRINT_STMT
};

struct ASTNode {
    NodeType type;
    std::string value;
    int num_val = 0;
    std::vector<ASTNode*> children;

    ASTNode(NodeType t, std::string v = "", int n = 0)
        : type(t), value(v), num_val(n) {}

    void add(ASTNode* child) {
        if (child) children.push_back(child);
    }

    ~ASTNode() {
        for (auto* c : children) delete c;
    }
};

extern ASTNode* root;

void interpret(ASTNode* node);
void generate_python(ASTNode* node, const std::string& filename);
void generate_tac(ASTNode* node, const std::string& filename);
void generate_d3_html(ASTNode* node, const std::string& filename);

#endif