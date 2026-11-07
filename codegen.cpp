#include "ast.hpp"

static std::map<std::string, int> sym_table;

int eval(ASTNode* node) {
    if (!node) return 0;

    switch (node->type) {
        case NodeType::PROGRAM:
        case NodeType::BLOCK:
            for (auto* child : node->children) eval(child);
            return 0;

        case NodeType::VAR_DECL: {
            int val = node->children.empty() ? 0 : eval(node->children[0]);
            sym_table[node->value] = val;
            return val;
        }

        case NodeType::ASSIGN: {
            if (sym_table.find(node->value) == sym_table.end()) {
                std::cerr << "[Семантична помилка] Неоголошена змінна: " << node->value << "\n";
            }
            int val = eval(node->children[0]);
            sym_table[node->value] = val;
            return val;
        }

        case NodeType::NUMBER:
            return node->num_val;

        case NodeType::IDENTIFIER:
            if (sym_table.find(node->value) == sym_table.end()) {
                std::cerr << "[Семантична помилка] Використання неоголошеної змінної: " << node->value << "\n";
                return 0;
            }
            return sym_table[node->value];

        case NodeType::BIN_OP: {
            int l = eval(node->children[0]);
            int r = eval(node->children[1]);
            const std::string& op = node->value;
            if (op == "+")  return l + r;
            if (op == "-")  return l - r;
            if (op == "*")  return l * r;
            if (op == "/")  return r != 0 ? l / r : 0;
            if (op == "==") return l == r;
            if (op == "!=") return l != r;
            if (op == "<")  return l < r;
            if (op == ">")  return l > r;
            if (op == "<=") return l <= r;
            if (op == ">=") return l >= r;
            return 0;
        }

        case NodeType::IF_STMT:
            if (eval(node->children[0])) {
                eval(node->children[1]);
            } else if (node->children.size() > 2) {
                eval(node->children[2]);
            }
            return 0;

        case NodeType::WHILE_STMT:
            while (eval(node->children[0])) {
                eval(node->children[1]);
            }
            return 0;

        case NodeType::PRINT_STMT: {
            int val = eval(node->children[0]);
            std::cout << "[Вивід програми] > " << val << "\n";
            return 0;
        }
    }
    return 0;
}

void interpret(ASTNode* node) {
    sym_table.clear();
    eval(node);
    std::cout << "Стан таблиці символів після виконання: ";
    for (const auto& [k, v] : sym_table) {
        std::cout << k << "=" << v << " ";
    }
    std::cout << "\n";
}

static std::string indent(int level) {
    return std::string(level * 4, ' ');
}

static std::string py_expr(ASTNode* node) {
    if (!node) return "";
    if (node->type == NodeType::NUMBER || node->type == NodeType::IDENTIFIER) {
        return node->value;
    }
    if (node->type == NodeType::BIN_OP) {
        std::string op = (node->value == "/") ? "//" : node->value; // Цілочисельне ділення
        return "(" + py_expr(node->children[0]) + " " + op + " " + py_expr(node->children[1]) + ")";
    }
    return "";
}

static void py_stmt(ASTNode* node, std::ostream& out, int lvl) {
    if (!node) return;
    switch (node->type) {
        case NodeType::PROGRAM:
            out << "# Автоматично згенеровано транслятором C -> Python\n";
            py_stmt(node->children[0], out, lvl);
            break;
        case NodeType::BLOCK:
            if (node->children.empty()) {
                out << indent(lvl) << "pass\n";
            } else {
                for (auto* c : node->children) py_stmt(c, out, lvl);
            }
            break;
        case NodeType::VAR_DECL:
            out << indent(lvl) << node->value << " = "
                << (node->children.empty() ? "0" : py_expr(node->children[0])) << "\n";
            break;
        case NodeType::ASSIGN:
            out << indent(lvl) << node->value << " = " << py_expr(node->children[0]) << "\n";
            break;
        case NodeType::PRINT_STMT:
            out << indent(lvl) << "print(" << py_expr(node->children[0]) << ")\n";
            break;
        case NodeType::IF_STMT: {
            out << indent(lvl) << "if " << py_expr(node->children[0]) << ":\n";
            int next_lvl = (node->children[1]->type == NodeType::BLOCK) ? lvl + 1 : lvl + 1;
            py_stmt(node->children[1], out, next_lvl);
            if (node->children.size() > 2) {
                out << indent(lvl) << "else:\n";
                py_stmt(node->children[2], out, next_lvl);
            }
            break;
        }
        case NodeType::WHILE_STMT:
            out << indent(lvl) << "while " << py_expr(node->children[0]) << ":\n";
            py_stmt(node->children[1], out, lvl + 1);
            break;
        default:
            break;
    }
}

void generate_python(ASTNode* node, const std::string& filename) {
    std::ofstream out(filename);
    py_stmt(node, out, 0);
    out.close();
    std::cout << "[Кодогенератор] Згенеровано код Python: " << filename << "\n";
}

static int temp_cnt = 0;
static int label_cnt = 0;

static std::string new_temp() { return "t" + std::to_string(++temp_cnt); }
static std::string new_label() { return "L" + std::to_string(++label_cnt); }

static std::string tac_gen(ASTNode* node, std::ostream& out) {
    if (!node) return "";
    switch (node->type) {
        case NodeType::PROGRAM:
        case NodeType::BLOCK:
            for (auto* c : node->children) tac_gen(c, out);
            return "";
        case NodeType::NUMBER:
        case NodeType::IDENTIFIER:
            return node->value;
        case NodeType::VAR_DECL: {
            std::string rhs = node->children.empty() ? "0" : tac_gen(node->children[0], out);
            out << "    " << node->value << " = " << rhs << "\n";
            return node->value;
        }
        case NodeType::ASSIGN: {
            std::string rhs = tac_gen(node->children[0], out);
            out << "    " << node->value << " = " << rhs << "\n";
            return node->value;
        }
        case NodeType::BIN_OP: {
            std::string l = tac_gen(node->children[0], out);
            std::string r = tac_gen(node->children[1], out);
            std::string t = new_temp();
            out << "    " << t << " = " << l << " " << node->value << " " << r << "\n";
            return t;
        }
        case NodeType::PRINT_STMT: {
            std::string v = tac_gen(node->children[0], out);
            out << "    PARAM " << v << "\n";
            out << "    CALL print, 1\n";
            return "";
        }
        case NodeType::IF_STMT: {
            std::string cond = tac_gen(node->children[0], out);
            std::string l_false = new_label();
            std::string l_end = new_label();
            out << "    IF_FALSE " << cond << " GOTO " << l_false << "\n";
            tac_gen(node->children[1], out);
            if (node->children.size() > 2) {
                out << "    GOTO " << l_end << "\n";
                out << l_false << ":\n";
                tac_gen(node->children[2], out);
                out << l_end << ":\n";
            } else {
                out << l_false << ":\n";
            }
            return "";
        }
        case NodeType::WHILE_STMT: {
            std::string l_start = new_label();
            std::string l_exit = new_label();
            out << l_start << ":\n";
            std::string cond = tac_gen(node->children[0], out);
            out << "    IF_FALSE " << cond << " GOTO " << l_exit << "\n";
            tac_gen(node->children[1], out);
            out << "    GOTO " << l_start << "\n";
            out << l_exit << ":\n";
            return "";
        }
    }
    return "";
}

void generate_tac(ASTNode* node, const std::string& filename) {
    temp_cnt = 0; label_cnt = 0;
    std::ofstream out(filename);
    out << "; Триадресний код (Three-Address Code IR)\n";
    tac_gen(node, out);
    out.close();
    std::cout << "[Кодогенератор] Згенеровано триадресний код (TAC): " << filename << "\n";
}

static std::string type_to_str(NodeType t) {
    switch (t) {
        case NodeType::PROGRAM:    return "PROGRAM";
        case NodeType::BLOCK:      return "BLOCK";
        case NodeType::VAR_DECL:   return "VAR_DECL";
        case NodeType::ASSIGN:     return "ASSIGN";
        case NodeType::BIN_OP:     return "BIN_OP";
        case NodeType::NUMBER:     return "NUMBER";
        case NodeType::IDENTIFIER: return "ID";
        case NodeType::IF_STMT:    return "IF";
        case NodeType::WHILE_STMT: return "WHILE";
        case NodeType::PRINT_STMT: return "PRINT";
    }
    return "NODE";
}

static void ast_to_json(ASTNode* node, std::ostream& out) {
    if (!node) return;
    out << "{\n";
    out << "  \"name\": \"" << type_to_str(node->type) << ": " << node->value << "\",\n";
    out << "  \"type\": \"" << type_to_str(node->type) << "\"";
    if (!node->children.empty()) {
        out << ",\n  \"children\": [\n";
        for (size_t i = 0; i < node->children.size(); ++i) {
            ast_to_json(node->children[i], out);
            if (i + 1 < node->children.size()) out << ",";
        }
        out << "\n  ]";
    }
    out << "\n}";
}

void generate_d3_html(ASTNode* node, const std::string& filename) {
    std::ofstream out(filename);
    out << R"HTML(<!DOCTYPE html>
<html lang="uk">
<head>
    <meta charset="UTF-8">
    <title>Візуалізація AST (D3.js)</title>
    <script src="https://d3js.org/d3.v7.min.js"></script>
    <style>
        body { margin: 0; font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background: #0f172a; color: #f8fafc; overflow: hidden; }
        #header { position: absolute; top: 15px; left: 20px; background: rgba(30, 41, 59, 0.85); padding: 12px 20px; border-radius: 10px; border: 1px solid #334155; pointer-events: none; }
        h2 { margin: 0 0 5px 0; font-size: 18px; color: #38bdf8; }
        p { margin: 0; font-size: 13px; color: #94a3b8; }
        .link { fill: none; stroke: #475569; stroke-width: 2px; }
        .node circle { stroke-width: 2.5px; cursor: pointer; transition: r 0.2s; }
        .node circle:hover { r: 10px; }
        .node text { font-size: 13px; font-weight: 500; fill: #f1f5f9; text-shadow: 0 1px 3px #000; }
    </style>
</head>
<body>
    <div id="header">
        <h2>Абстрактне синтаксичне дерево (AST)</h2>
        <p>Коліщатко миші — масштаб (Zoom), затиснути ЛКМ — переміщення (Pan)</p>
    </div>
    <script>
        const treeData = )HTML";

    ast_to_json(node, out);

    out << R"HTML(;

        const width = window.innerWidth;
        const height = window.innerHeight;

        const colorMap = {
            "PROGRAM": "#f43f5e",
            "BLOCK": "#64748b",
            "IF": "#a855f7",
            "WHILE": "#ec4899",
            "VAR_DECL": "#3b82f6",
            "ASSIGN": "#0ea5e9",
            "BIN_OP": "#eab308",
            "NUMBER": "#22c55e",
            "ID": "#10b981",
            "PRINT": "#f97316"
        };

        const svg = d3.select("body").append("svg")
            .attr("width", width)
            .attr("height", height);

        const g = svg.append("g");

        svg.call(d3.zoom()
            .scaleExtent([0.2, 3])
            .on("zoom", (event) => g.attr("transform", event.transform)));

        const root = d3.hierarchy(treeData);
        const treeLayout = d3.tree().nodeSize([140, 90]);
        treeLayout(root);

        // Центруємо дерево на екрані
        svg.call(d3.zoom().transform, d3.zoomIdentity.translate(width / 2, 80).scale(0.9));

        g.selectAll(".link")
            .data(root.links())
            .enter().append("path")
            .attr("class", "link")
            .attr("d", d3.linkVertical().x(d => d.x).y(d => d.y));

        const node = g.selectAll(".node")
            .data(root.descendants())
            .enter().append("g")
            .attr("class", "node")
            .attr("transform", d => `translate(${d.x},${d.y})`);

        node.append("circle")
            .attr("r", 7)
            .style("fill", d => colorMap[d.data.type] || "#38bdf8")
            .style("stroke", "#0f172a");

        node.append("text")
            .attr("dy", -12)
            .attr("text-anchor", "middle")
            .text(d => d.data.name);
    </script>
</body>
</html>)HTML";

    out.close();
    std::cout << "[Візуалізація] Інтерактивне дерево D3.js збережено у: " << filename << "\n";
}