# -*- coding: utf-8 -*-
"""Click 入口：把 carbonui_cli 暴露为 ``carbonui-agent`` 命令。

用法（``pip install -e agent-harness`` 后）::

    carbonui-agent info
    carbonui-agent snapshot --widget button --shape rect --out shot.png
    carbonui-agent repl

所有子命令输出单行 JSON 信封（与 CLI 的 ``--json`` 一致）。
"""

import json
import sys

import click

from .utils.backend import CliError, find_cli_bin, run
from .utils.repl_skin import repl


@click.group()
@click.option("--cli-bin", default=None, help="carbonui_cli 路径（默认自动定位）")
@click.option("--timeout", default=60.0, show_default=True, help="单条命令超时秒数")
@click.pass_context
def main(ctx, cli_bin, timeout):
    """控制 CarbonUI 样式库的命令行接口（渲染控件截图 / 导出 QSS）。"""
    ctx.ensure_object(dict)
    ctx.obj["cli_bin"] = cli_bin or find_cli_bin()
    ctx.obj["timeout"] = timeout


@main.command()
@click.pass_context
def info(ctx):
    """库版本 / Qt 版本 / QPA / 默认字体族。"""
    _emit(ctx, run("info", cli_bin=ctx.obj["cli_bin"], timeout=ctx.obj["timeout"]))


@main.command()
@click.option("--widget", required=True,
              type=click.Choice(["button", "radio", "checkbox", "lineedit",
                                 "tab", "combo", "tree", "statusbar", "groupbox"]),
              help="控件类型")
@click.option("--shape", default=None,
              type=click.Choice(["round", "left", "right", "rect"]),
              help="外形（仅 button/radio）")
@click.option("--state", default=None,
              type=click.Choice(["normal", "disabled", "checked", "collapsed"]),
              help="控件状态")
@click.option("--text", default=None, help="控件文字")
@click.option("--width", default=None, type=int, help="覆盖宽度（像素）")
@click.option("--height", default=None, type=int, help="覆盖高度（像素）")
@click.option("--out", default=None, help="PNG 输出路径")
@click.pass_context
def snapshot(ctx, widget, shape, state, text, width, height, out):
    """渲染单个控件到 PNG。"""
    payload = {"widget": widget, "shape": shape, "state": state,
               "text": text, "width": width, "height": height, "out": out}
    _emit(ctx, run("snapshot", payload,
                   cli_bin=ctx.obj["cli_bin"], timeout=ctx.obj["timeout"]))


@main.command()
@click.option("--out", default=None, help="PNG 输出路径")
@click.pass_context
def gallery(ctx, out):
    """渲染整套控件总览到 PNG。"""
    _emit(ctx, run("gallery", {"out": out},
                   cli_bin=ctx.obj["cli_bin"], timeout=ctx.obj["timeout"]))


@main.command("qss")
@click.option("--widget", default=None, help="控件类型（与 --all 二选一）")
@click.option("--all", "export_all", is_flag=True, help="导出全部控件的 QSS")
@click.option("--shape", default=None, help="外形（仅 button/radio）")
@click.pass_context
def qss_cmd(ctx, widget, export_all, shape):
    """导出控件 QSS 文本。"""
    payload = {"widget": widget, "shape": shape}
    if export_all:
        payload = {"all": True}
    _emit(ctx, run("qss", payload,
                   cli_bin=ctx.obj["cli_bin"], timeout=ctx.obj["timeout"]))


@main.command()
@click.pass_context
def repl_cmd(ctx):
    """交互式 REPL（每行一条命令）。"""
    repl(input, lambda text: print(text), cli_bin=ctx.obj["cli_bin"])


main.add_command(repl_cmd, name="repl")


def _emit(ctx, envelope):
    click.echo(json.dumps(envelope, ensure_ascii=False))
    if not envelope.get("ok"):
        ctx.exit(1)


if __name__ == "__main__":
    sys.exit(main())
