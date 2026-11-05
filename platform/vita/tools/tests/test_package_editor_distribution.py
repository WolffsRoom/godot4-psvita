#!/usr/bin/env python3
"""Regression tests for portable Linux/macOS editor packaging."""

from pathlib import Path
import importlib.util
import tempfile
import unittest
import zipfile

SCRIPT = Path(__file__).resolve().parents[1] / "package_editor_distribution.py"
SPEC = importlib.util.spec_from_file_location("package_editor_distribution", SCRIPT)
MODULE = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(MODULE)


class EditorDistributionTest(unittest.TestCase):
    def test_native_editor_module_is_enabled_for_all_desktop_hosts(self) -> None:
        config_path = SCRIPT.parents[3] / "modules" / "vita_editor" / "config.py"
        spec = importlib.util.spec_from_file_location("vita_editor_config", config_path)
        config = importlib.util.module_from_spec(spec)
        assert spec.loader is not None
        spec.loader.exec_module(config)

        class Environment:
            editor_build = True

        for platform in ("windows", "linuxbsd", "macos"):
            self.assertTrue(config.can_build(Environment(), platform), platform)
        self.assertFalse(config.can_build(Environment(), "vita"))

    def make_fixture(self, root: Path) -> tuple[Path, Path]:
        payload = root / "payload"
        support = root / "support"
        payload.mkdir()
        support.mkdir()
        (support / "vita" / "templates").mkdir(parents=True)
        (support / "vita" / "templates" / "vita_release.zip").write_bytes(b"template")
        (support / "LEIA-ME.md").write_text("guide", encoding="utf-8")
        (support / "LICENSE-Godot.txt").write_text("license", encoding="utf-8")
        return payload, support

    def test_linux_archive_places_support_beside_executable(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            payload, support = self.make_fixture(root)
            binary = payload / "Godot4Vita"
            binary.write_bytes(b"linux-editor")
            destination = root / "release.zip"
            MODULE.package_distribution("linux", "x86_64", binary, support, destination)
            with zipfile.ZipFile(destination) as archive:
                names = set(archive.namelist())
                prefix = "Godot4Vita-v0.3-Linux-x86_64/"
                self.assertIn(prefix + "Godot4Vita", names)
                mode = archive.getinfo(prefix + "Godot4Vita").external_attr >> 16
                self.assertEqual(mode & 0o111, 0o111)
                self.assertIn(prefix + "vita/templates/vita_release.zip", names)
                self.assertIn(prefix + "LEIA-ME.md", names)

    def test_macos_archive_places_support_inside_app_macos_directory(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            payload, support = self.make_fixture(root)
            app = payload / "Godot4Vita.app"
            executable = app / "Contents" / "MacOS" / "Godot4Vita"
            executable.parent.mkdir(parents=True)
            executable.write_bytes(b"universal-editor")
            (app / "Contents" / "Info.plist").write_text("plist", encoding="utf-8")
            destination = root / "release.zip"
            MODULE.package_distribution("macos", "universal", app, support, destination)
            with zipfile.ZipFile(destination) as archive:
                names = set(archive.namelist())
                prefix = "Godot4Vita-v0.3-macOS-universal/Godot4Vita.app/Contents/"
                self.assertIn(prefix + "MacOS/Godot4Vita", names)
                self.assertIn(prefix + "MacOS/vita/templates/vita_release.zip", names)
                self.assertIn(prefix + "Resources/LEIA-ME.md", names)

    def test_rejects_missing_template(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            payload = root / "Godot4Vita"
            payload.write_bytes(b"editor")
            support = root / "support"
            support.mkdir()
            with self.assertRaisesRegex(FileNotFoundError, "vita_release.zip"):
                MODULE.package_distribution("linux", "x86_64", payload, support, root / "out.zip")


if __name__ == "__main__":
    unittest.main()
