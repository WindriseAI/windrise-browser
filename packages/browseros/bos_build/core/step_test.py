#!/usr/bin/env python3
"""Tests for the step registry."""

import unittest

from bos_build.core import step as step_mod
from bos_build.core.step import (
    Step,
    phase_steps,
    step,
)


class RegistryContentTest(unittest.TestCase):

    def test_optional_steps_excluded_unless_requested(self):
        self.assertNotIn("series_patches", phase_steps("prep", "macos"))
        self.assertIn(
            "series_patches", phase_steps("prep", "macos", include_optional=True)
        )
        self.assertNotIn("merge_universal", phase_steps("build", "macos"))
        self.assertNotIn("mini_installer", phase_steps("sign", "windows"))

class RegistrationRulesTest(unittest.TestCase):
    def test_duplicate_name_raises(self):
        with self.assertRaisesRegex(ValueError, "Duplicate step name 'clean'"):

            @step("clean", phase="setup")
            class DuplicateClean(Step):
                pass

    def test_unknown_phase_raises(self):
        with self.assertRaisesRegex(ValueError, "Unknown phase 'deploy'"):
            step("deployer", phase="deploy")

    def test_registration_sets_metadata(self):
        try:

            @step(
                "temp_step_for_test",
                phase="prep",
                platforms=("linux",),
                env=("SOME_VAR",),
                optional=True,
            )
            class TempStep(Step):
                pass

            self.assertEqual(TempStep.name, "temp_step_for_test")
            self.assertEqual(TempStep.phase, "prep")
            self.assertEqual(TempStep.platforms, ("linux",))
            self.assertEqual(TempStep.env, ("SOME_VAR",))
            self.assertTrue(TempStep.optional)
            self.assertTrue(TempStep().applies_to("linux"))
            self.assertFalse(TempStep().applies_to("macos"))
        finally:
            step_mod._REGISTRY.pop("temp_step_for_test", None)


if __name__ == "__main__":
    unittest.main()
