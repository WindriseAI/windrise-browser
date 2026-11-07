"""GPT三先行验收，钩子17译文搬迁的消费行为；不靠搜索补丁字符串判断实现。"""
import importlib.util
from pathlib import Path
import unittest
import xml.etree.ElementTree as ET

MODULE = Path(__file__).with_name('xtb_rebrand.py')
spec = importlib.util.spec_from_file_location('xtb_rebrand', MODULE)
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)

SOURCE = '''<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE translationbundle>
<translationbundle lang="zh-CN">
  <translation id="100">关于 Chromium</translation>
  <translation id="101">重新启动 <ph name="PRODUCT"/>，保留 <ph name="COUNT"/> 个窗口</translation>
  <translation id="200">已经翻好的风桥文字</translation>
  <translation id="900">Chromium OS 版本</translation>
</translationbundle>'''

def nodes(text):
    root = ET.fromstring(text)
    return root, {n.attrib['id']: n for n in root.findall('translation')}

class XtbRebrand(unittest.TestCase):
    def test_old_id_backfill_is_real_translation_and_new_id(self):
        result = mod.rebrand_translations(SOURCE, [{'id': '201', 'legacy_ids': ['100']}], {'Chromium': 'Windrise Browser'})
        root, rows = nodes(result['xml'])
        self.assertEqual(root.attrib['lang'], 'zh-CN')
        self.assertEqual(rows['201'].text, '关于 Windrise Browser')
        self.assertEqual(result['missing'], [])
        self.assertEqual(result['translated'], 1)

    def test_placeholders_keep_structure_name_and_tail(self):
        result = mod.rebrand_translations(SOURCE, [{'id': '202', 'legacy_ids': ['101']}], {'Chromium': 'Windrise Browser'})
        _, rows = nodes(result['xml'])
        children = list(rows['202'])
        self.assertEqual([n.attrib for n in children], [{'name': 'PRODUCT'}, {'name': 'COUNT'}])
        self.assertEqual(children[0].tail, '，保留 ')
        self.assertEqual(children[1].tail, ' 个窗口')

    def test_existing_new_id_wins_and_repeated_run_is_idempotent(self):
        messages = [{'id': '200', 'legacy_ids': ['100']}, {'id': '203', 'legacy_ids': ['100']}]
        a = mod.rebrand_translations(SOURCE, messages, {'Chromium': 'Windrise Browser'})
        b = mod.rebrand_translations(a['xml'], messages, {'Chromium': 'Windrise Browser'})
        _, aa = nodes(a['xml']); _, bb = nodes(b['xml'])
        self.assertEqual(aa['200'].text, '已经翻好的风桥文字')
        self.assertEqual(ET.tostring(aa['203']), ET.tostring(bb['203']))
        self.assertEqual(len(ET.fromstring(b['xml']).findall("translation[@id='203']")), 1)
        self.assertEqual(b['missing'], [])

    def test_missing_translation_is_explicit_and_not_english_fallback(self):
        a = mod.rebrand_translations(SOURCE, [{'id': '204', 'legacy_ids': ['999']}], {'Chromium': 'Windrise Browser'})
        _, rows = nodes(a['xml'])
        self.assertEqual(a['missing'], ['204'])
        self.assertNotIn('204', rows)
        self.assertEqual(a['translated'], 0)

    def test_duplicate_ids_rejected_before_producing_output(self):
        bad = SOURCE.replace('</translationbundle>', '<translation id="100">冲突翻译</translation></translationbundle>')
        with self.assertRaises(ValueError):
            mod.rebrand_translations(bad, [{'id': '205', 'legacy_ids': ['100']}], {'Chromium': 'Windrise Browser'})

    def test_unrelated_entries_and_chromeos_are_not_blindly_rebranded(self):
        a = mod.rebrand_translations(SOURCE, [{'id': '206', 'legacy_ids': ['100']}], {'Chromium': 'Windrise Browser'})
        _, original = nodes(SOURCE); _, rows = nodes(a['xml'])
        self.assertEqual(ET.tostring(rows['900']), ET.tostring(original['900']))
        self.assertEqual(rows['100'].text, '关于 Chromium')

if __name__ == '__main__':
    unittest.main()
