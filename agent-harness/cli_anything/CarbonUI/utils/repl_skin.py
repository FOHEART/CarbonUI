# -*- coding: utf-8 -*-
"""carbonui_cli 交互式壳：可注入 reader/printer 的薄 REPL。

每行输入原地起一个 carbonui_cli 子进程执行（CLI 本就无状态），支持：

- ``json``        翻转后续输出的 JSON 模式（回显当前状态）；
- ``exit``/``quit``/EOF   退出；
- ``help``        打印命令表；
- 其余行          按 ``<action> key=value ...`` 解析为 harness 命令。

token 切分支持成对单/双引号（路径含空格）；key=value 中的 value 若以
``@`` 开头（如 ``out=@C:/tmp/x.png``）表示"展开为用户路径"，纯 Python
REPL 的便利语法，CLI 本身不认识 ``@``。
"""

import json
import os
import shlex

from .backend import run

__all__ = ["repl"]


def _split_keyvalue(tokens):
    """把 ``key=value`` 形式的 token 拆成 payload dict；非 k=v 的原样跳过。"""
    payload = {}
    for token in tokens:
        if "=" in token:
            key, _, value = token.partition("=")
            if value.startswith("@"):
                value = os.path.expanduser(value[1:])
            payload[key] = value
    return payload


def repl(reader, printer, cli_bin=None):
    """REPL 主循环。

    :param reader: 无参调用返回一行文本（EOF 返回 None 或 ""）。
    :param printer: 接受一个 str，负责输出（含换行）。
    :param cli_bin: 显式指定 CLI 路径；缺省走 backend 的定位顺序。
    """
    printer("carbonui agent REPL（help 看命令，json 切换输出，exit 退出）")
    json_mode = True
    while True:
        try:
            line = reader()
        except EOFError:
            line = None
        if line is None or line.strip() in ("exit", "quit"):
            break

        line = line.lstrip("\ufeff")
        if not line.strip():
            continue

        if line.strip() == "json":
            json_mode = not json_mode
            printer("json mode: %s" % ("on" if json_mode else "off"))
            continue

        if line.strip() == "help":
            printer("info | snapshot widget=... [shape=] [state=] [text=] [out=]"
                    " | gallery out=... | qss widget=...|all=true")
            continue

        try:
            tokens = shlex.split(line)
        except ValueError as exc:
            printer("解析失败：%s" % exc)
            continue

        if not tokens:
            continue
        action = tokens[0]
        payload = _split_keyvalue(tokens[1:])

        if action == "qss" and payload.get("all") in ("true", "1"):
            payload["all"] = True
        if "width" in payload:
            payload["width"] = int(payload["width"])
        if "height" in payload:
            payload["height"] = int(payload["height"])

        try:
            envelope = run(action, payload, cli_bin=cli_bin)
        except Exception as exc:  # CliError / CliProtocolError
            printer("error: %s" % exc)
            continue

        printer(json.dumps(envelope, ensure_ascii=False,
                           indent=None if json_mode else 2))
