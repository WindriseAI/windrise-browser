"""Regression tests for DS68 P1-3: Chrome & Chromium Edge lock classification.

    python packages/browseros/tools/windrise/windrise_importer_lock_1123_test.py
"""

import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
PATCHES = HERE.parents[1] / "chromium_patches"


def read_patch(rel: str) -> str:
    return (PATCHES / rel).read_text(encoding="utf-8")


class WindriseImporterLock1123Test(unittest.TestCase):
    def test_chrome_importer_utils_classifies_empty_locked_and_error(self):
        utils_h = read_patch(
            "chrome/utility/importer/browseros/chrome_importer_utils.h"
        )
        utils_cc = read_patch(
            "chrome/utility/importer/browseros/chrome_importer_utils.cc"
        )
        for token in (
            "enum class ImportItemStatus",
            "kSuccess,",
            "kEmpty,",
            "kLocked,",
            "kError,",
            "ReadFileToStringClassified(",
            "ClassifySqliteError(",
        ):
            self.assertIn(token, utils_h)

        self.assertIn("ERROR_SHARING_VIOLATION", utils_cc)
        self.assertIn("ERROR_LOCK_VIOLATION", utils_cc)
        self.assertIn("FILE_ERROR_IN_USE", utils_cc)
        self.assertIn("WAL sidecar locked", utils_cc)
        self.assertIn("primary == 5 || primary == 6", utils_cc)

    def test_cookie_and_all_importers_propagate_lock_and_support_network_cookies(self):
        cookies_cc = read_patch(
            "chrome/utility/importer/browseros/chrome_cookie_importer.cc"
        )
        self.assertIn('Append(FILE_PATH_LITERAL("Network"))', cookies_cc)
        self.assertIn('Append(FILE_PATH_LITERAL("Cookies"))', cookies_cc)
        self.assertIn("CopyDatabaseToTemp(cookies_path, &copy_status)", cookies_cc)
        self.assertIn("ClassifySqliteError(db.GetErrorCode())", cookies_cc)

        for rel in (
            "chrome/utility/importer/browseros/chrome_history_importer.cc",
            "chrome/utility/importer/browseros/chrome_password_importer.cc",
            "chrome/utility/importer/browseros/chrome_autofill_importer.cc",
            "chrome/utility/importer/browseros/chrome_bookmarks_importer.cc",
            "chrome/utility/importer/browseros/chrome_extensions_importer.cc",
        ):
            src = read_patch(rel)
            self.assertIn("ImportItemStatus", src, rel)

    def test_chrome_importer_never_emits_item_ended_on_lock_and_sends_structured_result(self):
        importer_cc = read_patch(
            "chrome/utility/importer/browseros/chrome_importer.cc"
        )
        self.assertIn("case browseros_importer::ImportItemStatus::kLocked:", importer_cc)
        self.assertIn('"partial_source_browser_locked"', importer_cc)
        self.assertIn('"source_browser_locked"', importer_cc)
        self.assertIn('"empty_source"', importer_cc)
        self.assertIn("bridge_->NotifyEndedWithResult(overall_succeeded, error_msg);", importer_cc)

        # Ensure locked branch does not invoke NotifyItemEnded.
        locked_branch = importer_cc.split(
            "case browseros_importer::ImportItemStatus::kLocked:"
        )[1].split("case browseros_importer::ImportItemStatus::kError:")[0]
        self.assertNotIn("NotifyItemEnded", locked_branch)

    def test_ipc_host_and_onboarding_propagate_lock_over_partial_success(self):
        bridge_h = read_patch("chrome/common/importer/importer_bridge.h")
        ext_bridge_cc = read_patch(
            "chrome/utility/importer/external_process_importer_bridge.cc"
        )
        in_bridge_cc = read_patch(
            "chrome/browser/importer/in_process_importer_bridge.cc"
        )
        client_cc = read_patch(
            "chrome/browser/importer/external_process_importer_client.cc"
        )
        host_cc = read_patch(
            "chrome/browser/importer/external_process_importer_host.cc"
        )
        observer_h = read_patch(
            "chrome/browser/importer/importer_progress_observer.h"
        )
        onboarding_cc = read_patch(
            "chrome/browser/browseros/onboarding/browseros_onboarding.cc"
        )

        self.assertIn("NotifyEndedWithResult(", bridge_h)
        self.assertIn("observer_->OnImportFinished(succeeded, error_msg);", ext_bridge_cc)
        self.assertIn("host_->NotifyImportEndedWithResult(succeeded, error_msg);", in_bridge_cc)
        self.assertIn("import_succeeded_ = succeeded;", client_cc)
        self.assertIn("import_error_msg_ = error_msg;", client_cc)
        self.assertIn("NotifyImportEndedWithResult(import_succeeded_,", client_cc)
        self.assertIn("observer_->ImportError(", host_cc)
        self.assertIn("virtual void ImportError(const std::string& error_msg) {}", observer_h)
        self.assertIn("void ImportError(const std::string& error_msg) override", onboarding_cc)
        self.assertIn('reported_import_error_ == "partial_source_browser_locked"', onboarding_cc)
        self.assertIn("if (!is_explicit_failure && import_did_succeed_)", onboarding_cc)
        # Windows fallback probe must not misclassify generic ACCESS_DENIED as lock.
        self.assertNotIn("FILE_ERROR_ACCESS_DENIED", onboarding_cc)

    def test_modern_chromium_edge_user_data_detected_and_legacy_edge_not_substituted(self):
        importer_list = read_patch("chrome/browser/importer/importer_list.cc")
        self.assertIn("GetChromiumEdgeUserDataFolder()", importer_list)
        self.assertIn('result.Append(FILE_PATH_LITERAL("Microsoft"))', importer_list)
        self.assertIn('result.Append(FILE_PATH_LITERAL("Edge"))', importer_list)
        self.assertIn('result.Append(FILE_PATH_LITERAL("User Data"))', importer_list)
        self.assertIn('result.Append("Application Support/Microsoft Edge")', importer_list)
        self.assertIn('result.Append("microsoft-edge")', importer_list)
        self.assertIn("DetectChromiumEdgeProfiles(&profiles)", importer_list)
        self.assertIn("DetectBuiltinWindowsProfiles(&profiles, !has_chromium_edge)", importer_list)
        self.assertIn('profile_path.Append(FILE_PATH_LITERAL("Network"))', importer_list)

        decryptor_mac = read_patch(
            "chrome/utility/importer/browseros/chrome_decryptor_mac.mm"
        )
        self.assertIn("Microsoft Edge Safe Storage", decryptor_mac)

    def test_state_machine_chrome_and_edge_normal_empty_locked_partial_and_retry(self):
        """Simulate ChromeImporter -> BrowserOSOnboardingHandler outcomes for Chrome & Edge."""

        def run_import(browser_name, item_statuses):
            completed = []
            succeeded = []
            empty = []
            locked = []
            failed = []
            for item, st in item_statuses.items():
                if st == "success":
                    succeeded.append(item)
                    completed.append(item)
                elif st == "empty":
                    empty.append(item)
                    completed.append(item)
                elif st == "locked":
                    locked.append(item)
                else:
                    failed.append(item)

            if locked:
                err = "partial_source_browser_locked" if succeeded else "source_browser_locked"
                overall = False
            elif failed:
                err = "partial_import_failed" if succeeded else "import_failed"
                overall = False
            elif not succeeded and empty:
                err = "empty_source"
                overall = True
            else:
                err = ""
                overall = True

            # BrowserOSOnboardingHandler::ImportEnded()
            is_lock = err in ("source_browser_locked", "partial_source_browser_locked")
            is_fail = is_lock or err in ("import_failed", "partial_import_failed")
            if is_lock:
                return {"status": "failed", "code": err, "browser": browser_name, "completed": completed}
            if not is_fail and bool(completed):
                return {"status": "succeeded", "code": err, "browser": browser_name, "completed": completed}
            return {"status": "failed", "code": err or "import_failed", "browser": browser_name, "completed": completed}

        for browser in ("Google Chrome", "Microsoft Edge"):
            # 1. Single locked item (e.g. Cookies while browser running) -> MUST fail, never succeed
            r_locked = run_import(browser, {"cookies": "locked"})
            self.assertEqual(r_locked["status"], "failed")
            self.assertEqual(r_locked["code"], "source_browser_locked")
            self.assertEqual(r_locked["completed"], [])

            # 2. Partial success + lock (Bookmarks readable, Cookies locked) -> MUST fail with partial lock
            r_partial = run_import(browser, {"bookmarks": "success", "cookies": "locked"})
            self.assertEqual(r_partial["status"], "failed")
            self.assertEqual(r_partial["code"], "partial_source_browser_locked")
            self.assertEqual(r_partial["completed"], ["bookmarks"])

            # 3. Clean empty profile -> succeeds without false lock error
            r_empty = run_import(browser, {"history": "empty", "cookies": "empty"})
            self.assertEqual(r_empty["status"], "succeeded")
            self.assertEqual(r_empty["code"], "empty_source")

            # 4. Retry after closing Chrome/Edge -> succeeds completely
            r_retry = run_import(browser, {"bookmarks": "success", "cookies": "success"})
            self.assertEqual(r_retry["status"], "succeeded")
            self.assertEqual(r_retry["code"], "")
            self.assertEqual(r_retry["completed"], ["bookmarks", "cookies"])


if __name__ == "__main__":
    unittest.main()
