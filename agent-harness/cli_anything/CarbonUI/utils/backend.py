# -*- coding: utf-8 -*-
"""carbonui_cli 的 Python backend：argv 构造 + subprocess 调用 + 信封解析。

协议（与 SKILL.md 一致）：
- stdout 最后一行必须是单行 JSON 信封 {"ok":bool,"code":int,"message":str,"data":{}}；
- 进程退出码只分 0（成功）/ 1（失败），细分错误在信封 code；
- 自动化一律走 ``--json``，不要解析人类可读文本。

二进制定位顺序：环境变量 CARBONUI_CLI_BIN > 部署目录（与 agent-harness 同级）
> 仓库构建产物目录 > PATH。
"""

import json
import os
import shutil
import subprocess

__all__ = [
    "CliError",
    "CliProtocolError",
    "build_argv",
    "find_cli_bin",
    "run",
]

#: 环境变量名：显式指定 carbonui_cli 可执行文件路径。
CLI_BIN_ENV = "CARBONUI_CLI_BIN"


class CliError(Exception):
    """CLI 起不来、超时或业务失败（信封 ok=false）。"""


class CliProtocolError(CliError):
    """stdout 不是合法信封——通常是 CLI 版本与 harness 不匹配。"""


def _repo_root():
    """从本文件位置反推仓库/部署根（utils -> CarbonUI -> cli_anything -> agent-harness -> 根）。"""
    here = os.path.dirname(os.path.abspath(__file__))
    return os.path.normpath(os.path.join(here, "..", "..", "..", ".."))


def _candidate_paths():
    """按优先级列出所有可能的 carbonui_cli 路径。"""
    exe = "carbonui_cli.exe" if os.name == "nt" else "carbonui_cli"
    candidates = []

    env_bin = os.environ.get(CLI_BIN_ENV)
    if env_bin:
        candidates.append(env_bin)

    # 部署目录：carbonui_cli.exe 与 agent-harness/ 同级（sync_agent_harness.bat 保证）
    candidates.append(os.path.join(_repo_root(), exe))

    # 源码检出的构建产物：多配置生成器在 Release/Debug 子目录，单配置在 cli/ 根
    build_base = os.path.join(_repo_root(), "out", "build", "tdd-tests", "cli")
    for sub in ("Release", "Debug", ""):
        candidates.append(os.path.join(build_base, sub, exe))

    return candidates


def find_cli_bin():
    """按定位顺序找到第一个存在的 carbonui_cli；找不到返回 None。"""
    for path in _candidate_paths():
        if path and os.path.isfile(path):
            return path
    return shutil.which("carbonui_cli")


def build_argv(action, payload=None):
    """纯函数：把 (action, payload) 映射成 CLI 的 argv。

    单测覆盖全部映射；底层 CLI 是标志式（``--flag value``），统一追加
    ``--json`` 并放在子命令之前。
    """
    payload = dict(payload or {})
    argv = ["--json", action]

    if action == "info":
        return argv

    if action == "snapshot":
        argv += ["--widget", payload["widget"]]
        if payload.get("shape"):
            argv += ["--shape", payload["shape"]]
        if payload.get("state"):
            argv += ["--state", payload["state"]]
        if payload.get("text") is not None:
            argv += ["--text", str(payload["text"])]
        if payload.get("width"):
            argv += ["--width", str(payload["width"])]
        if payload.get("height"):
            argv += ["--height", str(payload["height"])]
        if payload.get("out"):
            argv += ["--out", payload["out"]]
        return argv

    if action == "gallery":
        if payload.get("out"):
            argv += ["--out", payload["out"]]
        return argv

    if action == "qss":
        if payload.get("all"):
            argv += ["--all"]
        elif payload.get("widget"):
            argv += ["--widget", payload["widget"]]
            if payload.get("shape"):
                argv += ["--shape", payload["shape"]]
        # widget/all 都缺：原样传空参数，让 CLI 回用法错误信封（错误路径可测）
        return argv

    raise CliError("unknown action: %s" % action)


def _parse_envelope(stdout):
    """从 stdout 取最后一个能解析成 JSON 对象的行；没有则抛协议错误。"""
    for line in reversed((stdout or "").strip().splitlines()):
        line = line.strip()
        if not line.startswith("{"):
            continue
        try:
            envelope = json.loads(line)
        except ValueError:
            continue
        if isinstance(envelope, dict) and "ok" in envelope:
            return envelope
    raise CliProtocolError(
        "carbonui_cli stdout 没有合法信封（版本不匹配？）：%r" % (stdout[:200],))


def _run_subprocess(argv, timeout):
    """subprocess 封装（测试 mock 点）。"""
    return subprocess.run(
        argv, capture_output=True, timeout=timeout, check=False)


def run(action, payload=None, cli_bin=None, timeout=60.0):
    """执行一条 CLI 命令并返回信封 dict（默认不抛业务异常，便于 Agent 判定）。

    - CLI 起不来/超时：抛 :class:`CliError`；
    - stdout 无信封：抛 :class:`CliProtocolError`；
    - 业务失败：原样返回 ok=false 的信封。
    """
    binary = cli_bin or find_cli_bin()
    if not binary:
        raise CliError(
            "找不到 carbonui_cli：设 %s 或把它放到与 agent-harness 同级目录"
            % CLI_BIN_ENV)
    argv = [binary] + build_argv(action, payload)
    try:
        completed = _run_subprocess(argv, timeout)
    except subprocess.TimeoutExpired as exc:
        raise CliError("carbonui_cli 超时（%ss）：%s" % (timeout, action)) from exc
    except OSError as exc:
        raise CliError("无法启动 %s：%s" % (binary, exc)) from exc

    stdout_text = completed.stdout.decode("utf-8", "replace")
    # 退出码不在 {0,1} 说明进程压根没跑起来（缺运行时 DLL、被杀等），
    # 此时 stdout 必为空，别误报成"协议不匹配"，直接给出可行动的错误。
    if completed.returncode not in (0, 1) and not stdout_text.strip():
        stderr_text = completed.stderr.decode("utf-8", "replace").strip()
        raise CliError(
            "carbonui_cli 异常退出（code=%s，可能缺 Qt 运行时 DLL）：%s"
            % (completed.returncode, stderr_text[-200:]))

    envelope = _parse_envelope(stdout_text)
    envelope.setdefault("exitCode", completed.returncode)
    return envelope
