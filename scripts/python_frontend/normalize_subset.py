#!/usr/bin/env python3

import argparse
import ast
import sys


class LoweringError(Exception):
    pass


class Lowerer:
    def __init__(self, diag_name: str) -> None:
        self.diag_name = diag_name

    def fail(self, node: ast.AST, message: str) -> None:
        line = getattr(node, "lineno", 1)
        col = getattr(node, "col_offset", 0) + 1
        raise LoweringError(f"{self.diag_name}:{line}:{col}: {message}")

    def lower_module(self, tree: ast.Module) -> str:
        pieces = []
        for stmt in tree.body:
          if not isinstance(stmt, ast.FunctionDef):
              self.fail(stmt, "only top-level function definitions are supported in Python subset v0")
          pieces.append(self.lower_function(stmt))
        return "\n\n".join(pieces) + "\n"

    def lower_function(self, fn: ast.FunctionDef) -> str:
        if fn.decorator_list:
            self.fail(fn, "decorators are not supported in Python subset v0")
        if fn.returns is None or not self.is_name(fn.returns, "int"):
            self.fail(fn, "functions must declare return type 'int' in Python subset v0")
        if fn.name == "main" and fn.args.args:
            self.fail(fn, "main must have the exact signature 'def main() -> int' in Python subset v0")
        if fn.args.vararg or fn.args.kwarg or fn.args.kwonlyargs or fn.args.defaults or fn.args.kw_defaults:
            self.fail(fn, "default arguments, varargs, and keyword-only arguments are not supported in Python subset v0")

        params = []
        for arg in fn.args.args:
            if arg.annotation is None or not self.is_name(arg.annotation, "int"):
                self.fail(arg, "function parameters must be annotated as 'int' in Python subset v0")
            params.append(f"int {arg.arg}")

        body = "".join(self.lower_stmt(stmt, 1) for stmt in fn.body)
        return f"int {fn.name}({', '.join(params)}) {{\n{body}}}"

    def lower_stmt(self, stmt: ast.stmt, depth: int) -> str:
        ind = "  " * depth
        if isinstance(stmt, ast.AnnAssign):
            if not isinstance(stmt.target, ast.Name):
                self.fail(stmt, "only simple local bindings are supported in Python subset v0")
            if stmt.annotation is None or not self.is_name(stmt.annotation, "int"):
                self.fail(stmt, "local bindings must be annotated as 'int' in Python subset v0")
            if stmt.value is None:
                self.fail(stmt, "local bindings must have an initializer in Python subset v0")
            return f"{ind}int {stmt.target.id} = {self.lower_expr(stmt.value)};\n"
        if isinstance(stmt, ast.Assign):
            if len(stmt.targets) != 1 or not isinstance(stmt.targets[0], ast.Name):
                self.fail(stmt, "only simple assignment targets are supported in Python subset v0")
            return f"{ind}{stmt.targets[0].id} = {self.lower_expr(stmt.value)};\n"
        if isinstance(stmt, ast.Return):
            if stmt.value is None:
                self.fail(stmt, "return statements must return an expression in Python subset v0")
            return f"{ind}return {self.lower_expr(stmt.value)};\n"
        if isinstance(stmt, ast.If):
            text = f"{ind}if ({self.lower_expr(stmt.test)}) {{\n"
            text += "".join(self.lower_stmt(child, depth + 1) for child in stmt.body)
            text += f"{ind}}}"
            if stmt.orelse:
                if len(stmt.orelse) == 1 and isinstance(stmt.orelse[0], ast.If):
                    text += " else " + self.lower_stmt(stmt.orelse[0], depth).strip()
                else:
                    text += " else {\n"
                    text += "".join(self.lower_stmt(child, depth + 1) for child in stmt.orelse)
                    text += f"{ind}}}"
            return text + "\n"
        if isinstance(stmt, ast.Expr):
            return f"{ind}{self.lower_expr(stmt.value)};\n"
        if isinstance(stmt, ast.While):
            self.fail(stmt, "'while' is not supported in Python subset v0")
        if isinstance(stmt, ast.For):
            self.fail(stmt, "'for' is not supported in Python subset v0")
        self.fail(stmt, "unsupported statement in Python subset v0")

    def lower_expr(self, expr: ast.expr) -> str:
        if isinstance(expr, ast.Name):
            return expr.id
        if isinstance(expr, ast.Constant):
            if isinstance(expr.value, bool):
                return "1" if expr.value else "0"
            if isinstance(expr.value, int):
                return str(expr.value)
            self.fail(expr, "only integer and boolean literals are supported in Python subset v0")
        if isinstance(expr, ast.UnaryOp):
            if isinstance(expr.op, ast.USub):
                return f"(-{self.lower_expr(expr.operand)})"
            if isinstance(expr.op, ast.Not):
                return f"(!{self.lower_expr(expr.operand)})"
            self.fail(expr, "only unary minus and logical not are supported in Python subset v0")
        if isinstance(expr, ast.BoolOp):
            op = "&&" if isinstance(expr.op, ast.And) else "||" if isinstance(expr.op, ast.Or) else None
            if op is None:
                self.fail(expr, "unsupported boolean operator in Python subset v0")
            if len(expr.values) < 2:
                self.fail(expr, "boolean expressions must have at least two operands")
            text = self.lower_expr(expr.values[0])
            for value in expr.values[1:]:
                text = f"({text} {op} {self.lower_expr(value)})"
            return text
        if isinstance(expr, ast.BinOp):
            op_map = {
                ast.Add: "+",
                ast.Sub: "-",
                ast.Mult: "*",
                ast.Div: "/",
                ast.Mod: "%",
                ast.BitAnd: "&",
                ast.BitOr: "|",
                ast.BitXor: "^",
                ast.LShift: "<<",
                ast.RShift: ">>",
            }
            for ty, spelling in op_map.items():
                if isinstance(expr.op, ty):
                    return f"({self.lower_expr(expr.left)} {spelling} {self.lower_expr(expr.right)})"
            self.fail(expr, "unsupported binary operator in Python subset v0")
        if isinstance(expr, ast.Compare):
            if len(expr.ops) != 1 or len(expr.comparators) != 1:
                self.fail(expr, "chained comparisons are not supported in Python subset v0")
            op_map = {
                ast.Eq: "==",
                ast.NotEq: "!=",
                ast.Lt: "<",
                ast.LtE: "<=",
                ast.Gt: ">",
                ast.GtE: ">=",
            }
            for ty, spelling in op_map.items():
                if isinstance(expr.ops[0], ty):
                    return f"({self.lower_expr(expr.left)} {spelling} {self.lower_expr(expr.comparators[0])})"
            self.fail(expr, "unsupported comparison operator in Python subset v0")
        if isinstance(expr, ast.Call):
            if not isinstance(expr.func, ast.Name):
                self.fail(expr, "only direct same-file helper calls are supported in Python subset v0")
            if expr.keywords:
                self.fail(expr, "keyword arguments are not supported in Python subset v0")
            return f"{expr.func.id}({', '.join(self.lower_expr(arg) for arg in expr.args)})"
        if isinstance(expr, ast.Subscript):
            self.fail(expr, "indexing is not supported in Python subset v0")
        if isinstance(expr, ast.Attribute):
            self.fail(expr, "attribute access is not supported in Python subset v0")
        if isinstance(expr, ast.List):
            self.fail(expr, "lists are not supported in Python subset v0")
        self.fail(expr, "unsupported expression in Python subset v0")

    @staticmethod
    def is_name(node: ast.AST, ident: str) -> bool:
        return isinstance(node, ast.Name) and node.id == ident


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--input", required=True)
    ap.add_argument("--output", required=True)
    ap.add_argument("--diag-name", required=True)
    args = ap.parse_args()

    try:
        with open(args.input, "r", encoding="utf-8") as f:
            source = f.read()
        tree = ast.parse(source, filename=args.diag_name)
        c_source = Lowerer(args.diag_name).lower_module(tree)
        with open(args.output, "w", encoding="utf-8") as f:
            f.write(c_source)
        return 0
    except SyntaxError as exc:
        line = exc.lineno or 1
        col = exc.offset or 1
        sys.stderr.write(f"{args.diag_name}:{line}:{col}: {exc.msg}\n")
        return 1
    except LoweringError as exc:
        sys.stderr.write(str(exc) + "\n")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
