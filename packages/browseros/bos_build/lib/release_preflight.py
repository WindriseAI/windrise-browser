"""Release upload checks and the resource versions carried by each package."""

from pathlib import Path
from urllib.parse import urlsplit

from ..core.context import Context
from ..core.step import ValidationError
from ..products.resource_sources import source_resources_for_product
from ..release.components import normalize_component_version


def source_resource_manifest(ctx: Context):
    # Imported lazily: prepared_resources also uses the storage download step.
    from ..release.prepared_resources import (
        PreparedResourcesManifest,
        load_prepared_resources,
    )

    manifest = ctx.artifact_registry.get("prepared_resources")
    if not isinstance(manifest, PreparedResourcesManifest):
        prepared = ctx.prepared_resources
        if not isinstance(prepared, Path):
            raise ValueError("Source release metadata requires prepared resources")
        manifest = load_prepared_resources(prepared)
    if manifest.product != ctx.product.id:
        raise ValueError("Prepared-resource product does not match release metadata")
    if manifest.source_sha != ctx.source_sha:
        raise ValueError("Prepared-resource source SHA does not match release metadata")
    if manifest.browser_version != ctx.get_semantic_version():
        raise ValueError("Prepared-resource browser version does not match release metadata")
    return manifest


def bundled_resource_versions(ctx: Context) -> dict[str, str | None]:
    source = source_resources_for_product(ctx.product.id)
    if ctx.resource_mode == "source":
        versions = source_resource_manifest(ctx).component_versions
        return {
            source.server_component: versions.get(source.server_component),
            source.onboarding_component: versions.get(source.onboarding_component),
        }
    return {
        source.server_component: (
            ctx.env.browseros_server_resource_version
            if ctx.product.id == "browseros"
            else ctx.env.browserclaw_server_resource_version
        ),
        source.onboarding_component: ctx.env.onboarding_resource_version,
    }


def require_exact_resource_versions(ctx: Context) -> None:
    source = source_resources_for_product(ctx.product.id)
    names = {
        source.server_component: (
            "BROWSEROS_SERVER_RESOURCE_VERSION"
            if ctx.product.id == "browseros"
            else "BROWSERCLAW_SERVER_RESOURCE_VERSION"
        ),
        source.onboarding_component: "ONBOARDING_RESOURCE_VERSION",
    }
    for component, version in bundled_resource_versions(ctx).items():
        try:
            normalize_component_version(component, version or "")
        except ValueError:
            # Never echo the supplied value, including the normalizer's error.
            raise ValidationError(
                f"{names[component]} must specify an exact resource version"
            ) from None


def require_release_upload_config(ctx: Context) -> None:
    if ctx.build_type != "release":
        return
    value = ctx.env.r2_cdn_base_url
    try:
        parsed = urlsplit(value)
        valid = (
            bool(value)
            and not any(character.isspace() or ord(character) < 32 or ord(character) == 127 for character in value)
            and "\\" not in value
            and parsed.scheme == "https"
            and bool(parsed.hostname)
            and parsed.username is None
            and parsed.password is None
            and "?" not in value
            and "#" not in value
            and (parsed.port is None or 1 <= parsed.port <= 65535)
        )
    except ValueError:
        valid = False
    if not valid:
        raise ValidationError("R2_CDN_BASE_URL must specify an HTTPS base URL")
    if ctx.resource_mode == "published":
        require_exact_resource_versions(ctx)
