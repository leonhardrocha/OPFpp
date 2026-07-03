from pathlib import Path
from shutil import copy2

from setuptools import setup
from setuptools.command.build_py import build_py as _build_py


class build_py(_build_py):
    def run(self):
        super().run()

        root_dir = Path(__file__).resolve().parent
        source_bin_dir = root_dir / "bin"
        target_bin_dir = Path(self.build_lib) / "opfppy" / "bin"
        target_bin_dir.mkdir(parents=True, exist_ok=True)

        patterns = ("opfpy*.so", "opfpy*.pyd", "opfpy*.dylib")
        copied_any = False
        for pattern in patterns:
            for source in source_bin_dir.glob(pattern):
                copy2(source, target_bin_dir / source.name)
                copied_any = True

        if not copied_any:
            raise RuntimeError(
                "No compiled opfpy binary found in pythonlib/bin. "
                "Build opfpy before creating the wheel."
            )


setup(cmdclass={"build_py": build_py})
