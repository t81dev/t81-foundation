#!/usr/bin/env python3
# File: t81_compile.py
# Purpose: T81Lang CLI compiler — tokenize, parse, emit TISC, generate .cweb metadata

import re
import json
import argparse
import os

# --- Token Specs ---
KEYWORDS = {"let", "mut", "fn", "return"}
TYPES = {"T81Int", "T81Float", "T81BigInt", "T81Fraction", "Symbol"}
ANNOTATIONS = {"@entropy", "@tag"}

TOKEN_SPEC = [
    ("NUMBER",    r"[0-9]+t"),
    ("SYMBOL",    r"[⍺-⍵βγδθσφψΩ]+"),
    ("IDENT",     r"[a-zA-Z_][a-zA-Z0-9_]*"),
    ("OP",        r"[+\-*/=<>!]"),
    ("LPAREN",    r"\("),
    ("RPAREN",    r"\)"),
    ("LBRACE",    r"\{"),
    ("RBRACE",    r"\}"),
    ("COLON",     r":"),
    ("ARROW",     r"->"),
    ("SEMICOLON", r";"),
    ("EQUAL",     r"="),
    ("AT",        r"@[a-zA-Z_]+"),
    ("FLOAT",     r"[0-9]+\.[0-9]+"),
    ("WHITESPACE",r"\s+"),
]

tok_regex = '|'.join(f'(?P<{name}>{pattern})' for name, pattern in TOKEN_SPEC)

# --- Tokenizer ---
def tokenize(code):
    tokens = []
    for match in re.finditer(tok_regex, code):
        kind = match.lastgroup
        value = match.group()
        if kind != "WHITESPACE":
            tokens.append((kind, value))
    return tokens

# --- AST Node ---
class ASTNode:
    def __init__(self, node_type, **kwargs):
        self.node_type = node_type
        self.fields = kwargs

    def to_dict(self):
        return {"type": self.node_type, **self.fields}

# --- Entropy Tracker ---
entropy_log = []

def log_entropy(name, entropy, tag):
    entropy_log.append({
        "symbol": name,
        "entropy": entropy,
        "tag": tag
    })

# --- Parser Helpers ---
def parse_expression(tokens):
    if len(tokens) == 1:
        return ASTNode("Literal", value=tokens[0][1])
    elif len(tokens) == 3 and tokens[1][0] == "OP":
        return ASTNode("BinaryExpr", left=tokens[0][1], op=tokens[1][1], right=tokens[2][1])
    return ASTNode("Unknown")

# --- Statement Parsers ---
def parse_let_statement(tokens):
    assert tokens[0][1] == "let"
    name = tokens[1][1]
    type_hint = tokens[3][1] if tokens[2][0] == "COLON" else None
    expr_tokens = tokens[5:-1]
    expr = parse_expression(expr_tokens)

    entropy = None
    tag = None
    for i, t in enumerate(tokens):
        if t[1] == "@entropy" and tokens[i+1][0] == "LPAREN":
            entropy = float(tokens[i+2][1])
        elif t[1] == "@tag" and tokens[i+1][0] == "LPAREN":
            tag = tokens[i+2][1].strip('"')

    if entropy or tag:
        log_entropy(name, entropy, tag)

    return ASTNode("Let", name=name, type=type_hint, expr=expr, entropy=entropy, tag=tag)

def parse_return_statement(tokens):
    assert tokens[0][1] == "return"
    expr = parse_expression(tokens[1:-1])
    return ASTNode("Return", expr=expr)

def parse_function(tokens):
    fn_name = tokens[1][1]
    param_tokens = tokens[2:tokens.index(('RPAREN', ')'))]
    params = []
    i = 1
    while i < len(param_tokens):
        if param_tokens[i][0] == "IDENT" and param_tokens[i+1][0] == "COLON":
            param_name = param_tokens[i][1]
            param_type = param_tokens[i+2][1]
            params.append({"name": param_name, "type": param_type})
            i += 3
        else:
            i += 1

    return_type = tokens[tokens.index(('ARROW', '->')) + 1][1]
    body_tokens = tokens[tokens.index(('LBRACE', '{')) + 1 : tokens.index(('RBRACE', '}'))]

    statements = []
    i = 0
    while i < len(body_tokens):
        if body_tokens[i][1] == "let":
            end = i
            while body_tokens[end][0] != "SEMICOLON":
                end += 1
            statements.append(parse_let_statement(body_tokens[i:end+1]))
            i = end + 1
        elif body_tokens[i][1] == "return":
            statements.append(parse_return_statement(body_tokens[i:i+3]))
            i += 3
        else:
            i += 1

    return ASTNode("Function", name=fn_name, params=params, returns=return_type, body=statements)

def parse_program(tokens):
    functions = []
    i = 0
    while i < len(tokens):
        if tokens[i][1] == "fn":
            start = i
            depth = 0
            while i < len(tokens):
                if tokens[i][0] == "LBRACE":
                    depth += 1
                elif tokens[i][0] == "RBRACE":
                    depth -= 1
                    if depth == 0:
                        break
                i += 1
            fn_tokens = tokens[start:i+1]
            functions.append(parse_function(fn_tokens))
        i += 1
    return ASTNode("Program", functions=[fn.to_dict() for fn in functions])

# --- TISC Codegen ---
def emit_tisc(ast_node):
    if ast_node.node_type == "Program":
        return '\n\n'.join(emit_tisc(ASTNode("Function", **fn)) for fn in ast_node.fields['functions'])
    meta = ""
    if ast_node.node_type == "Let":
        code = emit_tisc(ast_node.fields['expr'])
        if ast_node.fields.get("entropy"):
            meta += f"; @entropy({ast_node.fields['entropy']})\n"
        if ast_node.fields.get("tag"):
            meta += f"; @tag(\"{ast_node.fields['tag']}\")\n"
        return f"{meta}{code}\nSTORE {ast_node.fields['name']}"
    elif ast_node.node_type == "BinaryExpr":
        return f"LOAD {ast_node.fields['left']}\n{ast_node.fields['op'].upper()} {ast_node.fields['right']}"
    elif ast_node.node_type == "Literal":
        return f"LOAD {ast_node.fields['value']}"
    elif ast_node.node_type == "Return":
        return f"{emit_tisc(ast_node.fields['expr'])}\nRETURN"
    elif ast_node.node_type == "Function":
        header = f"FUNC {ast_node.fields['name']} RETURNS {ast_node.fields['returns']}"
        body = '\n'.join([emit_tisc(stmt) for stmt in ast_node.fields['body']])
        return f"{header}\n{body}\nENDFUNC"

# --- .cweb Generator ---
def generate_cweb_module(module_name, version="0.1.0"):
    return {
        "@name": module_name,
        "@version": version,
        "@description": f"Auto-generated from T81Lang parser ({module_name})",
        "@license": "GPL-3.0",
        "@source": {
            "type": "local",
            "path": f"./{module_name}/"
        },
        "@build": {
            "system": "custom",
            "flags": ["-DUSE_AXION"]
        },
        "@dependencies": {
            "runtime": []
        },
        "@ai": {
            "optimize": True,
            "entropy-feedback": True
        },
        "@split": {
            "enabled": False,
            "max_size_mb": 50
        },
        "@symbols": entropy_log
    }

# --- CLI Entry Point ---
def main():
    parser = argparse.ArgumentParser(description="T81Lang Compiler — Parser → TISC + .cweb")
    parser.add_argument("--input", type=str, required=True, help="Path to .t81 source file")
    parser.add_argument("--emit-cweb", action="store_true", help="Also emit .cweb metadata")
    parser.add_argument("--out", type=str, default="out/", help="Output folder")
    args = parser.parse_args()

    os.makedirs(args.out, exist_ok=True)
    with open(args.input, 'r', encoding='utf-8') as f:
        source = f.read()

    module_name = os.path.splitext(os.path.basename(args.input))[0]
    tokens = tokenize(source)
    ast = parse_program(tokens)

    # Write AST JSON
    with open(f"{args.out}/{module_name}.ast.json", 'w') as f:
        json.dump(ast.to_dict(), f, indent=2)

    # Write TISC
    with open(f"{args.out}/{module_name}.tisc", 'w') as f:
        f.write(emit_tisc(ast))

    # Write entropy log
    with open(f"{args.out}/{module_name}.entropy.json", 'w') as f:
        json.dump(entropy_log, f, indent=2)

    # Optionally write .cweb
    if args.emit_cweb:
        with open(f"{args.out}/{module_name}.cweb", 'w') as f:
            json.dump(generate_cweb_module(module_name), f, indent=2)

    print(f"✅ Compiled '{args.input}' → {args.out}")

if __name__ == "__main__":
    main()
