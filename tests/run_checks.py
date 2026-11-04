"""Stable entry point for all host checks."""
import runpy
from pathlib import Path
runpy.run_path(str(Path(__file__).with_name("run_studio_checks611.py")), run_name="__main__")
