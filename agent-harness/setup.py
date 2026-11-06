# -*- coding: utf-8 -*-
"""CarbonUI 的 CLI-Anything harness 打包配置。

用法（在 agent-harness/ 目录里）::

    pip install -e .
    carbonui-agent info
"""

from setuptools import find_packages, setup

setup(
    name="cli-anything-carbonui",
    version="0.2.1",
    description="CLI-Anything harness for the carbonui_cli command line tool",
    packages=find_packages(include=["cli_anything", "cli_anything.*"]),
    python_requires=">=3.7",
    install_requires=[
        "click>=7.0",
    ],
    entry_points={
        "console_scripts": [
            "carbonui-agent=cli_anything.CarbonUI._cli:main",
        ],
    },
)
