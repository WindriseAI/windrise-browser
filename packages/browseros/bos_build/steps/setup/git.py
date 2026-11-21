#!/usr/bin/env python3
"""Git operations module for BrowserOS build system"""


from ...core.step import Step, ValidationError, step
from ...core.context import Context

from ..source.provision import prepare

BROWSEROS_BRANCH = "browseros"


@step("git_setup", phase="setup")
class GitSetupModule(Step):
    produces = []
    requires = []
    description = "Checkout Chromium version and sync dependencies"

    def validate(self, ctx: Context) -> None:
        if not ctx.chromium_src.exists():
            raise ValidationError(f"Chromium source not found: {ctx.chromium_src}")

        if not ctx.chromium_version:
            raise ValidationError("Chromium version not set")

    def execute(self, ctx: Context) -> None:
        # Build owns the checkout lock through packaging. This adapter must not
        # reacquire it or add destructive intent when --no-clean was selected.
        prepare(
            ctx.chromium_src,
            ctx.chromium_version,
            strategy="full",
            branch=BROWSEROS_BRANCH,
            patch_root=ctx.get_patch_root(),
        )


