# -*- coding: utf-8 -*-
"""端到端测试：真调 carbonui_cli。找不到二进制时整组 skip（CI 无构建产物时不红）。"""

import os
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))

from cli_anything.CarbonUI.utils import backend  # noqa: E402

_CLI_BIN = backend.find_cli_bin()


@unittest.skipUnless(_CLI_BIN, "carbonui_cli 不可用（未构建或不在定位路径）")
class EndToEndTest(unittest.TestCase):

    def test_info_envelope(self):
        envelope = backend.run("info", cli_bin=_CLI_BIN)
        self.assertTrue(envelope["ok"], envelope)
        self.assertEqual(0, envelope["code"])
        data = envelope["data"]
        self.assertTrue(data["version"])
        self.assertTrue(data["qtVersion"].startswith("5."))

    def test_snapshot_writes_real_png(self):
        out = os.path.join(tempfile.mkdtemp(prefix="carbonui-e2e-"), "button.png")
        envelope = backend.run("snapshot", {"widget": "button", "out": out},
                               cli_bin=_CLI_BIN)
        self.assertTrue(envelope["ok"], envelope)
        self.assertTrue(os.path.isfile(out))
        self.assertGreater(os.path.getsize(out), 0)

    def test_qss_button_contains_selector(self):
        envelope = backend.run("qss", {"widget": "button"}, cli_bin=_CLI_BIN)
        self.assertTrue(envelope["ok"], envelope)
        self.assertIn("QPushButton", envelope["data"]["qss"])

    def test_usage_error_exit_code_and_envelope(self):
        envelope = backend.run("qss", {}, cli_bin=_CLI_BIN)
        # 退出码细节在信封 exitCode（进程退出码只分 0/1）。
        self.assertFalse(envelope["ok"])
        self.assertEqual(1, envelope["code"])
        self.assertEqual(1, envelope["exitCode"])

    def test_click_entry_point_available(self):
        # Click 可导入即视为入口可用；真正跑 entry point 属部署验证。
        import click  # noqa: F401
        from cli_anything.CarbonUI import _cli  # noqa: F401

    def test_repl_over_real_cli(self):
        outputs = []
        script = iter(["info", "exit"])
        backend_run = backend.run

        import cli_anything.CarbonUI.utils.repl_skin as repl_skin
        with mock.patch.object(repl_skin, "run",
                               side_effect=lambda a, p, cli_bin=None: backend_run(a, p, cli_bin=_CLI_BIN)):
            repl_skin.repl(lambda: next(script, None), outputs.append, cli_bin=_CLI_BIN)
        self.assertTrue(any('"ok":true' in o or '"ok": true' in o for o in outputs))


if __name__ == "__main__":
    unittest.main()
