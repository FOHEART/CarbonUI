# -*- coding: utf-8 -*-
"""协议与映射的 mock 单测：不打真实 CLI，全靠 mock backend。"""

import os
import sys
import unittest
from unittest import mock

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))

from cli_anything.CarbonUI.utils import backend  # noqa: E402
from cli_anything.CarbonUI.utils import repl_skin  # noqa: E402


class BuildArgvTest(unittest.TestCase):

    def test_info_is_bare(self):
        self.assertEqual(
            ["--json", "info"], backend.build_argv("info"))

    def test_snapshot_full_mapping(self):
        argv = backend.build_argv("snapshot", {
            "widget": "button", "shape": "rect", "state": "disabled",
            "text": "取消", "width": 200, "height": 40, "out": "a b/c.png",
        })
        self.assertEqual(
            ["--json", "snapshot", "--widget", "button", "--shape", "rect",
             "--state", "disabled", "--text", "取消", "--width", "200",
             "--height", "40", "--out", "a b/c.png"],
            argv)

    def test_snapshot_minimal_defaults(self):
        argv = backend.build_argv("snapshot", {"widget": "radio"})
        self.assertEqual(
            ["--json", "snapshot", "--widget", "radio"], argv)

    def test_gallery_optional_out(self):
        self.assertEqual(["--json", "gallery"], backend.build_argv("gallery"))
        self.assertEqual(
            ["--json", "gallery", "--out", "g.png"],
            backend.build_argv("gallery", {"out": "g.png"}))

    def test_qss_all_vs_widget(self):
        self.assertEqual(
            ["--json", "qss", "--all"], backend.build_argv("qss", {"all": True}))
        self.assertEqual(
            ["--json", "qss", "--widget", "combo", "--shape", "left"],
            backend.build_argv("qss", {"widget": "combo", "shape": "left"}))

    def test_unknown_action_raises(self):
        with self.assertRaises(backend.CliError):
            backend.build_argv("deploy")


class EnvelopeParsingTest(unittest.TestCase):

    def test_last_json_line_wins(self):
        stdout = 'noise\n{"ok":true,"code":0,"data":{}}\n{"ok":false,"code":2,"data":{}}\n'
        envelope = backend._parse_envelope(stdout)
        self.assertFalse(envelope["ok"])
        self.assertEqual(2, envelope["code"])

    def test_garbage_raises_protocol_error(self):
        with self.assertRaises(backend.CliProtocolError):
            backend._parse_envelope("nothing json here")

    def test_repl_multi_line_takes_last(self):
        stdout = ('{"ok":true,"code":0,"data":{"json":true}}\n'
                  '{"ok":true,"code":0,"data":{"version":"0.2.1"}}\n')
        envelope = backend._parse_envelope(stdout)
        self.assertEqual("0.2.1", envelope["data"]["version"])


class RunTest(unittest.TestCase):

    def _fake_completed(self, stdout, returncode=0):
        completed = mock.Mock()
        completed.stdout = stdout.encode("utf-8")
        completed.returncode = returncode
        return completed

    def test_run_returns_business_failure_verbatim(self):
        envelope = {"ok": False, "code": 1, "message": "x", "data": {}}
        with mock.patch.object(backend, "find_cli_bin", return_value="cli.exe"), \
             mock.patch.object(backend, "_run_subprocess",
                               return_value=self._fake_completed('{"ok":false,"code":1,"message":"x","data":{}}\n', 1)):
            result = backend.run("qss", {"widget": "combo"})
        self.assertFalse(result["ok"])
        self.assertEqual(1, result["exitCode"])

    def test_run_without_binary_raises(self):
        with mock.patch.object(backend, "find_cli_bin", return_value=None):
            with self.assertRaises(backend.CliError):
                backend.run("info")


class ReplSkinTest(unittest.TestCase):

    def _repl(self, lines, cli_bin="fake"):
        """跑一个内存 REPL，返回 (输出列表, 每行调用的 argv 列表)。"""
        outputs = []
        argvs = []
        envelopes = iter([
            {"ok": True, "code": 0, "message": "ok", "data": {"version": "0.2.1"}},
            {"ok": True, "code": 0, "message": "ok", "data": {"json": True}},
        ])

        def fake_run(action, payload, cli_bin=None):
            argvs.append(backend.build_argv(action, payload))
            return next(envelopes)

        lines_iter = iter(lines)
        with mock.patch.object(repl_skin_backend, "run", side_effect=fake_run):
            repl_skin.repl(lambda: next(lines_iter, None), outputs.append, cli_bin=cli_bin)
        return outputs, argvs

    def test_bom_stripped_and_json_toggle(self):
        outputs, argvs = self._repl(["\ufeffinfo"])
        self.assertTrue(any("version" in o for o in outputs))
        self.assertEqual([["--json", "info"]], argvs)

    def test_exit_stops_loop(self):
        outputs, argvs = self._repl(["info", "exit", "info"])
        self.assertEqual(1, len(argvs))  # exit 之后不再执行


# repl_skin 里 `from .backend import run` 是按名字导入的，mock 点在 repl_skin 模块上。
from cli_anything.CarbonUI.utils import repl_skin as repl_skin_backend  # noqa: E402


if __name__ == "__main__":
    unittest.main()
