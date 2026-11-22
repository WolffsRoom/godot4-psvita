"""Smoke-test the portable Linux editor assembly and archive tools."""
import hashlib
import importlib.util
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[4]
TOOLS = ROOT / "platform/vita/tools"


class LinuxEditorPackagingTest(unittest.TestCase):
    def test_module_is_enabled_for_both_desktop_platforms(self):
        config_path = ROOT / "modules/vita_editor/config.py"
        spec = importlib.util.spec_from_file_location("vita_editor_config", config_path)
        config = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(config)

        class BuildEnvironment:
            editor_build = True

        env = BuildEnvironment()
        self.assertTrue(config.can_build(env, "linuxbsd"))
        self.assertTrue(config.can_build(env, "windows"))
        self.assertFalse(config.can_build(env, "vita"))
        env.editor_build = False
        self.assertFalse(config.can_build(env, "linuxbsd"))

    def test_package_and_archive(self):
        with tempfile.TemporaryDirectory() as temporary:
            work = Path(temporary)
            binary = work / "Godot4Vita"
            binary.write_bytes(b"test editor executable")
            binary.chmod(0o755)
            template = work / "vita_release.zip"
            with zipfile.ZipFile(template, "w") as archive:
                archive.writestr("vita-template.json", "{}")
            output = work / "portable-editor"

            subprocess.run(
                [
                    sys.executable,
                    str(TOOLS / "package_linux_editor.py"),
                    "--binary",
                    str(binary),
                    "--template",
                    str(template),
                    "--output",
                    str(output),
                ],
                check=True,
                capture_output=True,
                text=True,
            )

            editor = output / "Godot4Vita"
            self.assertEqual(editor.read_bytes(), binary.read_bytes())
            self.assertTrue(editor.stat().st_mode & 0o111)
            self.assertTrue((output / "_sc_").is_file())
            self.assertTrue((output / "Exemplo/project.godot").is_file())
            self.assertTrue((output / "TROFEUS.md").is_file())
            self.assertEqual(
                (output / "vita/templates/vita_release.zip").read_bytes(),
                template.read_bytes(),
            )
            for name in ("platform.gd", "defaults_inspector.gd"):
                script = (output / "vita" / name).read_text(encoding="utf-8")
                self.assertNotIn("res://addons/psvita/", script)
                self.assertIn('path_join("vita/', script)

            subprocess.run(
                [
                    sys.executable,
                    str(TOOLS / "package_linux_editor.py"),
                    "--binary",
                    str(editor),
                    "--template",
                    str(template),
                    "--output",
                    str(output),
                ],
                check=True,
                capture_output=True,
                text=True,
            )

            cache = output / "Exemplo/.godot/imported"
            cache.mkdir(parents=True)
            (cache / "unused.ctex").write_bytes(b"cache")
            export_output = output / "Exports"
            export_output.mkdir()
            (export_output / "old.vpk").write_bytes(b"old export")

            tarball = work / "Godot-Vita.tar.gz"
            subprocess.run(
                [
                    sys.executable,
                    str(TOOLS / "archive_linux_editor.py"),
                    "--source",
                    str(output),
                    "--output",
                    str(tarball),
                ],
                check=True,
                capture_output=True,
                text=True,
            )
            with tarfile.open(tarball, "r:gz") as archive:
                member = archive.getmember("portable-editor/Godot4Vita")
                self.assertTrue(member.mode & 0o111)
                self.assertIn("portable-editor/vita/platform.gd", archive.getnames())
                self.assertIn("portable-editor/Exemplo/project.godot", archive.getnames())
                self.assertFalse(any("/.godot/" in name for name in archive.getnames()))
                self.assertFalse(any(name.startswith("portable-editor/Exports/") for name in archive.getnames()))
            digest = hashlib.sha256(tarball.read_bytes()).hexdigest()
            self.assertEqual(
                (work / "Godot-Vita.tar.gz.sha256").read_text(encoding="ascii").split()[0],
                digest,
            )


if __name__ == "__main__":
    unittest.main()
